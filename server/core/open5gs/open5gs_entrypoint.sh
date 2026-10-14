#! /bin/bash

# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI

export UE_GATEWAY_IP="${UE_IP_BASE}.1"
export UE_IP_RANGE="${UE_IP_BASE}.0/24"

INSTALL_ARCH=x86_64-linux-gnu
if [ "$(uname -m)" = "aarch64" ]; then
    INSTALL_ARCH="aarch64-linux-gnu"
fi
export INSTALL_ARCH

envsubst < open5gs-5gc.yml.in > open5gs-5gc.yml

# create dummy interfaces on localhost ip range for open5gs entities to bind to
for IP in {2..22}
do
    ip link add name lo$IP type dummy
    ip ad ad 127.0.0.$IP/24 dev lo$IP
    ip link set lo$IP up
done

# run webui
cd webui && npm run dev &

# run mongodb
mkdir -p /data/db && mongod --logpath /tmp/mongodb.log &

# wait for mongodb to be available, otherwise open5gs will not start correctly
while ! ( nc -zv $MONGODB_IP 27017 2>&1 >/dev/null )
do
    echo waiting for mongodb
    sleep 1
done

# setup ogstun and routing
python3 setup_tun.py --ip_range ${UE_IP_RANGE}
if [ $? -ne 0 ]
then
    echo "Failed to setup ogstun and routing"
    exit 1
fi

# Allow the Docker host to reach UEs using the core's reachable tunnel address.
# The bridge gateway is the host; restrict SNAT to its traffic to the UE subnet.
host_gateway_ip=$(ip -4 route show default | awk 'NR == 1 {print $3}')
if [ -z "$host_gateway_ip" ]; then
    echo "Cannot configure host-to-UE NAT: no default gateway"
    exit 1
fi
host_nat_rule=(-s "${host_gateway_ip}/32" -d "$UE_IP_RANGE" -o ogstun
    -m comment --comment robot-host-to-ue -j SNAT --to-source "$UE_GATEWAY_IP")
iptables-legacy -w -t nat -C POSTROUTING "${host_nat_rule[@]}" 2>/dev/null ||
    iptables-legacy -w -t nat -I POSTROUTING 1 "${host_nat_rule[@]}" || exit 1

# Add subscriber data to open5gs mongo db
echo "SUBSCRIBER_DB=${SUBSCRIBER_DB}"
python3 add_users.py --mongodb ${MONGODB_IP} --subscriber_data ${SUBSCRIBER_DB}
if [ $? -ne 0 ]
then
    echo "Failed to add subscribers to database"
    exit 1
fi

if $DEBUG
then
    exec stdbuf -o L gdb -batch -ex=run -ex=bt --args $@
else
    exec stdbuf -o L $@
fi

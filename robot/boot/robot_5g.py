#!/usr/bin/env python3
"""Connect the robot's modem and restore its IP, route and DNS after reconnects."""

import argparse
import ipaddress
import json
import os
import re
import socket
import subprocess
import time


def run(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT, timeout=45)


def fields(text):
    return {key.strip(): value.strip() for line in text.splitlines()
            if ":" in line for key, value in [line.split(":", 1)]}


def connected_bearer(modem):
    info = fields(run("mmcli", "-m", modem, "--output-keyvalue"))
    for key, path in info.items():
        if key.startswith("modem.generic.bearers.value["):
            bearer = fields(run("mmcli", "-b", path, "--output-keyvalue"))
            if (bearer.get("bearer.status.connected") == "yes" and
                    bearer.get("bearer.properties.apn") == "srsapn"):
                return bearer
    return None


def configuration(bearer):
    if bearer.get("bearer.ipv4-config.method") != "static":
        raise RuntimeError("Expected static IPv4 settings from the modem")
    address = str(ipaddress.IPv4Address(bearer["bearer.ipv4-config.address"]))
    gateway = str(ipaddress.IPv4Address(bearer["bearer.ipv4-config.gateway"]))
    prefix = int(bearer["bearer.ipv4-config.prefix"])
    if not 0 <= prefix <= 32:
        raise ValueError("Invalid modem subnet prefix")
    interface = bearer["bearer.status.interface"]
    if not re.fullmatch(r"[a-zA-Z0-9_.-]+", interface):
        raise ValueError("Invalid modem interface")
    dns = [str(ipaddress.ip_address(value)) for key, value in bearer.items()
           if key.startswith("bearer.ipv4-config.dns.value[")]
    mtu_text = bearer.get("bearer.ipv4-config.mtu", "1400")
    mtu = int(mtu_text) if mtu_text.isdigit() and int(mtu_text) else 1400
    if not 576 <= mtu <= 9000:
        raise ValueError("Invalid modem MTU")
    if not dns:
        raise ValueError("The modem supplied no DNS servers")
    return interface, address, prefix, gateway, tuple(dns), mtu


def configure(config):
    interface, address, prefix, gateway, dns, mtu = config
    run("ip", "link", "set", interface, "up")
    run("ip", "link", "set", interface, "mtu", str(mtu))
    addresses = json.loads(run("ip", "-j", "-4", "address", "show", "dev", interface))
    # Preserve an already working address during live adoption of this service.
    if not any(item.get("local") == address for device in addresses
               for item in device.get("addr_info", [])):
        run("ip", "address", "replace", f"{address}/{prefix}", "dev", interface)
    run("ip", "route", "replace", "default", "via", gateway, "dev", interface,
        "metric", "100")
    run("resolvectl", "dns", interface, *dns)
    run("resolvectl", "domain", interface, "~.")
    run("resolvectl", "default-route", interface, "yes")


def route_present(config):
    interface, address, _, gateway, _, _ = config
    routes = json.loads(run("ip", "-j", "-4", "route", "show", "default"))
    addresses = json.loads(run("ip", "-j", "-4", "address", "show", "dev", interface))
    return (any(r.get("dev") == interface and r.get("gateway") == gateway
                and r.get("metric") == 100 for r in routes) and
            any(a.get("local") == address for d in addresses for a in d.get("addr_info", [])))


def notify_ready(config):
    target = os.environ.get("NOTIFY_SOCKET")
    if target:
        if target.startswith("@"):
            target = "\0" + target[1:]
        with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as sock:
            sock.connect(target)
            sock.sendall(f"READY=1\nSTATUS=5G ready: {config[1]} on {config[0]}".encode())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--once", action="store_true")
    args = parser.parse_args()
    previous = None
    while True:
        try:
            listing = run("mmcli", "-L")
            line = next(line for line in listing.splitlines() if "Quectel" in line)
            modem = re.search(r"/org/freedesktop/ModemManager1/Modem/\d+", line).group()
            bearer = connected_bearer(modem)
            if bearer is None:
                run("mmcli", "-m", modem, "--enable")
                run("mmcli", "-m", modem, "--simple-connect=apn=srsapn,ip-type=ipv4")
                bearer = connected_bearer(modem)
            if bearer is None:
                raise RuntimeError("No connected srsapn bearer yet")
            config = configuration(bearer)
            if config != previous or not route_present(config):
                configure(config)
                print(f"5G ready: {config[1]} on {config[0]}, gateway {config[3]}", flush=True)
                previous = config
            notify_ready(config)
            if args.once:
                return
        except (subprocess.SubprocessError, OSError, ValueError, KeyError,
                StopIteration, AttributeError, RuntimeError) as error:
            print(f"Waiting for 5G: {error}", flush=True)
            previous = None
            if args.once:
                raise SystemExit(1)
        time.sleep(5)


if __name__ == "__main__":
    main()

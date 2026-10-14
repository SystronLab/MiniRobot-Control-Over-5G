#!/usr/bin/env bash
set -euo pipefail

# The UE sends its H.264 RTP stream to the core's ogstun address, port 5010.
# Use that network namespace while displaying the video on the local desktop.
if (( EUID != 0 )); then
    exec sudo --preserve-env=DISPLAY,XAUTHORITY,XDG_RUNTIME_DIR "$0" "$@"
fi

viewer_user=${SUDO_USER:-mohit}
viewer_uid=$(id -u "$viewer_user")
core_pid=$(docker inspect -f '{{.State.Pid}}' open5gs_5gc)
if [[ "$core_pid" == 0 ]]; then
    echo 'The Open5GS core container is not running.' >&2
    exit 1
fi

exec nsenter --target "$core_pid" --net \
    runuser -u "$viewer_user" -- env \
    DISPLAY="${DISPLAY:-:1}" \
    XAUTHORITY="${XAUTHORITY:-/run/user/$viewer_uid/gdm/Xauthority}" \
    XDG_RUNTIME_DIR="/run/user/$viewer_uid" \
    gst-launch-1.0 -v \
    udpsrc address=10.45.0.1 port=5010 \
    caps='application/x-rtp,media=video,clock-rate=90000,encoding-name=H264,payload=96' \
    ! rtpjitterbuffer latency=100 drop-on-latency=true \
    ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert \
    ! fpsdisplaysink video-sink=ximagesink text-overlay=false sync=false

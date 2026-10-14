#!/usr/bin/env bash
set -euo pipefail

# 1080p30 MJPEG capture with a 6 Mbps, low-latency H.264 encoding target.
# Keep sharpening mild and prevent exposure from lowering the frame rate.
v4l2-ctl -d /dev/v4l/by-id/usb-Arducam_Technology_Co.__Ltd._Arducam_USB_Camera_UC684-video-index0 --set-ctrl=sharpness=3,exposure_dynamic_framerate=0

# Decode and encode on separate pipeline threads; bound the frame queue.
exec nice -n 5 gst-launch-1.0 -e -v v4l2src device=/dev/v4l/by-id/usb-Arducam_Technology_Co.__Ltd._Arducam_USB_Camera_UC684-video-index0 do-timestamp=true '!' image/jpeg,width=1920,height=1080,framerate=30/1 '!' jpegparse '!' jpegdec '!' queue max-size-buffers=2 max-size-bytes=0 max-size-time=0 leaky=downstream '!' videoconvert '!' video/x-raw,format=I420 '!' x264enc tune=zerolatency speed-preset=ultrafast threads=2 bitrate=6000 key-int-max=30 vbv-buf-capacity=100 '!' h264parse '!' rtph264pay pt=96 mtu=1200 config-interval=-1 '!' udpsink host=10.45.0.1 port=5010 sync=false async=false

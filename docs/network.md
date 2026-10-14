# Network and troubleshooting

```text
Gamepad -> server -> Open5GS -> 5G -> robot UDP 9020 -> ROS cmd_vel
Viewer  <- Open5GS tunnel address 10.45.0.1:5010 <- robot camera
```

| Setting | Value |
| --- | --- |
| Core container | `open5gs_5gc`, `10.53.1.2` |
| Host on core bridge | `10.53.1.1` |
| UE subnet / gateway | `10.45.0.0/24` / `10.45.0.1` |
| Robot reserved IP / SSH user | `10.45.0.2` / `wheeltec` |
| APN / PLMN / TAC / slice SST | `srsapn` / `00101` / `7` / `1` |
| Control / video | UDP `9020` / RTP H.264 UDP `5010`, payload `96` |
| Radio | USRP X300 family, `192.168.40.2`, external clock and sync |
| Server radio NIC | `ens7f0`, `192.168.40.1/24`, MTU `9000` |
| Cell | Band 78, ARFCN 627340, 90 MHz, 30 kHz SCS, 2 TX / 2 RX |
| Modem | Quectel RM500Q-GL, managed by ModemManager |

The modem service reads its actual address, prefix, gateway, MTU, and DNS from the connected bearer. It installs a default route with metric 100. The core reserves `10.45.0.2` through subscriber provisioning.

The core advertises DNS `144.32.128.242` and `144.32.128.243`. These are site-specific; change `server/core/open5gs/open5gs-5gc.yml` if restoring on another network.

## Server checks

```bash
sudo docker ps --filter name=open5gs_5gc
ip route get 10.45.0.2
ping -c 3 10.45.0.2
ls -l /dev/input/by-id/*event-joystick
```

The UE route should use `10.53.1.2` with source `10.53.1.1`. After recreating the Docker network, run `sudo systemctl restart robot-ue-route.service`.

The camera viewer joins the core container's network namespace because `10.45.0.1:5010` is inside that container. A viewer listening only in the host network will miss this stream. Run the viewer from the local desktop session; it uses X11/XWayland display authorization.

## Robot checks

```bash
ssh wheeltec@10.45.0.2
systemctl status robot-demo.target robot-5g robot-base robot-control robot-camera
sudo journalctl -u robot-5g -u robot-control -u robot-camera -n 80 --no-pager
ip -4 address show wwan0
ip route
resolvectl status wwan0
curl --interface wwan0 -I --max-time 15 https://www.google.com
ls -l /dev/wheeltec_controller /dev/v4l/by-id/
```

An unavailable 5G network leaves the modem service retrying; the other services wait for it. A disconnected camera causes the camera service to retry. To stop the robot services, use `sudo systemctl stop robot-demo.target`.

## Fixes included

- UE internet NAT masquerades traffic leaving `ogstun`; it no longer adds 256 duplicate rules in the wrong direction.
- Host-to-UE traffic gets a specific route and SNAT to the core tunnel address. The SNAT rule is restricted to the Docker host's traffic.
- The core subscriber helper preserves the fixed UE IP when adding or updating the subscriber.
- Systemd supervises the robot processes and replaces the old one-shot `@reboot` job.
- The D-pad sends a refresh every 200 ms while held. The sender sends neutral on Ctrl-C/SIGTERM.
- The camera uses its stable USB device path, MJPEG capture, and a 6 Mbps H.264 encoder.

Control packets are unauthenticated UDP. This setup assumes the trusted lab network. The five-second timeout is a software fallback, not an emergency stop.

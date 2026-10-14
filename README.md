# MiniRobot Control Over 5G

Control a Wheeltec robot with a gamepad over 5G and watch its HD camera on the server.

- D-pad Up/Down moves forward/backward relative to the rear-mounted camera.
- Speed is 0.30 m/s. Analog steering is preserved.
- Camera streams 1080p at 30 fps with a 6 Mbps H.264 target.
- The robot starts its modem, controller, and camera automatically at boot.
- The server programs are started manually after reboot.

## Start the existing setup

Run these on the server, in order. Use separate terminals for the gNB, gamepad, and viewer. Do not launch another copy if one is already running.

**1. Core and route**

```bash
cd ~/Desktop/5gCore/ocudu_parent/ocudu/docker
sudo docker compose up -d --wait 5gc
sudo systemctl restart robot-ue-route.service
```

**2. gNB**

```bash
cd ~/Desktop/robot_ocudu/ocudu/build/apps/gnb
sudo ./gnb -c ./gnb.yml
```

Power on the robot and wait for it to connect. It can also be powered on first; its modem service retries until the network is ready.

**3. Gamepad**

```bash
cd ~/Desktop/robot_ocudu
./gamepad_interface -p 9020 -i 10.45.0.2 \
  -g usb-D_TGZ_Controller_3E5296D0-event-joystick
```

**4. Camera viewer**

```bash
cd ~/Desktop/robot_ocudu
./view_ue_camera.sh
```

Keep these three terminals open. Releasing the D-pad returns to the left stick, so center the stick to stop. Ctrl-C in the gamepad terminal sends a stop command. The robot also has a five-second command timeout.

## Files

| Folder | Contents |
| --- | --- |
| `server/` | Gamepad source and test, camera viewer, gNB configuration, UE route service |
| `server/core/` | Open5GS Docker build, configuration, subscriber helper, NAT fixes |
| `robot/boot/`, `robot/systemd/` | Automatic 5G, base, controller, and camera startup |
| `robot/scripts/` | Camera pipeline and startup wrappers |
| `robot/ros2/` | Working UDP-to-ROS controller, including its stop timeout |
| `robot/vendor-src/`, `robot/udev/` | Base driver, dependencies, active Mini Mec model, launch settings, device rules |

[Restore/build instructions](docs/restore.md) · [Network and troubleshooting](docs/network.md) · [Versions and file origins](docs/provenance.md)

SIM credentials and SSH passwords are excluded. Fill in `server/core/open5gs/open5gs.env.example` locally when restoring. The existing machines already have their settings.

The robot startup services and 5G internet were verified after a power cycle on 14 October 2026. The connection later dropped and recovered, with RF underflows reported by the gNB. A full server cold-boot test and a clean-machine rebuild have not been performed.

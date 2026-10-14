# Restore the setup

These instructions target the same hardware and existing Ubuntu/ROS environment. The README has the everyday startup commands. Stop the relevant programs before replacing their files. Keep the robot stationary while restoring its controller.

Clone this repository on either machine:

```bash
git clone https://github.com/SystronLab/MiniRobot-Control-Over-5G.git
cd MiniRobot-Control-Over-5G
```

## Server

Install Docker Engine with Compose v2, a C compiler, Python 3, and GStreamer. Typical Ubuntu packages for the gamepad/viewer are:

```bash
sudo apt install build-essential python3 gstreamer1.0-tools \
  gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-x
make -C server
make -C server test
```

The test uses synthetic input and loopback UDP; it does not command the robot.

For the existing directory layout, copy the rebuilt sender and viewer:

```bash
cp server/gamepad_interface server/view_ue_camera.sh ~/Desktop/robot_ocudu/
cp server/gnb.yml ~/Desktop/robot_ocudu/ocudu/build/apps/gnb/gnb.yml
sudo install -m 644 server/robot-ue-route.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable robot-ue-route.service
```

If the controller device name differs, use the name from `/dev/input/by-id/`. The desktop user must have read permission for that input device.

### Core

On the existing server, the real environment file is already at `~/Desktop/5gCore/ocudu_parent/ocudu/docker/open5gs/open5gs.env`; preserve it. Copying the template over it would erase the local SIM settings.

For a restored checkout, prepare a local file:

```bash
cp server/core/open5gs/open5gs.env.example server/core/open5gs/open5gs.env
chmod 600 server/core/open5gs/open5gs.env
```

Edit the `YOUR_*` fields to match the physical SIM. The inline subscriber format is:

```text
IMSI,KEY,opc,OPC,AMF,9,10.45.0.2
```

Use `op` instead of `opc` only when supplying OP. The helper provisions APN `srsapn`. Keep the reserved IP at `10.45.0.2` for these commands. The real environment file is ignored by Git.

Use the repo's core directory instead of the original core directory when starting the restored stack:

```bash
cd server/core
sudo docker compose build 5gc
sudo docker compose up -d --wait 5gc
sudo systemctl restart robot-ue-route.service
```

Both copies use container name `open5gs_5gc` and project name `docker`; run one copy only. The core image build needs internet access. Use Compose `up` after environment changes so the container is recreated with them; a plain `docker restart` retains the old container environment. Reopen the viewer after recreating the core container.

### Radio software

The existing machine already has UHD and OCUDU built. For another machine, use the exact revisions in [provenance.md](provenance.md), install their documented build dependencies, and build UHD before OCUDU. The local reference build used Release mode with UHD enabled:

```bash
git clone https://github.com/EttusResearch/uhd.git
git -C uhd checkout 07a7a92ad6e09cc7e84aae5990aff563a4546e83
cmake -S uhd/host -B uhd/host/build -DCMAKE_BUILD_TYPE=Release
cmake --build uhd/host/build -j2
sudo cmake --install uhd/host/build
sudo ldconfig

git clone https://gitlab.com/ocudu/ocudu.git
git -C ocudu checkout 45b51f9a278c2f21905055aac63a91e63f519c23
cmake -S ocudu -B ocudu/build -DCMAKE_BUILD_TYPE=Release -DENABLE_UHD=ON
cmake --build ocudu/build --target gnb -j2
sudo ./ocudu/build/apps/gnb/gnb -c ./server/gnb.yml
```

Provide matching UHD FPGA images, the USRP connection at `192.168.40.2`, external clock/sync, and a server NIC at `192.168.40.1/24` with MTU 9000. Configure those NIC settings persistently in NetworkManager. The current NIC is `ens7f0`; another machine may use a different name. The RIC is not required for this control/video setup.

## Robot

Use the Wheeltec Jetson Ubuntu 22.04 image with ROS 2 Humble, its device drivers, and the existing `/home/wheeltec/wheeltec_ros2` and `/home/wheeltec/yolo_ws` workspaces. A complete Jetson OS image is not part of this repository.

Required camera/modem packages include:

```bash
sudo apt install modemmanager v4l-utils python3-colcon-common-extensions \
  gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly
```

`systemd-resolved` and `systemd-timesyncd` must be available and active. The system resolver must use resolved; on the current robot `/etc/resolv.conf` ultimately points to its `127.0.0.53` stub.

From the repository root, install the controller package as user `wheeltec`:

```bash
sudo systemctl stop robot-demo.target
mkdir -p ~/Projects/OranDemos/Demo1/ROS/src
cp -a robot/ros2/demo1_5g_robot_controller ~/Projects/OranDemos/Demo1/ROS/src/
source /opt/ros/humble/setup.bash
cd ~/Projects/OranDemos/Demo1/ROS/src
colcon build --packages-select demo1_5g_robot_controller
```

Return to the repository root, then install the scripts and units:

```bash
cp robot/scripts/*.sh ~/Projects/OranDemos/Demo1/
sudo install -m 755 robot/boot/* /usr/local/sbin/
sudo install -m 644 robot/systemd/* /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable ModemManager systemd-resolved systemd-timesyncd
sudo systemctl enable robot-demo.target
```

If restoring an older robot setup, remove only this obsolete line with `sudo crontab -e` to avoid duplicate controllers:

```text
@reboot /home/wheeltec/Projects/OranDemos/Demo1/oran_demo1_startup.sh
```

The existing robot already has the base driver and rules installed. If those need restoring, copy the saved `robot/vendor-src/` packages into the corresponding locations in `~/wheeltec_ros2/src/`, retaining only one copy of each ROS package. `serial_ros2` and `ackermann_msgs-ros2` originally live under `src/depend/`. Keep the existing `wheeltec_robot_urdf` package and its Mini Mec model; that backup is still pending.

Then use the vendor workspace dependencies (including ROS robot localization, Madgwick IMU filter, navigation messages/common, and state publishers), rebuild with `colcon build --packages-up-to turn_on_wheeltec_robot`, and source its `install/setup.bash`. The existing model package provides the active Mini Mec configuration. Install the controller udev rules if missing:

```bash
sudo install -m 644 robot/udev/wheeltec_controller*.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
```

Reconnect the USB controller or reboot for the device rule to apply. The camera script uses `/dev/v4l/by-id/usb-Arducam_Technology_Co.__Ltd._Arducam_USB_Camera_UC684-video-index0`; update that path if the camera is replaced. `wheeltec` needs membership in the `video` group.

With the server core and gNB running, start the robot services:

```bash
sudo systemctl start robot-demo.target
systemctl status robot-demo.target robot-5g robot-base robot-control robot-camera
```

The units restart failed processes and wait for 5G readiness. They do not launch the server programs. See [network.md](network.md) for connectivity and video checks.

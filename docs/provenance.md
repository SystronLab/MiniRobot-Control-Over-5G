# Versions and file origins

Snapshot collected from the working server and robot on 14 October 2026.

| Component | Source / version |
| --- | --- |
| OCUDU | `https://gitlab.com/ocudu/ocudu.git`, commit `45b51f9a278c2f21905055aac63a91e63f519c23` |
| UHD | `https://github.com/EttusResearch/uhd.git`, commit `07a7a92ad6e09cc7e84aae5990aff563a4546e83`, built library 4.9.0 |
| Open5GS | Dockerfile pins `v2.7.6`, Ubuntu 22.04 base |
| Robot | Ubuntu 22.04 on Jetson, ROS 2 Humble, Wheeltec vendor workspace |

The full OCUDU/UHD sources, compiled binaries, logs, packet captures, ROS build outputs, unrelated navigation models, and credentials are not included. Upstream source revisions are pinned above. The robot's existing Jetson/ROS environment remains a prerequisite.

## Server originals

- Gamepad, header, test, viewer: `/home/mohit/Desktop/robot_ocudu/`.
- gNB configuration: `/home/mohit/Desktop/robot_ocudu/ocudu/build/apps/gnb/gnb.yml`.
- Core files: `/home/mohit/Desktop/5gCore/ocudu_parent/ocudu/docker/open5gs/`.
- Route service: `/etc/systemd/system/robot-ue-route.service`.

The saved Compose file contains the active core service and its network. The unused containerized gNB/metrics sections and unused subscriber CSV mount were removed. Project name `docker` preserves the existing naming convention. No live Compose file was changed during export.

The environment file is a template with SIM identity, key, OPc/OP, and AMF replaced by placeholders. The actual subscriber file stays on the server. Upstream example keys in `add_users.py` are examples, not the live SIM credentials.

## Robot originals

- Boot scripts: `/usr/local/sbin/robot_*`.
- Units: `/etc/systemd/system/robot-*`.
- Camera and wrappers: `/home/wheeltec/Projects/OranDemos/Demo1/`.
- Controller package: `.../Demo1/ROS/src/demo1_5g_robot_controller/`.
- **Controller implementation:** copied from the installed package under `.../Demo1/ROS/src/install/demo1_5g_robot_controller/lib/python3.10/site-packages/`. This is the running implementation with the five-second timeout; the old source copy lacked it.
- Base source: `/home/wheeltec/wheeltec_ros2/src/`. Launch and configuration files were overlaid from the installed package to match runtime settings.
- Mini Mec URDF and all its referenced meshes: copied from the installed `wheeltec_robot_urdf` package. Package/build metadata and RViz files come from its source package. Other vehicle models are omitted.
- Device rules: `/etc/udev/rules.d/`.

The wrapper scripts retain the existing `/home/wheeltec` workspace paths and source `yolo_ws` and `wheeltec_ros2`, as on the robot. The old root cron entry has already been removed on the live robot.

## Attribution

Existing author headers and package metadata are retained. The core files come from OCUDU under BSD-3-Clause-Open-MPI; the license text is in `LICENSES/`. The serial dependency retains its Apache-2.0 declaration and author notices; the license text is included in `LICENSES/`. Other vendor packages retain their original license declarations, including any unresolved declarations. This snapshot does not assign a new license to third-party files.

#!/usr/bin/env bash
set -e
source /opt/ros/humble/setup.bash
source /home/wheeltec/yolo_ws/install/setup.bash
source /home/wheeltec/wheeltec_ros2/install/setup.bash
exec /opt/ros/humble/bin/ros2 launch turn_on_wheeltec_robot turn_on_wheeltec_robot.launch.py

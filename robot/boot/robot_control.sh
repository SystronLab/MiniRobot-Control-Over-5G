#!/usr/bin/env bash
set -e
source /opt/ros/humble/setup.bash
source /home/wheeltec/yolo_ws/install/setup.bash
source /home/wheeltec/wheeltec_ros2/install/setup.bash
source /home/wheeltec/Projects/OranDemos/Demo1/ROS/src/install/setup.bash
exec /opt/ros/humble/bin/ros2 run demo1_5g_robot_controller robot_controller

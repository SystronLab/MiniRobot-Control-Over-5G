#!/usr/bin/env python3
"""Request neutral motion locally before stopping the controller service."""
import json
import socket
import time

packet = json.dumps({
    "ButtonA": 0, "ButtonB": 0, "ButtonX": 0, "ButtonY": 0,
    "Left_X": "+00000", "Left_Y": "+00000",
    "Right_X": "+00000", "Right_Y": "+00000",
}, separators=(",", ":")).encode()
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
    for _ in range(3):
        sock.sendto(packet, ("127.0.0.1", 9020))
        time.sleep(0.05)

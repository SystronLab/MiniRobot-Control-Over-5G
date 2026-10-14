#!/usr/bin/env python3
"""Exercise the real sender using synthetic evdev data and loopback UDP only."""

import json
import os
from pathlib import Path
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import time


def main():
    binary = str(Path(sys.argv[1] if len(sys.argv) > 1 else "gamepad_interface").resolve())
    with tempfile.TemporaryDirectory() as directory, socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as receiver:
        receiver.bind(("127.0.0.1", 0))
        receiver.settimeout(1)
        fifo = Path(directory) / "events"
        os.mkfifo(fifo)
        process = subprocess.Popen(
            [binary, "-i", "127.0.0.1", "-p", str(receiver.getsockname()[1]), "-g", str(fifo)],
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
        )
        writer = None
        try:
            deadline = time.monotonic() + 3
            while writer is None:
                try:
                    writer = os.open(fifo, os.O_WRONLY | os.O_NONBLOCK)
                except OSError:
                    if process.poll() is not None or time.monotonic() > deadline:
                        raise RuntimeError("Sender did not open the synthetic input")
                    time.sleep(0.01)

            def event(code, value):
                os.write(writer, struct.pack("llHHi", 0, 0, 3, code, value))

            def expect(x, y):
                packet = receiver.recv(1024)
                assert len(packet) <= 140, len(packet)
                data = json.loads(packet)
                assert (int(data["Left_X"]), int(data["Left_Y"])) == (x, y), data
                return data

            event(17, -1)  # Up -> chassis reverse, toward the rear-facing camera.
            expect(0, 32767)
            expect(0, 32767)  # Holding refreshes the UE's command watchdog.
            expect(0, 32767)
            event(17, 0)
            expect(0, 0)
            event(17, 1)  # Down -> chassis forward, away from the camera.
            expect(0, -32767)
            event(17, 0)
            expect(0, 0)
            event(1, -8000)  # Existing stick control is preserved.
            expect(0, -8000)
            event(17, 1)
            expect(0, -32767)
            event(1, -4000)  # Stick updates cannot override a held D-pad.
            expect(0, -32767)
            event(17, 0)
            expect(0, -4000)  # Releasing restores the latest stick position.
            event(1, 0)
            expect(0, 0)
            event(0, -9000)
            expect(-9000, 0)
            process.send_signal(signal.SIGINT)
            expect(0, 0)  # Exiting sends a neutral command.
            _, errors = process.communicate(timeout=3)
            assert process.returncode == 0, errors.decode()
            assert not errors, errors.decode()
            print("PASS: Up/Down, hold heartbeat, release, stick handoff, steering, stop on exit, JSON size")
        finally:
            if writer is not None:
                os.close(writer)
            if process.poll() is None:
                process.terminate()
                process.communicate(timeout=3)


if __name__ == "__main__":
    main()

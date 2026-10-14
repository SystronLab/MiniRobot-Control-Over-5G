CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror

.PHONY: all test clean
all: gamepad_interface

gamepad_interface: gamepad_interface.c 5g_demo1_consts.h
	$(CC) $(CFLAGS) gamepad_interface.c -o $@

test: gamepad_interface
	python3 test_gamepad_dpad.py ./gamepad_interface

clean:
	rm -f gamepad_interface

/*
 ============================================================================
 Filename    : gamepad_interface.c

 Author(s)   : A Moulds

 Version     : Dev: 0.05

 Copyright   : University of York

 Description :

 Revisions:

 Notes:
 Port Number: specified for the robot (RosBot) UE. The robot listens on this port.

 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <linux/input.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include "5g_demo1_consts.h"
#include <limits.h>
#include <stdint.h>
#include <time.h>


#define DEFAULT_IP_ADDR "127.0.0.1"
#define DEMO_SEND_INTERVAL 3 // secs
//#define GAMEPAD_DRIVER_DEVICE_NAME "usb-D_TGZ_Controller_3E529610-event-joystick"
//#define GAMEPAD_DRIVER_DEVICE_NAME "usb-D_TGZ_Controller_3E5296C0-event-joystick"
//#define GAMEPAD_DRIVER_DEVICE_NAME "usb-D_TGZ_Controller_3E529630-event-joystick"
#define GAMEPAD_DRIVER_DEVICE_NAME "usb-D_TGZ_Controller_3E529680-event-joystick"
#define GAMEPAD_DRIVER_LOC "/dev/input/by-id/"
#define DPAD_AXIS_VALUE 32767 // Full-stick speed: approximately 0.30 m/s.
#define DPAD_REPEAT_MS 200


typedef struct
{
    int button_A_down;
    int button_B_down;
    int button_X_down;
    int button_Y_down;
    int stick_left_x;
    int stick_left_y;
    int stick_right_x;
    int stick_right_y;
} gamepadStruct, *gamepadStructptr;

int gamepad = -1;
int sockfd = -1;
char gamepadDevNode[PATH_MAX];
const char *gamepadDevname = GAMEPAD_DRIVER_DEVICE_NAME;
const char *gamepadDevLoc = GAMEPAD_DRIVER_LOC;
char gamepadDataJson[GAMEPAD_JSON_PKTSIZE + 1];

static int64_t monotonicMillis(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

/*
*
*/
void writeGamepadDataJson(gamepadStructptr gamepad)
{
	snprintf(gamepadDataJson, sizeof(gamepadDataJson),
	    "{\n\t\"ButtonA\":%d,\n\t\"ButtonB\":%d,\n\t\"ButtonX\":%d,\n\t\"ButtonY\":%d,"
	    "\n\t\"Left_X\":\"%+06d\",\n\t\"Left_Y\":\"%+06d\","
	    "\n\t\"Right_X\":\"%+06d\",\n\t\"Right_Y\":\"%+06d\"\n}",
	    gamepad->button_A_down, gamepad->button_B_down,
	    gamepad->button_X_down, gamepad->button_Y_down,
	    gamepad->stick_left_x, gamepad->stick_left_y,
	    gamepad->stick_right_x, gamepad->stick_right_y);
}


/*
*
*/
void printGamepadDataJson(void)
{
	if(isatty(STDOUT_FILENO)) fputs("\033[2J\033[H", stdout);
	puts("*** Gamepad Interface - Test Program ***");
	printf("%s\n",gamepadDataJson);
}


/*
*
*/
void sigHandler(int sigNo)
{
   if(sigNo == SIGINT || sigNo == SIGTERM)
   {
	   static const char stopPacket[] =
	       "{\"ButtonA\":0,\"ButtonB\":0,\"ButtonX\":0,\"ButtonY\":0,"
	       "\"Left_X\":\"+00000\",\"Left_Y\":\"+00000\","
	       "\"Right_X\":\"+00000\",\"Right_Y\":\"+00000\"}";
	   if(sockfd >= 0) {
	       send(sockfd, stopPacket, sizeof(stopPacket) - 1, 0);
	       close(sockfd);
	   }
	   if(gamepad >= 0)
		   close(gamepad);
	   _exit(EXIT_SUCCESS);
   }
}


/* ==================================================================================== */
int main(int argc, char **argv)
{

	int robot_port_number = DEFAULT_PORT_NO;
	char robot_ip_addr_str[16];

	int test_loop_count = 0;

	struct sockaddr_in robot_addr;

	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);

	// Get Options //
	int opt;
	char mode[5] = "norm";
	strcpy(robot_ip_addr_str,DEFAULT_IP_ADDR);
	while((opt = getopt(argc, argv, "hi:p:m:g:")) != -1)
	{
	   switch (opt)
	   {
	     case 'h':
	    	 printf("Usage: gamepad_test [OPTION]\n");
	    	 printf("Sends gamepad control signals to robot over 5G network.\n\n");
	    	 printf("  -i: robot ip address - format xxx.xxx.xxx.xxx\n");
	    	 printf("  -p: robot port number integer\n\n");
		 printf("  -h: help\n");
		 printf("  -g: gamepad device node\n");
		 printf("  -m: set mode to norm (default) or demo\n");
	    	 printf("Example:\ngamepad_test -i 192.100.0.1 -p 99700\n\n");
	    	 exit(EXIT_SUCCESS);
	    	 break;
		 case 'm':
	         strncpy(mode,optarg,4);	
			 if(strncmp(mode, "demo",4) && strncmp(mode,"norm",4))
			 	strcpy(mode,"norm");
			break;
		case 'g':
	    	 gamepadDevname = optarg;
	         break;
	     case 'i':
	    	 strcpy(robot_ip_addr_str, optarg);
	         break;
         case 'p':
	         robot_port_number = atoi(optarg);
	         break;
	     default:
	    	 printf("error: unknown argument!\n");
	    	 exit(EXIT_FAILURE);
	   }
   }


   	bzero(&robot_addr, sizeof(struct sockaddr_in));
	robot_addr.sin_family = AF_INET;
	robot_addr.sin_addr.s_addr = inet_addr(robot_ip_addr_str);
	robot_addr.sin_port = htons(robot_port_number);

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
	    printf("Error while creating the socket\n");
	    exit(EXIT_FAILURE);
	}

	if(connect(sockfd, (struct sockaddr *) &robot_addr, sizeof(robot_addr)) < 0)
	{
		printf("error: socket connection failed!\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}


	puts("*** Gamepad Interface for 5G ORAN Demo1 **");

	if(!strncmp(mode,"norm",4))
	{
		int pathLength = snprintf(gamepadDevNode, sizeof(gamepadDevNode), "%s%s",
		    gamepadDevname[0] == '/' ? "" : gamepadDevLoc, gamepadDevname);
		if(pathLength < 0 || (size_t)pathLength >= sizeof(gamepadDevNode)) {
		    fprintf(stderr, "Gamepad device path is too long.\n");
		    goto FINISH;
		}
		gamepad = open(gamepadDevNode, O_RDONLY | O_NONBLOCK);
		if(gamepad == -1)
		{
			printf("error: unable to open gamepad device!\n");
			goto FINISH;
		}
	}

    gamepadStruct gamepadVal = {0};
    gamepadStruct gamepadValOld = {0};
    int analogLeftY = 0;
    int dpadY = 0;
    int64_t lastDpadSend = monotonicMillis();

    while(1)
    {
		if(!strncmp(mode,"norm",4))
		{
			struct input_event events[8]; // 8 events per frame is pretty extreme, more like 1 or 2, sometimes as high as 4
			int r1 = read(gamepad, events, sizeof events);
			int repeatDpad = dpadY != 0 &&
			    monotonicMillis() - lastDpadSend >= DPAD_REPEAT_MS;
			if (r1 > 0 || repeatDpad)
			{
				int new_event_count = r1 > 0 ? r1/sizeof(struct input_event) : 0;
				for (int evi=0; evi<new_event_count; evi++)
				{
					struct input_event ev = events[evi];
					switch (ev.type)
					{
						case EV_ABS:
						{
							switch (ev.code)
							{
								case ABS_X: { gamepadVal.stick_left_x = ev.value; } break;
								case ABS_Y: {
								    analogLeftY = ev.value;
								    if(dpadY == 0) gamepadVal.stick_left_y = analogLeftY;
								} break;
								case ABS_HAT0Y: {
								    dpadY = (ev.value > 0) - (ev.value < 0);
								    gamepadVal.stick_left_y = dpadY != 0 ?
								        -dpadY * DPAD_AXIS_VALUE : analogLeftY; // Camera faces the rear.
								} break;
								case ABS_RX: { gamepadVal.stick_right_x = ev.value; } break;
								case ABS_RY: { gamepadVal.stick_right_y = ev.value; } break;
							}
						} break;
						case EV_KEY:
						{
							switch (ev.code)
							{
								case BTN_A: { gamepadVal.button_A_down = ev.value; } break;
								case BTN_B: { gamepadVal.button_B_down = ev.value; } break;
								case BTN_X: { gamepadVal.button_X_down = ev.value; } break;
								case BTN_Y: { gamepadVal.button_Y_down = ev.value; } break;

							}
						} break;
					}
				}
				// Test if any changes in gamepad axis values or button states //
				if((repeatDpad && dpadY != 0) ||
						gamepadValOld.button_A_down != gamepadVal.button_A_down ||
						gamepadValOld.button_B_down != gamepadVal.button_B_down ||
						gamepadValOld.button_X_down != gamepadVal.button_X_down ||
						gamepadValOld.button_Y_down != gamepadVal.button_Y_down ||
						gamepadValOld.stick_left_x != gamepadVal.stick_left_x ||
						gamepadValOld.stick_left_y != gamepadVal.stick_left_y ||
						gamepadValOld.stick_right_x != gamepadVal.stick_right_x ||
						gamepadValOld.stick_right_y != gamepadVal.stick_right_y)
				{
					writeGamepadDataJson(&gamepadVal);
					printGamepadDataJson();
					if((sendto(sockfd, 
						gamepadDataJson, 
							strlen(gamepadDataJson),
							0, // flags
						NULL,
							0)) < 0)
					{
						printf("error: unable to send data!n");
						perror("- ");
					}
					lastDpadSend = monotonicMillis();
				}
			}

			gamepadValOld = gamepadVal;
			usleep(1000);
		}
		else if(!strncmp(mode,"demo",4))
		{
			gamepadVal.button_A_down = 0;
			gamepadVal.button_B_down = 0;
			gamepadVal.button_X_down = 0;
			gamepadVal.button_Y_down = 0;
			gamepadVal.stick_left_x = 0;
			gamepadVal.stick_left_y = 0;
			gamepadVal.stick_right_x = 0;
			gamepadVal.stick_right_y = 0;

			sleep(DEMO_SEND_INTERVAL);
			writeGamepadDataJson(&gamepadVal);
			printGamepadDataJson();
			if((sendto(sockfd, gamepadDataJson, strlen(gamepadDataJson), 0,
				(struct sockaddr *) &robot_addr,
				sizeof(robot_addr))) < (ssize_t)strlen(gamepadDataJson))
			{
				printf("error: unable to send data!n");
				perror("- ");
			}
			test_loop_count++;
			printf("loop count = %d\n", test_loop_count);
		}

    }

FINISH:
	if(gamepad >= 0)
		close(gamepad);
	close(sockfd);
	return EXIT_SUCCESS;
}

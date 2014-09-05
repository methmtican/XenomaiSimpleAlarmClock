//============================================================================
// Name        : SimpleAlarmClock.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdlib.h>
#include <native/task.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <termios.h>
#include <signal.h>

#include "AlarmController.h"

RT_TASK obd2_task;
RT_TASK disp_task;

void run_main( void* arg )
{
	rt_task_set_periodic( NULL, TM_NOW, 1e9 );

	while( 1 )
	{
		rt_task_wait_period(NULL);

		((AlarmController*)arg)->run();
	}
}

void run_io( void* arg )
{
	AlarmController* controller = (AlarmController*)arg;

	// Disable echoing back to the terminal
	termios config;
	tcgetattr( STDIN_FILENO, &config );
	config.c_lflag &= ~ECHO;
	config.c_lflag &= ~ICANON;
	config.c_cc[VMIN]  = 1;
	config.c_cc[VTIME] = 0;
	tcsetattr( STDIN_FILENO, TCSANOW, &config );

	while( true )
	{
		fd_set fds;
		FD_ZERO( &fds );
		FD_SET( 0, &fds );
		if( select( 1, &fds, 0, 0, 0 ) )
		{
			char c = getchar();
			switch( c )
			{
			case 'a':
				FD_ZERO( &fds );
				FD_SET( 0, &fds );
				if( select( 1, &fds, 0, 0, 0 ) )
				{
					int id = getchar() - '0';
					if( id >= 0 && id < 4 )
						controller -> setMode( AlarmController::SET_ALARM, id );
					else
					{
						printf( "Invalid Timer Number!" );
						fflush( stdout );
					}

				}
				break;
			case 't':
				controller -> setMode( AlarmController::SET_TIME );
				break;
			case 'h':
				controller -> incrementHour();
				break;
			case 'm':
				controller -> incrementMin();
				break;
			case 27: //esc
				controller -> setMode( AlarmController::NORMAL );
				break;
			case 'e':
				FD_ZERO( &fds );
				FD_SET( 0, &fds );
				if( select( 1, &fds, 0, 0, 0 ) )
				{
					int id = getchar() - '0';
					if( 0 <= id && id < 4 )
						controller -> toggleAlarm( id );
					else
					{
						printf( " Invalid timer number!" );
						fflush( stdout );
					}
				}
				break;
			case 'c':
				controller -> unsetAlarm();
				break;
			case 'q':
				return;
			default:
				printf( "Bad Selection!" );
				fflush( stdout );
				break;
			}
		}
	}
}

void catch_signal(int sig)
{
	//printf( "Caught signal( %i )\n", sig );
	//fflush( stdin );
	// Renable echoing back to the terminal
	termios config;
	tcgetattr( STDIN_FILENO, &config );
	config.c_lflag |= ECHO;
	config.c_lflag |= ICANON;
	//config.c_cc[VMIN]  = 1;
	//config.c_cc[VTIME] = 0;
	tcsetattr( STDIN_FILENO, TCSANOW, &config );
}

int main() {

	signal(SIGTERM, catch_signal);
	signal(SIGINT, catch_signal);

	/* Avoids memory swapping for this program */
	mlockall(MCL_CURRENT|MCL_FUTURE);

	/*
	 * Arguments: &task,
	 *      name,
	 *      stack size (0=default),
	 *      priority,
	 *      mode (FPU, start suspended, ...)
	 */
	rt_task_create(&obd2_task, "main", 0, 99, 0);
	rt_task_create(&disp_task,   "io",   0, 98, 0);


	AlarmController controller;

	/*
	 * Arguments: &task,
	 *      task function,
	 *      function argument
	 */
	rt_task_start(&obd2_task, &run_main, (void*)(&controller) );
	rt_task_start(&disp_task,   &run_io,   (void*)(&controller) );

	pause();

	printf( "\n" );

	rt_task_delete(&obd2_task);
	rt_task_delete(&disp_task);

	return 0;
}


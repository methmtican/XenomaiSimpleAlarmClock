/*
 * AlarmController.cxx
 *
 *  Created on: Jul 12, 2014
 *      Author: methmtican
 */

#include "AlarmController.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int Time::id = -1;
Time::Time()
{
	id++;
	hour   = 0;
	minute = 0;
	second = 0;

	sprintf( name, "alarm %i", id );

	char buff[15];
	sprintf( buff, "MUTEX %s", name );
	int err = rt_mutex_create( &lock, buff );
	if( err )
		printf( "Error(%i) creating %s\n", err, buff );

	enable  = false;
	buzzing = false;
}

Time::~Time()
{
	rt_mutex_delete( &lock );
}

const char* Time::getName() const
{
	return name;
}

void Time::incrementHour()
{
	rt_mutex_acquire( &lock, TM_INFINITE );
	hour = ( hour + 1 ) % 24;
	rt_mutex_release( &lock );
}

void Time::incrementMin()
{
	rt_mutex_acquire( &lock, TM_INFINITE );
	minute = ( minute + 1 ) % 60;
	rt_mutex_release( &lock );
}

void Time::incrementSecond()
{
	rt_mutex_acquire( &lock, TM_INFINITE );
	second = ( second + 1 ) % 60;
	rt_mutex_release( &lock );
}

void Time::setTime( int _hour, int _minute, int _second )
{
	rt_mutex_acquire( &lock, TM_INFINITE );
	hour = _hour % 24;
	minute = _minute % 60;
	second = _second % 60;
	rt_mutex_release( &lock );
}

int Time::readHour()
{
	int _hour;

	rt_mutex_acquire( &lock, TM_INFINITE );
	_hour = hour;
	rt_mutex_release( &lock );

	return _hour;
}

int Time::readMinute()
{
	int min;

	rt_mutex_acquire( &lock, TM_INFINITE );
	min = minute;
	rt_mutex_release( &lock );

	return min;
}

int Time::readSecond()
{
	int sec;

	rt_mutex_acquire( &lock, TM_INFINITE );
	sec = second;
	rt_mutex_release( &lock );

	return sec;
}

void Time::toggle()
{
	rt_mutex_acquire( &lock, TM_INFINITE );
	enable = !enable;
	rt_mutex_release( &lock );
}

void Time::setBuzzing( bool en )
{
	rt_mutex_acquire( &lock, TM_INFINITE );
	buzzing = en;
	rt_mutex_release( &lock );
}

bool Time::isEnabled()
{
	bool en;

	rt_mutex_acquire( &lock, TM_INFINITE );
	en = enable;
	rt_mutex_release( &lock );

	return en;
}

bool Time::isBuzzing()
{
	bool en;

	rt_mutex_acquire( &lock, TM_INFINITE );
	en = buzzing;
	rt_mutex_release( &lock );

	return en;
}

void Time::fromTM( tm* _tm )
{
	if( !_tm )
		return;

	rt_mutex_acquire( &lock, TM_INFINITE );

	hour   = _tm -> tm_hour;
	minute = _tm -> tm_min;
	second = _tm -> tm_sec;

	rt_mutex_release( &lock );
}

void Time::toTM( tm* _tm )
{
	if( !_tm )
		return;

	rt_mutex_acquire( &lock, TM_INFINITE );

	_tm -> tm_hour = hour;
	_tm -> tm_min  = minute;
	_tm -> tm_sec  = second;

	rt_mutex_release( &lock );
}

bool Time::operator==(  Time& other )
{
	bool equal;

	rt_mutex_acquire( &lock, TM_INFINITE );

	equal = hour   == other.readHour() &&
			minute == other.readMinute() &&
			second == other.readSecond();

	rt_mutex_release( &lock );

	return equal;
}

AlarmController::AlarmController()
{
	mode = NORMAL;
	alarm_select = 0;

	time_t now = time(0);
    tm* now_tm = localtime( &now );

    curr.setTime( now_tm -> tm_hour,
    			  now_tm -> tm_min,
    			  now_tm -> tm_sec );

    printf( "Simple Alarm Clock\n" );
    printf( "==============================================\n" );
    printf( "Options:\n" );
    printf( "  a[0-3] - set alarm mode\n" );
    printf( "  t      - set time mode\n" );
    printf( "  esc    - when in set alarm or set time mode, return to normal mode\n" );
    printf( "  h      - when in set alarm or set time mode, increment hour\n" );
    printf( "  m      - when in set alarm or set time mode, increment minute\n" );
    printf( "  e[0-3] - enable alarm [1-4]\n" );
    printf( "  c      - clear all alarms\n" );
    printf( "\n" );
}

void AlarmController::setMode( Mode _mode, int id )
{
	mode = _mode;
	alarm_select = id;
	redisplay();
}

void AlarmController::unsetAlarm()
{
	for( int i=0; i<4; i++ )
		alarm[i].setBuzzing( false );
	redisplay();
}

void AlarmController::toggleAlarm( int alarm_id )
{
	alarm[alarm_id].toggle();
	redisplay();
}

void AlarmController::incrementHour()
{
	switch( mode )
	{
	case SET_ALARM:
		alarm[alarm_select].incrementHour();
		break;
	case SET_TIME:
		curr.incrementHour();
		break;
	case NORMAL:
		// do nothing
		break;
	}

	redisplay();
}

void AlarmController::incrementMin()
{
	switch( mode )
	{
	case SET_ALARM:
		alarm[alarm_select].incrementMin();
		break;
	case SET_TIME:
		curr.incrementMin();
		break;
	case NORMAL:
		// do nothing
		break;
	}

	redisplay();
}

void AlarmController::redisplay()
{
	//time_t now = time(NULL);
	tm now_tm;// = localtime( &now );

	char post[60];
	post[0] = 0;

	switch( mode )
	{
	case SET_ALARM:
		alarm[alarm_select].toTM( &now_tm );
		sprintf( post, " - Set Alarm %i", alarm_select );
		break;
	case SET_TIME:
		sprintf( post, " - Set Time" );
		curr.toTM( &now_tm );
		break;
	case NORMAL:
		curr.toTM( &now_tm );
		for( int i=0; i<4; i++ )
		{
			if( alarm[i].isEnabled() && alarm[i].isBuzzing() )
				sprintf( post, "%s - %s is buzzing", post, alarm[i].getName() );
		}
		break;
	}

	// display the current time or alarm time if in set mode
	char clear[90];
	memset( clear, ' ', 90 );
	clear[89]=0;

	char buff[30];
	strftime( buff, 30, "%r", &now_tm );

	printf( "\r   %s\r", clear );
	fflush( stdout );
	printf( "   %s%s\r", buff, post );
	fflush( stdout );
}

/**
 * Called by the main task periodically at a rate of 1hz.
 *
 * This updates the time and checks the alarms.
 */
void AlarmController::run()
{
	int hour = curr.readHour();
	int min  = curr.readMinute();
	int sec  = curr.readSecond();

	// increment the time
	sec += 1;
	if( sec == 60 )
	{
		sec = 0;
		min += 1;

		if( min == 60 )
		{
			min = 0;
			hour += 1;

			if( hour == 24 )
				hour = 0;
		}
	}

	curr.setTime( hour, min, sec );

	// check for alarms that should go off now
	for( int i=0; i<4; i++ )
	{
		if( alarm[i].isEnabled() && curr == alarm[i] )
			alarm[i].setBuzzing( true );
	}

	redisplay();
}




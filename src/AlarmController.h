/*
 * AlarmController.h
 *
 *  Created on: Jul 12, 2014
 *      Author: methmtican
 */

#ifndef ALARMCONTROLLER_H_
#define ALARMCONTROLLER_H_

#include <native/mutex.h>
#include <native/timer.h>
#include <time.h>

class Time
{
	int hour;
	int minute;
	int second;
	bool enable;
	bool buzzing;
	RT_MUTEX lock;
	char name[8];

	static int id;
public:

	Time();
	~Time();

	const char* getName() const;

	void incrementHour();
	void incrementMin();
	void incrementSecond();

	void setTime( int hour, int minute, int second );

	int readHour() ;
	int readMinute() ;
	int readSecond() ;

	void toggle();
	void setBuzzing( bool en );
	bool isEnabled() ;
	bool isBuzzing() ;

	void fromTM( tm* _tm );
	void toTM( tm* _tm ) ;

	bool operator==(  Time& other ) ;
};

class AlarmController
{
public:
	enum Mode { SET_ALARM, SET_TIME, NORMAL };

private:
	Mode mode;
	int alarm_select;

	Time alarm[4];
	Time curr;
	Time base;

	const static RTIME SECONDS_TO_NS = 1e9;

public:

	AlarmController();

    void setMode( Mode _mode, int id=-1 );
	void unsetAlarm();
	void toggleAlarm( int alarm_id );

	void incrementHour();
	void incrementMin();

	void redisplay();
	void run();
};

#endif /* ALARMCONTROLLER_H_ */

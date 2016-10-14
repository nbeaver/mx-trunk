/*
 * Name:    mx_sleep.c
 *
 * Purpose: Defines the interface to millisecond and microsecond sleep
 *          functions.  The time actually slept is greater than or equal
 *          to the time requested.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005-2007, 2011, 2015-2016
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_util.h"

#define USE_SIMPLE_MX_SLEEP	1001
#define USE_NANOSLEEP_MX_SLEEP	1002
#define USE_USLEEP_MX_SLEEP	1003
#define USE_SELECT_MX_SLEEP     1004
#define USE_WIN32_MX_SLEEP	1005
#define USE_PCBIOS_MX_SLEEP	1006
#define USE_VMS_MX_SLEEP	1007


#if defined(OS_MSDOS)
#  define MX_SLEEP_TYPE        USE_PCBIOS_MX_SLEEP

#elif defined(OS_VMS)

#  define MX_SLEEP_TYPE        USE_VMS_MX_SLEEP

#elif defined(OS_WIN32)

#  define MX_SLEEP_TYPE        USE_WIN32_MX_SLEEP

#elif defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_IRIX) \
	|| defined(OS_HPUX) || defined(OS_BSD) || defined(OS_QNX) \
	|| defined(OS_TRU64) || defined(OS_RTEMS) || defined(OS_ECOS) \
	|| defined(OS_HURD) || defined(OS_ANDROID) || defined(OS_MINIX)

#  define MX_SLEEP_TYPE        USE_NANOSLEEP_MX_SLEEP

#elif defined(OS_MACOSX) || defined(OS_CYGWIN)

#  define MX_SLEEP_TYPE        USE_USLEEP_MX_SLEEP

#elif defined(OS_SUNOS4) || defined(OS_UNIXWARE) || defined(OS_VXWORKS)

#     define MX_SLEEP_TYPE     USE_SELECT_MX_SLEEP
#else
#  error "Subsecond sleep functions have not been defined!"
#endif

/* ======================================================================== */

#if (MX_SLEEP_TYPE == USE_SIMPLE_MX_SLEEP)

#include "mx_unistd.h"

/* These versions do not work well enough to be used for routine operation
 * and should only be used for testing in the absence of real sub-second
 * timer functions.
 */

MX_EXPORT void
mx_sleep( unsigned long seconds )
{
	sleep( (unsigned int) seconds );
}

MX_EXPORT void
mx_msleep( unsigned long milliseconds )
{
	unsigned int seconds;

	seconds = (unsigned int) (( milliseconds + 500L ) / 1000L);

	sleep( seconds );
}

MX_EXPORT void
mx_usleep( unsigned long microseconds )
{
	unsigned int seconds;

	seconds = (unsigned int) (( microseconds + 500000L ) / 1000000L);

	sleep( seconds );
}

#endif /* USE_SIMPLE_MX_SLEEP */

/* ======================================================================== */

#if (MX_SLEEP_TYPE == USE_NANOSLEEP_MX_SLEEP)

/* nanosleep() is a POSIX.1b function (formerly POSIX.4). */

#include <time.h>

#if defined(OS_BSD)
#include <sys/time.h>
#endif

MX_EXPORT void
mx_sleep( unsigned long seconds )
{
	struct timespec sleep_time;

	sleep_time.tv_sec = (time_t) seconds;
	sleep_time.tv_nsec = 0L;

	(void) nanosleep( &sleep_time, NULL );
}

MX_EXPORT void
mx_msleep( unsigned long milliseconds )
{
	struct timespec sleep_time;
	int seconds;

	seconds = (int) ( milliseconds / 1000L );

	milliseconds -= (seconds * 1000L);

	sleep_time.tv_sec = (time_t) seconds;
	sleep_time.tv_nsec = (long) (1000000L * milliseconds);

	(void) nanosleep( &sleep_time, NULL );
}

MX_EXPORT void
mx_usleep( unsigned long microseconds )
{
	struct timespec sleep_time;
	int seconds;

	seconds = (int) ( microseconds / 1000000L );

	microseconds -= (seconds * 1000000L);

	sleep_time.tv_sec = (time_t) seconds;
	sleep_time.tv_nsec = (long) (1000L * microseconds);

	(void) nanosleep( &sleep_time, NULL );
}

#endif /* USE_NANOSLEEP_MX_SLEEP */

/* ======================================================================== */

#if (MX_SLEEP_TYPE == USE_USLEEP_MX_SLEEP)

/* usleep() is derived from 4.3BSD. */

#include <unistd.h>

MX_EXPORT void
mx_sleep( unsigned long seconds )
{
	u_int sleep_microseconds;

	sleep_microseconds = (u_int) ( 1000000L * seconds );

	usleep( sleep_microseconds );
}

MX_EXPORT void
mx_msleep( unsigned long milliseconds )
{
	u_int sleep_microseconds;

	sleep_microseconds = (u_int) ( 1000L * milliseconds );

	usleep( sleep_microseconds );
}

MX_EXPORT void
mx_usleep( unsigned long microseconds )
{
	u_int sleep_microseconds;

	sleep_microseconds = (u_int) microseconds;

	usleep( sleep_microseconds );
}

#endif /* USE_USLEEP_MX_SLEEP */

/* ======================================================================== */

#if (MX_SLEEP_TYPE == USE_SELECT_MX_SLEEP)

#include "mx_select.h"

MX_EXPORT void
mx_sleep( unsigned long seconds )
{
	struct timeval sleep_time;

	sleep_time.tv_sec = seconds;
	sleep_time.tv_usec = 0;

	(void) select( 0, NULL, NULL, NULL, &sleep_time );
}

MX_EXPORT void
mx_msleep( unsigned long milliseconds )
{
	struct timeval sleep_time;
	unsigned long seconds;

	seconds = milliseconds / 1000L;

	milliseconds -= (seconds * 1000L);

	sleep_time.tv_sec = seconds;
	sleep_time.tv_usec = (1000L * milliseconds);

	(void) select( 0, NULL, NULL, NULL, &sleep_time );
}

MX_EXPORT void
mx_usleep( unsigned long microseconds )
{
	struct timeval sleep_time;
	unsigned long seconds;

	seconds = microseconds / 1000000L;

	microseconds -= (seconds * 1000000L);

	sleep_time.tv_sec = seconds;
	sleep_time.tv_usec = microseconds;

	(void) select( 0, NULL, NULL, NULL, &sleep_time );
}

#endif /* USE_SELECT_MX_SLEEP */

/* ======================================================================== */

#if (MX_SLEEP_TYPE == USE_WIN32_MX_SLEEP)

#include <windows.h>

MX_EXPORT void
mx_sleep( unsigned long seconds )
{
	DWORD milliseconds;

	milliseconds = (DWORD) ( 1000L * seconds );

	Sleep( milliseconds );
}

MX_EXPORT void
mx_msleep( unsigned long milliseconds )
{
	Sleep( (DWORD) milliseconds );
}

MX_EXPORT void
mx_usleep( unsigned long microseconds )
{
	DWORD milliseconds;

	milliseconds = (DWORD) ( microseconds / 1000L );

	Sleep( milliseconds );
}


#endif /* USE_WIN32_MX_SLEEP */

/* ======================================================================== */

#if (MX_SLEEP_TYPE == USE_PCBIOS_MX_SLEEP)

/* On a PC, the clock timer normally ticks at a rate of 18.2 ticks per second.
 * If we use these clock ticks for our sub-second sleeps, this means that
 * the minimum time resolution available is 1/18.2 = 0.0549451 seconds.
 */

#include <bios.h>

/* Assuming the standard PC clock tick rate. */

#define TICKS_PER_MICROSECOND	(0.0000182)
#define TICKS_PER_MILLISECOND	(0.0182)
#define TICKS_PER_DAY		(1572480L)

static unsigned long
get_current_pc_timer_tick( int *midnight_has_passed )
{
	unsigned int midnight_passed_flag;
	unsigned long timer_tick_count;

	midnight_passed_flag
		= _bios_timeofday( _TIME_GETCLOCK, &timer_tick_count );

	if ( *midnight_has_passed == FALSE ) {
		if ( midnight_passed_flag ) {
			*midnight_has_passed = TRUE;

			timer_tick_count += TICKS_PER_DAY;
		}
	} else {
		timer_tick_count += TICKS_PER_DAY;
	}

	return timer_tick_count;
}

MX_EXPORT void
mx_sleep( unsigned long seconds )
{
	mx_msleep( seconds * 1000L );
}

MX_EXPORT void
mx_msleep( unsigned long milliseconds )
{
	long starting_tick, ticks_to_go, ending_tick;
	int midnight_has_passed;

	midnight_has_passed = FALSE;

	starting_tick = get_current_pc_timer_tick(&midnight_has_passed);

	/* Always round up to the next tick. */

	ticks_to_go = (unsigned long) ( 0.999
			+ TICKS_PER_MILLISECOND * (double) milliseconds );

	ending_tick = starting_tick + ticks_to_go;

	while ( get_current_pc_timer_tick(&midnight_has_passed) < ending_tick )
		;	/* Busy wait. */
}

MX_EXPORT void
mx_usleep( unsigned long microseconds )
{
	long starting_tick, ticks_to_go, ending_tick;
	int midnight_has_passed;

	midnight_has_passed = FALSE;

	starting_tick = get_current_pc_timer_tick(&midnight_has_passed);

	/* Always round up to the next tick. */

	ticks_to_go = (unsigned long) ( 0.999
			+ TICKS_PER_MICROSECOND * (double) microseconds );

	ending_tick = starting_tick + ticks_to_go;

	while ( get_current_pc_timer_tick(&midnight_has_passed) < ending_tick )
		;	/* Busy wait. */
}

#endif /* USE_PCBIOS_MX_SLEEP */

/* ======================================================================== */

#if (MX_SLEEP_TYPE == USE_VMS_MX_SLEEP)

#include <lib$routines.h>
#include <libwaitdef.h>

MX_EXPORT void
mx_sleep( unsigned long seconds )
{
	float wait_seconds;

#ifdef __ia64
	int float_type = LIB$K_IEEE_S;
#else
	int float_type = LIB$K_VAX_F;
#endif

	wait_seconds = (float) seconds;

	lib$wait( &wait_seconds, 0, &float_type );
}

MX_EXPORT void
mx_msleep( unsigned long milliseconds )
{
	float wait_seconds;

#ifdef __ia64
	int float_type = LIB$K_IEEE_S;
#else
	int float_type = LIB$K_VAX_F;
#endif

	wait_seconds = 1.0e-3 * (float) milliseconds;

	lib$wait( &wait_seconds, 0, &float_type );
}

MX_EXPORT void
mx_usleep( unsigned long microseconds )
{
	float wait_seconds;

#ifdef __ia64
	int float_type = LIB$K_IEEE_S;
#else
	int float_type = LIB$K_VAX_F;
#endif

	wait_seconds = 1.0e-6 * (float) microseconds;

	lib$wait( &wait_seconds, 0, &float_type );
}

#endif /* USE_VMS_MX_SLEEP */


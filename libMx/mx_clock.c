/*
 * Name:    mx_clock.c
 *
 * Purpose: MX time keeping functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2009, 2011, 2015-2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_CLOCK_DEBUG				FALSE

#define MX_CLOCK_DEBUG_CLOCK_RESOLUTION		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

#if !defined(OS_WIN32) && !defined(OS_ECOS)
#include <sys/times.h>
#endif

#if defined(OS_SUNOS4)
#include <sys/time.h>
#endif

#if defined(OS_WIN32)
#include <windows.h>
#endif

#if defined(OS_VXWORKS)
#include <drv/timer/timerDev.h>
#endif

#if defined(OS_RTEMS)
#include <rtems.h>
#endif

#if defined(OS_ECOS) || defined(OS_ANDROID)
#define USE_POSIX_CLOCKS
#endif

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_clock.h"

/* The most recent value of the system clock tick value for this process
 * is kept in the variable mx_most_recent_clock_tick_value.
 */

static MX_CLOCK_TICK mx_most_recent_clock_tick_value;

/* mx_clock_tick_divisor contains 1 + the maximum possible range
 * of values that the low order part of the MX_CLOCK_TICK structure
 * can have converted to a double.  You can use this value to convert
 * a clock tick value represented as a double into the individual
 * high order and low order words of the MX_CLOCK_TICK structure.
 */

static double mx_clock_tick_divisor;

#if defined(OS_SUNOS4)

static unsigned long mx_cpu_start_tick;

static int mx_cpu_start_tick_is_initialized = FALSE;

#endif

#if defined(USE_POSIX_CLOCKS)

#define TWO_TO_THE_31ST_POWER	(2147483648.0)

static double posix_clock_ticks_per_second = -1.0;
static double posix_clock_ticks_per_nsec   = -1.0;

#endif

/*-----------------------------------------------------------*/

static unsigned long
mx_current_cpu_tick( void )
{
#if MX_CLOCK_DEBUG
	static const char fname[] = "mx_current_cpu_tick()";
#endif

	unsigned long current_cpu_tick;

#if defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_IRIX) \
    || defined(OS_HPUX) || defined(OS_DJGPP) || defined(OS_MACOSX) \
    || defined(OS_BSD) || defined(OS_CYGWIN) || defined(OS_QNX) \
    || defined(OS_TRU64) || defined(OS_VMS) || defined(OS_UNIXWARE) \
    || defined(OS_HURD) || defined(OS_MINIX)

	struct tms buf;

	current_cpu_tick = times(&buf);

#elif defined(USE_POSIX_CLOCKS)

	/* Using clock_gettime() requires doing a bunch of floating point
	 * operations below, so you may not want to use it on a slow CPU.
	 */

	double cpu_tick_as_double;
	struct timespec current_time;
	int status, saved_errno;

	if ( posix_clock_ticks_per_second <= 0.0 ) {
		(void) mx_clock_ticks_per_second();
	}

	status = clock_gettime( CLOCK_REALTIME, &current_time );

	if ( status != 0 ) {
		static const char fname[] = "mx_current_cpu_tick()";

		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Could not get current operating system time.  "
		"errno = %d, error message = '%s'.",
				saved_errno, strerror(saved_errno) );
	}

	cpu_tick_as_double = posix_clock_ticks_per_second
				* (double) current_time.tv_sec;

	cpu_tick_as_double += posix_clock_ticks_per_nsec
				* (double) current_time.tv_nsec;
	
	cpu_tick_as_double = fmod( cpu_tick_as_double, TWO_TO_THE_31ST_POWER );

	current_cpu_tick = mx_round( cpu_tick_as_double );

#elif defined(OS_SUNOS4)

	struct timeval tp;
	struct timezone tzp;
	int status, saved_errno;
	
	status = gettimeofday( &tp, &tzp );

	if ( status != 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_FUNCTION_FAILED, "mx_current_cpu_tick()",
		"gettimeofday() failed.  errno = %d, error status = '%s'",
			saved_errno, strerror( saved_errno ) );

		return (clock_t) 0;
	}

	/* The CPU clock actually counts at 60 Hz, so convert the output
	 * of gettimeofday() into 60Hz clock ticks.
	 * 
	 * The value of current_cpu_tick in the following calculation will
	 * overflow, but that is OK since we are only interested in the
	 * lowest order digits anyway.
	 */

	current_cpu_tick = (clock_t) ( 60L * tp.tv_sec );

	current_cpu_tick += (clock_t) (( 60L * tp.tv_usec ) / 1000000L);

	/* FIXME: mx_add_clock_ticks() does not handle negative low order
	 * clock tick values correctly.  The following temporarily hides
	 * the problem with the low order of clock ticks rather than fixing
	 * it.  Eventually we must modify mx_add_clock_ticks() to do the
	 * right thing when the low order is negative, but there is no time
	 * at the moment. (WML -- Jan. 26, 2001)
	 */

	if ( mx_cpu_start_tick_is_initialized == FALSE ) {
		mx_cpu_start_tick_is_initialized = TRUE;

		mx_cpu_start_tick = current_cpu_tick;
	}

	current_cpu_tick -= mx_cpu_start_tick;

#elif defined(OS_WIN32)

	current_cpu_tick = (clock_t) timeGetTime();

#elif defined(OS_RTEMS)
	{
		uint32_t ticks_since_boot;

		(void) rtems_clock_get( RTEMS_CLOCK_GET_TICKS_SINCE_BOOT,
					&ticks_since_boot );

		current_cpu_tick = ticks_since_boot;
	}
#elif defined(OS_VXWORKS)
	{
		struct timespec current_time;
		double time_in_seconds;

		clock_gettime( CLOCK_REALTIME, &current_time );

		time_in_seconds = (double) current_time.tv_sec
				+ 1.0e-9 * (double) current_time.tv_nsec;

		current_cpu_tick =
			mx_round( time_in_seconds * (double) sysClkRateGet() );
	}
#else
#error mx_current_cpu_tick() has not been defined for this operating system.
#endif

#if MX_CLOCK_DEBUG
	MX_DEBUG(-2,("%s: current_cpu_tick = %ld",
			fname, (long) current_cpu_tick));
#endif

	return current_cpu_tick;
}

/*-----------------------------------------------------------*/

MX_EXPORT double
mx_clock_ticks_per_second( void )
{
#if MX_CLOCK_DEBUG || MX_CLOCK_DEBUG_CLOCK_RESOLUTION
	static const char fname[] = "mx_clock_ticks_per_second()";
#endif

	double clock_ticks_per_second;

#if defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_IRIX) \
    || defined(OS_HPUX) || defined(OS_VMS) || defined(OS_MACOSX) \
    || defined(OS_BSD) || defined(OS_CYGWIN) || defined(OS_QNX) \
    || defined(OS_TRU64) || defined(OS_UNIXWARE) || defined(OS_HURD) \
    || defined(OS_MINIX)

#  if defined(_SC_CLK_TCK)
	clock_ticks_per_second = (double) sysconf(_SC_CLK_TCK);
#  elif defined(CLK_TCK)
	clock_ticks_per_second = (double) CLK_TCK;
#  else
	clock_ticks_per_second = CLOCKS_PER_SEC;
#  endif

#elif defined(OS_DJGPP)

	clock_ticks_per_second = CLOCKS_PER_SEC;

#elif defined(OS_RTEMS)
	{
		uint32_t ticks_per_second;

		(void) rtems_clock_get( RTEMS_CLOCK_GET_TICKS_PER_SECOND,
					&ticks_per_second );

		clock_ticks_per_second = ticks_per_second;
	}
#elif defined(USE_POSIX_CLOCKS)

	if ( posix_clock_ticks_per_second <= 0.0 ) {
		struct timespec resolution;
		int status, saved_errno;

		status = clock_getres( CLOCK_REALTIME, &resolution );

		if ( status != 0 ) {
			static const char fname[] = 
				"mx_clock_ticks_per_second()";
			
			saved_errno = errno;

			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Could not get operating system realtime clock resolution.  "
		"errno = %d, error message = '%s'.",
				saved_errno, strerror(saved_errno) );
		}

		posix_clock_ticks_per_second = mx_divide_safely( 1.0,
				(double) resolution.tv_sec
				+ 1.0e-9 * (double) resolution.tv_nsec );

		posix_clock_ticks_per_nsec = mx_divide_safely( 1.0,
				1.0e9 * (double) resolution.tv_sec
				+ (double) resolution.tv_nsec );
	}

	clock_ticks_per_second = posix_clock_ticks_per_second;

#elif defined(OS_VXWORKS)

	clock_ticks_per_second = (double) sysClkRateGet();

#elif defined(OS_WIN32)

	clock_ticks_per_second = 1000.0;

#elif defined(OS_SUNOS4)

	clock_ticks_per_second = 60.0;

#else
#error mx_clock_ticks_per_second() has not been defined for this operating system.
#endif

#if MX_CLOCK_DEBUG || MX_CLOCK_DEBUG_CLOCK_RESOLUTION
	MX_DEBUG(-2,("%s: clock_ticks_per_second = %g",
		fname, clock_ticks_per_second ));
#endif
	return clock_ticks_per_second;
}

MX_EXPORT void
mx_initialize_clock_ticks( void )
{
	mx_most_recent_clock_tick_value.high_order = 0L;
	mx_most_recent_clock_tick_value.low_order  = mx_current_cpu_tick();

	mx_clock_tick_divisor = 1.0 + (double) ULONG_MAX;
}

/* mx_current_clock_tick() must be invoked from time to time in order to
 * not miss two rollovers of the low order value in a row.  Currently,
 * the platforms with the most stringent requirements on this are Solaris 2
 * and SGI where if you go more than 2147 seconds (about 36 minutes) 
 * between calls to mx_current_clock_tick(), the value returned by 
 * mx_current_cpu_tick() may roll over twice without this being detected
 * by the software.
 */

MX_EXPORT MX_CLOCK_TICK
mx_current_clock_tick( void )
{
#if MX_CLOCK_DEBUG
	static const char fname[] = "mx_current_clock_tick()";
#endif

	MX_CLOCK_TICK current_clock_tick;

	unsigned long current_cpu_tick, old_low_order;

	current_clock_tick = mx_most_recent_clock_tick_value;

	old_low_order = current_clock_tick.low_order;

	current_cpu_tick = mx_current_cpu_tick();

	/* Check for CPU time rollover. */

#if defined(OS_IRIX) && defined(__GNUC__)

	/* FIXME: As amazing as it seems, the comparison of current_cpu_tick
	 * with old_low_order _does not_ work correctly under Irix 6.5 using
	 * GCC 2.8.1.  Yes, I know its an old version of GCC, but I do not
	 * have control of the machine in question.
	 * 
	 * As an example, if the two tick values are _equal_ but greater than
	 * 2^31 (LONG_MAX), the expression ( current_cpu_tick < old_low_order ) 
	 * evaluates to _true_.  So is it a compiler bug, an error in my
	 * understanding, alien mind control rays, or the revenge of Bigfoot?
	 * You make the call!
	 *
	 * So far, the best fix has been to assign the difference to the
	 * long variable 'tick_delta' and then do the comparison of that
	 * value with 0.  However, if the magnitude of the difference
	 * between 'current_cpu_tick' and 'old_low_order' ever gets bigger
	 * than LONG_MAX then this simple test will give the wrong answer
	 * since the difference will wrap to a negative value.
	 */

	{
		long tick_delta;

		tick_delta = current_cpu_tick - old_low_order;

		if ( tick_delta < 0 ) {
			current_clock_tick.high_order ++;
		}
	}
#else
	if ( current_cpu_tick < old_low_order ) {
		current_clock_tick.high_order ++;
	}
#endif

	/* Save the new current clock tick value for the next time. */

	current_clock_tick.low_order = current_cpu_tick;

	mx_most_recent_clock_tick_value = current_clock_tick;

#if MX_CLOCK_DEBUG
	MX_DEBUG(-2,("%s: current_clock_tick = (%lu,%lu)", fname,
		current_clock_tick.high_order, current_clock_tick.low_order));
#endif
	return current_clock_tick;
}

MX_EXPORT MX_CLOCK_TICK
mx_relative_clock_tick( double relative_time_in_seconds )
{
	MX_CLOCK_TICK abs_relative_time_in_ticks;
	MX_CLOCK_TICK result;

	abs_relative_time_in_ticks = mx_convert_seconds_to_clock_ticks(
					fabs(relative_time_in_seconds) );

	if ( relative_time_in_seconds >= 0.0 ) {
		result = mx_add_clock_ticks( mx_current_clock_tick(),
					abs_relative_time_in_ticks );
	} else {
		result = mx_subtract_clock_ticks( mx_current_clock_tick(),
					abs_relative_time_in_ticks );
	}

	return result;
}

MX_EXPORT void
mx_wait_until_clock_tick( MX_CLOCK_TICK final_tick )
{
	MX_CLOCK_TICK current_tick;
	int comparison;

	while (TRUE) {
		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks( current_tick, final_tick );

		if ( comparison >= 0 ) {
			return;
		}
	}
}

MX_EXPORT MX_CLOCK_TICK
mx_convert_seconds_to_clock_ticks( double seconds )
{
#if MX_CLOCK_DEBUG
	static const char fname[] = "mx_convert_seconds_to_clock_ticks()";
#endif

	MX_CLOCK_TICK clock_tick_value;
	double clock_ticks_as_double;

	clock_ticks_as_double = seconds * mx_clock_ticks_per_second();

	clock_tick_value.high_order = (unsigned long)
	    mx_divide_safely( clock_ticks_as_double, mx_clock_tick_divisor );

	clock_tick_value.low_order = (clock_t) ( clock_ticks_as_double
	    - mx_clock_tick_divisor * (double) clock_tick_value.high_order );

#if MX_CLOCK_DEBUG
	MX_DEBUG(-2,
	("%s: seconds = %g ---> clock_tick_value = (%lu,%lu)", fname, seconds,
		clock_tick_value.high_order, clock_tick_value.low_order));
#endif

	return clock_tick_value;
}

MX_EXPORT double
mx_convert_clock_ticks_to_seconds( MX_CLOCK_TICK clock_tick_value )
{
#if MX_CLOCK_DEBUG
	static const char fname[] = "mx_convert_clock_ticks_to_seconds()";
#endif

	double clock_ticks_as_double, seconds;

	clock_ticks_as_double = (double) clock_tick_value.low_order
	    + mx_clock_tick_divisor * (double) clock_tick_value.high_order;

	seconds = clock_ticks_as_double / mx_clock_ticks_per_second();

#if MX_CLOCK_DEBUG
	MX_DEBUG(-2,
	("%s: clock_tick_value = (%lu,%lu) ---> seconds = %g", fname,
	    clock_tick_value.high_order, clock_tick_value.low_order, seconds));
#endif

	return seconds;
}

MX_EXPORT MX_CLOCK_TICK
mx_add_clock_ticks( MX_CLOCK_TICK clock_tick_1, MX_CLOCK_TICK clock_tick_2 )
{
#if MX_CLOCK_DEBUG
	static const char fname[] = "mx_add_clock_ticks()";
#endif

	MX_CLOCK_TICK result;
	unsigned long H1, H2;
	unsigned long L1, L2;

	H1 = clock_tick_1.high_order;
	L1 = clock_tick_1.low_order;

	H2 = clock_tick_2.high_order;
	L2 = clock_tick_2.low_order;

	result.low_order  = L1 + L2;

	result.high_order = H1 + H2;

	/* Check for carry. */

	if ( L2 > ULONG_MAX - L1 )
		result.high_order ++;

#if MX_CLOCK_DEBUG
	MX_DEBUG(-2,("%s: (%lu,%lu) + (%lu,%lu) ---> (%lu,%lu)", fname,
		clock_tick_1.high_order, clock_tick_1.low_order,
		clock_tick_2.high_order, clock_tick_2.low_order,
		result.high_order, result.low_order ));
#endif

	return result;
}

MX_EXPORT MX_CLOCK_TICK
mx_subtract_clock_ticks(MX_CLOCK_TICK clock_tick_1, MX_CLOCK_TICK clock_tick_2)
{
#if MX_CLOCK_DEBUG
	static const char fname[] = "mx_subtract_clock_ticks()";
#endif

	MX_CLOCK_TICK result;
	unsigned long H1, H2;
	unsigned long L1, L2;

	H1 = clock_tick_1.high_order;
	L1 = clock_tick_1.low_order;

	H2 = clock_tick_2.high_order;
	L2 = clock_tick_2.low_order;

	result.low_order  = L1 - L2;

	result.high_order = H1 - H2;

	/* Check for borrow. */

	if ( L1 < L2 )
		result.high_order --;

#if MX_CLOCK_DEBUG
	MX_DEBUG(-2,("%s: (%lu,%lu) - (%lu,%lu) ---> (%lu,%lu)", fname,
		clock_tick_1.high_order, clock_tick_1.low_order,
		clock_tick_2.high_order, clock_tick_2.low_order,
		result.high_order, result.low_order ));
#endif

	return result;
}

MX_EXPORT int
mx_compare_clock_ticks(MX_CLOCK_TICK clock_tick_1, MX_CLOCK_TICK clock_tick_2)
{
#if MX_CLOCK_DEBUG
	static const char fname[] = "mx_compare_clock_ticks()";
#endif

	unsigned long H1, H2;
	unsigned long L1, L2;
	int result;

	H1 = clock_tick_1.high_order;
	L1 = clock_tick_1.low_order;

	H2 = clock_tick_2.high_order;
	L2 = clock_tick_2.low_order;

	if ( H1 < H2 ) {
		result = -1;
	} else
	if ( H1 > H2 ) {
		result = 1;
	} else {
		if ( L1 < L2 ) {
			result = -1;
		} else
		if ( L1 > L2 ) {
			result = 1;
		} else {
			result = 0;
		}
	}

#if MX_CLOCK_DEBUG
	{
		char result_char;

		if ( result > 0 ) {
			result_char = '>';
		} else
		if ( result < 0 ) {
			result_char = '<';
		} else {
			result_char = '=';
		}

		MX_DEBUG(-2,("%s: (%lu,%lu) %c (%lu,%lu)", fname,
			clock_tick_1.high_order, clock_tick_1.low_order,
			result_char,
			clock_tick_2.high_order, clock_tick_2.low_order ));
	}
#endif

	return result;
}

MX_EXPORT MX_CLOCK_TICK
mx_set_clock_tick_to_zero( void )
{
#if MX_CLOCK_DEBUG
	static const char fname[] = "mx_set_clock_tick_to_zero()";
#endif

	MX_CLOCK_TICK result;

	result.low_order = 0;
	result.high_order = 0;

#if MX_CLOCK_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	return result;
}

MX_EXPORT MX_CLOCK_TICK
mx_set_clock_tick_to_maximum( void )
{
#if MX_CLOCK_DEBUG
	static const char fname[] = "mx_set_clock_tick_to_maximum()";
#endif

	MX_CLOCK_TICK result;

	result.low_order = ULONG_MAX;
	result.high_order = ULONG_MAX;

#if MX_CLOCK_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	return result;
}


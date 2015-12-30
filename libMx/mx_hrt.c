/*
 * Name:    mx_hrt.c
 *
 * Purpose: This file contains functions that manipulate times shorter
 *          than the scheduling interval for the operating system.
 *
 *          In general, the mx_sleep(), mx_msleep(), and mx_usleep() family
 *          of functions perform their tasks by yielding control to the
 *          operating system and requesting that they be woken up again
 *          by the OS at a later time.  This usually means that you cannot
 *          create delays this way that are shorter than the minimum process
 *          scheduling interval for the operating system (100 Hz for most
 *          versions of Linux).
 *
 *          mx_udelay() instead attempts to implement microsecond delays
 *          that do _not_ explicitly return control to the operating system
 *          by using a busy-wait loop to wait until the requested time has
 *          elapsed.  This allows for delays that are much shorter than the
 *          scheduler would permit.  This is not possible on all operating
 *          systems, so mx_udelay() reverts back to using mx_usleep() in
 *          that case.
 *
 *          mx_high_resolution_time() exports the internal clock used by
 *          mx_udelay() in a 'struct timespec' structure.  The matching
 *          function mx_high_resolution_time_in_seconds() reports back
 *          the same value in seconds using a double variable.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2004, 2006-2007, 2009-2012, 2014-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_stdint.h"
#include "mx_cfn.h"
#include "mx_clock.h"
#include "mx_hrt.h"

/* The time returned by mx_high_resolution_time_using_clock_ticks() is
 * generally "high resolution" in name only.  But this function serves
 * as a fallback if there is no better method.
 */

#if defined(OS_SOLARIS)
#  define MX_NEED_HRT_FALLBACK		FALSE

#elif defined(OS_IRIX)
#  define MX_NEED_HRT_FALLBACK		FALSE

#elif defined(OS_MACOSX)
#  if defined(__i386__)
#    define MX_NEED_HRT_FALLBACK	TRUE
#  elif defined(__amd64__)
#    define MX_NEED_HRT_FALLBACK	FALSE
#  elif defined(__ppc__)
#    define MX_NEED_HRT_FALLBACK	FALSE
#  else
#    error Unrecognized CPU architecture for MacOS X!
#  endif

#elif ( defined(__GNUC__) && defined(__x86_64__) )
#  define MX_NEED_HRT_FALLBACK		FALSE

#else
#  define MX_NEED_HRT_FALLBACK		TRUE
#endif

/*--------------------------------------------------------------------------*/

#if MX_NEED_HRT_FALLBACK

static struct timespec
mx_high_resolution_time_using_clock_ticks( void )
{
	MX_CLOCK_TICK clock_tick;
	struct timespec value;
	double time_in_seconds;

	clock_tick = mx_current_clock_tick();

	time_in_seconds = mx_convert_clock_ticks_to_seconds( clock_tick );

	value.tv_sec = (long) time_in_seconds;

	time_in_seconds -= (double) value.tv_sec;

	value.tv_nsec = (long) ( 1000000000.0 * time_in_seconds );

	return value;
}

#endif /* MX_NEED_HRT_FALLBACK */

/*---*/

MX_EXPORT struct timespec
mx_add_high_resolution_times( struct timespec time1,
				struct timespec time2 )
{
	struct timespec result;
	long nanoseconds;

	nanoseconds = time1.tv_nsec + time2.tv_nsec;

	result.tv_nsec = nanoseconds % 1000000000L;

	result.tv_sec = (time_t)
		( time1.tv_sec + time2.tv_sec + nanoseconds / 1000000000L );

	return result;
}

MX_EXPORT struct timespec
mx_subtract_high_resolution_times( struct timespec time1,
				struct timespec time2 )
{
	struct timespec result;

	if ( time1.tv_nsec < time2.tv_nsec ) {
		time1.tv_sec  -= 1L;
		time1.tv_nsec += 1000000000L;
	}

	result.tv_nsec = time1.tv_nsec - time2.tv_nsec;

	result.tv_sec = time1.tv_sec - time2.tv_sec;

	return result;
}

MX_EXPORT int
mx_compare_high_resolution_times( struct timespec time1,
				struct timespec time2 )
{
	unsigned long H1, H2, L1, L2;

	H1 = time1.tv_sec;
	H2 = time2.tv_sec;

	L1 = time1.tv_nsec;
	L2 = time2.tv_nsec;

	if ( H1 < H2 ) {
		return (-1);
	} else if ( H1 > H2 ) {
		return 1;
	} else {
		if ( L1 < L2 ) {
			return (-1);

		} else if ( L1 > L2 ) {
			return 1;

		} else {
			return 0;
		}
	}
}

MX_EXPORT struct timespec
mx_rescale_high_resolution_time( double scale_factor,
				struct timespec original_time )
{
	struct timespec new_time;
	double new_seconds, new_nanoseconds, extra_seconds;

	new_seconds = scale_factor * original_time.tv_sec;
	new_nanoseconds = scale_factor * original_time.tv_nsec;

	if ( new_nanoseconds >= 1.0e9 ) {
		extra_seconds = (long) (new_nanoseconds / 1.0e9);

		new_seconds = new_seconds + extra_seconds;
		new_nanoseconds = fmod( new_nanoseconds, 1.0e9 );
	}

	new_time.tv_sec = mx_round( new_seconds );

	new_time.tv_nsec = mx_round( new_nanoseconds );

	return new_time;
}

MX_EXPORT struct timespec
mx_convert_seconds_to_high_resolution_time( double seconds )
{
	struct timespec result;

	result.tv_sec = (time_t) seconds;

	seconds -= (double) result.tv_sec;

	result.tv_nsec = (long) ( 1.0e9 * seconds );

	return result;
}

MX_EXPORT double
mx_high_resolution_time_as_double( void )
{
	struct timespec hrt;
	double result;

	hrt = mx_high_resolution_time();

	result = mx_convert_high_resolution_time_to_seconds( hrt );

	return result;
}

/*--------------------------------------------------------------------------*/

#if defined(OS_WIN32)

/******* Microsoft Win32 *******/

#include <windows.h>

static int mx_high_resolution_time_init_invoked = FALSE;

static int mx_performance_counters_are_available = FALSE;

static double mx_hrt_counter_ticks_per_microsecond = 0.0;

static void
mx_hrt_error_message( const char *function_name )
{
	static const char fname[] = "mx_hrt_error_message()";

	DWORD last_error_code;
	TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

	last_error_code = GetLastError();

	mx_win32_error_message( last_error_code,
		message_buffer, sizeof(message_buffer) );

	(void) mx_error( MXE_FUNCTION_FAILED, fname,
	"Error invoking %s.  Win32 error code = %ld, error message = '%s'",
		function_name, last_error_code, message_buffer );

	return;
}

MX_EXPORT void
mx_udelay( unsigned long microseconds )
{
	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	if ( mx_performance_counters_are_available == FALSE ) {
		mx_usleep( microseconds );
	} else {
		LARGE_INTEGER counter_value;
		LONGLONG counter_start_value, counter_end_value;
		LONGLONG counter_current_value, counter_delay_ticks;
		BOOL status;

		counter_delay_ticks = (LONGLONG)
	( mx_hrt_counter_ticks_per_microsecond * (double)microseconds );

		status = QueryPerformanceCounter( &counter_value );

		if ( status == 0 ) {
			mx_hrt_error_message( "QueryPerformanceCounter()" );
			return;
		}

		counter_start_value = counter_value.QuadPart;

		counter_end_value = counter_start_value + counter_delay_ticks;

		/* Sit in a loop until we reach the end of the delay time. */

		counter_current_value = counter_start_value;

		while ( counter_current_value < counter_end_value ) {

			status = QueryPerformanceCounter( &counter_value );

			if ( status == 0 ) {
				mx_hrt_error_message(
					"QueryPerformanceCounter()" );
				return;
			}

			counter_current_value = counter_value.QuadPart;
		}
	}
	return;
}

MX_EXPORT struct timespec
mx_high_resolution_time( void )
{
	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	if ( mx_performance_counters_are_available == FALSE ) {

		return mx_high_resolution_time_using_clock_ticks();

	} else {
		struct timespec value;
		double time_in_microseconds;
		LARGE_INTEGER counter_value;
		BOOL status;

		status = QueryPerformanceCounter( &counter_value );

		if ( status == 0 ) {
			value.tv_sec = 0;
			value.tv_nsec = 0;

			mx_hrt_error_message( "QueryPerformanceCounter()" );

			return value;
		}

		time_in_microseconds = mx_divide_safely(
				(double) counter_value.QuadPart,
				mx_hrt_counter_ticks_per_microsecond );

		value.tv_sec = (unsigned long)
				( 0.000001 * time_in_microseconds );

		time_in_microseconds -= 1000000.0 * (double) value.tv_sec;

		value.tv_nsec = (unsigned long)
				( 1000.0 * time_in_microseconds );

		return value;
	}
}

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	LARGE_INTEGER performance_frequency;
	BOOL status;

	mx_high_resolution_time_init_invoked = TRUE;

	status = QueryPerformanceFrequency( &performance_frequency );

	if ( status == 0 ) {
		mx_hrt_error_message( "QueryPerformanceFrequency()" );

		mx_performance_counters_are_available = FALSE;

		mx_warning(
			"Win32 performance counters are not available, "
			"so we will revert to using mx_usleep() instead." );
	} else {
		mx_performance_counters_are_available = TRUE;

		mx_hrt_counter_ticks_per_microsecond
			= 1.0e-6 * (double) performance_frequency.QuadPart;
	}
	return;
}

/*--------------------------------------------------------------------------*/

#elif defined(OS_SOLARIS)

#include <sys/time.h>

MX_EXPORT void
mx_udelay( unsigned long microseconds )
{
	hrtime_t start_time, end_time, delay_in_nanoseconds;

	delay_in_nanoseconds = (hrtime_t) ( 1000L * microseconds );

	start_time = gethrtime();

	end_time = start_time + delay_in_nanoseconds;

	while ( gethrtime() < end_time )
		;				/* Do nothing */

	return;
}

MX_EXPORT struct timespec
mx_high_resolution_time( void )
{
	struct timespec value;
	hrtime_t time_in_nsec;

	time_in_nsec = gethrtime();

	value.tv_sec = (unsigned long)( time_in_nsec / (hrtime_t) 1000000000L );

	time_in_nsec -= (hrtime_t) value.tv_sec;

	value.tv_nsec = (unsigned long) time_in_nsec;

	return value;
}

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	/* Solaris does not need any initialization. */

	return;
}

/*--------------------------------------------------------------------------*/

#elif defined(OS_IRIX)

/******* SGI Irix *******/

#include <stddef.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syssgi.h>

static int mx_high_resolution_time_init_invoked = FALSE;

static int mx_cycle_counter_is_64bit = FALSE;

static volatile unsigned long long *mx_iotimer_64bit_addr = NULL;

static volatile unsigned int *mx_iotimer_32bit_addr = NULL;

static double mx_hrt_counter_ticks_per_microsecond = 0.0;

MX_EXPORT void
mx_udelay( unsigned long microseconds )
{
	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	if ( mx_cycle_counter_is_64bit ) {
		unsigned long long mx_64bit_start_value;
		unsigned long long mx_64bit_end_value;
		unsigned long long mx_64bit_delay_ticks;

		mx_64bit_delay_ticks = (unsigned long long)
	    ( mx_hrt_counter_ticks_per_microsecond * (double) microseconds );

		mx_64bit_start_value = *mx_iotimer_64bit_addr;

		mx_64bit_end_value = mx_64bit_start_value
					+ mx_64bit_delay_ticks;

		while ( (*mx_iotimer_64bit_addr) < mx_64bit_end_value )
			;				/* Do nothing */

	} else {
		unsigned int mx_32bit_start_value;
		unsigned int mx_32bit_end_value;
		unsigned int mx_32bit_delay_ticks;

		mx_32bit_delay_ticks = (unsigned int)
	( mx_hrt_counter_ticks_per_microsecond * (double) microseconds );

		mx_32bit_start_value = *mx_iotimer_32bit_addr;

		mx_32bit_end_value = mx_32bit_start_value
					+ mx_32bit_delay_ticks;

		while ( (*mx_iotimer_32bit_addr) < mx_32bit_end_value )
			;				/* Do nothing */

	}
	return;
}

MX_EXPORT struct timespec
mx_high_resolution_time( void )
{
	struct timespec value;
	double time_in_microseconds;

	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	if ( mx_cycle_counter_is_64bit ) {
		time_in_microseconds =
			mx_divide_safely( (double) *mx_iotimer_64bit_addr,
					mx_hrt_counter_ticks_per_microsecond );
	} else {
		time_in_microseconds =
			mx_divide_safely( (double) *mx_iotimer_32bit_addr,
					mx_hrt_counter_ticks_per_microsecond );
	}

	value.tv_sec = (time_t) ( 0.000001 * time_in_microseconds );

	time_in_microseconds -= 1000000.0 * (double) value.tv_sec;

	value.tv_nsec = (long) ( 1000.0 * time_in_microseconds );

	return value;
}

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	static const char fname[] = "mx_high_resolution_time_init()";

	volatile void *void_addr;
	__psunsigned_t phys_addr, raddr;
	int fd, poffmask;
	long cyclecntr_size;
	unsigned int cycleval;

	mx_high_resolution_time_init_invoked = TRUE;

	cyclecntr_size = syssgi( SGI_CYCLECNTR_SIZE );

	if ( cyclecntr_size == 64 ) {
		mx_cycle_counter_is_64bit = TRUE;
	} else {
		mx_cycle_counter_is_64bit = FALSE;
	}

	MX_DEBUG( 2,("%s: mx_cycle_counter_is_64bit = %d",
			fname, mx_cycle_counter_is_64bit));

	/* The following is derived from the Irix man page for syssgi(). */

	poffmask = getpagesize() - 1;

	phys_addr = syssgi( SGI_QUERY_CYCLECNTR, &cycleval );

	raddr = phys_addr & ~poffmask;

	mx_hrt_counter_ticks_per_microsecond = mx_divide_safely( 1000000.0,
							(double) cycleval );

	MX_DEBUG( 2,("%s: cycleval = %u", fname, cycleval));

	MX_DEBUG( 2,("%s: mx_hrt_counter_ticks_per_microsecond = %g",
			fname, mx_hrt_counter_ticks_per_microsecond));

	fd = open("/dev/mmem", O_RDONLY);

	void_addr = mmap( 0, poffmask, PROT_READ, MAP_PRIVATE,
				fd, (off_t) raddr );

	if ( mx_cycle_counter_is_64bit ) {
		mx_iotimer_64bit_addr =
			(volatile unsigned long long *) void_addr;

		mx_iotimer_64bit_addr = (unsigned long long *)
		  ((__psunsigned_t) mx_iotimer_64bit_addr
			+ (phys_addr & poffmask));
	} else {
		mx_iotimer_32bit_addr = (volatile unsigned int *) void_addr;

		mx_iotimer_32bit_addr = (unsigned int *)
		  ((__psunsigned_t) mx_iotimer_32bit_addr
			+ (phys_addr & poffmask));
	}

	return;
}

/*--------------------------------------------------------------------------*/

#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )

/* The x86 and x86_64 methods for GCC share much of the same code. */

#if defined(__x86_64__)

/******* GCC on x86_64 *******/

static int mx_high_resolution_time_init_invoked = FALSE;

static double mx_hrt_counter_ticks_per_microsecond = 0.0;

static __inline__ unsigned long
mx_rdtsc( void )
{
	unsigned long x;

	/* The following generates inline an RDTSC instruction
	 * and puts the results in the variable "x".
	 *
	 * As it happens, the version of this code used on 32-bit
         * machines will not put the value in the right place on
	 * a 64-bit machine, so we must use this 64-bit specific
	 * method instead.
	 *
	 * References:
	 *    http://www.technovelty.org/code/c/reading-rdtsc.html
	 *    http://lists.freedesktop.org/archives/cairo/2008-September/015029.html
	 */

	unsigned int a_register, d_register;

	__asm__ volatile( "rdtsc" : "=a" (a_register), "=d" (d_register) );

	x = ((unsigned long) a_register) | (((unsigned long) d_register) << 32);

	return x;
}

MX_EXPORT void
mx_udelay( unsigned long microseconds )
{
	unsigned long tsc_start_value, tsc_end_value;
	unsigned long tsc_delay_ticks;

	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	tsc_delay_ticks = (unsigned long)
	    ( mx_hrt_counter_ticks_per_microsecond * (double) microseconds );

	tsc_start_value = mx_rdtsc();

	tsc_end_value = tsc_start_value + tsc_delay_ticks;

	/* Sit in a loop until we reach the end of the delay time. */

	while ( mx_rdtsc() < tsc_end_value )
		;				/* Do nothing. */

	return;
}

MX_EXPORT struct timespec
mx_high_resolution_time( void )
{
	struct timespec value;
	unsigned long tsc_value;
	double time_in_microseconds;

	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	tsc_value = mx_rdtsc();

	time_in_microseconds = mx_divide_safely( (double) tsc_value,
				mx_hrt_counter_ticks_per_microsecond );

	value.tv_sec = (unsigned long) ( 0.000001 * time_in_microseconds );

	time_in_microseconds -= 1000000.0 * (double) value.tv_sec;

	value.tv_nsec = (unsigned long) ( 1000.0 * time_in_microseconds );

	return value;
}

#elif defined(__i386__)

/******* GCC on x86 *******/

/* The following was inspired by the Linux I/O port programming mini-HOWTO. */

#define MX_CPU_UNKNOWN		0
#define MX_CPU_USE_XCHG		1
#define MX_CPU_USE_RDTSC	2

static int mx_cpu_delay_type = MX_CPU_UNKNOWN;

static int mx_high_resolution_time_init_invoked = FALSE;

static double mx_hrt_counter_ticks_per_microsecond = 0.0;

static __inline__ unsigned long long int
mx_rdtsc( void )
{
	unsigned long long int x;

	/* The following generates inline an RDTSC instruction
	 * and puts the results in the variable "x".
	 */

	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));

	return x;
}

static __inline__ void
mx_xchg_delay( void )
{
	/* The following takes three clock cycles on i386 and i486 machines.
	 * We will be using RDTSC on other x86 CPU types.
	 */

	__asm__ volatile ("xchg %bx,%bx");

	return;
}

MX_EXPORT void
mx_udelay( unsigned long microseconds )
{
	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	if ( mx_cpu_delay_type == MX_CPU_USE_RDTSC ) {

		/* Warning: The following logic has the fundamental flaw
		 * that the TSC counter will overflow in around 584 years
		 * on a 1 GHz x86 machine, but I expect to be retired long
		 * before that.  (WML)
		 */

		unsigned long long int tsc_start_value, tsc_end_value;
		unsigned long long int tsc_delay_ticks;

		tsc_delay_ticks = (unsigned long long int)
	( mx_hrt_counter_ticks_per_microsecond * (double) microseconds );

		tsc_start_value = mx_rdtsc();

		tsc_end_value = tsc_start_value + tsc_delay_ticks;

		/* Sit in a loop until we reach the end of the delay time. */

		while ( mx_rdtsc() < tsc_end_value )
			;				/* Do nothing */

	} else {
		/* This code does not work as well as the RDTSC code since
		 * it does not take into account delays due to the process
		 * being preempted by the kernel.
		 */

		unsigned long long int i, num_times_to_loop;

		num_times_to_loop = (unsigned long long int)
	( mx_hrt_counter_ticks_per_microsecond * (double) microseconds );

		for ( i = 0; i < num_times_to_loop; i++ ) {
			mx_xchg_delay();
		}
	}
	return;
}

MX_EXPORT struct timespec
mx_high_resolution_time( void )
{
	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	if ( mx_cpu_delay_type != MX_CPU_USE_RDTSC ) {
		return mx_high_resolution_time_using_clock_ticks();
	} else {
		struct timespec value;
		unsigned long long tsc_value;
		double time_in_microseconds;

		tsc_value = mx_rdtsc();

		time_in_microseconds = mx_divide_safely( (double) tsc_value,
					mx_hrt_counter_ticks_per_microsecond );

		value.tv_sec = (unsigned long)
				( 0.000001 * time_in_microseconds );

		time_in_microseconds -= 1000000.0 * (double) value.tv_sec;

		value.tv_nsec = (unsigned long)
				( 1000.0 * time_in_microseconds );

		return value;
	}
}

#else
#error Unrecognized x86 variant for GCC.
#endif

/*--------------------------------------------------------------------------*/

#if defined(OS_LINUX) || defined(OS_CYGWIN)

/******* GCC on Linux or Cygwin using /proc/cpuinfo *******/

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	static const char fname[] = "mx_high_resolution_time_init()";

	char buffer[500];
	char *ptr, *ptr2;
	int cpu_family, have_tsc, num_items;
	double cpu_mhz;

	/* Reading /proc/cpuinfo is the only vaguely portable way I know of
	 * for getting the necessary information.
	 */

	FILE *cpuinfo;

	mx_high_resolution_time_init_invoked = TRUE;

	cpuinfo = fopen( "/proc/cpuinfo", "r" );

	if ( cpuinfo == NULL ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
		"/proc/cpuinfo could not be found or is unreadable.  "
		"This may indicate that there is something seriously "
		"wrong with your Linux system." );

		return;
	}

	cpu_family = 0;
	have_tsc = FALSE;
	cpu_mhz = 0.0;

	mx_fgets( buffer, sizeof buffer, cpuinfo );

	while ( !feof(cpuinfo) && !ferror(cpuinfo) ) {

		if ( strncmp( buffer, "cpu family", 10 ) == 0 ) {

			ptr = strchr( buffer, ':' );

			if ( ptr == NULL ) {
				(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Could not find a ':' character in a line "
				"read from /proc/cpuinfo.  line = '%s'",
					buffer );
			} else {
				ptr++;

				num_items = sscanf( ptr, "%d", &cpu_family );

				if ( num_items != 1 ) {
				    (void) mx_error( MXE_UNPARSEABLE_STRING,
					fname,
				"Could not find the cpu family number in a "
				"line read from /proc/cpuinfo.  line = '%s'",
					buffer );
				}
			}
		} else
		if ( strncmp( buffer, "cpu MHz", 7 ) == 0 ) {

			ptr = strchr( buffer, ':' );

			if ( ptr == NULL ) {
				(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Could not find a ':' character in a line "
				"read from /proc/cpuinfo.  line = '%s'",
					buffer );
			} else {
				ptr++;

				num_items = sscanf( ptr, "%lg", &cpu_mhz );

				if ( num_items != 1 ) {
				    (void) mx_error( MXE_UNPARSEABLE_STRING,
					fname,
				"Could not find the cpu MHz in a "
				"line read from /proc/cpuinfo.  line = '%s'",
					buffer );
				}
			}
		} else
		if ( strncmp( buffer, "flags", 5 ) == 0 ) {

			ptr = strchr( buffer, ':' );

			if ( ptr == NULL ) {
				(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Could not find a ':' character in a line "
				"read from /proc/cpuinfo.  line = '%s'",
					buffer );
			} else {
				ptr++;

				ptr2 = strstr( ptr, "tsc" );

				if ( ptr2 != NULL ) {
					have_tsc = TRUE;
				}
			}
		}

		mx_fgets( buffer, sizeof buffer, cpuinfo );
	}

	fclose( cpuinfo );

#if defined(__i386__)
	if ( have_tsc ) {
		mx_cpu_delay_type = MX_CPU_USE_RDTSC;
	} else {
		mx_cpu_delay_type = MX_CPU_USE_XCHG;
	}
#endif

	if ( (cpu_family == 3) || (cpu_family == 4) ) {
		mx_hrt_counter_ticks_per_microsecond = cpu_mhz / 3.0;
	} else {
		mx_hrt_counter_ticks_per_microsecond = cpu_mhz;
	}

	MX_DEBUG( 2,("%s: mx_hrt_counter_ticks_per_microsecond = %g",
		fname, mx_hrt_counter_ticks_per_microsecond));

	return;
}

/*--------------------------------------------------------------------------*/

#elif defined(__FreeBSD__)

#include <errno.h>
#include <sys/sysctl.h>

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	static const char fname[] = "mx_high_resolution_time_init()";

	uint64_t tsc_freq;
	size_t length;
	int status, saved_errno;

	mx_high_resolution_time_init_invoked = TRUE;

	tsc_freq = 0;

	length = sizeof(tsc_freq);

	status = sysctlbyname("machdep.tsc_freq", &tsc_freq, &length, NULL, 0);

	if ( status < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to read the value of 'machdep.tsc_freq' "
		"using sysctlbyname() failed.  Errno = %d, error = '%s'",
			saved_errno, strerror(saved_errno) );
		return;
	}

	MX_DEBUG( 2,("%s: tsc_freq = %llu",
		fname, (long long unsigned int) tsc_freq));

	mx_hrt_counter_ticks_per_microsecond = tsc_freq / 1000000LU;

	MX_DEBUG( 2,("%s: mx_hrt_counter_ticks_per_microsecond = %g",
		fname, mx_hrt_counter_ticks_per_microsecond));

	return;
}

/*--------------------------------------------------------------------------*/

#elif defined(__NetBSD__)

#include <errno.h>
#include <sys/sysctl.h>

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	static const char fname[] = "mx_high_resolution_time_init()";

	int mib[2];
	int status, saved_errno;
	struct clockinfo clockinfo_struct;
	size_t clockinfo_size;

	mx_high_resolution_time_init_invoked = TRUE;

	mib[0] = CTL_KERN;
	mib[1] = KERN_CLOCKRATE;

	clockinfo_size = sizeof(clockinfo_struct);

	status = sysctl(mib, 2, &clockinfo_struct, &clockinfo_size, NULL, 0);

	if ( status != 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_FUNCTION_FAILED, fname,
		"The attempt to get the x86 CPU speed failed.  "
		"Errno = %d, error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
		return;
	}

	mx_hrt_counter_ticks_per_microsecond =
		1.0e-6 * (double) clockinfo_struct.hz;

	return;
}

/*--------------------------------------------------------------------------*/

#elif defined(OS_MACOSX) || defined(__OpenBSD__)

#include <errno.h>
#include <sys/sysctl.h>

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	static const char fname[] = "mx_high_resolution_time_init()";

	int mib[2], cpu_speed;
	int status, saved_errno;
	size_t cpu_speed_size;

	mx_high_resolution_time_init_invoked = TRUE;

	mib[0] = CTL_HW;

#if defined(OS_MACOSX)
	mib[1] = HW_TB_FREQ;
#else
	mib[1] = HW_CPUSPEED;
#endif

	cpu_speed_size = sizeof(cpu_speed);

	status = sysctl(mib, 2, &cpu_speed, &cpu_speed_size, NULL, 0);

	if ( status != 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_FUNCTION_FAILED, fname,
		"The attempt to get the x86 CPU speed failed.  "
		"Errno = %d, error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
		return;
	}

	mx_hrt_counter_ticks_per_microsecond = 1.0e-6 * (double) cpu_speed;

	return;
}

/*--------------------------------------------------------------------------*/

#else	/* not OS_LINUX */

#  define MX_NEED_HRT_INIT_FROM_FILE	TRUE

double mx_high_resolution_time_init_from_file( void );

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	unsigned long long tsc_before, tsc_after;

	mx_high_resolution_time_init_invoked = TRUE;

	/* Initially, we attempt to to read the calibration from the
	 * file $MXDIR/etc/mx_timer_calib.dat
	 */

	mx_hrt_counter_ticks_per_microsecond =
		mx_high_resolution_time_init_from_file();

	if ( mx_hrt_counter_ticks_per_microsecond > 0.0 ) {
		return;
	}

	/* If reading the calibration from a file did not succeed,
	 * then we try to compute the calibration right now.
	 */

	mx_info(
"Calibrating high resolution timer.  This will take around 10 seconds." );

#if defined(__i386__)
	mx_warning(
"Calibration will fail on 386s and 486s and may cause the program to crash.");
#endif

	tsc_before = mx_rdtsc();

	sleep(10);

	tsc_after = mx_rdtsc();

	mx_hrt_counter_ticks_per_microsecond =
		1.0e-7 * (double) ( tsc_after - tsc_before );

	mx_info( "High resolution timer ticks per microsecond = %g",
		mx_hrt_counter_ticks_per_microsecond );

	return;
}

#endif  /* OS_LINUX */

#elif defined(__GNUC__) && defined(__ppc__)

/******* GCC on PowerPC *******/

static int mx_high_resolution_time_init_invoked = FALSE;

static double mx_hrt_counter_ticks_per_microsecond = 0.0;

static __inline__ unsigned long long
mx_read_timebase( void )
{
	unsigned long long timebase;
	unsigned long timebase_lower, timebase_upper, timebase_upper_2;

	do {
		__asm__ volatile("mftbu %0" : "=r" (timebase_upper));
		__asm__ volatile("mftb %0" : "=r" (timebase_lower));
		__asm__ volatile("mftbu %0" : "=r" (timebase_upper_2));
	} while ( timebase_upper != timebase_upper_2 );

	timebase =
	    (((unsigned long long) timebase_upper) << 32) | timebase_lower;

	return timebase;
}

MX_EXPORT void
mx_udelay( unsigned long microseconds )
{
	unsigned long long start_time, end_time, delay_ticks;

	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	delay_ticks = (unsigned long long) 
	    ( mx_hrt_counter_ticks_per_microsecond * (double) microseconds );

	start_time = mx_read_timebase();

	end_time = start_time + delay_ticks;

	while ( mx_read_timebase() < end_time )
		;				/* Do nothing. */

	return;
}

MX_EXPORT struct timespec
mx_high_resolution_time( void )
{
	struct timespec value;
	double time_in_microseconds;
	unsigned long long timebase_value;

	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	timebase_value = mx_read_timebase();

	time_in_microseconds = mx_divide_safely( (double) timebase_value,
					mx_hrt_counter_ticks_per_microsecond );

	value.tv_sec = (unsigned long)
			( 0.000001 * time_in_microseconds );

	time_in_microseconds -= 1000000.0 * (double) value.tv_sec;

	value.tv_nsec = (unsigned long) ( 1000.0 * time_in_microseconds );

	return value;
}

#if defined(OS_MACOSX)

#include <sys/sysctl.h>
#include <errno.h>

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	static const char fname[] = "mx_high_resolution_time_init()";

	int mib[2], tbfrequency;
	int status, saved_errno;
	size_t tbsize;

	mx_high_resolution_time_init_invoked = TRUE;

	mib[0] = CTL_HW;
	mib[1] = HW_TB_FREQ;

	tbsize = sizeof(tbfrequency);

	status = sysctl(mib, 2, &tbfrequency, &tbsize, NULL, 0);

	if ( status != 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_FUNCTION_FAILED, fname,
		"Attempt to get the PowerPC timebase frequency failed.  "
		"Errno = %d, error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
		return;
	}

	mx_hrt_counter_ticks_per_microsecond = 1.0e-6 * (double) tbfrequency;

	return;
}

#else	/* not OS_MACOSX */

#  define MX_NEED_HRT_INIT_FROM_FILE	TRUE

double mx_high_resolution_time_init_from_file( void );

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	unsigned long long timebase_before, timebase_after;

	mx_high_resolution_time_init_invoked = TRUE;

	/* Initially, we attempt to to read the calibration from the
	 * file $MXDIR/etc/mx_timer_calib.dat
	 */

	mx_hrt_counter_ticks_per_microsecond =
		mx_high_resolution_time_init_from_file();

	if ( mx_hrt_counter_ticks_per_microsecond > 0.0 ) {
		return;
	}

	/* If reading the calibration from a file did not succeed,
	 * then we try to compute the calibration right now.
	 */

	mx_info(
"Calibrating high resolution timer.  This will take around 10 seconds." );

	timebase_before = mx_read_timebase();

	sleep(10);

	timebase_after = mx_read_timebase();

	mx_hrt_counter_ticks_per_microsecond =
		1.0e-7 * (double) ( timebase_after - timebase_before );

	mx_info( "High resolution timer ticks per microsecond = %g",
		mx_hrt_counter_ticks_per_microsecond );

	return;
}
	
#endif

/****** End of GCC ******/

#else

/******* None of the above ********/

static int mx_high_resolution_time_init_invoked = FALSE;

MX_EXPORT void
mx_udelay( unsigned long microseconds )
{
	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	mx_usleep( microseconds );
}

MX_EXPORT struct timespec
mx_high_resolution_time( void )
{
	return mx_high_resolution_time_using_clock_ticks();
}

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	mx_high_resolution_time_init_invoked = TRUE;

	mx_warning( "High resolution time measurement is not yet implemented "
		"on this operating system." );
	return;
}

#endif

/*--------------------------------------------------------------------------*/

#if defined( MX_NEED_HRT_INIT_FROM_FILE )

double
mx_high_resolution_time_init_from_file( void )
{
	char calib_filename[MXU_FILENAME_LENGTH+1];
	FILE *calib_file;
	char buffer[80];
	int num_items;
	double hrt_counter_ticks_per_microsecond;
	mx_status_type mx_status;

	mx_status = mx_cfn_construct_filename( MX_CFN_CONFIG,
						"mx_timer_calib.dat",
						calib_filename,
						sizeof(calib_filename) );

	if ( mx_status.code != MXE_SUCCESS )
		return -1;

	calib_file = fopen( calib_filename, "r" );

	if ( calib_file == NULL )
		return -1;

	mx_fgets( buffer, sizeof(buffer), calib_file );

	fclose( calib_file );

	num_items = sscanf( buffer, "%lg", &hrt_counter_ticks_per_microsecond );

	if ( num_items != 1 ) {
		mx_warning(
		"The first line of timer calibration file '%s' did not contain "
		"an HRT 'counter ticks per microsecond' calibration value.  "
		"Instead, it contained '%s'.", calib_filename, buffer );

		return -1;
	}

	mx_info(
	"Setting high resolution timer to %g ticks per microsecond "
	"from calibration file '%s'.",
		hrt_counter_ticks_per_microsecond, calib_filename );

	return hrt_counter_ticks_per_microsecond;
}

#endif /* MX_NEED_HRT_INIT_FROM_FILE */


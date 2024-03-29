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
 * Copyright 2002-2004, 2006-2007, 2009-2012, 2014-2017, 2021-2023
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
#include "mx_time.h"
#include "mx_clock_tick.h"
#include "mx_hrt.h"

/*--------------------------------------------------------------------------*/

static mx_bool_type mx_high_resolution_time_init_invoked = FALSE;

/*--------------------------------------------------------------------------*/

static double mx_hrt_counter_ticks_per_microsecond = 0.0;

MX_EXPORT double
mx_get_hrt_counter_ticks_per_second( void )
{
	double result;

	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	result = 1000000.0 * mx_hrt_counter_ticks_per_microsecond;

	return result;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT double
mx_high_resolution_time_as_double( void )
{
	struct timespec hrt;
	double result;

	hrt = mx_high_resolution_time();

	result = mx_convert_timespec_time_to_seconds( hrt );

	return result;
}

/*--------------------------------------------------------------------------*/

static int mx_cpu_has_hardware_clock = TRUE;

MX_EXPORT int
mx_high_resolution_time_has_hardware_clock( void )
{
	return mx_cpu_has_hardware_clock;
}

/*--------------------------------------------------------------------------*/

/* The time returned by mx_high_resolution_time_using_clock_ticks() is
 * generally "high resolution" in name only.  But this function serves
 * as a fallback if there is no better method.
 */

MX_EXPORT struct timespec
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

/*--------------------------------------------------------------------------*/

/* This is intended for plaforms that do not provide a way to figure out
 * the rate at which the CPU clock ticks.
 */

MX_EXPORT double
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

/*==========================================================================*/

/* The following section defines primitives for reading the hardware
 * clock on various CPU architectures.
 */

#if defined(OS_WIN32)

/* FIXME: This probably does not work on all versions of Windows
 * and all compiler versions.  We will need to revisit this later.
 */

static inline uint64_t
mx_get_hrt_counter_tick( void )
{
	uint64_t x;

	x = __rdtsc();

	return x;
}

/******/

#elif defined(OS_QNX)

#include <sys/neutrino.h>
#include <inttypes.h>

static inline uint64_t
mx_get_hrt_counter_tick( void )
{
	uint64_t x;

	x = ClockCycles();

	return x;
}

/******/

#elif defined(OS_VXWORKS)

#include <tickLib.h>

static __inline__ uint64_t
mx_get_hrt_counter_tick( void )
{
	uint64_t x;

	x = tick64Get();

	return x;
}

/******/

#elif ( defined(__GNUC__) && defined(__x86_64__) )

/******* GCC on x86_64 *******/

/* Uses x86 RDTSC instruction */

static __inline__ uint64_t
mx_get_hrt_counter_tick( void )
{
	uint64_t x;

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

#elif ( defined(__GNUC__) && defined(__i386__) )

/******* GCC on x86 *******/

/* Uses x86 RDTSC instruction */

static __inline__ uint64_t
mx_get_hrt_counter_tick( void )
{
	MX_CLOCK_TICK current_clock_tick;
	uint64_t x;

#if ( defined(__i586__) || defined(__i686__) )
	/* i586 or above */

	mx_bool_type mx_hrt_use_clock_ticks = FALSE;
#else
	/* i386 or i486 */

	mx_bool_type mx_hrt_use_clock_ticks = TRUE;
#endif

	if ( mx_hrt_use_clock_ticks ) {
		current_clock_tick = mx_current_clock_tick();

		x = ( ULONG_MAX * (uint64_t) current_clock_tick.high_order )
			+ current_clock_tick.low_order;

		return x;
	}

	/* The following generates inline an RDTSC instruction
	 * and puts the results in the variable "x".
	 */

	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));

	return x;
}

#elif ( defined(__GNUC__) && ( defined(__ppc__) || defined(__powerpc__) ) )

/******* GCC on PowerPC *******/

/* Uses PowerPC TimeBase register */

static __inline__ uint64_t
mx_get_hrt_counter_tick( void )
{
	uint64_t timebase;
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

/******* GCC on 64-bit ARM (AARCH64) *******/

#elif ( defined(__GNUC__) && defined(__aarch64__) )

/* Uses ARM64 CNTVCT_EL0 system counter register */

/* Based on this web page:
 *    https://stackoverflow.com/questions/40454157/is-there-an-equivalent-instruction-to-rdtsc-in-arm
 */

static __inline__ uint64_t
mx_get_hrt_counter_tick( void )
{
	/* For Arm version 8.0 to 8.5, the system counter is at least
	 * 56 bits wide.  For Arm version 8.6 and onward, the counter
	 * must be 64 bits wide.
	 */

	uint64_t system_counter;

	asm volatile( "mrs %0, cntvct_el0" : "=r" (system_counter) );

	return system_counter;
}

/******* GCC on 32-bit ARM (for ARM Arch 6 and above) *******/

#elif ( defined(__GNUC__) && defined(__ARM_ARCH) && (__ARM_ARCH >= 6) )

/* Based on:
 *    https://github.com/google/benchmark/blob/v1.1.0/src/cycleclock.h
 * which was referenced by the 64-bit ARM case immediately above.
 */

static __inline__ uint64_t
mx_get_hrt_counter_tick( void )
{
	uint32_t pmccntr;
	uint32_t pmuseren;
	uint32_t pmcntenset;

	uint64_t system_counter;

	/* See if we have permission to read the performance counter. */

	asm volatile( "mrc p15, 0, %0, c9, c14, 0" : "=r"(pmuseren) );

	if ( (pmuseren & 1) == 0 ) {
		/* Permission denied. */
		return 0;
	}

	/* See if the performance counter is counting. */

	asm volatile( "mrc p15, 0, %0, c9, c12, 1" : "=r"(pmcntenset) );

	if ( (pmcntenset & 0x80000000ul) == 0 ) {
		/* Not counting. */
		return 0;
	}

	/* The counter counts every 64th cycle. */

	asm volatile( "mrc p15, 0, %0, c9, c13, 0" : "=r"(pmccntr) );

	system_counter = (uint64_t) ( pmccntr * 64 );

	return system_counter;
}

#elif 0

static __inline__ uint64_t
mx_get_hrt_counter_tick( void )
{
	struct timeval tv;

	uint64_t system_counter;

	gettimeofday( %tv, NULL );

	system_counter = (uint64_t) ( (tv.tv_sec * 1000000 + tv.tv_usec) );

	return system_counter;
}

#elif 0

static __inline__ uint64_t
mx_get_hrt_counter_tick( void )
{
	return 0;
}

#else
#error mx_get_hrt_counter_tick() not implemented for this platform.
#endif

/*--------------------------------------------------------------------------*/

#if defined(OS_WIN32)

/******* Microsoft Win32 *******/

#include <windows.h>

static int mx_performance_counters_are_available = FALSE;

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

/*==========================================================================*/

#elif defined(OS_VXWORKS)

#include <drv/timer/timerDev.h>

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	mx_hrt_counter_ticks_per_microsecond = 1.0e6 * sysClkRateGet();

	return;
}

/*==========================================================================*/

#elif defined(OS_LINUX) || defined(OS_CYGWIN) || defined(OS_MINIX) \
	|| defined(OS_ANDROID)

#define MX_HRT_USE_GENERIC	TRUE

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
	cpu_mhz = 0.0;

	have_tsc = FALSE;

	MXW_UNUSED(have_tsc);

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

#elif defined(OS_MACOSX) || defined(__FreeBSD__) || defined(__NetBSD__)

#define MX_HRT_USE_GENERIC	TRUE

#include <errno.h>
#include <sys/sysctl.h>

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	static const char fname[] = "mx_high_resolution_time_init()";

#if ( defined(OS_MACOSX) && defined(__aarch64__) )
	/* Note: hw.tbfrequency is apparently the bus frequency. */

	const char cpu_freq_mib_name[] = "hw.tbfrequency";

#elif ( defined(OS_MACOSX) )
	/* Note: hw.cpufrequency is just the nominal frequency,
	 * _not_ the current frequency.
	 */
	const char cpu_freq_mib_name[] = "hw.cpufrequency";

	/*==========================================================*/

	/* The rest of these definitions are for FreeBSD and NetBSD
	 * on various architectures.
	 */
#elif ( defined(__x86_64__) || defined(__i386__) )
	const char cpu_freq_mib_name[] = "machdep.tsc_freq";

#else
#  error cpu_freq_mib_name[] not defined for this build target.
#endif

	uint64_t cpu_freq = 0;
	size_t cpu_freq_length = sizeof(cpu_freq);
	int status, saved_errno;
	int i, max_attempts;

	mx_high_resolution_time_init_invoked = TRUE;

	/* On Arm64 macOS, sometimes the first time you read the value of
	 * hw.tbfrequency it returns 0, so you just try it again.
	 */

#if defined(OS_MACOSX)
	max_attempts = 2;
#else
	max_attempts = 1;
#endif

	for ( i = 0; i < max_attempts; i++ ) {
	    status = sysctlbyname( cpu_freq_mib_name,
			&cpu_freq, &cpu_freq_length, NULL, 0);

	    if ( status < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to read the value of '%s' "
		"using sysctlbyname() failed.  Errno = %d, error = '%s'",
			cpu_freq_mib_name, saved_errno, strerror(saved_errno) );
		return;
	    }

	    if ( cpu_freq > 0 ) {
		break;		/* Exit the for() loop. */
	    }
	}

#if 0
	MX_DEBUG(-2,("%s: cpu_freq = %llu",
		fname, (long long unsigned int) cpu_freq));
#endif

	mx_hrt_counter_ticks_per_microsecond = cpu_freq / 1000000LU;

	MX_DEBUG( 2,("%s: mx_hrt_counter_ticks_per_microsecond = %g",
		fname, mx_hrt_counter_ticks_per_microsecond));

	return;
}

/*--------------------------------------------------------------------------*/

#elif defined(__OpenBSD__)

/* For BSD-like platforms that cannot use sysctlbyname() for whatever reason. */

/* FIXME: The value returned for HW_CPUSPEED on OpenBSD does not seem to be
 *        the right answer, but we have not found any other CPU frequency mibs
 *        that give the "right" answer.
 */

#define MX_HRT_USE_GENERIC	TRUE

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

	mib[1] = HW_CPUSPEED;

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

#elif defined(OS_QNX)

#define MX_HRT_USE_GENERIC	TRUE

#include <sys/syspage.h>

MX_EXPORT void
mx_high_resolution_time_init( void )
{
	mx_hrt_counter_ticks_per_microsecond =
			1.0e-6 * SYSPAGE_ENTRY(qtime)->cycles_per_sec;

	return;
}

/*--------------------------------------------------------------------------*/

#elif defined(OS_HURD)

#define MX_HRT_USE_GENERIC	TRUE

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

	tsc_before = mx_get_hrt_counter_tick();

	sleep(10);

	tsc_after = mx_get_hrt_counter_tick();

	mx_hrt_counter_ticks_per_microsecond =
		1.0e-7 * (double) ( tsc_after - tsc_before );

	mx_info( "High resolution timer ticks per microsecond = %g",
		mx_hrt_counter_ticks_per_microsecond );

	return;
}

#else

#error No mx_high_resolution_time_init() defined.

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

#if defined( MX_HRT_USE_GENERIC )

MX_EXPORT void
mx_udelay( unsigned long microseconds )
{
	uint64_t start_time, end_time, delay_ticks;

	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	delay_ticks = (unsigned long long) 
	    ( mx_hrt_counter_ticks_per_microsecond * (double) microseconds );

	start_time = mx_get_hrt_counter_tick();

	end_time = start_time + delay_ticks;

	while ( mx_get_hrt_counter_tick() < end_time )
		;				/* Do nothing. */

	return;
}

MX_EXPORT struct timespec
mx_high_resolution_time( void )
{
	struct timespec value;
	double time_in_microseconds;
	uint64_t hrt_value;

	if ( mx_high_resolution_time_init_invoked == FALSE )
		mx_high_resolution_time_init();

	hrt_value = mx_get_hrt_counter_tick();

	time_in_microseconds = mx_divide_safely( (double) hrt_value,
					mx_hrt_counter_ticks_per_microsecond );

	value.tv_sec = (unsigned long)
			( 0.000001 * time_in_microseconds );

	time_in_microseconds -= 1000000.0 * (double) value.tv_sec;

	value.tv_nsec = (unsigned long) ( 1000.0 * time_in_microseconds );

	return value;
}

#endif

/*--------------------------------------------------------------------------*/


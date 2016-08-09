/*
 * Name:    mx_boot.c
 *
 * Purpose: Functions for displaying information about how the computer booted.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007-2008, 2011, 2015-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_BOOT_DEBUG	FALSE

#include <stdio.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_hrt.h"
#include "mx_clock.h"

/*---------------------- Win32 ----------------------*/

#if defined(OS_WIN32)

/* The following code was based on this MSDN example:
 *
 *   http://msdn2.microsoft.com/en-us/library/aa363675.aspx
 */

#include <windows.h>

#define EVENT_LOG_BUFFER_SIZE (1024 * 64)

MX_EXPORT mx_status_type
mx_get_system_boot_time( struct timespec *system_boot_timespec )
{
	static const char fname[] = "mx_get_system_boot_time()";

	HANDLE event_log_handle;
	EVENTLOGRECORD *event_log_pointer;
	BYTE buffer[ EVENT_LOG_BUFFER_SIZE ];
	DWORD bytes_read, bytes_left, bytes_needed;

	DWORD last_error_code;
	TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];

	mx_bool_type boot_event_found;
	int event_code;
	mx_status_type mx_status;

	if ( system_boot_timespec == (struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct timespec pointer passed was NULL." );
	}

	/* We can find out when the system booted from the System Event Log. */

	event_log_handle = OpenEventLog( NULL, "System" );

	if ( event_log_handle == NULL ) {
		last_error_code = GetLastError();

		if ( last_error_code == ERROR_CALL_NOT_IMPLEMENTED ) {
			/* If we get this error code, it probably means
			 * that we are running on a Windows 9x family
			 * operating system.  In that case, we fall back
			 * the ticks variant of this call.
			 */

			mx_status =  mx_get_system_boot_time_from_ticks(
							system_boot_timespec );

			return mx_status;
		}

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to open the Windows Event Log.  "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	/* Read the event log in reverse order looking for the last
	 * instance of an event of type 6009.  Event type 6009 is
	 * generated each time a Windows system boots and is normally
	 * the first event to be generated.
	 */

	boot_event_found = FALSE;

	event_log_pointer = (EVENTLOGRECORD *) &buffer;

	while ( ReadEventLog( event_log_handle,
			EVENTLOG_BACKWARDS_READ |
			EVENTLOG_SEQUENTIAL_READ,
			0,
			event_log_pointer,
			EVENT_LOG_BUFFER_SIZE,
			&bytes_read,
			&bytes_needed ) )
	{
		bytes_left = bytes_read;

		while ( bytes_left > 0 ) {

			event_code = event_log_pointer->EventID & 0xffff;

#if 0 && MX_BOOT_DEBUG
			MX_DEBUG(-2,
	   ("Event id = 0x%08X (%d), Event type = %lu, Event source = '%s'",
				event_log_pointer->EventID,
				event_code,
				event_log_pointer->EventType,
	    (LPSTR) ((LPBYTE) event_log_pointer + sizeof(EVENTLOGRECORD)) ));
#endif
			if ( event_code == 6009 ) {
				boot_event_found = TRUE;
				break;
			}

			bytes_left -= event_log_pointer->Length;

			event_log_pointer = (EVENTLOGRECORD *)
		    ((LPBYTE) event_log_pointer + event_log_pointer->Length);
		}

		if ( boot_event_found ) {
			break;
		}

		event_log_pointer = (EVENTLOGRECORD *) &buffer;
	}

	CloseEventLog( event_log_handle );

	if ( boot_event_found == FALSE ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The attempt to find the Windows boot time "
			"in the Event Log failed for an unknown reason." );
	}

	system_boot_timespec->tv_sec = event_log_pointer->TimeGenerated;
	system_boot_timespec->tv_nsec = 0;

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: system_boot_timespec = (%lu,%ld)", fname,
			system_boot_timespec->tv_sec,
			system_boot_timespec->tv_nsec));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------- VMS --------------------------*/

#elif defined(OS_VMS)

#include <math.h>

#include <ssdef.h>
#include <stsdef.h>
#include <syidef.h>
#include <descrip.h>
#include <lib$routines.h>

MX_EXPORT mx_status_type
mx_get_system_boot_time( struct timespec *system_boot_timespec )
{
	static const char fname[] = "mx_get_system_boot_time()";

	long item_code;
	int32_t boottime[2];
	unsigned long boottime_low, boottime_high;
	unsigned long unix_epoch_low, unix_epoch_high;
	unsigned long difference_low, difference_high;
	unsigned long nanoseconds;
	double hundreds_of_nanoseconds;
	double seconds_since_unix_epoch;
	struct timespec result;
	unsigned long vms_status;

	item_code = SYI$_BOOTTIME;

	vms_status = lib$getsyi( &item_code,
				boottime,
				0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Cannot get the boot time for this computer.  "
		"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );
	}

	/* VMS native time is a 64-bit integer and expresses the current
	 * time in terms of the number of 100 nanosecond ticks since the
	 * VMS Epoch of 00:00 on November 17, 1858.
	 */

	/* VAXes, Alphas, and Itaniums are little-endian. */

	boottime_low  = boottime[0];
	boottime_high = boottime[1];

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: VMS boot time = (%#lx,%#lx)",
		fname, boottime_high, boottime_low));
#endif

	/* The Unix Epoch of 00:00 on January 1, 1970 occurred at a 
	 * VMS time of 0x007c95674beb4000.
	 */

	unix_epoch_low = 0x4beb4000;
	unix_epoch_high = 0x007c9567;
	
#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: Unix Epoch = (%#lx,%#lx)",
		fname, unix_epoch_high, unix_epoch_low));
#endif

	/* Subtract the Unix Epoch from the VMS boot time.  This gives
	 * us the Unix boot time, but in units of 100 nanoseconds.
	 */

	difference_low  = boottime_low  - unix_epoch_low;
	difference_high = boottime_high - unix_epoch_high;

	/* Check for borrow. */

	if ( boottime_low < unix_epoch_low ) {
		difference_high--;
	}

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: Difference in VMS units = (%#lx,%#lx)",
		fname, difference_high, difference_low));
#endif

	/*--- Convert to struct timespec ---*/

	/* The nanosecond part is easy. */

	nanoseconds = 100L * ( difference_low % 10000000L );

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: nanoseconds = %lu (%#lx)",
		fname, nanoseconds, nanoseconds));
#endif
	difference_low -= nanoseconds;

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,
	("%s: Difference in integer seconds in VMS units = (%#lx,%#lx)",
		fname, difference_high, difference_low));
#endif

	/* At this point, it is easiest to convert the integer second
	 * part of the difference to a double precision floating point
	 * number, which should now be safe from roundoff.
	 */

	hundreds_of_nanoseconds = pow( 2.0, 32.0 ) * (double) difference_high;

	hundreds_of_nanoseconds += (double) difference_low;

	seconds_since_unix_epoch = hundreds_of_nanoseconds / 10000000.0;

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,
	("%s: hundreds_of_nanoseconds = %g, seconds since Unix epoch = %g",
		fname, hundreds_of_nanoseconds, seconds_since_unix_epoch));
#endif

	result.tv_sec = mx_round( seconds_since_unix_epoch );
	result.tv_nsec = nanoseconds;

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: result = (%lu,%lu)",
		fname, result.tv_sec, result.tv_nsec));
#endif

	*system_boot_timespec = result;

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------- Linux and Cygwin ----------------------*/

#elif defined(OS_LINUX) || defined(OS_CYGWIN) || defined(OS_ANDROID)

/* The following code has not been tested with Linux 2.2 kernels or before. */

MX_EXPORT mx_status_type
mx_get_system_boot_time( struct timespec *system_boot_timespec )
{
	static const char fname[] = "mx_get_system_boot_time()";

	FILE *proc_stat;
	char buffer[100];
	int saved_errno, num_items;
	unsigned long boot_time_in_seconds;

	if ( system_boot_timespec == (struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct timespec pointer passed was NULL." );
	}

	/* Find out when this computer booted from /proc/stat. */

	proc_stat = fopen( "/proc/stat", "r" );

	if ( proc_stat == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to open /proc/stat.  Errno = %d, error message = '%s'.",
			saved_errno, strerror(saved_errno) );
	}

	/* Read through the output from /proc/stat until we find a line that
	 * begins with the word 'btime'. */

	mx_fgets( buffer, sizeof(buffer), proc_stat );

	for(;;) {
		if ( feof(proc_stat) || ferror(proc_stat) ) {
			fclose(proc_stat);

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Did not find the line starting with 'btime' "
			"that was supposed to contain the boot time." );
		}

#if 0 && MX_BOOT_DEBUG
		MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif

		if ( strncmp( buffer, "btime", 5 ) == 0 ) {
			break;			/* Exit the for(;;) loop. */
		}

		mx_fgets( buffer, sizeof(buffer), proc_stat );
	}

	/* Parse the line that contains the boot time. */

	num_items = sscanf( buffer, "btime %lu", &boot_time_in_seconds );

	if ( num_items != 1 ) {
		fclose(proc_stat);

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The system boot time could not be found in "
			"the line '%s' returned by /proc/stat.",
				buffer );
	}

	system_boot_timespec->tv_sec = boot_time_in_seconds;
	system_boot_timespec->tv_nsec = 0;

	fclose(proc_stat);

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: system_boot_timespec = (%lu,%ld)", fname,
			system_boot_timespec->tv_sec,
			system_boot_timespec->tv_nsec));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------- MacOS X and BSD ----------------------*/

#elif defined(OS_MACOSX) || defined(OS_BSD) || defined(OS_MINIX)

#include <sys/sysctl.h>

MX_EXPORT mx_status_type
mx_get_system_boot_time( struct timespec *system_boot_timespec )
{
	static const char fname[] = "mx_get_system_boot_time()";

	struct timeval boot_time;
	size_t boot_time_size;
	int mib[2];
	int os_status, saved_errno;

	if ( system_boot_timespec == (struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct timespec pointer passed was NULL." );
	}

	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;

	boot_time_size = sizeof(boot_time);

	os_status = sysctl( mib, 2, &boot_time, &boot_time_size, NULL, 0 );

	if ( os_status == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get KERN_BOOTTIME from the kernel.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	system_boot_timespec->tv_sec = boot_time.tv_sec;
	system_boot_timespec->tv_nsec = 1000L * boot_time.tv_usec;

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: system_boot_timespec = (%lu,%ld)", fname,
		(unsigned long) system_boot_timespec->tv_sec,
			system_boot_timespec->tv_nsec));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------- Solaris and Irix ----------------------*/

#elif defined(OS_SOLARIS) || defined(OS_IRIX) || defined(OS_UNIXWARE)

#include <utmpx.h>

MX_EXPORT mx_status_type
mx_get_system_boot_time( struct timespec *system_boot_timespec )
{
	static const char fname[] = "mx_get_system_boot_time()";

	struct utmpx *entry;
	mx_bool_type found_system_boot;

	if ( system_boot_timespec == (struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct timespec pointer passed was NULL." );
	}

	found_system_boot = FALSE;

	while ( ( entry = getutxent() ) ) {
		if ( strcmp( "system boot", entry->ut_line ) == 0 ) {
			found_system_boot = TRUE;
			break;			/* Exit the while() loop. */
		}
	}

	if ( found_system_boot == FALSE ) {
		endutxent();

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Did not find a 'system boot' entry in utmpx." );
	}

	system_boot_timespec->tv_sec = entry->ut_tv.tv_sec;
	system_boot_timespec->tv_nsec = 1000L * entry->ut_tv.tv_usec;

	endutxent();

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: system_boot_timespec = (%lu,%ld)", fname,
			system_boot_timespec->tv_sec,
			system_boot_timespec->tv_nsec));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------- HP-UX ----------------------*/

#elif defined(OS_HPUX)

#include <sys/param.h>
#include <sys/pstat.h>

MX_EXPORT mx_status_type
mx_get_system_boot_time( struct timespec *system_boot_timespec )
{
	static const char fname[] = "mx_get_system_boot_time()";

	struct pst_static pst;

	if ( system_boot_timespec == (struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct timespec pointer passed was NULL." );
	}

	pstat_getstatic( &pst, sizeof(pst), (size_t) 1, 0 );

	system_boot_timespec->tv_sec = pst.boot_time;
	system_boot_timespec->tv_nsec = 0;

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: system_boot_timespec = (%lu,%ld)", fname,
			system_boot_timespec->tv_sec,
			system_boot_timespec->tv_nsec));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------- Tru64 ----------------------*/

#elif defined(OS_TRU64)

#include <sys/table.h>

MX_EXPORT mx_status_type
mx_get_system_boot_time( struct timespec *system_boot_timespec )
{
	static const char fname[] = "mx_get_system_boot_time()";

	struct tbl_sysinfo system_info;

	if ( system_boot_timespec == (struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct timespec pointer passed was NULL." );
	}

	table( TBL_SYSINFO, 0, &system_info, 1, sizeof(system_info) );

	system_boot_timespec->tv_sec = system_info.si_boottime;
	system_boot_timespec->tv_nsec = 0;

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: system_boot_timespec = (%lu,%ld)", fname,
			system_boot_timespec->tv_sec,
			system_boot_timespec->tv_nsec));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------- Use clock ticks and time() ----------------------*/

#elif defined(OS_ECOS) || defined(OS_RTEMS) || defined(OS_VXWORKS) \
	|| defined(OS_QNX) || defined(OS_DJGPP) || defined(OS_HURD)

/* For platforms that return a valid value for time() and which also report
 * the number of clock ticks since boot, we can calculate the boot time.
 */

MX_EXPORT mx_status_type
mx_get_system_boot_time( struct timespec *system_boot_timespec )
{
	return mx_get_system_boot_time_from_ticks( system_boot_timespec );
}

/*---------------------- Feature not available ----------------------*/

#elif 0

/* Some platforms do not record the boot time. */

MX_EXPORT mx_status_type
mx_get_system_boot_time( struct timespec *system_boot_timespec )
{
	static const char fname[] = "mx_get_system_boot_time()";

	if ( system_boot_timespec == (struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct timespec pointer passed was NULL." );
	}

	system_boot_timespec->tv_sec  = 0;
	system_boot_timespec->tv_nsec = 0;

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: system_boot_timespec = (%lu,%ld)", fname,
		(unsigned long) system_boot_timespec->tv_sec,
			system_boot_timespec->tv_nsec));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------- Unknown ----------------------*/

#else
#error mx_get_system_boot_time() has not yet been written for this platform.
#endif

/***************************************************************************/

/* For platforms that return a valid value for time() and which also report
 * the number of clock ticks since boot, we can calculate the boot time
 * from that.
 */

MX_EXPORT mx_status_type
mx_get_system_boot_time_from_ticks( struct timespec *system_boot_timespec )
{
	static const char fname[] = "mx_get_system_boot_time_from_ticks()";

	MX_CLOCK_TICK clock_ticks_since_boot;
	double seconds_since_the_epoch, seconds_since_boot;
	double boot_time_in_seconds;

	if ( system_boot_timespec == (struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct timespec pointer passed was NULL." );
	}

	seconds_since_the_epoch = time(NULL);
	clock_ticks_since_boot = mx_current_clock_tick();

	seconds_since_boot =
		mx_convert_clock_ticks_to_seconds( clock_ticks_since_boot );

	boot_time_in_seconds = seconds_since_the_epoch - seconds_since_boot;

	*system_boot_timespec =
	    mx_convert_seconds_to_high_resolution_time( boot_time_in_seconds );

#if MX_BOOT_DEBUG
	MX_DEBUG(-2,("%s: system_boot_timespec = (%lu,%ld)", fname,
		(unsigned long) system_boot_timespec->tv_sec,
			system_boot_timespec->tv_nsec));
#endif

	return MX_SUCCESSFUL_RESULT;
}


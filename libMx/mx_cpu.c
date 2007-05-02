/*
 * Name:    mx_cpu.c
 *
 * Purpose: Functions for controlling the use by programs of the available
 *          CPUs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_CPU_DEBUG	FALSE

#include <stdio.h>

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include "mx_util.h"

/*
 * Process affinity masks are used in multiprocessor systems to constrain
 * the set of CPUs on which the process is eligible to run.  For MX, this
 * is only likely to be an issue for real time processing.
 */

/*------------------------------ Win32 ------------------------------*/

#if defined(OS_WIN32)

MX_EXPORT mx_status_type
mx_get_process_affinity_mask( unsigned long process_id,
				unsigned long *mask )
{
	static const char fname[] = "mx_get_process_affinity_mask()";

	HANDLE process_handle, process_pseudo_handle;
	DWORD process_affinity_mask, system_affinity_mask;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	BOOL os_status;

	if ( mask == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The affinity mask pointer passed was NULL." );
	}

	if ( process_id == 0 ) {
		/* If process_id is 0, then we use the current process. */

		process_id = GetCurrentProcessId();
	}

	process_handle = OpenProcess( PROCESS_QUERY_INFORMATION,
						FALSE, process_id );

	if ( process_handle == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to get a handle for process id %lu.  "
			"Win32 error code = %ld, error message = '%s'",
				process_id, last_error_code, message_buffer );	
	}

	os_status = GetProcessAffinityMask( process_handle,
						&process_affinity_mask,
						&system_affinity_mask );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		CloseHandle( process_handle );

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to get the process affinity mask "
			"for process id %lu.  "
			"Win32 error code = %ld, error message = '%s'",
				process_id, last_error_code, message_buffer );	
	}

	*mask = process_affinity_mask;

#if MX_CPU_DEBUG
	MX_DEBUG(-2,("%s: process %lu CPU affinity mask = %#x",
		fname, process_id, *mask));
#endif

	CloseHandle( process_handle );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_set_process_affinity_mask( unsigned long process_id,
				unsigned long mask )
{
	static const char fname[] = "mx_set_process_affinity_mask()";

	HANDLE process_handle, process_pseudo_handle;
	DWORD process_affinity_mask, system_affinity_mask;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	BOOL os_status;

	if ( process_id == 0 ) {
		/* If process_id is 0, then we use the current process. */

		process_id = GetCurrentProcessId();
	}

#if MX_CPU_DEBUG
	MX_DEBUG(-2,("%s: setting process %lu CPU affinity mask to %#x",
		fname, process_id, mask));
#endif

	process_handle = OpenProcess( PROCESS_QUERY_INFORMATION
					| PROCESS_SET_INFORMATION,
						FALSE, process_id );

	if ( process_handle == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to get a handle for process id %lu.  "
			"Win32 error code = %ld, error message = '%s'",
				process_id, last_error_code, message_buffer );	
	}

	os_status = SetProcessAffinityMask( process_handle, mask );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		CloseHandle( process_handle );


		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to get the process affinity mask "
			"for process id %lu.  "
			"Win32 error code = %ld, error message = '%s'",
				process_id, last_error_code, message_buffer );	
	}

	CloseHandle( process_handle );

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------ Linux ------------------------------*/

#elif defined(OS_LINUX)

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#define __USE_GNU

#include <sched.h>

#include "mx_program_model.h"

MX_EXPORT mx_status_type
mx_get_process_affinity_mask( unsigned long process_id,
				unsigned long *mask )
{
	static const char fname[] = "mx_get_process_affinity_mask()";

	cpu_set_t cpu_set_struct;
	int os_status, saved_errno;
	unsigned long i, num_bits;

	if ( mask == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The affinity mask pointer passed was NULL." );
	}

	if ( process_id == 0 ) {
		/* If process_id is 0, then we use the current process. */

		process_id = getpid();
	}

	CPU_ZERO( &cpu_set_struct );

	os_status = sched_getaffinity( process_id,
				CPU_SETSIZE, &cpu_set_struct );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get the process affinity mask "
		"for process id %lu.  "
		"Errno = %d, error message = '%s'",
			process_id, saved_errno, strerror(saved_errno) );
	}

	num_bits = MX_WORDSIZE;

	*mask = 0;

	for ( i = 0; i < num_bits; i++ ) {

		if ( CPU_ISSET(i, &cpu_set_struct) ) {
			*mask |= (1UL << i);
		}
	}

#if MX_CPU_DEBUG
	MX_DEBUG(-2,("%s: process %lu CPU affinity mask = %#lx",
		fname, process_id, *mask));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_set_process_affinity_mask( unsigned long process_id,
				unsigned long mask )
{
	static const char fname[] = "mx_set_process_affinity_mask()";

	cpu_set_t cpu_set_struct;
	int os_status, saved_errno;
	unsigned long i, num_bits;

	if ( process_id == 0 ) {
		/* If process_id is 0, then we use the current process. */

		process_id = getpid();
	}

#if MX_CPU_DEBUG
	MX_DEBUG(-2,("%s: setting process %lu CPU affinity mask to %#lx",
		fname, process_id, mask));
#endif

	num_bits = MX_WORDSIZE;

	CPU_ZERO( &cpu_set_struct );

	for ( i = 0; i < num_bits; i++ ) {
		if ( mask & (1UL << i) ) {
			CPU_SET( i, &cpu_set_struct );
		}
	}

	os_status = sched_setaffinity( process_id,
				CPU_SETSIZE, &cpu_set_struct );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to set the process affinity mask "
		"for process id %lu.  "
		"Errno = %d, error message = '%s'",
			process_id, saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_CYGWIN)

MX_EXPORT mx_status_type
mx_get_process_affinity_mask( unsigned long process_id,
				unsigned long *mask )
{
	static const char fname[] = "mx_get_process_affinity_mask()";

	return mx_error( MXE_UNSUPPORTED, fname,
"Getting the CPU affinity mask is not currently supported on this platform." );
}

MX_EXPORT mx_status_type
mx_set_process_affinity_mask( unsigned long process_id,
				unsigned long mask )
{
	static const char fname[] = "mx_set_process_affinity_mask()";

	return mx_error( MXE_UNSUPPORTED, fname,
"Setting the CPU affinity mask is not currently supported on this platform." );
}

#else

#error CPU affinity mask functions have not yet been defined for this platform.

#endif


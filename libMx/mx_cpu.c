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

	cpu_set_t cpu_set;
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

	CPU_ZERO( &cpu_set );

	os_status = sched_getaffinity( process_id, sizeof(cpu_set), &cpu_set );

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

		if ( CPU_ISSET(i, &cpu_set) ) {
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

	cpu_set_t cpu_set;
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

	CPU_ZERO( &cpu_set );

	for ( i = 0; i < num_bits; i++ ) {
		if ( mask & (1UL << i) ) {
			CPU_SET( i, &cpu_set );
		}
	}

	os_status = sched_setaffinity( process_id, sizeof(cpu_set), &cpu_set );

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

/*------------------------------ Solaris ------------------------------*/

#elif defined(OS_SOLARIS)

#include <errno.h>
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>

#include "mx_program_model.h"

MX_EXPORT mx_status_type
mx_get_process_affinity_mask( unsigned long process_id,
				unsigned long *mask )
{
	static const char fname[] = "mx_get_process_affinity_mask()";

	processorid_t obind;
	int os_status, saved_errno;

	if ( mask == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The affinity mask pointer passed was NULL." );
	}

	if ( process_id == 0 ) {
		os_status = processor_bind( P_PID, P_MYID,
						PBIND_QUERY, &obind );
	} else {
		os_status = processor_bind( P_PID, process_id,
						PBIND_QUERY, &obind );
	}

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the process affinity mask failed."
		"Errno = %d, error message = '%s'.",
			saved_errno, strerror(saved_errno) );
	}

	*mask = 1 << obind;

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

	processorid_t processorid;
	unsigned long allbits, testmask;
	int i, os_status, saved_errno;

#if MX_CPU_DEBUG
	{
		unsigned long temp_id;

		if ( process_id == 0 ) {
			temp_id = getpid();
		} else {
			temp_id = process_id;
		}

		MX_DEBUG(-2,
		("%s: setting process %lu CPU affinity mask to %#lx",
			fname, temp_id, mask));
	}
#endif

	/* Either all bits should be set in the mask or only one bit. */

#if ( MX_WORDSIZE == 32 )
	allbits = 0xffffffff;
#else
	allbits = 0xffffffffffffffff;
#endif

	if ( mask == allbits ) {
		processorid = PBIND_NONE;
	} else
	if ( mask == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The supplied affinity mask of %lu is illegal since, "
		"in principle, it would not allow the process to run "
		"on _any_ of the CPUs.", mask );
	} else {
		/* Look for the first bit set. */

		testmask = mask;

		for ( i = 0; i < MX_WORDSIZE; i++ ) {
			if ( testmask & 0x1 ) {
				/* See if more than one bit is set. */

				testmask >>= 1;

				if ( testmask != 0 ) {
					return mx_error( MXE_UNSUPPORTED, fname,
			"On Solaris, either all bits in the processor affinity "
			"mask should be set or only one bit should be set.  "
			"The supplied bitmask of %#lx is not supported.", mask);
				}

				/* We have found the correct processor id. */

				processorid = i;
				break;		/* Exit the for() loop. */
			}
			testmask >>= 1;
		}
	}

	if ( process_id == 0 ) {
		os_status = processor_bind( P_PID, P_MYID,
						processorid, NULL );
	} else {
		os_status = processor_bind( P_PID, process_id,
						processorid, NULL );
	}

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to set the process affinity mask to %#lx failed.  "
		"Errno = %d, error message = '%s'.",
			mask, saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------ Irix ------------------------------*/

#elif defined(OS_IRIX)

/* FIXME: This code is untested and probably does not work as is. */

#include <pthread.h>

#include "mx_unistd.h"
#include "mx_program_model.h"

MX_EXPORT mx_status_type
mx_get_process_affinity_mask( unsigned long process_id,
				unsigned long *mask )
{
	static const char fname[] = "mx_get_process_affinity_mask()";

	int cur_cpu;
	int os_status, saved_errno;

	if ( mask == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The affinity mask pointer passed was NULL." );
	}

	if ( process_id == 0 ) {
		process_id = getpid();
	}

	os_status = pthread_getrunon_np( &cur_cpu );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the process affinity mask failed."
		"Errno = %d, error message = '%s'.",
			saved_errno, strerror(saved_errno) );
	}

	*mask = 1 << cur_cpu;

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

	unsigned long allbits, testmask;
	int i, cpu, os_status, saved_errno;

	/* Either all bits should be set in the mask or only one bit. */

#if ( MX_WORDSIZE == 32 )
	allbits = 0xffffffff;
#else
	allbits = 0xffffffffffffffff;
#endif
	cpu = 0;

	if ( mask == allbits ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Explicitly requesting that a process be runnable on all CPUs "
		"is not supported under Irix." );
	} else
	if ( mask == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The supplied affinity mask of %lu is illegal since, "
		"in principle, it would not allow the process to run "
		"on _any_ of the CPUs.", mask );
	} else {
		/* Look for the first bit set. */

		testmask = mask;

		for ( i = 0; i < MX_WORDSIZE; i++ ) {
			if ( testmask & 0x1 ) {
				/* See if more than one bit is set. */

				testmask >>= 1;

				if ( testmask != 0 ) {
					return mx_error( MXE_UNSUPPORTED, fname,
			"On Irix, either all bits in the processor affinity "
			"mask should be set or only one bit should be set.  "
			"The supplied bitmask of %#lx is not supported.", mask);
				}

				/* We have found the correct cpu_number. */

				cpu = i;
				break;		/* Exit the for() loop. */
			}
			testmask >>= 1;
		}
	}

	if ( process_id == 0 ) {
		os_status = pthread_setrunon_np( cpu );
	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Changing the proccessor affinity mask for another process "
		"is not yet implemented for Irix." );
	}

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to set the process affinity mask to %#lx failed.  "
		"Errno = %d, error message = '%s'.",
			mask, saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------ Unsupported ------------------------------*/

#elif defined(OS_MACOSX) || defined(OS_CYGWIN)

/* FIXME for OS_MACOSX:
 *        If you have the CHUD package installed on MacOS X, CHUD apparently
 *        contains the function utilBindThreadToCPU().  However, most machines
 *        do not have CHUD on them, so I can't rely on its presence.  Also,
 *        even in the context of CHUD, utilBindThreadToCPU() is stated to be
 *        undocumented.
 */

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


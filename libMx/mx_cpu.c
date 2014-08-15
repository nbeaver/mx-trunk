/*
 * Name:    mx_cpu.c
 *
 * Purpose: Functions for controlling the use of the available CPUs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007-2011, 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_CPU_DEBUG	FALSE

#include <stdio.h>
#include <errno.h>

#if defined(OS_WIN32)
#include <windows.h>
#endif

#if defined(OS_MACOSX)
#include <sys/sysctl.h>
#endif

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_program_model.h"

/*===================================================================*/

/* For multithreaded programs, mx_get_number_of_cpu_cores() can be used
 * to find out how many processor cores are available.  This can be used
 * to figure out how many threads to create for parallel operations such
 * as copying multiple files.
 *
 * Inspired by
 *     http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
 */

/*------------------------------ Win32 ------------------------------*/

#if defined(OS_WIN32)

MX_EXPORT mx_status_type
mx_get_number_of_cpu_cores( unsigned long *num_cores )
{
	static const char fname[] = "mx_get_number_of_cpu_cores()";

	SYSTEM_INFO sysinfo;

	if ( num_cores == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_cores pointer passed was NULL." );
	}

	GetSystemInfo( &sysinfo );

	*num_cores = sysinfo.dwNumberOfProcessors;

	return MX_SUCCESSFUL_RESULT;
}

/*----------------------- Linux, Solaris, AIX -----------------------*/

#elif defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_AIX) \
	|| defined(OS_UNIXWARE)

MX_EXPORT mx_status_type
mx_get_number_of_cpu_cores( unsigned long *num_cores )
{
	static const char fname[] = "mx_get_number_of_cpu_cores()";

	if ( num_cores == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_cores pointer passed was NULL." );
	}

	*num_cores = sysconf( _SC_NPROCESSORS_ONLN );

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------- Irix -----------------------------*/

#elif defined(OS_IRIX)

MX_EXPORT mx_status_type
mx_get_number_of_cpu_cores( unsigned long *num_cores )
{
	static const char fname[] = "mx_get_number_of_cpu_cores()";

	if ( num_cores == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_cores pointer passed was NULL." );
	}

	*num_cores = sysconf( _SC_NPROC_ONLN );

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------- HP/UX ----------------------------*/

#elif defined(OS_HPUX)

MX_EXPORT mx_status_type
mx_get_number_of_cpu_cores( unsigned long *num_cores )
{
	static const char fname[] = "mx_get_number_of_cpu_cores()";

	if ( num_cores == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_cores pointer passed was NULL." );
	}

	*num_cores = mpctl( MPC_GETNUMSPUS, NULL, NULL );

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------- MacOS X, BSD ------------------------*/

#elif defined(OS_MACOSX) || defined(OS_BSD)

MX_EXPORT mx_status_type
mx_get_number_of_cpu_cores( unsigned long *num_cores )
{
	static const char fname[] = "mx_get_number_of_cpu_cores()";

	int os_status, saved_errno;
	int mib[4];
	size_t ul_length = sizeof(unsigned long);

	if ( num_cores == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_cores pointer passed was NULL." );
	}

	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;

	os_status = sysctl( mib, 2, num_cores, &ul_length, NULL, 0 );

#if ( MX_WORDSIZE > 32 )
	(*num_cores) &= 0xffffffff;
#endif

	if ( os_status != 0 ) {
		saved_errno = errno;

		if ( saved_errno != ENOENT ) {
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"An attempt to read the value of HW_AVAILCPU failed.  "
			"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
		} else {
			/* If we get here, HW_AVAILCPU does not exist,
			 * so we try HW_NCPU instead.
			 */

			mib[1] = HW_NCPU;

			os_status = sysctl( mib, 2,
					num_cores, &ul_length, NULL, 0 );
#if ( MX_WORDSIZE > 32 )
			(*num_cores) &= 0xffffffff;
#endif
			if ( os_status != 0 ) {
				saved_errno = errno;

				if ( saved_errno != ENOENT ) {
					return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
					"An attempt to read the value of "
					"HW_NCPU failed.  "
					"Errno = %d, error message = '%s'",
						saved_errno,
						strerror(saved_errno) );
				} else {
					*num_cores = 1;
				}
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------- Hurd ------------------------*/

#elif defined(OS_HURD)

MX_EXPORT mx_status_type
mx_get_number_of_cpu_cores( unsigned long *num_cores )
{
	static const char fname[] = "mx_get_number_of_cpu_cores()";

	if ( num_cores == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_cores pointer passed was NULL." );
	}

	*num_cores = 1;

	return MX_SUCCESSFUL_RESULT;
}

#else
#  error mx_get_number_of_cpu_cores() not yet implemented for this platform.
#endif

/*===================================================================*/

#if ( defined(OS_WIN32) && defined(_MSC_VER) )

static DWORD
mxp_get_current_processor_number( void )
{
	_asm {mov eax, 1}
	_asm {cpuid}
	_asm {shr ebx, 24}
	_asm {mov eax, ebx}
}

MX_EXPORT unsigned long
mx_get_current_cpu_number( void )
{
	unsigned long cpu_number;

	cpu_number =  mxp_get_current_processor_number();

	return cpu_number;
}

#elif ( defined(MX_GLIBC_VERSION) && (MX_GLIBC_VERSION >= 2006000L) )

extern int sched_getcpu( void );

MX_EXPORT unsigned long
mx_get_current_cpu_number( void )
{
	unsigned long cpu_number;

	cpu_number = sched_getcpu();

	return cpu_number;
}

#else
#  error mx_get_current_cpu_number() not yet implemented for this platform.
#endif

/*===================================================================*/

/*
 * Process affinity masks are used in multiprocessor systems to constrain
 * the set of CPUs on which the process is eligible to run.  For MX, this
 * is only likely to be an issue for real time processing.
 */

/*------------------------------ Win32 ------------------------------*/

#if ( defined(OS_WIN32) && (_MSC_VER >= 1100) )

MX_EXPORT mx_status_type
mx_get_process_affinity_mask( unsigned long process_id,
				unsigned long *mask )
{
	static const char fname[] = "mx_get_process_affinity_mask()";

#if defined(_M_AMD64)
	DWORD_PTR process_affinity_mask, system_affinity_mask;
#else
	DWORD process_affinity_mask, system_affinity_mask;
#endif
	HANDLE process_handle;
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

	HANDLE process_handle;
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

#include "mx_version.h"
#include "mx_program_model.h"

#  if ( MX_GLIBC_VERSION < 2003004L )

	/* Linux with Glibc 2.3.3 or before.
	 * 
	 * Although the affinity interfaces were added at the start
	 * of the Glibc 2.3 series, the interfaces did not stabilize
	 * until 2.3.4.
	 */

MX_EXPORT mx_status_type
mx_get_process_affinity_mask( unsigned long process_id,
				unsigned long *mask )
{
	static const char fname[] = "mx_get_process_affinity_mask()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Getting the CPU affinity mask is not supported on Linux "
	"if MX is compiled with Glibc 2.3.3 or before." );
}

MX_EXPORT mx_status_type
mx_set_process_affinity_mask( unsigned long process_id,
				unsigned long mask )
{
	static const char fname[] = "mx_set_process_affinity_mask()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Setting the CPU affinity mask is not supported on Linux "
	"if MX is compiled with Glibc 2.3.3 or before." );
}

#  else
	/* Linux with Glibc 2.3.4 and above. */

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

#  endif /* Linux with Glibc 2.3.4 and above. */

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

/*------------------------------ OpenVMS ------------------------------*/

#elif defined(OS_VMS)

#if defined(__VAX)

MX_EXPORT mx_status_type
mx_get_process_affinity_mask( unsigned long process_id,
				unsigned long *mask )
{
	static const char fname[] = "mx_get_process_affinity_mask()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"VAX computers do not support a CPU affinity mask." );
}

MX_EXPORT mx_status_type
mx_set_process_affinity_mask( unsigned long process_id,
				unsigned long mask )
{
	static const char fname[] = "mx_set_process_affinity_mask()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"VAX computers do not support a CPU affinity mask." );
}

#else  /* Alpha or Itanium */

#include "mx_stdint.h"

#include <errno.h>
#include <ssdef.h>
#include <iodef.h>
#include <starlet.h>

MX_EXPORT mx_status_type
mx_get_process_affinity_mask( unsigned long process_id,
				unsigned long *mask )
{
	static const char fname[] = "mx_get_process_affinity_mask()";

	unsigned int pidadr;
	uint64_t prev_mask;
	int vms_status;

	if ( process_id == 0 ) {
		process_id = mx_process_id();
	}

	vms_status = sys$process_affinity( 0, 0, 0, 0, &prev_mask, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the process affinity mask failed "
		"with a VMS status of %d.  Error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );
	}

	*mask = prev_mask;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_set_process_affinity_mask( unsigned long process_id,
				unsigned long mask )
{
	static const char fname[] = "mx_set_process_affinity_mask()";

	unsigned int pidadr;
	uint64_t select_mask, modify_mask;
	int vms_status;

	if ( process_id == 0 ) {
		process_id = mx_process_id();
	}

	select_mask = (~0);
	modify_mask = mask;

	vms_status = sys$process_affinity( 0, 0,
				&select_mask, &modify_mask, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the process affinity mask failed "
		"with a VMS status of %d.  Error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif

/*------------------------------ QNX ------------------------------*/

#elif defined(OS_QNX)

#include <sys/neutrino.h>
#include <errno.h>

static int affinity_mask = ~0;

MX_EXPORT mx_status_type
mx_get_process_affinity_mask( unsigned long process_id,
				unsigned long *mask )
{
	static const char fname[] = "mx_get_process_affinity_mask()";

	if ( (process_id == 0) || (process_id == mx_process_id()) )
	{
		*mask = affinity_mask;

		return MX_SUCCESSFUL_RESULT;
	}

	return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the CPU affinity mask for a different process "
		"is not supported on QNX." );
}

MX_EXPORT mx_status_type
mx_set_process_affinity_mask( unsigned long process_id,
				unsigned long mask )
{
	static const char fname[] = "mx_set_process_affinity_mask()";

	int runmask, os_status, saved_errno;

	if ( (process_id != 0) && (process_id != mx_process_id()) )
	{
		return mx_error( MXE_UNSUPPORTED, fname,
			"Setting the CPU affinity mask for a different process "
			"is not supported on QNX." );
	}

	runmask = (int) mask;

	os_status = ThreadCtl( _NTO_TCTL_RUNMASK, &runmask );

	if ( os_status == (-1) ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An error occurred while attempting to set the "
		"CPU affinity to %#lx.  Errno = %d, error message = '%s'",
			mask, saved_errno, strerror(saved_errno) );
	}

	affinity_mask = (int) mask;

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------ Unsupported ------------------------------*/

#elif defined(OS_MACOSX) || defined(OS_CYGWIN) || defined(OS_ECOS) \
	|| defined(OS_RTEMS) || defined(OS_VXWORKS) || defined(OS_BSD) \
	|| defined(OS_HPUX) || defined(OS_TRU64) || defined(OS_DJGPP) \
	|| defined(OS_WIN32) || defined(OS_UNIXWARE) || defined(OS_HURD)

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


/*
 * Name:    mx_memory_system.c
 *
 * Purpose: This file contains functions that report the memory usage
 *          of a computer as a whole.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005-2007, 2010-2012, 2015-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_memory.h"

MX_EXPORT void
mx_display_system_meminfo( MX_SYSTEM_MEMINFO *meminfo )
{
	static const char fname[] = "mx_display_system_meminfo()";

	if ( meminfo == (MX_SYSTEM_MEMINFO *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYSTEM_MEMINFO pointer passed was NULL." );

		return;
	}

	mx_info("  total_bytes     = %lu",(unsigned long) meminfo->total_bytes);
	mx_info("  used_bytes      = %lu",(unsigned long) meminfo->used_bytes);
	mx_info("  free_bytes      = %lu",(unsigned long) meminfo->free_bytes);
	mx_info("  swap_bytes      = %lu",(unsigned long) meminfo->swap_bytes);
	mx_info("  swap_used_bytes = %lu",
				(unsigned long) meminfo->swap_used_bytes);
	mx_info("  swap_free_bytes = %lu",
				(unsigned long) meminfo->swap_free_bytes);
	return;
}

/***************************************************************************/

#if defined( OS_LINUX ) || defined( OS_ANDROID ) || defined( OS_HURD )

/* For system memory information, we read from /proc/meminfo. */

MX_EXPORT mx_status_type
mx_get_system_meminfo( MX_SYSTEM_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_system_meminfo()";

	FILE *procfile;
	int saved_errno, num_items;
	unsigned long field_value;
	char field_name[80];
	char buffer[80];

	if ( meminfo == (MX_SYSTEM_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYSTEM_MEMINFO pointer passed was NULL." );
	}

	memset( meminfo, 0, sizeof( MX_SYSTEM_MEMINFO ) );

	procfile = fopen( "/proc/meminfo", "r" );

	if ( procfile == (FILE *) NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
	    "Cannot open proc filesystem file '/proc/meminfo'.  Reason = '%s'",
			strerror( saved_errno ) );
	}

	/* Loop over the lines of output from /proc/meminfo. */

	while (1) {
		/* Get a line of text from /proc/meminfo. */

		mx_fgets( buffer, sizeof(buffer), procfile );

		if ( feof(procfile) ) {
			/* We have read the last line from /proc/meminfo,
			 * so break out of the while loop.
			 */

			break;
		}

		if ( ferror(procfile) ) {
			saved_errno = errno;

			(void) fclose(procfile);
			return mx_error( MXE_FILE_IO_ERROR, fname,
		"Error reading data from /proc/meminfo.  Error = '%s'",
						strerror( saved_errno ) );
		}

		/* Attempt to parse output lines of the form
		 *
		 *    MemTotal:      256424 kB
		 *
		 * Note that not all lines of output have this format.
		 * Lines that do not match should be skipped.
		 */

		num_items = sscanf( buffer, "%s %lu kB",
					field_name, &field_value );

		if ( num_items != 2 ) {
			/* This line does not contain the type of data
			 * that we are looking for, so go back for
			 * the next line.
			 */

			continue;
		}

		/* Look for matching field names. */

		if ( strcmp( field_name, "MemFree:" ) == 0 ) {
			meminfo->free_bytes = 1024L * field_value;
		} else
		if ( strcmp( field_name, "MemTotal:" ) == 0 ) {
			meminfo->total_bytes = 1024L * field_value;
		} else
		if ( strcmp( field_name, "SwapFree:" ) == 0 ) {
			meminfo->swap_free_bytes = 1024L * field_value;
		} else
		if ( strcmp( field_name, "SwapTotal:" ) == 0 ) {
			meminfo->swap_bytes = 1024L * field_value;
		}
	}

	/* Compute values not directly reported by /proc/meminfo. */

	if ( meminfo->total_bytes >= meminfo->free_bytes ) {
		meminfo->used_bytes =
			meminfo->total_bytes - meminfo->free_bytes;
	}
	if ( meminfo->swap_bytes >= meminfo->swap_free_bytes ) {
		meminfo->swap_used_bytes =
			meminfo->swap_bytes - meminfo->swap_free_bytes;
	}

	/* We are done, so return. */

	(void) fclose( procfile );

#if 0
	mx_display_system_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_WIN32 )

/* Use GlobalMemoryStatusEx() if it is available, or else GlobalMemoryStatus()
 * if it is not.
 */

#include <Windows.h>

/* We must determine whether GlobalMemoryStatusEx() is defined in our
 * Platform SDK.  There is no obvious way of doing this and we can't use
 * autoconf on Windows, so I have resorted to indirect methods.  My current
 * technique is to look for an ERROR definition in WinError.h that is defined
 * in recent versions of the Platform SDK, but not for older versions.  At the
 * moment, I am testing using the macro definition ERROR_PRODUCT_VERSION,
 * which has no obvious relationship to GlobalMemoryStatusEx(), but
 * nevertheless seems to do the job.
 */

#if ! defined(ERROR_PRODUCT_VERSION)

/* MEMORYSTATUSEX is not defined in our copy of the Platform SDK,
 * so we must define it ourself.
 */

typedef struct _MEMORYSTATUSEX {
	DWORD dwLength;
	DWORD dwMemoryLoad;
	DWORDLONG ullTotalPhys;
	DWORDLONG ullAvailPhys;
	DWORDLONG ullTotalPageFile;
	DWORDLONG ullAvailPageFile;
	DWORDLONG ullTotalVirtual;
	DWORDLONG ullAvailVirtual;
	DWORDLONG ullAvailExtendedVirtual;
} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;

#endif

typedef BOOL (*GlobalMemoryStatusEx_type)( LPMEMORYSTATUSEX );

static GlobalMemoryStatusEx_type ptrGlobalMemoryStatusEx = NULL;

static int global_memory_status_ex_is_available = TRUE;

MX_EXPORT mx_status_type
mx_get_system_meminfo( MX_SYSTEM_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_system_meminfo()";

	MEMORYSTATUSEX memory_status_ex;
	MEMORYSTATUS memory_status;
	BOOL status;

	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( meminfo == (MX_SYSTEM_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYSTEM_MEMINFO pointer passed was NULL." );
	}

	if ( global_memory_status_ex_is_available ) {
		/* See if we need to get a pointer to GlobalMemoryStatusEx(). */

		if ( ptrGlobalMemoryStatusEx == NULL ) {
			HMODULE hinst_kernel32;

			hinst_kernel32 = GetModuleHandle(TEXT("kernel32.dll"));

			if ( hinst_kernel32 == NULL ) {
				global_memory_status_ex_is_available = FALSE;
				return mx_error(
				MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to get a module handle for KERNEL32.DLL failed.  "
		"This should NEVER, EVER happen.  In fact, you should not be "
		"reading this message, since the computer should have "
		"crashed by now.  If you DO see this message, let "
		"William Lavender know." );
			}

			ptrGlobalMemoryStatusEx = (GlobalMemoryStatusEx_type)
				GetProcAddress( hinst_kernel32,
					TEXT("GlobalMemoryStatusEx") );

			MX_DEBUG( 2,("%s: ptrGlobalMemoryStatusEx = %p",
				fname, ptrGlobalMemoryStatusEx));

			if ( ptrGlobalMemoryStatusEx == NULL ) {
				global_memory_status_ex_is_available = FALSE;
			}
		}
	}

	if ( global_memory_status_ex_is_available ) {
	
		/* GlobalMemoryStatusEx() can handle computers with more
		 * than 4 gigabytes of memory.
		 */

		memory_status_ex.dwLength = sizeof(memory_status_ex);

		status = (ptrGlobalMemoryStatusEx)( &memory_status_ex );

		if ( status == 0 ) {
			global_memory_status_ex_is_available = FALSE;

			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			MX_DEBUG(-2,
		("%s: GlobalMemoryStatusEx() failed with error code = %lu, "
			"Error message = '%s'", fname, last_error_code,
			message_buffer ));
		}
	}

	if ( global_memory_status_ex_is_available ) {
		meminfo->total_bytes = memory_status_ex.ullTotalPhys;
		meminfo->free_bytes = memory_status_ex.ullAvailPhys;

		meminfo->swap_bytes = memory_status_ex.ullTotalPageFile;
		meminfo->swap_free_bytes = memory_status_ex.ullAvailPageFile;
	} else {
		GlobalMemoryStatus( &memory_status );

		meminfo->total_bytes = memory_status.dwTotalPhys;
		meminfo->free_bytes = memory_status.dwAvailPhys;

		meminfo->swap_bytes = memory_status.dwTotalPageFile;
		meminfo->swap_free_bytes = memory_status.dwAvailPageFile;
	}

	meminfo->used_bytes = meminfo->total_bytes - meminfo->free_bytes;

	meminfo->swap_used_bytes =
			meminfo->swap_bytes - meminfo->swap_free_bytes;

#if 0
	mx_display_system_meminfo( meminfo );
#endif

	/* FIXME FIXME FIXME!!!!! --
	 *
	 * For some unknown reason, this routine crashes while returning
	 * unless the 'return MX_SUCCESSFUL_RESULT' statement is included
	 * _twice_.  If you include it twice, it seems to work.   Obviously
	 * this is quite baffling and distressing, but I haven't been able
	 * to figure out why it is happening.
	 *
	 * For reference, this problem was observed with Microsoft Visual
	 * Studio .NET 2003 running on Windows XP SP2 on April 15, 2005.
	 *
	 * (William Lavender)
	 */

	return MX_SUCCESSFUL_RESULT;

#if 1
	/* Magical pixie dust. */

	return MX_SUCCESSFUL_RESULT;
#endif
}

/***************************************************************************/

#elif defined(OS_MACOSX)

/* Inspired by
 *    http://lists.apple.com/archives/darwin-development/2004/Jul/msg00050.html
 */

#include <mach/mach_init.h>
#include <mach/mach_host.h>

MX_EXPORT mx_status_type
mx_get_system_meminfo( MX_SYSTEM_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_system_meminfo()";

	vm_size_t              pagesize;
	vm_statistics_data_t   page_info;
	mach_msg_type_number_t count;
	kern_return_t          kreturn;

	if ( meminfo == (MX_SYSTEM_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYSTEM_MEMINFO pointer passed was NULL." );
	}

	memset( meminfo, 0, sizeof( MX_SYSTEM_MEMINFO ) );

	/* How big is a virtual memory page? */

	kreturn = host_page_size( mach_host_self(), &pagesize );

	if ( kreturn != KERN_SUCCESS ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the host page size failed." );
	}

#if 0
	MX_DEBUG(-2,("%s: pagesize = %lu", fname, (unsigned long) pagesize));
#endif

	/* Get virtual memory statistics in units of pages. */

	count = HOST_VM_INFO_COUNT;

	kreturn = host_statistics( mach_host_self(), HOST_VM_INFO,
				(host_info_t) &page_info, &count );

	if ( kreturn != KERN_SUCCESS ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to read operating system statistics failed." );
	}

#if 0
	MX_DEBUG(-2,("%s: free_count = %lu", fname,
		(unsigned long) (pagesize * page_info.free_count / 1024L)));

	MX_DEBUG(-2,("%s: active_count = %lu", fname,
		(unsigned long) (pagesize * page_info.active_count / 1024L)));

	MX_DEBUG(-2,("%s: inactive_count = %lu", fname,
		(unsigned long) (pagesize * page_info.inactive_count / 1024L)));

	MX_DEBUG(-2,("%s: wire_count = %lu", fname,
		(unsigned long) (pagesize * page_info.wire_count / 1024L)));
#endif

	meminfo->free_bytes = pagesize * page_info.free_count;

	meminfo->used_bytes = pagesize * ( page_info.wire_count
			+ page_info.active_count + page_info.inactive_count );

	meminfo->total_bytes = meminfo->used_bytes + meminfo->free_bytes;

#if 0
	mx_display_system_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_SOLARIS )

MX_EXPORT mx_status_type
mx_get_system_meminfo( MX_SYSTEM_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_system_meminfo()";

	static const char command[] = "/usr/sbin/swap -s";
	FILE *file;
	unsigned long page_size, swap_kbytes_used, swap_kbytes_available;
	int num_items;
	char buffer[200];

	if ( meminfo == (MX_SYSTEM_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYSTEM_MEMINFO pointer passed was NULL." );
	}

	memset( meminfo, 0, sizeof( MX_SYSTEM_MEMINFO ) );

	/* Get the system page size. */

	page_size = (unsigned long) getpagesize();

	/* Get swap statistics using the 'swap' shell command. */

	file = popen( command, "r" );

	if ( file == NULL ) {
		mx_warning(
    "Could not execute the %s command, so swap statistics are not available.",
			command );
	} else {
		fgets( buffer, sizeof(buffer), file );

		if ( feof(file) || ferror(file) ) {
			mx_warning(
		"An error occurred reading the output from the %s command.",
				command );
		} else {
			num_items = sscanf( buffer,
	"total: %*s bytes allocated + %*s reserved = %luk used, %luk available",
				&swap_kbytes_used, &swap_kbytes_available );

			if ( num_items != 2 ) {
				mx_warning(
		"Could not parse the output of the %s command.  Output = '%s'",
					command, buffer );
			} else {
				meminfo->swap_bytes =
					1024L * swap_kbytes_available;

				meminfo->swap_used_bytes =
					1024L * swap_kbytes_used;

				meminfo->swap_free_bytes = meminfo->swap_bytes
						- meminfo->swap_used_bytes;
			}
		}
		pclose(file);
	}

	/* Get system memory statistics. */

	meminfo->total_bytes = page_size * sysconf(_SC_PHYS_PAGES);

	meminfo->free_bytes = page_size * sysconf(_SC_AVPHYS_PAGES);

	meminfo->used_bytes = meminfo->total_bytes - meminfo->free_bytes;

#if 0
	mx_display_system_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_IRIX )

/* This information is derived from the "Interrogating the Memory System"
 * section of Chapter 1 of the "Topics in IRIX Programming" manual.
 */

#include <sys/types.h>
#include <sys/sysmp.h>
#include <sys/sysinfo.h>

MX_EXPORT mx_status_type
mx_get_system_meminfo( MX_SYSTEM_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_system_meminfo()";

	struct rminfo rminfo_struct;
	unsigned long pagesize;

	if ( meminfo == (MX_SYSTEM_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYSTEM_MEMINFO pointer passed was NULL." );
	}

	memset( meminfo, 0, sizeof( MX_SYSTEM_MEMINFO ) );

	/* Get the system page size. */

	pagesize = getpagesize();

#if 0
	MX_DEBUG(-2,("%s: pagesize = %lu", fname, pagesize));
#endif

	(void) sysmp( MP_KERNADDR, MPSA_RMINFO, &rminfo_struct );

#if 0
	MX_DEBUG(-2,("%s: freemem = %lu", fname,
				(unsigned long) rminfo_struct.freemem));
	MX_DEBUG(-2,("%s: availsmem = %lu", fname,
				(unsigned long) rminfo_struct.availsmem));
	MX_DEBUG(-2,("%s: availrmem = %lu", fname,
				(unsigned long) rminfo_struct.availrmem));
	MX_DEBUG(-2,("%s: physmem = %lu", fname,
				(unsigned long) rminfo_struct.physmem));
#endif

	meminfo->total_bytes = pagesize * rminfo_struct.physmem;

	meminfo->free_bytes = pagesize * rminfo_struct.availrmem;

	meminfo->used_bytes = meminfo->total_bytes - meminfo->free_bytes;

#if 0
	mx_display_system_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_QNX ) || defined( OS_CYGWIN )

MX_EXPORT mx_status_type
mx_get_system_meminfo( MX_SYSTEM_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_system_meminfo()";

	unsigned long pagesize;

	if ( meminfo == (MX_SYSTEM_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYSTEM_MEMINFO pointer passed was NULL." );
	}

	memset( meminfo, 0, sizeof( MX_SYSTEM_MEMINFO ) );

	pagesize = sysconf( _SC_PAGESIZE );

	meminfo->total_bytes = pagesize * sysconf( _SC_PHYS_PAGES );

	meminfo->free_bytes = pagesize * sysconf( _SC_AVPHYS_PAGES );

	meminfo->used_bytes = meminfo->total_bytes - meminfo->free_bytes;

#if 0
	mx_display_system_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_BSD ) && defined(__FreeBSD__)

/* FreeBSD is a special case. */

#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <vm/vm_param.h>

MX_EXPORT mx_status_type
mx_get_system_meminfo( MX_SYSTEM_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_system_meminfo()";

	struct vmtotal vmtotal_struct;
	size_t vmtotal_struct_size;
	int mib[2];
	int status, saved_errno;

	if ( meminfo == (MX_SYSTEM_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYSTEM_MEMINFO pointer passed was NULL." );
	}

	memset( meminfo, 0, sizeof( MX_SYSTEM_MEMINFO ) );

	/* Read in the struct vmtotal structure. */

	mib[0] = CTL_VM;
	mib[1] = VM_METER;

	vmtotal_struct_size = sizeof(vmtotal_struct);

	status = sysctl(mib, 2, &vmtotal_struct, &vmtotal_struct_size, NULL, 0);

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"An error occurred while using sysctl() to read in "
			"system wide virtual memory statistics.  "
			"Errno = %d, error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
	}

	meminfo->total_bytes = vmtotal_struct.t_rm;

#if 0
	mx_display_system_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_RTEMS ) || defined( OS_VXWORKS ) || defined( OS_BSD ) \
	|| defined( OS_HPUX ) || defined( OS_TRU64 ) || defined( OS_DJGPP ) \
	|| defined( OS_ECOS ) || defined( OS_UNIXWARE ) || defined( OS_MINIX )

MX_EXPORT mx_status_type
mx_get_system_meminfo( MX_SYSTEM_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_system_meminfo()";

	if ( meminfo == (MX_SYSTEM_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYSTEM_MEMINFO pointer passed was NULL." );
	}

	memset( meminfo, 0, sizeof( MX_SYSTEM_MEMINFO ) );

	/* Currently all counters are set to zero for this platform. */

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_VMS )

#include <ssdef.h>
#include <starlet.h>
#include <syidef.h>
#include <rmidef.h>

#if 1
/* FIXME: This prototype should not be necessary since it should be
 * in <starlet.h> (I think).
 */

extern int
sys$getrmi( unsigned int, unsigned int, unsigned int,
		void *, short *, void (*)(__unknown_params), int );

#endif

MX_EXPORT mx_status_type
mx_get_system_meminfo( MX_SYSTEM_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_system_meminfo()";

	int vms_status;
	short iosb[4];

	unsigned long physical_memory_pages;
	unsigned long pagefile_free;
	unsigned long pagefile_pages;
	unsigned long swapfile_free;
	unsigned long swapfile_pages;

	unsigned long free_list_pages;
	unsigned long modified_pages;

	struct {
		unsigned short buffer_length;
		unsigned short item_code;
		void *buffer_address;
		void *return_length_address;
	} syi_item_list[] =
		{ {4, SYI$_MEMSIZE, &physical_memory_pages, NULL},
		  {4, SYI$_PAGEFILE_FREE, &pagefile_free, NULL},
		  {4, SYI$_PAGEFILE_PAGE, &pagefile_pages, NULL},
		  {4, SYI$_SWAPFILE_FREE, &swapfile_free, NULL},
		  {4, SYI$_SWAPFILE_PAGE, &swapfile_pages, NULL},
		  {0, 0, NULL, NULL} };

	struct {
		unsigned short buffer_length;
		unsigned short item_code;
		void *buffer_address;
		void *return_length_address;
	} rmi_item_list[] =
		{ {4, RMI$_FRLIST, &free_list_pages, NULL},
		  {4, RMI$_MODLIST, &modified_pages, NULL},
		  {0, 0, NULL, NULL} };

	if ( meminfo == (MX_SYSTEM_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYSTEM_MEMINFO pointer passed was NULL." );
	}

	memset( meminfo, 0, sizeof( MX_SYSTEM_MEMINFO ) );

	/* Some information comes from sys$getsyiw(). */

	vms_status = sys$getsyiw( 0, 0, 0, &syi_item_list, iosb, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Cannot get local system information from $GETSYI.  "
		"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );
	}

#if 0
	MX_DEBUG(-2,("%s: physical_memory_pages = %lu", fname,
				physical_memory_pages));
	MX_DEBUG(-2,("%s: pagefile_free = %lu", fname,
				pagefile_free));
	MX_DEBUG(-2,("%s: pagefile_pages = %lu", fname,
				pagefile_pages));
	MX_DEBUG(-2,("%s: swapfile_free = %lu", fname,
				swapfile_free));
	MX_DEBUG(-2,("%s: swapfile_pages = %lu", fname,
				swapfile_pages));
#endif

	meminfo->total_bytes = 512L * physical_memory_pages;

	meminfo->swap_bytes = 512L * swapfile_pages;

	meminfo->swap_free_bytes = 512L * swapfile_free;

	meminfo->swap_used_bytes = meminfo->swap_bytes
					- meminfo->swap_free_bytes;

	/* Other information comes from sys$getrmi(). */

	vms_status = sys$getrmi( 0, 0, 0, &rmi_item_list, iosb, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Cannot get local performance information from $GETRMI.  "
		"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );
	}

#if 0
	MX_DEBUG(-2,("%s: free_list_pages = %lu", fname,
				free_list_pages));
	MX_DEBUG(-2,("%s: modified_pages = %lu", fname,
				modified_pages));
#endif

	meminfo->free_bytes = 512L * free_list_pages;

	meminfo->used_bytes = meminfo->total_bytes - meminfo->free_bytes;

#if 0
	mx_display_system_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#else

#error mx_get_system_meminfo() is not yet defined for this platform.

#endif


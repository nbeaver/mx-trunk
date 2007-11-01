/*
 * Name:    mx_memory_process.c
 *
 * Purpose: This file contains functions that report the memory usage
 *          of a process.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005-2007 Illinois Institute of Technology
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
mx_display_process_meminfo( MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mx_display_process_meminfo()";

	if ( meminfo == (MX_PROCESS_MEMINFO *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PROCESS_MEMINFO pointer passed was NULL." );

		return;
	}

	mx_info("  total_bytes              = %lu",
			(unsigned long) meminfo->total_bytes);

	mx_info("  locked_in_memory_bytes   = %lu",
			(unsigned long) meminfo->locked_in_memory_bytes);

	mx_info("  resident_in_memory_bytes = %lu",
			(unsigned long) meminfo->resident_in_memory_bytes);

	mx_info("  data_bytes               = %lu",
			(unsigned long) meminfo->data_bytes);

	mx_info("  stack_bytes              = %lu",
			(unsigned long) meminfo->stack_bytes);

	mx_info("  heap_bytes               = %lu",
			(unsigned long) meminfo->heap_bytes);

	mx_info("  code_bytes               = %lu",
			(unsigned long) meminfo->code_bytes);

	mx_info("  library_bytes            = %lu",
				(unsigned long) meminfo->library_bytes);

	mx_info("  allocated_bytes          = %lu",
				(unsigned long) meminfo->allocated_bytes);

	mx_info("  memory_mapped_bytes      = %lu",
				(unsigned long) meminfo->memory_mapped_bytes);
	return;
}

/***************************************************************************/

#if defined( OS_LINUX )

#include <malloc.h>

/*** Read /proc/PID/status for Linux. ***/

MX_EXPORT mx_status_type
mx_get_process_meminfo( unsigned long process_id,
			MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_process_meminfo()";

	struct mallinfo mallinfo_struct;
	char buffer[80];
	char field_name[80];
	unsigned long field_value;
	char proc_filename[ MXU_FILENAME_LENGTH+1 ];
	FILE *procfile;
	int current_process, saved_errno, length, num_items;

	if ( meminfo == (MX_PROCESS_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PROCESS_MEMINFO pointer passed was NULL." );
	}

	if ( process_id == MXF_PROCESS_ID_SELF ) {
		current_process = TRUE;
	} else
	if ( process_id == mx_process_id() ) {
		current_process = TRUE;
	} else {
		current_process = FALSE;
	}

	if ( process_id == MXF_PROCESS_ID_SELF ) {
		process_id = mx_process_id();
	}

	memset( meminfo, 0, sizeof( MX_PROCESS_MEMINFO ) );

	meminfo->process_id = process_id;

	/* Open the /proc/PID/status file. */

	sprintf( proc_filename, "/proc/%lu/status", process_id );

	procfile = fopen( proc_filename, "r" );

	if ( procfile == (FILE *) NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open proc filesystem file '%s'.  Reason = '%s'",
			proc_filename, strerror( saved_errno ) );
	}

	/* Loop over the lines of output from /proc/PID/status. */

	while (1) {
		fgets( buffer, sizeof(buffer), procfile );

		if ( feof(procfile) ) {
			/* We have read the last line from /proc/PID/status,
			 * so break out of the while loop.
			 */

			(void) fclose(procfile);
			break;
		}

		if ( ferror(procfile) ) {
			saved_errno = errno;

			(void) fclose(procfile);
			return mx_error( MXE_FILE_IO_ERROR, fname,
		"Error reading data from /proc/%lu/status.  Error = '%s'",
					process_id, strerror( saved_errno ) );
		}

		/* Delete any trailing newline. */

		length = strlen( buffer );

		if ( buffer[length-1] == '\n' ) {
			buffer[length-1] = '\0';
		}

		/* Attempt to parse output lines of the form
		 *
		 *     VmSize:     1528 kB
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

		if ( strcmp( field_name, "VmData:" ) == 0 ) {
			meminfo->data_bytes = 1024L * field_value;
		} else
		if ( strcmp( field_name, "VmExe:" ) == 0 ) {
			meminfo->code_bytes = 1024L * field_value;
		} else
		if ( strcmp( field_name, "VmLck:" ) == 0 ) {
			meminfo->locked_in_memory_bytes = 1024L * field_value;
		} else
		if ( strcmp( field_name, "VmLib:" ) == 0 ) {
			meminfo->library_bytes = 1024L * field_value;
		} else
		if ( strcmp( field_name, "VmRSS:" ) == 0 ) {
			meminfo->resident_in_memory_bytes = 1024L * field_value;
		} else
		if ( strcmp( field_name, "VmSize:" ) == 0 ) {
			meminfo->total_bytes = 1024L * field_value;
		} else
		if ( strcmp( field_name, "VmStk:" ) == 0 ) {
			meminfo->stack_bytes = 1024L * field_value;
		}
	}

	/* Get heap statistics from mallinfo() */

	if ( current_process ) {
		mallinfo_struct = mallinfo();

		meminfo->heap_bytes = mallinfo_struct.arena;
		meminfo->allocated_bytes = mallinfo_struct.uordblks;
		meminfo->memory_mapped_bytes = mallinfo_struct.hblkhd;
	}

#if 0
	mx_display_process_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_WIN32 )

/* Use GetProcessMemoryInfo() from PSAPI.DLL for Win32 (not on Win 9x). */

#include <Windows.h>

/* We must determine whether the Psapi.h header file is available.  There
 * is no obvious way of doing this and we can't use autoconf on Windows,
 * so I have resorted to indirect methods.  My current technique is to look
 * for an ERROR definition in WinError.h that is defined in recent versions
 * of the Platform SDK, but not for older versions.  At the moment, I am
 * testing using the macro definition ERROR_PRODUCT_VERSION, which has no
 * relationship to Psapi.h but nevertheless seems to do the job.
 */

#if defined(ERROR_PRODUCT_VERSION)
#  include <Psapi.h>
#else

/* Psapi.h is not available, so we must make
 * the necessary definitions ourself.
 */

typedef struct _PROCESS_MEMORY_COUNTERS {
	DWORD cb;
	DWORD PageFaultCount;
	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;
	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;

typedef PROCESS_MEMORY_COUNTERS *PPROCESS_MEMORY_COUNTERS;

#endif

static int psapi_is_available = TRUE;

static HINSTANCE hinst_psapi = NULL;

typedef BOOL (*GetProcessMemoryInfo_type)( HANDLE,
				PPROCESS_MEMORY_COUNTERS, DWORD );

static GetProcessMemoryInfo_type ptrGetProcessMemoryInfo = NULL;

static mx_status_type
mxp_get_psapi_memory_info( MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mxp_get_psapi_memory_info()";

	HANDLE process_handle;
	PROCESS_MEMORY_COUNTERS pmc;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	BOOL status;

	/* If the pointer to GetProcessMemoryInfo is NULL, then PSAPI.DLL
	 * has not yet been loaded.  If so, then we must load it.
	 */

	if ( ptrGetProcessMemoryInfo == NULL ) {

		if ( hinst_psapi != NULL ) {
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Somehow the instance of PSAPI.DLL is not NULL, "
			"but the pointer to GetProcessMemoryInfo() "
			"_is_ NULL.  This should not be able to happen." );
		}

		hinst_psapi = LoadLibrary(TEXT("psapi.dll"));

		if ( hinst_psapi == NULL ) {
			mx_warning(
			"PSAPI.DLL is not available on this computer, "
			"so process memory statistics will not be available." );

			psapi_is_available = FALSE;

			return MX_SUCCESSFUL_RESULT;
		}

		ptrGetProcessMemoryInfo = (GetProcessMemoryInfo_type)
			GetProcAddress( hinst_psapi,
					TEXT("GetProcessMemoryInfo") );

		if ( ptrGetProcessMemoryInfo == NULL ) {
			mx_warning(
			"GetProcessMemoryInfo() was not found in PSAPI.DLL.  "
			"This should not be able to happen." );

			status = FreeLibrary( hinst_psapi );

			if ( status == 0 ) {
				last_error_code = GetLastError();

				mx_win32_error_message( last_error_code,
					message_buffer, sizeof(message_buffer));

				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
				"The attempt to free a handle for PSAPI.DLL "
				"failed.  Win32 error code = %ld, "
				"error_message = '%s'",
					last_error_code, message_buffer );
			}

			hinst_psapi = NULL;

			psapi_is_available = FALSE;

			return MX_SUCCESSFUL_RESULT;
		}
	}

	/* Open a handle for the requested process id. */

	process_handle = OpenProcess(
				PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				FALSE, meminfo->process_id );

	if ( process_handle == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Error opening a handle for process %lu.  "
		"Win32 error code = %ld, error message = '%s'",
			meminfo->process_id, last_error_code, message_buffer );
	}

	/* Now we are ready to read the process memory counters. */

	status = (ptrGetProcessMemoryInfo)( process_handle,
						&pmc, sizeof(pmc) );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Error invoking GetProcessMemoryInfo() for process %lu.  "
		"Win32 error code = %ld, error message = '%s'",
			meminfo->process_id, last_error_code, message_buffer );
	}

	status = CloseHandle( process_handle );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Error closing process handle for process %lu.  "
		"Win32 error code = %ld, error message = '%s'",
			meminfo->process_id, last_error_code, message_buffer );
	}

#if 0
	MX_DEBUG(-2,("%s: working set size = %lu", fname,
				pmc.WorkingSetSize));
	MX_DEBUG(-2,("%s: paged pool usage = %lu", fname,
				pmc.QuotaPagedPoolUsage));
	MX_DEBUG(-2,("%s: non-paged pool usage = %lu", fname,
				pmc.QuotaNonPagedPoolUsage));
	MX_DEBUG(-2,("%s: pagefile usage = %lu", fname,
				pmc.PagefileUsage));
#endif

	meminfo->total_bytes = pmc.WorkingSetSize;

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

static mx_status_type
mxp_get_local_heap_size( HANDLE heap_handle,
			DWORD *heap_size,
			DWORD *allocated_size,
			int debug_flag )
{
	static const char fname[] = "mxp_get_local_heap_size()";

	PROCESS_HEAP_ENTRY heap_entry;
	DWORD local_heap_size, heap_element_size;
	DWORD local_allocated_size;
	DWORD last_error_code, heapwalk_error_code;
	TCHAR message_buffer[100];
	BOOL status;
	unsigned long i, num_24_byte_regions;

	if ( heap_size == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The heap_size pointer passed was NULL." );
	}

	status = HeapLock( heap_handle );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Could not lock heap.  "
			"Win32 error code = %ld, error message = '%s'",
				last_error_code, message_buffer );
	}

	heap_entry.lpData = NULL;

	local_heap_size = 0;
	local_allocated_size = 0;

#if 1
	num_24_byte_regions = 0;
#endif

	for ( i = 0; ; i++ ) {

		status = HeapWalk( heap_handle, &heap_entry );

		if ( status == 0 ) {
			heapwalk_error_code = GetLastError();

			status = HeapUnlock( heap_handle );

			if ( status == 0 ) {
				last_error_code = GetLastError();

				mx_win32_error_message( last_error_code,
				    message_buffer, sizeof(message_buffer) );

				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
					"Could not unlock heap.  "
				"Win32 error code = %ld, error message = '%s'",
					last_error_code, message_buffer );
			}

			if ( heapwalk_error_code == ERROR_NO_MORE_ITEMS ) {


#if 0
				if ( debug_flag ) {
					fprintf(stderr, "\n");
				}
#endif

				/* We have reached the end of the heap,
				 * so we may return the heap size now.
				 */

				*heap_size = local_heap_size;

				*allocated_size = local_allocated_size;

				return MX_SUCCESSFUL_RESULT;
			} else {
				mx_win32_error_message( heapwalk_error_code,
				    message_buffer, sizeof(message_buffer) );

				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
				"Error walking the heap for heap handle %lu.  "
				"Win32 error code = %ld, error message = '%s'",
					(unsigned long) heap_handle,
					heapwalk_error_code, message_buffer );
			}
		}

		heap_element_size = heap_entry.cbData;

		local_heap_size += heap_element_size;

		if ( heap_entry.wFlags & PROCESS_HEAP_ENTRY_BUSY ) {
			local_allocated_size += heap_element_size;
		}

		if ( debug_flag ) {
#if 0
			MX_DEBUG(-2,("i = %d", i));
			MX_DEBUG(-2,("lpData = %p", heap_entry.lpData));
			MX_DEBUG(-2,("cbData = %ld", heap_entry.cbData));
			MX_DEBUG(-2,("cbOverhead = %d",
						heap_entry.cbOverhead));
			MX_DEBUG(-2,("iRegionIndex = %d",
						heap_entry.iRegionIndex));
			MX_DEBUG(-2,("wFlags = %#x", heap_entry.wFlags));
			MX_DEBUG(-2,("Block.hMem = %p",
						heap_entry.Block.hMem));
			MX_DEBUG(-2,("dwCommittedSize = %ld",
					heap_entry.Region.dwCommittedSize));
			MX_DEBUG(-2,("dwUnCommittedSize = %ld",
					heap_entry.Region.dwUnCommittedSize));
			MX_DEBUG(-2,("lpFirstBlock = %p",
					heap_entry.Region.lpFirstBlock));
			MX_DEBUG(-2,("lpLastBlock = %p",
					heap_entry.Region.lpLastBlock));
#endif

#if 0
			MX_DEBUG(-2,("%s: i = %d, wFlags = %#x, cbData = %ld",
			    fname, i, heap_entry.wFlags, heap_entry.cbData));
#endif

#if 0
			if ( heap_entry.wFlags & PROCESS_HEAP_ENTRY_BUSY ) {
				fprintf(stderr, "%lu ", heap_entry.cbData);
			} else {
				fprintf(stderr, "-%lu ", heap_entry.cbData);
			}
#endif

#if 0
			if ( (i % 10) == 0 ) {
				fprintf(stderr, "| ");
			}
#endif

#if 1
			if ( heap_entry.wFlags & PROCESS_HEAP_ENTRY_BUSY ) {
				if ( heap_entry.cbData == 24 ) {
					num_24_byte_regions++;
				}
			}
#endif

		}
	}

	/* We should never get here. */
}

static mx_status_type
mxp_get_total_heap_size( MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mxp_get_total_heap_size()";

	DWORD number_of_heaps, new_number_of_heaps;
	HANDLE *heap_handle_array;
	DWORD total_heap_bytes, local_heap_bytes;
	DWORD total_allocated_bytes, local_allocated_bytes;

	unsigned long i, heap_handle_attempts;
	int debug_flag;
	mx_status_type mx_status;

	heap_handle_attempts = 100;

	number_of_heaps = 0;
	heap_handle_array = NULL;

	for ( i = 0; i < heap_handle_attempts; i++ ) {
		new_number_of_heaps = GetProcessHeaps( number_of_heaps,
						heap_handle_array );

		if ( new_number_of_heaps <= number_of_heaps ) {
			number_of_heaps = new_number_of_heaps;

			break;		/* Exit the for() loop. */
		}

		if ( heap_handle_array == NULL ) {
			if ( new_number_of_heaps <= 0 ) {
				continue;	/* Loop again */
			}

			heap_handle_array = (HANDLE *) malloc(
				new_number_of_heaps * sizeof(HANDLE));
		} else {
			if ( new_number_of_heaps <= 0 ) {
				mx_free( heap_handle_array );

				continue;	/* Loop again */
			}

			heap_handle_array = (HANDLE *) realloc(
				heap_handle_array,
				new_number_of_heaps * sizeof(HANDLE));
		}

		if ( heap_handle_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"array of %lu heap handles.",
				new_number_of_heaps );
		}

		number_of_heaps = new_number_of_heaps;
	}

	if ( i >= heap_handle_attempts ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Unable to get a complete array of process heap "
		"handles after %lu attempts.", heap_handle_attempts );
	}

	total_heap_bytes = 0;

	total_allocated_bytes = 0;

	for ( i = 0; i < number_of_heaps; i++ ) {
#if 0
		if ( i == 4 ) {
			debug_flag = TRUE;
		} else {
			debug_flag = FALSE;
		}
#else
		debug_flag = TRUE;
#endif

		mx_status = mxp_get_local_heap_size(
					heap_handle_array[i],
					&local_heap_bytes,
					&local_allocated_bytes,
					debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 1
		MX_DEBUG(-2,
	("Heap %lu, heap_bytes = %lu, allocated_bytes = %lu",
			i, (unsigned long) local_heap_bytes,
			(unsigned long) local_allocated_bytes));
#endif

		total_heap_bytes += local_heap_bytes;

		total_allocated_bytes += local_allocated_bytes;
	}

	meminfo->heap_bytes = total_heap_bytes;

	meminfo->allocated_bytes = total_allocated_bytes;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_process_meminfo( unsigned long process_id,
			MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_process_meminfo()";

	mx_status_type mx_status;

	if ( meminfo == (MX_PROCESS_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PROCESS_MEMINFO pointer passed was NULL." );
	}

	if ( process_id == MXF_PROCESS_ID_SELF ) {
		/* We want to examine the current process. */

		process_id = mx_process_id();
	}

	memset( meminfo, 0, sizeof( MX_PROCESS_MEMINFO ) );

	meminfo->process_id = process_id;

	/* If PSAPI.DLL is not available, then we just skip the process
	 * memory usage determination.  This means that on Windows 9x all
	 * the values will be reported as 0.
	 *
	 * In principle we could add support for the Windows 9x TOOLHELP.DLL,
	 * but TOOLHELP.DLL is complicated and Windows 9x is obsolete, so we
	 * will not do this unless someone loudly complains.
	 */

	if ( psapi_is_available ) {
		mx_status = mxp_get_psapi_memory_info( meminfo );
	}

#if 0
	{
		HANDLE crt_heap;

		crt_heap = GetProcessHeap();

		mx_status = mxp_get_local_heap_size( crt_heap,
						&local_heap_bytes,
						&local_allocated_bytes,
						FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG(-2,("%s: crt_heap size = %lu",
			fname, local_allocated_bytes));

		CloseHandle(crt_heap);
	}
#endif

	/* Examine heap memory.
	 *
	 * Note: This does not rely on the presence of PSAPI.DLL.
	 */

	if ( process_id == mx_process_id() ) {

		/* We only do this if we are asking about the current
		 * process, because apparently there is no easy way to
		 * examine the heap of a different process.
		 */

		mx_status = mxp_get_total_heap_size( meminfo );
	}

#if 1
	mx_display_process_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined(OS_MACOSX)

#include <sys/sysctl.h>
#include <mach/mach_port.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/mach_traps.h>
#include <mach/message.h>
#include <mach/shared_memory_server.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <mach/vm_map.h>
#include <malloc/malloc.h>

/* Some parts of the following function were inspired by the Darwin source
 * code for 'ps' and 'top'.
 */ 

MX_EXPORT mx_status_type
mx_get_process_meminfo( unsigned long process_id,
			MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_process_meminfo()";

	vm_size_t pagesize;
	task_t task;
	task_basic_info_data_t basic_info;
	mach_msg_type_number_t info_count;
	kern_return_t kreturn;

	vm_address_t vm_address;
	vm_size_t vm_size;
	vm_region_basic_info_data_64_t vm_basic_info;
	mach_port_t vm_object_name;

	malloc_zone_t **malloc_zone_array, *zone;
	unsigned int i, num_malloc_zones;
	malloc_statistics_t zone_statistics;

	if ( meminfo == (MX_PROCESS_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PROCESS_MEMINFO pointer passed was NULL." );
	}

	if ( process_id == MXF_PROCESS_ID_SELF ) {
		/* We want to examine the current process. */

		process_id = mx_process_id();
	}

	memset( meminfo, 0, sizeof( MX_PROCESS_MEMINFO ) );

	meminfo->process_id = process_id;

	/* Suppress 'might be used uninitialized in this function' warning. */

	malloc_zone_array = NULL;

	/* Get the virtual memory page size. */

	kreturn = host_page_size( mach_host_self(), &pagesize );

	if ( kreturn != KERN_SUCCESS ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the host page size failed." );
	}

	/* The rest of the functions we call need a task pointer which
	 * we can now get from the process id.
	 */

	kreturn = task_for_pid( current_task(), process_id, &task );

	if ( ( kreturn == KERN_FAILURE )
	  && ( mx_process_exists( process_id ) == FALSE ) )
	{
		return mx_error( MXE_NOT_FOUND, fname,
		"Process %lu does not exist.", process_id );
	}

	if ( kreturn != KERN_SUCCESS ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Call to task_for_pid( ..., %lu, ... ) failed.  kreturn = %ld",
			process_id, (long) kreturn );
	}

	/* Find the number of real pages in use by the task. */

	info_count = TASK_BASIC_INFO_COUNT;

	kreturn = task_info( task, TASK_BASIC_INFO,
				(task_info_t) &basic_info, &info_count );

	if ( kreturn != KERN_SUCCESS ) {
		mach_port_deallocate( mach_host_self(), task );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Call to task_info() failed.  kreturn = %ld", (long) kreturn );
	}

	meminfo->resident_in_memory_bytes = basic_info.resident_size;

	/************** Start of magical code ***************/

	/* If this task has split libraries mapped in, then adjust its
	 * virtual size down by the two segments used for split libraries.
	 *
	 * (WML) I do not fully know what this does.  I am just parroting
	 * what the equivalent source code in the Darwin 'ps' command does.
	 */

	info_count = VM_REGION_BASIC_INFO_COUNT_64;
	vm_address = GLOBAL_SHARED_TEXT_SEGMENT;

	kreturn = vm_region_64( task, &vm_address, &vm_size,
			VM_REGION_BASIC_INFO, (vm_region_info_t) &vm_basic_info,
			&info_count, &vm_object_name );

	if ( kreturn != KERN_SUCCESS ) {
		mach_port_deallocate( mach_host_self(), task );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Call to vm_region_64() failed.  kreturn = %ld",
			(long) kreturn );
	}

	if ( ( vm_basic_info.reserved != 0 )
	  && ( vm_size == SHARED_TEXT_REGION_SIZE )
	  && ( basic_info.virtual_size >
			(SHARED_TEXT_REGION_SIZE + SHARED_DATA_REGION_SIZE) ) )
	{
		basic_info.virtual_size
			-= (SHARED_TEXT_REGION_SIZE + SHARED_DATA_REGION_SIZE);
	}

	/************** End of magical code ***************/

	meminfo->total_bytes = basic_info.virtual_size;

	/* Get all malloc zones so that we can compute the size of the heap. */

	kreturn = malloc_get_all_zones( task, NULL,
				(vm_address_t **) &malloc_zone_array,
						&num_malloc_zones );

	if ( kreturn != KERN_SUCCESS ) {
		mach_port_deallocate( mach_host_self(), task );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Call to malloc_get_all_zones() failed.  kreturn = %ld",
			(long) kreturn );
	}

#if 0
	MX_DEBUG(-2,("%s: *******************************************", fname));

	MX_DEBUG(-2,("%s: num_malloc_zones = %d, malloc_zone_array = %p",
		fname, num_malloc_zones, malloc_zone_array));

	malloc_zone_print( NULL, TRUE );

	MX_DEBUG(-2,("%s: *******************************************", fname));
#endif

	meminfo->heap_bytes = 0;
	meminfo->allocated_bytes = 0;

	for ( i = 0; i < num_malloc_zones; i++ ) {
		zone = malloc_zone_array[i];

#if 0
		MX_DEBUG( 2,("%s: i = %d, zone = %p", fname, i, zone));

		MX_DEBUG(-2,
		("%s: +++++++++++++++++++++++++++++++++++++++++++", fname));

		malloc_zone_print( zone, TRUE );

		MX_DEBUG(-2,
		("%s: +++++++++++++++++++++++++++++++++++++++++++", fname));
#endif

		malloc_zone_statistics( zone, &zone_statistics );

		meminfo->heap_bytes += zone_statistics.size_allocated;
		meminfo->allocated_bytes += zone_statistics.size_in_use;
	}

	/* We are done, so get rid of the task structure. */

	mach_port_deallocate( mach_host_self(), task );

#if 0
	mx_display_process_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_SOLARIS )

#include <malloc.h>

/* FIXME: Including <string.h> does not seem to be enough to find strtok_r(). */

extern char *
strtok_r( char *, const char *, char **);

MX_EXPORT mx_status_type
mx_get_process_meminfo( unsigned long process_id,
			MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_process_meminfo()";

	FILE *pmap_file;
	struct mallinfo mallinfo_struct;
	int current_process, num_items;
	int buffer_length, blank_length, size_length;
	unsigned long pmap_process_id;
	int heap_seen, total_seen;
	char *address_ptr, *size_ptr, *access_type_ptr, *block_id_ptr;
	char *last_ptr, *after_token_ptr;
	char last_char;
	unsigned long block_size, multiplier;
	char command[80], buffer[120];

	if ( meminfo == (MX_PROCESS_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PROCESS_MEMINFO pointer passed was NULL." );
	}

	if ( process_id == MXF_PROCESS_ID_SELF ) {
		current_process = TRUE;
	} else
	if ( process_id == mx_process_id() ) {
		current_process = TRUE;
	} else {
		current_process = FALSE;
	}

	if ( process_id == MXF_PROCESS_ID_SELF ) {
		process_id = mx_process_id();
	}

	memset( meminfo, 0, sizeof( MX_PROCESS_MEMINFO ) );

	meminfo->process_id = process_id;

	/* Get the process memory map from the 'pmap' command. */

	sprintf( command, "/usr/bin/pmap %lu", process_id );

	MX_DEBUG( 2,("%s: popen() command = '%s'", fname, command));

	pmap_file = popen( command, "r" );

	if ( pmap_file == NULL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Could not run the command '%s'", command );
	}

	/* The first line of output either starts with the process ID
	 * or an error message.
	 */

	fgets( buffer, sizeof(buffer), pmap_file );

	/* Delete any trailing newline. */

	buffer_length = strlen( buffer );

	if ( buffer[buffer_length-1] == '\n' ) {
		buffer[buffer_length-1] = '\0';
	}

	MX_DEBUG( 2,("%s: buffer = '%s'", fname, buffer ));

	/* Parse the response to see if we got a valid response. */

	num_items = sscanf( buffer, "%lu", &pmap_process_id );

	if ( num_items != 1 ) {
		pclose( pmap_file );

		if ( strncmp( buffer, "pmap: cannot examine", 20 ) == 0 ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"The attempt to run the command '%s' failed.  "
			"Error message = '%s'", command, buffer );
		} else {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
		    "The command '%s' returned an unrecognizable response '%s'",
				command, buffer );
		}
	}

	if ( pmap_process_id != process_id ) {
		pclose( pmap_file );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The process id %lu returned by the command '%s' "
		"does not match the originally requested process id %lu.",
			pmap_process_id, command, process_id );
	}

	/* Loop reading lines of output from 'pmap'. */

	heap_seen = FALSE;

	total_seen = FALSE;

	while ( total_seen == FALSE ) {
		fgets( buffer, sizeof(buffer), pmap_file );

		if ( ferror(pmap_file) ) {
			pclose( pmap_file );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"An error occurred while reading the output "
			"from the command '%s'.", command );
		}
		if ( feof(pmap_file) ) {
			pclose( pmap_file );

			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"The output of the command '%s' did not finish "
			"with a 'total' line.", command );
		}

		/* Delete any trailing newline. */

		buffer_length = strlen( buffer );

		if ( buffer[buffer_length-1] == '\n' ) {
			buffer[buffer_length-1] = '\0';
		}

		MX_DEBUG( 2,("%s: buffer = '%s'", fname, buffer ));

		/* Get the address of the memory block. */

		address_ptr = strtok_r( buffer, " ", &last_ptr );

		if ( address_ptr == NULL ) {
			pclose( pmap_file );

			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"The memory address for the next memory block "
			"in the output of the command '%s' was not found.",
				command );
		}

		/* The last line of output starts with the word 'total'
		 * rather than a memory block address.
		 */

		if ( strcmp( address_ptr, "total" ) == 0 ) {
			total_seen = TRUE;
		}

		/* Get the size of the memory block. */

		 size_ptr = strtok_r( NULL, " ", &last_ptr );

		if ( size_ptr == NULL ) {
			pclose( pmap_file );

			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"The memory block size for the next memory block "
			"in the output of the command '%s' was not found.",
				command );
		}

		size_length = strlen( size_ptr );

		last_char = size_ptr[size_length - 1];

		if ( last_char == 'K' ) {
			multiplier = 1024L;
		} else
		if ( last_char == 'M' ) {
			multiplier = 1048576L;
		} else
		if ( last_char == 'B' ) {
			multiplier = 1L;
		} else {
			multiplier = 1L;
		}

		MX_DEBUG( 2,("%s: last_char = '%c', multiplier = %lu",
			fname, last_char, multiplier));

		num_items = sscanf( size_ptr, "%lu", &block_size );

		if ( num_items != 1 ) {
			pclose( pmap_file );

			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Could not find a memory block size in the string '%s'",
				size_ptr );
		}

		block_size = block_size * multiplier;

		MX_DEBUG( 2,("%s: block_size = %lu", fname, block_size));

		if ( total_seen ) {
			MX_DEBUG( 2,("%s: address_ptr = '%s', size_ptr = '%s'",
				fname, address_ptr, size_ptr));

			meminfo->total_bytes = block_size;

			/* Normally, this should be the only way out
			 * of the while() loop.
			 */

			MX_DEBUG( 2,("%s: original heap size = %lu",
				fname, meminfo->heap_bytes));

			break;	/* Exit the while loop(). */
		}

		access_type_ptr = strtok_r( NULL, " ", &last_ptr );

		if ( access_type_ptr == NULL ) {
			pclose( pmap_file );

			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"The access type for the next memory block "
			"in the output of the command '%s' was not found.",
				command );
		}

		/* Look for the last token without using strtok_r().
		 * The reason for this requirement is that fact that
		 * the last token often has embedded space characters
		 * in it.
		 */
#if 0
		/* This makes assumptions about the implementation of 
		 * strtok_r() that are likely to be true, but are not
		 * guaranteed by the documentation.
		 */

		after_token_ptr = last_ptr;
#else
		/* On the other hand, this should be guaranteed to work. */

		after_token_ptr = access_type_ptr + strlen(access_type_ptr) + 1;
#endif

		blank_length = strspn( after_token_ptr, " " );

		block_id_ptr = after_token_ptr + blank_length;

		MX_DEBUG( 2,("%s: address_ptr = '%s', size_ptr = '%s', "
			"access_type_ptr = '%s', block_id_ptr = '%s'",
			fname, address_ptr, size_ptr,
			access_type_ptr, block_id_ptr ));

		/* Determine what kind of block this is.
		 *
		 * FIXME: The heuristics used to determine this are 
		 * not perfect.  In principle, we could do better by
		 * spawning a 'file' command to determine the file
		 * type of the file.  However, that would add more
		 * overhead, so for now we are not going to do that.
		 */

		if ( strcmp( block_id_ptr, "[ heap ]" ) == 0 ) {
			/* Heap memory block */

			meminfo->heap_bytes += block_size;

			/* All program code occurs before the first heap
			 * block, so we must record the fact that we have
			 * seen a heap block.
			 */

			heap_seen = TRUE;
		} else
		if ( strcmp( block_id_ptr, "[ anon ]" ) == 0 ) {
			/* Private memory-mapped memory block */

			meminfo->memory_mapped_bytes += block_size;
		} else
		if ( strcmp( block_id_ptr, "[ stack ]" ) == 0 ) {
			/* Stack block */

			meminfo->stack_bytes += block_size;
		} else
		if ( block_id_ptr[0] == '/' ) {
			/* Absolute pathname seen */

			if ( heap_seen ) {
				/* Shared library block */

				meminfo->library_bytes += block_size;
			} else {
				/* Program code block */

				meminfo->code_bytes += block_size;
			}
		} else
		if ( strchr(block_id_ptr, '/') != NULL ) {
			/* Relative pathname seen */

			if ( heap_seen ) {
				/* Shared library block */

				meminfo->library_bytes += block_size;
			} else {
				/* Program code block */

				meminfo->code_bytes += block_size;
			}
		} else
		if ( strncmp( block_id_ptr, "dev:", 4 ) == 0 ) {
			/* A block ID like this
			 *         dev:136,7 ino:6964620
			 * has been seen.  This is very likely to be a
			 * shared library.
			 */

			meminfo->library_bytes += block_size;
		} else
		if ( strstr( block_id_ptr, ".so" ) != NULL ) {
			/* If the block ID has the string '.so'
			 * embedded in it, then this is probably
			 * a shared library that is located in
			 * the current directory.  As we said
			 * above, this heuristic is not perfect.
			 */

			meminfo->library_bytes += block_size;
		} else {
			mx_warning( "Unrecognized memory block type seen.  "
				"Block ID = '%s', address = %s, size = %lu",
				block_id_ptr, address_ptr, block_size );
		}
	}

	pclose( pmap_file );

	/* If we were asking about the current process, execution of the
	 * popen() function will have affected the reported heap usage.
	 * In this case, we can get better heap statistics from mallinfo().
	 */

	if ( current_process ) {
		mallinfo_struct = mallinfo();

		meminfo->heap_bytes = mallinfo_struct.arena;
		meminfo->allocated_bytes = mallinfo_struct.uordblks;
	}

#if 0
	mx_display_process_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_IRIX )

#include <sys/procfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>

MX_EXPORT mx_status_type
mx_get_process_meminfo( unsigned long process_id,
			MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_process_meminfo()";

	prpsinfo_t prpsinfo_struct;
	struct mallinfo mallinfo_struct;
	unsigned long page_size;
	int psinfo_fd;
	int current_process, status, saved_errno;
	char pname[ MXU_FILENAME_LENGTH + 1 ];

	if ( meminfo == (MX_PROCESS_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PROCESS_MEMINFO pointer passed was NULL." );
	}

	if ( process_id == MXF_PROCESS_ID_SELF ) {
		current_process = TRUE;
	} else
	if ( process_id == mx_process_id() ) {
		current_process = TRUE;
	} else {
		current_process = FALSE;
	}

	if ( process_id == MXF_PROCESS_ID_SELF ) {
		process_id = mx_process_id();
	}

	memset( meminfo, 0, sizeof( MX_PROCESS_MEMINFO ) );

	meminfo->process_id = mx_process_id();

	/* Get system page size. */

	page_size = getpagesize();

	/* Open the /proc file for this process id. */

	sprintf( pname, "/proc/pinfo/%010lu", meminfo->process_id );

	psinfo_fd = open( pname, O_RDONLY );

	if ( psinfo_fd < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to open the file '%s' failed.  "
		"errno = %d, error message = '%s'.",
			pname, saved_errno, strerror( saved_errno ) );
	}

	status = ioctl( psinfo_fd, PIOCPSINFO, &prpsinfo_struct );

	if ( status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to perform a PIOCPSINFO ioctl on the "
		"file '%s' failed.  errno = %d, error message = '%s'.",
			pname, saved_errno, strerror( saved_errno ) );
	}

	meminfo->total_bytes = prpsinfo_struct.pr_size * page_size;

	meminfo->resident_in_memory_bytes =
				prpsinfo_struct.pr_rssize * page_size;

	(void) close( psinfo_fd );

	/* Get heap statistics from mallinfo() */

	if ( current_process ) {
		mallinfo_struct = mallinfo();

		meminfo->heap_bytes = mallinfo_struct.arena;
		meminfo->allocated_bytes = mallinfo_struct.uordblks;
	}

#if 0
	mx_display_process_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_BSD ) || defined( OS_HPUX ) || defined( OS_QNX ) \
	|| defined( OS_TRU64 ) || defined( OS_CYGWIN ) || defined( OS_DJGPP )

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#if ! defined( OS_BSD )
#include <malloc.h>
#endif

MX_EXPORT mx_status_type
mx_get_process_meminfo( unsigned long process_id,
			MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_process_meminfo()";

	struct rusage rusage_struct;
	int status, saved_errno;

	if ( meminfo == (MX_PROCESS_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PROCESS_MEMINFO pointer passed was NULL." );
	}

	if ( process_id != MXF_PROCESS_ID_SELF ) {
		if ( process_id != mx_process_id() ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"%s for process IDs other than the current process "
			"is not supported on this platform.", fname );
		}
	}

	memset( meminfo, 0, sizeof( MX_PROCESS_MEMINFO ) );

	meminfo->process_id = mx_process_id();

	/* Get process memory statistics from getrusage() */

	status = getrusage( RUSAGE_SELF, &rusage_struct );

	if ( status == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to use getrusage() failed.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	meminfo->total_bytes =
		((unsigned long) rusage_struct.ru_maxrss) * 1024L;

#if ! defined( OS_BSD ) && ! defined( OS_DJGPP )
	{
		/* Get heap statistics from mallinfo() */

		struct mallinfo mallinfo_struct;

		meminfo->data_bytes =
			((unsigned long) rusage_struct.ru_idrss) * 1024L;

		meminfo->stack_bytes =
			((unsigned long) rusage_struct.ru_isrss) * 1024L;

		meminfo->code_bytes =
			((unsigned long) rusage_struct.ru_ixrss) * 1024L;

		mallinfo_struct = mallinfo();

		meminfo->heap_bytes = mallinfo_struct.arena;
		meminfo->allocated_bytes = mallinfo_struct.uordblks;
	}
#endif

#if 0
	mx_display_process_meminfo( meminfo );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_RTEMS ) || defined( OS_VXWORKS ) || defined( OS_ECOS )

MX_EXPORT mx_status_type
mx_get_process_meminfo( unsigned long process_id,
			MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_process_meminfo()";

	if ( meminfo == (MX_PROCESS_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PROCESS_MEMINFO pointer passed was NULL." );
	}

	if ( process_id != MXF_PROCESS_ID_SELF ) {
		if ( process_id != mx_process_id() ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"%s for process IDs other than the current process "
			"is not supported on this platform.", fname );
		}
	}

	memset( meminfo, 0, sizeof( MX_PROCESS_MEMINFO ) );

	meminfo->process_id = mx_process_id();

	/* Currently all counters are set to zero for this platform. */

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#elif defined( OS_VMS )

#include <ssdef.h>
#include <jpidef.h>
#include <libdef.h>
#include <starlet.h>
#include <lib$routines.h>

MX_EXPORT mx_status_type
mx_get_process_meminfo( unsigned long process_id,
			MX_PROCESS_MEMINFO *meminfo )
{
	static const char fname[] = "mx_get_process_meminfo()";

	unsigned int vms_pid;
	int vms_status, code;
	unsigned long value;
	short iosb[4];

	unsigned long active_page_table_count;
	unsigned long default_working_set_size;
	unsigned long free_page_count;
	unsigned long global_page_count;
	unsigned long remaining_paging_file_quota;
	unsigned long paging_file_quota;
	unsigned long num_pages_in_working_set;
	unsigned long maximum_authorized_working_set_size;
	unsigned long peak_working_set_size;
	unsigned long working_set_size;

	struct {
		unsigned short buffer_length;
		unsigned short item_code;
		void *buffer_address;
		void *return_length_address;
	} jpi_item_list[] =
		{ {4, JPI$_APTCNT, &active_page_table_count, NULL},
		  {4, JPI$_DFWSCNT, &default_working_set_size, NULL},
		  {4, JPI$_FREPTECNT, &free_page_count, NULL},
		  {4, JPI$_GPGCNT, &global_page_count, NULL},
		  {4, JPI$_PAGFILCNT, &remaining_paging_file_quota, NULL},
		  {4, JPI$_PGFLQUOTA, &paging_file_quota, NULL},
		  {4, JPI$_PPGCNT, &num_pages_in_working_set, NULL},
		  {4, JPI$_WSAUTH, &maximum_authorized_working_set_size, NULL},
		  {4, JPI$_WSPEAK, &peak_working_set_size, NULL},
		  {4, JPI$_WSSIZE, &working_set_size, NULL},
		  {0, 0, NULL, NULL} };

	if ( meminfo == (MX_PROCESS_MEMINFO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PROCESS_MEMINFO pointer passed was NULL." );
	}

	if ( process_id == MXF_PROCESS_ID_SELF ) {
		process_id = mx_process_id();
	}

	memset( meminfo, 0, sizeof( MX_PROCESS_MEMINFO ) );

	meminfo->process_id = mx_process_id();

	/* sys$getjpiw() gives us information about the process as a whole. */

	vms_pid = (unsigned int) meminfo->process_id;

	vms_status = sys$getjpiw( 0, &vms_pid, 0, &jpi_item_list, iosb, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Cannot get job/process information for process id %ld.  "
		"VMS error number %d, error message = '%s'",
			vms_pid, vms_status, strerror( EVMSERR, vms_status ) );
	}

#if 0
	MX_DEBUG(-2,("%s: active_page_table_count = %lu", fname,
				active_page_table_count));
	MX_DEBUG(-2,("%s: default_working_set_size = %lu", fname,
				default_working_set_size));
	MX_DEBUG(-2,("%s: free_page_count = %lu", fname,
				free_page_count));
	MX_DEBUG(-2,("%s: global_page_count = %lu", fname,
				global_page_count));
	MX_DEBUG(-2,("%s: remaining_paging_file_quota = %lu", fname,
				remaining_paging_file_quota));
	MX_DEBUG(-2,("%s: paging_file_quota = %lu", fname,
				paging_file_quota));
	MX_DEBUG(-2,("%s: num_pages_in_working_set = %lu", fname,
				num_pages_in_working_set));
	MX_DEBUG(-2,("%s: maximum_authorized_working_set_size = %lu", fname,
				maximum_authorized_working_set_size));
	MX_DEBUG(-2,("%s: peak_working_set_size = %lu", fname,
				peak_working_set_size));
	MX_DEBUG(-2,("%s: working_set_size = %lu", fname,
				working_set_size));
#endif

	/* FIXME: I am not sure just exactly what each of these quantities are,
	 * so the following is just a guess.
	 */

	meminfo->resident_in_memory_bytes = 512L * working_set_size;

	/* FIXME: The following is surely bogus. */

	meminfo->total_bytes = meminfo->resident_in_memory_bytes;

	/* Get heap memory usage. */

	/* FIXME: Apparently for recent versions of OpenVMS, lib$stat_vm()
	 * does not include memory allocated using malloc(), calloc(), and
	 * realloc().  It only reports memory allocated using lib$get_vm().
	 */

	code = 3;

	vms_status = lib$stat_vm( &code, &value );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Cannot value from lib$stat_vm().  "
		"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );
	}

#if 0
	MX_DEBUG(-2,("%s: lib$stat_vm() value = %lu", fname, value));
#endif

	meminfo->heap_bytes = value;
	meminfo->allocated_bytes = value;

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************************/

#else

#error mx_get_process_meminfo() is not yet defined for this platform.

#endif


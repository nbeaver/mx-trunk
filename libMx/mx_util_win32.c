/*
 * Name:    mx_util_win32.c
 *
 * Purpose: This file defines replacement versions of malloc(), free(),
 *          and friends for Visual C++ and Borland C++ on Win32.
 *
 *          We must make sure that MX DLLs and EXEs all use the same heap,
 *          but the default C runtime creates separate heaps for each DLL
 *          and EXE.  Thus, we define replacements for the malloc(), etc.
 *          functions that use HeapAlloc(), etc. on the heap returned by
 *          GetProcessHeap().
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2007, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_LOG		FALSE
#define DEBUG_SHOW_CALLER	FALSE

#include <stdio.h>

#if defined(OS_WIN32)

#include <windows.h>

#if defined(__BORLANDC__)
#include <alloc.h>
#endif

#include "mx_util.h"

/*-----------------------------------------------------------------------*/

#if defined(OS_WIN32) && (MX_MALLOC_REDIRECT == FALSE)

static HANDLE process_heap = NULL;

static void *
mxp_win32_process_heap_error( void )
{
	static const char fname[] = "mxp_win32_process_heap_error()";

	DWORD last_error_code;
	TCHAR message_buffer[100];

	last_error_code = GetLastError();

	mx_win32_error_message( last_error_code,
		message_buffer, sizeof(message_buffer) );

	(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
	"The attempt to get a HANDLE from GetProcessHeap() failed.  "
	"Error code %ld, error message = '%s'",
		last_error_code, message_buffer );

	return NULL;
}

MX_EXPORT void *
mx_win32_calloc( size_t num_items, size_t item_size )
{
	LPVOID block_ptr;
	size_t num_bytes;

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();

		if ( process_heap == NULL ) {
			return mxp_win32_process_heap_error();
		}
	}

	num_bytes = num_items * item_size;

	block_ptr = HeapAlloc( process_heap, HEAP_ZERO_MEMORY, num_bytes );

#if DEBUG_LOG
	MX_DEBUG(-2,("HeapAlloc( %p, HEAP_ZERO_MEMORY, %ld ) = %p",
			process_heap, (long) num_bytes, block_ptr));
#endif
#if DEBUG_SHOW_CALLER
	mx_stack_traceback();
#endif

	return block_ptr;
}

MX_EXPORT void
mx_win32_free( void *block_ptr )
{
	static const char fname[] = "mx_win32_free()";

	BOOL status;

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();

		if ( process_heap == NULL ) {
			(void) mxp_win32_process_heap_error();
			return;
		}
	}

#if 0
	mx_heap_check();
#endif

	status = HeapFree( process_heap, 0, block_ptr );

#if DEBUG_LOG
	MX_DEBUG(-2,("HeapFree( %p, 0, %p ) = %d",
		process_heap, block_ptr, status));
#endif

	if ( status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[100];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );
#if 0
		mx_stack_traceback();
#endif

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to free memory located at %p "
		"by HeapFree() failed.  "
		"Error code = %ld, error message = '%s'",
			block_ptr, last_error_code, message_buffer );
	}

#if DEBUG_SHOW_CALLER
	mx_stack_traceback();
#endif
	/* If status == 0, then an error occurred and we could get
	 * more information with GetLastError().  However, we have
	 * no way of communicating that back to our caller, so there
	 * is no point in doing it.
	 */

	return;
}

MX_EXPORT void *
mx_win32_malloc( size_t num_bytes )
{
	LPVOID block_ptr;

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();

		if ( process_heap == NULL ) {
			return mxp_win32_process_heap_error();
		}
	}

	block_ptr = HeapAlloc( process_heap, 0, num_bytes );

#if DEBUG_LOG
	MX_DEBUG(-2,("HeapAlloc( %p, 0, %ld ) = %p",
			process_heap, (long) num_bytes, block_ptr));
#endif
#if DEBUG_SHOW_CALLER
	mx_stack_traceback();
#endif

	return block_ptr;
}

MX_EXPORT void *
mx_win32_realloc( void *old_block_ptr, size_t new_num_bytes )
{
	LPVOID new_block_ptr;

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();

		if ( process_heap == NULL ) {
			return mxp_win32_process_heap_error();
		}
	}

	new_block_ptr = HeapReAlloc( process_heap, 0,
					old_block_ptr, new_num_bytes );

#if DEBUG_LOG
	MX_DEBUG(-2,("HeapReAlloc( %p, 0, %p, %ld ) = %p",
			process_heap, old_block_ptr,
			(long) new_num_bytes, new_block_ptr));
#endif
#if DEBUG_SHOW_CALLER
	mx_stack_traceback();
#endif

	return new_block_ptr;
}

#endif /* OS_WIN32 */

#endif /* defined(OS_WIN32) */


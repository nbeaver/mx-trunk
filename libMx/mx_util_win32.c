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

#include <stdio.h>

#if defined(OS_WIN32)

#include <windows.h>

#if defined(__BORLANDC__)
#include <alloc.h>
#endif

#include "mx_util.h"

#define DEBUG_USING_DUMA	FALSE

#define DEBUG_MX_DUMA_LOGGING	FALSE

/*-----------------------------------------------------------------------*/

#if DEBUG_USING_DUMA

/* If we are using the DUMA memory allocation debugging package, then we
 * must _not_ redirect allocations to the HeapAlloc() family of function,
 * since that will prevent DUMA from intercepting the calls to memory
 * allocation functions.
 */

#ifdef calloc
#undef calloc
#endif

#ifdef free
#undef free
#endif

#ifdef malloc
#undef malloc
#endif

#ifdef realloc
#undef realloc
#endif

MX_EXPORT void *
mx_win32_calloc( size_t num_items, size_t item_size )
{
	void *block_ptr;

	block_ptr = calloc( num_items, item_size );

#if DEBUG_MX_DUMA_LOGGING
	MX_DEBUG(-2,("%p = mx_win32_calloc( %lu, %lu )",
		block_ptr, (unsigned long) num_items,
		(unsigned long) item_size));
#endif

	return block_ptr;
}

MX_EXPORT void
mx_win32_free( void *block_ptr )
{
#if DEBUG_MX_DUMA_LOGGING
	MX_DEBUG(-2,("mx_win32_free( %p )", block_ptr));
#endif

	free( block_ptr );
}

MX_EXPORT void *
mx_win32_malloc( size_t num_bytes )
{
	void *block_ptr;

	block_ptr = malloc( num_bytes );

#if DEBUG_MX_DUMA_LOGGING
	MX_DEBUG(-2,("%p = mx_win32_malloc( %lu )",
		block_ptr, (unsigned long) num_bytes));
#endif

	return block_ptr;
}

MX_EXPORT void *
mx_win32_realloc( void *old_block_ptr, size_t new_num_bytes )
{
	void *new_block_ptr;

	new_block_ptr = realloc( old_block_ptr, new_num_bytes );

#if DEBUG_MX_DUMA_LOGGING
	MX_DEBUG(-2,("%p = mx_win32_realloc( %p, %lu )",
		new_block_ptr, old_block_ptr,
		(unsigned long) new_num_bytes));
#endif

	return new_block_ptr;
}

/*-----------------------------------------------------------------------*/

#else /* not DEBUG_USING_DUMA */

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

	return new_block_ptr;
}

#endif /* not DEBUG_USING_DUMA */

#endif /* defined(OS_WIN32) */


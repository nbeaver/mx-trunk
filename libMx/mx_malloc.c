/*
 * Name:    mx_malloc.c
 *
 * Purpose: This file provides memory allocation related functions for MX.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2007, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_DEBUG_LOG		FALSE
#define MX_DEBUG_SHOW_CALLER	FALSE
#define MX_DEBUG_HEAP		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if defined(OS_WIN32)
#  include <windows.h>

#  if defined(__BORLANDC__)
#    include <alloc.h>
#  endif
#endif

#include "mx_util.h"


/*-------------------------------------------------------------------------*/

/* We provide a version of strdup() in libMx for the sake of
 * Microsoft Windows, so that mx_strdup() can be compatible
 * with pointers produced by mx_win32_malloc() and friends.
 *
 * It is also used by build targets that do not natively
 * define strdup().
 */

MX_EXPORT char *
mx_strdup( const char *original )
{
	char *duplicate;
	size_t original_length;

	if ( original == NULL ) {
		errno = EINVAL;

		return NULL;
	}

	original_length = strlen(original) + 1;

	duplicate = malloc( original_length );

	if ( duplicate == NULL ) {
		errno = ENOMEM;

		return NULL;
	}

	strlcpy( duplicate, original, original_length );

	return duplicate;
}

/*-----------------------------------------------------------------------*/

#if defined(OS_WIN32) && (MX_MALLOC_REDIRECT == FALSE)

/*
 * This section defines replacement versions of malloc(), free(), and friends
 * for Visual C++ and Borland C++ on Win32.
 *
 * We must make sure that MX DLLs and EXEs all use the same heap, but the
 * default C runtime creates separate heaps for each DLL and EXE.  Thus,
 * we define replacements for the malloc(), etc. functions that use
 * HeapAlloc(), etc. on the heap returned by GetProcessHeap().
 */

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

#if MX_DEBUG_HEAP
	int heap_status;
#endif

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();

		if ( process_heap == NULL ) {
			return mxp_win32_process_heap_error();
		}
	}

#if MX_DEBUG_HEAP
	heap_status = mx_heap_check( MXF_HEAP_CHECK_CORRUPTED_ALL );

	if ( heap_status == FALSE ) {
		mx_warning( "calloc(): Heap corrupted before allocation." );
		mx_stack_traceback();
	}
#endif

	num_bytes = num_items * item_size;

	block_ptr = HeapAlloc( process_heap, HEAP_ZERO_MEMORY, num_bytes );

#if MX_DEBUG_LOG
	MX_DEBUG(-2,("HeapAlloc( %p, HEAP_ZERO_MEMORY, %ld ) = %p",
			process_heap, (long) num_bytes, block_ptr));
#endif
#if MX_DEBUG_SHOW_CALLER
	mx_stack_traceback();
#endif

#if MX_DEBUG_HEAP
	heap_status = mx_heap_check( MXF_HEAP_CHECK_CORRUPTED_ALL );

	if ( heap_status == FALSE ) {
		mx_warning( "calloc(): Heap corrupted after allocation." );
		mx_stack_traceback();
	}
#endif

	return block_ptr;
}

MX_EXPORT void
mx_win32_free( void *block_ptr )
{
	static const char fname[] = "mx_win32_free()";

	BOOL os_status;

#if MX_DEBUG_HEAP
	int heap_status;
#endif

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();

		if ( process_heap == NULL ) {
			(void) mxp_win32_process_heap_error();
			return;
		}
	}

#if MX_DEBUG_HEAP
	heap_status = mx_heap_check( MXF_HEAP_CHECK_CORRUPTED_ALL );

	if ( heap_status == FALSE ) {
		mx_warning( "free(): Heap corrupted before free." );
		mx_stack_traceback();
	}
#endif

	os_status = HeapFree( process_heap, 0, block_ptr );

#if MX_DEBUG_LOG
	MX_DEBUG(-2,("HeapFree( %p, 0, %p ) = %d",
		process_heap, block_ptr, status));
#endif

	if ( os_status == 0 ) {
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

#if MX_DEBUG_HEAP
	heap_status = mx_heap_check( MXF_HEAP_CHECK_CORRUPTED_ALL );

	if ( heap_status == FALSE ) {
		mx_warning( "free(): Heap corrupted after free." );
		mx_stack_traceback();
	}
#endif

#if MX_DEBUG_SHOW_CALLER
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

#if MX_DEBUG_HEAP
	int heap_status;
#endif

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();

		if ( process_heap == NULL ) {
			return mxp_win32_process_heap_error();
		}
	}

#if MX_DEBUG_HEAP
	heap_status = mx_heap_check( MXF_HEAP_CHECK_CORRUPTED_ALL );

	if ( heap_status == FALSE ) {
		mx_warning( "malloc(): Heap corrupted before allocation." );
		mx_stack_traceback();
	}
#endif

	block_ptr = HeapAlloc( process_heap, 0, num_bytes );

#if MX_DEBUG_LOG
	MX_DEBUG(-2,("HeapAlloc( %p, 0, %ld ) = %p",
			process_heap, (long) num_bytes, block_ptr));
#endif
#if MX_DEBUG_SHOW_CALLER
	mx_stack_traceback();
#endif

#if MX_DEBUG_HEAP
	heap_status = mx_heap_check( MXF_HEAP_CHECK_CORRUPTED_ALL );

	if ( heap_status == FALSE ) {
		mx_warning( "malloc(): Heap corrupted after allocation." );
		mx_stack_traceback();
	}
#endif

	return block_ptr;
}

MX_EXPORT void *
mx_win32_realloc( void *old_block_ptr, size_t new_num_bytes )
{
	LPVOID new_block_ptr;

#if MX_DEBUG_HEAP
	int heap_status;
#endif

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();

		if ( process_heap == NULL ) {
			return mxp_win32_process_heap_error();
		}
	}

#if MX_DEBUG_HEAP
	heap_status = mx_heap_check( MXF_HEAP_CHECK_CORRUPTED_ALL );

	if ( heap_status == FALSE ) {
		mx_warning( "realloc(): Heap corrupted before reallocation." );
		mx_stack_traceback();
	}
#endif

	new_block_ptr = HeapReAlloc( process_heap, 0,
					old_block_ptr, new_num_bytes );

#if MX_DEBUG_LOG
	MX_DEBUG(-2,("HeapReAlloc( %p, 0, %p, %ld ) = %p",
			process_heap, old_block_ptr,
			(long) new_num_bytes, new_block_ptr));
#endif
#if MX_DEBUG_SHOW_CALLER
	mx_stack_traceback();
#endif

#if MX_DEBUG_HEAP
	heap_status = mx_heap_check( MXF_HEAP_CHECK_CORRUPTED_ALL );

	if ( heap_status == FALSE ) {
		mx_warning( "realloc(): Heap corrupted after reallocation." );
		mx_stack_traceback();
	}
#endif

	return new_block_ptr;
}

#endif /* defined(OS_WIN32) */

/*-----------------------------------------------------------------------*/

#if defined(DEBUG_MPATROL) && ( MPATROL_VERSION <= 10501 )

/* MX uses mallinfo(), but Mpatrol does not provide an implementation of
 * mallinfo().  We define a stub version of mallinfo() here so that we do
 * not accidentally use the system runtime library's version instead.
 *
 * We can hope that versions of Mpatrol newer than 1.5.1 will provide a
 * native implementation of mallinfo().
 */

MX_EXPORT struct mallinfo
mallinfo( void )
{
	struct mallinfo mallinfo_struct;
	int mp_status;
	__mp_heapinfo heapinfo_struct;

	memset( &mallinfo_struct, 0, sizeof(struct mallinfo) );

	/* Mpatrol does provide enough information to return the
	 * total size of the allocated blocks.
	 */

	mp_status = __mp_stats( &heapinfo_struct );

	if ( mp_status == 0 ) {
		return mallinfo_struct;
	}

	mallinfo_struct.uordblks = heapinfo_struct.atotal;

	return mallinfo_struct;
}

#endif

/*-----------------------------------------------------------------------*/


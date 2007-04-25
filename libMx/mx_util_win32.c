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
 *          The default versions of malloc()
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#if defined(OS_WIN32) && (defined(_MSC_VER) || defined(__BORLANDC__))

#include <windows.h>

#if defined(__BORLANDC__)
#include <alloc.h>
#endif

#include "mx_util.h"

static HANDLE process_heap = NULL;

MX_EXPORT void *
mx_win32_calloc( size_t num_items, size_t item_size )
{
	LPVOID block_ptr;
	size_t num_bytes;

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();
	}

	num_bytes = num_items * item_size;

	block_ptr = HeapAlloc( process_heap, HEAP_ZERO_MEMORY, num_bytes );

	return block_ptr;
}

MX_EXPORT void
mx_win32_free( void *block_ptr )
{
	BOOL status;

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();
	}

	status = HeapFree( process_heap, 0, block_ptr );

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
	}

	new_block_ptr = HeapReAlloc( process_heap, 0,
					old_block_ptr, new_num_bytes );

	return new_block_ptr;
}

#endif /* defined(OS_WIN32) && (defined(_MSC_VER) || defined(__BORLANDC__)) */


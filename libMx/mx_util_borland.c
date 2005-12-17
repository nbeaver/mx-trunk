/*
 * Name:    mx_util_borland.c
 *
 * Purpose: This file defines replacement versions of malloc(), free(),
 *          and friends for Borland C++ 5.5 on Win32.
 *
 *          I wrote replacements for the functions to get around mysterious
 *          behavior that I found when using the standard Borland functions.
 *          'mxserver' always allocates a large network I/O buffer when a
 *          new MX client connects to the server.  As of December 2005,
 *          the second time an MX network client connects to the server,
 *          the attempt to malloc() a 163840 byte network buffer hangs.
 *          That is, the program never returns from malloc() and the CPU
 *          utilization of 'mxserver' goes up to above 95% and stays there
 *          until you kill it.  This kind of behavior only happens with
 *          the 'win32-borland' build target and not with any of the
 *          other MX build targets, so this seems to point to a problem
 *          with the Borland runtime libraries, or some such.
 *
 *          At the moment (Dec. 2005), I do not have time to diagnose in
 *          detail the reasons for this behavior, so I have chosen to
 *          implement my own versions of malloc(), free() and friends
 *          directly on top of the Win32 Heap functions.  I have seen
 *          other signs of problems such as memory leaks with the
 *          Borland-provided functions, so this may all be for the best.
 *          Hopefully I will not live to regret this.
 *
 * Author:  William Lavender
 *
 * FIXME:   This file should not need to exist.
 *
 * FIXME:   This file does not really work yet and has been disabled in
 *          the makefile.
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#if defined( OS_WIN32 ) && defined( __BORLANDC__ )

#include <windows.h>
#include <alloc.h>

static HANDLE process_heap = NULL;

void * __export
calloc( size_t num_items, size_t item_size )
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

void __export
free( void *block_ptr )
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

void * __export
malloc( size_t num_bytes )
{
	LPVOID block_ptr;

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();
	}

	block_ptr = HeapAlloc( process_heap, 0, num_bytes );

	return block_ptr;
}

void * __export
realloc( void *old_block_ptr, size_t new_num_bytes )
{
	LPVOID new_block_ptr;

	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();
	}

	new_block_ptr = HeapReAlloc( process_heap, 0,
					old_block_ptr, new_num_bytes );

	return new_block_ptr;
}

int __export
heapcheck( void )
{
	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();
	}

	return _HEAPOK;
}

int __export
heapcheckfree( unsigned int fillvalue )
{
	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();
	}

	return _HEAPOK;
}

int __export
heapchecknode( void *node_ptr )
{
	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();
	}

	return _HEAPEMPTY;
}

int __export
heapfillfree( unsigned int fillvalue )
{
	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();
	}

	return _HEAPOK;
}

int __export
heapwalk( struct heapinfo *hi )
{
	if ( process_heap == NULL ) {
		process_heap = GetProcessHeap();
	}

	return _HEAPEND;
}

#endif /* defined( OS_WIN32 ) && defined( __BORLANDC__ ) */


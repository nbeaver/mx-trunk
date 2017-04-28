/*
 * Name:    mx_heap.c
 *
 * Purpose: MX functions to check the heap for corruption.
 *
 *          This functionality is not available on all platforms.  Even if
 *          it is, it may not work reliably if the program is in the process
 *          of crashing when this function is invoked.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2005, 2007-2008, 2010, 2012, 2015-2017
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#if defined( OS_WIN32 )
#include <windows.h>
#endif

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_stdint.h"

#if defined(DMALLOC)

/* This uses the dmalloc malloc() debugger from http://dmalloc.com/  */

MX_EXPORT int
mx_heap_pointer_is_valid( void *pointer )
{
	DMALLOC_SIZE user_size;
	DMALLOC_SIZE total_size;
	char *filename;
	unsigned int line_number;
	DMALLOC_PNT return_address;
	unsigned long user_mark;
	unsigned long num_times_seen;
	int status;

	status = dmalloc_examine( pointer, &user_size, &total_size,
			&filename, &line_number, &return_address,
			&user_mark, &num_times_seen );

	MX_DEBUG(-2,
	("dmalloc_examine(): status = %d, ptr = %p, us = %lu, ts = %lu, "
	"file = '%s', line = %d, ra = %p, um = %lu, seen = %lu",
		status, pointer,
		(unsigned long) user_size,
		(unsigned long) total_size,
		filename, line_number, return_address,
		user_mark, num_times_seen));

	if ( status == DMALLOC_NOERROR ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#elif defined( OS_WIN32 )

/* Theoretically, one could use IsBadReadPtr() for checking pointers.
 * However, MSDN and mailing list messages on the net say that
 * dereferencing invalid pointers can sometimes cause automatic
 * stack expansion to fail, so we should not use IsBadReadPtr().
 *
 * MSDN says that the use of HeapValidate() can degrade performance,
 * so we should not use this routinely.
 */

MX_EXPORT int
mx_heap_pointer_is_valid( void *pointer )
{
	if ( pointer == NULL ) {
		return FALSE;
	} else {
		return HeapValidate( GetProcessHeap(), 0, pointer );
	}
}

#else

/* If we do not have a heap-specific pointer check function, then we
 * just check to see if the pointer points to valid memory.
 */

MX_EXPORT int
mx_heap_pointer_is_valid( void *pointer )
{
	int result;

	result = mx_pointer_is_valid( pointer, sizeof(void *), R_OK );

	return result;
}

#endif

/*-------------------------------------------------------------------------*/

MX_EXPORT void
mx_free_pointer( void *pointer )
{
	if ( pointer == NULL ) {
		return;
	}

	if ( mx_heap_pointer_is_valid( pointer ) ) {
		free( pointer );
		return;
	} else {
		/* Experimentation shows that freeing a pointer that fails
		 * the mx_heap_pointer_is_valid() test usually causes a
		 * program crash, so we must just live with the possibilty
		 * of a memory leak instead.
		 */

		mx_warning(
	"Pointer %p passed to mx_free_pointer() is not a valid heap pointer.",
			pointer );

		return;
	}
}

/*-------------------------------------------------------------------------*/

#if defined( OS_WIN32 )

/* Warning: MSDN says that using HeapValidate() can degrade the performance
 * of your program until it exits.
 */

MX_EXPORT int
mx_heap_check( unsigned long heap_flags )
{
	static const char fname[] = "mx_heap_check()";

	DWORD number_of_heaps;
	DWORD maximum_number_of_heaps;
	HANDLE heap_handle_array[100];
	HANDLE heap_handle;
	BOOL validate_status;
	int heap_ok;
	unsigned long i;

	/* This implementation does not call GetProcessHeaps() in a loop
	 * to resize heap_handle_array to the right size, since this will
	 * result in an infinite loop if mx_heap_check() is called from
	 * within any of the mx_win32_... allocation functions.  So we
	 * use a fixed array instead which is probably large enough.
	 */

	static const char getprocessheaps_error[] =
		"ERROR: A call to GetProcessHeaps() failed.\n";

	static const char toomanyheaps_error[] = 
		"ERROR: Too many heaps reported by GetProcessHeaps().  "
		"Increase the size of heap_handle_array and recompile MX "
		"to fix this.\n";

	maximum_number_of_heaps = sizeof( heap_handle_array )
					/ sizeof( heap_handle_array[0] );

	number_of_heaps = GetProcessHeaps( maximum_number_of_heaps,
						heap_handle_array );

	if ( number_of_heaps == 0 ) {
		write(2, getprocessheaps_error, sizeof(getprocessheaps_error));
	}

	if ( number_of_heaps > maximum_number_of_heaps ) {
		write(2, toomanyheaps_error, sizeof(toomanyheaps_error));
		return FALSE;
	}

	heap_ok = TRUE;

	for ( i = 0; i < number_of_heaps; i++ ) {

		heap_handle = heap_handle_array[i];

		validate_status = HeapValidate( heap_handle, 0, 0 );

		if ( validate_status == 0 ) {
			if ( heap_flags & MXF_HEAP_CHECK_CORRUPTED_VERBOSE ) {
#if defined(_WIN64)
			    mx_warning( "%s: Heap %lu (%#I64x) is CORRUPTED.",
					fname, i, (uint64_t) heap_handle );
#else
			    mx_warning( "%s: Heap %lu (%#lx) is CORRUPTED.",
					fname, i, (unsigned long) heap_handle );
#endif
			}

			heap_ok = FALSE;
		} else {
			if ( heap_flags & MXF_HEAP_CHECK_OK_VERBOSE ) {
#if defined(_WIN64)
			    mx_info( "%s: Heap %lu (%#I64x) is OK.",
					fname, i, (uint64_t) heap_handle );
#else
			    mx_info( "%s: Heap %lu (%#lx) is OK.",
					fname, i, (unsigned long) heap_handle );
#endif
			}
		}
	}

	if ( heap_ok ) {
		if ( heap_flags & MXF_HEAP_CHECK_OK ) {
			mx_info( "%s: Heap is OK", fname );
		}
	} else {
		if ( heap_flags & MXF_HEAP_CHECK_CORRUPTED ) {
			mx_warning("%s: Heap is corrupted.", fname);
		}
	}

	if ( heap_flags & MXF_HEAP_CHECK_STACK_TRACEBACK ) {
		mx_stack_traceback();
	}

	return heap_ok;
}

#elif defined( OS_MACOSX )

#include <malloc/malloc.h>

MX_EXPORT int
mx_heap_check( unsigned long heap_flags )
{
	static const char fname[] = "mx_heap_check()";

	boolean_t ok;
	mx_bool_type heap_ok;

	heap_ok = TRUE;

	ok = malloc_zone_check( NULL );

	if ( ok ) {
		if ( heap_flags & MXF_HEAP_CHECK_OK ) {
			mx_info("%s: Heap is OK.", fname);
		}
	} else {
		if ( heap_flags & MXF_HEAP_CHECK_CORRUPTED ) {
			mx_warning("%s: Heap is corrupted.", fname);
		}

		heap_ok = FALSE;
	}

	return heap_ok;
}

#else /* No heap check support. */

MX_EXPORT int
mx_heap_check( unsigned long heap_flags )
{
	return TRUE;
}

#endif


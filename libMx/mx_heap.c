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
 * Copyright 2004-2005, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"

#include "mx_util.h"

#if defined( OS_WIN32 )

#  include <windows.h>

#  if defined( _DEBUG )

/* Theoretically, one could use IsBadReadPtr() for non-debug builds.
 * However, MSDN and mailing list messages on the net say that
 * dereferencing invalid pointers can sometimes cause automatic
 * stack expansion to fail, so we should not use IsBadReadPtr().
 *
 * In addition, MSDN says that the use of HeapValidate() can degrade
 * performance, so we do not use it with non-debug builds.
 */

MX_EXPORT int
mx_is_valid_heap_pointer( void *pointer )
{
	if ( pointer == NULL ) {
		return FALSE;
	} else {
		return HeapValidate( GetProcessHeap(), 0, pointer );
	}
}

#  else

MX_EXPORT int
mx_is_valid_heap_pointer( void *pointer )
{
	if ( pointer == NULL ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

#  endif
#endif

/*-------------------------------------------------------------------------*/

#if defined( OS_WIN32 )

/* Warning: MSDN says that using HeapValidate() can degrade the performance
 * of your program until it exits.
 */

MX_EXPORT int
mx_heap_check( void )
{
	static const char fname[] = "mx_heap_check()";

	DWORD number_of_heaps, new_number_of_heaps;
	HANDLE *heap_handle_array;
	HANDLE heap_handle;
	BOOL validate_status;
	unsigned long i, heap_handle_attempts;
	int heap_ok;

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
			(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"array of %lu heap handles.",
				new_number_of_heaps );

			return FALSE;
		}

		number_of_heaps = new_number_of_heaps;
	}

	if ( i >= heap_handle_attempts ) {
		(void) mx_error( MXE_UNKNOWN_ERROR, fname,
		"Unable to get a complete array of process heap "
		"handles after %lu attempts.", heap_handle_attempts );

		return FALSE;
	}

	heap_ok = TRUE;

	for ( i = 0; i < number_of_heaps; i++ ) {

		heap_handle = heap_handle_array[i];

		validate_status = HeapValidate( heap_handle, 0, 0 );

		if ( validate_status == 0 ) {
			MX_DEBUG( 2,("%s: Heap %lu (%#lx) is CORRUPTED.",
				fname, i, (unsigned long) heap_handle ));

			heap_ok = FALSE;
		} else {
			MX_DEBUG( 2,("%s: Heap %lu (%#lx) is OK.",
				fname, i, (unsigned long) heap_handle ));
		}
	}

	if ( heap_ok ) {
		mx_info( "%s: Heap is OK", fname );
	} else {
		mx_warning("%s: Heap is corrupted.", fname);
	}

	return heap_ok;
}

#elif defined( OS_MACOSX )

#include <malloc/malloc.h>

MX_EXPORT int
mx_heap_check( void )
{
	static const char fname[] = "mx_heap_check()";

	boolean_t ok;

	ok = malloc_zone_check( NULL );

	if ( ok ) {
		mx_info("%s: Heap is OK.", fname);

		return TRUE;
	} else {
		mx_warning("%s: Heap is corrupted.", fname);

		return FALSE;
	}
}

#else /* No heap check support. */

MX_EXPORT int
mx_heap_check( void )
{
	return TRUE;
}

#endif


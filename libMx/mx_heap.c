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
 * Copyright 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"

#include "mx_util.h"

#if defined( OS_WIN32 ) && defined( _DEBUG )

/* Theoretically, one could use IsBadReadPtr() for non-debug builds.
 * However, MSDN and mailing list messages on the net say that
 * dereferencing invalid pointers can sometimes cause automatic
 * stack expansion to fail, so we do not use IsBadReadPtr().
 */

MX_EXPORT int
mx_is_valid_heap_pointer( void *pointer )
{
	if ( pointer == NULL ) {
		return FALSE;
	} else {
		return _CrtIsValidHeapPointer( pointer );
	}
}

#else

MX_EXPORT int
mx_is_valid_heap_pointer( void *pointer )
{
	if ( pointer == NULL ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

#endif

/*-------------------------------------------------------------------------*/

#if defined( OS_WIN32 )

#if defined( _DEBUG )
MX_EXPORT int
mx_heap_check( void )
{
	static const char fname[] = "mx_heap_check()";

	int status;

	status = _CrtCheckMemory();

	if ( status ) {
		mx_info("%s: Heap is OK.", fname);
	} else {
		mx_warning("%s: Heap is corrupted.", fname);
	}

	return status;
}
#else
MX_EXPORT int
mx_heap_check( void )
{
	static const char fname[] = "mx_heap_check()";

	mx_warning( "%s: Win32 heap checking is only supported for "
			"debug versions of the C runtime libraries." );

	return TRUE;
}
#endif

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


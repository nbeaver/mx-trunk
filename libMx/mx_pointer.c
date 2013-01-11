/*
 * Name:    mx_pointer.c
 *
 * Purpose: Utility functions for pointer variables.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2008, 2010-2011, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_POINTER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_vm_alloc.h"

/*-------------------------------------------------------------------------*/

#if defined(OS_WIN32)

#include <windows.h>

/* The following implementation is inspired by the following web page:
 *
 *     http://blogs.msdn.com/ericlippert/articles/105186.aspx
 *
 * VirtualQuery() is used since it is safe to call at any time.  The
 * alternative would be to use the IsBadxxxPtr() family of functions.
 * However, the IsBadxxxPtr() functions are very unsafe and should 
 * _never_ be used, since they can have painful side effects like
 * disabling automatic stack expansion.  It is far safer to use
 * VirtualQuery().  Unfortunately, VirtualQuery() is very _slow_.
 */

MX_EXPORT int
mx_pointer_is_valid( void *pointer, size_t length, int access_mode )
{
	DWORD dwSize, dwFlags;
	MEMORY_BASIC_INFORMATION meminfo;
	unsigned long pointer_offset, unused_region_size;

	if ( pointer == NULL )
		return FALSE;

	if ( access_mode & W_OK ) {
		dwFlags = PAGE_READWRITE | PAGE_WRITECOPY 
			| PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
	} else
	if ( access_mode & R_OK ) {
		dwFlags = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY 
				| PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE
				| PAGE_EXECUTE_WRITECOPY;
	} else {
		return FALSE;
	}

	memset( &meminfo, 0, sizeof(meminfo) );

	dwSize = VirtualQuery( pointer, &meminfo, sizeof(meminfo) );

	/* VirtualQuery() may return 0, if the pointer is a kernel-mode
	 * pointer.
	 */

	if ( dwSize == 0 )
		return FALSE;

	if ( meminfo.State != MEM_COMMIT )
		return FALSE;

	if ( (meminfo.Protect & dwFlags) == 0 )
		return FALSE;

	if ( length > meminfo.RegionSize )
		return FALSE;

	/* pointer_offset contains the offset of the pointer relative
	 * to the start of the memory region it is in.
	 */

	pointer_offset = (unsigned long)
		( ((char *) pointer) - ((char *) meminfo.BaseAddress) );

	/* unused_region_size is the amount of the memory region that is
	 * not used by the memory range specified in the call to this
	 * routine.
	 */

	unused_region_size = (unsigned long) ( meminfo.RegionSize - length );

	/* If pointer_offset is bigger than unused_region_size, then the
	 * end of the requested memory range is beyond the end of the
	 * current memory region.
	 */

	if ( pointer_offset > unused_region_size )
		return FALSE;

	return TRUE;
}

#endif


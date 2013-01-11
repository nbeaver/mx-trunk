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

#if defined(OS_LINUX) | defined(OS_MACOSX) | defined(OS_BSD) \
	| defined(OS_SOLARIS) | defined(OS_IRIX)

/* This is a generic implementation of mx_pointer_is_valid()
 * that uses the new mx_vm_get_protection() function.
 */

MX_EXPORT int
mx_pointer_is_valid( void *pointer, size_t length, int access_mode )
{
#if MX_POINTER_DEBUG
	static const char fname[] = "mx_pointer_is_valid()";
#endif

	int valid;
	mx_bool_type address_range_exists;
	unsigned long protection_flags;
	mx_status_type mx_status;

	mx_status = mx_vm_get_protection( pointer, length,
				&address_range_exists, &protection_flags );

	if ( mx_status.code != MXE_SUCCESS ) {
		valid = FALSE;
	} else
	if ( address_range_exists == FALSE ) {
		valid = FALSE;
	} else {
		/* We only care about the lowest 3 bits of protection_flags. */

		protection_flags &= 0x7;

		if ( protection_flags == access_mode ) {
			valid = TRUE;
		} else {
			valid = FALSE;
		}
	}

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,
	("%s: pointer %p, length = %lu, access_mode = %#x, valid = %d",
		fname, pointer, (unsigned long) length, access_mode, valid ));
#endif

	return valid;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

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

/*-------------------------------------------------------------------------*/

#elif defined(OS_VMS)

#include <sys_starlet_c/psldef.h>

/* FIXME: I should not have to make these definitions, but including
 * <builtins.h> does not do the trick for some reason.
 */

int __PAL_PROBER( const void *__base_address, int __length, char __mode );
int __PAL_PROBEW( const void *__base_address, int __length, char __mode );

MX_EXPORT int
mx_pointer_is_valid( void *pointer, size_t length, int access_mode )
{
	int valid;

	if ( pointer == NULL ) {
		return FALSE;
	}

	valid = FALSE;

	if ( (access_mode & R_OK) && (access_mode & W_OK) ) {
		valid = __PAL_PROBER( pointer, length, PSL$C_USER );

		if ( valid ) {
			valid = __PAL_PROBEW( pointer, length, PSL$C_USER );
		}
	} else
	if ( access_mode & R_OK ) {
		valid = __PAL_PROBER( pointer, length, PSL$C_USER );
	} else
	if ( access_mode & W_OK ) {
		valid = __PAL_PROBEW( pointer, length, PSL$C_USER );
	}

	return valid;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_CYGWIN) || defined(OS_QNX) || defined(OS_ECOS) \
	|| defined(OS_RTEMS) || defined(OS_VXWORKS) || defined(OS_BSD) \
	|| defined(OS_HPUX) || defined(OS_TRU64) || defined(OS_DJGPP) \
	|| defined(OS_UNIXWARE) || defined(OS_HURD)

MX_EXPORT int
mx_pointer_is_valid( void *pointer, size_t length, int access_mode )
{
	if ( pointer == NULL ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

#else

#error mx_pointer_is_valid() has not yet been defined for this build target.

#endif


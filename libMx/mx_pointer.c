/*
 * Name:    mx_pointer.c
 *
 * Purpose: Utility functions for pointer variables.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
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

#if defined(OS_WIN32)

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
mx_is_valid_pointer( void *pointer, size_t length, int access_mode )
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

	pointer_offset = (unsigned long) ( pointer - meminfo.BaseAddress );

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

#elif defined( OS_LINUX )

#include <sys/mman.h>

MX_EXPORT int
mx_is_valid_pointer( void *pointer, size_t length, int access_mode )
{
	static const char fname[] = "mx_is_valid_pointer()";

	FILE *file;
	char buffer[300];
	int i, argc, valid, saved_errno;
	char **argv;
	unsigned long start_address, end_address, pointer_address;
	char permissions[20];

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s invoked for pointer %p, length %lu, access_mode %d",
		fname, pointer, (unsigned long) length, access_mode ));
#endif

	/* Under Linux, the preferred technique is to read the
	 * necessary information from /proc/self/maps.
	 */

	file = fopen( "/proc/self/maps", "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to open '/proc/self/maps' failed with "
		"errno = %d, error message = '%s'.",
			saved_errno, strerror( saved_errno ) );

		return FALSE;
	}

	/* If we get here, then opening /proc/self/maps succeeded. */

	pointer_address = (unsigned long) pointer;

	for ( i = 0; ; i++ ) {

		valid = FALSE;

		fgets( buffer, sizeof(buffer), file );

		if ( feof(file) ) {
			valid = FALSE;
			break;		/* Exit the for() loop. */
		} else
		if ( ferror(file) ) {
			(void) mx_error( MXE_FILE_IO_ERROR, fname,
			"The attempt to read line %d of the output "
			"from /proc/self/maps failed.", i+1 );

			valid = FALSE;
			break;		/* Exit the for() loop. */
		}

#if MX_POINTER_DEBUG
		MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif

		/* Split up the string using both space characters
		 * and '-' characters as delimiters.
		 */

		mx_string_split( buffer, " -", &argc, &argv );

		start_address = mx_hex_string_to_unsigned_long( argv[0] );
		end_address   = mx_hex_string_to_unsigned_long( argv[1] );

		strlcpy( permissions, argv[2], sizeof(permissions) );

		mx_free(argv);

#if MX_POINTER_DEBUG
		MX_DEBUG(-2,("%s: pointer = %p, pointer_address = %#lx, "
			"i = %d, start_address = %#lx, "
			"end_address = %#lx, permissions = '%s'", fname,
			pointer, pointer_address, i,
			start_address, end_address, permissions));
#endif
		/* If the pointer is located before the start of the
		 * first memory block, then it is definitely bad.
		 */

		if ( (i == 0) && (pointer_address <= start_address) ) {
			valid = FALSE;

			break;		/* Exit the for() loop. */
		}

		/* See if the pointer is in the current memory block. */

		if ( (start_address <= pointer_address)
			&& (pointer_address <= end_address ) )
		{
#if MX_POINTER_DEBUG
			MX_DEBUG(-2,("%s: pointer is in this block.",fname));
#endif

			if ( access_mode == (R_OK|W_OK) ) {
				if ( ( permissions[0] == 'r' )
				  && ( permissions[1] == 'w' ) )
				{
					valid = TRUE;
				} else {
					valid = FALSE;
				}
			} else
			if ( access_mode == R_OK ) {
				if ( permissions[0] == 'r' ) {
					valid = TRUE;
				} else {
					valid = FALSE;
				}
			} else
			if ( access_mode == W_OK ) {
				if ( permissions[0] == 'w' ) {
					valid = TRUE;
				} else {
					valid = FALSE;
				}
			} else {
				valid = FALSE;
			}

			break;		/* Exit the for() loop. */
		}
	}

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: valid = %d", fname, valid));
#endif

	return valid;
}

#else

/* FIXME: Apparently for other operating systems, the appropriate 
 * equivalents are:
 *
 * FreeBSD               /proc/curproc/map
 * All BSDs:             mincore()
 * MacOS X:              vm_region()
 * Irix, OSF/1, Solaris: ioctl(PIOCMAP)
 *
 * These will be implemented as time permits.
 */

MX_EXPORT int
mx_is_valid_pointer( void *pointer, size_t length, int access_mode )
{
	if ( pointer == NULL ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

#endif


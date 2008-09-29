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

/*-------------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------------*/

#elif defined(OS_MACOSX)

#include <mach/mach.h>

MX_EXPORT int
mx_is_valid_pointer( void *pointer, size_t length, int access_mode )
{
	static const char fname[] = "mx_is_valid_pointer()";

	task_t task;
	vm_address_t region_address;
	vm_size_t region_size;
	struct vm_region_basic_info region_info;
	mach_msg_type_number_t region_info_size;
	mach_port_t object_name;
	kern_return_t kreturn;
	int valid;

	kreturn = task_for_pid( current_task(), mx_process_id(), &task );

	if ( kreturn != KERN_SUCCESS ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"A call to task_for_pid() failed.  kreturn = %ld",
			(long) kreturn );

		return FALSE;
	}

	region_address = (vm_address_t) pointer;

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: region_address = %lu",
		fname, (unsigned long) region_address));
#endif

	region_info_size = VM_REGION_BASIC_INFO_COUNT;

	kreturn = vm_region( task,
			&region_address,
			&region_size,
			VM_REGION_BASIC_INFO,
			(vm_region_info_t) &region_info,
			&region_info_size,
			&object_name );

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: vm_region() kreturn = %lu",
		fname, (unsigned long) kreturn));
#endif

	if ( kreturn == KERN_INVALID_ADDRESS ) {
		return FALSE;
	} else
	if ( kreturn == KERN_INVALID_ARGUMENT ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "One or more of the arguments passed to vm_region() was invalid." );
	} else
	if ( kreturn != KERN_SUCCESS ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"A call to vm_region() failed.  kreturn = %ld",
			(long) kreturn );

		return FALSE;
	}

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: region_info.offset = %ld",
		fname, (long) region_info.offset));
	MX_DEBUG(-2,("%s: region_info.protection = %ld",
		fname, (long) region_info.protection));
	MX_DEBUG(-2,("%s: region_info.inheritance = %ld",
		fname, (long) region_info.inheritance));
	MX_DEBUG(-2,("%s: region_info.max_protection = %ld",
		fname, (long) region_info.max_protection));
	MX_DEBUG(-2,("%s: region_info.behavior = %ld",
		fname, (long) region_info.behavior));
	MX_DEBUG(-2,("%s: region_info.user_wired_count = %ld",
		fname, (long) region_info.user_wired_count));
	MX_DEBUG(-2,("%s: region_info.reserved = %ld",
		fname, (long) region_info.reserved));
	MX_DEBUG(-2,("%s: region_info.shared = %ld",
		fname, (long) region_info.shared));
#endif

	valid = FALSE;

	if ( (access_mode & R_OK) && ( access_mode & W_OK ) ) {
		if ( (region_info.protection & VM_PROT_READ)
		  && (region_info.protection & VM_PROT_WRITE) )
		{
			valid = TRUE;
		} else {
			valid = FALSE;
		}
	} else
	if ( access_mode & R_OK ) {
		if ( region_info.protection & VM_PROT_READ ) {
			valid = TRUE;
		} else {
			valid = FALSE;
		}
	} else
	if ( access_mode & W_OK ) {
		if ( region_info.protection & VM_PROT_WRITE ) {
			valid = TRUE;
		} else {
			valid = FALSE;
		}
	} else {
		valid = FALSE;
	}

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: valid = %d", fname, valid));
#endif

	return valid;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_SOLARIS)

#include <fcntl.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/procfs.h>

MX_EXPORT int
mx_is_valid_pointer( void *pointer, size_t length, int access_mode )
{
	static const char fname[] = "mx_is_valid_pointer()";

	FILE *proc_file;
	int saved_errno;
	prmap_t map_entry;
	size_t items_read;
	char *pointer_addr;
	unsigned long object_offset, object_end, flags;

	pointer_addr = pointer;

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: pointer_addr = %#lx", fname, pointer_addr));
#endif

	/* Open the virtual address map for the current process. */

	proc_file = fopen( "/proc/self/map", "r" );

	if ( proc_file == NULL ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to open '/proc/self/map' failed with errno = %d, "
		"error message = '%s'", saved_errno, strerror(saved_errno) );

		return FALSE;
	}

	/* Walk through the array of virtual address map entries.
	 *
	 * FIXME: We _assume_ the mappings are in increasing order
	 * although we haven't yet found a document that explicitly
	 * states that.  It works though.
	 */

	while (1) {
		items_read = fread(&map_entry, sizeof(map_entry), 1, proc_file);

		if ( items_read < 1 ) {

			if ( feof(proc_file) ) {
#if MX_POINTER_DEBUG
				MX_DEBUG(-2,("%s: Reached the end of the "
				"virtual address map array.", fname));
#endif
				fclose( proc_file );
				return FALSE;
			}

			(void) mx_error( MXE_FILE_IO_ERROR, fname,
			"An unknown error occurred while reading "
			"from /proc/self/map." );

			fclose( proc_file );
			return FALSE;
		}

		/* Skip mappings that are of zero length. */

		if ( map_entry.pr_size == 0 )
			continue;

#if MX_POINTER_DEBUG
		MX_DEBUG(-2,("%s: %#lx, %#lx, %#x", fname,
			(unsigned long) map_entry.pr_vaddr,
			(unsigned long) map_entry.pr_size,
			(unsigned int) map_entry.pr_mflags ));
#endif

		if ( pointer_addr < map_entry.pr_vaddr ) {
			/* The pointer is located before this address
			 * mapping, but was not found by a previous
			 * pass through the loop.  This means that
			 * either the pointer is before the first
			 * mapping or is inbetween mappings.  Either
			 * way the pointer is invalid.
			 */
#if MX_POINTER_DEBUG
			MX_DEBUG(-2,("%s: Invalid pointer %p", fname, pointer));
#endif
			fclose( proc_file );
			return FALSE;
		}

		object_offset = pointer_addr - map_entry.pr_vaddr;

		object_end = object_offset + length;

		if ( object_end < map_entry.pr_size ) {

			/* We have found the correct map entry. */

			fclose( proc_file );
#if MX_POINTER_DEBUG
			MX_DEBUG(-2,("%s: map entry found!", fname));
			MX_DEBUG(-2,
			("%s: object_offset = %#lx, object_end = %#lx",
				fname, object_offset, object_end));
#endif
			/* Does the requested access mode match? */

			flags = 0;

			if ( access_mode & R_OK ) {
				flags |= MA_READ;
			}
			if ( access_mode & W_OK ) {
				flags |= MA_WRITE;
			}
			if ( access_mode & X_OK ) {
				flags |= MA_EXEC;
			}

			if ( (map_entry.pr_mflags & flags) == flags ) {
#if MX_POINTER_DEBUG
				MX_DEBUG(-2,
			    ("%s: The requested access is allowed.",fname));
#endif
				return TRUE;
			} else {
#if MX_POINTER_DEBUG
				MX_DEBUG(-2,
			    ("%s: The requested access is NOT allowed.",fname));
#endif
				return FALSE;
			}
		}
	}
}

/*-------------------------------------------------------------------------*/

#elif 0 /* FIXME: Unfinished PIOCMAP implementation. */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/procfs.h>

MX_EXPORT int
mx_is_valid_pointer( void *pointer, size_t length, int access_mode )
{
	static const char fname[] = "mx_is_valid_pointer()";

	prmap_t *prmap_array, *array_element;
	int i, num_mappings;
	int proc_fd, saved_errno, os_status;
	void *ioctl_ptr;
	char proc_filename[40];

	/* Open the /proc file for the current process. */

	snprintf( proc_filename, sizeof(proc_filename),
		"/proc/%d", mx_process_id() );

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: About to open '%s' for read-only access.",
		fname, proc_filename));
#endif

	proc_fd = open( proc_filename, O_RDONLY );

	if ( proc_fd == (-1) ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to open '%s' failed with errno = %d, "
		"error message = '%s'", proc_filename,
			saved_errno, strerror(saved_errno) );

		return FALSE;
	}

	/* Find out how many memory mappings there are for the current
	 * process.
	 */

	os_status = ioctl( proc_fd, PIOCNMAP, (void *) &num_mappings );

	if ( os_status < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The PIOCNMAP ioctl() on file '%s' failed.  "
		"Errno = %d, error message = '%s'",
			proc_filename, saved_errno, strerror(saved_errno) );

		close( proc_fd );
		return FALSE;
	}

#if MX_POINTER_DEBUG
	/* FIXME: It may actually be unsafe to make this MX_DEBUG()
	 * call here.  What if the call to MX_DEBUG() increases the
	 * number of memory mappings?
	 */

	MX_DEBUG(-2,("%s: number of memory mappings = %d",
		fname, num_mappings));
#endif

	/* Allocate memory for an array of memory mappings. */

	prmap_array = (prmap_t *) malloc( num_mappings * sizeof(prmap_t) );

	if ( prmap_array == NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element "
		"array of prmap_t structures.", num_mappings );

		close( proc_fd );
		return FALSE;
	}

	/* Fill in the array of memory mappings.
	 *
	 * WARNING: If somehow the number of memory mappings is changed
	 * out from under us, then the upcoming call to PIOCMAP may fail
	 * with a segmentation fault or overwrite memory.  EEK!
	 *
	 * Since the process in question is the current process, I hope
	 * that it is obvious why we do not attempt to stop the process
	 * for tracing.
	 */

	os_status = ioctl( proc_fd, PIOCMAP, (void *) prmap_array );

	if ( os_status < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to fill in the prmap_array using the "
		"PIOCMAP ioctl() on file '%s' failed.  "
		"Errno = %d, error message = '%s'",
			proc_filename, saved_errno, strerror(saved_errno) );

		free( prmap_array );
		close( proc_fd );
		return FALSE;
	}

	/* If we get here, then we do not need the file descriptor anymore. */

	close( proc_fd );

	/* Walk through the array of prmap_t structures. */

	for ( i = 0; i < num_mappings; i++ ) {
		array_element = &prmap_array[i];
	}

#if MX_POINTER_DEBUG
#endif
}

/*-------------------------------------------------------------------------*/

#elif 0

/* FIXME: Apparently for other operating systems, the appropriate 
 * equivalents are:
 *
 * FreeBSD               /proc/curproc/map
 * All BSDs:             mincore()
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

#else

#error mx_is_valid_pointer() has not yet been defined for this build target.

#endif


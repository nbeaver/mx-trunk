/*
 * Name:    mx_vm_alloc.c
 *
 * Purpose: MX functions that allocate virtual memory directly from the
 *          underlying memory manager.  Read, write, and execute protection
 *          can be set for this memory.  Specifying a requested address of
 *          NULL lets the operating system select the actual address.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_VM_ALLOC_DEBUG	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_unistd.h"
#include "mx_vm_alloc.h"

/*=========================== Microsoft Windows ===========================*/

#if defined(OS_WIN32)

#include <windows.h>

MX_EXPORT void *
mx_vm_alloc( void *requested_address,
		size_t requested_region_size_in_bytes,
		unsigned long protection_flags )
{
	static const char fname[] = "mx_vm_alloc()";

	DWORD vm_allocation_type;
	DWORD vm_protection_flags;
	void *actual_address;

	DWORD last_error_code;
	TCHAR message_buffer[100];

	vm_allocation_type = MEM_COMMIT | MEM_RESERVE;

	/* Write permission implies read permission on Win32. */

	vm_protection_flags = 0;

	if ( protection_flags & X_OK ) {
		if ( protection_flags & W_OK ) {
			vm_protection_flags = PAGE_EXECUTE_READWRITE;
		} else
		if ( protection_flags & R_OK ) {
			vm_protection_flags = PAGE_EXECUTE_READ;
		} else {
			vm_protection_flags = PAGE_EXECUTE;
		}
	} else
	if ( protection_flags & W_OK ) {
		vm_protection_flags = PAGE_READWRITE;
	} else
	if ( protection_flags & R_OK ) {
		vm_protection_flags = PAGE_READONLY;
	} else {
		vm_protection_flags = PAGE_NOACCESS;
	}
	
	actual_address = VirtualAlloc( requested_address,
					requested_region_size_in_bytes,
					vm_allocation_type,
					vm_protection_flags );

	if ( actual_address == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"VirtualAlloc() failed with "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	return actual_address;
}

MX_EXPORT void
mx_vm_free( void *address )
{
	static const char fname[] = "mx_vm_free()";

	BOOL virtual_free_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	virtual_free_status = VirtualFree( address, 0, MEM_RELEASE );

	if ( virtual_free_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"VirtualFree() failed with "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	return;
}

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	MEMORY_BASIC_INFORMATION memory_info;
	SIZE_T bytes_returned;
	DWORD vm_protection_flags;

	DWORD last_error_code;
	TCHAR message_buffer[100];

	unsigned long protection_flags_value;

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );
	}
	if ( protection_flags == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The protection_flags pointer passed was NULL." );
	}

	bytes_returned = VirtualQuery( address,
					&memory_info,
					sizeof(memory_info) );

	if ( bytes_returned == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"VirtualQuery() failed with "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	vm_protection_flags = memory_info.Protect;

	switch( vm_protection_flags ) {
	case PAGE_EXECUTE_READWRITE:
		protection_flags_value = ( R_OK | W_OK | X_OK );
		break;
	case PAGE_EXECUTE_READ:
		protection_flags_value = ( R_OK | X_OK );
		break;
	case PAGE_EXECUTE:
		protection_flags_value = X_OK;
		break;
	case PAGE_READWRITE:
		protection_flags_value = ( R_OK | W_OK );
		break;
	case PAGE_READONLY:
		protection_flags_value = R_OK;
		break;
	case PAGE_NOACCESS:
	case 0:
		protection_flags_value = 0;
		break;
	default:
		protection_flags_value = 0;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The access protection value %#lx returned by "
		"VirtualQuery for address %p is not recognized.",
			(unsigned long) vm_protection_flags,
			address );
		break;
	}

	if ( valid_address_range != NULL ) {
		*valid_address_range = TRUE;
	}

	if ( protection_flags != NULL ) {
		*protection_flags = protection_flags_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_vm_set_protection( void *address,
		size_t region_size_in_bytes,
		unsigned long protection_flags )
{
	static const char fname[] = "mx_vm_set_protection()";

	BOOL virtual_protect_status;
	DWORD vm_protection_flags, old_vm_protection_flags;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );
	}

	/* Write permission implies read permission on Win32. */

	vm_protection_flags = 0;

	if ( protection_flags & X_OK ) {
		if ( protection_flags & W_OK ) {
			vm_protection_flags = PAGE_EXECUTE_READWRITE;
		} else
		if ( protection_flags & R_OK ) {
			vm_protection_flags = PAGE_EXECUTE_READ;
		} else {
			vm_protection_flags = PAGE_EXECUTE;
		}
	} else
	if ( protection_flags & W_OK ) {
		vm_protection_flags = PAGE_READWRITE;
	} else
	if ( protection_flags & R_OK ) {
		vm_protection_flags = PAGE_READONLY;
	} else {
		vm_protection_flags = PAGE_NOACCESS;
	}

	virtual_protect_status = VirtualProtect( address,
						region_size_in_bytes,
						vm_protection_flags,
						&old_vm_protection_flags );
	
	if ( virtual_protect_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"VirtualProtect() failed with "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*================================= Posix =================================*/

/*
 * Posix platforms provide a standard way of implementing these functions:
 *
 *   mx_vm_alloc() -------------> mmap()
 *   mx_vm_free() --------------> munmap()
 *   mx_vm_set_protection() ----> mprotect()
 *
 * However, there is no standard way of implementing mx_vm_get_protection().
 * Thus, mx_vm_get_protection() is implemented in platform-specific code
 * later in this section of the file.
 */

#elif defined(OS_LINUX) | defined(OS_MACOSX) | defined(OS_BSD) \
	| defined(OS_SOLARIS)

#include <errno.h>
#include <sys/mman.h>

#if !defined( MAP_ANONYMOUS )
#  define MAP_ANONYMOUS    MAP_ANON
#endif

MX_EXPORT void *
mx_vm_alloc( void *requested_address,
		size_t requested_region_size_in_bytes,
		unsigned long protection_flags )
{
	static const char fname[] = "mx_vm_alloc()";

	void *actual_address;
	int vm_protection_flags, vm_visibility_flags;
	int saved_errno;

	vm_protection_flags = 0;

	if ( protection_flags & X_OK ) {
		vm_protection_flags |= PROT_EXEC;
	}
	if ( protection_flags & R_OK ) {
		vm_protection_flags |= PROT_READ;
	}
	if ( protection_flags & W_OK ) {
		vm_protection_flags |= PROT_WRITE;
	}

	if ( vm_protection_flags == 0 ) {
		vm_protection_flags = PROT_NONE;
	}

	vm_visibility_flags = MAP_PRIVATE | MAP_ANONYMOUS;

	actual_address = mmap( requested_address,
				requested_region_size_in_bytes,
				vm_protection_flags,
				vm_visibility_flags,
				-1, 0 );

	if ( actual_address == MAP_FAILED ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"mmap() failed with errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );

		return NULL;
	}

	return actual_address;
}

MX_EXPORT void
mx_vm_free( void *address )
{
	static const char fname[] = "mx_vm_free()";

	int munmap_status, saved_errno;

	if ( address == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );

		return;
	}

	munmap_status = munmap( address, 0 );

	if ( munmap_status == (-1) ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"munmap() failed with errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );

		return;
	}

	return;
}

MX_EXPORT mx_status_type
mx_vm_set_protection( void *address,
		size_t region_size_in_bytes,
		unsigned long protection_flags )
{
	static const char fname[] = "mx_vm_set_protection()";

	int vm_protection_flags;
	int mprotect_status, saved_errno;

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );
	}

	vm_protection_flags = 0;

	if ( protection_flags & X_OK ) {
		vm_protection_flags |= PROT_EXEC;
	}
	if ( protection_flags & R_OK ) {
		vm_protection_flags |= PROT_READ;
	}
	if ( protection_flags & W_OK ) {
		vm_protection_flags |= PROT_WRITE;
	}

	if ( vm_protection_flags == 0 ) {
		vm_protection_flags = PROT_NONE;
	}

	mprotect_status = mprotect( address,
				region_size_in_bytes,
				vm_protection_flags );

	if ( mprotect_status == (-1) ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"mprotect() failed with errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----- Platform-specific mx_vm_get_protection() for Posix platforms ------*/

#  if defined(OS_LINUX)

#  include <sys/mman.h>

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	FILE *file;
	char buffer[300];
	int i, argc, saved_errno;
	char **argv;
	unsigned long start_address, end_address, pointer_address;
	char permissions[20];

	mx_bool_type valid;
	unsigned long protection_flags_value;

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );
	}

#if MX_VM_ALLOC_DEBUG
	MX_DEBUG(-2,("%s invoked for address %p, size %lu",
		fname, address, (unsigned long) region_size_in_bytes ));
#endif

	/* Under Linux, the preferred technique is to read the
	 * necessary information from /proc/self/maps.
	 */

	file = fopen( "/proc/self/maps", "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to open '/proc/self/maps' failed with "
		"errno = %d, error message = '%s'.",
			saved_errno, strerror( saved_errno ) );
	}

	/* If we get here, then opening /proc/self/maps succeeded. */

	protection_flags_value = 0;

	pointer_address = (unsigned long) address;

	for ( i = 0; ; i++ ) {

		valid = FALSE;

		mx_fgets( buffer, sizeof(buffer), file );

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

#if MX_VM_ALLOC_DEBUG
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

#if MX_VM_ALLOC_DEBUG
		MX_DEBUG(-2,("%s: address = %p, pointer_address = %#lx, "
			"i = %d, start_address = %#lx, "
			"end_address = %#lx, permissions = '%s'", fname,
			address, pointer_address, i,
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
#if MX_VM_ALLOC_DEBUG
			MX_DEBUG(-2,("%s: pointer is in this block.",fname));
#endif
			valid = TRUE;

			if ( permissions[0] == 'r' ) {
				protection_flags_value |= R_OK;
			}
			if ( permissions[1] == 'w' ) {
				protection_flags_value |= W_OK;
			}
			if ( permissions[2] == 'x' ) {
				protection_flags_value |= X_OK;
			}

			break;		/* Exit the for() loop. */
		}
	}

#if MX_VM_ALLOC_DEBUG
	MX_DEBUG(-2,("%s: valid = %d", fname, valid));
#endif

	fclose(file);

	if ( valid_address_range != NULL ) {
		*valid_address_range = valid;
	}

	if ( protection_flags != NULL ) {
		*protection_flags = protection_flags_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#  elif defined(OS_MACOSX)

#include <mach/mach.h>

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	task_t task;
	vm_address_t region_address;
	vm_size_t region_size;
	struct vm_region_basic_info region_info;
	mach_msg_type_number_t region_info_size;
	mach_port_t object_name;
	kern_return_t kreturn;

	mx_bool_type valid = FALSE;
	unsigned long protection_flags_value = 0;

	kreturn = task_for_pid( current_task(), mx_process_id(), &task );

	if ( kreturn != KERN_SUCCESS ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"A call to task_for_pid() failed.  kreturn = %ld",
			(long) kreturn );
	}

	region_address = (vm_address_t) address;
	region_size    = (vm_size_t) region_size_in_bytes;

#if MX_VM_ALLOC_DEBUG
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

#if MX_VM_ALLOC_DEBUG
	MX_DEBUG(-2,("%s: vm_region() kreturn = %lu",
		fname, (unsigned long) kreturn));
#endif

	if ( kreturn == KERN_INVALID_ADDRESS ) {
		valid = FALSE;
	} else
	if ( kreturn == KERN_INVALID_ARGUMENT ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "One or more of the arguments passed to vm_region() was invalid." );
	} else
	if ( kreturn != KERN_SUCCESS ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"A call to vm_region() failed.  kreturn = %ld",
			(long) kreturn );
	} else {

#if MX_VM_ALLOC_DEBUG
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
		valid = TRUE;

		protection_flags_value = 0;

		if ( region_info.protection & VM_PROT_READ ) {
			protection_flags_value |= R_OK;
		}
		if ( region_info.protection & VM_PROT_WRITE ) {
			protection_flags_value |= W_OK;
		}
		if ( region_info.protection & VM_PROT_EXECUTE ) {
			protection_flags_value |= X_OK;
		}
	}

#if MX_VM_ALLOC_DEBUG
	MX_DEBUG(-2,("%s: valid = %d", fname, valid));
#endif

	if ( valid_address_range != NULL ) {
		*valid_address_range = valid;
	}

	if ( protection_flags != NULL ) {
		if ( valid ) {
			*protection_flags = protection_flags_value;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#  elif defined(OS_BSD)

/* On some BSD based operating systems, mincore() can be used to determine
 * whether or not an address range is valid, since mincore() will return
 * ENOMEM if the address range is not fully mapped into virtual memory.
 *
 * Some discussion about this can be found at
 * http://unix.derkeiler.com/Mailing-Lists/FreeBSD/hackers/2006-06/msg00106.html
 *
 * See http://www.istild.com/man-pages/man2/mincore.2.html for a discussion
 * of how to compute the necessary length of the vector.
 */

#include <sys/mman.h>

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	char *vector;
	char *aligned_pointer, *original_pointer;
	unsigned long original_pointer_as_ulong;
	unsigned long page_size, vector_size;
	unsigned long num_pages, page_offset;
	unsigned long modified_length;
	int os_status, saved_errno;
	mx_bool_type pointer_is_valid;

	/* aligned_pointer must be aligned to the start of a memory page. */

	page_size = sysconf(_SC_PAGESIZE);

	if ( page_size == 0 ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The operating system said that the value of _SC_PAGESIZE "
		"was 0." );
	}

	original_pointer = address;

	/* Converting a pointer to an integer is icky, but necessary here. */

	original_pointer_as_ulong = (unsigned long) original_pointer;

	num_pages = original_pointer_as_ulong / page_size;

	page_offset = original_pointer_as_ulong % page_size;

	/* Converting the integer back to a pointer makes us doubly icky. */

	aligned_pointer = (char *) (num_pages * page_size );

	modified_length = region_size_in_bytes + page_offset;

	/* We now must allocate memory for the vector to be returned
	 * by mincore().
	 */

	vector_size = ( modified_length + page_size - 1 ) / page_size;

	vector = malloc( vector_size );

	if ( vector == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu byte vector "
		"for mincore().", vector_size );
	}

	os_status = mincore( aligned_pointer, modified_length, vector );

	saved_errno = errno;

	mx_free(vector);

	if ( os_status == 0 ) {
		pointer_is_valid = TRUE;
	} else {
		if ( saved_errno == ENOMEM ) {
			pointer_is_valid = FALSE;
		} else {
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"mincore() returned with an unexpected error code.  "
			"Errno = %d, error message = '%s'.",
				saved_errno, strerror(saved_errno) );
		}
	}

	if ( valid_address_range != NULL ) {
		*valid_address_range = pointer_is_valid;
	}

	/* FIXME! We do not yet know how to find out on BSD the permissions
	 *        for a given region of virtual memory.
	 */

	if ( protection_flags != NULL ) {
		*protection_flags = R_OK | W_OK | X_OK;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#  elif defined(OS_SOLARIS)

#include <fcntl.h>
#include <procfs.h>

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	FILE *proc_file;
	int saved_errno;
	prmap_t map_entry;
	size_t items_read;
	unsigned long pointer_addr;
	unsigned long object_offset, object_end;
	unsigned long protection_flags_value;

	pointer_addr = (unsigned long) address;

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: pointer_addr = %#lx", fname, pointer_addr));
#endif

	/* Open the virtual address map for the current process. */

	proc_file = fopen( "/proc/self/map", "r" );

	if ( proc_file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to open '/proc/self/map' failed with errno = %d, "
		"error message = '%s'", saved_errno, strerror(saved_errno) );
	}

	/* Walk through the array of virtual address map entries. */

	while (1) {
		items_read = fread(&map_entry, sizeof(map_entry), 1, proc_file);

		if ( items_read < 1 ) {

			if ( feof(proc_file) ) {
#if MX_POINTER_DEBUG
				MX_DEBUG(-2,("%s: Reached the end of the "
				"virtual address map array.", fname));
#endif
				fclose( proc_file );

				if ( valid_address_range != NULL ) {
					*valid_address_range = FALSE;
				}

				return MX_SUCCESSFUL_RESULT;
			}

			fclose( proc_file );

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An unknown error occurred while reading "
			"from /proc/self/map." );
		}

		/* Skip mappings that are of zero length. */

		if ( map_entry.pr_size == 0 )
			continue;

#if MX_POINTER_DEBUG
		MX_DEBUG(-2,("%s: %#lx, %#lx, %#x, '%s'", fname,
			(unsigned long) map_entry.pr_vaddr,
			(unsigned long) map_entry.pr_size,
			map_entry.pr_mflags,
			map_entry.pr_mapname));
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

			if ( valid_address_range != NULL ) {
				*valid_address_range = FALSE;
			}

			return MX_SUCCESSFUL_RESULT;
		}

		object_offset = pointer_addr - map_entry.pr_vaddr;

		object_end = object_offset + region_size_in_bytes;

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

			protection_flags_value = 0;

			if ( map_entry.pr_mflags & MA_READ ) {
				protection_flags_value |= R_OK;
			}
			if ( map_entry.pr_mflags & MA_WRITE ) {
				protection_flags_value |= W_OK;
			}
			if ( map_entry.pr_mflags & MA_EXEC ) {
				protection_flags_value |= X_OK;
			}

			if ( valid_address_range != NULL ) {
				*valid_address_range = TRUE;
			}
			if ( protection_flags != NULL ) {
				*protection_flags = protection_flags_value;
			}

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

/*-------------------------------------------------------------------------*/
#  else
#  error mx_vm_get_protection() is not yet implemented for this Posix platform.
#  endif

/*=========================================================================*/

#else
#error Virtual memory functions are not yet implemented for this platform.
#endif


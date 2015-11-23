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
 * Copyright 2013-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_VM_ALLOC_DEBUG	FALSE

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

/* The following implementation of mx_vm_get_protection() is inspired by
 * the following web page:
 *
 *     http://blogs.msdn.com/ericlippert/articles/105186.aspx
 *
 * VirtualQuery() is used since it is safe to call at any time.  For
 * the purposes of mx_pointer_is_valid() an alternative would be to use
 * the IsBadxxxPtr() family of functions.  However, the IsBadxxxPtr()
 * functions are very unsafe and should _never_ be used, since they can
 * have painful side effects like disabling automatic stack expansion.
 * It is far safer to use VirtualQuery().  Unfortunately, VirtualQuery()
 * is very _slow_.
 */

#ifndef SIZE_T
#define SIZE_T  DWORD
#endif

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t mx_region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	MEMORY_BASIC_INFORMATION memory_info;
	SIZE_T bytes_returned;
	DWORD vm_protection_flags;

	DWORD last_error_code;
	TCHAR message_buffer[100];

	unsigned long pointer_offset, unused_region_size;
	unsigned long protection_flags_value;

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );
	}
	if ( protection_flags == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The protection_flags pointer passed was NULL." );
	}

	memset( &memory_info, 0, sizeof(memory_info) );

	bytes_returned = VirtualQuery( address,
					&memory_info,
					sizeof(memory_info) );

	if ( bytes_returned == 0 ) {

		/* Please note that VirtualQuery() may return 0 if
		 * the pointer is a kernel-mode pointer.
		 */

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"VirtualQuery() failed with "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	/* If the memory has not yet been committed, then we do not
	 * have access to it.
	 */

	if ( memory_info.State != MEM_COMMIT ) {

		if ( protection_flags != NULL ) {
			*protection_flags = 0;
		}
		return MX_SUCCESSFUL_RESULT;
	}

	/* If the memory range from address to address+mx_region_size_in_bytes
	 * does not all fit into this memory region, then we return with
	 * all MX permission bits set to 0.
	 */

	/* pointer_offset contains the offset of the pointer relative
	 * to the start of the memory region it is in.
	 */

	pointer_offset = (unsigned long)
		( ((char *) address) - ((char *) memory_info.BaseAddress) );

	/* unused_region_size is the amount of the memory region that is
	 * not used by the memory range specified in the call to this
	 * routine.
	 */

	unused_region_size = (unsigned long)
			( memory_info.RegionSize - mx_region_size_in_bytes );

	/* If pointer_offset is bigger than unused_region_size, then the
	 * end of the requested memory range is beyond the end of the
	 * current Win32 memory region.
	 */

	if ( pointer_offset > unused_region_size ) {

		if ( protection_flags != NULL ) {
			*protection_flags = 0;
		}
		return MX_SUCCESSFUL_RESULT;
	}

	/* Use the Win32 permission bits to construct the MX permission bits. */

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

/* For definitions of the hex bits below, look in the Microsoft include
 * file "WinNT.h" and look for the set of definitions that start with
 * the macro PAGE_NOACCESS.
 */

MX_EXPORT mx_status_type
mx_vm_show_os_info( FILE *file,
		void *address,
		size_t region_size_in_bytes )
{
	static const char fname[] = "mx_vm_show_os_info()";

	MEMORY_BASIC_INFORMATION memory_info;
	SIZE_T bytes_returned;

	if ( file == (FILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The FILE pointer passed was NULL." );
	}
	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The address pointer passed was NULL." );
	}

	fprintf( file, "    Address =           %p\n", address );

	memset( &memory_info, 0, sizeof(memory_info) );

	bytes_returned = VirtualQuery( address,
					&memory_info,
					sizeof(memory_info) );

	fprintf( file, "    BaseAddress =       %p\n",
						memory_info.BaseAddress );
	fprintf( file, "    AllocationBase =    %p\n",
						memory_info.AllocationBase );
	fprintf( file, "    AllocationProtect = %#lx\n",
						memory_info.AllocationProtect );
	fprintf( file, "    RegionSize =        %lu\n",
						memory_info.RegionSize );
	fprintf( file, "    State =             %#lx\n", memory_info.State );
	fprintf( file, "    Protect =           %#lx\n", memory_info.Protect );
	fprintf( file, "    Type =              %#lx\n", memory_info.Type );

	return MX_SUCCESSFUL_RESULT;
}

/*================================= Posix =================================*/

/*
 * Posix-like platforms provide a standard way of implementing these functions:
 *
 *   mx_vm_alloc() -------------> mmap()
 *   mx_vm_free() --------------> munmap()
 *   mx_vm_set_protection() ----> mprotect()
 *
 * Even some non-Posix platforms like OpenVMS implement these functions.
 *
 * However, there is no standard way of implementing mx_vm_get_protection().
 * Thus, mx_vm_get_protection() is implemented in platform-specific code
 * later in this section of the file.
 */

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_BSD) \
	|| defined(OS_SOLARIS) || defined(OS_UNIXWARE) || defined(OS_CYGWIN) \
	|| defined(OS_VMS) || defined(OS_HURD) || defined(OS_ANDROID)

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

	if ( actual_address == (void *) MAP_FAILED ) {
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

#  if defined(OS_LINUX) || defined(OS_CYGWIN) || defined(OS_ANDROID)

#  include <sys/mman.h>

static mx_status_type
mx_vm_get_protection_entry( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags,
		char *entry_buffer,
		size_t entry_buffer_size )
{
	static const char fname[] = "mx_vm_get_protection_entry()";

	FILE *file;
	char buffer[300];
	int i, argc, saved_errno;
	char **argv;
	char *buffer_copy;
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

	if ( entry_buffer != NULL ) {
		*entry_buffer = '\0';
	}

	/* Under Linux and Cygwin, the preferred technique is to read the
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

		buffer_copy = strdup( buffer );

		if ( buffer_copy == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"The attempt to duplicate a /proc/self/maps "
			"entry failed." );
		}

		mx_string_split( buffer_copy, " -", &argc, &argv );

		start_address = mx_hex_string_to_unsigned_long( argv[0] );
		end_address   = mx_hex_string_to_unsigned_long( argv[1] );

		strlcpy( permissions, argv[2], sizeof(permissions) );

		mx_free(argv);
		mx_free(buffer_copy);

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

	if ( entry_buffer != NULL ) {
		if ( valid ) {
			strlcpy( entry_buffer, buffer, entry_buffer_size );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	mx_status_type mx_status;

	mx_status = mx_vm_get_protection_entry( address,
					region_size_in_bytes,
					valid_address_range,
					protection_flags,
					NULL, 0 );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_vm_show_os_info( FILE *file,
		void *address,
		size_t region_size_in_bytes )
{
	static const char fname[] = "mx_vm_show_os_info()";

	char entry_buffer[250];
	mx_bool_type valid_address_range;
	char *address_start, *address_end;
	int argc, dup_argc;
	char **argv, **dup_argv;
	char *dup_argv0;
	char *dup_entry_buffer;
	intptr_t vm_base_address, vm_end_address, vm_region_size;
	mx_status_type mx_status;

	mx_status = mx_vm_get_protection_entry( address,
					region_size_in_bytes,
					&valid_address_range,
					NULL,
					entry_buffer,
					sizeof(entry_buffer) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	address_start = (char *) address;
	address_end = address_start + ( region_size_in_bytes - 1 );

	if ( valid_address_range == FALSE ) {
		fprintf( file, "Address range %p to %p is invalid.\n",
			address_start, address_end );

		return MX_SUCCESSFUL_RESULT;
	}

#if 0
	MX_DEBUG(-2,("%s: entry_buffer = '%s'", fname, entry_buffer));
#endif

	dup_entry_buffer = strdup( entry_buffer );

	mx_string_split( dup_entry_buffer, " ", &argc, &argv );

	if ( argc <= 4 ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Invalid entry returned from /proc/self/maps.  Entry = '%s'",
			entry_buffer );
	}

	dup_argv0 = strdup( argv[0] );

	mx_string_split( dup_argv0, "-", &dup_argc, &dup_argv );

	vm_base_address = mx_hex_string_to_unsigned_long( dup_argv[0] );
	vm_end_address  = mx_hex_string_to_unsigned_long( dup_argv[1] );

	mx_free( dup_argv );
	mx_free( dup_argv0 );

	vm_region_size = vm_end_address - vm_base_address;

	fprintf( stderr, "    Address      = %p\n", address );
	fprintf( stderr, "    Base Address = %p\n", (void *) vm_base_address );
	fprintf( stderr, "    Region_Size  = %lu\n",
					(unsigned long) vm_region_size );
	fprintf( stderr, "    Permissions  = %s\n", argv[1] );
	fprintf( stderr, "    Offset       = %lu\n",
				mx_hex_string_to_unsigned_long( argv[2] ) );
	fprintf( stderr, "    Device       = '%s'\n", argv[3] );
	fprintf( stderr, "    Inode        = %lu\n", atol( argv[4] ) );

	if ( argc == 5 ) {
		fprintf( stderr, " <No pathname>\n" );
	} else {
		fprintf( stderr, "    Pathname     = '%s'\n", argv[5] );
	}

	mx_free( dup_entry_buffer );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#  elif defined(OS_MACOSX)

#include <mach/mach.h>

#if ( MX_DARWIN_VERSION >= 9000000L)
#  include <mach/mach_vm.h>

#  define vm_region	mach_vm_region
#  define vm_address_t	mach_vm_address_t
#  define vm_size_t	mach_vm_size_t
#endif

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

MX_EXPORT mx_status_type
mx_vm_show_os_info( FILE *file,
		void *address,
		size_t region_size_in_bytes )
{
	static const char fname[] = "mx_vm_show_os_info()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"mx_vm_show_os_info() is not yet implemented on MacOS X." );
}

/*-------------------------------------------------------------------------*/

#  elif defined(OS_BSD) || defined(OS_SOLARIS) || defined(OS_UNIXWARE)

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

MX_EXPORT mx_status_type
mx_vm_show_os_info( FILE *file,
		void *address,
		size_t region_size_in_bytes )
{
	static const char fname[] = "mx_vm_show_os_info()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"mx_vm_show_os_info() is not yet implemented on this platform." );
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

#elif defined(OS_IRIX)

#include <fcntl.h>
#include <sys/procfs.h>

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	prmap_t *prmap_array = NULL;
	prmap_t *array_element;
	int i, num_mappings, num_mappings_to_allocate;
	unsigned long pointer_addr, pr_vaddr;
	unsigned long object_offset, object_end, flags;
	int proc_fd, saved_errno, os_status;
	char proc_filename[40];

	unsigned long protection_flags_value;

	pointer_addr = (unsigned long) address;

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,
	("%s: pointer_addr = %#lx, region_size_in_bytes = %lu",
		fname, pointer_addr, (unsigned long) region_size_in_bytes));
#endif

	/* Open the /proc file for the current process. */

	snprintf( proc_filename, sizeof(proc_filename),
		"/proc/%lu", mx_process_id() );

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: About to open '%s' for read-only access.",
		fname, proc_filename));
#endif

	proc_fd = open( proc_filename, O_RDONLY );

	if ( proc_fd == (-1) ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to open '%s' failed with errno = %d, "
		"error message = '%s'", proc_filename,
			saved_errno, strerror(saved_errno) );
	}

	/* Find out how many memory mappings there are for the current
	 * process.
	 */

	os_status = ioctl( proc_fd, PIOCNMAP, (void *) &num_mappings );

	if ( os_status < 0 ) {
		saved_errno = errno;

		close( proc_fd );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The PIOCNMAP ioctl() on file '%s' failed.  "
		"Errno = %d, error message = '%s'",
			proc_filename, saved_errno, strerror(saved_errno) );
	}

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: number of memory mappings = %d",
		fname, num_mappings));
#endif

	/* Allocate memory for an array of memory mappings.
	 * We deliberately allocate memory for a lot more
	 * mappings than that reported by PIOCNMAP just
	 * in case the number of mappings has increased.
	 */

	num_mappings_to_allocate = 2 * num_mappings + 10;

	prmap_array = (prmap_t *) calloc( num_mappings_to_allocate,
						sizeof(prmap_t) );

	if ( prmap_array == NULL ) {
		close( proc_fd );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element "
		"array of prmap_t structures.", num_mappings );
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

		mx_free( prmap_array );
		close( proc_fd );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to fill in the prmap_array using the "
		"PIOCMAP ioctl() on file '%s' failed.  "
		"Errno = %d, error message = '%s'",
			proc_filename, saved_errno, strerror(saved_errno) );
	}

	/* If we get here, then we do not need the file descriptor anymore. */

	close( proc_fd );

	/* Walk through the array of virtual address map entries. */

	for ( i = 0; i < num_mappings_to_allocate; i++ ) {
		array_element = &prmap_array[i];

		pr_vaddr = (unsigned long) array_element->pr_vaddr;

		if ( pr_vaddr == 0 ) {
			/* We have reached the end of the array. */
			break;
		}

#if MX_POINTER_DEBUG
		MX_DEBUG(-2,("%s: %#lx %#lx %#x", fname, pr_vaddr,
			(unsigned long) array_element->pr_size,
			(unsigned int) array_element->pr_mflags ));
#endif

		if ( pointer_addr < pr_vaddr ) {
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
			mx_free( prmap_array );

			if ( valid_address_range != NULL ) {
				*valid_address_range = FALSE;
			}

			return MX_SUCCESSFUL_RESULT;
		}

		object_offset = pointer_addr - pr_vaddr;

		object_end = object_offset + region_size_in_bytes;

		if ( object_end < array_element->pr_size ) {

			/* We have found the correct map entry. */
#if MX_POINTER_DEBUG
			MX_DEBUG(-2,("%s: map entry found!", fname));
			MX_DEBUG(-2,
			("%s: object_offset = %#lx, object_end = %#lx",
				fname, object_offset, object_end));
#endif

			protection_flags_value = 0;

			if ( array_element->pr_mflags & MA_READ ) {
				protection_flags_value |= R_OK;
			}
			if ( array_element->pr_mflags & MA_WRITE ) {
				protection_flags_value |= W_OK;
			}
			if ( array_element->pr_mflags & MA_EXEC ) {
				protection_flags_value |= X_OK;
			}

			mx_free( prmap_array );

			return MX_SUCCESSFUL_RESULT;
		}
	}

#if MX_POINTER_DEBUG
	MX_DEBUG(-2,("%s: No entry found in array.", fname));
#endif
	mx_free( prmap_array );

	if ( valid_address_range != NULL ) {
		*valid_address_range = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------- VMS ----------------------------------*/

#  elif defined(OS_VMS)

#    if defined(__VAX)

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"This function is not yet implemented on VAX-based OpenVMS systems." );
}

#    else /* Not VAX */

#  include <sys_starlet_c/psldef.h>

/* FIXME: I should not have to make these definitions, but including
 * <builtins.h> does not do the trick for some reason.
 */

int __PAL_PROBER( const void *__base_address, int __length, char __mode );
int __PAL_PROBEW( const void *__base_address, int __length, char __mode );

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	mx_bool_type read_allowed, write_allowed;
	unsigned long protection_flags_value;

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The address pointer passed is NULL." );
	}

	read_allowed = __PAL_PROBER(address, region_size_in_bytes, PSL$C_USER);

	write_allowed = __PAL_PROBEW(address, region_size_in_bytes, PSL$C_USER);

	if ( valid_address_range != NULL ) {
		if ( read_allowed || write_allowed ) {
			*valid_address_range = TRUE;
		} else {
			*valid_address_range = FALSE;
		}
	}

	protection_flags_value = 0;

	if ( read_allowed ) {
		protection_flags_value |= R_OK;
	}
	if ( write_allowed ) {
		protection_flags_value |= W_OK;
	}

	if ( protection_flags != NULL ) {
		*protection_flags = protection_flags_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

#    endif /* Not VAX */

MX_EXPORT mx_status_type
mx_vm_show_os_info( FILE *file,
		void *address,
		size_t region_size_in_bytes )
{
	static const char fname[] = "mx_vm_show_os_info()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"mx_vm_show_os_info() is not yet implemented on OpenVMS." );
}

/*-------------------------------------------------------------------------*/

#  elif defined(OS_HURD)

#  define MX_VM_GET_PROTECTION_USES_STUB

MX_EXPORT mx_status_type
mx_vm_show_os_info( FILE *file,
		void *address,
		size_t region_size_in_bytes )
{
	static const char fname[] = "mx_vm_show_os_info()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"mx_vm_show_os_info() is not yet implemented on this platform." );
}

/*-------------------------------------------------------------------------*/
#  else
#  error mx_vm_get_protection() is not yet implemented for this Posix platform.
#  endif

/*===================== Platforms that only use stubs =====================*/

/* FIXME: Some of the following platforms may provide a way to figure these
 * things out (especially the RTOSes).  But, if so I haven't found them.
 */

#elif defined(OS_DJGPP) || defined(OS_VXWORKS) || defined(OS_QNX) \
	|| defined(OS_RTEMS)

#  define MX_VM_ALLOC_USES_MALLOC
#  define MX_VM_ALLOC_USES_FREE
#  define MX_VM_SET_PROTECTION_USES_STUB
#  define MX_VM_GET_PROTECTION_USES_STUB
#  define MX_VM_SHOW_OS_INFO_USES_STUB

/*=========================================================================*/

#else
#error Virtual memory functions are not yet implemented for this platform.
#endif

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* This section contains dummy functions for platforms that do not
 * support the given functionality.
 */

#if defined(MX_VM_ALLOC_USES_MALLOC)

MX_EXPORT void *
mx_vm_alloc( void *requested_address,
		size_t requested_region_size_in_bytes,
		unsigned long protection_flags )
{
	void *ptr;

	ptr = malloc( requested_region_size_in_bytes );

	return ptr;
}

#endif /* MX_VM_ALLOC_USES_MALLOC */

/*.......................................................................*/

#if defined(MX_VM_FREE_USES_FREE)

MX_EXPORT void
mx_vm_free( void *address )
{
	mx_free( address );

	return;
}

#endif /* MX_VM_FREE_USES_FREE */

/*.......................................................................*/

#if defined(MX_VM_SET_PROTECTION_USES_STUB)

MX_EXPORT mx_status_type
mx_vm_set_protection( void *address,
		size_t region_size_in_bytes,
		unsigned long protection_flags )
{
	return MX_SUCCESSFUL_RESULT;
}

#endif /* MX_VM_SET_PROTECTION_USES_STUB */

/*.......................................................................*/

#if defined(MX_VM_GET_PROTECTION_USES_STUB)

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The address pointer passed is NULL." );
	}

	if ( valid_address_range != NULL ) {
		if ( address == NULL ) {
			*valid_address_range = FALSE;
		} else {
			*valid_address_range = TRUE;
		}
	}

	if ( protection_flags != NULL ) {
		if ( address == NULL ) {
			*protection_flags = 0;
		} else {
			*protection_flags = R_OK | W_OK;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* MX_VM_GET_PROTECTION_USES_STUB */

/*.......................................................................*/

#if defined(MX_VM_SHOW_OS_INFO_USES_STUB)

MX_EXPORT mx_status_type
mx_vm_show_os_info( FILE *file,
		void *address,
		size_t region_size_in_bytes )
{
	return MX_SUCCESSFUL_RESULT;
}

#endif /* MX_VM_SHOW_OS_INFO_USES_STUB */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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

		if ( ( protection_flags & access_mode ) == access_mode ) {

			/* All of the bits requested in 'access_mode'
			 * are set in 'protection_flags'.
			 */

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


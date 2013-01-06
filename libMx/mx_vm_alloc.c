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

#include <stdio.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_vm_alloc.h"

/*************************** Microsoft Windows ***************************/

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
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	MEMORY_BASIC_INFORMATION memory_info;
	SIZE_T bytes_returned;
	DWORD vm_protection_flags;

	DWORD last_error_code;
	TCHAR message_buffer[100];

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
		*protection_flags = ( R_OK | W_OK | X_OK );
		break;
	case PAGE_EXECUTE_READ:
		*protection_flags = ( R_OK | X_OK );
		break;
	case PAGE_EXECUTE:
		*protection_flags = X_OK;
		break;
	case PAGE_READWRITE:
		*protection_flags = ( R_OK | W_OK );
		break;
	case PAGE_READONLY:
		*protection_flags = R_OK;
		break;
	case PAGE_NOACCESS:
	case 0:
		*protection_flags = 0;
		break;
	default:
		*protection_flags = 0;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The access protection value %#lx returned by "
		"VirtualQuery for address %p is not recognized.",
			(unsigned long) vm_protection_flags,
			address );
		break;
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

/*************************** Posix ***************************/

#elif defined(OS_LINUX)

#include <errno.h>
#include <sys/mman.h>

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
mx_vm_get_protection( void *address,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );
	}
	if ( protection_flags == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The protection_flags pointer passed is NULL." );
	}

	/* FIXME - On Linux, this probably must be done by reading
	 *         and parsing /proc/self/maps.
	 */

	*protection_flags = 0;

	return MX_SUCCESSFUL_RESULT;
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

#else
#error Virtual memory allocation functions are not yet implemented for this platform.
#endif


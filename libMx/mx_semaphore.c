/*
 * Name:    mx_semaphore.c
 *
 * Purpose: MX semaphore functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SEMAPHORE_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_unistd.h"
#include "mx_thread.h"
#include "mx_semaphore.h"

/************************ Microsoft Win32 ***********************/

#if defined(OS_WIN32)

#include <windows.h>

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
			long initial_value,
			char *name )
{
	static const char fname[] = "mx_semaphore_create()";

	HANDLE *semaphore_handle_ptr;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	size_t name_length;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( semaphore == (MX_SEMAPHORE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	*semaphore = (MX_SEMAPHORE *) malloc( sizeof(MX_SEMAPHORE) );

	if ( *semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_SEMAPHORE structure." );
	}

	semaphore_handle_ptr = (HANDLE *) malloc( sizeof(HANDLE) );

	if ( semaphore_handle_ptr == (HANDLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a HANDLE pointer." );
	}
	
	(*semaphore)->private = semaphore_handle_ptr;

	(*semaphore)->semaphore_type = MXT_SEM_WIN32;

	/* Allocate space for a name if one was specified. */

	if ( name == NULL ) {
		(*semaphore)->name = NULL;
	} else {
		name_length = strlen( name );

		name_length++;

		(*semaphore)->name = (char *)
					malloc( name_length * sizeof(char) );

		strlcpy( (*semaphore)->name, name, name_length );
	}

	if ( initial_value < 0 ) {

		/* Connect to an existing semaphore. */

		if ( (*semaphore)->name == NULL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot connect to an existing semaphore if the "
			"semaphore name is NULL." );
		}

		*semaphore_handle_ptr = OpenSemaphore( SEMAPHORE_ALL_ACCESS,
							FALSE,
							(*semaphore)->name );
	} else {
		/* Create a new semaphore. */

		*semaphore_handle_ptr = CreateSemaphore( NULL,
						initial_value,
						initial_value,
						(*semaphore)->name );
	}

	if ( *semaphore_handle_ptr == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to create or open a semaphore.  "
			"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	HANDLE *semaphore_handle_ptr;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	semaphore_handle_ptr = semaphore->private;

	if ( semaphore_handle_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	status = CloseHandle( *semaphore_handle_ptr );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to close the Win32 handle for semaphore %p. "
			"Win32 error code = %ld, error_message = '%s'",
			semaphore, last_error_code, message_buffer );
	}

	if ( semaphore->name != NULL ) {
		mx_free( semaphore->name );
	}

	mx_free( semaphore_handle_ptr );

	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_lock()";

	HANDLE *semaphore_handle_ptr;
	BOOL status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_handle_ptr = semaphore->private;

	if ( semaphore_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = WaitForSingleObject( *semaphore_handle_ptr, INFINITE );

	switch( status ) {
	case WAIT_ABANDONED:
		return MXE_OBJECT_ABANDONED;
		break;
	case WAIT_OBJECT_0:
		return MXE_SUCCESS;
		break;
	case WAIT_TIMEOUT:
		return MXE_TIMED_OUT;
		break;
	case WAIT_FAILED:
		{
			DWORD last_error_code;
			TCHAR message_buffer[100];

			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unexpected error from WaitForSingleObject() "
				"for semaphore %p. "
				"Win32 error code = %ld, error_message = '%s'",
				semaphore, last_error_code, message_buffer );

			return MXE_OPERATING_SYSTEM_ERROR;
		}
		break;
	}

	return MXE_UNKNOWN_ERROR;
}

MX_EXPORT long
mx_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_unlock()";

	HANDLE *semaphore_handle_ptr;
	BOOL status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_handle_ptr = semaphore->private;

	if ( semaphore_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = ReleaseSemaphore( *semaphore_handle_ptr, 1, NULL );

	if ( status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[100];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error from ReleaseSemaphore() "
			"for semaphore %p. "
			"Win32 error code = %ld, error_message = '%s'",
			semaphore, last_error_code, message_buffer );

		return MXE_OPERATING_SYSTEM_ERROR;
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_trylock()";

	HANDLE *semaphore_handle_ptr;
	BOOL status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_handle_ptr = semaphore->private;

	if ( semaphore_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = WaitForSingleObject( *semaphore_handle_ptr, 0 );

	switch( status ) {
	case WAIT_ABANDONED:
		return MXE_OBJECT_ABANDONED;
		break;
	case WAIT_OBJECT_0:
		return MXE_SUCCESS;
		break;
	case WAIT_TIMEOUT:
		return MXE_NOT_AVAILABLE;
		break;
	case WAIT_FAILED:
		{
			DWORD last_error_code;
			TCHAR message_buffer[100];

			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unexpected error from WaitForSingleObject() "
				"for semaphore %p. "
				"Win32 error code = %ld, error_message = '%s'",
				semaphore, last_error_code, message_buffer );

			return MXE_OPERATING_SYSTEM_ERROR;
		}
		break;
	}

	return MXE_UNKNOWN_ERROR;
}

/* The current value of a Win32 semaphore is only available for the NT series
 * via the undocumented NtQuerySemaphore() function.
 */

static int ntdll_is_available = TRUE;

static HINSTANCE hinst_ntdll = NULL;

typedef long NTSTATUS;

typedef NTSTATUS ( __stdcall *NtQuerySemaphore_type)( HANDLE,
					UINT, PVOID, ULONG, PULONG );

static NtQuerySemaphore_type ptrNtQuerySemaphore = NULL;

MX_EXPORT mx_status_type
mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	struct {
		ULONG current_value;
		ULONG maximum_value;
	} win32_semaphore_info;

	HANDLE *semaphore_handle_ptr;
	UINT semaphore_info_class;
	ULONG return_length;
	NTSTATUS status;

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( current_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The current_value pointer passed was NULL." );
	}

	semaphore_handle_ptr = semaphore->private;

	if ( semaphore_handle_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The semaphore handle pointer for semaphore %p is NULL.",
			semaphore );
	}

	/* If NTDLL.DLL is not available, all we can do is return 0. */

	if ( ntdll_is_available == FALSE ) {
		*current_value = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If the pointer to NtQuerySemaphore is NULL, then we have not
	 * yet explicitly loaded NTDLL.DLL.  If so, then we must load it.
	 */

	if ( ptrNtQuerySemaphore == NULL ) {

		if ( hinst_ntdll != NULL ) {

			ntdll_is_available = FALSE;

			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Somehow the instance of NTDLL.DLL is not NULL, "
			"but the pointer to NtQuerySemaphore() "
			"_is_ NULL.  This should not be able to happen." );
		}

		hinst_ntdll = LoadLibrary(TEXT("ntdll.dll"));

		if ( hinst_ntdll == NULL ) {
			mx_warning(
			"NTDLL.DLL is not available on this computer, "
			"so it will not be possible to query the value "
			"of a Win32 semaphore." );

			ntdll_is_available = FALSE;

			return MX_SUCCESSFUL_RESULT;
		}

		ptrNtQuerySemaphore = (NtQuerySemaphore_type)
			GetProcAddress( hinst_ntdll,
					TEXT("NtQuerySemaphore") );

		if ( ptrNtQuerySemaphore == NULL ) {
			mx_warning(
			"NtQuerySemaphore() was not found in NTDLL.DLL." );

			/* I have no idea if it is safe to do FreeLibrary()
			 * on NTDLL.DLL, so it is safest not to try.
			 */

			ntdll_is_available = FALSE;

			return MX_SUCCESSFUL_RESULT;
		}
	}

	semaphore_info_class = 0;

	status = (ptrNtQuerySemaphore)( *semaphore_handle_ptr,
					semaphore_info_class,
					&win32_semaphore_info,
					sizeof(win32_semaphore_info),
					&return_length );

	if ( status < 0 ) {
		*current_value = -1;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to invoke NtQuerySemaphore() failed." );
	}

	*current_value = win32_semaphore_info.current_value;

	return MX_SUCCESSFUL_RESULT;
}

/************************ OpenVMS ***********************/

#elif defined(OS_VMS)

#include <starlet.h>
#include <descrip.h>
#include <ssdef.h>
#include <lckdef.h>
#include <builtins.h>

typedef struct {
	uint32_t lock_status_block[4];
	struct dsc$descriptor_s name_descriptor;
} MX_VMS_SEMAPHORE_PRIVATE;

static volatile uint32_t mx_lock_number = 0;

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
			long initial_value,
			char *name )
{
	static const char fname[] = "mx_semaphore_create()";

	MX_VMS_SEMAPHORE_PRIVATE *vms_private;
	char local_name_buffer[80];
	size_t name_length;
	unsigned long new_lock_number;
	int vms_status, condition_value;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( semaphore == (MX_SEMAPHORE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	if ( initial_value < 0 ) {
		if ( name == NULL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot connect to an existing semaphore if the "
			"semaphore name is NULL." );
		} else {
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Connecting to an existing semaphore is "
			"not yet implemented." );
		}
	}

	*semaphore = (MX_SEMAPHORE *) malloc( sizeof(MX_SEMAPHORE) );

	if ( *semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_SEMAPHORE structure." );
	}

	vms_private = (MX_VMS_SEMAPHORE_PRIVATE *)
				malloc( sizeof(MX_VMS_SEMAPHORE_PRIVATE) );

	if ( vms_private == (MX_VMS_SEMAPHORE_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for an MX_VMS_SEMAPHORE_PRIVATE pointer." );
	}
	
	(*semaphore)->private = vms_private;

	(*semaphore)->semaphore_type = MXT_SEM_VMS;

	/* If no name was specified, specify an internal private name. */

	if ( name == NULL ) {
		/* FIXME: The VAX version of the code is not atomic, so
		 *        it is unsafe.
		 */
#if defined(__VAX)
		new_lock_number = (mx_lock_number)++;
#else
		new_lock_number = __ATOMIC_INCREMENT_LONG( &mx_lock_number );
#endif

		snprintf( local_name_buffer, sizeof(local_name_buffer),
		"MX_SEMAPHORE_%lu", new_lock_number );

		name_length = strlen( local_name_buffer );
	} else {
		name_length = strlen( name );
	}

	/* Allocate space for the name and copy the name there. */

	(*semaphore)->name = (char *) malloc( (name_length+1) * sizeof(char) );

	if ( name == NULL ) {
		strlcpy( (*semaphore)->name, local_name_buffer, name_length );
	} else {
		strlcpy( (*semaphore)->name, name, name_length );
	}

	/* Get ready to create the lock with sys$enqw(). */

	vms_private->name_descriptor.dsc$w_length = name_length;
	vms_private->name_descriptor.dsc$a_pointer = (*semaphore)->name;
	vms_private->name_descriptor.dsc$b_class = DSC$K_CLASS_S;
	vms_private->name_descriptor.dsc$b_dtype = DSC$K_DTYPE_T;

	memset( vms_private->lock_status_block, 0, 4 * sizeof(uint32_t) );

	/* Create the lock resource, but do not lock it. */

	vms_status = sys$enqw( 0, LCK$K_NLMODE,
				vms_private->lock_status_block, 0,
				&(vms_private->name_descriptor),
				0, 0, 0, 0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to create semaphore '%s' failed with a "
		"VMS error code of %d.", (*semaphore)->name, vms_status );
	}

	condition_value = (int)((vms_private->lock_status_block[0]) & 0xffff);

	if ( condition_value != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to create semaphore '%s' failed with a "
		"VMS lock status code of %d.",
			(*semaphore)->name, condition_value );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	MX_VMS_SEMAPHORE_PRIVATE *vms_private;
	uint32_t lock_id;
	int vms_status;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	vms_private = semaphore->private;

	if ( vms_private == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The private field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	lock_id = vms_private->lock_status_block[1];

	vms_status = sys$deq( lock_id, vms_private->lock_status_block, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to destroy semaphore '%s' failed with a "
		"VMS error code of %d.", semaphore->name, vms_status );
	}

	mx_free( semaphore->name );

	mx_free( semaphore->private );

	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	MX_VMS_SEMAPHORE_PRIVATE *vms_private;
	int vms_status, condition_value;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	vms_private = semaphore->private;

	if ( vms_private == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	vms_status = sys$enqw( 0, LCK$K_EXMODE,
				vms_private->lock_status_block, 0,
				&(vms_private->name_descriptor),
				0, 0, 0, 0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL )
		return MXE_OPERATING_SYSTEM_ERROR;

	condition_value = (int)((vms_private->lock_status_block[0]) & 0xffff);

	if ( condition_value != SS$_NORMAL )
		return MXE_OPERATING_SYSTEM_ERROR;

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	MX_VMS_SEMAPHORE_PRIVATE *vms_private;
	int vms_status, condition_value;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	vms_private = semaphore->private;

	if ( vms_private == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	vms_status = sys$enqw( 0, LCK$K_NLMODE,
				vms_private->lock_status_block, 0,
				&(vms_private->name_descriptor),
				0, 0, 0, 0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL )
		return MXE_OPERATING_SYSTEM_ERROR;

	condition_value = (int)((vms_private->lock_status_block[0]) & 0xffff);

	if ( condition_value != SS$_NORMAL )
		return MXE_OPERATING_SYSTEM_ERROR;

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	MX_VMS_SEMAPHORE_PRIVATE *vms_private;
	int vms_status, condition_value;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	vms_private = semaphore->private;

	if ( vms_private == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	vms_status = sys$enqw( 0, LCK$K_EXMODE,
				vms_private->lock_status_block, 0,
				&(vms_private->name_descriptor),
				0, 0, 0, 0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL )
		return MXE_OPERATING_SYSTEM_ERROR;

	condition_value = (int)((vms_private->lock_status_block[0]) & 0xffff);

	if ( condition_value != SS$_NORMAL )
		return MXE_OPERATING_SYSTEM_ERROR;

	return MXE_SUCCESS;
}

MX_EXPORT mx_status_type
mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( current_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The current_value pointer passed was NULL." );
	}

	/* I do not know how to do this on VMS. */

	*current_value = 0;

	return MX_SUCCESSFUL_RESULT;
}

/************************ VxWorks ***********************/

#elif defined(OS_VXWORKS)

#include "taskLib.h"	/* For 'private/semLibP.h'. */
#include "semLib.h"
#include "intLib.h"

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
			long initial_value,
			char *name )
{
	static const char fname[] = "mx_semaphore_create()";

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	if ( name != (char *) NULL ) {
		mx_warning( "VxWorks semaphores do not have names, "
			"so the requested name '%s' was ignored.",
			name );
	}

	/* Allocate the data structures we need. */

	*semaphore = (MX_SEMAPHORE *) malloc( sizeof(MX_SEMAPHORE) );

	if ( *semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_SEMAPHORE structure." );
	}

	/* Create the semaphore. */

	(*semaphore)->private = semCCreate( SEM_Q_FIFO, (int) initial_value );

	if ( (*semaphore)->private == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Insufficient memory exists to initialize the semaphore." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	SEM_ID semaphore_id;
	STATUS status;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	semaphore_id = semaphore->private;

	if ( semaphore_id == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The private field for the MX_SEMAPHORE pointer passed was NULL.");
	}

	status = semDelete( semaphore_id );

	if ( status == ERROR ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The semaphore passed to semDelete() was invalid." );
	}

	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	SEM_ID semaphore_id;
	int status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = semaphore->private;

	if ( semaphore_id == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = semTake( semaphore_id, WAIT_FOREVER );

	if ( status != OK ) {
		switch( errno ) {
		case S_objLib_OBJ_TIMEOUT:
			return MXE_TIMED_OUT;
			break;
		case S_intLib_NOT_ISR_CALLABLE:
			return MXE_NOT_VALID_FOR_CURRENT_STATE;
			break;
		case S_objLib_OBJ_ID_ERROR:
		case S_objLib_OBJ_UNAVAILABLE:
		default:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		}
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	SEM_ID semaphore_id;
	int status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = semaphore->private;

	if ( semaphore_id == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = semGive( semaphore_id );

	if ( status != OK ) {
		switch( errno ) {
		case S_intLib_NOT_ISR_CALLABLE:
			return MXE_NOT_VALID_FOR_CURRENT_STATE;
			break;
		case S_objLib_OBJ_ID_ERROR:
		case S_semLib_INVALID_OPERATION:
		default:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		}
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	SEM_ID semaphore_id;
	int status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = semaphore->private;

	if ( semaphore_id == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = semTake( semaphore_id, WAIT_FOREVER );

	if ( status != OK ) {
		switch( errno ) {
		case S_objLib_OBJ_TIMEOUT:
			return MXE_NOT_AVAILABLE;
			break;
		case S_intLib_NOT_ISR_CALLABLE:
			return MXE_NOT_VALID_FOR_CURRENT_STATE;
			break;
		case S_objLib_OBJ_ID_ERROR:
		case S_objLib_OBJ_UNAVAILABLE:
		default:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		}
	}

	return MXE_SUCCESS;
}

MX_EXPORT mx_status_type
mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	SEM_ID semaphore_id;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( current_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The current_value pointer passed was NULL." );
	}

	semaphore_id = semaphore->private;

	if ( semaphore_id == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The private field for the MX_SEMAPHORE pointer passed was NULL.");
	}

	/* Can only get the value from a data structure defined in
	 * the private include file 'include/semLibP.h'.
	 */

	*current_value = (unsigned long) semaphore_id->recurse;

	return MX_SUCCESSFUL_RESULT;
}

/************************ RTEMS ***********************/

#elif defined(OS_RTEMS)

#include "rtems.h"

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
			long initial_value,
			char *name )
{
	static const char fname[] = "mx_semaphore_create()";

	rtems_id *semaphore_id;
	rtems_name semaphore_name;
	rtems_unsigned32 initial_state;
	rtems_status_code rtems_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	/* Allocate the data structures we need. */

	*semaphore = (MX_SEMAPHORE *) malloc( sizeof(MX_SEMAPHORE) );

	if ( *semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_SEMAPHORE structure." );
	}

	semaphore_id = (rtems_id *) malloc( sizeof(rtems_id) );

	if ( semaphore_id == (rtems_id *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an rtems_id structure." );
	}

	/* Create the semaphore. */

	if ( name == (char *) NULL ) {
		semaphore_name = rtems_build_name('M','X','S','E');
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Semaphores with shared names are not supported under RTEMS." );
	}

	initial_state = (rtems_unsigned32) initial_value;

	rtems_status = rtems_semaphore_create(
	    semaphore_name, initial_state,
	    RTEMS_FIFO | RTEMS_COUNTING_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY \
	    	| RTEMS_NO_PRIORITY_CEILING | RTEMS_LOCAL, 0, semaphore_id );

	switch( rtems_status ) {
	case RTEMS_SUCCESSFUL:
		break;
	case RTEMS_INVALID_NAME:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The RTEMS name specified for rtems_semaphore_create() "
			"was invalid." );
		break;
	case RTEMS_INVALID_ADDRESS:
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The rtems_id pointer passed to "
			"rtems_semaphore_create() was NULL." );
		break;
	case RTEMS_TOO_MANY:
		return mx_error( MXE_TRY_AGAIN, fname,
			"Too many RTEMS semaphores or global objects "
			"are in use." );
		break;
	case RTEMS_NOT_DEFINED:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"One or more of the attributes specified for the call "
			"to rtems_semaphore_create() were invalid." );
		break;
	case RTEMS_INVALID_NUMBER:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Invalid starting count %lu specified for the call "
			"to rtems_semaphore_create().",
				(unsigned long) initial_state );
		break;
	case RTEMS_MP_NOT_CONFIGURED:
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
			"Multiprocessing has not been configured for this "
			"copy of RTEMS." );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"An unexpected status code %lu was returned by the "
			"call to rtems_semaphore_create().",
				(unsigned long) rtems_status );
		break;
	}

	(*semaphore)->private = semaphore_id;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	rtems_id *semaphore_id;
	rtems_status_code rtems_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	semaphore_id = semaphore->private;

	if ( semaphore_id == (rtems_id *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The private field for the MX_SEMAPHORE pointer passed was NULL.");
	}

	rtems_status = rtems_semaphore_delete( *semaphore_id );

	switch( rtems_status ) {
	case RTEMS_SUCCESSFUL:
		break;
	case RTEMS_INVALID_ID:
		return mx_error( MXE_NOT_FOUND, fname,
			"The semaphore id %#lx supplied to "
			"rtems_semaphore_delete() was not found.",
			(unsigned long) (*semaphore_id) );
		break;
	case RTEMS_ILLEGAL_ON_REMOTE_OBJECT:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Cannot delete remote semaphore id %#lx.",
			(unsigned long) (*semaphore_id) );
		break;
	case RTEMS_RESOURCE_IN_USE:
		return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"Semaphore id %#lx is currently in use.",
			(unsigned long) (*semaphore_id) );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"An unexpected status code %lu was returned by the "
			"call to rtems_semaphore_delete().",
				(unsigned long) rtems_status );
		break;
	}

	mx_free( semaphore->private );
	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	rtems_id *semaphore_id;
	rtems_status_code rtems_status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = semaphore->private;

	if ( semaphore_id == (rtems_id *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	rtems_status = rtems_semaphore_obtain( *semaphore_id,
					RTEMS_WAIT, RTEMS_NO_TIMEOUT );

	switch( rtems_status ) {
	case RTEMS_SUCCESSFUL:
		break;
	case RTEMS_UNSATISFIED:
	case RTEMS_TIMEOUT:
		return MXE_OPERATING_SYSTEM_ERROR;
		break;
	case RTEMS_OBJECT_WAS_DELETED:
		return MXE_BAD_HANDLE;
		break;
	case RTEMS_INVALID_ID:
		return MXE_NOT_FOUND;
		break;
	default:
		return MXE_UNKNOWN_ERROR;
		break;
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	rtems_id *semaphore_id;
	rtems_status_code rtems_status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = semaphore->private;

	if ( semaphore_id == (rtems_id *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	rtems_status = rtems_semaphore_release( *semaphore_id );

	switch( rtems_status ) {
	case RTEMS_SUCCESSFUL:
		break;
	case RTEMS_INVALID_ID:
		return MXE_NOT_FOUND;
		break;
	case RTEMS_NOT_OWNER_OF_RESOURCE:
		return MXE_PERMISSION_DENIED;
		break;
	default:
		return MXE_UNKNOWN_ERROR;
		break;
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	rtems_id *semaphore_id;
	rtems_status_code rtems_status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = semaphore->private;

	if ( semaphore_id == (rtems_id *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	rtems_status = rtems_semaphore_obtain( *semaphore_id,
					RTEMS_NO_WAIT, 0 );

	switch( rtems_status ) {
	case RTEMS_SUCCESSFUL:
		break;
	case RTEMS_UNSATISFIED:
		return MXE_NOT_AVAILABLE;
		break;
	case RTEMS_TIMEOUT:
		return MXE_OPERATING_SYSTEM_ERROR;
		break;
	case RTEMS_OBJECT_WAS_DELETED:
		return MXE_BAD_HANDLE;
		break;
	case RTEMS_INVALID_ID:
		return MXE_NOT_FOUND;
		break;
	default:
		return MXE_UNKNOWN_ERROR;
		break;
	}

	return MXE_SUCCESS;
}

MX_EXPORT mx_status_type
mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	rtems_id *semaphore_id;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( current_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The current_value pointer passed was NULL." );
	}

	semaphore_id = semaphore->private;

	if ( semaphore_id == (rtems_id *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The private field for the MX_SEMAPHORE pointer passed was NULL.");
	}

	/* There does not seem to be an obvious way to do this under RTEMS. */

	*current_value = 0;

	return mx_error( MXE_UNSUPPORTED, fname,
		"RTEMS does not support getting the current value "
		"of a counting semaphore." );
}

/*********************** Unix and Posix systems **********************/

#elif defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_ECOS)

/* NOTE:  On some platforms, such as Linux and MacOS X, it is necessary
 * to include support for both System V and Posix semaphores.  The reason
 * is that on those platforms, one kind of semaphore only supports named
 * semaphores, while the other kind only supports unnamed semaphores.
 * So you need both.  The choice of which kind of semaphore to use is
 * made using the variables called mx_use_posix_unnamed_semaphores and 
 * mx_use_posix_named_semaphores as defined below.
 *
 * We use static variables below rather than #defines since that keeps
 * GCC from complaining about 'defined but not used' functions.
 */

#if defined(OS_LINUX) || defined(OS_SOLARIS)

static int mx_use_posix_unnamed_semaphores = TRUE;
static int mx_use_posix_named_semaphores   = FALSE;

#elif defined(OS_MACOSX)

/* FIXME: MacOS X might be better off with System V named semaphores since
 *        sem_getvalue() does not work for Posix named semaphores there.
 */

static int mx_use_posix_unnamed_semaphores = FALSE;
static int mx_use_posix_named_semaphores   = TRUE;

#elif defined(OS_IRIX) || defined(OS_QNX) || defined(OS_ECOS)

static int mx_use_posix_unnamed_semaphores = TRUE;
static int mx_use_posix_named_semaphores   = TRUE;

#else

static int mx_use_posix_unnamed_semaphores = FALSE;
static int mx_use_posix_named_semaphores   = FALSE;

#endif

/*======================= System V Semaphores ======================*/

#if !defined(OS_ECOS)

#if defined(OS_HPUX) && defined(__ia64) && defined(__GNUC__)
   typedef int32_t cid_t;
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

#if defined(_POSIX_THREADS)
#  include <pthread.h>
#endif

#if 1
#  define SEMVMX  32767
#endif

#if !defined(SEM_A)
#define SEM_A  0200	/* Alter permission */
#define SEM_R  0400	/* Read permission  */
#endif

#if _SEM_SEMUN_UNDEFINED || defined(OS_SOLARIS) || defined(OS_CYGWIN) \
	|| defined(OS_HPUX) || defined(OS_TRU64) || defined(OS_QNX) \
	|| defined(__NetBSD_Version__)

/* I wonder what possible advantage there is to making this
 * union definition optional?
 */

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
};
#endif

typedef struct {
	key_t semaphore_key;
	int semaphore_id;

	uid_t creator_process;
#if defined(_POSIX_THREADS)
	pthread_t creator_thread;
#endif
} MX_SYSTEM_V_SEMAPHORE_PRIVATE;

#if defined(OS_QNX)

/* Although they are defined in the header files, semctl(), semget(),
 * and semop() are not really available in QNX, so we include stubs here.
 */

int semctl( int semid, int semnum, int cmd, ... )
{
	return EINVAL;
}
int semop( int semid, struct sembuf *sops, unsigned nsops )
{
	return EINVAL;
}
int semget( key_t key, int nsems, int semflg )
{
	return EINVAL;
}

#endif /* QNX */

static mx_status_type
mx_sysv_named_semaphore_exclusive_create_file( MX_SEMAPHORE *semaphore,
						FILE **new_semaphore_file )
{
	static const char fname[] =
		"mx_sysv_named_semaphore_exclusive_create_file()";

	int fd, saved_errno;

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( new_semaphore_file == (FILE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The FILE pointer passed was NULL." );
	}

	*new_semaphore_file = NULL;

	fd = open( semaphore->name, O_RDWR | O_CREAT | O_EXCL, 0600 );

	if ( fd < 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EEXIST:
			return mx_error( MXE_ALREADY_EXISTS, fname,
			"Semaphore file '%s' already exists.",
				semaphore->name );

		case EACCES:
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The filesystem permissions do not allow us to "
			"write to semaphore file '%s'.",
				semaphore->name );

		default:
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An attempt to create semaphore file '%s' failed.  "
			"Errno = %d, error message = '%s'.",
				semaphore->name, saved_errno,
				strerror( saved_errno ) );
		}
	}

	/* Convert the file descriptor to a file pointer. */

	(*new_semaphore_file) = fdopen( fd, "w" );

	if ( (*new_semaphore_file) == NULL ) {
		saved_errno = errno;

		(void) close(fd);

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to convert file descriptor %d for semaphore file '%s' "
		"to a file pointer.", fd, semaphore->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_sysv_named_semaphore_get_server_process_id( MX_SEMAPHORE *semaphore,
					unsigned long *process_id )
{
	static const char fname[] =
			"mx_sysv_named_semaphore_get_server_process_id()";

	FILE *semaphore_file;
	int num_items, saved_errno;
	char buffer[80];

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( process_id == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The process_id pointer passed was NULL." );
	}

	semaphore_file = fopen( semaphore->name, "r" );

	if ( semaphore_file == (FILE *) NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to open semaphore file '%s' for reading.  "
			"Errno = %d, error message = '%s'.",
				semaphore->name, saved_errno,
				strerror( saved_errno ) );
	}

	fgets( buffer, sizeof(buffer), semaphore_file );

	if ( feof(semaphore_file) || ferror(semaphore_file) ) {
		(void) fclose(semaphore_file);

		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unable to read semaphore id string from semaphore file '%s'.",
			semaphore->name );
	}

	(void) fclose(semaphore_file);

	num_items = sscanf( buffer, "%lu", process_id );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unable to find the server process id in the line '%s' "
		"read from semaphore file '%s'.", buffer, semaphore->name);
	}

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: process_id = %lu", fname, *process_id));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_sysv_named_semaphore_create_new_file( MX_SEMAPHORE *semaphore,
					FILE **new_semaphore_file )
{
	static const char fname[] = "mx_sysv_named_semaphore_create_new_file()";

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	int file_status, saved_errno, process_exists;
	unsigned long process_id;
	mx_status_type mx_status;

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( new_semaphore_file == (FILE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The FILE pointer passed was NULL." );
	}

	*new_semaphore_file = NULL;

	system_v_private = semaphore->private;

	if ( system_v_private == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	/* The specified semaphore name is expected to be a filename
	 * in the file system.  If the named file does not already
	 * exist, then we assume that we are creating a new semaphore.
	 * If it does exist, then we assume that we are connecting
	 * to an existing semaphore.
	 */

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: semaphore->name = '%s'.", fname, semaphore->name));
#endif

	/* Attempt to create the semaphore file. */

	mx_status = mx_sysv_named_semaphore_exclusive_create_file( semaphore,
							new_semaphore_file );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_ALREADY_EXISTS:
		/* The semaphore file already exists.  Does it belong to
		 * a currently running server?
		 */

		process_exists = FALSE;

		mx_status = mx_sysv_named_semaphore_get_server_process_id(
					semaphore, &process_id );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			/* Check to see if the process corresponding to
			 * that process id is still running.
			 */

			process_exists = mx_process_exists( process_id );
			break;
		case MXE_NOT_FOUND:
			/* If the server process id could not be found in
			 * the semaphore file, just delete the semaphore
			 * file.
			 */

			break;
		default:
			/* Otherwise, return an error. */

			return mx_status;
		}

		if ( process_exists ) {
			/* A process with that process id exists, so we
			 * assume that it is the server and exit withou
			 * changing the file.  Note that *new_semaphore_file
			 * will be NULL at this point.
			 */

			 return mx_error( MXE_ALREADY_EXISTS, fname,
		 "Semaphore '%s' is owned by already existing process id %lu.",
		 		semaphore->name, process_id );
		}

		/* The server process mentioned by the semaphore file
		 * no longer exists, so delete the semaphore file.
		 */
#if MX_SEMAPHORE_DEBUG
		MX_DEBUG(-2,("%s: removing stale semaphore '%s'.",
			fname, semaphore->name));
#endif

		file_status = remove( semaphore->name );

		if ( file_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot remove stale semaphore file '%s'.  "
			"Errno = %d, error message = '%s'.",
				semaphore->name, saved_errno,
				strerror(saved_errno) );
		}

		/* We can now try again to create the semaphore file. */

		mx_status = mx_sysv_named_semaphore_exclusive_create_file(
						semaphore, new_semaphore_file );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static mx_status_type
mx_sysv_semaphore_create( MX_SEMAPHORE **semaphore,
				long initial_value,
				char *name )
{
	static const char fname[] = "mx_sysv_semaphore_create()";

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	FILE *new_semaphore_file;
	int num_semaphores, semget_flags;
	int status, saved_errno, create_new_semaphore;
	unsigned long process_id;
	size_t name_length;
	union semun value_union;
	mx_status_type mx_status;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Allocate the data structures we need. */

	*semaphore = (MX_SEMAPHORE *) malloc( sizeof(MX_SEMAPHORE) );

	if ( *semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_SEMAPHORE structure." );
	}

	system_v_private = (MX_SYSTEM_V_SEMAPHORE_PRIVATE *)
				malloc( sizeof(MX_SYSTEM_V_SEMAPHORE_PRIVATE) );

	if ( system_v_private == (MX_SYSTEM_V_SEMAPHORE_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Unable to allocate memory for a "
			"MX_SYSTEM_V_SEMAPHORE_PRIVATE structure." );
	}

	(*semaphore)->private = system_v_private;

	(*semaphore)->semaphore_type = MXT_SEM_SYSV;

	/* Is the initial value of the semaphore greater than the 
	 * maximum allowed value?
	 */

	if ( initial_value > SEMVMX ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested initial value (%ld) for the semaphore "
		"is larger than the maximum allowed value of %lu.",
			initial_value, (unsigned long) SEMVMX );
	}

	/* Allocate space for a name if one was specified. */

	if ( name == NULL ) {
		(*semaphore)->name = NULL;
	} else {
		name_length = strlen( name );

		name_length++;

		(*semaphore)->name = (char *)
					malloc( name_length * sizeof(char) );

		strlcpy( (*semaphore)->name, name, name_length );
	}

	/* Get the semaphore key. */

	create_new_semaphore = FALSE;
	new_semaphore_file = NULL;

	if ( (*semaphore)->name == NULL ) {

		if ( initial_value < 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot open an existing unnamed System V semaphore.  "
			"They can only be created." );
		}

		/* No name was specified, so we create a new
		 * private semaphore.
		 */

		system_v_private->semaphore_key = IPC_PRIVATE;

		create_new_semaphore = TRUE;

#if MX_SEMAPHORE_DEBUG
		MX_DEBUG(-2,("%s: Private semaphore, create_new_semaphore = %d",
			fname, create_new_semaphore));
#endif
	} else {
		if ( initial_value < 0 ) {
			/* We will connect to an old named semaphore.
			 *
			 * Check to see if a valid semaphore file is already
			 * there by asking for the server id.
			 */

			MX_DEBUG(-2,
			("%s: Connecting to existing semaphore '%s'.",
				fname, (*semaphore)->name ));

			mx_status =
				mx_sysv_named_semaphore_get_server_process_id(
					*semaphore, &process_id );
		} else {
			/* We will create a new named semaphore. */

			MX_DEBUG(-2,("%s: Creating new semaphore '%s'.",
				fname, (*semaphore)->name ));

			mx_status = mx_sysv_named_semaphore_create_new_file(
					*semaphore, &new_semaphore_file );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Construct the semaphore key from the filename.
		 * We always use 1 as the integer identifier.
		 */

		system_v_private->semaphore_key = ftok( (*semaphore)->name, 1 );

		if ( system_v_private->semaphore_key == (-1) ) {

			if ( new_semaphore_file != NULL ) {
				fclose( new_semaphore_file );

				new_semaphore_file = NULL;
			}

			return mx_error( MXE_FILE_IO_ERROR, fname,
		"The semaphore file '%s' does not exist or is not accessable.",
				(*semaphore)->name );
		}

#if MX_SEMAPHORE_DEBUG
		MX_DEBUG(-2,("%s: semaphore key = %#lx", 
		    fname, (unsigned long) system_v_private->semaphore_key));
#endif

		if ( new_semaphore_file == NULL ) {
			create_new_semaphore = FALSE;
		} else {
			create_new_semaphore = TRUE;
		}

#if MX_SEMAPHORE_DEBUG
		MX_DEBUG(-2,("%s: Named semaphore, create_new_semaphore = %d",
			fname, create_new_semaphore));
#endif
	}

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: create_new_semaphore = %d",
			fname, create_new_semaphore));
#endif

	if ( create_new_semaphore == FALSE ) {
		num_semaphores = 0;
		semget_flags = 0;
	} else {
		/* Create a new semaphore. */

		num_semaphores = 1;

		semget_flags = IPC_CREAT;

		/* Allow access by anyone. */

		semget_flags |= SEM_R | SEM_A ;
		semget_flags |= (SEM_R>>3) | (SEM_A>>3);
		semget_flags |= (SEM_R>>6) | (SEM_A>>6);
	}

	/* Create or connect to the semaphore. */

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: semaphore_key = %#lx, semget_flags = %#o", fname,
		(unsigned long) system_v_private->semaphore_key,
			semget_flags ));
#endif

	system_v_private->semaphore_id =
		semget( system_v_private->semaphore_key,
				num_semaphores, semget_flags );

	if ( (system_v_private->semaphore_id) == (-1) ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Cannot connect to preexisting semaphore key %#lx, file '%s'.  "
		"Errno = %d, error message = '%s'.",
			(unsigned long) system_v_private->semaphore_key,
			(*semaphore)->name,
			saved_errno, strerror(saved_errno) );
	}

	/* If necessary, set the initial value of the semaphore.
	 *
	 * WARNING: Note that the gap in time between semget() and semctl()
	 *          theoretically leaves a window of opportunity for a race
	 *          condition, so this function is not really thread safe.
	 */

	if ( create_new_semaphore ) {

		value_union.val = (int) initial_value;

		status = semctl( system_v_private->semaphore_id, 0,
							SETVAL, value_union );

		if ( status != 0 ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case EINVAL:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Either the semaphore key %lu, filename '%s' does not "
			"exist or semaphore number 0 was not found.",
				(unsigned long) system_v_private->semaphore_key,
				(*semaphore)->name );

			case EPERM:
			case EACCES:
				return mx_error( MXE_PERMISSION_DENIED, fname,
			"The current process lacks the appropriated privilege "
			"to access the semaphore in the requested manner." );

			case ERANGE:
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Attempted to set semaphore value (%ld) outside the allowed "
		"range of 0 to %d.", initial_value, SEMVMX );

			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected semctl() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
			}
		}

		system_v_private->creator_process = getpid();

#if defined(_POSIX_THREADS)
		system_v_private->creator_thread = pthread_self();
#endif

		if ( new_semaphore_file != (FILE *) NULL ) {
			/* Write the process ID, semaphore key and
			 * semaphore ID to the semaphore file.
			 */

#if MX_SEMAPHORE_DEBUG
			MX_DEBUG(-2,
			("%s: process ID = %lu, key = %#lx, sem ID = %lu",fname,
			    (unsigned long) system_v_private->creator_process,
			    (unsigned long) system_v_private->semaphore_key,
			    (unsigned long) system_v_private->semaphore_id));
#endif

			status = fprintf( new_semaphore_file, "%lu %#lx %lu\n",
			    (unsigned long) system_v_private->creator_process,
				(unsigned long) system_v_private->semaphore_key,
				(unsigned long) system_v_private->semaphore_id);

			if ( status < 0 ) {
				saved_errno = errno;

				(void) fclose( new_semaphore_file );

				return mx_error( MXE_FILE_IO_ERROR, fname,
				"Unable to write identification to semaphore "
				"file '%s'.  Errno = %d, error message = '%s'.",
					(*semaphore)->name,
					saved_errno, strerror( saved_errno ) );
			}

			status = fclose( new_semaphore_file );

			if ( status < 0 ) {
				saved_errno = errno;

				return mx_error( MXE_FILE_IO_ERROR, fname,
				"Unable to close semaphore file '%s'.  "
				"Errno = %d, error message = '%s'.",
					(*semaphore)->name,
					saved_errno, strerror( saved_errno ) );
			}

			new_semaphore_file = NULL;
		}
	}

	if ( new_semaphore_file != (FILE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The FILE pointer for semaphore file '%s' was non-NULL at "
		"a time that it should have been NULL.  This is an "
		"internal error in MX and should be reported.",
			(*semaphore)->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_sysv_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_sysv_semaphore_destroy()";

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	int status, saved_errno, destroy_semaphore;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	system_v_private = semaphore->private;

	if ( system_v_private == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	destroy_semaphore = FALSE;

	if ( semaphore->name == NULL ) {
		destroy_semaphore = TRUE;
	} else {
		if ( system_v_private->creator_process == getpid() ) {
#if defined(_POSIX_THREADS)
		    if ( system_v_private->creator_thread == pthread_self() ){
			destroy_semaphore = TRUE;
		    }
#else
		    destroy_semaphore = TRUE;
#endif
		}
	}

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: destroy_semaphore = %d", fname, destroy_semaphore));
#endif

	if ( destroy_semaphore ) {
		status = semctl( system_v_private->semaphore_id, 0, IPC_RMID );

		if ( status != 0 ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case EINVAL:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Either the semaphore key %lu, filename '%s' does not "
			"exist or semaphore number 0 was not found.",
				(unsigned long) system_v_private->semaphore_key,
				semaphore->name );

			case EPERM:
				return mx_error( MXE_PERMISSION_DENIED, fname,
			"The current process lacks the appropriated privilege "
			"to access the semaphore." );

			case EACCES:
				return mx_error( MXE_TYPE_MISMATCH, fname,
			"The operation and the mode of the semaphore set "
			"do not match." );

			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected semctl() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
			}
		}

		if ( semaphore->name != NULL ) {
			status = remove( semaphore->name );

			if ( status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_FILE_IO_ERROR, fname,
				"Cannot remove semaphore file '%s'.  "
				"Errno = %d, error message = '%s'.",
					semaphore->name, saved_errno,
					strerror(saved_errno) );
			}
			mx_free( semaphore->name );
		}
	}

	mx_free( system_v_private );

	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

static long
mx_sysv_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	static char fname[] = "mx_sysv_semaphore_lock()";

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	struct sembuf sembuf_struct;
	int status, saved_errno;

	system_v_private = semaphore->private;

	if ( system_v_private == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	sembuf_struct.sem_num = 0;
	sembuf_struct.sem_op = -1;
	sembuf_struct.sem_flg = 0;

	status = semop( system_v_private->semaphore_id, &sembuf_struct, 1 );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
		case EACCES:
			return MXE_TYPE_MISMATCH;
		case EAGAIN:
			return MXE_MIGHT_CAUSE_DEADLOCK;
		case E2BIG:
			return MXE_WOULD_EXCEED_LIMIT;
		case EFBIG:
			return MXE_ILLEGAL_ARGUMENT;
#if defined(EIDRM)
		case EIDRM:
			return MXE_NOT_FOUND;
#endif
		case EINTR:
			return MXE_INTERRUPTED;
		case ENOSPC:
			return MXE_OUT_OF_MEMORY;
		case ERANGE:
			return MXE_WOULD_EXCEED_LIMIT;
		default:
			(void) mx_error( MXE_UNKNOWN_ERROR, fname,
				"Unexpected error code returned by semop().  "
				"Errno = %d, error message = '%s'.",
				saved_errno, strerror( saved_errno ) );

			return MXE_UNKNOWN_ERROR;
		}
	}
	return MXE_SUCCESS;
}

static long
mx_sysv_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_sysv_semaphore_unlock()";

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	struct sembuf sembuf_struct;
	int status, saved_errno;

	system_v_private = semaphore->private;

	if ( system_v_private == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	sembuf_struct.sem_num = 0;
	sembuf_struct.sem_op  = 1;
	sembuf_struct.sem_flg = 0;

	status = semop( system_v_private->semaphore_id, &sembuf_struct, 1 );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
		case EACCES:
			return MXE_TYPE_MISMATCH;
		case EAGAIN:
			return MXE_MIGHT_CAUSE_DEADLOCK;
		case E2BIG:
			return MXE_WOULD_EXCEED_LIMIT;
		case EFBIG:
			return MXE_ILLEGAL_ARGUMENT;
#if defined(EIDRM)
		case EIDRM:
			return MXE_NOT_FOUND;
#endif
		case EINTR:
			return MXE_INTERRUPTED;
		case ENOSPC:
			return MXE_OUT_OF_MEMORY;
		case ERANGE:
			return MXE_WOULD_EXCEED_LIMIT;
		default:
			(void) mx_error( MXE_UNKNOWN_ERROR, fname,
				"Unexpected error code returned by semop().  "
				"Errno = %d, error message = '%s'.",
				saved_errno, strerror( saved_errno ) );

			return MXE_UNKNOWN_ERROR;
		}
	}
	return MXE_SUCCESS;
}

static long
mx_sysv_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_sysv_semaphore_trylock()";

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	struct sembuf sembuf_struct;
	int status, saved_errno;

	system_v_private = semaphore->private;

	if ( system_v_private == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	sembuf_struct.sem_num = 0;
	sembuf_struct.sem_op = -1;
	sembuf_struct.sem_flg = IPC_NOWAIT;

	status = semop( system_v_private->semaphore_id, &sembuf_struct, 1 );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
		case EACCES:
			return MXE_TYPE_MISMATCH;
		case EAGAIN:
			return MXE_NOT_AVAILABLE;
		case E2BIG:
			return MXE_WOULD_EXCEED_LIMIT;
		case EFBIG:
			return MXE_ILLEGAL_ARGUMENT;
#if defined(EIDRM)
		case EIDRM:
			return MXE_NOT_FOUND;
#endif
		case EINTR:
			return MXE_INTERRUPTED;
		case ENOSPC:
			return MXE_OUT_OF_MEMORY;
		case ERANGE:
			return MXE_WOULD_EXCEED_LIMIT;
		default:
			(void) mx_error( MXE_UNKNOWN_ERROR, fname,
				"Unexpected error code returned by semop().  "
				"Errno = %d, error message = '%s'.",
				saved_errno, strerror( saved_errno ) );

			return MXE_UNKNOWN_ERROR;
		}
	}
	return MXE_SUCCESS;
}

static mx_status_type
mx_sysv_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	int saved_errno, semaphore_value;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	system_v_private = semaphore->private;

	if ( system_v_private == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	semaphore_value = semctl( system_v_private->semaphore_id, 0, GETVAL );

	if ( semaphore_value < 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected semctl() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
		}
	}

	*current_value = (unsigned long) semaphore_value;

	return MX_SUCCESSFUL_RESULT;
}

#else     /* Do not have System V semaphores */

#define mx_sysv_semaphore_unsupported \
		mx_error( MXE_UNSUPPORTED, fname, \
		"System V semaphores are not supported on this computer." )

#define mx_sysv_semaphore_create(x,y,z)  mx_sysv_semaphore_unsupported
#define mx_sysv_semaphore_destroy(x)     mx_sysv_semaphore_unsupported
#define mx_sysv_semaphore_lock(x)        MXE_UNSUPPORTED
#define mx_sysv_semaphore_unlock(x)      MXE_UNSUPPORTED
#define mx_sysv_semaphore_trylock(x)     MXE_UNSUPPORTED
#define mx_sysv_semaphore_get_value(x,y) mx_sysv_semaphore_unsupported

#endif    /* System V semaphores */

/*======================= Posix Pthreads ======================*/

#if defined(_POSIX_SEMAPHORES) || defined(OS_IRIX) || defined(OS_MACOSX)

#include <fcntl.h>
#include <semaphore.h>

typedef struct {
	sem_t *p_semaphore;
} MX_POSIX_SEMAPHORE_PRIVATE;

static mx_status_type
mx_posix_sem_init( MX_SEMAPHORE *semaphore,
			int pshared,
			unsigned int value )
{
	static const char fname[] = "mx_posix_sem_init()";

	MX_POSIX_SEMAPHORE_PRIVATE *posix_private;
	int status, saved_errno;

	posix_private = (MX_POSIX_SEMAPHORE_PRIVATE *) semaphore->private;

	/* Create an unnamed semaphore. */

	status = sem_init( posix_private->p_semaphore, pshared, value );

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: unnamed semaphore = %p",
		fname, posix_private->p_semaphore));
#endif

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EINVAL:

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested initial value %u exceeds the maximum "
		"allowed value.", value );
		case ENOSPC:
			return mx_error( MXE_NOT_AVAILABLE, fname,
    "A resource required to create the semaphore is not available.");
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
	"Posix semaphores are not supported on this platform." );
		case EPERM:
			return mx_error( MXE_PERMISSION_DENIED, fname,
		"The current process lacks the appropriated privilege "
		"to create the semaphore." );
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected sem_init() error code %d returned.  "
		"Error message = '%s'.",
			saved_errno, strerror( saved_errno ) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_posix_sem_open( MX_SEMAPHORE *semaphore,
			int open_flag,
			mode_t mode,
			unsigned int value )
{
	static const char fname[] = "mx_posix_sem_open()";

	MX_POSIX_SEMAPHORE_PRIVATE *posix_private;
	int saved_errno;

	posix_private = (MX_POSIX_SEMAPHORE_PRIVATE *) semaphore->private;

	/* Create or open a named semaphore. */

	posix_private->p_semaphore = sem_open( semaphore->name,
						open_flag, mode, value );

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: named semaphore = %p",
		fname, posix_private->p_semaphore));
#endif

#if defined(OS_HPUX)
	if ( posix_private->p_semaphore == (sem_t *) (-1) ) {
#else
	if ( posix_private->p_semaphore == (sem_t *) SEM_FAILED ) {
#endif
		saved_errno = errno;

		switch( saved_errno ) {
		case EACCES:
			return mx_error( MXE_PERMISSION_DENIED, fname,
				"Could not connect to semaphore '%s'.",
					semaphore->name );
		case EEXIST:
			return mx_error(
				(MXE_ALREADY_EXISTS | MXE_QUIET), fname,
				"Semaphore '%s' already exists, but you "
				"requested exclusive access.",
					semaphore->name );
		case EINTR:
			return mx_error( MXE_INTERRUPTED, fname,
			    "sem_open() for semaphore '%s' was interrupted.",
			    		semaphore->name );
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Either sem_open() is not supported for "
				"semaphore '%s' or the requested initial "
				"value %u was larger than the maximum "
				"allowed value.",
					semaphore->name, value );
		case EMFILE:
			return mx_error( MXE_NOT_AVAILABLE, fname,
				"Ran out of semaphore or file descriptors "
				"for this process." );
		case ENAMETOOLONG:
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Semaphore name '%s' is longer than the "
				"maximum allowed length for this system.",
					semaphore->name );
		case ENFILE:
			return mx_error( MXE_NOT_AVAILABLE, fname,
				"Too many semaphores are currently open "
				"on this computer." );
		case ENOENT:
			return mx_error( MXE_NOT_FOUND, fname,
				"Semaphore '%s' was not found and the call "
				"to sem_open() did not specify O_CREAT.",
					semaphore->name );
		case ENOSPC:
			return mx_error( MXE_OUT_OF_MEMORY, fname,
				"There is insufficient space for the creation "
				"of the new named semaphore." );
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
				"The function sem_open() is not supported "
				"on this platform." );
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected sem_open() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_posix_sem_unlink( MX_SEMAPHORE *semaphore )
{
#if ! defined(OS_CYGWIN)

	static const char fname[] = "mx_posix_sem_unlink()";

	int status, saved_errno;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked for semaphore '%s'",
		fname, semaphore->name));
#endif

	status = sem_unlink( semaphore->name );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		default:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error for sem_unlink( '%s' ),  "
			"Errno = %d, error message = '%s'",
				semaphore->name, saved_errno,
				strerror( saved_errno ) );
		}
	}

#endif    /* ! OS_CYGWIN */

	return MX_SUCCESSFUL_RESULT;
}

/*-----*/

static mx_status_type
mx_posix_semaphore_create( MX_SEMAPHORE **semaphore,
				long initial_value,
				char *name )
{
	static const char fname[] = "mx_posix_semaphore_create()";

	MX_POSIX_SEMAPHORE_PRIVATE *posix_private;
	long sem_value_max;
	size_t name_length;
	mx_status_type mx_status;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Allocate the data structures we need. */

	*semaphore = (MX_SEMAPHORE *) malloc( sizeof(MX_SEMAPHORE) );

	if ( *semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_SEMAPHORE structure." );
	}

	posix_private = (MX_POSIX_SEMAPHORE_PRIVATE *)
				malloc( sizeof(MX_POSIX_SEMAPHORE_PRIVATE) );

	if ( posix_private == (MX_POSIX_SEMAPHORE_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Unable to allocate memory for a MX_POSIX_SEMAPHORE_PRIVATE structure." );
	}

	/* If the semaphore is unnamed, then we need to allocate memory
	 * for the sem_t object;
	 */

	if ( name == NULL ) {
		posix_private->p_semaphore = (sem_t *) malloc( sizeof(sem_t) );

		if ( posix_private->p_semaphore == (sem_t *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Unable to allocate memory for a sem_t object." );
		}
	}

	(*semaphore)->private = posix_private;

	(*semaphore)->semaphore_type = MXT_SEM_POSIX;

	/* Is the initial value of the semaphore greater than the 
	 * maximum allowed value?
	 */
#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(__NetBSD_Version__)
	sem_value_max = SEM_VALUE_MAX;
#else
	sem_value_max = sysconf(_SC_SEM_VALUE_MAX);
#endif

	if ( initial_value > sem_value_max ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested initial value (%ld) for the semaphore "
		"is larger than the maximum allowed value of %ld.",
			initial_value, sem_value_max );
	}

	/* Allocate space for a name if one was specified. */

	if ( name == NULL ) {
		(*semaphore)->name = NULL;
	} else {
		name_length = strlen( name );

		name_length++;

		(*semaphore)->name = (char *)
					malloc( name_length * sizeof(char) );

		strlcpy( (*semaphore)->name, name, name_length );
	}

	/* Create the semaphore. */

	if ( (*semaphore)->name == NULL ) {

		if ( initial_value < 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot open an existing unnamed Posix semaphore.  "
			"They can only be created." );
		}

		/* Create an unnamed semaphore. */

		mx_status = mx_posix_sem_init( *semaphore, 0,
					(unsigned int) initial_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		if ( initial_value < 0 ) {

			/* Open an existing named semaphore. */

			mx_status = mx_posix_sem_open( *semaphore, 0, 0, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

		} else {
			/* Create a new named semaphore. */

			mx_status = mx_posix_sem_open( *semaphore,
					O_CREAT | O_EXCL,
					0644,
					(unsigned int) initial_value );

			switch( mx_status.code ) {
			case MXE_SUCCESS:
				break;
			case MXE_ALREADY_EXISTS:
				/* If the semaphore already exists, try to
				 * delete it.
				 */

				MX_DEBUG(-2,
				("%s: removing stale semaphore '%s'.",
					fname, (*semaphore)->name ));

				mx_status = mx_posix_sem_unlink( *semaphore );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* Then create a new semaphore. */

				mx_status = mx_posix_sem_open( *semaphore,
					O_CREAT | O_EXCL,
					0644,
					(unsigned int) initial_value );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
				break;
			default:
				return mx_status;
			}
		}
	}

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: posix_private->p_semaphore = %p",
			fname, posix_private->p_semaphore));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_posix_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_posix_semaphore_destroy()";

	MX_POSIX_SEMAPHORE_PRIVATE *posix_private;
	int status, saved_errno;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	posix_private = semaphore->private;

	if ( posix_private == (MX_POSIX_SEMAPHORE_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	if ( semaphore->name == NULL ) {
		/* Destroy an unnamed semaphore. */

		status = sem_destroy( posix_private->p_semaphore );
	} else {
		/* Close a named semaphore. */

		status = sem_close( posix_private->p_semaphore );
	}

	if ( status != 0 ) {
		saved_errno = errno;

		switch( status ) {
		case EBUSY:
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
				"Detected an attempt to destroy a semaphore "
				"that is locked or referenced by "
				"another thread." );
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The semaphore passed was not "
				"a valid semaphore." );
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"Posix semaphores are not supported on this platform." );
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
		}
	}

	if ( semaphore->name == NULL ) {
		mx_free( posix_private->p_semaphore );
	} else {
		mx_free( semaphore->name );
	}

	mx_free( posix_private );

	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

static long
mx_posix_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_posix_semaphore_lock()";

	MX_POSIX_SEMAPHORE_PRIVATE *posix_private;
	int status, saved_errno;

#if 0
	MX_DEBUG(-2,("%s: semaphore = %p", fname, semaphore));
	MX_DEBUG(-2,("%s: semaphore->name = %p", fname, semaphore->name));
	if ( semaphore->name != NULL ) {
		MX_DEBUG(-2,("%s: semaphore->name = '%s'",
				fname, semaphore->name));
	}
	MX_DEBUG(-2,("%s: semaphore->semaphore_type = %d",
			fname, semaphore->semaphore_type));
	MX_DEBUG(-2,("%s: semaphore->private = %p",
			fname, semaphore->private));
#endif

	posix_private = semaphore->private;

	if ( posix_private == (MX_POSIX_SEMAPHORE_PRIVATE *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

#if 0
	MX_DEBUG(-2,("%s: posix_private->p_semaphore = %p",
			fname, posix_private->p_semaphore));
	MX_DEBUG(-2,("%s: About to call sem_wait()", fname));
#endif

	status = sem_wait( posix_private->p_semaphore );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
		case EINTR:
			return MXE_INTERRUPTED;
		case ENOSYS:
			return MXE_UNSUPPORTED;
		case EDEADLK:
			return MXE_MIGHT_CAUSE_DEADLOCK;
		default:
			(void) mx_error( MXE_UNKNOWN_ERROR, fname,
			    "Unexpected error code returned by sem_wait().  "
				"Errno = %d, error message = '%s'.",
				saved_errno, strerror( saved_errno ) );

			return MXE_UNKNOWN_ERROR;
		}
	}
	return MXE_SUCCESS;
}

static long
mx_posix_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_posix_semaphore_unlock()";

	MX_POSIX_SEMAPHORE_PRIVATE *posix_private;
	int status, saved_errno;

	posix_private = semaphore->private;

	if ( posix_private == (MX_POSIX_SEMAPHORE_PRIVATE *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = sem_post( posix_private->p_semaphore );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
		case ENOSYS:
			return MXE_UNSUPPORTED;
		default:
			(void) mx_error( MXE_UNKNOWN_ERROR, fname,
			    "Unexpected error code returned by sem_post().  "
				"Errno = %d, error message = '%s'.",
				saved_errno, strerror( saved_errno ) );

			return MXE_UNKNOWN_ERROR;
		}
	}
	return MXE_SUCCESS;
}

static long
mx_posix_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_posix_semaphore_trylock()";

	MX_POSIX_SEMAPHORE_PRIVATE *posix_private;
	int status, saved_errno;

	posix_private = semaphore->private;

	if ( posix_private == (MX_POSIX_SEMAPHORE_PRIVATE *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = sem_trywait( posix_private->p_semaphore );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
#if defined(OS_SOLARIS)
		/* Saw this value returned on Solaris 8.  (WML) */
		case 0:
#endif
		case EAGAIN:
			return MXE_NOT_AVAILABLE;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
		case EINTR:
			return MXE_INTERRUPTED;
		case ENOSYS:
			return MXE_UNSUPPORTED;
		case EDEADLK:
			return MXE_MIGHT_CAUSE_DEADLOCK;
		default:
			(void) mx_error( MXE_UNKNOWN_ERROR, fname,
			    "Unexpected error code returned by sem_trywait().  "
				"Errno = %d, error message = '%s'.",
				saved_errno, strerror( saved_errno ) );

			return MXE_UNKNOWN_ERROR;
		}
	}
	return MXE_SUCCESS;
}

static mx_status_type
mx_posix_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_posix_semaphore_get_value()";

	MX_POSIX_SEMAPHORE_PRIVATE *posix_private;
	int semaphore_value, status, saved_errno;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked for thread %p.",
		fname, mx_get_current_thread_pointer() ));
#endif

	status = saved_errno = 0;  /* Keep quiet about unused variables. */

	posix_private = semaphore->private;

	if ( posix_private == (MX_POSIX_SEMAPHORE_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: posix_private->p_semaphore = %p",
		fname, posix_private->p_semaphore));
#endif

#if defined(OS_MACOSX)
	/* sem_getvalue() does not seem to work on MacOS X. */

	semaphore_value = 0;
#else
	status = sem_getvalue( posix_private->p_semaphore, &semaphore_value );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( status ) {
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected sem_getvalue() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
		}
	}
#endif

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: semaphore value = %lu",
		fname, (unsigned long) semaphore_value));
#endif

	*current_value = (unsigned long) semaphore_value;

	return MX_SUCCESSFUL_RESULT;
}

/*=============================================================*/

#else     /* Do not have Posix semaphores */

#define mx_posix_semaphore_unsupported \
		mx_error( MXE_UNSUPPORTED, fname, \
		"Posix semaphores are not supported on this computer." )

#define mx_posix_semaphore_create(x,y,z)  mx_posix_semaphore_unsupported
#define mx_posix_semaphore_destroy(x)     mx_posix_semaphore_unsupported
#define mx_posix_semaphore_lock(x)        MXE_UNSUPPORTED
#define mx_posix_semaphore_unlock(x)      MXE_UNSUPPORTED
#define mx_posix_semaphore_trylock(x)     MXE_UNSUPPORTED
#define mx_posix_semaphore_get_value(x,y) mx_posix_semaphore_unsupported

#endif   /* Posix semaphores */

/*=============================================================*/

/* The following functions route semaphore calls to the correct
 * Posix or System V function depending on which kinds of semaphores
 * are supported on the current platform.
 */

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
			long initial_value,
			char *name )
{
	static const char fname[] = "mx_semaphore_create()";

	mx_status_type mx_status;

	if ( semaphore == (MX_SEMAPHORE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	if ( name == (char *) NULL ) {
		if ( mx_use_posix_unnamed_semaphores ) {
			mx_status = mx_posix_semaphore_create( semaphore,
								initial_value,
								name );
		} else {
			mx_status = mx_sysv_semaphore_create( semaphore,
								initial_value,
								name );
		}
	} else {
		if ( mx_use_posix_named_semaphores ) {
			mx_status = mx_posix_semaphore_create( semaphore,
								initial_value,
								name );
		} else {
			mx_status = mx_sysv_semaphore_create( semaphore,
								initial_value,
								name );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	mx_status_type mx_status;

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	if ( semaphore->semaphore_type == MXT_SEM_POSIX ) {
		mx_status = mx_posix_semaphore_destroy( semaphore );
	} else {
		mx_status = mx_sysv_semaphore_destroy( semaphore );
	}

	return mx_status;
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	long result;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	if ( semaphore->semaphore_type == MXT_SEM_POSIX ) {
		result = mx_posix_semaphore_lock( semaphore );
	} else {
		result = mx_sysv_semaphore_lock( semaphore );
	}

	return result;
}

MX_EXPORT long
mx_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	long result;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	if ( semaphore->semaphore_type == MXT_SEM_POSIX ) {
		result = mx_posix_semaphore_unlock( semaphore );
	} else {
		result = mx_sysv_semaphore_unlock( semaphore );
	}

	return result;
}

MX_EXPORT long
mx_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	long result;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	if ( semaphore->semaphore_type == MXT_SEM_POSIX ) {
		result = mx_posix_semaphore_trylock( semaphore );
	} else {
		result = mx_sysv_semaphore_trylock( semaphore );
	}

	return result;
}

MX_EXPORT mx_status_type
mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	mx_status_type mx_status;

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( current_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The current_value pointer passed was NULL." );
	}

	if ( semaphore->semaphore_type == MXT_SEM_POSIX ) {
		mx_status = mx_posix_semaphore_get_value( semaphore,
							current_value );
	} else {
		mx_status = mx_sysv_semaphore_get_value( semaphore,
							current_value );
	}

	return mx_status;
}

/********** Use the following stubs when threads are not supported **********/

#elif defined(OS_DJGPP)

/* WARNING: You must _not_ use the following version if there is any
 * possibility that threads or asynchronous event handlers will be used,
 * since it is not thread safe in any way.
 */

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
			long initial_value,
			char *name )
{
	static const char fname[] = "mx_semaphore_create()";

	unsigned long *semaphore_value_ptr;
	size_t name_length;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( initial_value < 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Negative initial values are not supported on this platform." );
	}

	if ( semaphore == (MX_SEMAPHORE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	*semaphore = (MX_SEMAPHORE *) malloc( sizeof(MX_SEMAPHORE) );

	if ( *semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_SEMAPHORE structure." );
	}

	semaphore_value_ptr = (unsigned long *) malloc( sizeof(unsigned long) );

	if ( semaphore_value_ptr == (unsigned long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a unsigned long pointer." );
	}
	
	(*semaphore)->private = semaphore_value_ptr;

	(*semaphore)->semaphore_type = MXT_SEM_NONE;

	/* Allocate space for a name if one was specified. */

	if ( name == NULL ) {
		(*semaphore)->name = NULL;
	} else {
		name_length = strlen( name );

		name_length++;

		(*semaphore)->name = (char *)
					malloc( name_length * sizeof(char) );

		strlcpy( (*semaphore)->name, name, name_length );
	}

	*semaphore_value_ptr = initial_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	unsigned long *semaphore_value_ptr;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	semaphore_value_ptr = semaphore->private;

	if ( semaphore_value_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	if ( semaphore->name != NULL ) {
		mx_free( semaphore->name );
	}

	mx_free( semaphore_value_ptr );

	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	return mx_semaphore_trylock( semaphore );
}

MX_EXPORT long
mx_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	unsigned long *semaphore_value_ptr;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_value_ptr = semaphore->private;

	if ( semaphore_value_ptr == (unsigned long *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	(*semaphore_value_ptr)++;

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	unsigned long *semaphore_value_ptr;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_value_ptr = semaphore->private;

	if ( semaphore_value_ptr == (unsigned long *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	if ( *semaphore_value_ptr == 0 )
		return MXE_MIGHT_CAUSE_DEADLOCK;

	(*semaphore_value_ptr)--;

	return MXE_SUCCESS;
}

MX_EXPORT mx_status_type
mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( current_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The current_value pointer passed was NULL." );
	}

	*current_value = (unsigned long)
				*((unsigned long *) semaphore->private);

	return MX_SUCCESSFUL_RESULT;
}

/***************************************************************/

#elif 0

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
			long initial_value,
			char *name )
{
	static const char fname[] = "mx_semaphore_create()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"MX semaphore functions are not supported for this operating system." );
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"MX semaphore functions are not supported for this operating system." );
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	return MXE_UNSUPPORTED;
}

MX_EXPORT long
mx_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	return MXE_UNSUPPORTED;
}

MX_EXPORT long
mx_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	return MXE_UNSUPPORTED;
}

MX_EXPORT mx_status_type
mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"MX semaphore functions are not supported for this operating system." );
}

#else

#error MX semaphore functions have not yet been defined for this platform.

#endif


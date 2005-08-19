/*
 * Name:    mx_semaphore.c
 *
 * Purpose: MX semaphore functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
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
#include "mx_unistd.h"
#include "mx_semaphore.h"

/************************ Microsoft Win32 ***********************/

#if defined(OS_WIN32)

#include <windows.h>

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
		unsigned long initial_value,
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
	
	(*semaphore)->semaphore_ptr = semaphore_handle_ptr;

	(*semaphore)->semaphore_type = MXT_SEM_WIN32;

	/* Allocate space for a name if one was specified. */

	if ( name == NULL ) {
		(*semaphore)->name = NULL;
	} else {
		name_length = strlen( name );

		name_length++;

		(*semaphore)->name = (char *)
					malloc( name_length * sizeof(char) );

		mx_strncpy( (*semaphore)->name, name, name_length );
	}

	*semaphore_handle_ptr = CreateSemaphore( NULL,
						initial_value,
						initial_value,
						(*semaphore)->name );

	if ( *semaphore_handle_ptr == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to create a semaphore.  "
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

	semaphore_handle_ptr = semaphore->semaphore_ptr;

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

	semaphore_handle_ptr = semaphore->semaphore_ptr;

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

	semaphore_handle_ptr = semaphore->semaphore_ptr;

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
			"Unexpected error from WaitForSingleObject() "
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

	semaphore_handle_ptr = semaphore->semaphore_ptr;

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

MX_EXPORT mx_status_type
mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	HANDLE *semaphore_handle_ptr;
	BOOL status;
	LONG old_value;

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( current_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The current_value pointer passed was NULL." );
	}

	semaphore_handle_ptr = semaphore->semaphore_ptr;

	if ( semaphore_handle_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The semaphore handle pointer for semaphore %p is NULL.",
			semaphore );
	}

	status = ReleaseSemaphore( *semaphore_handle_ptr, 0, &old_value );

	if ( status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[100];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error from WaitForSingleObject() "
			"for semaphore %p. "
			"Win32 error code = %ld, error_message = '%s'",
			semaphore, last_error_code, message_buffer );
	}

	*current_value = (unsigned long) old_value;

	return MX_SUCCESSFUL_RESULT;
}

/*********************** Unix and Posix systems **********************/

#elif defined(OS_UNIX)

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

#if defined(OS_LINUX)

static int mx_use_posix_unnamed_semaphores = TRUE;
static int mx_use_posix_named_semaphores   = FALSE;

#elif defined(OS_MACOSX)

static int mx_use_posix_unnamed_semaphores = FALSE;
static int mx_use_posix_named_semaphores   = TRUE;

#else

static int mx_use_posix_unnamed_semaphores = TRUE;
static int mx_use_posix_named_semaphores   = TRUE;

#endif

/*======================= System V Semaphores ======================*/

#if defined(OS_LINUX) || defined(OS_MACOSX)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

#if defined(_POSIX_THREADS)
#  include <pthread.h>
#endif

#if defined(OS_LINUX) || defined(OS_MACOSX)
#  define SEMVMX  32767
#endif

#if !defined(SEM_A)
#define SEM_A  0200	/* Alter permission */
#define SEM_R  0400	/* Read permission  */
#endif

#if _SEM_SEMUN_UNDEFINED
/* I wonder what possible advantage there is to making this
 * union definition optional?
 */
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
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

static mx_status_type
mx_sysv_named_semaphore_create_new_file( MX_SEMAPHORE *semaphore,
				FILE **new_semaphore_file )
{
	static const char fname[] = "mx_sysv_named_semaphore_create_new_file()";

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

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: MARKER A, fd = %d", fname, fd));
#endif

	if ( fd < 0 ) {
		saved_errno = errno;

#if MX_SEMAPHORE_DEBUG
		MX_DEBUG(-2,("%s: MARKER A.1, saved_errno = %d",
				fname, saved_errno));
#endif

		switch( saved_errno ) {
		case EEXIST:
			return mx_error( MXE_ALREADY_EXISTS, fname,
			"Semaphore file '%s' already exists.",
				semaphore->name );
			break;
		case EACCES:
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The filesystem permissions do not allow us to "
			"write to semaphore file '%s'.",
				semaphore->name );
			break;
		default:
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An attempt to create semaphore file '%s' failed.  "
			"Errno = %d, error message = '%s'.",
				semaphore->name, saved_errno,
				strerror( saved_errno ) );
			break;
		}
	}

	/* Convert the file descriptor to a file pointer. */

	(*new_semaphore_file) = fdopen( fd, "w" );

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: MARKER B, *new_semaphore_file = %p",
		fname, *new_semaphore_file));
#endif

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

		return mx_error( MXE_NOT_FOUND, fname,
		"Unable to read semaphore id string from semaphore file '%s'.",
			semaphore->name );
	}

	(void) fclose(semaphore_file);

	num_items = sscanf( buffer, "%lu", process_id );

	if ( num_items != 1 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Unable to find the server process id in the line '%s' "
		"read from semaphore file '%s'.", buffer, semaphore->name);
	}

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: process_id = %lu", fname, *process_id));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_sysv_named_semaphore_get_key( MX_SEMAPHORE *semaphore,
			FILE **new_semaphore_file )
{
	static const char fname[] = "mx_sysv_named_semaphore_get_key()";

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

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: MARKER 1, *new_semaphore_file = %p",
			fname, *new_semaphore_file));
#endif

	system_v_private = semaphore->semaphore_ptr;

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

	mx_status = mx_sysv_named_semaphore_create_new_file( semaphore,
						new_semaphore_file );

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: MARKER 2, *new_semaphore_file = %p",
			fname, *new_semaphore_file));
#endif

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
			break;
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

#if MX_SEMAPHORE_DEBUG
		MX_DEBUG(-2,("%s: MARKER 2.1, *new_semaphore_file = %p",
			fname, *new_semaphore_file));
#endif

		mx_status = mx_sysv_named_semaphore_create_new_file( semaphore,
							new_semaphore_file );

#if MX_SEMAPHORE_DEBUG
		MX_DEBUG(-2,("%s: MARKER 2.2, *new_semaphore_file = %p",
			fname, *new_semaphore_file));
#endif

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_status;
		break;
	}

	/* Construct the semaphore key from the filename.
	 * We always use 1 as the integer identifier.
	 */

	system_v_private->semaphore_key = ftok( semaphore->name, 1 );

	if ( system_v_private->semaphore_key == (-1) ) {

#if MX_SEMAPHORE_DEBUG
		MX_DEBUG(-2,("%s: MARKER 3, *new_semaphore_file = %p",
			fname, *new_semaphore_file));
#endif

		if ( *new_semaphore_file != NULL ) {
			fclose( *new_semaphore_file );

			*new_semaphore_file = NULL;
		}

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The semaphore file '%s' does not exist or is not accessable.",
			semaphore->name );
	}

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: MARKER 4, *new_semaphore_file = %p",
			fname, *new_semaphore_file));
#endif

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: semaphore key = %#lx", 
		fname, (unsigned long) system_v_private->semaphore_key));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static mx_status_type
mx_sysv_semaphore_create( MX_SEMAPHORE **semaphore,
			unsigned long initial_value,
			char *name )
{
	static const char fname[] = "mx_sysv_semaphore_create()";

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	FILE *new_semaphore_file;
	int num_semaphores, semget_flags;
	int status, saved_errno, create_new_semaphore;
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

	(*semaphore)->semaphore_ptr = system_v_private;

	(*semaphore)->semaphore_type = MXT_SEM_SYSV;

	/* Is the initial value of the semaphore greater than the 
	 * maximum allowed value?
	 */

	if ( initial_value > SEMVMX ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested initial value (%lu) for the semaphore "
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

		mx_strncpy( (*semaphore)->name, name, name_length );
	}

	/* Get the semaphore key. */

	create_new_semaphore = FALSE;
	new_semaphore_file = NULL;

	if ( (*semaphore)->name == NULL ) {

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
		mx_status = mx_sysv_named_semaphore_get_key( *semaphore,
							&new_semaphore_file );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

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
		"Cannot connect to preexisting semaphore key %lu, file '%s'.  "
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
				break;
			case EPERM:
			case EACCES:
				return mx_error( MXE_PERMISSION_DENIED, fname,
			"The current process lacks the appropriated privilege "
			"to access the semaphore in the requested manner." );
				break;
			case ERANGE:
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Attempted to set semaphore value (%lu) outside the allowed "
		"range of 0 to %d.", initial_value, SEMVMX );
				break;
			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected semctl() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
				break;
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

	system_v_private = semaphore->semaphore_ptr;

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
				break;
			case EPERM:
				return mx_error( MXE_PERMISSION_DENIED, fname,
			"The current process lacks the appropriated privilege "
			"to access the semaphore." );
				break;
			case EACCES:
				return mx_error( MXE_TYPE_MISMATCH, fname,
			"The operation and the mode of the semaphore set "
			"do not match." );
			    	break;
			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected semctl() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
				break;
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
	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	struct sembuf sembuf_struct;
	int status, saved_errno;

	system_v_private = semaphore->semaphore_ptr;

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
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EACCES:
			return MXE_TYPE_MISMATCH;
			break;
		case EAGAIN:
			return MXE_MIGHT_CAUSE_DEADLOCK;
			break;
		case E2BIG:
			return MXE_WOULD_EXCEED_LIMIT;
			break;
		case EFBIG:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EIDRM:
			return MXE_NOT_FOUND;
			break;
		case EINTR:
			return MXE_INTERRUPTED;
			break;
		case ENOSPC:
			return MXE_OUT_OF_MEMORY;
			break;
		case ERANGE:
			return MXE_WOULD_EXCEED_LIMIT;
			break;
		default:
			return MXE_UNKNOWN_ERROR;
			break;
		}
	}
	return MXE_SUCCESS;
}

static long
mx_sysv_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	struct sembuf sembuf_struct;
	int status, saved_errno;

	system_v_private = semaphore->semaphore_ptr;

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
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EACCES:
			return MXE_TYPE_MISMATCH;
			break;
		case EAGAIN:
			return MXE_MIGHT_CAUSE_DEADLOCK;
			break;
		case E2BIG:
			return MXE_WOULD_EXCEED_LIMIT;
			break;
		case EFBIG:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EIDRM:
			return MXE_NOT_FOUND;
			break;
		case EINTR:
			return MXE_INTERRUPTED;
			break;
		case ENOSPC:
			return MXE_OUT_OF_MEMORY;
			break;
		case ERANGE:
			return MXE_WOULD_EXCEED_LIMIT;
			break;
		default:
			return MXE_UNKNOWN_ERROR;
			break;
		}
	}
	return MXE_SUCCESS;
}

static long
mx_sysv_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	struct sembuf sembuf_struct;
	int status, saved_errno;

	system_v_private = semaphore->semaphore_ptr;

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
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EACCES:
			return MXE_TYPE_MISMATCH;
			break;
		case EAGAIN:
			return MXE_NOT_AVAILABLE;
			break;
		case E2BIG:
			return MXE_WOULD_EXCEED_LIMIT;
			break;
		case EFBIG:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EIDRM:
			return MXE_NOT_FOUND;
			break;
		case EINTR:
			return MXE_INTERRUPTED;
			break;
		case ENOSPC:
			return MXE_OUT_OF_MEMORY;
			break;
		case ERANGE:
			return MXE_WOULD_EXCEED_LIMIT;
			break;
		default:
			return MXE_UNKNOWN_ERROR;
			break;
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
	int status, saved_errno, semaphore_value;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	system_v_private = semaphore->semaphore_ptr;

	if ( system_v_private == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	semaphore_value = semctl( system_v_private->semaphore_id, 0, GETVAL );

	if ( semaphore_value < 0 ) {
		saved_errno = errno;

		switch( status ) {
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected semctl() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
			break;
		}
	}

	*current_value = (unsigned long) semaphore_value;

	return MX_SUCCESSFUL_RESULT;
}

#endif    /* System V semaphores */

/*======================= Posix Pthreads ======================*/

#if defined(_POSIX_SEMAPHORES) || defined(OS_IRIX)

#include <fcntl.h>
#include <semaphore.h>

static mx_status_type
mx_posix_semaphore_create( MX_SEMAPHORE **semaphore,
			unsigned long initial_value,
			char *name )
{
	static const char fname[] = "mx_posix_semaphore_create()";

	sem_t *p_semaphore_ptr;
	int status, saved_errno;
	unsigned long sem_value_max;
	size_t name_length;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Allocate the data structures we need. */

	*semaphore = (MX_SEMAPHORE *) malloc( sizeof(MX_SEMAPHORE) );

	if ( *semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_SEMAPHORE structure." );
	}

	p_semaphore_ptr = (sem_t *) malloc( sizeof(sem_t) );

	if ( p_semaphore_ptr == (sem_t *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a sem_t structure." );
	}

	(*semaphore)->semaphore_ptr = p_semaphore_ptr;

	(*semaphore)->semaphore_type = MXT_SEM_POSIX;

	/* Is the initial value of the semaphore greater than the 
	 * maximum allowed value?
	 */
#if defined(OS_SOLARIS) || defined(OS_IRIX)
	sem_value_max = sysconf(_SC_SEM_VALUE_MAX);
#else
	sem_value_max = SEM_VALUE_MAX;
#endif

	if ( initial_value > sem_value_max ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested initial value (%lu) for the semaphore "
		"is larger than the maximum allowed value of %lu.",
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

		mx_strncpy( (*semaphore)->name, name, name_length );
	}

	/* Create the semaphore. */

	if ( (*semaphore)->name == NULL ) {

		/* Create an unnamed semaphore. */

		status = sem_init( p_semaphore_ptr, 0,
					(unsigned int) initial_value );

		if ( status != 0 ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case EINVAL:

				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested initial value %lu exceeds the maximum "
			"allowed value of %lu.", initial_value, sem_value_max );
				break;
			case ENOSPC:
				return mx_error( MXE_NOT_AVAILABLE, fname,
	    "A resource required to create the semaphore is not available.");
			    	break;
			case ENOSYS:
				return mx_error( MXE_UNSUPPORTED, fname,
		"Posix semaphores are not supported on this platform." );
				break;
			case EPERM:
				return mx_error( MXE_PERMISSION_DENIED, fname,
			"The current process lacks the appropriated privilege "
			"to create the semaphore." );
				break;
			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected sem_init() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
				break;
			}
		}

	} else {
		/* Create a named semaphore. */

		p_semaphore_ptr = sem_open( (*semaphore)->name,
					O_CREAT,
					0644,
					(unsigned int) initial_value );

		if ( p_semaphore_ptr == (sem_t *) SEM_FAILED ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case EACCES:
				return mx_error( MXE_PERMISSION_DENIED, fname,
				"Could not connect to semaphore '%s'.",
					(*semaphore)->name );
				break;
			case EEXIST:
				return mx_error( MXE_PERMISSION_DENIED, fname,
				"Semaphore '%s' already exists, but you "
				"requested exclusive access.",
					(*semaphore)->name );
				break;
			case EINTR:
				return mx_error( MXE_INTERRUPTED, fname,
			    "sem_open() for semaphore '%s' was interrupted.",
			    		(*semaphore)->name );
				break;
			case EINVAL:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Either sem_open() is not supported for "
				"semaphore '%s' or the requested initial "
				"value %lu was larger than the maximum (%lu).",
					(*semaphore)->name,
					initial_value,
					sem_value_max );
				break;
			case EMFILE:
				return mx_error( MXE_NOT_AVAILABLE, fname,
				"Ran out of semaphore or file descriptors "
				"for this process." );
				break;
			case ENAMETOOLONG:
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Semaphore name '%s' is longer than the "
				"maximum allowed length for this system.",
					(*semaphore)->name );
				break;
			case ENFILE:
				return mx_error( MXE_NOT_AVAILABLE, fname,
				"Too many semaphores are currently open "
				"on this computer." );
				break;
			case ENOENT:
				return mx_error( MXE_NOT_FOUND, fname,
				"Semaphore '%s' was not found and the call "
				"to sem_open() did not specify O_CREAT.",
					(*semaphore)->name );
				break;
			case ENOSPC:
				return mx_error( MXE_OUT_OF_MEMORY, fname,
				"There is insufficient space for the creation "
				"of the new named semaphore." );
				break;
			case ENOSYS:
				return mx_error( MXE_UNSUPPORTED, fname,
				"The function sem_open() is not supported "
				"on this platform." );
				break;
			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected sem_open() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
				break;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_posix_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_posix_semaphore_destroy()";

	sem_t *p_semaphore_ptr;
	int status, saved_errno;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	p_semaphore_ptr = semaphore->semaphore_ptr;

	if ( p_semaphore_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	if ( semaphore->name == NULL ) {
		/* Destroy an unnamed semaphore. */

		status = sem_destroy( p_semaphore_ptr );
	} else {
		/* Close a named semaphore. */

		status = sem_close( p_semaphore_ptr );
	}

	if ( status != 0 ) {
		saved_errno = errno;

		switch( status ) {
		case EBUSY:
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
				"Detected an attempt to destroy a semaphore "
				"that is locked or referenced by "
				"another thread." );
			break;
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The semaphore passed was not "
				"a valid semaphore." );
			break;
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"Posix semaphores are not supported on this platform." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
			break;
		}
	}

	if ( semaphore->name != NULL ) {
		mx_free( semaphore->name );
	}

	mx_free( p_semaphore_ptr );

	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

static long
mx_posix_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	sem_t *p_semaphore_ptr;
	int status, saved_errno;

	p_semaphore_ptr = semaphore->semaphore_ptr;

	if ( p_semaphore_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = sem_wait( p_semaphore_ptr );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EINTR:
			return MXE_INTERRUPTED;
			break;
		case ENOSYS:
			return MXE_UNSUPPORTED;
			break;
		case EDEADLK:
			return MXE_MIGHT_CAUSE_DEADLOCK;
			break;
		default:
			return MXE_UNKNOWN_ERROR;
			break;
		}
	}
	return MXE_SUCCESS;
}

static long
mx_posix_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	sem_t *p_semaphore_ptr;
	int status, saved_errno;

	p_semaphore_ptr = semaphore->semaphore_ptr;

	if ( p_semaphore_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = sem_post( p_semaphore_ptr );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case ENOSYS:
			return MXE_UNSUPPORTED;
			break;
		default:
			return MXE_UNKNOWN_ERROR;
			break;
		}
	}
	return MXE_SUCCESS;
}

static long
mx_posix_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	sem_t *p_semaphore_ptr;
	int status, saved_errno;

	p_semaphore_ptr = semaphore->semaphore_ptr;

	if ( p_semaphore_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = sem_trywait( p_semaphore_ptr );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
#if defined(OS_SOLARIS)
		/* Saw this value returned on Solaris 8.  (WML) */
		case 0:
#endif
		case EAGAIN:
			return MXE_NOT_AVAILABLE;
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EINTR:
			return MXE_INTERRUPTED;
			break;
		case ENOSYS:
			return MXE_UNSUPPORTED;
			break;
		case EDEADLK:
			return MXE_MIGHT_CAUSE_DEADLOCK;
			break;
		default:
#if 1
			MX_DEBUG(-2,
		("mx_semaphore_trylock(): errno = %d, error message = '%s'.",
					saved_errno, strerror(saved_errno) ));
#endif
			return MXE_UNKNOWN_ERROR;
			break;
		}
	}
	return MXE_SUCCESS;
}

static mx_status_type
mx_posix_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	sem_t *p_semaphore_ptr;
	int status, saved_errno, semaphore_value;

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	p_semaphore_ptr = semaphore->semaphore_ptr;

	if ( p_semaphore_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	status = sem_getvalue( p_semaphore_ptr, &semaphore_value );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( status ) {
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected sem_getvalue() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
			break;
		}
	}

	*current_value = (unsigned long) semaphore_value;

	return MX_SUCCESSFUL_RESULT;
}

#endif   /* Posix semaphores */

/*=============================================================*/

/* The following functions route semaphore calls to the correct
 * Posix or System V function depending on which kinds of semaphores
 * are supported on the current platform.
 */

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
		unsigned long initial_value,
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

/***************************************************************/

#elif 0

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
		unsigned long initial_value,
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


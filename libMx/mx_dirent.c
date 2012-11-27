/*
 * Name:    mx_dirent.c
 *
 * Purpose: Posix style directory stream functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2007, 2011-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_dirent.h"

/************************* Microsoft Windows *************************/

#if defined(OS_WIN32)

/* Typical POSIX/Unix/Linux errno values for opendir() are:
 *
 *   EACCES - Permission denied.
 *   ELOOP  - Too many symbolic links were encountered in resolving the name.
 *   EMFILE - Too many file descriptors in use by process.
 *   ENAMETOOLONG - The directory name is too long.
 *   ENFILE - Too many files are currently open in the system.
 *   ENOENT - Directory does not exist, or name is an empty string.
 *   ENOMEM - Insufficient memory to complete the operation.
 *   ENOTDIR - Name is not a directory.
 */

MX_EXPORT DIR *
opendir( const char *name )
{
	static const char fname[] = "opendir()";

	DIR *dir;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	errno = 0;

	/* See if the name pointer is NULL. */

	if ( name == NULL ) {
		errno = ENOENT;
		return NULL;
	}

	/* See if the name is of zero length. */

	if ( name[0] == '\0' ) {
		errno = ENOENT;
		return NULL;
	}

	/* Allocate a DIR structure to put the results in. */

	dir = malloc(sizeof(DIR));

	if ( dir == NULL ) {
		errno = ENOMEM;
		return NULL;
	}

	/* Save the directory name for use by rewinddir(). */

	strlcpy( dir->directory_name, name, sizeof(dir->directory_name) );

	/* Create a Win32 handle for the find search. */

	dir->find_handle = FindFirstFile( name, &(dir->find_data) );

	if ( dir->find_handle == INVALID_HANDLE_VALUE ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		switch( last_error_code ) {
		case ERROR_FILE_NOT_FOUND:
			mx_error( MXE_NOT_FOUND, fname,
			"The requested directory '%s' does not exist.", name );

			errno = ENOENT;
			break;

		default:
			mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while trying to use "
			"directory '%s'.  "
			"Win32 error code = %ld, error message = '%s'",
				name, last_error_code, message_buffer );

			errno = EIO;
			break;
		}

		free(dir);

		return NULL;
	}

	dir->file_number = 0;

	errno = 0;

	return dir;
}

/* Typical POSIX/Unix/Linux errno values for closedir() are:
 *
 *   EBADF - Invalid directory stream descriptor 'dir'.
 *   EINTR - The closedir() function was interrupted by a signal.
 */

MX_EXPORT int
closedir( DIR *dir )
{
	static const char fname[] = "closedir()";

	BOOL os_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	errno = 0;

	/* See if the dir pointer is NULL. */

	if ( dir == NULL ) {
		errno = EBADF;
		return (-1);
	}

	/* Close the file search handle. */

	os_status = FindClose( dir->find_handle );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		MX_DEBUG(-2,("closedir(): last_error_code = %ld",
				last_error_code));

		switch( last_error_code ) {
		default:
			mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occured while trying to close "
			"directory '%s'.  "
			"Win32 error code = %ld, error message = '%s'",
				dir->directory_name,
				last_error_code,
				message_buffer );

			errno = EIO;
			break;
		}

		free(dir);

		return (-1);
	}

	free(dir);

	errno = 0;

	return 0;
}

/* Typical POSIX/Unix/Linux errno values for readdir() are:
 *
 *   EBADF  - Invalid directory stream descriptor 'dir'.
 *   ENOENT - The current position of the directory stream is invalid.
 *   EOVERFLOW - One of the values in the structure cannot be represented
 *               correctly.
 */

MX_EXPORT struct dirent *
readdir( DIR *dir )
{
	static const char fname[] = "readdir()";

	static struct dirent entry;
	LPVOID message_buffer;
	BOOL os_status;
	DWORD last_error_code;

	errno = 0;

	memset( &entry, 0, sizeof(struct dirent) );

	/* See if the dir pointer is NULL. */

	if ( dir == NULL ) {
		errno = EBADF;
		return NULL;
	}

	/* Find the next entry, if needed. */

	if ( dir->file_number > 0 ) {
		os_status = FindNextFile( dir->find_handle, &(dir->find_data) );

		if ( os_status == 0 ) {
			last_error_code = GetLastError();

			if ( last_error_code == ERROR_NO_MORE_FILES ) {
				errno = 0;

				return NULL;
			}

			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				last_error_code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &message_buffer,
				0, NULL );

			mx_error( MXE_FILE_IO_ERROR, fname,
			"FindNextFile() failed with error %d: %s",
				last_error_code, message_buffer );

			mx_free( message_buffer );

			errno = EIO;

			return NULL;
		}
	}

	strlcpy(entry.d_name, dir->find_data.cFileName, sizeof(entry.d_name));

	dir->file_number++;

	errno = 0;

	return &entry;
}

/* rewinddir() is not defined to return any error codes, even though
 * it _can_ fail.
 */

MX_EXPORT void
rewinddir( DIR *dir )
{
	/* See if the dir pointer is NULL. */

	if ( dir == NULL ) {
		return;
	}

	(void) FindClose( dir->find_handle );

	dir->find_handle = FindFirstFile( dir->directory_name,
					&(dir->find_data) );

	dir->file_number = 0;

	return;
}

#endif


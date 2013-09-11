/*
 * Name:    mx_dirent.c
 *
 * Purpose: Posix style directory stream functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2007, 2011-2013 Illinois Institute of Technology
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
	size_t original_length;
	char *name_copy;
	size_t name_copy_length;

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

	/*------------------------------------------------------------------*/

	/* Fun Facts for Inquiring Minds Episode 257.
	 *
	 * Hi Kids!  Were you aware that the MSDN entry for FindFirstFile()
	 * contains this pithy statement?
	 *
	 *   An attempt to open a search with a trailing backlash always fails.
	 *
	 * So what does this mean for all of you kids out there in Internet
	 * Land?  Well it means that starting a search for D:\wml using
	 * FindFirstFile() will succeed, but a search using D:\wml\
	 * will fail horribly.  Don't believe me?  Go try it yourself.
	 * I'll wait.
	 *
	 *   ...
	 *
	 * So did you see it?  FindFirstFile() on D:\wml\ returned 
	 * INVALID_HANDLE_VALUE!  Wow, that's a surprise.
	 *
	 * So what do we do about it?  Well the most obvious fix would be
	 * to just zap the trailing backslash with a null character.  That
	 * turns something like D:\wml\ into D:\wml, which we already know
	 * works.  However, ... you might want to try that trick again with
	 * a directory name like D:\.  I'll wait until you do.
	 *
	 *   ...
	 *
	 * So ... You got INVALID_HANDLE_VALUE, didn't you.  Well, the zap
	 * trick turned D:\ into D:, but plain driver names like A:, B:,
	 * C:, D:, etc. are not accepted by FindFirstFile(), no way, no how.
	 *
	 * So what do we do?  As it happens, Microsoft does have some more
	 * verbiage about this:
	 *
	 *   As stated previously, you cannot use a trailing backslash (\) in
	 *   the lpFileName input string for FindFirstFile, therefore it may
	 *   not be obvious how to search root directories. If you want to see
	 *   files or get the attributes of a root directory, the following
	 *   options would apply:
	 *
	 *     To examine files in a root directory, you can use "C:\*" and
	 *     step through the directory by using FindNextFile.
	 *
	 *     To get the attributes of a root directory, use the
	 *     GetFileAttributes function.
	 *
	 * In that case, the fix is to look and see if there is a trailing
	 * backslash and append a '*' character after it.  Easy enough.
	 * And for a bare drive name like D:, we just append two characters,
	 * namely, '\' and '*'.  Wow, that was easy too!
	 *
	 * Oh well.  I can see by the clock that we are out of time now,
	 * so that's it for this episode of "Fun Facts for Inquiring Minds".
	 * Bye Kids!
	 */

	/*------------------------------------------------------------------*/

	/* Allocate a DIR structure to put the results in. */

	dir = malloc(sizeof(DIR));

	if ( dir == NULL ) {
		errno = ENOMEM;
		return NULL;
	}

	/* Save the directory name for use by rewinddir(). */

	strlcpy( dir->directory_name, name, sizeof(dir->directory_name) );

	/* Do we need to append anything to the directory name?
	 *
	 * (See FFIM episode transcript above).
	 */

	original_length = strlen( dir->directory_name );

	name_copy_length = original_length + 4;

	name_copy = malloc( name_copy_length );

	if ( name_copy == NULL ) {
		errno = ENOMEM;
		return NULL;
	}

	strlcpy( name_copy, dir->directory_name, name_copy_length );

	if ( ( name_copy[original_length-1] == '\\' )
	  || ( name_copy[original_length-1] == '/' ) )
	{

		/* Append a '*' after any trailing backslashes. */

		strlcat( name_copy, "*", name_copy_length );
	} else
	if ( original_length == 2 ) {
		if ( name_copy[1] == ':' ) {

			/* Append "\*" after any bare drive names. */

			strlcat( name_copy, "\\*", name_copy_length );
		}
	}

	/* Create a Win32 handle for the find search. */

	dir->find_handle = FindFirstFile( name_copy, &(dir->find_data) );

	mx_free( name_copy );

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


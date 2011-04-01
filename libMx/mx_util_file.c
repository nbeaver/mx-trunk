/*
 * Name:    mx_util_file.c
 *
 * Purpose: File related utility functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined( OS_WIN32 )
#include <windows.h>
#include <direct.h>
#endif

#if 0

#if defined( OS_UNIX ) || defined( OS_CYGWIN )
#include <sys/time.h>
#include <pwd.h>
#endif

#endif

#include "mx_osdef.h"

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_stdint.h"
#include "mx_cfn.h"

/*-------------------------------------------------------------------------*/

#define COPY_FILE_CLEANUP \
		do {                                           \
			if ( existing_fd >= 0 ) {              \
				(void) close( existing_fd );   \
			}                                      \
			if ( new_fd >= 0 ) {                   \
				(void) close( new_fd );        \
			}                                      \
			if ( buffer != (char *) NULL ) {       \
				mx_free( buffer );             \
			}                                      \
		} while(0)

MX_EXPORT mx_status_type
mx_copy_file( char *existing_filename, char *new_filename, int new_file_mode )
{
	static const char fname[] = "mx_copy_file()";

	int existing_fd, new_fd, saved_errno;
	struct stat stat_struct;
	unsigned long existing_file_size, new_file_size, file_blocksize;
	unsigned long bytes_already_written, bytes_to_write;
	long bytes_read, bytes_written;
	char *buffer;

	existing_fd = -1;
	new_fd      = -1;
	buffer      = NULL;

	/* Open the existing file. */

	existing_fd = open( existing_filename, O_RDONLY, 0 );

	if ( existing_fd < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The file '%s' could not be opened.  Reason = '%s'",
			existing_filename, strerror( saved_errno ) );
	}

	/* Get information about the existing file. */

	if ( fstat( existing_fd, &stat_struct ) != 0 ) {
		saved_errno = errno;

		COPY_FILE_CLEANUP;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Could not fstat() the file '%s'.  Reason = '%s'",
			existing_filename, strerror( saved_errno ) );
	}

	existing_file_size = (unsigned long) stat_struct.st_size;

#if defined( OS_WIN32 )
	file_blocksize = 4096;	/* FIXME: This is just a guess. */

#elif defined( OS_VMS )
	file_blocksize = 512;	/* FIXME: Is this correct for all VMS disks? */

#elif defined( OS_ECOS )
	file_blocksize = 512;	/* FIXME: This is just a guess. */
#else
	file_blocksize = stat_struct.st_blksize;
#endif

	/* Allocate a buffer to read the old file contents into. */

	buffer = (char *) malloc( file_blocksize * sizeof( char ) );

	if ( buffer == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an %lu byte file copy buffer.",
			(unsigned long) (file_blocksize * sizeof(char)) );
	}

	/* Open the new file. */

	new_fd = open( new_filename, O_WRONLY | O_CREAT, new_file_mode );

	if ( new_fd < 0 ) {
		saved_errno = errno;

		COPY_FILE_CLEANUP;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"New file '%s' could not be opened.  Reason = '%s'",
			new_filename, strerror( saved_errno ) );
	}

	/* Copy the contents of the existing file to the new file. */

	new_file_size = 0;

	for (;;) {
		bytes_read = read( existing_fd, buffer, file_blocksize );

		if ( bytes_read < 0 ) {
			saved_errno = errno;

			COPY_FILE_CLEANUP;

			return mx_error( MXE_FILE_IO_ERROR, fname,
	"An error occurred while reading the existing file '%s', so the "
	"new file '%s' is probably incomplete.  Error = '%s'.",
				existing_filename, new_filename,
				strerror( saved_errno ) );
		} else
		if ( bytes_read == 0 ) {
			/* This means end of file. */

			COPY_FILE_CLEANUP;

			break;	/* Exit the while() loop. */
		}

		bytes_already_written = 0;

		while ( bytes_already_written < bytes_read ) {

			bytes_to_write = bytes_read - bytes_already_written;

			bytes_written = write( new_fd,
				    &buffer[ bytes_already_written ],
				    bytes_to_write );

			if ( bytes_written < 0 ) {
				saved_errno = errno;

				COPY_FILE_CLEANUP;

				return mx_error( MXE_FILE_IO_ERROR, fname,
	"An error occurred while writing the new file '%s', so the "
	"new file is probably incomplete.  Error = '%s'",
					new_filename,
					strerror( saved_errno ) );
			} else
			if ( bytes_written == 0 ) {
				return mx_error( MXE_FILE_IO_ERROR, fname,
	"An attempt to write %lu bytes to the new file '%s' "
	"resulted in 0 bytes being written.  This should not happen, "
	"but it is not obvious why it happened.  Please report this as a bug.",
					bytes_to_write,
					new_filename );
			} else
			if ( bytes_written > bytes_to_write ) {
				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
	"Somehow the number of bytes written %ld to file '%s' "
	"is greater than the number "
	"of bytes (%lu) that we asked to write.  This should not happen, "
	"but it is not obvious why it happened.  Please report this as a bug.",
					bytes_written,
					new_filename,
					bytes_to_write );
			}

			bytes_already_written += bytes_written;
		}

		new_file_size += bytes_read;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#if defined( OS_WIN32 ) && !defined( __BORLANDC__ )
#define getcwd _getcwd
#endif

MX_EXPORT mx_status_type
mx_get_current_directory_name( char *filename_buffer,
				size_t max_filename_length )
{
	static const char fname[] = "mx_get_current_directory_name()";

	char *ptr;
	int saved_errno;

	/* Some versions of getcwd() will malloc() a buffer if filename_buffer
	 * is NULL.  However, since mx_get_current_directory_name() is 
	 * prototyped to return an mx_status_type structure, we cannot permit
	 * mallocing() of a buffer since there is no way to return the buffer
	 * pointer to the caller.
	 */

	if ( filename_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename_buffer pointer passed was NULL." );
	}

	ptr = getcwd( filename_buffer, max_filename_length );

	if ( ptr == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while trying to get the name of the current "
		"directory.  Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

/* mx_change_filename_prefix() is intended for use in cases where a given
 * system can access a file under two different names and it is necessary
 * to convert between the two names.  For example, suppose that a client
 * program running on host 'ad_client' is communicating with an area
 * detector server running on host 'ad_server'.  Also suppose, that area
 * detector images are saved on 'ad_server' in the directory /data/lavender
 * which is NFS or SMB exported to 'ad_client' as /mnt/ad_server/lavender.
 * In this case, mx_change_filename_prefix() can be used to strip off the
 * leading /data string at the start of the filename and replace it with
 * /mnt/ad_server or vice versa.
 */

MX_EXPORT mx_status_type
mx_change_filename_prefix( char *old_filename,
			char *old_filename_prefix,
			char *new_filename_prefix,
			char *new_filename,
			size_t max_new_filename_length )
{
	static const char fname[] = "mx_change_filename_prefix()";

	char *old_filename_ptr;
	size_t old_prefix_length, new_prefix_length;
	char end_char;

	if ( old_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The old_filename pointer passed was NULL." );
	}
	if ( new_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The new_filename pointer passed was NULL." );
	}

	/* If present, strip off the old prefix. */

	if ( old_filename_prefix == NULL ) {
		old_filename_ptr = old_filename;
	} else {
		old_prefix_length = strlen(old_filename_prefix);

		if ( old_prefix_length == 0 ) {
			old_filename_ptr = old_filename;
		} else {
			/* Does the old prefix match the start of the
			 * old filename?
			 */

			if ( strncmp( old_filename_prefix, old_filename,
					old_prefix_length ) != 0 )
			{
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The old filename prefix '%s' does not match "
				"the start of the old filename '%s'.",
					old_filename_prefix,
					old_filename );
			} else {
				old_filename_ptr =
					old_filename + old_prefix_length;
			}
		}
	}

	/* Skip over any leading '/' character. */

	if ( *old_filename_ptr == '/' ) {
		old_filename_ptr++;
	}

	/* Now create the new filename. */

	if ( new_filename_prefix == NULL ) {
		strlcpy( new_filename, old_filename_ptr,
				max_new_filename_length );
	} else {
		new_prefix_length = strlen(new_filename_prefix);

		if ( new_prefix_length == 0 ) {
			strlcpy( new_filename, old_filename_ptr,
					max_new_filename_length );
		} else {
			/* Does the new prefix have a path separator '/'
			 * at the end of the filename?
			 */

			end_char = new_filename_prefix[ new_prefix_length - 1 ];

			if (end_char == '/') {
				/* If the filename already contains a path
				 * separator, then we can just concatenate
				 * the strings as is.
				 */

				snprintf( new_filename, max_new_filename_length,
					"%s%s", new_filename_prefix,
					old_filename_ptr );
			} else {
				/* Otherwise, we must insert a path separator.*/

				snprintf( new_filename, max_new_filename_length,
					"%s/%s", new_filename_prefix,
					old_filename_ptr );
			}
		}
	}

#if 0
	MX_DEBUG(-2,("%s: old_filename = '%s'", fname, old_filename));
	MX_DEBUG(-2,("%s: old_filename_prefix = '%s'",
					fname, old_filename_prefix));
	MX_DEBUG(-2,("%s: new_filename_prefix = '%s'",
					fname, new_filename_prefix));
	MX_DEBUG(-2,("%s: new_filename = '%s'", fname, new_filename));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#if defined(OS_UNIX) || defined(OS_WIN32)

MX_EXPORT int
mx_command_found( char *command_name )
{
	static const char fname[] = "mx_command_found()";

	char pathname[MXU_FILENAME_LENGTH+1];
	char *path, *start_ptr, *end_ptr;
	int os_status;
	size_t length;
	mx_bool_type try_pathname;

#if defined(OS_WIN32)
	char path_separator = ';';
#else
	char path_separator = ':';
#endif

	if ( command_name == NULL )
		return FALSE;

	/* See first if the file can be treated as an absolute
	 * or relative pathname.
	 */

	try_pathname = FALSE;

	if ( strchr( command_name, '/' ) != NULL ) {
		try_pathname = TRUE;
	}

#if defined(OS_WIN32)
	if ( strchr( command_name, '\\' ) != NULL ) {
		try_pathname = TRUE;
	}
#endif

	/* If the supplied command name appears to be a relative or absolute
	 * pathname, try using access() to see if the file exists and is
	 * executable.
	 */

	if ( try_pathname ) {
		os_status = access( command_name, X_OK );

		if ( os_status == 0 ) {
			return TRUE;
		} else {
			return FALSE;
		}
	}

	/* If we get here, look for the command in the directories listed
	 * by the PATH variable.
	 */

	path = getenv( "PATH" );

	MX_DEBUG( 2,("%s: path = '%s'", fname, path));

	/* Loop through the path components. */

	start_ptr = path;

	for(;;) {
		/* Look for the end of the next path component. */

		end_ptr = strchr( start_ptr, path_separator );

		if ( end_ptr == NULL ) {
			length = strlen( start_ptr );
		} else {
			length = end_ptr - start_ptr;
		}

		/* If the next path component is longer than the
		 * maximum filename length, skip it.
		 */

		if ( length > MXU_FILENAME_LENGTH ) {
			start_ptr = end_ptr + 1;

			continue;  /* Go back to the top of the for(;;) loop. */
		}

		/* Copy the path directory to the pathname buffer
		 * and then null terminate it.
		 */

		memset( pathname, '\0', sizeof(pathname) );

		memcpy( pathname, start_ptr, length );

		pathname[length] = '\0';

		/* Append a directory separator to the filename.  Forward
		 * slashes work here just as well on Windows as backslashes.
		 */

		strlcat( pathname, "/", sizeof(pathname) );

		/* Append the command name. */

		strlcat( pathname, command_name, sizeof(pathname) );

		/* See if this pathname exists and is executable. */

		os_status = access( pathname, X_OK );

		MX_DEBUG( 2,("%s: pathname = '%s', os_status = %d",
				fname, pathname, os_status));

		if ( os_status == 0 ) {

			/* If the returned status is 0, we have found the
			 * command and know that it is executable, so we
			 * can return now with a success status code.
			 */

			return TRUE;
		}

		if ( end_ptr == NULL ) {
			break;		/* Exit the for(;;) loop. */
		} else {
			start_ptr = end_ptr + 1;
		}
	}

	return FALSE;
}

#else

MX_EXPORT int
mx_command_found( char *command_name )
{
	return FALSE;
}

#endif

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_verify_directory( char *directory_name, int create_flag )
{
	static const char fname[] = "mx_verify_directory()";

	struct stat stat_buf;
	int os_status, saved_errno;

	if ( directory_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The directory name pointer passed was NULL." );
	}

	/* Does a filesystem object with this name already exist? */

	os_status = access( directory_name, F_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		if ( saved_errno != ENOENT ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while testing for the presence of "
			"directory '%s'.  "
			"Errno = %d, error message = '%s'",
				directory_name,
				saved_errno, strerror( saved_errno ) );
		}

		if ( create_flag == FALSE ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"The directory named '%s' does not exist.",
				directory_name );
		}

		/* The directory does not already exist, so create it. */

		os_status = mkdir( directory_name, 0777 );

		if ( os_status == 0 ) {
			/* Creating the directory succeeded, so we are done! */

			return MX_SUCCESSFUL_RESULT;
		} else {
			/* Creating the directory failed, so report the error.*/

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Creating subdirectory '%s' failed.  "
			"Errno = %d, error message = '%s'",
				directory_name,
				saved_errno, strerror(saved_errno) );
		}
	}

	/*------------*/

	/* A filesystem object with this name already exists, so 
	 * we must find out more about it.
	 */

	os_status = stat( directory_name, &stat_buf );

	if ( os_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"stat() failed for file '%s'.  "
		"Errno = %d, error message = '%s'",
			directory_name,
			saved_errno, strerror(saved_errno) );
	}

	/* Is the object a directory? */

#if defined(OS_WIN32)
	if ( (stat_buf.st_mode & _S_IFDIR) == 0 ) {
#else
	if ( S_ISDIR(stat_buf.st_mode) == 0 ) {
#endif
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Existing file '%s' is not a directory.",
			directory_name );
	}

	/* Although we just did stat(), determining the access permissions
	 * is more portably done with access().
	 */

	os_status = access( directory_name, R_OK | W_OK | X_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		if ( saved_errno == EACCES ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"We do not have read, write, and execute permission "
			"for directory '%s'.", directory_name );
		}

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Checking the access permissions for directory '%s' "
		"failed.  Errno = %d, error message = '%s'",
			directory_name,
			saved_errno, strerror( saved_errno ) );
	}

	/* If we get here, the directory already exists and is useable. */

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#if 1

MX_EXPORT mx_status_type
mx_canonicalize_filename( char *original_filename,
			char *canonical_filename,
			size_t max_canonical_filename_length )
{
	static const char fname[] = "mx_canonicalize_filename()";

	char *new_filename;

	new_filename = mx_normalize_filename( original_filename,
					canonical_filename,
					max_canonical_filename_length );

	if ( new_filename == NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"The attempt to canonicalize the filename '%s' failed.",
			original_filename );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

#elif defined(OS_WIN32)

/* FIXME: This version dies with an access violation while returning. */

static mx_bool_type mx_canonicalize_filename_is_initialized = FALSE;

static HINSTANCE hinst_shlwapi;

typedef BOOL (*PathCanonicalize_type)( char *, char * );

static PathCanonicalize_type ptrPathCanonicalize = NULL;

MX_EXPORT mx_status_type
mx_canonicalize_filename( char *original_filename,
			char *canonical_filename,
			size_t max_canonical_filename_length )
{
	static const char fname[] = "mx_canonicalize_filename()";

	char local_canonical_filename[MAX_PATH];
	BOOL os_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( mx_canonicalize_filename_is_initialized == FALSE ) {

		mx_canonicalize_filename_is_initialized = TRUE;

		hinst_shlwapi = LoadLibrary( TEXT("shlwapi.dll") );

		if ( hinst_shlwapi == NULL ) {
			ptrPathCanonicalize = NULL;
		} else {
			ptrPathCanonicalize = (PathCanonicalize_type)
					GetProcAddress( hinst_shlwapi,
					TEXT("PathCanonicalizeA") );
		}
	}

	if ( ptrPathCanonicalize == NULL ) {

		/* For very old versions of Windows that do not have
		 * PathCanonicalize() , just copy the unmodified
		 * filename through.
		 */

		strlcpy( canonical_filename, original_filename,
					sizeof(canonical_filename ) );
	} else {
		os_status = ptrPathCanonicalize( local_canonical_filename,
							original_filename );

		if ( os_status == FALSE ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to canonicalize the path '%s'.  "
			"Win32 error code = %ld, error message = '%s'.",
			original_filename, last_error_code, message_buffer );
		}
	}

	strlcpy( canonical_filename, local_canonical_filename,
					max_canonical_filename_length );

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_UNIX) || defined(OS_CYGWIN)

MX_EXPORT mx_status_type
mx_canonicalize_filename( char *original_filename,
			char *canonical_filename,
			size_t max_canonical_filename_length )
{
	static const char fname[] = "mx_canonicalize_filename()";

	char *local_canonical_filename, *ptr;
	long max_path_length;
	int saved_errno;

#if defined(PATH_MAX)
	max_path_length = PATH_MAX;
#else
	max_path_length = pathconf( xxx, _PC_PATH_MAX );

	if ( max_path_length <= 0 ) {
		max_path_length = 4096;
	}
#endif

	local_canonical_filename = malloc( max_path_length );

	if ( local_canonical_filename == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"canonical filename buffer.", max_path_length );
	}

	ptr = realpath( original_filename, local_canonical_filename );

	if ( ptr == NULL ) {
		mx_free( local_canonical_filename );

		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to canonicalize the filename '%s'.  "
		"Errno = %d, error message = '%s'.",
			original_filename, saved_errno, strerror(saved_errno) );
	}
	

	strlcpy( canonical_filename, local_canonical_filename,
					max_canonical_filename_length );

	mx_free( local_canonical_filename );

	return MX_SUCCESSFUL_RESULT;
}

#else
#error mx_canonicalize_filename() has not been defined for this platform.
#endif

/*-------------------------------------------------------------------------*/

#if defined(OS_VXWORKS)

/* If the current platform does not have an access() function, then we
 * implement one using stat().
 */

int
access( char *pathname, int mode )
{
	struct stat status_struct;
	mode_t file_mode;
	int status;

	status = stat( pathname, &status_struct );

	if ( status != 0 )
		return status;

	/* If we are just checking to see if the file exists and
	 * stat() succeeded, then the file must exist and we can
	 * return now.
	 */

	if ( mode == F_OK )
		return 0;

	/* Select out the bits from the st_mode field that we need
	 * to compare with the 'mode' argument to this function.
	 */

#if defined(OS_VXWORKS)
	/* Only look at the world permissions. */

	file_mode = 0x7 & status_struct.st_mode;
#else
	if ( getuid() == status_struct.st_uid ) {

		/* We need to check the owner permissions. */

		file_mode = 0x7 & ( status_struct.st_mode >> 8 );

	} else if ( getgid() == status_struct.st_gid ) {

		/* We need to check the group permissions. */

		file_mode = 0x7 & ( status_struct.st_mode >> 4 );

	} else {

		/* We need to check the world permissions. */

		file_mode = 0x7 & status_struct.st_mode;
	}
#endif

	if ( file_mode == mode ) {

		/* Success: the modes match. */

		return 0;
	} else {
		/* The modes do _not_ match. */

		errno = EACCES;

		return -1;
	}
}

#endif

/*-------------------------------------------------------------------------*/

#if defined(OS_VXWORKS) || defined(OS_WIN32)

/* Some platforms define mkdir() differently or not at all, so we provide
 * our own front end here.
 */

/* WARNING: This code must be after any calls to mkdir() in this file
 * due to the #undef that follows.
 */

#ifdef mkdir
#undef mkdir
#endif

#if defined(__BORLANDC__)
#include <dir.h>
#endif

MX_EXPORT int
mx_mkdir( char *pathname, mode_t mode )
{
	int os_status;

#if defined(OS_WIN32)
	os_status = _mkdir( pathname );
#else
	os_status = mkdir( pathname );
#endif

	return os_status;
}

#endif


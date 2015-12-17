/*
 * Name:    mx_util_file.c
 *
 * Purpose: File related utility functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2011, 2013-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_DEBUG_DIRECTORY_HIERARCHY	FALSE

#define MX_DEBUG_FIND_FILE_IN_PATH	FALSE

/* On Linux, we must define _GNU_SOURCE before including any C library header
 * in order to get splice() from fcntl.h 
 */

#if defined(OS_LINUX)
#  define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined( OS_WIN32 )
#include <windows.h>
#include <winioctl.h>
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
#include "mx_io.h"
#include "mx_dynamic_library.h"

/*-------------------------------------------------------------------------*/

#if defined(OS_WIN32)

static HINSTANCE mxp_hinst_shlwapi = NULL;

static mx_bool_type mxp_shlwapi_tested_for = FALSE;

static HINSTANCE
mxp_get_shlwapi_hinstance( void )
{
	if ( mxp_hinst_shlwapi != NULL )
		return mxp_hinst_shlwapi;

	if ( mxp_shlwapi_tested_for )
		return NULL;

	mxp_hinst_shlwapi = LoadLibrary( TEXT("shlwapi.dll") );

	mxp_shlwapi_tested_for = TRUE;

	return mxp_hinst_shlwapi;
}

#endif /* defined(OS_WIN32) */

/*=========================================================================*/

#if defined(OS_WIN32) 

MX_EXPORT mx_status_type
mx_copy_file( char *existing_filename,
		char *new_filename,
		int new_file_mode,
		unsigned long copy_flags )
{
	static const char fname[] = "mx_copy_file()";

	BOOL fail_if_exists;
	BOOL os_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	mx_status_type mx_status;

#if 0
	MX_DEBUG(-2,("%s: mx_copy_file( '%s', '%s', %#lx )",
		fname, existing_filename, new_filename, copy_flags));
#endif

	/*---*/

	if ( copy_flags & MXF_CP_USE_CLASSIC_COPY ) {
		mx_status = mx_copy_file_classic( existing_filename,
						new_filename,
						new_file_mode );
		return mx_status;
	}

	/*---*/

	if ( copy_flags & MXF_CP_OVERWRITE ) {
		fail_if_exists = FALSE;
	} else {
		fail_if_exists = TRUE;
	}

	/* We use CopyFile() rather than CopyFileEx() here since CopyFile()
	 * exists for all versions of Win32.
	 */

	os_status = CopyFile( existing_filename, new_filename, fail_if_exists );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		switch( last_error_code ) {
		case ERROR_ACCESS_DENIED:
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"You do not have the permissions required "
			"to copy file '%s' to file '%s'.",
				existing_filename, new_filename );
			break;
		default:
			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to copy file '%s' to '%s'.  "
			"Win32 error code = %ld, error message = '%s'.",
				existing_filename, new_filename,
				last_error_code, message_buffer );
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif ( defined(OS_LINUX) && \
	( (MX_GLIBC_VERSION >= 2005000L) || defined(MX_MUSL_VERSION) ) )

/* If Linux and Glibc are new enough, use splice() to do the copy. */

MX_EXPORT mx_status_type
mx_copy_file( char *existing_filename,
		char *new_filename,
		int new_file_mode,
		unsigned long copy_flags )
{
	static const char fname[] = "mx_copy_file()";

	int existing_file_fd, new_file_fd;
	int pipe_fd[2];
	ssize_t bytes_read, bytes_written;
	int os_status, saved_errno;
	size_t buffer_size;
	unsigned long linux_version;
	mx_status_type mx_status;

	/*---------------------------------------------------------------*/

	/* Check to see if the kernel we are running supports splice(). */

	linux_version = mx_get_os_version_number();

	if ( linux_version < 2006017L ) {
		mx_status = mx_copy_file_classic( existing_filename,
						new_filename,
						new_file_mode );
		return mx_status;
	}

	/*---------------------------------------------------------------*/

	/* Check to see if the caller requested the classic copy function. */

	if ( copy_flags & MXF_CP_USE_CLASSIC_COPY ) {
		mx_status = mx_copy_file_classic( existing_filename,
						new_filename,
						new_file_mode );
		return mx_status;
	}

	/*---------------------------------------------------------------*/

	/* If we get here, we use splice(). */

	/* First, create a pipe, since splice() uses the pipe buffer
	 * for in-kernel data transfer.
	 */

	os_status = pipe( pipe_fd );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An attempt to create a pipe for use by splice() failed.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	/* Create file descriptors for the existing and new files. */

	existing_file_fd = open( existing_filename, O_RDONLY );

	if ( existing_file_fd < 0 ) {
		saved_errno = errno;

		close( pipe_fd[0] );
		close( pipe_fd[1] );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An attempt to create a file descriptor for reading from "
		"existing file '%s' failed.  Errno = %d, error message = '%s'",
		existing_filename, saved_errno, strerror(saved_errno) );
	}

	new_file_fd = open( new_filename, O_WRONLY | O_CREAT, new_file_mode );

	if ( new_file_fd < 0 ) {
		saved_errno = errno;

		close( existing_file_fd );
		close( pipe_fd[0] );
		close( pipe_fd[1] );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An attempt to create a file descriptor for writing to "
		"new file '%s' failed.  Errno = %d, error message = '%s'",
		new_filename, saved_errno, strerror(saved_errno) );
	}

	/* Now copy the file contents using splice(). */

	mx_status = MX_SUCCESSFUL_RESULT;

	buffer_size = 16384;	/* FIXME: Where should this number come from? */

	while(1) {
		/* Read from the existing file into the kernel pipe buffer. */

		bytes_read = splice( existing_file_fd, NULL, pipe_fd[1], NULL,
					buffer_size, 0 );

		if ( bytes_read == 0 ) {
			/* We have reached the end of the file, so it is
			 * time to break out of the while() loop.
			 */
			break;
		} else
		if ( bytes_read < 0 ) {
			saved_errno = errno;

			mx_status = mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while reading from existing "
			"file '%s' into a kernel pipe buffer.  "
			"Errno = %d, error message = '%s'",
			existing_filename, saved_errno, strerror(saved_errno) );

			break;	/* Exit the while() loop. */
		}

		/* Write the kernel pipe buffer contents to the new file. */

		bytes_written = splice( pipe_fd[0], NULL, new_file_fd, NULL,
					bytes_read, 0 );

		if ( bytes_written < 0 ) {
			saved_errno = errno;

			mx_status = mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while writing from a kernel pipe "
			"buffer into new file '%s'.  "
			"Errno = %d, error message = '%s'",
			new_filename, saved_errno, strerror(saved_errno) );

			break;	/* Exit the while() loop. */
		}
	}

	close( existing_file_fd );
	close( pipe_fd[0] );
	close( pipe_fd[1] );
	close( new_file_fd );

	return mx_status;
}

#elif ( defined(OS_MACOSX) && (MX_DARWIN_VERSION >= 9000000L) )

#include <copyfile.h>

MX_EXPORT mx_status_type
mx_copy_file( char *existing_filename,
		char *new_filename,
		int new_file_mode,
		unsigned long copy_flags )
{
	static const char fname[] = "mx_copy_file()";

	int os_status, saved_errno;
	mx_status_type mx_status;

	/* Check to see if the caller requested the classic copy function. */

	if ( copy_flags & MXF_CP_USE_CLASSIC_COPY ) {
		mx_status = mx_copy_file_classic( existing_filename,
						new_filename,
						new_file_mode );
		return mx_status;
	}

	os_status = copyfile( existing_filename, new_filename,
					NULL, COPYFILE_ALL );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to copy file '%s' to file '%s' failed.  "
		"Errno = %d, error message = '%s'",
			existing_filename, new_filename,
			saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#else /* mx_copy_file() */

MX_EXPORT mx_status_type
mx_copy_file( char *existing_filename,
		char *new_filename,
		int new_file_mode,
		unsigned long copy_flags )
{
	mx_status_type mx_status;

	mx_status = mx_copy_file_classic( existing_filename,
					new_filename,
					new_file_mode );

	return mx_status;
}

#endif /* mx_copy_file() */

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
mx_copy_file_classic( char *existing_filename,
			char *new_filename,
			int new_file_mode )
{
	static const char fname[] = "mx_copy_file_classic()";

	int existing_fd, new_fd, saved_errno;
	struct stat stat_struct;
	unsigned long existing_file_size, new_file_size, file_blocksize;
	unsigned long bytes_already_written, bytes_to_write;
	long bytes_read, bytes_written;
	char *buffer;

	/*---*/

	existing_fd = -1;
	new_fd      = -1;
	buffer      = NULL;

	/* Open the existing file. */

	existing_fd = open( existing_filename, O_RDONLY, 0 );

	if ( existing_fd < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The file '%s' could not be opened.  "
		"Errno = %d, error message = '%s'",
			existing_filename,
			saved_errno, strerror( saved_errno ) );
	}

	/* Get information about the existing file. */

	if ( fstat( existing_fd, &stat_struct ) != 0 ) {
		saved_errno = errno;

		COPY_FILE_CLEANUP;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Could not fstat() the file '%s'.  "
		"Errno = %d, error message = '%s'",
			existing_filename,
			saved_errno, strerror( saved_errno ) );
	}

	existing_file_size = (unsigned long) stat_struct.st_size;

	MXW_UNUSED( existing_file_size );

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
		"New file '%s' could not be opened.  "
		"Errno = %d, error message = '%s'",
			new_filename,
			saved_errno, strerror( saved_errno ) );
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
	"new file '%s' is probably incomplete.  "
	"Errno = %d, error message = '%s'.",
				existing_filename, new_filename,
				saved_errno, strerror( saved_errno ) );
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
	"new file is probably incomplete.  "
	"Errno = %d, error message = '%s'",
					new_filename,
					saved_errno, strerror( saved_errno ) );
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

/*=========================================================================*/

MX_EXPORT mx_status_type
mx_get_num_lines_in_file( char *filename, size_t *num_lines_in_file )
{
	static const char fname[] = "mx_get_num_lines_in_file()";

	int fd, saved_errno;
	char buffer[1000];
	size_t num_lines;
	ssize_t i, bytes_read;
	mx_bool_type have_read_a_partial_line;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}
	if ( num_lines_in_file == (size_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_lines_in_file pointer passed was NULL." );
	}

	/* Open the file. */

#if defined(OS_VXWORKS)
	fd = open( filename, O_RDONLY, 0 );
#else
	fd = open( filename, O_RDONLY );
#endif

	if ( fd < 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case ENOENT:
			return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
			"File '%s' was not found.", filename );
			break;
		default:
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to open file '%s'.  "
			"Errno = %d, error message = '%s'",
				filename, saved_errno, strerror(saved_errno) );
			break;
		}
	}

	/* Now figure out how many lines are currently in this file.
	 *
	 * WARNING: If the file is being written to more rapidly than
	 * we can read from it, then this function may never return!
	 */

	num_lines = 0;
	have_read_a_partial_line = FALSE;

	while (1) {
		bytes_read = read( fd, buffer, sizeof(buffer) );

		if ( bytes_read < 0 ) {
			saved_errno = errno;

			close(fd);

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Read from file '%s' failed.  "
			"Errno = %d, error_message = '%s'",
			filename, saved_errno, strerror(saved_errno) );
		} else
		if ( bytes_read == 0 ) {
			/* We are at the end of the file.  We test for the
			 * possibility that there was an incomplete final
			 * line in the data.
			 */

			if ( have_read_a_partial_line ) {
				num_lines++;
			}

			*num_lines_in_file = num_lines;

			close(fd);

			return MX_SUCCESSFUL_RESULT;
		} else {
			/* We read some bytes from the file.  Look for
			 * newlines in the data we just read.
			 */

			for ( i = 0; i < bytes_read; i++ ) {
				if ( buffer[i] == '\n' ) {
					num_lines++;
				}
			}
		}
	}

	MXW_NOT_REACHED( return MX_SUCCESSFUL_RESULT );
}

/*=========================================================================*/

MX_EXPORT mx_status_type
mx_skip_num_lines_in_file( FILE *file, size_t num_lines_to_skip )
{
	static const char fname[] = "mx_skip_num_lines_in_file()";

	char buffer[1000];
	size_t num_lines, line_length;
	mx_bool_type have_read_a_partial_line;

	if ( file == (FILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The FILE pointer passed was NULL." );
	}

	num_lines = 0;
	have_read_a_partial_line = FALSE;

	while (1) {
		fgets( buffer, sizeof(buffer), file );

		if ( feof(file) ) {
			if ( have_read_a_partial_line ) {
				num_lines++;
			}
			if ( num_lines >= num_lines_to_skip ) {
				return MX_SUCCESSFUL_RESULT;
			} else {
			}
		} else
		if ( ferror(file) ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An I/O error occured while skipping line %lu "
			"in the file.", (unsigned long) num_lines );
		}

		line_length = strlen( buffer );

		if ( buffer[line_length - 1] == '\n' ) {
			/* If there is a newline at the end of the line,
			 * then we have finished reading the current line.
			 */

			have_read_a_partial_line = FALSE;
			num_lines++;
		} else {
			have_read_a_partial_line = TRUE;
		}

		if ( num_lines >= num_lines_to_skip ) {
			return MX_SUCCESSFUL_RESULT;
		}
	}

	MXW_NOT_REACHED( return MX_SUCCESSFUL_RESULT );
}

/*=========================================================================*/

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

/*=========================================================================*/

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
mx_change_filename_prefix( const char *old_filename,
			const char *old_filename_prefix,
			const char *new_filename_prefix,
			char *new_filename,
			size_t max_new_filename_length )
{
	static const char fname[] = "mx_change_filename_prefix()";

	const char *old_filename_ptr;
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

/*=========================================================================*/

MX_EXPORT mx_status_type
mx_construct_file_name_from_file_pattern( char *filename_buffer,
					size_t filename_buffer_size,
					const char filename_pattern_char,
					unsigned long file_number,
					const char *filename_pattern )
{
	static const char fname[] =
		"mx_construct_file_name_from_file_pattern()";

	char *start_of_varying_number, *trailing_segment;
	long length_of_varying_number;
	long length_of_leading_segment;
	char filename_pattern_string[2];
	char file_number_string[200];
	char format[20];

#if 0
	MX_DEBUG(-2,("%s: filename_pattern_char = '%c'",
				fname, filename_pattern_char));
	MX_DEBUG(-2,("%s: file_number = %lu", fname, file_number));
	MX_DEBUG(-2,("%s: filename_pattern = '%s'", fname, filename_pattern));
#endif

	if ( filename_buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename_buffer pointer passed was NULL." );
	}
	if ( filename_buffer_size == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The specified filename buffer size is 0." );
	}
	if ( filename_pattern == (const char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The file_pattern pointer passed was NULL." );
	}

	filename_pattern_string[0] = filename_pattern_char;
	filename_pattern_string[1] = '\0';

	/* Look for the start of the varying number in the filename pattern. */

	start_of_varying_number = strchr( filename_pattern,
					filename_pattern_char );

	/* If there is no varying part, then copy the pattern to the filename
	 * and then return.
	 */

	if ( start_of_varying_number == NULL ) {
		strlcpy( filename_buffer, filename_pattern,
				filename_buffer_size );
#if 0
		MX_DEBUG(-2,("%s: Using filename pattern as the filename '%s'.",
			fname, filename_buffer ));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	length_of_leading_segment =
		start_of_varying_number - filename_pattern + 1;

	/* How long is the varying number? */

	length_of_varying_number = strspn( start_of_varying_number,
						filename_pattern_string );

	if ( length_of_varying_number <= 0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The length of the varying number is %lu in the "
		"filename pattern '%s', even though the the "
		"filename pattern char '%c' was found.  "
		"It should be impossible for this to happen.",
			length_of_varying_number,
			filename_pattern,
			filename_pattern_char );
	}

	/* Construct the new varying part of the string. */

	snprintf( format, sizeof(format),
		"%%0%lulu", length_of_varying_number );

	snprintf( file_number_string, sizeof(file_number_string),
		format, file_number );

	/* Protect against buffer overflows. */

	if ( length_of_leading_segment > filename_buffer_size ) {
		mx_warning( "The filename for file number %lu and "
		"file pattern '%s' will be truncated, since it does not "
		"fit into the provided buffer.",
			file_number, filename_pattern );

		length_of_leading_segment = filename_buffer_size;
	}

	/* Construct the new filename. */

	strlcpy( filename_buffer, filename_pattern, length_of_leading_segment );

	strlcat( filename_buffer, file_number_string, filename_buffer_size );

	trailing_segment = start_of_varying_number + length_of_varying_number;

	strlcat( filename_buffer, trailing_segment, filename_buffer_size );

#if 0
	MX_DEBUG(-2,("%s: filename_buffer = '%s'", fname, filename_buffer));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*=========================================================================*/

MX_EXPORT mx_status_type
mx_make_directory_hierarchy( char *directory_name )
{
	static const char fname[] = "mx_make_directory_hierarchy()";

	char *copy_of_directory_name = NULL;
	char *next_directory_level_ptr = NULL;
	char *slash_ptr = NULL;
	char name_to_test[ MXU_FILENAME_LENGTH+1 ];
	struct stat stat_struct;
	int os_status, saved_errno;
	size_t chars_to_copy, chars_used, strlcat_limit;
	mx_status_type mx_status;

	enum {
		searching_for_directory = 1,
		creating_directory
	} search_state_enum;

	search_state_enum = searching_for_directory;

	/* Make a copy of the directory name, so that we can modify it. */

	copy_of_directory_name = strdup( directory_name );

	if ( copy_of_directory_name == (const char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The directory name pointer passed was NULL." );
	}

#if defined(OS_WIN32)
	/* For Windows, convert all backslashes to forward slashes. */

	{
		size_t i, directory_name_length;

		directory_name_length = strlen( copy_of_directory_name );

		for ( i = 0; i < directory_name_length; i++ ) {
			if ( copy_of_directory_name[i] == '\\' ) {
				copy_of_directory_name[i] = '/';
			}
		}
	}
#endif /* OS_WIN32 */

#if MX_DEBUG_DIRECTORY_HIERARCHY
	MX_DEBUG(-2,("%s: directory name = '%s'",
		fname, copy_of_directory_name));
#endif

	/* Does this directory already exist and is it writeable? */

	os_status = access( copy_of_directory_name, R_OK | W_OK | X_OK );

#if MX_DEBUG_DIRECTORY_HIERARCHY
	MX_DEBUG(-2,("%s: access() returned %d", fname, os_status));
#endif

	if ( os_status == 0 ) {

#if MX_DEBUG_DIRECTORY_HIERARCHY
		MX_DEBUG(-2,("%s: Directory '%s' is already set up correctly.",
			fname, copy_of_directory_name ));
#endif
		mx_free( copy_of_directory_name );

		return MX_SUCCESSFUL_RESULT;
	}

	saved_errno = errno;

#if MX_DEBUG_DIRECTORY_HIERARCHY
	MX_DEBUG(-2,("%s: Directory '%s' is _not_ yet set up correctly.  "
			"Errno = %d, error message = '%s'",
			fname, copy_of_directory_name,
			saved_errno, strerror(saved_errno) ));
#endif

	switch( saved_errno ) {
	case EACCES:
		mx_status = mx_error( MXE_PERMISSION_DENIED, fname,
		"MX does not have permission to create new files "
		"in the directory '%s'.",
			copy_of_directory_name );

		mx_free( copy_of_directory_name );

		return mx_status;
		break;
	default:
		break;
	}

	/* If we get here, then we have been instructed by the flag variable
	 * to attempt to create the directory that was requested by the user.
	 */

#if defined(OS_WIN32)
	/* If we are running on Microsoft Windows, then we must check for
	 * the optional presence and validity of a drive letter at the
	 * start of the directory name.
	 */

	if ( copy_of_directory_name[1] == ':' ) {
		UINT drive_type;

		/* Copy the first two characters (the drive name) to a
		 * test buffer.  Note that a length of 3 is specified
		 * to leave room for the null byte string terminator.
		 */

		strlcpy( name_to_test, copy_of_directory_name, 3 );

		/* We can check for drive validity by trying to get
		 * the drive type.  Append a backslash character,
		 * since GetDriveType() will fail without it.
		 */

		strlcat( name_to_test, "\\", sizeof(name_to_test) );

#if MX_DEBUG_DIRECTORY_HIERARCHY
		MX_DEBUG(-2,("%s: drive name = '%s'", fname, name_to_test));
#endif

		drive_type = GetDriveType( name_to_test );

#if MX_DEBUG_DIRECTORY_HIERARCHY
		MX_DEBUG(-2,("%s: drive_type = %u", fname, drive_type));
#endif

		switch( drive_type ) {
		case DRIVE_UNKNOWN:
		case DRIVE_NO_ROOT_DIR:
			mx_status = mx_error( MXE_NOT_FOUND, fname,
			"Drive '%s' specified in directory name '%s' "
			"is not a valid drive letter for this computer.",
				name_to_test, copy_of_directory_name );

			mx_free( copy_of_directory_name );

			return mx_status;
			break;
		default:
			break;
		}

		/* Strip off the backslash we added, since we are beyond
		 * the point where we need it.
		 */

		name_to_test[2] = '\0';

		/* Initialize next_directory_level_ptr to point to the
		 * character immediately after the drive name.
		 */

		next_directory_level_ptr = copy_of_directory_name + 2;
	} else
	if ( (copy_of_directory_name[0] == '/')
	  && (copy_of_directory_name[1] == '/') )
	{

#if 1
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Windows network share support is not yet implemented "
		"for directory name '%s'.", copy_of_directory_name );

#else  /* Win32 share support */

		NET_API_STATUS net_api_status;

		/* If this is a network share name, then we need to
		 * verify that it is valid.
		 */

		/* NetBIOS server names are limited to 15 characters
		 * and we are still stuck with that in SMB.
		 */

		char server_name[16];

		/* Share names are similarly limited, but the actual
		 * limit is 14 characters for it.
		 */

		char share_name[15];

		mx_warning("%s: FIXME: This particular code path has not "
		"yet been tested adequately.  copy_of_directory_name = '%s'",
			fname, copy_of_directory_name);

		/* Extract the server name from 'copy_of_directory_name'. */

		start_ptr = copy_of_directory_name + 2;

		slash_ptr = strchr( start_ptr, '/' );

		if ( slash_ptr == (char *) NULL ) {
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,

			"The requested directory name '%s' "
			"appears to have a server name, but not a share name.",
				copy_of_directory_name );

			mx_free( copy_of_directory_name );

			return mx_status;
		}

		chars_to_copy = slash_ptr - start_ptr + 1;

		if ( chars_to_copy > sizeof( server_name ) ) {
			chars_to_copy = sizeof( server_name );
		}

		strlcpy( server_name, start_ptr, chars_to_copy );

		/* The share name should start at the character after
		 * the slash character.
		 */

		start_ptr = slash_ptr + 1;

		slash_ptr = strchr( start_ptr, '/' );

		if ( slash_ptr == (char *) NULL ) {
			chars_to_copy = strlen( start_ptr ) + 1;

			next_directory_level_ptr = NULL;
		} else {
			chars_to_copy = slash_ptr - start_ptr + 1;

			next_directory_level_ptr = slash_ptr;
		}

		if ( chars_to_copy > sizeof( share_name ) ) {
			chars_to_copy = sizeof( share_name );
		}

		strlcpy( share_name, start_ptr, chars_to_copy );

#if MX_DEBUG_DIRECTORY_HIERARCHY
		MX_DEBUG(-2,("%s: server_name = '%s', share_name = '%s'",
			fname, server_name, share_name ));
#endif

		/* Verify that this share exists. */

		net_api_status = NetShareGetInfo( server_name, share_name,
						1, &share_info_1_struct );
#endif  /* Have Win32 share support */

	}
#endif  /* OS_WIN32 */

#if MX_DEBUG_DIRECTORY_HIERARCHY
	MX_DEBUG(-2,("%s: (before loop) name_to_test = '%s'",
		fname, name_to_test));

	MX_DEBUG(-2,("%s: (before loop) next_directory_level_ptr = '%s'",
		fname, next_directory_level_ptr));
#endif

	/* Work through the components of the directory pathname to see
	 * if we find a directory that either does not exist or has
	 * permissions that prevent us from using the directory.
	 */

	search_state_enum = searching_for_directory;

	while ( next_directory_level_ptr != (char *) NULL ) {

#if MX_DEBUG_DIRECTORY_HIERARCHY
		MX_DEBUG(-2,("%s: next_directory_level_ptr = '%s'",
			fname, next_directory_level_ptr));
#endif

		slash_ptr = strchr( next_directory_level_ptr + 1, '/' );

#if MX_DEBUG_DIRECTORY_HIERARCHY
		MX_DEBUG(-2,("%s: slash_ptr = '%s'", fname, slash_ptr));
#endif

		if ( slash_ptr == (char *) NULL ) {
			strlcat( name_to_test, next_directory_level_ptr,
						sizeof(name_to_test) );

			next_directory_level_ptr = NULL;
		} else {
			chars_to_copy =
				slash_ptr - next_directory_level_ptr + 1;

#if MX_DEBUG_DIRECTORY_HIERARCHY
			MX_DEBUG(-2,("%s: chars_to_copy = %ld",
				fname, (long) chars_to_copy));
#endif

			chars_used = strlen( name_to_test );

#if MX_DEBUG_DIRECTORY_HIERARCHY
			MX_DEBUG(-2,("%s: chars_used = %ld",
					fname, (long) chars_used ));
#endif
			strlcat_limit = chars_to_copy + chars_used;

			if ( strlcat_limit > sizeof(name_to_test) ) {
				strlcat_limit = sizeof(name_to_test);
			}

#if MX_DEBUG_DIRECTORY_HIERARCHY
			MX_DEBUG(-2,("%s: strlcat_limit = %ld",
				fname, (long) strlcat_limit));
#endif

			strlcat( name_to_test, next_directory_level_ptr,
						strlcat_limit );

			next_directory_level_ptr = slash_ptr;
		}

#if MX_DEBUG_DIRECTORY_HIERARCHY
		MX_DEBUG(-2,("%s: name_to_test = '%s'", fname, name_to_test ));

		MX_DEBUG(-2,("%s: #1 search_state_enum = %d",
				fname, (int) search_state_enum ));
#endif

		if ( search_state_enum == searching_for_directory ) {

			/* We are still searching for the first directory
			 * component that either does not exist or is
			 * not useable.
			 */

			os_status = stat( name_to_test, &stat_struct );

#if MX_DEBUG_DIRECTORY_HIERARCHY
			MX_DEBUG(-2,("%s: stat( '%s' ) = %d",
				fname, name_to_test, os_status ));
#endif
			if ( os_status < 0 ) {
				saved_errno = errno;

#if MX_DEBUG_DIRECTORY_HIERARCHY
				MX_DEBUG(-2,
				("%s: saved_errno = %d, error message = '%s'",
					fname, saved_errno,
					strerror(saved_errno) ));
#endif
				switch( saved_errno ) {
				case ENOENT:
					search_state_enum = creating_directory;
					break;
				default:
					return mx_error(MXE_FILE_IO_ERROR,fname,
					"Cannot get file status for "
					"directory '%s'.  "
					"Errno = %d, error message = '%s'",
						name_to_test,
					saved_errno, strerror( saved_errno ) );
	
					break;
				}
			}

			if ( search_state_enum == searching_for_directory ) {

				if ( ( stat_struct.st_mode & S_IFDIR ) == 0 ) {
					return mx_error(
					MXE_ILLEGAL_ARGUMENT, fname,
					"Directory component '%s' in "
					"pathname '%s' is not a directory.",
						directory_name, name_to_test );
				}
			}
		}

#if MX_DEBUG_DIRECTORY_HIERARCHY
		MX_DEBUG(-2,("%s: #2 search_state_enum = %d",
				fname, (int) search_state_enum ));
#endif
		switch( search_state_enum ) {
		case searching_for_directory:
			break;

		case creating_directory:

			/* We have found a directory component in the
			 * pathname that does not exist and are now
			 * trying to create it or the directories
			 * underneath it.
			 */

#if MX_DEBUG_DIRECTORY_HIERARCHY
			MX_DEBUG(-2,("%s: mkdir '%s'", fname, name_to_test));
#endif

			os_status = mkdir( name_to_test, R_OK | W_OK );

			if ( os_status < 0 ) {
				saved_errno = errno;

				return mx_error( MXE_FILE_IO_ERROR, fname,
				"The attempt to create directory '%s' as "
				"part of pathname '%s' failed.  "
				"Errno = %d, error message = '%s'.",
					name_to_test, directory_name,
					saved_errno, strerror(saved_errno) );
			}
			break;

		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Somehow search_state_enum is in state %d "
			"which should not be able to exist.",
				(int) search_state_enum );
			break;
		}

#if MX_DEBUG_DIRECTORY_HIERARCHY
		MX_DEBUG(-2,("%s: #3 search_state_enum = %d",
				fname, (int) search_state_enum ));
#endif
	}

	if ( 0 ) {
		/* In principle, we should not be able to get here, since
		 * the status of the directory was tested at the beginning
		 * of this routine with access().
		 */

		mx_free( copy_of_directory_name );

		return MX_SUCCESSFUL_RESULT;
	}

	/* If the directory at some level of a pathname was not found,
	 * then we must create that directory and all of the necessary
	 * directories underneath it.
	 */

	

	mx_free( copy_of_directory_name );

	return MX_SUCCESSFUL_RESULT;
}


/*=========================================================================*/

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

/*=========================================================================*/

#define FREE_FIND_FILE_DATA \
	do { \
		mx_free( argv ); \
		mx_free( path_variable_copy ); \
		mx_free( original_filename_with_extension ); \
		mx_free( pathname ); \
		mx_free( pathname_with_extension ); \
		mx_free( path_variable_copy ); \
	} while(0)

MX_EXPORT mx_status_type
mx_find_file_in_path( const char *original_filename,
			char *full_filename,
			size_t full_filename_length,
			const char *path_variable_name,
			const char *extension,
			int file_access_mode,
			unsigned long flags,
			int *match_found )
{
	static const char fname[] = "mx_find_file_in_path()";

	mx_bool_type look_in_current_directory;
	mx_bool_type try_without_extension;

	mx_bool_type ignore_case;
	mx_bool_type already_has_extension;

	int access_status, split_status;

	const char *filename_to_use = NULL;
	const char *separator_ptr = NULL;
	const char *path_variable_string = NULL;

	char path_env_separator[2];
	char *path_variable_copy = NULL;
	int i, argc;
	char **argv = NULL;

	char *original_filename_with_extension = NULL;

	char *pathname = NULL;
	char *pathname_with_extension = NULL;

	if ( original_filename == (const char *) NULL ) {
		return mx_error( MXE_QUIET | MXE_NULL_ARGUMENT, fname,
		"The original filename pointer passed was NULL." );
	}
	if ( full_filename == (char *) NULL ) {
		return mx_error( MXE_QUIET | MXE_NULL_ARGUMENT, fname,
		"The full filename pointer passed was NULL." );
	}
	if ( match_found == (int *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The match found pointer passed was NULL" );
	}

	*match_found = FALSE;

	look_in_current_directory = flags & MXF_FPATH_LOOK_IN_CURRENT_DIRECTORY;

	try_without_extension = flags & MXF_FPATH_TRY_WITHOUT_EXTENSION;

#if ( defined(OS_WIN32) || defined(OS_DOS) )
	ignore_case = TRUE;
#else
	ignore_case = FALSE;
#endif
	/*---------------------------------------------------------------*/

	original_filename_with_extension = malloc( MXU_FILENAME_LENGTH + 1 );

	if ( original_filename_with_extension == NULL ) {
	    return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an array to contain "
		"the original filename with an extension." );
	}

	/* Does the filename already have the extension attached? */

	if ( extension == (const char *) NULL ) {
	    already_has_extension = TRUE;
	} else {
	    const char *extension_ptr = NULL;
	    size_t original_length = strlen( original_filename );
	    size_t extension_length = strlen( extension );

	    if ( extension_length > original_length ) {
		already_has_extension = FALSE;
	    } else {
		extension_ptr = original_filename
				+ ( original_length - extension_length );

		if ( ignore_case ) {
		    if ( mx_strcasecmp( extension_ptr, extension ) == 0 ) {
			already_has_extension = TRUE;
		    } else {
			already_has_extension = FALSE;
		    }
		} else {
		    if ( strcmp( extension_ptr, extension ) == 0 ) {
			already_has_extension = TRUE;
		    } else {
			already_has_extension = FALSE;
		    }
		}
	    }
	}

	strlcpy( original_filename_with_extension,
			original_filename, MXU_FILENAME_LENGTH );

	if ( already_has_extension == FALSE ) {
		strlcat( original_filename_with_extension,
			extension, MXU_FILENAME_LENGTH );
	}

	/*---------------------------------------------------------------*/

	/* Allocate some memory to contain the filenames to test
	 * with access().
	 */

	pathname_with_extension = (char *) malloc( MXU_FILENAME_LENGTH + 1 );

	if ( pathname_with_extension == (char *) NULL ) {
	    FREE_FIND_FILE_DATA;

	    return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to allocate a buffer to contain "
	    "a test pathname with extension." );
	}

	pathname = (char *) malloc( MXU_FILENAME_LENGTH + 1 );

	if ( pathname == (char *) NULL ) {
	    FREE_FIND_FILE_DATA;

	    return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to allocate a buffer to contain "
	    "a test pathname." );
	}

	/*---------------------------------------------------------------*/

	/* Does the filename start with a path separator? */

#if ( defined(OS_WIN32) || defined(OS_DOS) )
	if ( ( original_filename[0] == '\\' )
	  || ( original_filename[0] == '/' ) )
#else
	if ( original_filename[0] == '/' )
#endif
	{
	    access_status = access( original_filename_with_extension,
							file_access_mode );

#if MX_DEBUG_FIND_FILE_IN_PATH
	    MX_DEBUG(-2,("%s: access( '%s', %d ) = %d", fname,
			original_filename_with_extension,
			file_access_mode, access_status ));
#endif

	    if ( access_status == 0 ) {
		filename_to_use = original_filename_with_extension;
	    } else {
		if ( try_without_extension && (already_has_extension==FALSE) )
		{
		    access_status = access( original_filename,
						file_access_mode );

#if MX_DEBUG_FIND_FILE_IN_PATH
		    MX_DEBUG(-2,("%s: access( '%s', %d ) = %d", fname,
			original_filename,
			file_access_mode, access_status ));
#endif

		    if ( access_status == 0 ) {
			filename_to_use = original_filename;
		    }
		}
	    }
	}

	if ( filename_to_use != (const char *) NULL ) {
	    *match_found = TRUE;

	    strlcpy( full_filename, filename_to_use, full_filename_length );

	    FREE_FIND_FILE_DATA;

	    return MX_SUCCESSFUL_RESULT;
	}

	/*---------------------------------------------------------------*/

	/* Is this a relative pathname that contains a path separator? */

#if ( defined(OS_WIN32) || defined(OS_DOS) )
	separator_ptr = strchr( original_filename_with_extension, '\\' );

	if ( separator_ptr == NULL ) {
	    separator_ptr = strchr( original_filename_with_extension, '/' );
	}
#else
	separator_ptr = strchr( original_filename_with_extension, '/' );
#endif
	if ( separator_ptr ) {
	    access_status = access( original_filename_with_extension,
						file_access_mode );

#if MX_DEBUG_FIND_FILE_IN_PATH
	    MX_DEBUG(-2,("%s: access( '%s', %d ) = %d", fname,
			original_filename_with_extension,
			file_access_mode, access_status ));
#endif

	    if ( access_status == 0 ) {
		filename_to_use = original_filename_with_extension;
	    } else {
		if ( try_without_extension && (already_has_extension==FALSE) )
		{
		    access_status = access( original_filename,
						file_access_mode );

#if MX_DEBUG_FIND_FILE_IN_PATH
		    MX_DEBUG(-2,("%s: access( '%s', %d ) = %d", fname,
			original_filename,
			file_access_mode, access_status ));
#endif

		    if ( access_status == 0 ) {
			filename_to_use = original_filename;
		    }
		}
	    }
	}

	if ( filename_to_use != (const char *) NULL ) {
	    *match_found = TRUE;

	    strlcpy( full_filename, filename_to_use, full_filename_length );

	    FREE_FIND_FILE_DATA;

	    return MX_SUCCESSFUL_RESULT;
	}

	/*---------------------------------------------------------------*/

	/* If we get here, we have a filename that does not contain a
	 * directory name.
	 */

	/* If a path environment variable was specified, then read its value. */

	if ( path_variable_name != (const char *) NULL ) {

	    path_variable_string = getenv( path_variable_name );

#if MX_DEBUG_FIND_FILE_IN_PATH
	    MX_DEBUG(-2,("%s: getenv('%s') = '%s'",
		fname, path_variable_name, path_variable_string));
#endif

	    if ( path_variable_string != (const char *) NULL ) {

		/* Split the path environment variable up. */

		path_variable_copy = strdup( path_variable_string );

#if ( defined(OS_WIN32) || defined(OS_DOS) )
		strlcpy( path_env_separator, ";", sizeof(path_env_separator) );
#else
		strlcpy( path_env_separator, ":", sizeof(path_env_separator) );
#endif

		split_status = mx_string_split( path_variable_copy,
						path_env_separator,
						&argc, &argv );

		if ( split_status != 0 ) {
		    FREE_FIND_FILE_DATA;

		    return mx_error( MXE_OUT_OF_MEMORY, fname,
		    "Ran out of memory trying to split up a copy of the "
		    "environment string '%s'", path_variable_name );
		}

		for ( i = 0; i < argc; i++ ) {
		    strlcpy( pathname, argv[i], MXU_FILENAME_LENGTH );

#if ( defined(OS_WIN32) || defined(OS_DOS) )
		    strlcat( pathname, "\\", MXU_FILENAME_LENGTH );
#else
		    strlcat( pathname, "/", MXU_FILENAME_LENGTH );
#endif

		    strlcat( pathname, original_filename, MXU_FILENAME_LENGTH );

		    strlcpy( pathname_with_extension,
				pathname, MXU_FILENAME_LENGTH );
		    strlcat( pathname_with_extension,
				extension, MXU_FILENAME_LENGTH );

		    access_status = access( pathname_with_extension,
							file_access_mode );

#if MX_DEBUG_FIND_FILE_IN_PATH
		    MX_DEBUG(-2,("%s: (%d) access( '%s', %d ) = %d", fname,
			i, pathname_with_extension,
			file_access_mode, access_status ));
#endif

		    if ( access_status == 0 ) {
			filename_to_use = pathname_with_extension;
		    } else {
			if ( try_without_extension
			  && (already_has_extension == FALSE) )
			{
			    access_status = access( pathname,
							file_access_mode );

#if MX_DEBUG_FIND_FILE_IN_PATH
			    MX_DEBUG(-2,("%s: (%d) access( '%s', %d ) = %d",
				fname, i, pathname,
				file_access_mode, access_status ));
#endif

			    if ( access_status == 0 ) {
				filename_to_use = pathname;
			    }
			}
		    }

		    if ( filename_to_use != (const char *) NULL ) {
			*match_found = TRUE;

			strlcpy( full_filename, filename_to_use,
					full_filename_length );

			FREE_FIND_FILE_DATA;

			return MX_SUCCESSFUL_RESULT;
		    }
		}
	    }
	}

	/*---------------------------------------------------------------*/

	/* If we get here, the tests for absolute and relative filenames,
	 * and filenames from the path have either failed or been skipped.
	 *
	 * At this point the only thing left to try is to look in the
	 * current directory, if requested.
	 */

	if ( look_in_current_directory ) {
	    access_status = access( original_filename_with_extension,
							file_access_mode );

#if MX_DEBUG_FIND_FILE_IN_PATH
	    MX_DEBUG(-2,("%s: access( '%s', %d ) = %d", fname,
			original_filename_with_extension,
			file_access_mode, access_status ));
#endif

	    if ( access_status == 0 ) {
		filename_to_use = original_filename_with_extension;
	    } else {
		if ( try_without_extension && (already_has_extension==FALSE) )
		{
		    access_status = access( original_filename,
							file_access_mode );

#if MX_DEBUG_FIND_FILE_IN_PATH
		    MX_DEBUG(-2,("%s: access( '%s', %d ) = %d", fname,
			original_filename,
			file_access_mode, access_status ));
#endif

		    if ( access_status == 0 ) {
			filename_to_use = original_filename;
		    }
		}
	    }

	    if ( filename_to_use != (const char *) NULL ) {
		*match_found = TRUE;

		strlcpy( full_filename, filename_to_use, full_filename_length );

		FREE_FIND_FILE_DATA;

		return MX_SUCCESSFUL_RESULT;
	    }
	}

	/*---------------------------------------------------------------*/

	FREE_FIND_FILE_DATA;

	return MX_SUCCESSFUL_RESULT;
}

/*=========================================================================*/

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

/*=========================================================================*/

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
	HINSTANCE hinst_shlwapi;

	if ( mx_canonicalize_filename_is_initialized == FALSE ) {

		mx_canonicalize_filename_is_initialized = TRUE;

		hinst_shlwapi = mxp_get_shlwapi_hinstance();

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

/*=========================================================================*/

#if defined(OS_WIN32)

typedef int (*mxp_PathGetDriveNumber_type)( LPCTSTR );
typedef int (*mxp_PathBuildRoot_type)( LPCTSTR, int );

MX_EXPORT mx_status_type
mx_get_filesystem_root_name( char *filename,
				char *fs_root_name,
				size_t max_fs_root_name_length )
{
	static const char fname[] = "mx_get_filesystem_root_name()";

	static mx_bool_type shlwapi_tested_for = FALSE;
	static mx_bool_type have_shlwapi_path_functions = FALSE;

	static mxp_PathGetDriveNumber_type pPathGetDriveNumber = NULL;
	static mxp_PathBuildRoot_type pPathBuildRoot = NULL;

	HINSTANCE hinst_shlwapi;
	char *source_fs_root_name_ptr, *source_colon_ptr;
	int drive_number;
	DWORD last_error_code;
	TCHAR error_message[200];
	mx_status_type mx_status;

	if ( filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}
	if ( fs_root_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filesystem root name pointer passed was NULL." );
	}
	if ( max_fs_root_name_length < 4 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The specified length (%ld) of the filesystem root name buffer "
		"is shorter than the minimum value of 4 for Windows.",
			max_fs_root_name_length );
	}

	/* Check to see if SHLWAPI.DLL is loaded. */

	if ( shlwapi_tested_for == FALSE ) {
		hinst_shlwapi = mxp_get_shlwapi_hinstance();

		if ( hinst_shlwapi == NULL ) {
			have_shlwapi_path_functions = FALSE;
		} else {
			have_shlwapi_path_functions = TRUE;

			pPathGetDriveNumber = (mxp_PathGetDriveNumber_type)
				GetProcAddress( hinst_shlwapi,
					TEXT("PathGetDriveNumber") );

			if ( pPathGetDriveNumber == NULL ) {
				have_shlwapi_path_functions = FALSE;
			} else {
				pPathBuildRoot = (mxp_PathBuildRoot_type)
					GetProcAddress( hinst_shlwapi,
						TEXT("PathBuildRoot") );

				if ( pPathBuildRoot == NULL ) {
					have_shlwapi_path_functions = FALSE;
				}
			}
		}

		shlwapi_tested_for = TRUE;
	}

	/* Now get the name of the filesystem root for the file
	 * that we are checking.
	 */

	if ( have_shlwapi_path_functions ) {

		drive_number = pPathGetDriveNumber( filename );

		if ( drive_number == -1 ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				error_message, sizeof(error_message) );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The attempt to get the drive number for file '%s' "
			"failed.  Win32 error code = %ld, error message = '%s'",
				filename, last_error_code, error_message );
		}

		fs_root_name[0] = '\0';

		pPathBuildRoot( fs_root_name, drive_number );

		if ( fs_root_name[0] == '\0' ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				error_message, sizeof(error_message) );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The attempt to find the root directory name "
			"for file '%s' failed.  "
			"Win32 error code = %ld, error message = '%s'",
				filename, last_error_code, error_message );
		}
	} else {
		/* No shlwapi.dll path functions. */

		char *filename_dup = strdup( filename );

		if ( filename_dup == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a duplicate "
			"memory buffer for string '%s'.", filename );
		}

		/* Find the first colon ':' character in the string. */

		source_colon_ptr = strchr( filename_dup, ':' );

		if ( source_colon_ptr == NULL ) {
			mx_free( filename_dup );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Filename '%s' does not have a drive letter in it.",
				filename_dup );
		}

		/* Null terminate after the colon. */

		source_colon_ptr[1] = '\0';

		/* See if the character before the colon corresponds to
		 * a valid drive letter.
		 *
		 * Warning: This logic will probably only work for
		 *          the 'C' locale.
		 */

		source_fs_root_name_ptr = source_colon_ptr - 1;

		if ( isupper(*source_fs_root_name_ptr) ) {
			*source_fs_root_name_ptr =
				tolower( *source_fs_root_name_ptr );
		} else
		if ( islower(*source_fs_root_name_ptr) ) {

			/* We do nothing in this case. */
		} else {
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The name '%s' found in filename '%s' is not "
				"a valid drive name.",
					fs_root_name, filename );

			mx_free( filename_dup );

			return mx_status;
		}

		strlcpy( fs_root_name,
			source_fs_root_name_ptr,
			max_fs_root_name_length );

		mx_free( filename_dup );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif

/*=========================================================================*/

#if defined(OS_WIN32)

typedef int (*mxp_PathGetDriveNumber_type)( LPCTSTR );
typedef int (*mxp_PathBuildRoot_type)( LPCTSTR, int );

#define MXP_MXF_FST_CDROM	999

MX_EXPORT mx_status_type
mx_get_filesystem_type( char *filename,
			unsigned long *filesystem_type )
{
	static const char fname[] = "mx_get_filesystem_type()";

	UINT drive_type;
	DWORD last_error_code;
	char drive_root_name[ MXU_FILENAME_LENGTH+1 ];
	char error_message[200];
	mx_status_type mx_status;

	if ( filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}
	if ( filesystem_type == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filesystem type pointer passed was NULL." );
	}

	/* Get a first approximation to the filesystem type.
	 * This approximation will be refined later on.
	 */

	mx_status = mx_get_filesystem_root_name( filename,
						drive_root_name,
						sizeof(drive_root_name) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	drive_type = GetDriveType( drive_root_name );

	switch( drive_type ) {
	case DRIVE_UNKNOWN:
	case DRIVE_NO_ROOT_DIR:
		*filesystem_type = MXF_FST_NOT_FOUND;
		break;

	case DRIVE_REMOVABLE:
	case DRIVE_FIXED:
	case DRIVE_RAMDISK:
		*filesystem_type = MXF_FST_LOCAL;
		break;

	case DRIVE_REMOTE:
		*filesystem_type = MXF_FST_REMOTE;
		break;

	case DRIVE_CDROM:
		*filesystem_type = MXP_MXF_FST_CDROM;
		break;

	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unrecognized drive type %u returned for drive '%s' "
		"used by file '%s'.",
			drive_type, drive_root_name, filename );
		break;
	}

	/* For some filesystem types, it is possible to get more precise
	 * information.
	 */

	switch( *filesystem_type ) {
	case MXP_MXF_FST_CDROM:
		*filesystem_type = MXF_FST_ISO9660;
		break;

	case MXF_FST_LOCAL:

#if (_MSC_VER < 1200)
		/* For Visual C++ 5.0 or before. */
		*filesystem_type = MXF_FST_LOCAL;

#else /* _MSC_VER >= 1200 */
		{
			BYTE ioctl_buffer[10240];
			FILESYSTEM_STATISTICS *fs_statistics;
			HANDLE file_handle;
			DWORD bytes_returned;
			BOOL ioctl_result;

			file_handle = CreateFile( drive_root_name,
					0,
					FILE_SHARE_READ
					  | FILE_SHARE_WRITE
					  | FILE_SHARE_DELETE,
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_BACKUP_SEMANTICS,
					NULL );

			if ( file_handle == INVALID_HANDLE_VALUE ) {
				last_error_code = GetLastError();

				mx_win32_error_message( last_error_code,
					error_message, sizeof(error_message) );

				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
				"The attempt to access the drive root '%s' "
				"used by file '%s' failed.  "
				"Win32 error code = %ld, error message = '%s'",
					drive_root_name, filename,
					last_error_code, error_message );
			}

			ioctl_result = DeviceIoControl( file_handle,
					FSCTL_FILESYSTEM_GET_STATISTICS,
					NULL, 0,
					ioctl_buffer, sizeof(ioctl_buffer),
					&bytes_returned, NULL );

			if ( ioctl_result == 0 ) {
				last_error_code = GetLastError();

				CloseHandle( file_handle );

				mx_win32_error_message( last_error_code,
				error_message, sizeof(error_message) );

				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
				"The attempt to get filesystem statistics "
				"for filesystem '%s' failed.  "
				"Win32 error code = %ld, error message = '%s'",
					drive_root_name,
					last_error_code, error_message );
			}

			CloseHandle( file_handle );

			fs_statistics = (FILESYSTEM_STATISTICS *) ioctl_buffer;

			switch( fs_statistics->FileSystemType ) {

#if defined(FILESYSTEM_STATISTICS_TYPE_EXFAT)
			case FILESYSTEM_STATISTICS_TYPE_EXFAT:
				*filesystem_type = MXF_FST_EXFAT;
				break;
#endif

			case FILESYSTEM_STATISTICS_TYPE_FAT:
				*filesystem_type = MXF_FST_FAT;
				break;

			case FILESYSTEM_STATISTICS_TYPE_NTFS:
				*filesystem_type = MXF_FST_NTFS;
				break;

			default:
				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
					"Filesystem '%s' has unrecognized "
					"filesystem type %ld.",
						drive_root_name,
						fs_statistics->FileSystemType );
				break;
			}
		}
#endif /* _MSC_VER >= 1200 */
		break;

	case MXF_FST_REMOTE:
		*filesystem_type = MXF_FST_SMB;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_CYGWIN) || defined(OS_BSD) || defined(OS_UNIXWARE) \
	|| defined(OS_QNX) || defined(OS_RTEMS) || defined(OS_VXWORKS) \
	|| defined(OS_HURD) || defined(OS_VMS) || defined(OS_DJGPP) \
	|| defined(OS_ANDROID)

/* FIXME: On Linux, at least, it should be possible to do something
 * with statfs().  On MacOS X, statfs() does not appear to return the
 * necessary information in the f_type field.
 */

MX_EXPORT mx_status_type
mx_get_filesystem_type( char *filename,
			unsigned long *filesystem_type )
{
	*filesystem_type = MXF_FST_LOCAL;

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#else

#error Getting the filesystem type is not yet supported for this platform.

#endif

/*=========================================================================*/

#if defined(OS_WIN32)

/* WARNING: This code must be after any calls to access() in this file
 * due to the #undef that follows.
 */

#ifdef access
#undef access
#endif

/* The _access() function on Win32 does not support the X_OK flag.
 * It also does not _ignore_ the presence of the X_OK flag.  Instead,
 * it returns an error if X_OK is present.  Thus, we have to remove
 * the X_OK bit from the 'mode' argument before sending it on to
 * _access().
 */

MX_EXPORT int
mx_access( const char *pathname, int mode )
{
	int access_status;

	mode &= (~X_OK);

	access_status = _access( pathname, mode );

	return access_status;
}

#elif defined(OS_VXWORKS)

/* If the current platform does not have an access() function, then we
 * implement one using stat().
 */

MX_EXPORT int
access( const char *pathname, int mode )
{
	struct stat status_struct;
	mode_t file_mode;
	int status;

	/* FIXME: VxWorks has an incorrect prototype for stat(), namely,
	 *        the pathname argument is prototyped as "char *",
	 *        rather than "const char *".  At the moment, the least
	 *        offensive way around it seems to be to strdup() the
	 *        pathname and hand that pointer to stat().  Other
	 *        suggestions are welcome, but remember that they have
	 *        to work with versions of GCC as old as GCC 2.7.2.
	 */

#if !defined(OS_VXWORKS)
	status = stat( pathname, &status_struct );
#else
	{
		/* Ick */

		char *pathname_ptr = strdup( pathname );

		status = stat( pathname_ptr, &status_struct );

		mx_free( pathname_ptr );
	}
#endif

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

#endif  /* OS_VXWORKS */

/*=========================================================================*/

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


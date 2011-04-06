/*
 * Name:    mx_pipe.c
 *
 * Purpose: MX pipe functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2007-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_PIPE_DEBUG			FALSE

#define MX_PIPE_DEBUG_AVAILABLE_BYTES	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_osdef.h"

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_unistd.h"
#include "mx_socket.h"
#include "mx_pipe.h"

/************************ Windows ***********************/

#if defined(OS_WIN32)

#include <windows.h>

typedef struct {
	HANDLE read_handle;
	HANDLE write_handle;
} MX_WIN32_PIPE;

static mx_status_type
mx_pipe_get_pointers( MX_PIPE *mx_pipe,
			MX_WIN32_PIPE **win32_pipe,
			const char *calling_fname )
{
	static const char fname[] = "mx_pipe_get_pointers()";

	if ( mx_pipe == (MX_PIPE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PIPE pointer passed by '%s' was NULL.", calling_fname );
	}

	if ( win32_pipe == (MX_WIN32_PIPE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_WIN32_PIPE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	(*win32_pipe) = mx_pipe->private_ptr;

	if ( (*win32_pipe) == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_WIN32_PIPE pointer for MX_PIPE %p "
			"passed by '%s' was NULL.", mx_pipe, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_open( MX_PIPE **mx_pipe )
{
	static const char fname[] = "mx_pipe_open()";

	MX_WIN32_PIPE *win32_pipe;
	BOOL os_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	SECURITY_ATTRIBUTES pipe_attributes;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( mx_pipe == (MX_PIPE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PIPE pointer passed was NULL." );
	}

	*mx_pipe = (MX_PIPE *) malloc( sizeof(MX_PIPE) );

	if ( *mx_pipe == (MX_PIPE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_PIPE structure." );
	}

	win32_pipe = (MX_WIN32_PIPE *) malloc( sizeof(MX_WIN32_PIPE) );

	if ( win32_pipe == (MX_WIN32_PIPE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_WIN32_PIPE pointer." );
	}

	(*mx_pipe)->private_ptr = win32_pipe;

	/* Make sure the pipe handles can be inherited by a new process. */

	pipe_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	pipe_attributes.lpSecurityDescriptor = NULL;
	pipe_attributes.bInheritHandle = TRUE;

	/* Now create the pipe. */

	os_status = CreatePipe( &(win32_pipe->read_handle),
				&(win32_pipe->write_handle),
				&pipe_attributes, 0 );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to create a Win32 anonymous pipe failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: MX pipe %p created.  "
		"Win32 read handle = %p, Win32 write handle = %p",
		fname, *mx_pipe, win32_pipe->read_handle,
		win32_pipe->write_handle));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_close( MX_PIPE *mx_pipe, int flags )
{
	static const char fname[] = "mx_pipe_close()";

	MX_WIN32_PIPE *win32_pipe;
	BOOL os_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked for MX pipe %p.", fname, mx_pipe));
#endif

	mx_status = mx_pipe_get_pointers( mx_pipe, &win32_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( flags & MXF_PIPE_READ ) {
		os_status = CloseHandle( win32_pipe->read_handle );

		if ( os_status == 0 ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"The attempt to close the read handle %p for "
			"MX pipe %p failed.  "
			"Win32 error code = %ld, error message = '%s'.",
				win32_pipe->read_handle, mx_pipe,
				last_error_code, message_buffer );
		}

		win32_pipe->read_handle = NULL;
	}

	if ( flags & MXF_PIPE_WRITE ) {
		os_status = CloseHandle( win32_pipe->write_handle );

		if ( os_status == 0 ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"The attempt to close the write handle %p for "
			"MX pipe %p failed.  "
			"Win32 error code = %ld, error message = '%s'.",
				win32_pipe->write_handle, mx_pipe,
				last_error_code, message_buffer );
		}

		win32_pipe->write_handle = NULL;
	}

	if ( (win32_pipe->read_handle == NULL)
	  && (win32_pipe->write_handle == NULL) )
	{
		mx_free( win32_pipe );

		mx_free( mx_pipe );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_read( MX_PIPE *mx_pipe,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read )
{
	static const char fname[] = "mx_pipe_read()";

	MX_WIN32_PIPE *win32_pipe;
	BOOL read_status;
	DWORD last_error_code, number_of_bytes_read;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked for MX pipe %p.", fname, mx_pipe));
#endif

	mx_status = mx_pipe_get_pointers( mx_pipe, &win32_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_status = ReadFile( win32_pipe->read_handle,
					buffer, max_bytes_to_read,
					&number_of_bytes_read, NULL );

	if ( read_status == 0 ) {
		last_error_code = GetLastError();

		if ( last_error_code == ERROR_NO_DATA ) {
			return mx_error( MXE_END_OF_DATA | MXE_QUIET, fname,
			"MX pipe %p has no unread data.", mx_pipe );
		} else {
			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"The attempt to read %ld bytes from MX pipe %p failed."
			"  Win32 error code = %ld, error message = '%s'.",
				(long) max_bytes_to_read, mx_pipe,
				last_error_code, message_buffer );
		}
	}

	if ( bytes_read != NULL ) {
		*bytes_read = number_of_bytes_read;
	}

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: %lu bytes read from MX pipe %p into buffer %p",
		fname, (unsigned long) number_of_bytes_read, mx_pipe, buffer));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_write( MX_PIPE *mx_pipe,
		char *buffer,
		size_t bytes_to_write )
{
	static const char fname[] = "mx_pipe_write()";

	MX_WIN32_PIPE *win32_pipe;
	BOOL write_status;
	DWORD last_error_code, number_of_bytes_written;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: writing %ld bytes from buffer %p to MX pipe %p",
		fname, (long) bytes_to_write, buffer, mx_pipe));
#endif

	mx_status = mx_pipe_get_pointers( mx_pipe, &win32_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	write_status = WriteFile( win32_pipe->write_handle,
					buffer, bytes_to_write,
					&number_of_bytes_written, NULL );

	if ( write_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to write %ld bytes to MX pipe %p failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			(long) bytes_to_write, mx_pipe,
			last_error_code, message_buffer );
	}

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: %lu bytes written to MX pipe %p",
		fname, (unsigned long) number_of_bytes_written, mx_pipe));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_num_bytes_available( MX_PIPE *mx_pipe,
				size_t *num_bytes_available )
{
	static const char fname[] = "mx_pipe_num_bytes_available()";

	MX_WIN32_PIPE *win32_pipe;
	BOOL pipe_status;
	DWORD last_error_code, bytes_read, total_bytes_avail;
	TCHAR message_buffer[100];
	TCHAR peek_buffer[1000];
	mx_status_type mx_status;

	mx_status = mx_pipe_get_pointers( mx_pipe, &win32_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_bytes_available == (size_t) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available_pointer passed was NULL." );
	}

	if ( win32_pipe->read_handle == NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"MX pipe %p is not open for reading.", mx_pipe );
	}


	pipe_status = PeekNamedPipe( win32_pipe->read_handle,
			peek_buffer, sizeof(peek_buffer),
			&bytes_read, &total_bytes_avail, NULL );

#if MX_PIPE_DEBUG
	if ( total_bytes_avail != 0 ) {
		MX_DEBUG(-2,("Peek: bytes_read = %ld, total_bytes_avail = %ld",
				(long) bytes_read, (long) total_bytes_avail ));
	}
#endif

	if ( pipe_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to determine the number of bytes available "
		"in MX pipe %p failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			mx_pipe, last_error_code, message_buffer );
	}

	*num_bytes_available = total_bytes_avail;

#if MX_PIPE_DEBUG_AVAILABLE_BYTES
	if ( (*num_bytes_available) > 0 ) {
		MX_DEBUG(-2,("%s: %ld bytes available for MX pipe %p.",
			fname, (long) *num_bytes_available, mx_pipe ));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_set_blocking_mode( MX_PIPE *mx_pipe,
				int flags,
				mx_bool_type blocking_mode )
{
	static const char fname[] = "mx_pipe_set_blocking_mode()";

	MX_WIN32_PIPE *win32_pipe;
	BOOL pipe_status;
	DWORD pipe_mode;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

	mx_status = mx_pipe_get_pointers( mx_pipe, &win32_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,
	("%s invoked for MX pipe %p, flags = %#x, blocking_mode = %d",
		fname, mx_pipe, flags, blocking_mode));
#endif

	if ( (flags & MXF_PIPE_READ)
	  && (win32_pipe->read_handle != INVALID_HANDLE_VALUE)
	  && (win32_pipe->read_handle != NULL) )
	{
		/* Ignored. */

		/* FIXME: Is ignoring it the right thing to do?
		 *        I haven't found anything in the Win32
		 *        API about changing the blocking mode
		 *        for reading from a pipe.
		 */

		if ( blocking_mode == FALSE ) {
			pipe_mode = PIPE_NOWAIT;
		} else {
			pipe_mode = PIPE_WAIT;
		}

		pipe_status = SetNamedPipeHandleState(
				win32_pipe->read_handle,
				&pipe_mode, NULL, NULL );

		if ( pipe_status == 0 ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"The attempt to change the read blocking mode "
			"for MX pipe %p failed.  "
			"Win32 error code = %ld, error message = '%s'.",
				mx_pipe, last_error_code, message_buffer );
		}
	}

	if ( (flags & MXF_PIPE_WRITE)
	  && (win32_pipe->write_handle != INVALID_HANDLE_VALUE)
	  && (win32_pipe->write_handle != NULL) )
	{
		/* FIXME: If we start using message mode for pipes
		 *        in MX, then we will need to detect and
		 *        and preserve the state of that bit.
		 *        However, byte mode is more compatible
		 *        with other operating systems.
		 */

		if ( blocking_mode == FALSE ) {
			pipe_mode = PIPE_NOWAIT;
		} else {
			pipe_mode = PIPE_WAIT;
		}

		pipe_status = SetNamedPipeHandleState(
				win32_pipe->write_handle,
				&pipe_mode, NULL, NULL );

		if ( pipe_status == 0 ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"The attempt to change the write blocking mode "
			"for MX pipe %p failed.  "
			"Win32 error code = %ld, error message = '%s'.",
				mx_pipe, last_error_code, message_buffer );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/************************ Unix ***********************/

#elif defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_VMS) \
	|| defined(OS_DJGPP)

#include <limits.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#if defined(OS_SOLARIS)
#  include <sys/filio.h>
#endif

typedef struct {
	int read_fd;
	int write_fd;
} MX_UNIX_PIPE;

static mx_status_type
mx_pipe_get_pointers( MX_PIPE *mx_pipe,
			MX_UNIX_PIPE **unix_pipe,
			const char *calling_fname )
{
	static const char fname[] = "mx_pipe_get_pointers()";

	if ( mx_pipe == (MX_PIPE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PIPE pointer passed by '%s' was NULL.", calling_fname );
	}

	if ( unix_pipe == (MX_UNIX_PIPE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_UNIX_PIPE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	(*unix_pipe) = mx_pipe->private_ptr;

	if ( (*unix_pipe) == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_UNIX_PIPE pointer for MX_PIPE %p "
			"passed by '%s' was NULL.", mx_pipe, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_open( MX_PIPE **mx_pipe )
{
	static const char fname[] = "mx_pipe_open()";

	MX_UNIX_PIPE *unix_pipe;
	int pipe_array[2];
	int os_status, saved_errno;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( mx_pipe == (MX_PIPE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PIPE pointer passed was NULL." );
	}

	*mx_pipe = (MX_PIPE *) malloc( sizeof(MX_PIPE) );

	if ( *mx_pipe == (MX_PIPE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_PIPE structure." );
	}

	unix_pipe = (MX_UNIX_PIPE *) malloc( sizeof(MX_UNIX_PIPE) );

	if ( unix_pipe == (MX_UNIX_PIPE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_UNIX_PIPE pointer." );
	}

	(*mx_pipe)->private_ptr = unix_pipe;

	os_status = pipe( pipe_array );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to create a pipe failed.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	unix_pipe->read_fd  = pipe_array[0];
	unix_pipe->write_fd = pipe_array[1];

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: MX pipe %p created.  "
		"Unix read fd = %d, Unix write fd = %d",
		fname, *mx_pipe, unix_pipe->read_fd,
		unix_pipe->write_fd));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_close( MX_PIPE *mx_pipe, int flags )
{
	static const char fname[] = "mx_pipe_close()";

	MX_UNIX_PIPE *unix_pipe;
	int os_status, saved_errno;
	mx_status_type mx_status;

	unix_pipe = NULL;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked for MX pipe %p.", fname, mx_pipe));
#endif

	mx_status = mx_pipe_get_pointers( mx_pipe, &unix_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( flags & MXF_PIPE_READ ) {
		os_status = close( unix_pipe->read_fd );

		if ( os_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"The attempt to close the read fd for pipe %p failed.  "
			"Errno = %d, error message = '%s'",
				mx_pipe, saved_errno, strerror(saved_errno) );
		}

		unix_pipe->read_fd = -1;
	}

	if ( flags & MXF_PIPE_WRITE ) {
		os_status = close( unix_pipe->write_fd );

		if ( os_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"The attempt to close the write fd for pipe %p failed."
			"  Errno = %d, error message = '%s'",
				mx_pipe, saved_errno, strerror(saved_errno) );
		}

		unix_pipe->write_fd = -1;
	}

	if ( (unix_pipe->read_fd < 0) && (unix_pipe->write_fd < 0) ) {
		mx_free( unix_pipe );

		mx_free( mx_pipe );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_read( MX_PIPE *mx_pipe,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read )
{
	static const char fname[] = "mx_pipe_read()";

	MX_UNIX_PIPE *unix_pipe;
	int read_status, saved_errno;
	mx_status_type mx_status;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	unix_pipe = NULL;

	mx_status = mx_pipe_get_pointers( mx_pipe, &unix_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_status = read( unix_pipe->read_fd, buffer, max_bytes_to_read );

	if ( read_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to read %ld bytes from MX pipe %p failed.  "
		"Errno = %d, error message = '%s'",
			(long) max_bytes_to_read, mx_pipe,
			saved_errno, strerror(saved_errno) );
	}

	if ( bytes_read != NULL ) {
		*bytes_read = read_status;
	}

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: %d bytes read from MX pipe %p into buffer %p",
		fname, read_status, mx_pipe, buffer));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_write( MX_PIPE *mx_pipe,
		char *buffer,
		size_t bytes_to_write )
{
	static const char fname[] = "mx_pipe_write()";

	MX_UNIX_PIPE *unix_pipe;
	int write_status, saved_errno;
	mx_status_type mx_status;

	unix_pipe = NULL;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: writing %ld bytes from buffer %p to MX pipe %p",
		fname, (long) bytes_to_write, buffer, mx_pipe));
#endif

	mx_status = mx_pipe_get_pointers( mx_pipe, &unix_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_to_write > PIPE_BUF ) {
		mx_warning( "You have requested to write %ld bytes "
		"to the pipe %p.  Writes of greater than %d (PIPE_BUF) bytes "
		"are not guaranteed to be atomic and may be interleaved with "
		"bytes from other processes, threads, or signal handlers.",
			(long) bytes_to_write, mx_pipe, PIPE_BUF );
	}

	write_status = write( unix_pipe->write_fd, buffer, bytes_to_write );

	if ( write_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to write %ld bytes to MX pipe %p failed.  "
		"Errno = %d, error message = '%s'",
			(long) bytes_to_write, mx_pipe,
			saved_errno, strerror(saved_errno) );
	}

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: %d bytes written to MX pipe %p",
		fname, write_status, mx_pipe));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_num_bytes_available( MX_PIPE *mx_pipe,
				size_t *num_bytes_available )
{
	static const char fname[] = "mx_pipe_num_bytes_available()";

	MX_UNIX_PIPE *unix_pipe;
	int os_status;
	mx_status_type mx_status;

	unix_pipe = NULL;

	mx_status = mx_pipe_get_pointers( mx_pipe, &unix_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_bytes_available == (size_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available_pointer passed was NULL." );
	}

	if ( unix_pipe->read_fd < 0 ) {
		return mx_error( MXE_NOT_READY, fname,
		"MX pipe %p is not open for reading.", mx_pipe );
	}

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
		|| defined(OS_IRIX) || defined(OS_CYGWIN)
	{
		int saved_errno;

		/* Use FIONREAD */

		int num_chars_available;

		os_status = ioctl( unix_pipe->read_fd,
					FIONREAD, &num_chars_available );

		if ( os_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
		"An error occurred while checking MX pipe %p to see if any "
		"input characters are available.  "
		"Errno = %d, error message = '%s'",
				mx_pipe, saved_errno, strerror(saved_errno) );
		}

		*num_bytes_available = num_chars_available;
	}
#else	
	{
		/* Use select() */

		fd_set mask;
		struct timeval timeout;

		FD_ZERO( &mask );
		FD_SET( unix_pipe->read_fd, &mask );

		timeout.tv_sec = 0;  timeout.tv_usec = 0;

		os_status = select( 1 + unix_pipe->read_fd,
					&mask, NULL, NULL, &timeout );

		if ( os_status ) {
			*num_bytes_available = 1;
		} else {
			*num_bytes_available = 0;
		}
	}
#endif

#if MX_PIPE_DEBUG_AVAILABLE_BYTES
	if ( (*num_bytes_available) > 0 ) {
		MX_DEBUG(-2,("%s: %ld bytes available for MX pipe %p.",
			fname, (long) *num_bytes_available, mx_pipe ));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

#if defined(OS_DJGPP)
#  define O_NDELAY	O_NONBLOCK
#endif

MX_EXPORT mx_status_type
mx_pipe_set_blocking_mode( MX_PIPE *mx_pipe,
				int flags,
				mx_bool_type blocking_mode )
{
	static const char fname[] = "mx_pipe_set_blocking_mode()";

	MX_UNIX_PIPE *unix_pipe;
	int old_flags, new_flags, os_status, saved_errno;
	mx_status_type mx_status;

	unix_pipe = NULL;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,
	("%s invoked for MX pipe %p, flags = %#x, blocking_mode = %d",
		fname, mx_pipe, flags, blocking_mode));
#endif

	mx_status = mx_pipe_get_pointers( mx_pipe, &unix_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (flags & MXF_PIPE_READ) && (unix_pipe->read_fd >= 0) )
	{
		old_flags = fcntl( unix_pipe->read_fd, F_GETFL, 0 );

		if ( old_flags < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"Error using F_GETFL for read fd %d of MX pipe %p.  "
			"Errno = %d, error text = '%s'.",
				unix_pipe->read_fd, mx_pipe,
				saved_errno, strerror(saved_errno) );
		}

		if ( blocking_mode == FALSE ) {
			new_flags = old_flags | O_NDELAY;
		} else {
			new_flags = old_flags & ~O_NDELAY;
		}

		os_status = fcntl( unix_pipe->read_fd, F_SETFL, new_flags );

		if ( os_status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"Error using F_SETFL for read fd %d of MX pipe %p.  "
			"Errno = %d, error text = '%s'.",
				unix_pipe->read_fd, mx_pipe,
				saved_errno, strerror(saved_errno) );
		}
	}

	if ( (flags & MXF_PIPE_WRITE) && (unix_pipe->write_fd >= 0) )
	{
		old_flags = fcntl( unix_pipe->write_fd, F_GETFL, 0 );

		if ( old_flags < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"Error using F_GETFL for write fd %d of MX pipe %p.  "
			"Errno = %d, error text = '%s'.",
				unix_pipe->write_fd, mx_pipe,
				saved_errno, strerror(saved_errno) );
		}

		if ( blocking_mode == FALSE ) {
			new_flags = old_flags | O_NDELAY;
		} else {
			new_flags = old_flags & ~O_NDELAY;
		}

		os_status = fcntl( unix_pipe->write_fd, F_SETFL, new_flags );

		if ( os_status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"Error using F_SETFL for write fd %d of MX pipe %p.  "
			"Errno = %d, error text = '%s'.",
				unix_pipe->write_fd, mx_pipe,
				saved_errno, strerror(saved_errno) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/************************ VxWorks ***********************/

#elif defined(OS_VXWORKS)

#include <msgQLib.h>

/* FIXME: The rest of MX expects that pipes are stream oriented devices.
 *        However, for VxWorks, both pipes and message queues are message
 *        oriented rather than stream oriented.  For now, we just use
 *        VxWorks message queues with the message size set to 1 to 
 *        simulate" stream oriented behavior.  This may actually be very
 *        inefficient, but we have not tested the performance yet.
 */

typedef struct {
	MSG_Q_ID message_queue_id;
	mx_bool_type blocking_mode;
} MX_VXWORKS_PIPE;

static mx_status_type
mx_pipe_get_pointers( MX_PIPE *mx_pipe,
			MX_VXWORKS_PIPE **vxworks_pipe,
			const char *calling_fname )
{
	static const char fname[] = "mx_pipe_get_pointers()";

	if ( mx_pipe == (MX_PIPE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PIPE pointer passed by '%s' was NULL.", calling_fname );
	}

	if ( vxworks_pipe == (MX_VXWORKS_PIPE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VXWORKS_PIPE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	(*vxworks_pipe) = mx_pipe->private_ptr;

	if ( (*vxworks_pipe) == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_VXWORKS_PIPE pointer for MX_PIPE %p "
			"passed by '%s' was NULL.", mx_pipe, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_open( MX_PIPE **mx_pipe )
{
	static const char fname[] = "mx_pipe_open()";

	MX_VXWORKS_PIPE *vxworks_pipe;
	int saved_errno;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( mx_pipe == (MX_PIPE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PIPE pointer passed was NULL." );
	}

	*mx_pipe = (MX_PIPE *) malloc( sizeof(MX_PIPE) );

	if ( *mx_pipe == (MX_PIPE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_PIPE structure." );
	}

	vxworks_pipe = (MX_VXWORKS_PIPE *) malloc( sizeof(MX_VXWORKS_PIPE) );

	if ( vxworks_pipe == (MX_VXWORKS_PIPE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_VXWORKS_PIPE pointer." );
	}

	(*mx_pipe)->private_ptr = vxworks_pipe;

	vxworks_pipe->blocking_mode = TRUE;

	/* Create the message queue with 1-byte messages and a maximum
	 * size of 512 messages.  The queue returns messages in FIFO order.
	 */

	vxworks_pipe->message_queue_id = msgQCreate( 512, 1, MSG_Q_FIFO );

	if ( vxworks_pipe->message_queue_id == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to create a VxWorks message queue failed.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: MX pipe %p created.  VxWorks message queue id = %lu",
		fname, mx_pipe, vxworks_pipe));
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_close( MX_PIPE *mx_pipe, int flags )
{
	static const char fname[] = "mx_pipe_close()";

	MX_VXWORKS_PIPE *vxworks_pipe;
	STATUS os_status;
	mx_status_type mx_status;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked for MX pipe %p.", fname, mx_pipe));
#endif

	mx_status = mx_pipe_get_pointers( mx_pipe, &vxworks_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	os_status = msgQDelete( vxworks_pipe->message_queue_id );

	if ( os_status != OK ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An error occurred while attempting to close a "
		"VxWorks message queue.  VxWorks status = %d",
			os_status );
	}

	mx_free( vxworks_pipe );

	mx_free( mx_pipe );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_read( MX_PIPE *mx_pipe,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read )
{
	static const char fname[] = "mx_pipe_read()";

	MX_VXWORKS_PIPE *vxworks_pipe;
	STATUS os_status;
	int i, timeout;
	mx_status_type mx_status;

	mx_status = mx_pipe_get_pointers( mx_pipe, &vxworks_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}

	if ( vxworks_pipe->blocking_mode ) {
		timeout = WAIT_FOREVER;
	} else {
		timeout = NO_WAIT;
	}

	for ( i = 0; i < max_bytes_to_read; i++ ) {
		os_status = msgQReceive( vxworks_pipe->message_queue_id,
						&buffer[i], 1, timeout );

		if ( os_status != OK ) {
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"An error occurred while reading from MX pipe %p.  "
			"VxWorks error code = %d", mx_pipe, (int) os_status );
		}
	}

	if ( bytes_read != NULL ) {
		if ( i >= max_bytes_to_read ) {
			*bytes_read = max_bytes_to_read;
		} else {
			*bytes_read = i;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_write( MX_PIPE *mx_pipe,
		char *buffer,
		size_t bytes_to_write )
{
	static const char fname[] = "mx_pipe_write()";

	MX_VXWORKS_PIPE *vxworks_pipe;
	STATUS os_status;
	int i, timeout;
	mx_status_type mx_status;

	mx_status = mx_pipe_get_pointers( mx_pipe, &vxworks_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}

	if ( vxworks_pipe->blocking_mode ) {
		timeout = WAIT_FOREVER;
	} else {
		timeout = NO_WAIT;
	}

	for ( i = 0; i < bytes_to_write; i++ ) {
		os_status = msgQSend( vxworks_pipe->message_queue_id,
				&buffer[i], 1, timeout, MSG_PRI_NORMAL );

		if ( os_status != OK ) {
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"An error occurred while writing to MX pipe %p.  "
			"VxWorks error code = %d", mx_pipe, (int) os_status );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_num_bytes_available( MX_PIPE *mx_pipe,
				size_t *num_bytes_available )
{
	static const char fname[] = "mx_pipe_num_bytes_available()";

	MX_VXWORKS_PIPE *vxworks_pipe;
	mx_status_type mx_status;

	mx_status = mx_pipe_get_pointers( mx_pipe, &vxworks_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_bytes_available == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available argument passed was NULL." );
	}

	*num_bytes_available = msgQNumMsgs( vxworks_pipe->message_queue_id );

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked for MX pipe %p, num_bytes_available = %ld",
		fname, mx_pipe, (long) *num_bytes_available));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_set_blocking_mode( MX_PIPE *mx_pipe,
				int flags,
				mx_bool_type blocking_mode )
{
	static const char fname[] = "mx_pipe_set_blocking_mode()";

	MX_VXWORKS_PIPE *vxworks_pipe;
	mx_status_type mx_status;

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked for MX pipe %p, blocking_mode = %d",
		fname, mx_pipe, (int) blocking_mode));
#endif

	mx_status = mx_pipe_get_pointers( mx_pipe, &vxworks_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vxworks_pipe->blocking_mode = blocking_mode;

	return MX_SUCCESSFUL_RESULT;
}

/************************ Not supported ***********************/

#elif defined(OS_ECOS) || defined(OS_RTEMS)

MX_EXPORT mx_status_type
mx_pipe_open( MX_PIPE **mx_pipe )
{
	static const char fname[] = "mx_pipe_open()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Pipes are not supported for this operating system." );
}

MX_EXPORT mx_status_type
mx_pipe_close( MX_PIPE *mx_pipe, int flags )
{
	static const char fname[] = "mx_pipe_close()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Pipes are not supported for this operating system." );
}

MX_EXPORT mx_status_type
mx_pipe_read( MX_PIPE *mx_pipe,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read )
{
	static const char fname[] = "mx_pipe_read()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Pipes are not supported for this operating system." );
}

MX_EXPORT mx_status_type
mx_pipe_write( MX_PIPE *mx_pipe,
		char *buffer,
		size_t bytes_to_write )
{
	static const char fname[] = "mx_pipe_write()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Pipes are not supported for this operating system." );
}

MX_EXPORT mx_status_type
mx_pipe_num_bytes_available( MX_PIPE *mx_pipe,
				size_t *num_bytes_available )
{
	static const char fname[] = "mx_pipe_num_bytes_available()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Pipes are not supported for this operating system." );
}

MX_EXPORT mx_status_type
mx_pipe_set_blocking_mode( MX_PIPE *mx_pipe,
				int flags,
				mx_bool_type blocking_mode )
{
	static const char fname[] = "mx_pipe_set_blocking_mode()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Pipes are not supported for this operating system." );
}

/************************ Not yet implemented ***********************/

#else

#error MX pipe functions have not yet been defined for this platform.

#endif

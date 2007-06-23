/*
 * Name:    mx_pipe.c
 *
 * Purpose: MX pipe functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_PIPE_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_osdef.h"

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_unistd.h"
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

	(*win32_pipe) = mx_pipe->private;

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

	(*mx_pipe)->private = win32_pipe;

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
	MX_DEBUG(-2,("%s invoked.", fname));
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
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_pipe_get_pointers( mx_pipe, &win32_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_status = ReadFile( win32_pipe->read_handle,
					buffer, max_bytes_to_read,
					&number_of_bytes_read, NULL );

	if ( read_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to read %ld bytes from MX pipe %p failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			max_bytes_to_read, mx_pipe,
			last_error_code, message_buffer );
	}

	if ( bytes_read != NULL ) {
		*bytes_read = number_of_bytes_read;
	}

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
	MX_DEBUG(-2,("%s invoked.", fname));
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
			bytes_to_write, mx_pipe,
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_num_bytes_available( MX_PIPE *mx_pipe,
				size_t *num_bytes_available )
{
	static const char fname[] = "mx_pipe_num_bytes_available()";

	MX_WIN32_PIPE *win32_pipe;
	BOOL pipe_status;
	DWORD last_error_code, total_bytes_avail;
	TCHAR message_buffer[100];
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
				NULL, 0, NULL, &total_bytes_avail, NULL );

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

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: num_bytes_available = %ld",
		fname, (long) *num_bytes_available ));
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

#elif defined(OS_UNIX)

#include <fcntl.h>
#include <sys/ioctl.h>

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

	(*unix_pipe) = mx_pipe->private;

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

	(*mx_pipe)->private = unix_pipe;

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
	MX_DEBUG(-2,("%s: create MX pipe %p", fname, *mx_pipe));
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

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
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
	MX_DEBUG(-2,("%s: read %d bytes from MX pipe %p",
		fname, read_status, mx_pipe));
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

#if MX_PIPE_DEBUG
	MX_DEBUG(-2,("%s: writing %ld bytes to MX pipe %p",
		fname, (long) bytes_to_write, mx_pipe));
#endif

	mx_status = mx_pipe_get_pointers( mx_pipe, &unix_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	write_status = write( unix_pipe->write_fd, buffer, bytes_to_write );

	if ( write_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to write %ld bytes to MX pipe %p failed.  "
		"Errno = %d, error message = '%s'",
			(long) bytes_to_write, mx_pipe,
			saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pipe_num_bytes_available( MX_PIPE *mx_pipe,
				size_t *num_bytes_available )
{
	static const char fname[] = "mx_pipe_num_bytes_available()";

	MX_UNIX_PIPE *unix_pipe;
	int os_status, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_pipe_get_pointers( mx_pipe, &unix_pipe, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_bytes_available == (size_t) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available_pointer passed was NULL." );
	}

	if ( unix_pipe->read_fd < 0 ) {
		return mx_error( MXE_NOT_READY, fname,
		"MX pipe %p is not open for reading.", mx_pipe );
	}

#if defined(OS_LINUX)
	{
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

#if MX_PIPE_DEBUG
	if ( (*num_bytes_available) > 0 ) {
		MX_DEBUG(-2,("%s: num_bytes_available = %ld",
			fname, (long) *num_bytes_available ));
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

	MX_UNIX_PIPE *unix_pipe;
	int old_flags, new_flags, os_status, saved_errno;
	mx_status_type mx_status;

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

#else

#error MX pipe functions have not yet been defined for this platform.

#endif

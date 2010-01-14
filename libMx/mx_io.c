/*
 * Name:    mx_io.c
 *
 * Purpose: MX-specific file/pipe/etc I/O related functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_select.h"
#include "mx_io.h"

/*-------------------------------------------------------------------------*/

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_VMS) \
	|| defined(OS_DJGPP)

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#if defined(OS_SOLARIS)
#  include <sys/filio.h>
#endif

/*---*/

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_IRIX) || defined(OS_CYGWIN)

#  define USE_FIONREAD	TRUE
#else
#  define USE_FIONREAD	FALSE
#endif

/*---*/

MX_EXPORT mx_status_type
mx_fd_num_input_bytes_available( int fd, size_t *num_bytes_available )
{
	static const char fname[] = "mx_fd_num_input_bytes_available()";

	int os_status;

	if ( num_bytes_available == NULL )  {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available pointer passed was NULL." );
	}
	if ( fd < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"File descriptor %d passed is not a valid file descriptor.",
			fd );
	}

#if USE_FIONREAD
	{
		int saved_errno;
		int num_chars_available;

		os_status = ioctl( fd, FIONREAD, &num_chars_available );

		if ( os_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"An error occurred while checking file descriptor %d "
			"to see if any input characters are available.  "
			"Errno = %d, error message = '%s'",
				fd, saved_errno, strerror(saved_errno) );
		}

		*num_bytes_available = num_chars_available;
	}
#else
	{
		/* Use select() instead. */

		fd_set mask;
		struct timeval timeout;

		FD_ZERO( &mask );
		FD_SET( fd, &mask );

		timeout.tv_sec = 0;  timeout.tv_usec = 0;

		os_status = select( 1 + fd, &mask, NULL, NULL, &timeout );

		if ( os_status ) {
			*num_bytes_available = 1;
		} else {
			*num_bytes_available = 0;
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

MX_EXPORT mx_status_type
mx_fd_num_input_bytes_available( int fd, size_t *num_bytes_available )
{
	static const char fname[] = "mx_fd_num_input_bytes_available()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented." );
}

/*-------------------------------------------------------------------------*/

#else
#error mx_io.c is not yet implemented for this platform.
#endif


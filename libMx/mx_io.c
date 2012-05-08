/*
 * Name:    mx_io.c
 *
 * Purpose: MX-specific file/pipe/etc I/O related functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2010-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_stdint.h"
#include "mx_select.h"
#include "mx_io.h"

#if defined(OS_WIN32)
#  define popen(x,p) _popen(x,p)
#  define pclose(x)  _pclose(x)
#endif

/*-------------------------------------------------------------------------*/

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_VMS) \
	|| defined(OS_DJGPP) || defined(OS_RTEMS) || defined(OS_VXWORKS)

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#if defined(OS_SOLARIS)
#  include <sys/filio.h>
#endif

#if defined(OS_CYGWIN)
#  include <sys/socket.h>
#endif

/*---*/

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_IRIX) || defined(OS_CYGWIN) || defined(OS_RTEMS)

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
#error MX file descriptor I/O functions are not yet implemented for this platform.
#endif

/*=========================================================================*/

MX_EXPORT int
mx_get_max_file_descriptors( void )
{
	int result;

#if defined( OS_UNIX ) || defined( OS_CYGWIN ) || defined( OS_DJGPP ) \
	|| defined( OS_VMS )

	result = getdtablesize();

#elif defined( OS_WIN32 )

	result = FD_SETSIZE;

#elif defined( OS_ECOS )

	result = CYGNUM_FILEIO_NFD;

#elif defined( OS_RTEMS ) || defined( OS_VXWORKS )

	result = 16;		/* A wild guess and probably wrong. */

#else

#error mx_get_max_file_descriptors() has not been implemented for this platform.

#endif

	return result;
}

/*=========================================================================*/

#if defined( OS_UNIX ) || defined( OS_CYGWIN ) || defined( OS_DJGPP ) \
	|| defined( OS_VMS ) || defined( OS_ECOS ) || defined( OS_RTEMS ) \
	|| defined( OS_VXWORKS )

MX_EXPORT int
mx_get_number_of_open_file_descriptors( void )
{
	static const char fname[] = "mx_get_number_of_open_file_descriptors()";

	int i, max_descriptors, os_status, saved_errno, num_open;
	struct stat stat_buf;

	num_open = 0;
	max_descriptors = mx_get_max_file_descriptors();

	for ( i = 0; i < max_descriptors; i++ ) {
		os_status = fstat( i, &stat_buf );

		saved_errno = errno;

#if 0
		MX_DEBUG(-2,("open fds: i = %d, os_status = %d, errno = %d",
			i, os_status, saved_errno));
#endif

		if ( os_status == 0 ) {
			num_open++;
		} else {
			if ( saved_errno != EBADF ) {
				mx_warning(
		"%s: fstat(%d,...) returned errno = %d, error message = '%s'",
				fname, i, saved_errno, strerror(saved_errno) );
			}
		}
	}

	return num_open;
}

#elif defined( OS_WIN32 )

MX_EXPORT int
mx_get_number_of_open_file_descriptors( void )
{
	return( -1 );	/* Not yet implemented. */
}

#else

#error mx_get_number_of_open_file_descriptors() has not been implemented for this platform.

#endif

/*=========================================================================*/

#if !defined(OS_VXWORKS)

#define MXP_LSOF_FILE	1
#define MXP_LSOF_PIPE	2
#define MXP_LSOF_SOCKET	3

static void
mxp_lsof_get_pipe_peer( unsigned long process_id,
			int inode,
			unsigned long *peer_pid,
			char *peer_command_name,
			size_t peer_command_name_length )
{
	static const char fname[] = "mxp_lsof_get_pipe_peer()";

	FILE *file;
	char command[200];
	char response[MXU_FILENAME_LENGTH+20];
	int saved_errno;
	unsigned long current_pid, current_inode;
	char current_command_name[MXU_FILENAME_LENGTH+1];
	int max_fds;
	mx_bool_type is_fifo;

#if 0
	fprintf( stderr, "Looking for pipe inode %d\n", inode );
#endif

	max_fds = mx_get_max_file_descriptors();

	snprintf( command, sizeof(command), "lsof -d0-%d -FfatDin", max_fds );

	file = popen( command, "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to run the command '%s'.  "
		"Errno = %d, error message = '%s'",
			command, saved_errno, strerror(saved_errno) );

		return;
	}

	mx_fgets( response, sizeof(response), file );

	if ( feof(file) || ferror(file) ) {
		/* Did not get any output from lsof. */

		pclose(file);
		return;
	}

	is_fifo = FALSE;
	current_inode = 0;
	current_command_name[0] = '\0';
	current_pid = 0;

	*peer_pid = 0;
	peer_command_name[0] = '\0';

	while (1) {

#if 0
		fprintf( stderr, "response = '%s'\n", response );
#endif
		switch( response[0] ) {
		case 'c':
			strlcpy( current_command_name, response+1,
				sizeof(current_command_name) );
			break;
		case 'f':
			is_fifo = FALSE;
			current_inode = 0;
			current_command_name[0] = '\0';
			break;
		case 'i':
			current_inode = mx_string_to_unsigned_long(response+1);
			break;
		case 'p':
			current_pid = mx_string_to_unsigned_long(response+1);
			break;
		case 't':
			if ( strcmp( response, "tFIFO" ) == 0 ) {
				is_fifo = TRUE;
			} else {
				is_fifo = FALSE;
			}
			break;
		}

		if ( is_fifo
		  && ( current_inode == inode )
		  && ( current_pid != process_id )
		) {
			/* We have found the correct combination. */

			break;
		}

		mx_fgets( response, sizeof(response), file );

		if ( feof(file) || ferror(file) ) {
			/* End of lsof output, so give up. */

			pclose(file);
			return;
		}
	}

	*peer_pid = current_pid;

	strlcpy( peer_command_name, current_command_name,
		peer_command_name_length );

	pclose(file);
	return;
}

static char *
mxp_parse_lsof_output( FILE *file,
			unsigned long process_id, int fd,
			char *buffer, size_t buffer_size )
{
	static const char fname[] = "mxp_parse_lsof_output()";

	mx_bool_type is_self;
	char response[MXU_FILENAME_LENGTH + 20];

	char mode[4] = "";
	char filename[MXU_FILENAME_LENGTH + 1] = "";
	char socket_protocol[20] = "";
	char socket_state[40] = "";
	unsigned long inode = 0;
	int fd_type = 0;

	int c, i, response_fd;
	size_t length;
	unsigned long peer_pid = 0;
	char peer_command_name[MXU_FILENAME_LENGTH+1];

	if ( process_id == mx_process_id() ) {
		is_self = TRUE;
	} else {
		is_self = FALSE;
	}

	c = fgetc( file );

	if ( feof(file) || ferror(file) ) {
		return NULL;
	}

	ungetc( c, file );

	if ( c != 'f' ) {
		(void) mx_error( MXE_PROTOCOL_ERROR, fname,
		"The first character from lsof for fd %d is not 'f'.  "
		"Instead, it is '%c' %x.", fd, c, c );

		return NULL;
	}

	while (1) {
		mx_fgets( response, sizeof(response), file );

		if ( feof(file) || ferror(file) ) {
			break;
		}

#if 0
		fprintf(stderr, "response = '%s'\n", response);
#endif

		switch( response[0] ) {
		case 'f':
			response_fd = mx_string_to_long( response+1 );

			if ( fd < 0 ) {
				fd = response_fd;
			} else
			if ( response_fd != fd ) {
				(void) mx_error( MXE_PROTOCOL_ERROR, fname,
				"The file descriptor in the response line "
				"did not have the expected value of %d.  "
				"Instead, it had a value of %d.",
					fd, response_fd );
				return NULL;
			}
			break;
			
		case 'a':
			if ( response[1] == 'r' ) {
				strlcpy( mode, "r", sizeof(mode) );
			} else
			if ( response[1] == 'w' ) {
				strlcpy( mode, "w", sizeof(mode) );
			} else
			if ( response[1] == 'u' ) {
				strlcpy( mode, "rw", sizeof(mode) );
			} else {
				strlcpy( mode, "?", sizeof(mode) );
			}
			break;
		case 'i':
			inode = mx_string_to_long( response+1 );
			break;
		case 'n':
			strlcpy( filename, response+1, sizeof(filename) );
			break;
		case 't':
			if ( strcmp( response+1, "REG" ) == 0 ) {
				fd_type = MXP_LSOF_FILE;
			} else
			if ( strcmp( response+1, "CHR" ) == 0 ) {
				fd_type = MXP_LSOF_FILE;
			} else
			if ( strcmp( response+1, "FIFO" ) == 0 ) {
				fd_type = MXP_LSOF_PIPE;
			} else
			if ( strcmp( response+1, "IPv4" ) == 0 ) {
				fd_type = MXP_LSOF_SOCKET;
			} else {
				fd_type = 0;
			}
			break;
		case 'P':
			length = sizeof(socket_protocol);

			for ( i = 0; i < length; i++ ) {
				c = response[i+1];

				if ( isupper(c) ) {
					c = tolower(c);
				}

				socket_protocol[i] = c;

				if ( c == '\0' )
					break;
			}
			break;
		case 'T':
			if ( strncmp( response, "TST=", 4 ) == 0 ) {
				snprintf( socket_state, sizeof(socket_state),
					"(%s)", response+4 );
			}
			break;
		default:
			break;
		}

		/* Check to see if the next character is 'f'.  If it is,
		 * then we have received all of the lines that we are
		 * going to receive for this file descriptor and it is
		 * time to break out of the loop.
		 */

		c = fgetc( file );

		if ( feof(file) || ferror(file) ) {
			/* There is no more data to receive, so we
			 * break out of the loop.
			 */
			break;
		}

		ungetc( c, file );

		if ( c == 'f' ) {
			break;
		}

	}

	switch( fd_type ) {
	case MXP_LSOF_FILE:
		snprintf( buffer, buffer_size,
			"%d - (%s) file: %s", fd, mode, filename );
		break;
	case MXP_LSOF_PIPE:
		mxp_lsof_get_pipe_peer( process_id, inode, &peer_pid,
				peer_command_name, sizeof(peer_command_name) );

		snprintf( buffer, buffer_size,
			"%d - (%s) pipe: connected to PID %lu '%s'",
			fd, mode, peer_pid, peer_command_name );
		break;
	case MXP_LSOF_SOCKET:
		snprintf( buffer, buffer_size,
			"%d - (%s) %s: %s %s",
			fd, mode, socket_protocol, filename, socket_state );
		break;
	default:
		strlcpy( buffer, "???:", buffer_size );
		break;
	}

	return buffer;
}

static char *
mxp_get_fd_name_from_lsof( unsigned long process_id, int fd,
			char *buffer, size_t buffer_size )
{
	static const char fname[] = "mxp_get_fd_name_from_lsof()";

	FILE *file;
	int saved_errno;
	char command[ MXU_FILENAME_LENGTH + 20 ];
	char response[80];
	char *ptr;

	fprintf(stderr, "START: fd = %d\n", fd );

	snprintf( command, sizeof(command),
		"lsof -a -p%lu -d%d -n -P -FfatDinPT",
		process_id, fd );

	file = popen( command, "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to run the command '%s'.  "
		"Errno = %d, error message = '%s'",
			command, saved_errno, strerror(saved_errno) );

		return NULL;
	}

	mx_fgets( response, sizeof(response), file );

	if ( feof(file) || ferror(file) ) {
		/* Did not get any output from lsof. */

		pclose(file);
		return NULL;
	}

	if ( response[0] != 'p' ) {
		/* This file descriptor is not open. */

		pclose(file);
		return NULL;
	}

	ptr = mxp_parse_lsof_output( file, process_id, fd,
					buffer, buffer_size );

	pclose(file);

	fprintf(stderr, "END: fd = %d\n", fd );

	return ptr;
}

#endif

/*-------------------------------------------------------------------------*/

MX_EXPORT char *
mx_get_fd_name( unsigned long process_id, int fd,
		char *buffer, size_t buffer_size )
{
	static char fname[] = "mx_get_fd_name()";

	char *ptr;

	if ( fd < 0 ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal file descriptor %d passed.", fd );

		return NULL;
	}
	if ( buffer == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL buffer pointer passed." );

		return NULL;
	}
	if ( buffer_size < 1 ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The specified buffer length of %ld is too short "
		"for the file descriptor name to fit.",
			(long) buffer_size );

		return NULL;
	}

#if 0
	ptr = mxp_get_fd_name_from_linux_proc( process_id, fd,
						buffer, buffer_size );
#elif !defined(OS_VXWORKS)
	ptr = mxp_get_fd_name_from_lsof( process_id, fd, buffer, buffer_size );
#else
	snprintf( buffer, buffer_size, "FD %d", fd );

	ptr = buffer;
#endif

	return ptr;
}

/*=========================================================================*/

#if !defined(OS_VXWORKS)

MX_EXPORT void
mx_show_fd_names( unsigned long process_id )
{
	static const char fname[] = "mx_show_fd_names()";

	FILE *file;
	char command[MXU_FILENAME_LENGTH+20];
	char response[80];
	char buffer[MXU_FILENAME_LENGTH+20];
	char *ptr;
	int max_fds, num_open_fds, saved_errno;

	max_fds = mx_get_max_file_descriptors();

	snprintf( command, sizeof(command),
		"lsof -a -p%lu -d0-%d -n -P -FfatDinPT",
		process_id, max_fds );

	file = popen( command, "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to run the command '%s'.  "
		"Errno = %d, error message = '%s'",
			command, saved_errno, strerror(saved_errno) );

		return;
	}

	mx_fgets( response, sizeof(response), file );

	if ( feof(file) || ferror(file) ) {
		/* Did not get any output from lsof. */

		pclose(file);
		return;
	}

	if ( response[0] != 'p' ) {
		/* This file descriptor is not open. */

		pclose(file);
		return;
	}

	while (1) {
		if ( feof(file) || ferror(file) ) {
			break;
		}

		ptr = mxp_parse_lsof_output( file, process_id, -1,
					buffer, sizeof(buffer) );

		if ( ptr != NULL ) {
			mx_info( "%s", ptr );
		}
	}

	num_open_fds = mx_get_number_of_open_file_descriptors();

	pclose(file);

	mx_info( "max_fds = %d, num_open_fds = %d", max_fds, num_open_fds );

	return;
}

#else

MX_EXPORT void
mx_show_fd_names( unsigned long process_id )
{
	int i, max_fds, num_open_fds;
	char *ptr;
	char buffer[MXU_FILENAME_LENGTH+20];

	max_fds = mx_get_max_file_descriptors();

	num_open_fds = 0;

	for ( i = 0; i < max_fds; i++ ) {
		ptr = mx_get_fd_name( process_id, i, buffer, sizeof(buffer) );

		if ( ptr != NULL ) {
			num_open_fds++;

			mx_info( "%d - %s", i, ptr );
		}
	}

	mx_info( "max_fds = %d, num_open_fds = %d", max_fds, num_open_fds );

	return;
}

#endif

/*=========================================================================*/

/*-------------------------------------------------------------------------*/

#if defined(OS_LINUX)

#  if ( MX_GLIBC_VERSION < 2003006L )

#error use old polling method

#  else    /* Glibc > 2.3.6 */

	/*** Use Linux inotify ***/

#include <linux/inotify.h>

#define MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH \
			( 1024 * (sizeof(struct inotify_event) + 16) )

typedef struct {
	int inotify_file_descriptor;
	int inotify_watch_descriptor;
	char event_buffer[MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH];

} MXP_LINUX_INOTIFY_MONITOR;

MX_EXPORT mx_status_type
mx_create_file_monitor( MX_FILE_MONITOR **monitor_ptr,
			unsigned long access_type,
			char *filename )
{
	static const char fname[] = "mx_create_file_monitor()";

	MXP_LINUX_INOTIFY_MONITOR *linux_monitor;
	unsigned long flags;
	int saved_errno;

	if ( monitor_ptr == (MX_FILE_MONITOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	*monitor_ptr = (MX_FILE_MONITOR *) malloc( sizeof(MX_FILE_MONITOR) );

	if ( (*monitor_ptr) == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_FILE_MONITOR structure." );
	}

	linux_monitor = (MXP_LINUX_INOTIFY_MONITOR *)
				malloc( sizeof(MXP_LINUX_INOTIFY_MONITOR) );

	if ( linux_monitor == (MXP_LINUX_INOTIFY_MONITOR *) NULL ) {
		mx_free( *monitor_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MXP_LINUX_INOTIFY_MONITOR structure." );
	}

	(*monitor_ptr)->private_ptr = linux_monitor;


	linux_monitor->inotify_file_descriptor = inotify_init();

	if ( linux_monitor->inotify_file_descriptor < 0 ) {
		saved_errno = errno;

		mx_free( linux_monitor );
		mx_free( *monitor_ptr );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while invoking inotify_init().  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	/* FIXME: For the moment we ignore the value of 'access_type'. */

	linux_monitor->inotify_watch_descriptor = inotify_add_watch(
					linux_monitor->inotify_file_descriptor,
					filename,
					IN_CLOSE_WRITE | IN_MODIFY );

	if ( linux_monitor->inotify_watch_descriptor < 0 ) {
		saved_errno = errno;

		mx_free( linux_monitor );
		mx_free( *monitor_ptr );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while invoking inotify_add_watch().  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_file_monitor( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_delete_file_monitor()";

	MXP_LINUX_INOTIFY_MONITOR *linux_monitor;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	linux_monitor = monitor->private_ptr;

	if ( linux_monitor == (MXP_LINUX_INOTIFY_MONITOR *) NULL ) {
		mx_free( monitor );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_LINUX_INOTIFY_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );
	}

	(void) close( linux_monitor->inotify_file_descriptor );

	mx_free( linux_monitor );
	mx_free( monitor );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_bool_type
mx_file_has_changed( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_file_has_changed()";

	MXP_LINUX_INOTIFY_MONITOR *linux_monitor;
	struct inotify_event *inotify_event;
	size_t num_bytes_available, event_length;
	mx_status_type mx_status;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );

		return FALSE;
	}

	linux_monitor = monitor->private_ptr;

	if ( linux_monitor == (MXP_LINUX_INOTIFY_MONITOR *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_LINUX_INOTIFY_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );

		return FALSE;
	}

	/* How many bytes have been sent to the inotify file descriptor? */

	mx_status = mx_fd_num_input_bytes_available(
				linux_monitor->inotify_file_descriptor,
				&num_bytes_available );

	if ( mx_status.code != MXE_SUCCESS ) {
		return FALSE;
	}

	if ( num_bytes_available > MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH ) {
		num_bytes_available = MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH;
	}

	/* We only want to read complete event structures here, so we
	 * leave any not yet completed event structures in the fd buffer.
	 */

	length = read( linux_monitor->inotify_file_descriptor,
			linux_monitor->event_buffer,
			num_bytes_available );

	if ( length < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while reading from inotify fd %d "
		"for filename '%s'.  Errno = %d, error message = '%s'.",
			linux_monitor->inotify_file_descriptor,
			monitor->filename,
			saved_errno, strerror(saved_errno) );
	} else
	if ( length < num_bytes_available ) {
		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Short read of %lu bytes seen when %lu bytes were expected "
		"from inotify fd %d for file '%s'",
			length, num_bytes_available,
			linux_monitor->inotify_file_descriptor,
			monitor->filename );

		num_event_structures = length / sizeof(struct inotify_event);
	}

	end_of_buffer_ptr = linux_monitor->event_buffer
				+ MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH;

	inotify_event = linux_monitor->event_buffer;

	while ( inotify_event < end_of_buffer_ptr ) { 
	    if ( linux_monitor->inotify_watch_descriptor == inotify_event->wd ){
		return TRUE;
	    }

	    event_length = sizeof(struct inotify_event) + inotify_event->len;

	    inotify_event += event_length;
	}

	return FALSE;
}

#  endif

#elif defined(OS_WIN32)

	/*** Use Win32 FindFirstChangeNotification() ***/

#include <windows.h>

typedef struct {
	HANDLE change_handle;
} MXP_WIN32_FILE_MONITOR;

MX_EXPORT mx_status_type
mx_create_file_monitor( MX_FILE_MONITOR **monitor_ptr,
			unsigned long access_type,
			char *filename )
{
	static const char fname[] = "mx_create_file_monitor()";

	MXP_WIN32_FILE_MONITOR *win32_monitor;
	unsigned long flags;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( monitor_ptr == (MX_FILE_MONITOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	*monitor_ptr = (MX_FILE_MONITOR *) malloc( sizeof(MX_FILE_MONITOR) );

	if ( (*monitor_ptr) == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_FILE_MONITOR structure." );
	}

	win32_monitor = (MXP_WIN32_FILE_MONITOR *)
				malloc( sizeof(MXP_WIN32_FILE_MONITOR) );

	if ( win32_monitor == (MXP_WIN32_FILE_MONITOR *) NULL ) {
		mx_free( *monitor_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MXP_WIN32_FILE_MONITOR structure." );
	}

	(*monitor_ptr)->private_ptr = win32_monitor;

	/* FIXME: For the moment we ignore the value of 'access_type'. */

	flags = FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE;

	win32_monitor->change_handle = FindFirstChangeNotification(
					filename, FALSE, flags );

	if ( win32_monitor->change_handle == INVALID_HANDLE_VALUE ) {
		last_error_code = GetLastError();

		mx_free( win32_monitor );
		mx_free( *monitor_ptr );

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while invoking "
		"FindFirstChangeNotifcation() for file '%s'.  "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_file_monitor( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_delete_file_monitor()";

	MXP_WIN32_FILE_MONITOR *win32_monitor;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	win32_monitor = monitor->private_ptr;

	if ( win32_monitor == (MXP_WIN32_FILE_MONITOR *) NULL ) {
		mx_free( monitor );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_WIN32_FILE_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );
	}

	(void) FindCloseChangeNotification( win32_monitor->change_handle );

	mx_free( win32_monitor );
	mx_free( monitor );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_bool_type
mx_file_has_changed( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_file_has_changed()";

	MXP_WIN32_FILE_MONITOR *win32_monitor;
	BOOL os_status;
	DWORD wait_status, last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );

		return FALSE;
	}

	win32_monitor = monitor->private_ptr;

	if ( win32_monitor == (MXP_WIN32_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_WIN32_FILE_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );

		return FALSE;
	}

	/* Is a change notification available? */

	wait_status = WaitForSingleObject( win32_monitor->change_handle, 0 );

	switch( wait_status ) {
	case WAIT_OBJECT_0:
		/* Restart the notification. */

		os_status =
		    FindNextChangeNotification( win32_monitor->change_handle );

		/* The file has changed, so return TRUE. */

		return TRUE;
		break;

	case WAIT_TIMEOUT:
		/* Restart the notification. */

		os_status =
		    FindNextChangeNotification( win32_monitor->change_handle );

		/* The file has NOT changed, so return FALSE. */

		return FALSE;
		break;

	default:
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"WaitForSingleObject( %#lx, 0 ) for file '%s' failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			win32_monitor->change_handle,
			monitor->filename,
			last_error_code, message_buffer );

		return FALSE;
		break;
	}

	return FALSE;
}

#else

#error MX file monitors not yet implemented for this platform.

#endif


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

#define DEBUG_LSOF	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_stdint.h"
#include "mx_socket.h"
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
		/* Use select() instead.
		 *
		 * select() does not give us a way of knowing how many
		 * bytes are available, so we can only promise that there
		 * is at least 1 byte available.
		 */

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

	result = _getmaxstdio();

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
	intptr_t handle;
	int i, max_fds, used_fds;

	max_fds = _getmaxstdio();

	used_fds = 0;

	for ( i = 0 ; i < max_fds ; i++ ) {
		handle = _get_osfhandle( i );

		if ( handle != (-1) ) {
			used_fds++;
		}
	}

	return used_fds;
}

#else

#error mx_get_number_of_open_file_descriptors() has not been implemented for this platform.

#endif

/*=========================================================================*/

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS)

MX_EXPORT mx_bool_type
mx_fd_is_valid( int fd )
{
	int flags;

	flags = fcntl( fd, F_GETFD );

	if ( flags == -1 ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

#elif defined(OS_WIN32)

MX_EXPORT mx_bool_type
mx_fd_is_valid( int fd )
{
	intptr_t handle;

	handle = _get_osfhandle( fd );

	if ( handle == (-1) ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

#else

#error mx_fd_is_valid() has not been implemented for this platform.

#endif

/*=========================================================================*/

#if defined(OS_MACOSX)

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

#if DEBUG_LSOF
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

#if DEBUG_LSOF
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
	mx_bool_type inode_found;
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

	fd_type = -1;
	filename[0] = '\0';
	inode_found = FALSE;
	mode[0] = '\0';
	response_fd = -1;
	socket_protocol[0] = '\0';

	while (1) {
		mx_fgets( response, sizeof(response), file );

		if ( feof(file) || ferror(file) ) {
			break;
		}

#if DEBUG_LSOF
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
			inode_found = TRUE;
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
			if ( strcmp( response+1, "PIPE" ) == 0 ) {
				fd_type = MXP_LSOF_PIPE;
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
		strlcpy( buffer, filename, buffer_size );
		break;
	case MXP_LSOF_PIPE:
		if ( inode_found == FALSE ) {
			strlcpy( buffer, "pipe:", buffer_size );
		} else {
			mxp_lsof_get_pipe_peer( process_id, inode, &peer_pid,
				peer_command_name, sizeof(peer_command_name) );

			snprintf( buffer, buffer_size,
				"pipe: connected to PID %lu '%s'",
				peer_pid, peer_command_name );
		}
		break;
	case MXP_LSOF_SOCKET:
		snprintf( buffer, buffer_size,
			"%s: %s %s",
			socket_protocol, filename, socket_state );
		break;
	default:
		strlcpy( buffer, "???:", buffer_size );
		break;
	}

	return buffer;
}

#endif

/*=========================================================================*/

#if defined(OS_LINUX) || defined(OS_SOLARIS)

/* On Linux, use readlink() on the /proc/NNN/fd/NNN files.
 * On Solaris, use readline() on  the /proc/NNN/path/NNN files.
 */

MX_EXPORT char *
mx_get_fd_name( unsigned long process_id, int fd,
			char *buffer, size_t buffer_size )
{
	static char fname[] = "mx_get_fd_name()";

	char fd_pathname[MXU_FILENAME_LENGTH+1];
	int bytes_read, saved_errno;

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

#  if defined(OS_LINUX)
	snprintf( fd_pathname, sizeof(fd_pathname),
		"/proc/%lu/fd/%d",
		process_id, fd );

#  elif defined(OS_SOLARIS)
	snprintf( fd_pathname, sizeof(fd_pathname),
		"/proc/%lu/path/%d",
		process_id, fd );

#  else
#     error readlink() method not used for this build target.
#  endif

	bytes_read = readlink( fd_pathname, buffer, buffer_size );

	if ( bytes_read >= 0 ) {
		buffer[bytes_read] = '\0';
	} else {
		saved_errno = errno;

		switch( saved_errno ) {
		case ENOENT:
			return NULL;

		default:
			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Could not read the filename corresponding to "
			"file descriptor %d from /proc directory entry '%s'.  "
			"Errno = %d, error message = '%s'",
				fd, fd_pathname,
				saved_errno, strerror(saved_errno) );
			return NULL;
		}
	}

#  if defined(OS_LINUX)

	if ( strncmp( buffer, "socket:", 7 ) == 0 ) {
		union {
			struct sockaddr socket_address;
			struct sockaddr_in ip_address;
			struct sockaddr_in6 ipv6_address;
			struct sockaddr_un unix_address;
		} local;

		union {
			struct sockaddr socket_address;
			struct sockaddr_in ip_address;
			struct sockaddr_in6 ipv6_address;
			struct sockaddr_un unix_address;
		} peer;

		socklen_t local_length, peer_length;
		int os_status;
		int local_socket_type;
		int option_length;
		char socket_type_name[40];
		char temp_string[100];
		mx_bool_type socket_is_connected;

		/*----*/

		local_length = sizeof(local);

		os_status = getsockname( fd,
				(struct sockaddr *) &local,
				&local_length );

		if ( os_status == (-1) ) {
			saved_errno = errno;

			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Could not get address of local socket "
			"for socket %d.  Errno = %d, error message = '%s'",
				fd, saved_errno, strerror(saved_errno) );

			return NULL;
		}

		/*----*/

		option_length = sizeof(local_socket_type);

		os_status = getsockopt( fd, SOL_SOCKET, SO_TYPE,
					&local_socket_type,
					(socklen_t *) &option_length );

		if ( os_status == (-1) ) {
			saved_errno = errno;

			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Could not get local socket type for socket %d.  "
			"Errno = %d, error message = '%s'",
				fd, saved_errno, strerror(saved_errno) );

			return NULL;
		}

		switch( local_socket_type ) {
		case SOCK_STREAM:
			strlcpy( socket_type_name, "tcp",
				sizeof(socket_type_name) );
			break;
		case SOCK_DGRAM:
			strlcpy( socket_type_name, "udp",
				sizeof(socket_type_name) );
			break;
		case SOCK_RAW:
			strlcpy( socket_type_name, "raw",
				sizeof(socket_type_name) );
			break;
		}

		/*----*/

		peer_length = sizeof(peer);

		os_status = getpeername( fd,
				(struct sockaddr *) &peer,
				&peer_length );

		if ( os_status == (-1) ) {
			saved_errno = errno;

			if ( saved_errno == ENOTCONN ) {
				socket_is_connected = FALSE;
			} else {
				(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
				"Could not get address of remote socket peer "
				"for socket %d.  "
				"Errno = %d, error message = '%s'",
					fd, saved_errno, strerror(saved_errno));

				return NULL;
			}
		} else {
			socket_is_connected = TRUE;
		}

		/*----*/

		switch( local.socket_address.sa_family ) {
		case AF_INET:
			if ( socket_is_connected ) {
			    snprintf( buffer, buffer_size,
				"%s: %s:%d -> %s:%d",
				socket_type_name,
				inet_ntoa( local.ip_address.sin_addr ),
				(int) ntohs( local.ip_address.sin_port ),
				inet_ntoa( peer.ip_address.sin_addr ),
				(int) ntohs( peer.ip_address.sin_port ) );
			} else {
			    snprintf( buffer, buffer_size,
				"%s: %s:%d",
				socket_type_name,
				inet_ntoa( local.ip_address.sin_addr ),
				(int) ntohs( local.ip_address.sin_port ) );
			}
			break;

#if 0
		case AF_INET6:
			snprintf( buffer, buffer_size, "socket: %s",
				inet_ntop( AF_INET6,
					&(peer.ipv6_address.sin6_addr),
					temp_string, sizeof(temp_string) ) );
			break;
#endif

		case AF_UNIX:
			if ( socket_is_connected ) {
				snprintf( buffer, buffer_size, "unix: %s -> %s",
					local.unix_address.sun_path,
					peer.unix_address.sun_path );
			} else {
				snprintf( buffer, buffer_size, "unix: %s",
					local.unix_address.sun_path );
			}
			break;
		default:
			snprintf( temp_string, sizeof(temp_string),
				"unrecognized address family %d",
				peer.socket_address.sa_family );

			strlcat( buffer, temp_string, buffer_size );
			break;
		}
	}

#  endif /* OS_LINUX */

	return buffer;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

/* The method we use depends on which version of Windows we are running. */

static mx_bool_type tested_for_mx_get_fd_name = FALSE;

/* For Windows Vista and newer, we can use GetFileInformationByHandleEx(). */

typedef BOOL (*GetFileInformationByHandleEx_type)( HANDLE,
						FILE_INFO_BY_HANDLE_CLASS,
						LPVOID, DWORD );

static GetFileInformationByHandleEx_type
	ptrGetFileInformationByHandleEx = NULL;

static mx_status_type
mxp_get_fd_name_gfi_by_handle_ex( HANDLE fd_handle, int fd,
				char *buffer, size_t buffer_size )
{
	static const char fname[] = "mxp_get_fd_name_gfi_by_handle_ex()";

	BOOL os_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	size_t num_converted;
	errno_t errno_status;

	FILE_NAME_INFO *name_info;
	size_t name_info_length;
	mx_status_type mx_status;

	/* The definition of FILE_NAME_INFO only contains enough space
	 * for a single character filename, so we must allocate enough
	 * space for the filename at the end of the structure.
	 */

	name_info_length = MXU_FILENAME_LENGTH + 50;

	name_info = (FILE_NAME_INFO *) malloc( name_info_length );

	if ( name_info == (FILE_NAME_INFO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu byte buffer "
		"for a FILE_NAME_INFO structure.", name_info_length );
	}

	/* Now get information about this handle. */

	os_status = ptrGetFileInformationByHandleEx( fd_handle,
						FileNameInfo,
						name_info,
						name_info_length );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get filename information for file descriptor %d.  "
		"Win32 error code = %ld, error message = '%s'.",
			fd, last_error_code, message_buffer );

		mx_free( name_info );

		return mx_status;
	}

	/* Convert the wide character filename into single-byte characters. */

	errno_status = wcstombs_s( &num_converted,
				buffer,
				buffer_size,
				name_info->FileName,
				_TRUNCATE );

	if ( errno_status != 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to convert wide character filename for "
		"file descriptor %d.  "
		"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );

		mx_free( name_info );

		return mx_status;
	}

	mx_free( name_info );

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT char *
mx_get_fd_name( unsigned long process_id, int fd,
			char *buffer, size_t buffer_size )
{
	static char fname[] = "mx_get_fd_name()";

	HANDLE fd_handle;
	mx_status_type mx_status;

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

	if ( fd == fileno(stdin) ) {
		strlcpy( buffer, "<standard input>", buffer_size );
		return buffer;
	}

	if ( fd == fileno(stdout) ) {
		strlcpy( buffer, "<standard output>", buffer_size );
		return buffer;
	}

	if ( fd == fileno(stderr) ) {
		strlcpy( buffer, "<standard error>", buffer_size );
		return buffer;
	}

	fd_handle = (HANDLE) _get_osfhandle( fd );

	if ( fd_handle == INVALID_HANDLE_VALUE ) {
		return NULL;
	}

	if ( tested_for_mx_get_fd_name == FALSE ) {
		HMODULE hinst_kernel32;

		tested_for_mx_get_fd_name = TRUE;

		hinst_kernel32 = GetModuleHandle(TEXT("kernel32.dll"));

		if ( hinst_kernel32 == NULL ) {
			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to get a module handle for KERNEL32.DLL failed.  "
		"This should NEVER, EVER happen.  In fact, you should not be "
		"reading this message, since the computer should have "
		"crashed by now.  If you DO see this message, let "
		"William Lavender know." );
			return NULL;
		}

		ptrGetFileInformationByHandleEx =
			(GetFileInformationByHandleEx_type)
			GetProcAddress( hinst_kernel32,
				TEXT("GetFileInformationByHandleEx") );

		MX_DEBUG(-2,("%s: ptrGetFileInformationByHandleEx = %p",
			fname, ptrGetFileInformationByHandleEx));
	}

	/* Now we branch out to the various version specific methods. */

	if ( ptrGetFileInformationByHandleEx != NULL ) {
		mx_status = mxp_get_fd_name_gfi_by_handle_ex( fd_handle,
						fd, buffer, buffer_size );

		if ( mx_status.code != MXE_SUCCESS )
			return NULL;
	} else {
		MX_DEBUG(-2,("%s is not yet implemented for this platform.",
			fname));
	}

	return buffer;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_MACOSX)

/* Use the external 'lsof' program to get the fd name. */

MX_EXPORT char *
mx_get_fd_name( unsigned long process_id, int fd,
		char *buffer, size_t buffer_size )
{
	static const char fname[] = "mxp_get_fd_name_from_lsof()";

	FILE *file;
	int saved_errno;
	char command[ MXU_FILENAME_LENGTH + 20 ];
	char response[80];
	char *ptr;

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

	return ptr;
}

/*-------------------------------------------------------------------------*/

#elif 0

MX_EXPORT char *
mx_get_fd_name( unsigned long process_id, int fd,
		char *buffer, size_t buffer_size )
{
	static char fname[] = "mx_get_fd_name()";

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

	snprintf( buffer, buffer_size, "FD %d", fd );

	return buffer;
}

#else

#error mx_get_fd_name() has not yet been written for this platform.

#endif

/*=========================================================================*/

#if 0

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

#if defined(OS_LINUX) && ( MX_GLIBC_VERSION >= 2003006L )

	/*** Use Linux inotify via Glibc >= 2.3.6 ***/

/* WARNING: This is not yet tested, so it probably does not work. */

#include <sys/inotify.h>
#include <stddef.h>

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

	flags = ( IN_CLOSE_WRITE | IN_MODIFY );

	linux_monitor->inotify_watch_descriptor = inotify_add_watch(
					linux_monitor->inotify_file_descriptor,
					filename, flags );

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

/* The following was inspired by
 *
http://unix.derkeiler.com/Newsgroups/comp.unix.programmer/2011-03/msg00059.html
 *
 * but is structured differently.  Much of the complexity is to deal with
 * pointer alignment issues on some platforms like ARM.
 */

MX_EXPORT mx_bool_type
mx_file_has_changed( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_file_has_changed()";

	MXP_LINUX_INOTIFY_MONITOR *linux_monitor;
	size_t num_bytes_available, read_length;
	int saved_errno;
	char *buffer_ptr, *end_of_buffer_ptr;
	union {
		struct inotify_event event;
		char padding[ roundup(
				offsetof(struct inotify_event,name)
				+ NAME_MAX + 1,
					__alignof__(struct inotify_event) ) ];
	} u;
	uint32_t event_length;
	char *len_ptr;
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

	read_length = read( linux_monitor->inotify_file_descriptor,
			linux_monitor->event_buffer,
			num_bytes_available );

	if ( read_length < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while reading from inotify fd %d "
		"for filename '%s'.  Errno = %d, error message = '%s'.",
			linux_monitor->inotify_file_descriptor,
			monitor->filename,
			saved_errno, strerror(saved_errno) );
	} else
	if ( read_length < num_bytes_available ) {
		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Short read of %lu bytes seen when %lu bytes were expected "
		"from inotify fd %d for file '%s'",
			(unsigned long) read_length,
			(unsigned long) num_bytes_available,
			linux_monitor->inotify_file_descriptor,
			monitor->filename );
	}

	end_of_buffer_ptr = linux_monitor->event_buffer
				+ MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH;

	buffer_ptr = linux_monitor->event_buffer;

	while ( buffer_ptr < end_of_buffer_ptr ) { 
	    len_ptr = buffer_ptr + offsetof(struct inotify_event, len);

	    memcpy( &event_length, len_ptr, sizeof(event_length) );

	    event_length += offsetof( struct inotify_event, name );

	    memcpy( &(u.event), buffer_ptr, event_length );

	    if ( linux_monitor->inotify_watch_descriptor == u.event.wd ){
		return TRUE;
	    }

	    buffer_ptr += event_length;
	}

	return FALSE;
}

/*-------------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------------*/

#elif defined(OS_MACOSX)

#include <sys/event.h>
#include <sys/time.h>

typedef struct {
	int kqueue_fd;
	int file_fd;
} MXP_KQUEUE_VNODE_MONITOR;

MX_EXPORT mx_status_type
mx_create_file_monitor( MX_FILE_MONITOR **monitor_ptr,
			unsigned long access_type,
			char *filename )
{
	static const char fname[] = "mx_create_file_monitor()";

	MXP_KQUEUE_VNODE_MONITOR *kqueue_monitor;
	int num_events, saved_errno;
	struct kevent event;

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

	kqueue_monitor = (MXP_KQUEUE_VNODE_MONITOR *)
				malloc( sizeof(MXP_KQUEUE_VNODE_MONITOR) );

	if ( kqueue_monitor == (MXP_KQUEUE_VNODE_MONITOR *) NULL ) {
		mx_free( *monitor_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MXP_KQUEUE_VNODE_MONITOR structure." );
	}

	(*monitor_ptr)->private_ptr = kqueue_monitor;

	kqueue_monitor->kqueue_fd = kqueue();

	if ( kqueue_monitor->kqueue_fd < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Cannot create a new BSD kqueue event object.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	kqueue_monitor->file_fd = open( filename, O_RDONLY );

	if ( kqueue_monitor->file_fd < 0 ) {
		saved_errno = errno;

		close( kqueue_monitor->kqueue_fd );
		mx_free( kqueue_monitor );
		mx_free( *monitor_ptr );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open file '%s'.  Errno = %d, error message = '%s'",
			filename, saved_errno, strerror(saved_errno) );
	}

	/* FIXME: For the moment we ignore the value of 'access_type'. */

	/* Add this event to the kqueue. */

	event.ident = kqueue_monitor->file_fd;
	event.filter = EVFILT_VNODE;
	event.flags = EV_ADD | EV_ENABLE;
	event.fflags = NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB;
	event.data = 0;
	event.udata = 0;

	num_events = kevent( kqueue_monitor->kqueue_fd,
				&event, 1, NULL, 0, NULL );

	if ( num_events < 0 ) {
		saved_errno = errno;

		close( kqueue_monitor->kqueue_fd );
		mx_free( kqueue_monitor );
		mx_free( *monitor_ptr );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Cannot monitor changes to the file '%s'.  "
			"Errno = %d, error message = '%s'.",
				filename, saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_file_monitor( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_delete_file_monitor()";

	MXP_KQUEUE_VNODE_MONITOR *kqueue_monitor;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	kqueue_monitor = monitor->private_ptr;

	if ( kqueue_monitor == (MXP_KQUEUE_VNODE_MONITOR *) NULL ) {
		mx_free( monitor );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_KQUEUE_VNODE_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );
	}

	close( kqueue_monitor->kqueue_fd );
	close( kqueue_monitor->file_fd );

	mx_free( kqueue_monitor );
	mx_free( monitor );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_bool_type
mx_file_has_changed( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_file_has_changed()";

	MXP_KQUEUE_VNODE_MONITOR *kqueue_monitor;
	struct kevent event;
	int num_events, saved_errno;
	struct timespec timeout;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );

		return FALSE;
	}

	kqueue_monitor = monitor->private_ptr;

	if ( kqueue_monitor == (MXP_KQUEUE_VNODE_MONITOR *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_KQUEUE_VNODE_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );

		return FALSE;
	}

	timeout.tv_sec = 0;
	timeout.tv_nsec = 0;

	num_events = kevent( kqueue_monitor->kqueue_fd,
				NULL, 0,
				&event, 1,
				&timeout );

	if ( num_events < 0 ) {
		saved_errno = errno;
	} else
	if ( num_events > 0 ) {
		return TRUE;
	}

	return FALSE;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_SOLARIS)

/*
 * https://blogs.oracle.com/darren/entry/file_notification_in_opensolaris_and
 * contains a basic description of how to use port_associate() and friends.
 */

#include <port.h>

typedef struct {
	int port;
	file_obj_t file_object;
	int events_to_monitor;
} MXP_SOLARIS_PORT_MONITOR;

static mx_status_type
mxp_solaris_port_associate( MX_FILE_MONITOR *file_monitor )
{
	static const char fname[] = "mxp_solaris_port_associate()";

	MXP_SOLARIS_PORT_MONITOR *port_monitor;
	file_obj_t *file_object;
	int os_status, saved_errno;

	port_monitor = file_monitor->private_ptr;

	file_object = &(port_monitor->file_object);

	file_object->fo_name = file_monitor->filename;

	os_status = port_associate( port_monitor->port,
				PORT_SOURCE_FILE,
				(uintptr_t) file_object,
				port_monitor->events_to_monitor,
				NULL );

	if ( os_status == (-1) ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"A call to port_associate() for file '%s' failed.  "
		"Errno = %d, error message = '%s'.",
			file_monitor->filename,
			saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_create_file_monitor( MX_FILE_MONITOR **monitor_ptr,
			unsigned long access_type,
			char *filename )
{
	static const char fname[] = "mx_create_file_monitor()";

	MXP_SOLARIS_PORT_MONITOR *port_monitor;
	int saved_errno;
	mx_status_type mx_status;

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

	port_monitor = (MXP_SOLARIS_PORT_MONITOR *)
				malloc( sizeof(MXP_SOLARIS_PORT_MONITOR) );

	if ( port_monitor == (MXP_SOLARIS_PORT_MONITOR *) NULL ) {
		mx_free( *monitor_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MXP_SOLARIS_PORT_MONITOR structure." );
	}

	(*monitor_ptr)->private_ptr = port_monitor;

	port_monitor->port = port_create();

	if ( port_monitor->port == (-1) ) {
		saved_errno = errno;

		mx_free( port_monitor );
		mx_free( *monitor_ptr );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while invoking port_create().  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	port_monitor->events_to_monitor = FILE_MODIFIED;

	memset( &(port_monitor->file_object), 0, sizeof(file_obj_t) );

	mx_status = mxp_solaris_port_associate( *monitor_ptr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_delete_file_monitor( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_delete_file_monitor()";

	MXP_SOLARIS_PORT_MONITOR *port_monitor;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	port_monitor = monitor->private_ptr;

	if ( port_monitor == (MXP_SOLARIS_PORT_MONITOR *) NULL ) {
		mx_free( monitor );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_SOLARIS_PORT_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );
	}

	(void) close( port_monitor->port );

	mx_free( port_monitor );
	mx_free( monitor );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_bool_type
mx_file_has_changed( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_file_has_changed()";

	MXP_SOLARIS_PORT_MONITOR *port_monitor;
	port_event_t port_event;
	timespec_t timeout;
	int os_status, saved_errno;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );

		return FALSE;
	}

	port_monitor = monitor->private_ptr;

	if ( port_monitor == (MXP_SOLARIS_PORT_MONITOR *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_SOLARIS_PORT_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );

		return FALSE;
	}

	timeout.tv_sec = 0;
	timeout.tv_nsec = 0;

	os_status = port_get( port_monitor->port, &port_event, &timeout );

	if ( os_status == 0 ) {
		return TRUE;
	} else
	if ( os_status == (-1) ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case ETIME:
		case EINTR:
			return FALSE;
			break;
		default:
			(void) mx_error( MXE_FILE_IO_ERROR, fname,
		    "Checking for file status changes for file '%s' failed.  "
		    "Errno = %d, error message = '%s'.",
				monitor->filename,
				saved_errno, strerror(saved_errno) );

			return FALSE;
			break;
		}
	}

	return FALSE;
}
/*-------------------------------------------------------------------------*/

#elif defined(OS_LINUX)

error_not_found();

/*-------------------------------------------------------------------------*/

#else

#error MX file monitors not yet implemented for this platform.

#endif


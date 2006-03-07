/*
 * Name:    i_tty.c
 *
 * Purpose: MX driver for Unix TTY RS-232 devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_TTY_DEBUG	FALSE

#include <stdio.h>

#include "mx_osdef.h"

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_RTEMS)

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

#if HAVE_READV_WRITEV
#include <sys/uio.h>
#endif

#include "mx_util.h"
#include "mx_rs232.h"
#include "mx_record.h"
#include "mx_select.h"
#include "i_tty.h"

#if defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_IRIX) \
	|| defined(OS_SUNOS4) || defined(OS_AIX) || defined(OS_HPUX) \
	|| defined(OS_MACOSX) || defined(OS_BSD) || defined(OS_CYGWIN) \
	|| defined(OS_QNX) || defined(OS_RTEMS) || defined(OS_TRU64)
#    define USE_POSIX_TERMIOS	TRUE
#  else
#    error "No Unix TTY handling interface has been defined."
#endif

#if defined(OS_HPUX)
#include <sys/modem.h>
#endif

#if USE_POSIX_TERMIOS
#include <termios.h>

/* Define private functions to handle POSIX termios style terminal I/O. */

static mx_status_type mxi_tty_posix_termios_write_settings( MX_RS232 *rs232 );

static mx_status_type mxi_tty_posix_termios_initialize_settings(
							MX_RS232 *rs232 );

static mx_status_type mxi_tty_posix_termios_set_speed( MX_RS232 *rs232 );

static mx_status_type mxi_tty_posix_termios_set_word_size( MX_RS232 *rs232 );

static mx_status_type mxi_tty_posix_termios_set_parity( MX_RS232 *rs232 );

static mx_status_type mxi_tty_posix_termios_set_stop_bits( MX_RS232 *rs232 );

static mx_status_type mxi_tty_posix_termios_set_flow_control( MX_RS232 *rs232 );

#if MXI_TTY_DEBUG
static mx_status_type mxi_tty_posix_termios_print_settings( MX_RS232 *rs232 );
#endif

#endif /* USE_POSIX_TERMIOS */

/* Make some platform dependent macro definitions. */

/* Not everyone has a definition for ioctl(). */

#if defined(TIOCMGET)
#   if defined(OS_LINUX) || defined(OS_BSD) || defined(OS_MACOSX)
#      include <sys/ioctl.h>
#   else
       extern int ioctl( int fd, int request, ... );
#   endif

#   if !defined(TIOCM_LE)
#      define TIOCM_LE   TIOCM_DSR	/* For Cygwin... */
#   endif
#endif

/* On Linux, CMSPAR is used to select mark or space parity.  However, it is
 * only defined in /usr/include/asm/termbits.h, which application programs
 * are _not_ supposed to include.  The safer route is to just define it
 * here ourselves.
 */

#if defined(OS_LINUX)
#  if !defined(CMSPAR)
#     define CMSPAR	010000000000          /* mark or space parity */
#  endif
#endif

/* SGI Irix defines CNEW_RTS_CTS rather than the CRTSCTS that others use.
 * We simplify our lives here by just defining CRTSCTS as CNEW_RTSCTS.
 */

#if !defined(CRTSCTS)
#  if defined(CNEW_RTSCTS)
#     define CRTSCTS	CNEW_RTSCTS
#  endif
#endif

/* --- */

MX_RECORD_FUNCTION_LIST mxi_tty_record_function_list = {
	NULL,
	mxi_tty_create_record_structures,
	mxi_tty_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_tty_open,
	mxi_tty_close,
	NULL,
	mxi_tty_resynchronize
};

MX_RS232_FUNCTION_LIST mxi_tty_rs232_function_list = {
	mxi_tty_getchar,
	mxi_tty_putchar,
	mxi_tty_read,
	mxi_tty_write,
	NULL,
#if HAVE_READV_WRITEV
	mxi_tty_putline,
#else
	NULL,
#endif
	mxi_tty_num_input_bytes_available,
	mxi_tty_discard_unread_input,
	mxi_tty_discard_unwritten_output,
	mxi_tty_get_signal_state,
	mxi_tty_set_signal_state
};

MX_RECORD_FIELD_DEFAULTS mxi_tty_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_TTY_STANDARD_FIELDS
};

long mxi_tty_num_record_fields
		= sizeof( mxi_tty_record_field_defaults )
			/ sizeof( mxi_tty_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_tty_rfield_def_ptr
			= &mxi_tty_record_field_defaults[0];

/* ---- */

/* A private function for the use of the driver. */

static mx_status_type
mxi_tty_get_pointers( MX_RS232 *rs232,
			MX_TTY **tty,
			const char *calling_fname )
{
	const char fname[] = "mxi_tty_get_pointers()";

	MX_RECORD *tty_record;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( tty == (MX_TTY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_TTY pointer passed by '%s' was NULL.",
			calling_fname );
	}

	tty_record = rs232->record;

	if ( tty_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The tty_record pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*tty = (MX_TTY *) tty_record->record_type_struct;

	if ( *tty == (MX_TTY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TTY pointer for record '%s' is NULL.",
			tty_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_tty_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_tty_create_record_structures()";

	MX_RS232 *rs232;
	MX_TTY *tty;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	tty = (MX_TTY *) malloc( sizeof(MX_TTY) );

	if ( tty == (MX_TTY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TTY structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = tty;
	record->class_specific_function_list = &mxi_tty_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tty_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_tty_finish_record_initialization()";

	MX_TTY *tty;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	tty = (MX_TTY *) record->record_type_struct;

	if ( tty == (MX_TTY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TTY structure pointer for '%s' is NULL.", record->name );
	}

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the TTY device as being closed. */

	tty->file_handle = -1;

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tty_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_tty_open()";

	MX_RS232 *rs232;
	MX_TTY *tty;
	int status, flags, saved_errno;
	unsigned long do_not_change_port_settings;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 structure pointer is NULL." );
	}

	tty = (MX_TTY *) record->record_type_struct;

	if ( tty == (MX_TTY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_TTY structure pointer for '%s' is NULL.",
			record->name );
	}

	MX_DEBUG( 2,
("mxi_tty_open(): Somewhere in here we need to worry about TTY lock files."));

	/* If the tty device is currently open, close it. */

	if ( tty->file_handle >= 0 ) {
		status = close( tty->file_handle );

		if ( status != 0 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error while closing previously open TTY device '%s'.",
				record->name );
		}
	}

	/* Decide on the flags for the open() call. */

#if defined(O_NDELAY) && defined(F_SETFL)

	switch( rs232->flow_control ) {
	case MXF_232_NO_FLOW_CONTROL:
	case MXF_232_SOFTWARE_FLOW_CONTROL:
	case MXF_232_RTS_CTS_FLOW_CONTROL:
		/* A comment concerning MXF_232_RTS_CTS_FLOW_CONTROL:
		 *
		 * With normal (~CLOCAL) hardware flow control, as used
		 * by MXF_232_HARDWARE_FLOW_CONTROL, open will block if
		 * Data Carrier Detect is not asserted.  This is not good
		 * if we want to use RTS/CTS with a device that does not
		 * assert DCD.  Thus, the addition of O_NDELAY for 
		 * MXF_232_RTS_CTS_FLOW_CONTROL lets the open() complete
		 * even if DCD (carrier detect) is absent.
		 */
		flags = O_RDWR | O_NOCTTY | O_NDELAY;
		break;
	default:
		flags = O_RDWR | O_NOCTTY;
		break;
	}
#else
	flags = O_RDWR | O_NOCTTY;
#endif

	/* Now open the tty device. */

	tty->file_handle = open( tty->filename, flags );

	if ( tty->file_handle < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error opening TTY device '%s, device filename = '%s''.  "
		"Errno = %d, error text = '%s'",
			record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	/* If blocking was turned off for the open(), turn it back on. */

#if defined(O_NDELAY) && defined(F_SETFL)

	if ( flags & O_NDELAY ) {
		int old_flags, new_flags;

		/* Get rid of the O_NDELAY flags. */

		old_flags = fcntl( tty->file_handle, F_GETFL, 0 );

		if ( old_flags < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Error using F_GETFL for TTY device '%s'.  Errno = %d, error text = '%s'.",
				record->name, saved_errno,
				strerror( saved_errno ) );
		}

		new_flags = old_flags & ~O_NDELAY;

		status = fcntl( tty->file_handle, F_SETFL, new_flags );

		if ( status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Error using F_SETFL for TTY device '%s'.  Errno = %d, error text = '%s'.",
				record->name, saved_errno,
				strerror( saved_errno ) );
		}
	}

#endif

	do_not_change_port_settings =
		rs232->rs232_flags & MXF_232_DO_NOT_CHANGE_PORT_SETTINGS;

	if ( do_not_change_port_settings )
		return MX_SUCCESSFUL_RESULT;

	/* Set the serial port parameters. */

#if USE_POSIX_TERMIOS
	mx_status = mxi_tty_posix_termios_write_settings( rs232 );

#else  /* USE_POSIX_TERMIOS */

#error "No TTY parameter setting mechanism is defined."

#endif /* USE_POSIX_TERMIOS */

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_tty_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_tty_close()";

	MX_RS232 *rs232;
	MX_TTY *tty;
	int result, saved_errno;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) (record->record_class_struct);

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 structure pointer is NULL." );
	}

	tty = (MX_TTY *) (record->record_type_struct);

	if ( tty == (MX_TTY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_TTY structure pointer is NULL." );
	}

	MX_DEBUG( 2,
("mxi_tty_close(): Somewhere in here we need to worry about TTY lock files."));

	/* If the tty device is currently open, close it. */

	if ( tty->file_handle >= 0 ) {
		result = close( tty->file_handle );

		if ( result != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error while closing TTY device '%s'.  "
			"Errno = %d, error message = '%s'",
				record->name, saved_errno,
				strerror( saved_errno ) );
		}
	}

	tty->file_handle = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tty_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_tty_resynchronize()";

	MX_RS232 *rs232;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_tty_discard_unread_input( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_tty_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_tty_open( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_tty_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_tty_getchar()";

	MX_TTY *tty;
	int num_chars, saved_flags, new_flags, result, saved_errno;
	char c_temp;
	unsigned char c_mask;

	tty = (MX_TTY*) rs232->record->record_type_struct;

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {
		/* Set the file descriptor to be non-blocking. */

		saved_flags = fcntl( tty->file_handle, F_GETFL );

		new_flags = saved_flags & O_NONBLOCK;

		if ( new_flags != saved_flags ) {
			result = fcntl( tty->file_handle, F_SETFL,
					(long) new_flags );

			if ( result == -1 ) {
				saved_errno = errno;

				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"fcntl( ,F_SETFL, ) error.  Errno = %d, errno text = '%s'",
					saved_errno, strerror( saved_errno ) );
			}

			num_chars = (int) read( tty->file_handle, &c_temp, 1 );

			result = fcntl( tty->file_handle, F_SETFL,
					(long) saved_flags );

			if ( result == -1 ) {
				saved_errno = errno;

				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"fcntl( ,F_GETFL, ) error.  Errno = %d, errno text = '%s'",
					saved_errno, strerror( saved_errno ) );
			}
		} else {
			num_chars = (int) read( tty->file_handle, &c_temp, 1 );
		}
	} else {

		num_chars = (int) read( tty->file_handle, &c_temp, 1 );
	}

	/* === Mask the character to the appropriate number of bits. === */

	/* Note that the following code _assumes_ that chars are 8 bits. */

	c_mask = 0xff >> ( 8 - rs232->word_size );

	c_temp &= c_mask;

	*c = (char) c_temp;

#if MXI_TTY_DEBUG
	MX_DEBUG(-2, ("%s: num_chars = %d, c = 0x%x, '%c'",
		fname, num_chars, (unsigned char) *c, *c));
#endif

	/* mxi_tty_getchar() is often used to test whether there is
	 * input ready on the TTY port.  Normally, it is not desirable
	 * to broadcast a message to the world when this fails, so we
	 * use mx_error_quiet() rather than mx_error().
	 */

	if ( num_chars != 1 ) {
		return mx_error_quiet( MXE_NOT_READY, fname,
			"Failed to read a character from port '%s'.",
			rs232->record->name );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxi_tty_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_tty_putchar()";

	MX_TTY *tty;
	int num_chars;

#if MXI_TTY_DEBUG
	MX_DEBUG(-2,("%s invoked.  c = 0x%x, '%c'",
		fname, (unsigned char) c, c));
#endif

	tty = (MX_TTY*) (rs232->record->record_type_struct);

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Non-blocking TTY I/O not yet implemented.");
	} else {

		num_chars = (int) write( tty->file_handle, &c, 1 );
	}

	if ( num_chars != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Attempt to send a character to port '%s' failed.  character = '%c'",
			rs232->record->name, c );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxi_tty_read( MX_RS232 *rs232,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read )
{
	static const char fname[] = "mxi_tty_read()";

	MX_TTY *tty;
	char *buffer_ptr;
	ssize_t result;
	unsigned long total_bytes_read, bytes_left_to_read;
	int i, saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	buffer_ptr = buffer;
	total_bytes_read = 0;
	bytes_left_to_read = max_bytes_to_read;
	i = 0;

#if MXI_TTY_DEBUG
	MX_DEBUG(-2,("%s: About to read %lu bytes from TTY '%s'.",
		fname, (unsigned long) max_bytes_to_read,
		rs232->record->name));
#endif

	do {
		result = read( tty->file_handle,
				buffer_ptr, bytes_left_to_read );

		saved_errno = errno;

#if MXI_TTY_DEBUG
		if ( result < 0 ) {
			MX_DEBUG(-2,("%s: i = %d, result = %d, errno = %d",
				fname, i, (int) result, saved_errno ));
		} else {
			MX_DEBUG(-2,("%s: i = %d, result = %d",
				fname, i, (int) result ));
		}
#endif
		if (result > 0) {
			total_bytes_read += result;

			if ( bytes_left_to_read >= result ) {
				bytes_left_to_read -= result;

				buffer_ptr += result;
			} else {
				bytes_left_to_read = 0;

				buffer_ptr += bytes_left_to_read;
			}
		} else if ( result == 0 ) {
			bytes_left_to_read = 0;

			mx_status = mx_error_quiet( MXE_END_OF_DATA, fname,
				"End of file for RS-232 port '%s'.",
					rs232->record->name );
		} else {
			switch( saved_errno ) {
			case EAGAIN:
				return mx_error(MXE_TRY_AGAIN, fname,
				"No input was available for RS-232 port '%s'.",
					rs232->record->name );
			case EBADF:
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				  "RS-232 port '%s' is not open for reading.",
					rs232->record->name );
			case EINTR:
				return mx_error( MXE_INTERRUPTED, fname,
				"The read of RS-232 port '%s' was interrupted "
				"by a signal before any data was read.",
					rs232->record->name );
			case EIO:
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Unexpected I/O error for RS-232 port '%s'.",
					rs232->record->name );
			default:
				return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Unexpected errno value returned for RS-232 port '%s'.  "
		"errno = %d, error message = '%s'.", rs232->record->name,
					saved_errno, strerror( saved_errno ) );
			}
		}

		i++;

		if ( bytes_read != NULL ) {
			*bytes_read = total_bytes_read;
		}
	} while ( bytes_left_to_read > 0 );

#if MXI_TTY_DEBUG
	MX_DEBUG(-2,("%s: total_bytes_read = %lu", fname, total_bytes_read));

	{
		int bytes_to_show;

		if ( total_bytes_read <= 10 ) {
			bytes_to_show = total_bytes_read;
		} else {
			bytes_to_show = 10;
		}

		fprintf(stderr, "%s: Read ", fname);

		for ( i = 0; i < bytes_to_show; i++ ) {
			fprintf(stderr, "%#02x ", (unsigned char) (buffer[i]) );
		}

		if ( total_bytes_read <= 10 ) {
			fprintf(stderr, "\n");
		} else {
			fprintf(stderr, "... \n");
		}
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_tty_write( MX_RS232 *rs232,
		char *buffer,
		size_t max_bytes_to_write,
		size_t *bytes_written )
{
	static const char fname[] = "mxi_tty_write()";

	MX_TTY *tty;
	ssize_t result;
	unsigned long bytes_left_to_write, local_bytes_written;
	int saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_TTY_DEBUG
	{
		int i, bytes_to_show;

		if ( max_bytes_to_write <= 10 ) {
			bytes_to_show = max_bytes_to_write;
		} else {
			bytes_to_show = 10;
		}

		fprintf(stderr, "%s: Writing ", fname);

		for ( i = 0; i < bytes_to_show; i++ ) {
			fprintf(stderr, "%#02x ", (unsigned char) (buffer[i]) );
		}

		if ( max_bytes_to_write <= 10 ) {
			fprintf(stderr, "\n");
		} else {
			fprintf(stderr, "... \n");
		}
	}
#endif

	local_bytes_written = 0;

	bytes_left_to_write = max_bytes_to_write;

	while ( bytes_left_to_write > 0 ) {

		result = write( tty->file_handle, buffer, bytes_left_to_write );

		if ( result >= 0 ) {
			buffer += result;
			local_bytes_written += result;
			bytes_left_to_write -= result;
		} else {
			saved_errno = errno;

			switch( saved_errno ) {
			case EAGAIN:
			case EINTR:
				/* Just try again. */
				break;
			case EBADF:
				mx_status = mx_error(
					MXE_INTERFACE_IO_ERROR, fname,
				"RS-232 port '%s' is not open for reading.",
					rs232->record->name );
				break;
			case EIO:
				mx_status = mx_error(
					MXE_INTERFACE_IO_ERROR, fname,
				"Unexpected I/O error for RS-232 port '%s'.",
					rs232->record->name );
				break;
			default:
				mx_status = mx_error( MXE_UNKNOWN_ERROR, fname,
		"Unexpected errno value returned for RS-232 port '%s'.  "
		"errno = %d, error message = '%s'.", rs232->record->name,
					saved_errno, strerror( saved_errno ) );
				break;
			}
		}
		if ( mx_status.code != MXE_SUCCESS )
			break;
	}

	if ( bytes_written != NULL ) {
		*bytes_written = local_bytes_written;
	}

	return mx_status;
}

#if HAVE_READV_WRITEV

/* FIXME: mxi_tty_getline() can probably be implemented by reading into
 *        a two element iovec_array where the second element points to
 *        a buffer long enough to hold the expected read terminators.
 *        Presumably at startup time, MX_TTY structure can allocate memory
 *        for this extra read terminator buffer.
 */

MX_EXPORT mx_status_type
mxi_tty_putline( MX_RS232 *rs232,
		char *buffer,
		size_t *bytes_written )
{
	static const char fname[] = "mxi_tty_write()";

	MX_TTY *tty;
	struct iovec iovec_array[2];
	ssize_t result;
	size_t buffer_length;
	unsigned long bytes_left_to_write, original_bytes_left_to_write;
	unsigned long local_bytes_written;
	int saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	buffer_length = strlen( buffer );

	iovec_array[0].iov_base = buffer;
	iovec_array[0].iov_len = buffer_length;

	iovec_array[1].iov_base = rs232->write_terminator_array;
	iovec_array[1].iov_len = rs232->num_write_terminator_chars;

	local_bytes_written = 0;

	original_bytes_left_to_write =
		buffer_length + rs232->num_write_terminator_chars;

	bytes_left_to_write = original_bytes_left_to_write;

	while ( bytes_left_to_write > 0 ) {

		/* Write the buffer and terminators using scatter/gather I/O. */

		result = writev( tty->file_handle, iovec_array, 2 );

		if ( result >= 0 ) {
			local_bytes_written += result;

			if ( result > bytes_left_to_write ) {

				/* This should not be able to happen, but we
				 * will test for it anyway.
				 */

				mx_status = mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
		"The operating system claims to have written more bytes (%lu) "
		"to RS-232 port '%s' than the number we requested (%lu).  "
		"This definitely should not be able to happen!",
				(unsigned long) local_bytes_written,
				rs232->record->name,
				(unsigned long) original_bytes_left_to_write );

				bytes_left_to_write = 0;

				/* Go back to the top of the while() loop. */
				continue;
			}

			bytes_left_to_write -= result;

			if ( bytes_left_to_write == 0 ){

				/* Nothing left to do.  The loop will
				 * automatically terminate the next time
				 * we get back to the top of the while()
				 * loop.
				 */

			} else if ( local_bytes_written <= buffer_length ) {

				/* There is still some of the supplied
				 * buffer left to write.
				 */

				iovec_array[0].iov_base =
					buffer + local_bytes_written;

				iovec_array[0].iov_len =
					buffer_length - local_bytes_written;
			} else {
				/* All of the supplied buffer has been
				 * written, but there are still some of
				 * the write terminators left to write.
				 */

				/* Note that setting iovec_array[0].iov_base
				 * to NULL may not be safe, so we set it to
				 * a pointer we know to be safe.  However,
				 * the length of the vector is set to zero.
				 */

				iovec_array[0].iov_base = buffer;
				iovec_array[0].iov_len = 0;

				iovec_array[1].iov_base =
					rs232->write_terminator_array
					+ rs232->num_write_terminator_chars
					- bytes_left_to_write;

				iovec_array[1].iov_len = bytes_left_to_write;
			}
		} else {
			saved_errno = errno;

			switch( saved_errno ) {
			case EAGAIN:
			case EINTR:
				/* Just try again. */
				break;
			case EBADF:
				mx_status = mx_error(
					MXE_INTERFACE_IO_ERROR, fname,
				"RS-232 port '%s' is not open for reading.",
					rs232->record->name );
				break;
			case EIO:
				mx_status = mx_error(
					MXE_INTERFACE_IO_ERROR, fname,
				"Unexpected I/O error for RS-232 port '%s'.",
					rs232->record->name );
				break;
			default:
				mx_status = mx_error( MXE_UNKNOWN_ERROR, fname,
		"Unexpected errno value returned for RS-232 port '%s'.  "
		"errno = %d, error message = '%s'.", rs232->record->name,
					saved_errno, strerror( saved_errno ) );
				break;
			}
		}
		if ( mx_status.code != MXE_SUCCESS )
			break;
	}

	if ( bytes_written != NULL ) {
		*bytes_written = local_bytes_written;
	}

	return mx_status;
}

#endif /* HAVE_READV_WRITEV */

#if HAVE_FIONREAD_FOR_TTY_PORTS

/* Check for input with FIONREAD. */

MX_EXPORT mx_status_type
mxi_tty_fionread_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_tty_fionread_num_input_bytes_available()";

	MX_TTY *tty;
	int tty_fd, num_chars_available, status, saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	tty_fd = tty->file_handle;

	if ( tty_fd < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"TTY port device '%s' is not open.  TTY fd = %d",
		rs232->record->name, tty_fd);
	}

	status = ioctl( tty_fd, FIONREAD, &num_chars_available );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while checking TTY port '%s' to see if any "
		"input characters are available.  errno = %d, error text '%s'.",
			rs232->record->name, saved_errno,
			strerror(saved_errno) );
	}

	rs232->num_input_bytes_available = num_chars_available;

	return MX_SUCCESSFUL_RESULT;
}

#else

/* Check for input with select(). */

MX_EXPORT mx_status_type
mxi_tty_select_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
			"mxi_tty_select_num_input_bytes_available()";

	MX_TTY *tty;
	int tty_fd, select_status;
	struct timeval timeout;
	mx_status_type mx_status;

#if HAVE_FD_SET
	fd_set mask;

	FD_ZERO( &mask );

#else /* not HAVE_FD_SET */

	long mask;

#endif /* HAVE_FD_SET */

	MX_DEBUG( 2, ("%s invoked.", fname));

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	tty_fd = tty->file_handle;

	if ( tty_fd < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"TTY port device '%s' is not open.  TTY fd = %d",
		rs232->record->name, tty_fd);
	}

#if HAVE_FD_SET
	FD_SET( tty_fd, &mask );
#else
	mask = 1 << tty_fd;
#endif

	/* Set timeout to zero. */

	timeout.tv_usec = 0;
	timeout.tv_sec = 0;
	
	/* Do the test. */

	select_status = select( 1 + tty_fd, &mask, NULL, NULL, &timeout );

	if ( select_status ) {
		rs232->num_input_bytes_available = 1;
	} else {
		rs232->num_input_bytes_available = 0;
	}

	return MX_SUCCESSFUL_RESULT;
}
#endif

MX_EXPORT mx_status_type
mxi_tty_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tty_discard_unread_input()";

	int debug_level;
	unsigned long i, timeout, debug_flag;
	char c;
	mx_status_type mx_status;

	char buffer[100];
	unsigned long j, k;
	unsigned long buffer_length = sizeof(buffer) / sizeof(buffer[0]);

	MX_DEBUG( 2,("%s invoked.", fname));

	timeout = 10000L;

	/* If input is available, read until there is no more input.
	 * If we do this to a device that is constantly generating
	 * output, then we will never get to the end.  Thus, we have
	 * built a timeout into the function.
	 */

	rs232->transfer_flags &= ( ~ MXF_232_NOWAIT );

	debug_flag = rs232->transfer_flags & MXF_232_DEBUG;

	for ( i=0; i < timeout; i++ ) {

		mx_msleep(5);

		mx_status = mxi_tty_num_input_bytes_available( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( rs232->num_input_bytes_available == 0 ) {

			break;	/* Here we exit the for() loop. */

		} else {
			mx_status = mxi_tty_getchar( rs232, &c );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( debug_flag ) {
				if ( i < buffer_length ) {
					buffer[i] = c;
				}
			}
		}
	}

	if ( debug_flag ) {
		debug_level = mx_get_debug_level();

		if ( debug_level >= -2 ) {
			if ( i > buffer_length )
				k = buffer_length;
			else
				k = i;

			if ( i > 0 ) {
				fprintf(stderr,
				"%s: First %lu characters discarded were:\n\n",
					fname,k);

				for ( j = 0; j < k; j++ ) {
					c = buffer[j];

					if ( isprint( (int) c ) ) {
						fputc( c, stderr );
					} else {
						fprintf( stderr, "(0x%x)",
							(unsigned char) c );
					}
				}
				fprintf(stderr,"\n\n");
			}
		}
	}

	if ( i >= timeout ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"As many as %lu characters were read while trying to empty the input buffer.  "
"Perhaps this device continuously generates output?", timeout );

	} else if ( debug_flag && (i > 0) ) {

		MX_DEBUG(-2,("%s: %lu characters discarded.", fname, i));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tty_discard_unwritten_output( MX_RS232 *rs232 )
{
	/* Apparently, this is not generally possible with
	 * a Unix RS-232 device.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tty_get_signal_state( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tty_get_signal_state()";

	MX_TTY *tty;
	int status_bits;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, rs232->record->name ));

	tty = (MX_TTY*) rs232->record->record_type_struct;

	rs232->signal_state = 0;

	status_bits = 0;

#ifdef TIOCMGET
	ioctl( tty->file_handle, TIOCMGET, &status_bits );

	if ( status_bits & TIOCM_RTS ) {
		rs232->signal_state |= MXF_232_REQUEST_TO_SEND;
	}

	if ( status_bits & TIOCM_CTS ) {
		rs232->signal_state |= MXF_232_CLEAR_TO_SEND;
	}

	if ( status_bits & TIOCM_LE ) {
		rs232->signal_state |= MXF_232_DATA_SET_READY;
	}

	if ( status_bits & TIOCM_DTR ) {
		rs232->signal_state |= MXF_232_DATA_TERMINAL_READY;
	}

	if ( status_bits & TIOCM_CAR ) {
		rs232->signal_state |= MXF_232_DATA_CARRIER_DETECT;
	}

	if ( status_bits & TIOCM_RNG ) {
		rs232->signal_state |= MXF_232_RING_INDICATOR;
	}
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tty_set_signal_state( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tty_set_signal_state()";

	MX_TTY *tty;
	int status_bits;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, rs232->record->name ));

	tty = (MX_TTY*) rs232->record->record_type_struct;

	status_bits = 0;

#ifdef TIOCMGET
	/* Get the existing TTY state. */

	ioctl( tty->file_handle, TIOCMGET, &status_bits );

	/* RTS */

	if ( rs232->signal_state & MXF_232_REQUEST_TO_SEND ) {
		status_bits |= TIOCM_RTS;
	} else {
		status_bits &= ~TIOCM_RTS;
	}

	/* CTS */

	if ( rs232->signal_state & MXF_232_CLEAR_TO_SEND ) {
		status_bits |= TIOCM_CTS;
	} else {
		status_bits &= ~TIOCM_CTS;
	}

	/* DSR */

	if ( rs232->signal_state & MXF_232_DATA_SET_READY ) {
		status_bits |= TIOCM_LE;
	} else {
		status_bits &= ~TIOCM_LE;
	}

	/* DTR */

	if ( rs232->signal_state & MXF_232_DATA_TERMINAL_READY ) {
		status_bits |= TIOCM_DTR;
	} else {
		status_bits &= ~TIOCM_DTR;
	}

	/* CD */

	if ( rs232->signal_state & MXF_232_DATA_CARRIER_DETECT ) {
		status_bits |= TIOCM_CAR;
	} else {
		status_bits &= ~TIOCM_CAR;
	}

	/* RI */

	if ( rs232->signal_state & MXF_232_RING_INDICATOR ) {
		status_bits |= TIOCM_RNG;
	} else {
		status_bits &= ~TIOCM_RNG;
	}

	ioctl( tty->file_handle, TIOCMSET, &status_bits );
#endif
	return MX_SUCCESSFUL_RESULT;
}

/* ==== Unix version dependent I/O functions ==== */

#if USE_POSIX_TERMIOS

static mx_status_type
mxi_tty_posix_termios_write_settings( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tty_posix_termios_write_settings()";

	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for tty '%s'.", fname, rs232->record->name));

	mx_status = mxi_tty_posix_termios_initialize_settings( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_tty_posix_termios_set_speed( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_tty_posix_termios_set_word_size( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_tty_posix_termios_set_parity( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_tty_posix_termios_set_stop_bits( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_tty_posix_termios_set_flow_control( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_TTY_DEBUG
	mx_status = mxi_tty_posix_termios_print_settings( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	MX_DEBUG( 2,("%s complete for tty '%s'.", fname, rs232->record->name));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_tty_posix_termios_initialize_settings( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_tty_posix_termios_initialize_settings()";

	MX_TTY *tty;
	struct termios attr;
	int termios_status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for tty '%s'", fname, rs232->record->name));

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	termios_status = tcgetattr( tty->file_handle, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error getting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	/* Setup default TTY settings. */

	/* Set input modes */

	attr.c_iflag |= IGNBRK;		/* Ignore BREAK on input */
	attr.c_iflag |= IGNPAR;		/* Ignore framing and parity errors */

	attr.c_iflag &= ~BRKINT;	/* Read BREAK as character \0 */
	attr.c_iflag &= ~PARMRK;	/* Read framing/parity error as \0 */
	attr.c_iflag &= ~INPCK;		/* Do not enable input parity checking*/
	attr.c_iflag &= ~ISTRIP;	/* Do not strip off the eighth bit */
	attr.c_iflag &= ~INLCR;		/* Do not translate NL to CR on input */
	attr.c_iflag &= ~IGNCR;		/* Do not ignore CR on input */
	attr.c_iflag &= ~ICRNL;		/* Do not translate CR to NL on input */
	attr.c_iflag &= ~IXON;		/* No software flow control on input */
	attr.c_iflag &= ~IXOFF;		/* No software flow control on output */

#ifdef IUCLC
	attr.c_iflag &= ~IUCLC;		/* Do not translate uppercase
							to lowercase on input */
#endif
#ifdef IXANY
	attr.c_iflag &= ~IXANY;		/* Do not allow any character
							to restart output */
#endif
#ifdef IMAXBEL
	attr.c_iflag &= ~IMAXBEL;	/* Do not ring bell when the input
							queue is full */
#endif

	/* Set output modes */

	attr.c_oflag &= ~OPOST;		/* Do not enable output processing */

	/* Set control modes */

	attr.c_cflag |= CREAD;		/* Enable receiver */
	attr.c_cflag |= CLOCAL;		/* Ignore modem control lines */
	attr.c_cflag |= HUPCL;		/* Hangup on last close */

	attr.c_cflag &= ~CSIZE;
	attr.c_cflag |= CS8;		/* Default to 8 bit characters */

	attr.c_cflag &= ~PARENB;
	attr.c_cflag &= ~PARODD;	/* Default to no parity */

	attr.c_cflag &= ~CSTOPB;	/* Default to 1 stop bit */

#ifdef CMSPAR   /* For Linux */
	attr.c_cflag &= ~CMSPAR;	/* No mark or space parity */
#endif
#ifdef PAREXT   /* For System III derived systems (Solaris, Irix, etc.) */
	attr.c_cflag &= ~PAREXT;	/* No mark or space parity */
#endif

#ifdef CRTSCTS
	attr.c_cflag &= ~CRTSCTS;	/* No hardware flow control */
#endif

	/* Set local modes */

	attr.c_lflag &= ~ISIG;		/* Do not enable terminal
							generated signals */
	attr.c_lflag &= ~ICANON;	/* Disable canonical input mode */
	attr.c_lflag &= ~IEXTEN;	/* Disable extended input processing */

	attr.c_lflag &= ~ECHO;		/* Do not echo input characters */
	attr.c_lflag &= ~ECHOE;		/* Do not visually erase characters */
	attr.c_lflag &= ~ECHOK;		/* Do not echo kill line operations */
	attr.c_lflag &= ~ECHONL;	/* Do not echo newlines */

#ifdef ECHOCTL
	attr.c_lflag &= ~ECHOCTL;	/* Do not echo ctrl chars as ^X */
#endif
#ifdef ECHOKE
	attr.c_lflag &= ~ECHOKE;	/* Do not visually echo kill lines */
#endif
#ifdef ECHOPRT
	attr.c_lflag &= ~ECHOPRT;	/* Do not print characters when erased*/
#endif
#ifdef XCASE
	attr.c_lflag &= ~XCASE;		/* No upper/lower case changes */
#endif

	/* Set control chars */

	/* Read at least one character */

	if ( rs232->rs232_flags & MXF_232_POSIX_VMIN_FIX ) {

		/* FIXME - We need to better understand the circumstances
		 * for which the Posix VMIN fix is necessary.
		 */

		attr.c_cc[VMIN] = 0;
		attr.c_cc[VTIME] = 10;
	} else {
		attr.c_cc[VMIN] = 1;
		attr.c_cc[VTIME] = 0;
	}

	/* Apply the changed settings. */

	termios_status = tcsetattr( tty->file_handle, TCSADRAIN, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error setting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	MX_DEBUG( 2,("%s complete for tty '%s'", fname, rs232->record->name));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_tty_posix_termios_set_speed( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tty_posix_termios_set_speed()";

	MX_TTY *tty;
	struct termios attr;
	speed_t speed;
	int termios_status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for tty '%s'", fname, rs232->record->name));

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	termios_status = tcgetattr( tty->file_handle, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error getting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	switch( rs232->speed ) {
	case 0: 	speed = B0; 		break;
	case 50: 	speed = B50; 		break;
	case 75:	speed = B75;		break;
	case 110:	speed = B110;		break;
	case 134:	speed = B134;		break;
	case 150:	speed = B150;		break;
	case 200:	speed = B200;		break;
	case 300:	speed = B300;		break;
	case 600:	speed = B600;		break;
	case 1200:	speed = B1200;		break;
	case 1800:	speed = B1800;		break;
	case 2400:	speed = B2400;		break;
	case 4800:	speed = B4800;		break;
	case 9600:	speed = B9600;		break;
	case 19200:	speed = B19200;		break;
	case 38400:	speed = B38400;		break;
	case 57600:	speed = B57600;		break;
	case 115200:	speed = B115200;	break;

#ifdef B230400
	case 230400:	speed = B230400;	break;
#endif
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
    "Unsupported RS-232 port speed %ld requested for record '%s', tty '%s'.",
			rs232->speed, rs232->record->name, tty->filename );
	}

	/* Set the output speed. */

	termios_status = cfsetospeed( &attr, speed );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error setting output speed %ld for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.", rs232->speed,
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	/* Set the input speed. */

	termios_status = cfsetispeed( &attr, speed );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error setting input speed %ld for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.", rs232->speed,
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	/* Apply the changed settings. */

	termios_status = tcsetattr( tty->file_handle, TCSADRAIN, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error setting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	MX_DEBUG( 2,("%s complete for tty '%s'", fname, rs232->record->name));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_tty_posix_termios_set_word_size( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tty_posix_termios_set_word_size()";

	MX_TTY *tty;
	struct termios attr;
	int termios_status, saved_errno;
	unsigned long word_bit;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for tty '%s'", fname, rs232->record->name));

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	termios_status = tcgetattr( tty->file_handle, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error getting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	switch( rs232->word_size ) {
	case 5: 	word_bit = CS5;		break;
	case 6: 	word_bit = CS6;		break;
	case 7: 	word_bit = CS7;		break;
	case 8: 	word_bit = CS8;		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Unsupported RS-232 word size %ld requested for RS-232 record '%s', tty '%s'.",
			rs232->word_size, rs232->record->name, tty->filename );
	}

	attr.c_cflag &= ~CSIZE;
	attr.c_cflag |= word_bit;

	/* Apply the changed settings. */

	termios_status = tcsetattr( tty->file_handle, TCSADRAIN, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error setting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	MX_DEBUG( 2,("%s complete for tty '%s'", fname, rs232->record->name));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_tty_posix_termios_set_parity( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tty_posix_termios_set_parity()";

	MX_TTY *tty;
	struct termios attr;
	int termios_status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for tty '%s'", fname, rs232->record->name));

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	termios_status = tcgetattr( tty->file_handle, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error getting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	/* Note: Mark and space parity are not part of POSIX termios,
	 * but Linux and System III derived systems provide ways of
	 * generating them.  Unfortunately, they use different mechanisms,
	 * namely, CMSPAR and PAREXT.
	 *
	 * For Linux CMSPAR:
         *
	 *    ( PARENB | CMSPAR ) & ~PARODD    ==> Mark parity
	 *    ( PARENB | CMSPAR | PARODD )     ==> Space parity
	 *
	 *    Unfortunately, not all Linux drivers agree on this issue.
	 *    For example, the Axis Etrax drivers reverse the above sense.
	 *
	 * For PAREXT:
	 *
	 *    ( PARENB | PAREXT | PARODD )     ==> Mark parity
	 *    ( PARENB | PAREXT ) & ~PARODD    ==> Space parity
	 *
	 *    Note that some operating systems define PAREXT, but do not
	 *    actually implement the functionality behind it.
	 *
	 * Clearly, support of mark and space parity is spotty at best.
	 */

	/* Setup parity bits. */

	switch( rs232->parity ) {
	case MXF_232_NO_PARITY:
		attr.c_iflag &= ~INPCK;	    /* Disable input parity checking */
		attr.c_cflag &= ~PARENB;    /* Clear parity enable */
		attr.c_cflag &= ~PARODD;    /* Set this to a definite value */
#ifdef CMSPAR
		attr.c_cflag &= ~CMSPAR;    /* No mark or space parity */
#endif
#ifdef PAREXT
		attr.c_cflag &= ~PAREXT;    /* No mark or space parity */
#endif
		break;
	case MXF_232_EVEN_PARITY:
		attr.c_iflag |= INPCK;	    /* Enable input parity checking */
		attr.c_cflag |= PARENB;	    /* Enable parity */
		attr.c_cflag &= ~PARODD;    /* Select even parity */
#ifdef CMSPAR
		attr.c_cflag &= ~CMSPAR;    /* No mark or space parity */
#endif
#ifdef PAREXT
		attr.c_cflag &= ~PAREXT;    /* No mark or space parity */
#endif
		break;
	case MXF_232_ODD_PARITY:
		attr.c_iflag |= INPCK;	    /* Enable input parity checking */
		attr.c_cflag |= PARENB;	    /* Enable parity */
		attr.c_cflag |= PARODD;     /* Select odd parity */
#ifdef CMSPAR
		attr.c_cflag &= ~CMSPAR;    /* No mark or space parity */
#endif
#ifdef PAREXT
		attr.c_cflag &= ~PAREXT;    /* No mark or space parity */
#endif
		break;
	
	/* Mark and space parity. */
#ifdef CMSPAR
	case MXF_232_MARK_PARITY:
		attr.c_iflag |= INPCK;	    /* Enable input parity checking */
		attr.c_cflag |= PARENB;	    /* Enable parity */
		attr.c_cflag &= ~PARODD;    /* Select mark parity (part 1) */
		attr.c_cflag |= CMSPAR;     /* Select mark parity (part 2) */
		break;
	case MXF_232_SPACE_PARITY:
		attr.c_iflag |= INPCK;	    /* Enable input parity checking */
		attr.c_cflag |= PARENB;	    /* Enable parity */
		attr.c_cflag |= PARODD;     /* Select space parity (part 1) */
		attr.c_cflag |= CMSPAR;     /* Select space parity (part 2) */
		break;
#endif
#ifdef PAREXT
	case MXF_232_MARK_PARITY:
		attr.c_iflag |= INPCK;	    /* Enable input parity checking */
		attr.c_cflag |= PARENB;	    /* Enable parity */
		attr.c_cflag |= PARODD;    /* Select mark parity (part 1) */
		attr.c_cflag |= PAREXT;     /* Select mark parity (part 2) */
		break;
	case MXF_232_SPACE_PARITY:
		attr.c_iflag |= INPCK;	    /* Enable input parity checking */
		attr.c_cflag |= PARENB;	    /* Enable parity */
		attr.c_cflag &= ~PARODD;    /* Select space parity (part 1) */
		attr.c_cflag |= PAREXT;     /* Select space parity (part 2) */
		break;
#endif
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
    "Unsupported parity type '%c' requested for RS-232 record '%s', tty '%s'.",
			rs232->parity, rs232->record->name, tty->filename );
	}

	/* Apply the changed settings. */

	termios_status = tcsetattr( tty->file_handle, TCSADRAIN, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error setting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	MX_DEBUG( 2,("%s complete for tty '%s'", fname, rs232->record->name));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_tty_posix_termios_set_stop_bits( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tty_posix_termios_set_stop_bits()";

	MX_TTY *tty;
	struct termios attr;
	int termios_status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for tty '%s'", fname, rs232->record->name));

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	termios_status = tcgetattr( tty->file_handle, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error getting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	switch( rs232->stop_bits ) {
	case 1:
		attr.c_cflag &= ~CSTOPB;
		break;
	case 2:
		attr.c_cflag |= CSTOPB;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported number of stop bits (%ld) requested "
		"for RS-232 record '%s', tty '%s'.",
			rs232->stop_bits, rs232->record->name, tty->filename );
	}

	/* Apply the changed settings. */

	termios_status = tcsetattr( tty->file_handle, TCSADRAIN, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error setting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	MX_DEBUG( 2,("%s complete for tty '%s'", fname, rs232->record->name));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_tty_posix_termios_set_flow_control( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tty_posix_termios_set_flow_control()";

	MX_TTY *tty;
	struct termios attr;
	int termios_status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for tty '%s'", fname, rs232->record->name));

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	termios_status = tcgetattr( tty->file_handle, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error getting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	/**** Software flow control settings. ****/

#ifdef IXANY
	attr.c_iflag &= ~IXANY;		/* We never use IXANY. */
#endif

	switch( rs232->flow_control ) {
	case MXF_232_NO_FLOW_CONTROL:
	case MXF_232_HARDWARE_FLOW_CONTROL:
	case MXF_232_RTS_CTS_FLOW_CONTROL:

		attr.c_iflag &= ~(IXON | IXOFF);
		break;

	case MXF_232_SOFTWARE_FLOW_CONTROL:
	case MXF_232_BOTH_FLOW_CONTROL:

		attr.c_iflag |= (IXON | IXOFF);
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported flow control type '%c' requested "
		"for RS-232 record '%s', tty '%s'.",
		    rs232->flow_control, rs232->record->name, tty->filename );
	}

	/**** Hardware flow control settings. ****/

	/* Hardware flow control has less standardization.  Sometimes,
	 * even if the operating system defines the appropriate bits,
	 * the device drivers do not implement them.
	 */

	switch( rs232->flow_control ) {
	case MXF_232_NO_FLOW_CONTROL:
	case MXF_232_SOFTWARE_FLOW_CONTROL:

#ifdef CRTSCTS
		attr.c_cflag &= ~CRTSCTS;
#endif
		break;

	case MXF_232_HARDWARE_FLOW_CONTROL:
	case MXF_232_BOTH_FLOW_CONTROL:
	case MXF_232_RTS_CTS_FLOW_CONTROL:

#ifdef CRTSCTS
		attr.c_cflag |= CRTSCTS;
#endif
		break;
	}

	/* Apply the changed settings. */

	termios_status = tcsetattr( tty->file_handle, TCSADRAIN, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error setting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	MX_DEBUG( 2,("%s complete for tty '%s'", fname, rs232->record->name));

	return MX_SUCCESSFUL_RESULT;
}

#if MXI_TTY_DEBUG

static mx_status_type
mxi_tty_posix_termios_print_settings( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tty_posix_termios_print_settings()";

	MX_TTY *tty;
	struct termios attr;
	int termios_status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for tty '%s'", fname, rs232->record->name));

	mx_status = mxi_tty_get_pointers( rs232, &tty, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	termios_status = tcgetattr( tty->file_handle, &attr );

	if ( termios_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error getting termios attributes for record '%s', tty '%s'.  "
		"Errno = %d, error message = '%s'.",
			rs232->record->name, tty->filename,
			saved_errno, strerror( saved_errno ) );
	}

	MX_DEBUG(-2,("attr.c_iflag = %#lx", (unsigned long) attr.c_iflag));
	MX_DEBUG(-2,("attr.c_oflag = %#lx", (unsigned long) attr.c_oflag));
	MX_DEBUG(-2,("attr.c_cflag = %#lx", (unsigned long) attr.c_cflag));
	MX_DEBUG(-2,("attr.c_lflag = %#lx", (unsigned long) attr.c_lflag));
	MX_DEBUG(-2,("attr.c_cc[VMIN] = %d", attr.c_cc[VMIN]));
	MX_DEBUG(-2,("attr.c_cc[VTIME] = %d", attr.c_cc[VTIME]));
	MX_DEBUG(-2,("output speed = %#o, input speed = %#o",
		(int) cfgetospeed( &attr ), (int) cfgetispeed( &attr ) ));

	MX_DEBUG(-2,("%s complete for tty '%s'", fname, rs232->record->name));

	return MX_SUCCESSFUL_RESULT;
}

#endif /* MXI_TTY_DEBUG */

#endif /* USE_POSIX_TERMIOS */

#endif /* OS_UNIX || OS_CYGWIN || OS_RTEMS */

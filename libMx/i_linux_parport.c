/*
 * Name:    i_linux_parport.c
 *
 * Purpose: MX interface driver to control Linux parallel ports as digital
 *          input/output registers via the Linux parport driver.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_LINUX_PARPORT_DEBUG		FALSE

#include <stdio.h>

#if defined( OS_LINUX )

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/parport.h>
#include <linux/ppdev.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"

#include "i_linux_parport.h"

MX_RECORD_FUNCTION_LIST mxi_linux_parport_record_function_list = {
	NULL,
	mxi_linux_parport_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_linux_parport_open,
	mxi_linux_parport_close
};

MX_RECORD_FIELD_DEFAULTS mxi_linux_parport_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_LINUX_PARPORT_STANDARD_FIELDS
};

mx_length_type mxi_linux_parport_num_record_fields
		= sizeof( mxi_linux_parport_record_field_defaults )
			/ sizeof( mxi_linux_parport_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_linux_parport_rfield_def_ptr
			= &mxi_linux_parport_record_field_defaults[0];

static mx_status_type
mxi_linux_parport_get_pointers( MX_RECORD *record,
			MX_LINUX_PARPORT **linux_parport,
			const char *calling_fname )
{
	static const char fname[] = "mxi_linux_parport_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( linux_parport == (MX_LINUX_PARPORT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_LINUX_PARPORT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*linux_parport = (MX_LINUX_PARPORT *) (record->record_type_struct);

	if ( *linux_parport == (MX_LINUX_PARPORT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LINUX_PARPORT pointer for record '%s' is NULL.",
			record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

#define CLOSE_PARPORT_FD(a) \
		do { \
			(void) close( (a)->parport_fd ); \
			(a)->parport_fd = -1; \
		} while(0)

/*=== Public functions ===*/

MX_EXPORT mx_status_type
mxi_linux_parport_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_linux_parport_create_record_structures()";

	MX_LINUX_PARPORT *linux_parport;

	/* Allocate memory for the necessary structures. */

	linux_parport = (MX_LINUX_PARPORT *) malloc( sizeof(MX_LINUX_PARPORT) );

	if ( linux_parport == (MX_LINUX_PARPORT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_LINUX_PARPORT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = linux_parport;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	linux_parport->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_parport_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_linux_parport_open()";

	MX_LINUX_PARPORT *linux_parport;
	int i, length, status, saved_errno, data_port_direction;
	unsigned long parport_flags;
	mx_status_type mx_status;

	mx_status = mxi_linux_parport_get_pointers( record,
						&linux_parport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Force the 'linux_parport' mode name to upper case. */

	length = strlen( linux_parport->parport_mode_name );

	for ( i = 0; i < length; i++ ) {
	    if ( islower( (int) (linux_parport->parport_mode_name[i]) )) {
			linux_parport->parport_mode_name[i] = toupper(
			    (int) (linux_parport->parport_mode_name[i]) );
		}
	}

	MX_DEBUG( 2,("%s: Linux parport '%s' mode name = '%s'",
		fname, record->name, linux_parport->parport_mode_name));

	/* Figure out which parallel port mode this is. */

	if ( strcmp( linux_parport->parport_mode_name, "SPP" ) == 0 ) {
		linux_parport->parport_mode = IEEE1284_MODE_COMPAT;
	} else
	if ( strcmp( linux_parport->parport_mode_name, "EPP" ) == 0 ) {
		linux_parport->parport_mode = IEEE1284_MODE_EPP;
	} else
	if ( strcmp( linux_parport->parport_mode_name, "ECP" ) == 0 ) {
		linux_parport->parport_mode = IEEE1284_MODE_ECP;
	} else
	if ( strcmp( linux_parport->parport_mode_name, "COMPAT" ) == 0 ) {
		linux_parport->parport_mode = IEEE1284_MODE_COMPAT;
	} else
	if ( strcmp( linux_parport->parport_mode_name, "NIBBLE" ) == 0 ) {
		linux_parport->parport_mode = IEEE1284_MODE_NIBBLE;
	} else
	if ( strcmp( linux_parport->parport_mode_name, "BYTE" ) == 0 ) {
		linux_parport->parport_mode = IEEE1284_MODE_BYTE;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"'linux_parport' mode '%s' for record '%s' is not recognized "
		"as a legal parallel port mode.",
			linux_parport->parport_mode_name, record->name );
	}

	/* Open the parport device. */

	linux_parport->parport_fd = open( linux_parport->parport_name, O_RDWR );

	if ( linux_parport->parport_fd == -1 ) {
		saved_errno = errno;

		return mx_error( mx_errno_to_mx_status_code(saved_errno), fname,
"The attempt to open device '%s' by record '%s' failed.  "
"  errno = %d, error message = '%s'",
			linux_parport->parport_name, record->name,
			saved_errno, strerror( saved_errno ) );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Ask for exclusive access when the port is claimed. */

	status = ioctl( linux_parport->parport_fd, PPEXCL );

	if ( status != 0 ) {
		saved_errno = errno;

		CLOSE_PARPORT_FD( linux_parport );

		return mx_error( mx_errno_to_mx_status_code(saved_errno), fname,
"An attempt to prepare for exclusive access for device '%s' by "
"record '%s' failed.  errno = %d, error message = '%s'",
			linux_parport->parport_name, record->name,
			saved_errno, strerror( saved_errno ) );
	}

	/* Claim the port. */

	status = ioctl( linux_parport->parport_fd, PPCLAIM );

	if ( status != 0 ) {
	    saved_errno = errno;

	    CLOSE_PARPORT_FD( linux_parport );

	    if ( saved_errno == ENXIO ) {
		return mx_error( mx_errno_to_mx_status_code(saved_errno), fname,
"The hardware for device '%s' used by record '%s' either does not exist or "
"is already in use by another Linux kernel module such as 'lp'.\n"
"-> errno = %d, error message = '%s'",
			linux_parport->parport_name, record->name,
			saved_errno, strerror( saved_errno ) );
	    } else {
		return mx_error( mx_errno_to_mx_status_code(saved_errno), fname,
"An attempt to claim exclusive access for device '%s' by record '%s' failed.  "
"errno = %d, error message = '%s'",
			linux_parport->parport_name, record->name,
			saved_errno, strerror( saved_errno ) );
	    }
	}

	/* Negotiate the requested IEEE 1284 mode. */

	status = ioctl( linux_parport->parport_fd, PPNEGOT,
				&(linux_parport->parport_mode) );

	if ( status != 0 ) {
		saved_errno = errno;

		CLOSE_PARPORT_FD( linux_parport );

		return mx_error( mx_errno_to_mx_status_code(saved_errno), fname,
"An attempt to negotiate '%s' mode for device '%s' by "
"record '%s' failed.  errno = %d, error message = '%s'",
			linux_parport->parport_mode_name,
			linux_parport->parport_name, record->name,
			saved_errno, strerror( saved_errno ) );
	}

	/* Set the data direction for the data port. */

	parport_flags = linux_parport->parport_flags;

	if ( parport_flags & MXF_LINUX_PARPORT_DATA_PORT_REVERSE_MODE ) {
		data_port_direction = 1;
	} else {
		data_port_direction = 0;
	}

	status = ioctl( linux_parport->parport_fd, PPDATADIR,
				&data_port_direction );

	if ( status != 0 ) {
		saved_errno = errno;

		CLOSE_PARPORT_FD( linux_parport );

		return mx_error( mx_errno_to_mx_status_code(saved_errno), fname,
"An attempt to set the data port direction to %d for device '%s' by "
"record '%s' failed.  errno = %d, error message = '%s'",
				data_port_direction,
				linux_parport->parport_name, record->name,
				saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_parport_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_linux_parport_close()";

	MX_LINUX_PARPORT *linux_parport;
	int status, saved_errno, saved_fd;
	mx_status_type mx_status;

	mx_status = mxi_linux_parport_get_pointers( record,
						&linux_parport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( linux_parport->parport_fd != -1 ) {

		/* Restore the parport to compatibility mode if needed.
		 * Ignore any errors that occur.
		 */

		if ( linux_parport->parport_mode != IEEE1284_MODE_COMPAT ) {
			linux_parport->parport_mode = IEEE1284_MODE_COMPAT;

			(void) ioctl( linux_parport->parport_fd, PPNEGOT,
				&(linux_parport->parport_mode) );

		}

		/* Release the port.  Ignore any errors that occur. */

		(void) ioctl( linux_parport->parport_fd, PPRELEASE );

		/* Finally, close the file descriptor. */

		status = close( linux_parport->parport_fd );

		saved_errno = errno;
		saved_fd = linux_parport->parport_fd;

		linux_parport->parport_fd = -1;

		if ( status == -1 ) {
			return mx_error(
				mx_errno_to_mx_status_code(saved_errno), fname,
"The attempt to close device '%s' by record '%s' failed."
"  errno = %d, error message = '%s'",
				linux_parport->parport_name, record->name,
				saved_errno, strerror( saved_errno ) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === Driver specific functions === */

MX_EXPORT mx_status_type
mxi_linux_parport_read_port( MX_LINUX_PARPORT *linux_parport,
			int port_number, uint8_t *value )
{
	static const char fname[] = "mxi_linux_parport_read_port()";

	int port_ioctl, status, saved_errno;
	unsigned long invert_bits;
	unsigned char value_read;

	if ( linux_parport->parport_fd == -1 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		    "Device '%s' used by record '%s' is not currently open.",
			linux_parport->parport_name,
			linux_parport->record->name );
	}

	/* Read the value from the port. */

	switch( port_number ) {
	case MX_LINUX_PARPORT_DATA_PORT:
		port_ioctl = PPRDATA;
		break;
	case MX_LINUX_PARPORT_STATUS_PORT:
		port_ioctl = PPRSTATUS;
		break;
	case MX_LINUX_PARPORT_CONTROL_PORT:
		port_ioctl = PPRCONTROL;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal port number %d.", port_number );
		break;
	}

	status = ioctl( linux_parport->parport_fd, port_ioctl, &value_read );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( mx_errno_to_mx_status_code(saved_errno), fname,
"An attempt to read port %d of device '%s' by "
"record '%s' failed.  errno = %d, error message = '%s'",
			port_number, linux_parport->parport_name,
			linux_parport->record->name,
			saved_errno, strerror( saved_errno ) );
	}

	/* The value read from the port is handled differently depending
	 * on which port it is.
	 */

	invert_bits = linux_parport->parport_flags
				& MXF_LINUX_PARPORT_INVERT_INVERTED_BITS;

	switch( port_number ) {
	case MX_LINUX_PARPORT_DATA_PORT:
		linux_parport->data_port_value = value_read;
		break;
	case MX_LINUX_PARPORT_STATUS_PORT:
		linux_parport->status_port_value = value_read;

		if ( invert_bits ) {
			value_read ^= 0x80;
		}

		value_read >>= 3;

		value_read &= 0x1F;

		break;
	case MX_LINUX_PARPORT_CONTROL_PORT:
		linux_parport->control_port_value = value_read;

		if ( invert_bits ) {
			value_read ^= 0x0B;
		}

		value_read &= 0x0F;

		break;
	} 

	if ( value != NULL ) {
		*value = value_read;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_parport_write_port( MX_LINUX_PARPORT *linux_parport,
			int port_number, uint8_t value )
{
	static const char fname[] = "mxi_linux_parport_write_port()";

	int port_ioctl, status, saved_errno;
	unsigned long invert_bits;
	unsigned char value_written;

	if ( linux_parport->parport_fd == -1 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		    "Device '%s' used by record '%s' is not currently open.",
			linux_parport->parport_name,
			linux_parport->record->name );
	}

	/* The value to be written is handled differently depending
	 * on which port it is.
	 */

	invert_bits = linux_parport->parport_flags
				& MXF_LINUX_PARPORT_INVERT_INVERTED_BITS;

	switch( port_number ) {
	case MX_LINUX_PARPORT_DATA_PORT:
		port_ioctl = PPWDATA;

		linux_parport->data_port_value = value;
		break;
	case MX_LINUX_PARPORT_STATUS_PORT:
#if 0
		port_ioctl = PPWSTATUS;

		value &= 0x1F;

		value <<= 3;

		if ( invert_bits ) {
			value ^= 0x80;
		}

		linux_parport->status_port_value = value;
#else
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"The kernel device driver for device '%s' used by "
	"record '%s' does not allow the status port to be written to.",
			linux_parport->parport_name,
			linux_parport->record->name );
#endif
		break;
	case MX_LINUX_PARPORT_CONTROL_PORT:
		port_ioctl = PPWCONTROL;

		value &= 0x0F;

		if ( invert_bits ) {
			value ^= 0x0B;
		}

		linux_parport->control_port_value = value;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal port number %d.", port_number );
		break;
	} 

	value_written = value;

	/* Write the value to the port. */

	status = ioctl( linux_parport->parport_fd, port_ioctl, &value_written );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( mx_errno_to_mx_status_code(saved_errno), fname,
"An attempt to write to port %d of device '%s' by "
"record '%s' failed.  errno = %d, error message = '%s'",
			port_number, linux_parport->parport_name,
			linux_parport->record->name,
			saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_LINUX */


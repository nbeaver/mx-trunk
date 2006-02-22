/*
 * Name:    i_vxworks_rs232.c
 *
 * Purpose: MX driver for VxWorks RS-232 devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#if defined(OS_VXWORKS)

#define MXI_VXWORKS_RS232_DEBUG			FALSE

#include <stdlib.h>
#include <ioLib.h>
#include <sioLib.h>

#include "mx_unistd.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_vxworks_rs232.h"

MX_RECORD_FUNCTION_LIST mxi_vxworks_rs232_record_function_list = {
	NULL,
	mxi_vxworks_rs232_create_record_structures,
	mxi_vxworks_rs232_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_vxworks_rs232_open,
	mxi_vxworks_rs232_close
};

MX_RS232_FUNCTION_LIST mxi_vxworks_rs232_rs232_function_list = {
	mxi_vxworks_rs232_getchar,
	mxi_vxworks_rs232_putchar,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_vxworks_rs232_num_input_bytes_available,
	mxi_vxworks_rs232_discard_unread_input,
	mxi_vxworks_rs232_discard_unwritten_output,
	mxi_vxworks_rs232_get_signal_state,
	mxi_vxworks_rs232_set_signal_state
};

MX_RECORD_FIELD_DEFAULTS mxi_vxworks_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_VXWORKS_RS232_STANDARD_FIELDS
};

long mxi_vxworks_rs232_num_record_fields
		= sizeof( mxi_vxworks_rs232_record_field_defaults )
			/ sizeof( mxi_vxworks_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_vxworks_rs232_rfield_def_ptr
			= &mxi_vxworks_rs232_record_field_defaults[0];

/* ---- */

/* A private function for the use of the driver. */

static mx_status_type
mxi_vxworks_rs232_get_pointers( MX_RS232 *rs232,
			MX_VXWORKS_RS232 **vxworks_rs232,
			const char *calling_fname )
{
	const char fname[] = "mxi_vxworks_rs232_get_pointers()";

	MX_RECORD *vxworks_rs232_record;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vxworks_rs232 == (MX_VXWORKS_RS232 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VXWORKS_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	vxworks_rs232_record = rs232->record;

	if ( vxworks_rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The vxworks_rs232_record pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*vxworks_rs232 = (MX_VXWORKS_RS232 *)
		vxworks_rs232_record->record_type_struct;

	if ( *vxworks_rs232 == (MX_VXWORKS_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VXWORKS_RS232 pointer for record '%s' is NULL.",
			vxworks_rs232_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_vxworks_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_vxworks_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_VXWORKS_RS232 *vxworks_rs232;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	vxworks_rs232 = (MX_VXWORKS_RS232 *) malloc( sizeof(MX_VXWORKS_RS232) );

	if ( vxworks_rs232 == (MX_VXWORKS_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VXWORKS_RS232 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = vxworks_rs232;
	record->class_specific_function_list
				= &mxi_vxworks_rs232_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_rs232_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_vxworks_rs232_finish_record_initialization()";

	MX_VXWORKS_RS232 *vxworks_rs232;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	vxworks_rs232 = (MX_VXWORKS_RS232 *) (record->record_type_struct);

	if ( vxworks_rs232 == (MX_VXWORKS_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_VXWORKS_RS232 structure pointer for '%s' is NULL.",
			record->name );
	}

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the VXWORKS_RS232 device as being closed. */

	vxworks_rs232->file_handle = -1;

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_rs232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_vxworks_rs232_open()";

	MX_RS232 *rs232;
	MX_VXWORKS_RS232 *vxworks_rs232;
	int status, flags, saved_errno;
	int hardware_options, fiosetoptions_value;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) (record->record_class_struct);

	mx_status = mxi_vxworks_rs232_get_pointers( rs232,
						&vxworks_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the vxworks_rs232 device is currently open, close it. */

	if ( vxworks_rs232->file_handle >= 0 ) {
		status = close( vxworks_rs232->file_handle );

		if ( status != 0 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Error while closing previously open VxWorks RS-232 device '%s'.",
				record->name );
		}
	}

	/* Now open the vxworks_rs232 device. */

	flags = O_RDWR | O_NOCTTY;

	vxworks_rs232->file_handle = open( vxworks_rs232->filename, flags, 0 );

	if ( vxworks_rs232->file_handle < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error opening VxWorks RS-232 device '%s'.  "
			"errno = %d, error message = '%s'",
			record->name, saved_errno, strerror( saved_errno ) );
	}

	/* Set the the baud rate. */

	status = ioctl(vxworks_rs232->file_handle, SIO_BAUD_SET, rs232->speed);

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unable to set the RS-232 port speed for "
			"VxWorks RS-232 device '%s'.  "
			"errno = %d, error message = '%s'",
			record->name, saved_errno, strerror( saved_errno ) );
	}

	/* Set the other serial port parameters. */

	hardware_options = CREAD;

	switch( rs232->flow_control ) {
	case MXF_232_NO_FLOW_CONTROL:
	case MXF_232_SOFTWARE_FLOW_CONTROL:
		hardware_options |= CLOCAL;
		break;
	}

	switch( rs232->word_size ) {
	case 7:
		hardware_options |= CS7;
		break;
	case 8:
		hardware_options |= CS8;
		break;
	}

	switch( rs232->parity ) {
	case MXF_232_EVEN_PARITY:
		hardware_options |= PARENB;
		break;
	case MXF_232_ODD_PARITY:
		hardware_options |= ( PARENB | PARODD );
		break;
	case MXF_232_NO_PARITY:
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Parity type '%c' is not supported for VxWorks RS-232 port '%s'.",
			rs232->parity, record->name );
		break;
	}

	switch( rs232->stop_bits ) {
	case 2:
		hardware_options |= STOPB;
		break;
	}

	status = ioctl(vxworks_rs232->file_handle,
			SIO_HW_OPTS_SET, hardware_options );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unable to set RS-232 hardware parameters for "
			"VxWorks RS-232 device '%s'.  "
			"errno = %d, error message = '%s'",
			record->name, saved_errno, strerror( saved_errno ) );
	}

	/* Turning on software flow control (XON-XOFF) requires an additional
	 * command.
	 */

	switch( rs232->flow_control ) {
	case MXF_232_SOFTWARE_FLOW_CONTROL:
	case MXF_232_BOTH_FLOW_CONTROL:
		fiosetoptions_value = OPT_TANDEM;
		break;
	default:
		fiosetoptions_value = 0;
		break;
	}

	status = ioctl(vxworks_rs232->file_handle,
			FIOSETOPTIONS, fiosetoptions_value );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Failed in FIOSETOPTIONS ioctl() " 
			"for VxWorks RS-232 device '%s'.  "
			"errno = %d, error message = '%s'",
			record->name, saved_errno, strerror( saved_errno ) );
	}

	/* Discard any characters that may be in the incoming or outgoing
	 * character buffers.
	 */

	status = ioctl(vxworks_rs232->file_handle, FIOFLUSH, 0);

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Failed at discarding unread and unwritten characters "
			"for VxWorks RS-232 device '%s'.  "
			"errno = %d, error message = '%s'",
			record->name, saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_rs232_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_vxworks_rs232_close()";

	MX_RS232 *rs232;
	MX_VXWORKS_RS232 *vxworks_rs232;
	int result;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) (record->record_class_struct);

	mx_status = mxi_vxworks_rs232_get_pointers( rs232,
						&vxworks_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the VxWorks RS-232 device is currently open, close it. */

	if ( vxworks_rs232->file_handle >= 0 ) {
		result = close( vxworks_rs232->file_handle );

		if ( result != 0 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error while closing VxWorks RS-232 device '%s'.",
				record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_vxworks_rs232_getchar()";

	MX_VXWORKS_RS232 *vxworks_rs232;
	int num_chars;
	char c_temp;
	unsigned char c_mask;

	mx_status_type mx_status;

	mx_status = mxi_vxworks_rs232_get_pointers( rs232,
						&vxworks_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {

		mx_status = mxi_vxworks_rs232_num_input_bytes_available(rs232);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( rs232->num_input_bytes_available == 0 ) {
			c_temp = 0;
			num_chars = 0;
		} else {
			num_chars = read( vxworks_rs232->file_handle,
						&c_temp, 1 );
		}
	} else {
		num_chars = read( vxworks_rs232->file_handle, &c_temp, 1 );
	}

	/* === Mask the character to the appropriate number of bits. === */

	/* Note that the following code _assumes_ that chars are 8 bits. */

	c_mask = 0xff >> ( 8 - rs232->word_size );

	c_temp &= c_mask;

	*c = (char) c_temp;

#if MXI_VXWORKS_RS232_DEBUG
	MX_DEBUG(-2, ("%s: c = 0x%x, '%c'", *c, *c));
#endif

	/* mxi_vxworks_rs232_getchar() is often used to test whether there
	 * is input ready on the VxWorks RS-232 port.  Normally, it is not
	 * desirable * to broadcast a message to the world when this fails,
	 * so we use mx_error_quiet() rather than mx_error().
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
mxi_vxworks_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_vxworks_rs232_putchar()";

	MX_VXWORKS_RS232 *vxworks_rs232;
	int num_chars;
	mx_status_type mx_status;

	mx_status = mxi_vxworks_rs232_get_pointers( rs232,
						&vxworks_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_VXWORKS_RS232_DEBUG
	MX_DEBUG(-2, ("%s: c = 0x%x, '%c'", c, c));
#endif

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Non-blocking VxWorks RS-232 I/O not yet implemented.");
	} else {

		num_chars = write( vxworks_rs232->file_handle, &c, 1 );
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
mxi_vxworks_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_vxworks_rs232_num_input_bytes_available()";

	MX_VXWORKS_RS232 *vxworks_rs232;
	int fd, status, num_bytes_available, saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_vxworks_rs232_get_pointers( rs232,
						&vxworks_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	fd = vxworks_rs232->file_handle;

	if ( fd < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"VxWorks RS-232 port device '%s' is not open.  VxWorks RS-232 fd = %d",
			rs232->record->name, fd );
	}

	status = ioctl( fd, FIONREAD, (int) &num_bytes_available );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"An attempt to get the number of characters available "
			"for reading from VxWorks RS-232 device '%s' failed.  "
			"errno = %d, error message = '%s'", rs232->record->name,
			saved_errno, strerror( saved_errno ) );
	}

	rs232->num_input_bytes_available = num_bytes_available;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_rs232_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_vxworks_rs232_discard_unread_input()";

	MX_VXWORKS_RS232 *vxworks_rs232;
	int fd, status, saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_vxworks_rs232_get_pointers( rs232,
						&vxworks_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	fd = vxworks_rs232->file_handle;

	if ( fd < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"VxWorks RS-232 port device '%s' is not open.  VxWorks RS-232 fd = %d",
			rs232->record->name, fd );
	}

	status = ioctl( fd, FIORFLUSH, 0 );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"An attempt to discard unread input "
			"for VxWorks RS-232 device '%s' failed.  "
			"errno = %d, error message = '%s'", rs232->record->name,
			saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_rs232_discard_unwritten_output( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_vxworks_rs232_discard_unwritten_output()";

	MX_VXWORKS_RS232 *vxworks_rs232;
	int fd, status, saved_errno;
	mx_status_type mx_status;

	mx_status = mxi_vxworks_rs232_get_pointers( rs232,
						&vxworks_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	fd = vxworks_rs232->file_handle;

	if ( fd < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"VxWorks RS-232 port device '%s' is not open.  VxWorks RS-232 fd = %d",
			rs232->record->name, fd );
	}

	status = ioctl( fd, FIOWFLUSH, 0 );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"An attempt to discard unwritten output "
			"for VxWorks RS-232 device '%s' failed.  "
			"errno = %d, error message = '%s'", rs232->record->name,
			saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_rs232_get_signal_state( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_vxworks_rs232_get_signal_state()";

	MX_VXWORKS_RS232 *vxworks_rs232;
	int status_bits;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, rs232->record->name ));

	vxworks_rs232 = (MX_VXWORKS_RS232*) rs232->record->record_type_struct;

	rs232->signal_state = 0;

	status_bits = 0;

#ifdef TIOCMGET
	ioctl( vxworks_rs232->file_handle, TIOCMGET, &status_bits );

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
mxi_vxworks_rs232_set_signal_state( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_vxworks_rs232_set_signal_state()";

	MX_VXWORKS_RS232 *vxworks_rs232;
	int status_bits;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, rs232->record->name ));

	vxworks_rs232 = (MX_VXWORKS_RS232*) rs232->record->record_type_struct;

	status_bits = 0;

#ifdef TIOCMGET
	/* Get the existing VXWORKS_RS232 state. */

	ioctl( vxworks_rs232->file_handle, TIOCMGET, &status_bits );

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

	ioctl( vxworks_rs232->file_handle, TIOCMSET, &status_bits );
#endif
	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_VXWORKS */


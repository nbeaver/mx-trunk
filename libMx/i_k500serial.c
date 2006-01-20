/*
 * Name:    i_k500serial.c
 *
 * Purpose: MX driver for the Keithley 500-SERIAL RS232 to GPIB interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_gpib.h"
#include "mx_rs232.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "i_k500serial.h"

MX_RECORD_FUNCTION_LIST mxi_k500serial_record_function_list = {
	NULL,
	mxi_k500serial_create_record_structures,
	mxi_k500serial_finish_record_initialization,
	NULL,
	mxi_k500serial_print_interface_structure,
	NULL,
	NULL,
	mxi_k500serial_open
};

MX_GPIB_FUNCTION_LIST mxi_k500serial_gpib_function_list = {
	mxi_k500serial_open_device,
	mxi_k500serial_close_device,
	mxi_k500serial_read,
	mxi_k500serial_write,
	mxi_k500serial_interface_clear,
	mxi_k500serial_device_clear,
	mxi_k500serial_selective_device_clear,
	mxi_k500serial_local_lockout,
	mxi_k500serial_remote_enable,
	mxi_k500serial_go_to_local,
	mxi_k500serial_trigger,
	mxi_k500serial_wait_for_service_request,
	mxi_k500serial_serial_poll,
	mxi_k500serial_serial_poll_disable
};

MX_RECORD_FIELD_DEFAULTS mxi_k500serial_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_GPIB_STANDARD_FIELDS,
	MXI_K500SERIAL_STANDARD_FIELDS
};

long mxi_k500serial_num_record_fields
		= sizeof( mxi_k500serial_record_field_defaults )
			/ sizeof( mxi_k500serial_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_k500serial_rfield_def_ptr
			= &mxi_k500serial_record_field_defaults[0];

#define K500SERIAL_DEBUG	FALSE

/* ==== Private function for the driver's use only. ==== */

static mx_status_type
mxi_k500serial_get_record_pointers( MX_RECORD *gpib_record,
				MX_GPIB **gpib,
				const char *calling_fname )
{
	static const char fname[] = "mxi_k500serial_get_record_pointers()";

	if ( gpib_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( (gpib_record->mx_superclass != MXR_INTERFACE)
	  || (gpib_record->mx_class != MXI_GPIB)
	  || (gpib_record->mx_type != MXI_GPIB_K500SERIAL) ) {

		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' is not a Keithley 500-SERIAL interface record.",
			gpib_record->name );
	}

	if ( gpib == (MX_GPIB **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GPIB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*gpib = (MX_GPIB *) (gpib_record->record_class_struct);

	if ( *gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_GPIB pointer for record '%s' is NULL.",
			gpib_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_k500serial_get_pointers( MX_GPIB *gpib,
				MX_K500SERIAL **k500serial,
				MX_RECORD **rs232_record,
				const char *calling_fname )
{
	static const char fname[] = "mxi_k500serial_get_pointers()";

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GPIB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( k500serial == (MX_K500SERIAL **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_K500SERIAL pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( rs232_record == (MX_RECORD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The RS-232 MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*k500serial = (MX_K500SERIAL *) (gpib->record->record_type_struct);

	if ( *k500serial == (MX_K500SERIAL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_K500SERIAL pointer for record '%s' is NULL.",
			gpib->record->name );
	}

	*rs232_record = (*k500serial)->rs232_record;

	if ( *rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The RS-232 MX_RECORD pointer for record '%s' is NULL.",
			gpib->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ==== Public functions ==== */

MX_EXPORT mx_status_type
mxi_k500serial_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_k500serial_create_record_structures()";

	MX_GPIB *gpib;
	MX_K500SERIAL *k500serial;

	/* Allocate memory for the necessary structures. */

	gpib = (MX_GPIB *) malloc( sizeof(MX_GPIB) );

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GPIB structure." );
	}

	k500serial = (MX_K500SERIAL *) malloc( sizeof(MX_K500SERIAL) );

	if ( k500serial == (MX_K500SERIAL *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_K500SERIAL structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = gpib;
	record->record_type_struct = k500serial;
	record->class_specific_function_list
				= &mxi_k500serial_gpib_function_list;

	gpib->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_k500serial_finish_record_initialization( MX_RECORD *record )
{
	return mx_gpib_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxi_k500serial_print_interface_structure( FILE *file, MX_RECORD *record )
{
	MX_GPIB *gpib;
	MX_K500SERIAL *k500serial;
	char read_eos_char, write_eos_char;

	gpib = (MX_GPIB *) (record->record_class_struct);

	k500serial = (MX_K500SERIAL *) (record->record_type_struct);

	fprintf(file, "GPIB parameters for interface '%s':\n", record->name);
	fprintf(file, "  GPIB type              = K500SERIAL.\n\n");

	fprintf(file, "  name                   = %s\n", record->name);
	fprintf(file, "  rs232 port name        = %s\n",
					k500serial->rs232_record->name);
	fprintf(file, "  default I/O timeout    = %g\n",
					gpib->default_io_timeout);
	fprintf(file, "  default EOI mode       = %d\n",
					gpib->default_eoi_mode);

	read_eos_char = (char) ( gpib->default_read_terminator & 0xff );

	if ( isprint( (int) read_eos_char) ) {
		fprintf(file,
		      "  default read EOS char  = 0x%02x (%c)\n",
			read_eos_char, read_eos_char);
	} else {
		fprintf(file,
		      "  default read EOS char  = 0x%02x\n", read_eos_char);
	}

	write_eos_char = (char) ( gpib->default_write_terminator & 0xff );

	if ( isprint( (int) write_eos_char) ) {
		fprintf(file,
		      "  default write EOS char = 0x%02x (%c)\n",
			write_eos_char, write_eos_char);
	} else {
		fprintf(file,
		      "  default write EOS char = 0x%02x\n", write_eos_char);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_k500serial_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_k500serial_open()";

	MX_GPIB *gpib;
	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	MX_RS232 *rs232;
	char command[40];
	int i;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	gpib = NULL;

	mx_status = mxi_k500serial_get_record_pointers( record, &gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232 = (MX_RS232 *) (rs232_record->record_class_struct );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RS232 pointer for RS-232 record '%s' is NULL.",
			rs232_record->name );
	}

	(void) mx_rs232_discard_unread_input( rs232_record, K500SERIAL_DEBUG );

	/* Allow the 500-serial to adjust its baud rate by sending it
	 * 5 carriage returns separated by 0.1 second gaps.
	 */

	mx_msleep(100);

	for ( i = 0; i < 5; i++ ) {
		MX_DEBUG( 2,("%s: Sending CR.", fname));

		mx_status = mx_rs232_putchar( rs232_record,
						'\015', MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(100);
	}

	(void) mx_rs232_discard_unread_input( rs232_record, K500SERIAL_DEBUG );

	/* Send an init command to the converter. */

	mx_status = mx_rs232_putline( rs232_record,
					"I", NULL, K500SERIAL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn command echo off. */

	mx_status = mx_rs232_putline( rs232_record, "EC;0",
					NULL, K500SERIAL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set hardware handshaking. */

	switch( rs232->flow_control ) {
	case MXF_232_HARDWARE_FLOW_CONTROL:
	case MXF_232_BOTH_FLOW_CONTROL:
		strcpy( command, "H;1" );
		break;
	default:
		strcpy( command, "H;0" );
		break;
	}

	mx_status = mx_rs232_putline( rs232_record, command,
					NULL, K500SERIAL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set software handshaking. */

	switch( rs232->flow_control ) {
	case MXF_232_SOFTWARE_FLOW_CONTROL:
	case MXF_232_BOTH_FLOW_CONTROL:
		strcpy( command, "X;1" );
		break;
	default:
		strcpy( command, "X;0" );
		break;
	}

	mx_status = mx_rs232_putline( rs232_record, command,
					NULL, K500SERIAL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the 500-SERIAL what the serial line terminator is. */

	if ( rs232->read_terminators != rs232->write_terminators ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The RS-232 read terminators and write terminators must be the same for "
"a Keithley 500-SERIAL interface." );
	}

	switch( rs232->read_terminators ) {
	case 0x0a:				/* LF */
		strcpy( command, "TC;1" );
		break;
	case 0x0d:				/* CR */
		strcpy( command, "TC;2" );
		break;
	case 0x0a0d:				/* LF CR */
		strcpy( command, "TC;3" );
		break;
	case 0x0d0a:				/* CR LF */
		strcpy( command, "TC;4" );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The RS-232 line termination characters '%lx' are not compatible with "
"a Keithley 500-SERIAL interface.", rs232->read_terminators );
		break;
	}

	mx_status = mx_rs232_putline( rs232_record, command,
					NULL, K500SERIAL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the 500-SERIAL what the GPIB bus termination character
	 * must be.
	 */

	if ( gpib->default_read_terminator
				!= gpib->default_write_terminator ) {

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The GPIB read EOS character and write EOS character must be the same for "
"a Keithley 500-SERIAL interface." );
	}

	switch( gpib->default_read_terminator ) {
	case 0x0a:				/* LF */
		strcpy( command, "TB;1" );
		break;
	case 0x0d:
		strcpy( command, "TB;2" );	/* CR */
		break;
	case 0x0a0d:
		strcpy( command, "TB;3" );	/* LF CR */
		break;
	case 0x0d0a:
		strcpy( command, "TB;4" );	/* CR LF */
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The GPIB EOS terminator '%lx' is not compatible with "
"a Keithley 500-SERIAL interface.", gpib->default_read_terminator );
		break;
	}

	mx_status = mx_rs232_putline( rs232_record, command,
					NULL, K500SERIAL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the EOI mode for the 500-SERIAL. */

	switch( gpib->default_eoi_mode ) {
	case 1:
		strcpy( command, "EO;1" );	/* EOI on. */
		break;
	case 0:
		strcpy( command, "EO;0" ); 	/* EOI off. */
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal GPIB EOI mode %d", gpib->default_eoi_mode );
		break;
	}

	mx_status = mx_rs232_putline( rs232_record, command,
					NULL, K500SERIAL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait a bit to make sure the commands have been sent and then
	 * throw away any garbage characters on the serial line.
	 */

	mx_msleep(500);

	mx_status = mx_rs232_discard_unread_input( rs232_record,
						K500SERIAL_DEBUG);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	gpib->read_buffer_length = 0;
	gpib->write_buffer_length = 0;

	mx_status = mx_gpib_setup_communication_buffers( record );

	return mx_status;
}

/* ========== Device specific calls ========== */

MX_EXPORT mx_status_type
mxi_k500serial_open_device( MX_GPIB *gpib, int address )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_k500serial_close_device( MX_GPIB *gpib, int address )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_k500serial_read( MX_GPIB *gpib,
		int address,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		int flags )
{
	static const char fname[] = "mxi_k500serial_read()";

	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "EN;%02d", address );

	mx_status = mx_rs232_putline( rs232_record, command, NULL, flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( rs232_record, buffer, max_bytes_to_read,
					bytes_read, flags );

	return mx_status;
}

#define MX_PREFIX_BUFFER_LENGTH   6

MX_EXPORT mx_status_type
mxi_k500serial_write( MX_GPIB *gpib,
		int address,
		char *buffer,
		size_t bytes_to_write,
		size_t *bytes_written,
		int flags )
{
	static const char fname[] = "mxi_k500serial_write()";

	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	char prefix[ MX_PREFIX_BUFFER_LENGTH + 1 ];
	size_t device_bytes_written;
	mx_status_type mx_status;

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the 500-SERIAL command prefix. */

	sprintf( prefix, "OA;%02d;", address );

	mx_status = mx_rs232_write( rs232_record, prefix,
					MX_PREFIX_BUFFER_LENGTH, NULL, flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putline( rs232_record, buffer,
					&device_bytes_written, flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_written != NULL ) {
		*bytes_written = device_bytes_written
					+ MX_PREFIX_BUFFER_LENGTH;
	}

	return MX_SUCCESSFUL_RESULT;
}

#undef MX_PREFIX_BUFFER_LENGTH

MX_EXPORT mx_status_type
mxi_k500serial_interface_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_k500serial_interface_clear()";

	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	mx_status_type mx_status;

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putline( rs232_record,
					"I", NULL, K500SERIAL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_k500serial_device_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_k500serial_device_clear()";

	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	mx_status_type mx_status;

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putline( rs232_record,
					"C", NULL, K500SERIAL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_k500serial_selective_device_clear( MX_GPIB *gpib, int address )
{
	static const char fname[] = "mxi_k500serial_selective_device_clear()";

	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "C;%02d", address );

	mx_status = mx_rs232_putline( rs232_record, command,
						NULL, K500SERIAL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_k500serial_local_lockout( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_k500serial_local_lockout()";

	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	mx_status_type mx_status;

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putline( rs232_record,
					"LL", NULL, K500SERIAL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_k500serial_remote_enable( MX_GPIB *gpib, int address )
{
	static const char fname[] = "mxi_k500serial_remote_enable()";

	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "RE;%02d", address );

	mx_status = mx_rs232_putline( rs232_record, command,
						NULL, K500SERIAL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_k500serial_go_to_local( MX_GPIB *gpib, int address )
{
	static const char fname[] = "mxi_k500serial_go_to_local()";

	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "L;%02d", address );

	mx_status = mx_rs232_putline( rs232_record, command,
						NULL, K500SERIAL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_k500serial_trigger( MX_GPIB *gpib, int address )
{
	static const char fname[] = "mxi_k500serial_trigger_device()";

	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	char command[20];
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the trigger command. */

	if ( address < 0 ) {
		strcpy( command, "TR" );
	} else {
		sprintf( command, "TR;%02d", address );
	}

	mx_status = mx_rs232_putline( rs232_record, command,
						NULL, K500SERIAL_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_k500serial_wait_for_service_request( MX_GPIB *gpib, double timeout )
{
	static const char fname[] = "mxi_k500serial_wait_for_service_request()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Not yet implemented." );
}

MX_EXPORT mx_status_type
mxi_k500serial_serial_poll( MX_GPIB *gpib, int address,
				unsigned char *serial_poll_byte)
{
	static const char fname[] = "mxi_k500serial_serial_poll()";

	MX_K500SERIAL *k500serial;
	MX_RECORD *rs232_record;
	char command[20];
	char response[20];
	unsigned short short_value;
	int num_items;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	mx_status = mxi_k500serial_get_pointers( gpib,
				&k500serial, &rs232_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the serial poll command. */

	sprintf( command, "SP;%02d", address );

	mx_status = mx_rs232_putline( rs232_record, command,
						NULL, K500SERIAL_DEBUG );

	/* Get the serial poll byte. */

	mx_status = mx_rs232_getline( rs232_record,
				response, sizeof(response) - 1,
				NULL, K500SERIAL_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%hu", &short_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Serial poll byte not seen in response '%s' to command '%s'",
			response, command );
	}

	*serial_poll_byte = (unsigned char) ( short_value & 0xff );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_k500serial_serial_poll_disable( MX_GPIB *gpib )
{
	/* Apparently this function is not needed for this interface. */

	return MX_SUCCESSFUL_RESULT;
}


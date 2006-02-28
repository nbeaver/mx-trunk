/*
 * Name:    i_wago750_serial.c
 *
 * Purpose: MX driver for Wago 750-65x serial interface modules.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_WAGO750_SERIAL_DEBUG	FALSE

#define MX_WAGO_DEBUG(x)  MX_DEBUG( 2,x)

#define MX_WAGO_DEBUG_IO(x)  MX_DEBUG( 2,x)

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "mx_constants.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_key.h"
#include "mx_rs232.h"
#include "mx_modbus.h"
#include "i_wago750_serial.h"

#if (0 & MXI_WAGO750_SERIAL_DEBUG)
#  define MXI_WAGO750_SERIAL_PAUSE(x) \
	do { \
	    mx_info("!!!!! PAUSE in %s.  PRESS ANY KEY...",(x)); \
	    (void) mx_getch(); \
	} while (0)
#else
#  define MXI_WAGO750_SERIAL_PAUSE(x)
#endif

MX_RECORD_FUNCTION_LIST mxi_wago750_serial_record_function_list = {
	NULL,
	mxi_wago750_serial_create_record_structures,
	mxi_wago750_serial_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_wago750_serial_open,
	NULL,
	NULL,
	mxi_wago750_serial_open
};

MX_RS232_FUNCTION_LIST mxi_wago750_serial_rs232_function_list = {
	mxi_wago750_serial_getchar,
	mxi_wago750_serial_putchar,
	NULL,
	mxi_wago750_serial_write,
	NULL,
	mxi_wago750_serial_putline,
	mxi_wago750_serial_num_input_bytes_available,
	mxi_wago750_serial_discard_unread_input,
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxi_wago750_serial_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_WAGO750_SERIAL_STANDARD_FIELDS
};

long mxi_wago750_serial_num_record_fields
		= sizeof( mxi_wago750_serial_record_field_defaults )
			/ sizeof( mxi_wago750_serial_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_wago750_serial_rfield_def_ptr
			= &mxi_wago750_serial_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_wago750_serial_get_pointers( MX_RS232 *rs232,
			MX_WAGO750_SERIAL **wago750_serial,
			const char *calling_fname )
{
	static const char fname[] = "mxi_wago750_serial_get_pointers()";

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( wago750_serial == (MX_WAGO750_SERIAL **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_WAGO750_SERIAL pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( rs232->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*wago750_serial = (MX_WAGO750_SERIAL *)
				rs232->record->record_type_struct;

	if ( *wago750_serial == (MX_WAGO750_SERIAL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WAGO750_SERIAL pointer for record '%s' is NULL.",
			rs232->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_wago750_serial_read_status_byte( MX_WAGO750_SERIAL *wago750_serial,
					uint8_t *status_byte,
					uint8_t *data_byte_0 )
{
	static const char fname[] = "mxi_wago750_serial_read_status_byte()";

	uint16_t register_value;
	mx_status_type mx_status;

	switch( wago750_serial->bus_record->mx_class ) {
	case MXI_MODBUS:
		mx_status = mx_modbus_read_input_registers(
				wago750_serial->bus_record,
				wago750_serial->status_address,
				1, &register_value );

		*status_byte = register_value & 0xff;

		*data_byte_0 = ( register_value >> 8 ) & 0xff;

		MX_WAGO_DEBUG_IO(
		("STATUS: register_value = %#04x, status_byte = %#02x",
			register_value, *status_byte ));
		break;
	default:
		mx_status = mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support by MX Wago serial interface '%s' for bus driver "
		"type '%s' used by record '%s' is not yet implemented.",
			wago750_serial->record->name,
			mx_get_driver_name( wago750_serial->bus_record ),
			wago750_serial->bus_record->name );
		break;
	}

	MXI_WAGO750_SERIAL_PAUSE(fname);

	return mx_status;
}

static mx_status_type
mxi_wago750_serial_write_control_byte( MX_WAGO750_SERIAL *wago750_serial,
					uint8_t control_byte,
					uint8_t data_byte_0 )
{
	static const char fname[] = "mxi_wago750_serial_write_control_byte()";

	uint16_t register_value;
	mx_status_type mx_status;

	switch( wago750_serial->bus_record->mx_class ) {
	case MXI_MODBUS:
		register_value = control_byte;

		register_value |= ( data_byte_0 << 8 );

		MX_WAGO_DEBUG_IO(
		("CONTROL: register_value = %#04x, control_byte = %#02x",
			register_value, control_byte));

		mx_status = mx_modbus_write_single_register(
				wago750_serial->bus_record,
				wago750_serial->control_address,
				register_value );
		break;
	default:
		mx_status = mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support by MX Wago serial interface '%s' for bus driver "
		"type '%s' used by record '%s' is not yet implemented.",
			wago750_serial->record->name,
			mx_get_driver_name( wago750_serial->bus_record ),
			wago750_serial->bus_record->name );
		break;
	}

	MXI_WAGO750_SERIAL_PAUSE(fname);

	return mx_status;
}

static mx_status_type
mxi_wago750_serial_read_control_byte( MX_WAGO750_SERIAL *wago750_serial,
					uint8_t *control_byte )
{
	static const char fname[] = "mxi_wago750_serial_read_control_byte()";

	uint16_t register_value;
	mx_status_type mx_status;

	switch( wago750_serial->bus_record->mx_class ) {
	case MXI_MODBUS:
		/* For Wago 750 controllers, add 0x200 to the MODBUS address
		 * to read the value of an output.
		 */

		mx_status = mx_modbus_read_holding_registers(
				wago750_serial->bus_record,
				wago750_serial->control_address + 0x200,
				1, &register_value );

		*control_byte = register_value & 0xff;

		MX_WAGO_DEBUG_IO(
		("READ CONTROL: register_value = %#04x, control_byte = %#02x",
			register_value, *control_byte));
		break;
	default:
		mx_status = mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support by MX Wago serial interface '%s' for bus driver "
		"type '%s' used by record '%s' is not yet implemented.",
			wago750_serial->record->name,
			mx_get_driver_name( wago750_serial->bus_record ),
			wago750_serial->bus_record->name );
		break;
	}

	MXI_WAGO750_SERIAL_PAUSE(fname);

	return mx_status;
}

static mx_status_type
mxi_wago750_serial_read_data_registers( MX_WAGO750_SERIAL *wago750_serial,
					unsigned long num_registers_to_read,
					uint16_t *input_register_array )
{
	static const char fname[] = "mxi_wago750_serial_read_data_registers()";

	mx_status_type mx_status;

	switch( wago750_serial->bus_record->mx_class ) {
	case MXI_MODBUS:
		mx_status = mx_modbus_read_input_registers(
					wago750_serial->bus_record,
					wago750_serial->input_bus_address,
					num_registers_to_read,
					input_register_array );
#if MXI_WAGO750_SERIAL_DEBUG
		{
			int i;

			for ( i = 0; i < num_registers_to_read; i++ ) {
				MX_WAGO_DEBUG_IO(
				("READ: input_register_array[%d] = %#04x",
					i, input_register_array[i]));
			}
		}
#endif
		break;
	default:
		mx_status = mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support by MX Wago serial interface '%s' for bus driver "
		"type '%s' used by record '%s' is not yet implemented.",
			wago750_serial->record->name,
			mx_get_driver_name( wago750_serial->bus_record ),
			wago750_serial->bus_record->name );
		break;
	}
	MXI_WAGO750_SERIAL_PAUSE(fname);

	return mx_status;
}

static mx_status_type
mxi_wago750_serial_write_data_registers( MX_WAGO750_SERIAL *wago750_serial,
					unsigned long num_registers_to_write,
					uint16_t *output_register_array )
{
	static const char fname[] = "mxi_wago750_serial_write_data_registers()";

	mx_status_type mx_status;

	switch( wago750_serial->bus_record->mx_class ) {
	case MXI_MODBUS:

#if MXI_WAGO750_SERIAL_DEBUG
		{
			int i;

			for ( i = 0; i < num_registers_to_write; i++ ) {
				MX_WAGO_DEBUG_IO(
				("WRITE: output_register_array[%d] = %#04x",
					i, output_register_array[i]));
			}
		}
#endif
		mx_status = mx_modbus_write_multiple_registers(
					wago750_serial->bus_record,
					wago750_serial->output_bus_address,
					num_registers_to_write,
					output_register_array );
		break;
	default:
		mx_status = mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support by MX Wago serial interface '%s' for bus driver "
		"type '%s' used by record '%s' is not yet implemented.",
			wago750_serial->record->name,
			mx_get_driver_name( wago750_serial->bus_record ),
			wago750_serial->bus_record->name );
		break;
	}

	MXI_WAGO750_SERIAL_PAUSE(fname);

	return mx_status;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_wago750_serial_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_wago750_serial_create_record_structures()";

	MX_RS232 *rs232;
	MX_WAGO750_SERIAL *wago750_serial;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	wago750_serial = (MX_WAGO750_SERIAL *)
				malloc( sizeof(MX_WAGO750_SERIAL) );

	if ( wago750_serial == (MX_WAGO750_SERIAL *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_WAGO750_SERIAL structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = wago750_serial;
	record->class_specific_function_list
				= &mxi_wago750_serial_rs232_function_list;

	rs232->record = record;
	wago750_serial->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_wago750_serial_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_wago750_serial_finish_record_initialization()";

	MX_RS232 *rs232;
	MX_WAGO750_SERIAL *wago750_serial;
	mx_status_type status;

	MX_WAGO_DEBUG(("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	status = mxi_wago750_serial_get_pointers( rs232,
						&wago750_serial, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Check to see if the RS-232 parameters are valid. */

	status = mx_rs232_check_port_parameters( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Mark the wago750_serial device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_wago750_serial_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_wago750_serial_open()";

	MX_RS232 *rs232;
	MX_WAGO750_SERIAL *wago750_serial;
	uint8_t status_byte, data_byte_0;
	long initialization_acknowledge_bit;
	unsigned long i, wait_ms, max_attempts;
	double real_max_attempts, actual_timeout;
	mx_status_type mx_status;

	MX_WAGO_DEBUG(("*** %s invoked ***", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_wago750_serial_get_pointers( rs232,
						&wago750_serial, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( wago750_serial->num_data_bytes != 3 )
	  && ( wago750_serial->num_data_bytes != 5 ) )
	{
		return mx_error( MXE_UNSUPPORTED, fname,
			"'num_data_bytes' = %ld is not supported for Wago "
			"serial interface '%s'.  The only supported values "
			"are 3 and 5.", wago750_serial->num_data_bytes,
			record->name );
	}

	/* Initialize data address values. */

	wago750_serial->control_address =
			(int) wago750_serial->output_bus_address;

	wago750_serial->status_address =
			(int) wago750_serial->input_bus_address;

	switch( wago750_serial->num_data_bytes ) {
	case 3:
		wago750_serial->interleaved_data_addresses = TRUE;
		break;
	case 5:
		if ( wago750_serial->bus_record->mx_class == MXI_MODBUS ) {

			wago750_serial->interleaved_data_addresses = TRUE;
		} else {
			/* FIXME: This will need to be checked in more detail
			 * if non-MODBUS fieldbuses are implemented.
			 */

			wago750_serial->interleaved_data_addresses = FALSE;
		}
	}

	/* Initialize the I/O timeout values. */

	wait_ms = 100;	/* 100 milliseconds per retry */

	max_attempts = MX_ULONG_MAX;

	if ( rs232->timeout >= 0.0 ) {
		real_max_attempts = mx_divide_safely( rs232->timeout,
						    0.001 * (double) wait_ms );

		if ( real_max_attempts < (double) max_attempts ) {
			max_attempts = mx_round( real_max_attempts );
		}
	}

	if ( max_attempts == 0 ) {
		max_attempts = 1;
	}

	actual_timeout = 0.001 * (double) wait_ms * (double) (max_attempts - 1);

	wago750_serial->wait_ms        = wait_ms;
	wago750_serial->max_attempts   = max_attempts;
	wago750_serial->actual_timeout = actual_timeout;

	/* Initialize the local buffer variables. */

	wago750_serial->num_data_bytes_warning = FALSE;

	wago750_serial->num_input_bytes_in_buffer = 0;

	/* Now we are ready to begin communication with the Wago 750-65x
	 * serial interface.
	 */

	/* Set the initialization request (IR) bit in the control byte. */

	MX_WAGO_DEBUG(("%s: setting initialization request bit for '%s'.",
		fname, wago750_serial->record->name));

	mx_status = mxi_wago750_serial_write_control_byte( wago750_serial,
			    MXF_WAGO750_SERIAL_CTRL_INITIALIZATION_REQUEST, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the initialization to complete. */

	for ( i = 0; i < max_attempts; i++ ) {

		MX_WAGO_DEBUG(("%s: reading status byte, i = %lu", fname, i ));

		mx_status = mxi_wago750_serial_read_status_byte( wago750_serial,
						&status_byte, &data_byte_0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		initialization_acknowledge_bit = status_byte &
			MXF_WAGO750_SERIAL_STAT_INITIALIZATION_ACKNOWLEDGE;

		if ( initialization_acknowledge_bit != 0 ) {

			/* The initialization has successfully completed,
			 * so break out of the for() loop.
			 */

			break;
		}

		mx_msleep( wait_ms );
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %g seconds for Wago 750 "
			"serial interface '%s' to finish initialization.",
			actual_timeout, record->name );
	}

	/* Turn off the initialization request bit. */

	MX_WAGO_DEBUG(("%s: turning off initialization request bit.", fname));

	mx_status = mxi_wago750_serial_write_control_byte( wago750_serial,
								0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the initialization acknowledge bit to turn off. */

	for ( i = 0; i < max_attempts; i++ ) {

		MX_WAGO_DEBUG(("%s: reading status byte, i = %lu", fname, i ));

		mx_status = mxi_wago750_serial_read_status_byte( wago750_serial,
						&status_byte, &data_byte_0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		initialization_acknowledge_bit = status_byte &
			MXF_WAGO750_SERIAL_STAT_INITIALIZATION_ACKNOWLEDGE;

		if ( initialization_acknowledge_bit == 0 ) {

			/* The initialization acknowledge bit has been
			 * turned off so break out of the for() loop.
			 */

			break;
		}

		mx_msleep( wait_ms );
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %g seconds for Wago 750 "
			"serial interface '%s' to turn off the "
			"initialization acknowledge bit.",
			actual_timeout, record->name );
	}

	MX_WAGO_DEBUG(("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_wago750_serial_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_wago750_serial_getchar()";

	MX_WAGO750_SERIAL *wago750_serial;
	uint16_t input_register_array[3];
	uint8_t *input_buffer;
	uint8_t new_control_byte, control_byte, status_byte, data_byte_0;
	unsigned long byte_index, num_bytes_in_buffer;
	long num_wago_bytes_to_read, real_num_wago_bytes_to_read;
	long receive_request_bit, new_receive_acknowledge_bit;
	long mask, saved_control_bits;
	unsigned long j, num_registers_to_read;
	mx_status_type mx_status;

	mx_status = mxi_wago750_serial_get_pointers( rs232,
						&wago750_serial, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_WAGO_DEBUG(("*** %s invoked ***", fname));

	MX_WAGO_DEBUG(
	("%s: See if there are any bytes available already.",fname));

	/* See if some bytes are already available in the local input buffer. */

	input_buffer = wago750_serial->input_buffer;

	byte_index = wago750_serial->next_input_byte_index;

	num_bytes_in_buffer = wago750_serial->num_input_bytes_in_buffer;

	MX_WAGO_DEBUG(("%s: #1 byte_index = %lu, num_bytes_in_buffer = %lu",
		fname, byte_index, num_bytes_in_buffer));

	if ( byte_index < num_bytes_in_buffer ) {

		/* There are bytes in the local buffer, so we do not need 
		 * to read from the Wago.
		 */

		num_wago_bytes_to_read = 0;
	} else {

		/* There are no bytes available in the local buffer.  Check
		 * to see if the Wago 750-65x has any bytes available in it.
		 */

		mx_status = mxi_wago750_serial_num_input_bytes_available(rs232);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_wago_bytes_to_read = (int) rs232->num_input_bytes_available;

		if ( num_wago_bytes_to_read == 0 ) {

			/* If we get here, there are no bytes in the local
			 * buffer and no bytes available in the Wago, so
			 * there are no characters available to return
			 * to the caller.
			 */

			MX_WAGO_DEBUG(("%s: No bytes available.",fname));

			return mx_error_quiet( MXE_NOT_READY, fname,
				"Failed to read a character from port '%s'.",
				rs232->record->name );
		}
	}

	MX_WAGO_DEBUG(("%s: num_wago_bytes_to_read = %ld",
		fname, num_wago_bytes_to_read));

	if ( num_wago_bytes_to_read != 0 ) {

		/* Read the data bytes.  Add one for the status byte. */

		real_num_wago_bytes_to_read = num_wago_bytes_to_read + 1;

		if ( ( real_num_wago_bytes_to_read % 2 ) == 0 ) {
			num_registers_to_read = real_num_wago_bytes_to_read / 2;
		} else {
			num_registers_to_read =
					1 + real_num_wago_bytes_to_read / 2;
		}

		MX_WAGO_DEBUG(("%s: num_registers_to_read = %lu",
			fname, num_registers_to_read));

		mx_status = mxi_wago750_serial_read_data_registers(
						wago750_serial,
						num_registers_to_read,
						input_register_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 1
		for ( j = 0; j < num_registers_to_read; j++ ) {
			MX_WAGO_DEBUG(("%s: input_register_array[%lu] = %#x",
				fname, j, input_register_array[j]));
		}
#endif

		/* Copy the data bytes to the local input buffer.
		 *
		 * The fallthroughs to the next case in the switch() statement
		 * below due to the absence of 'break' statements _is_ 
		 * intentional.  This style allows me to write the code
		 * without lots of if...then...else logic.
		 */

		if ( wago750_serial->interleaved_data_addresses ) {
			switch( num_wago_bytes_to_read ) {
			case 5:
				input_buffer[4] =
					(input_register_array[2] >> 8) & 0xff;
			case 4:
				input_buffer[3] =
					input_register_array[2] & 0xff;
			case 3:
				input_buffer[2] =
					(input_register_array[1] >> 8) & 0xff;
			case 2:
				input_buffer[1] =
					input_register_array[1] & 0xff;
			case 1:
				input_buffer[0] =
					(input_register_array[0] >> 8) & 0xff;
			}
		} else {
			switch( num_wago_bytes_to_read ) {
			case 5:
				input_buffer[4] =
					input_register_array[2] & 0xff;
			case 4:
				input_buffer[3] =
					(input_register_array[2] >> 8) & 0xff;
			case 3:
				input_buffer[2] =
					input_register_array[1] & 0xff;
			case 2:
				input_buffer[1] =
					(input_register_array[1] >> 8) & 0xff;
			case 1:
				input_buffer[0] =
					input_register_array[0] & 0xff;
			}
		}

#if 1
		for ( j = 0; j < num_wago_bytes_to_read; j++ ) {
			MX_WAGO_DEBUG(("%s: input_buffer[%lu] = %#x",
				fname, j, input_buffer[j]));
		}
#endif

		wago750_serial->num_input_bytes_in_buffer
					= num_wago_bytes_to_read;

		wago750_serial->next_input_byte_index = 0;

		/* Let the Wago serial interface know that we have
		 * received the characters it sent to us by setting
		 * the 'receive_acknowledge' bit in the control byte
		 * to be equal to the 'receive_request' bit in the
		 * status byte.  However, we must make sure that the
		 * state of the 'transmit_request' bit in the control
		 * byte is not changed.
		 */

		mx_status = mxi_wago750_serial_read_status_byte( wago750_serial,
						&status_byte, &data_byte_0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		receive_request_bit = status_byte &
				MXF_WAGO750_SERIAL_STAT_RECEIVE_REQUEST;

		new_receive_acknowledge_bit = receive_request_bit;

		mx_status = mxi_wago750_serial_read_control_byte(wago750_serial,
						&control_byte );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mask = MXF_WAGO750_SERIAL_CTRL_TRANSMIT_REQUEST
			| MXF_WAGO750_SERIAL_CTRL_FRAMES_AVAILABLE;

		saved_control_bits = control_byte & mask;

		MX_WAGO_DEBUG(
		("%s: status_byte = %#x, control_byte = %#x, mask = %#lx",
			fname, status_byte, control_byte, mask));

		MX_WAGO_DEBUG(("%s: receive_request_bit = %#lx",
			fname, receive_request_bit));

		MX_WAGO_DEBUG(
	("%s: new_receive_acknowledge_bit = %#lx, saved_control_bits = %#lx",
		    fname, new_receive_acknowledge_bit, saved_control_bits));

		new_control_byte =  (uint8_t)
			( new_receive_acknowledge_bit | saved_control_bits );

		MX_WAGO_DEBUG(("%s: new_control_byte = %#x",
			fname, new_control_byte));

		mx_status = mxi_wago750_serial_write_control_byte(
					wago750_serial, new_control_byte, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* We should now have some bytes available in the local input buffer. */

	byte_index = wago750_serial->next_input_byte_index;

	num_bytes_in_buffer = wago750_serial->num_input_bytes_in_buffer;

	MX_WAGO_DEBUG(("%s: #2 byte_index = %lu, num_bytes_in_buffer = %lu",
		fname, byte_index, num_bytes_in_buffer));

	/* Return the next available byte to the routine that called us
	 * and update the buffer indices accordingly.
	 */

	*c = input_buffer[byte_index];

	byte_index++;

	wago750_serial->next_input_byte_index = byte_index;

	if ( byte_index >= num_bytes_in_buffer ) {
		wago750_serial->next_input_byte_index = 0;
		wago750_serial->num_input_bytes_in_buffer = 0;
	}

#if MXI_WAGO750_SERIAL_DEBUG
	MX_DEBUG(-2,("%s: received 0x%x, '%c'", fname, *c, *c));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_wago750_serial_putchar( MX_RS232 *rs232, char c )
{
	size_t bytes_written;
	mx_status_type mx_status;

	mx_status = mxi_wago750_serial_write( rs232, &c, 1, &bytes_written );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_wago750_serial_write( MX_RS232 *rs232,
		char *buffer,
		size_t bytes_to_write,
		size_t *bytes_written )
{
	static const char fname[] = "mxi_wago750_serial_write()";

	MX_WAGO750_SERIAL *wago750_serial;
	uint16_t output_buffer[3];
	uint8_t status_byte, data_byte_0;
	uint8_t old_control_byte, new_control_byte;
	char *buffer_ptr;
	unsigned long i, j, num_full_blocks, num_registers_to_write;
	unsigned long write_size, final_write_size, real_write_size;
	long input_buffer_full, transmit_request, transmit_acknowledge;
	long new_transmit_request, new_transmit_acknowledge;
	long old_receive_acknowledge;
	mx_status_type mx_status;

	mx_status = mxi_wago750_serial_get_pointers( rs232,
						&wago750_serial, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_WAGO_DEBUG(("*** %s invoked ***", fname));

#if MXI_WAGO750_SERIAL_DEBUG
	for ( i = 0; i < bytes_to_write; i++ ) {
		MX_DEBUG(-2,("%s: sending buffer[%ld] = 0x%x, '%c'",
				fname, i, buffer[i], buffer[i]));
	}
#endif

	/* We cannot transmit more bytes in one operation than will fit
	 * into the Wago's input buffer, so we must break up the write
	 * into multiple smaller blocks that fit into the Wago's buffer.
	 */

	num_full_blocks = bytes_to_write / wago750_serial->num_data_bytes;

	final_write_size = bytes_to_write % wago750_serial->num_data_bytes;

	MX_WAGO_DEBUG(("%s: num_full_blocks = %lu, final_write_size = %lu",
		fname, num_full_blocks, final_write_size));

	/* Loop over the blocks to write. */

	for ( i = 0; i < (num_full_blocks + 1); i++ ) {

		if ( i == num_full_blocks ) {
			write_size = final_write_size;
		} else {
			write_size = wago750_serial->num_data_bytes;
		}

		if ( write_size == 0 ) {
			MX_WAGO_DEBUG(("%s: *** No bytes left to write ***",
				fname));

			break;		/* Exit the for(i...) loop. */
		}

		MX_WAGO_DEBUG(("%s: *** Beginning block %lu ***", fname, i));

		buffer_ptr = buffer + i * wago750_serial->num_data_bytes;

		/* If the Wago's input buffer is full, wait until there is room
		 * in it.
		 */

		for ( j = 0; j < wago750_serial->max_attempts; j++ ) {

			mx_status = mxi_wago750_serial_read_status_byte(
					wago750_serial, &status_byte,
					&data_byte_0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			input_buffer_full = status_byte
				& MXF_WAGO750_SERIAL_STAT_INPUT_BUFFER_IS_FULL;

			if ( input_buffer_full == 0 ) {
				/* There is now room in the Wago serial
				 * interface's input buffer, so we can
				 * now transmit.
				 */

				break;	/* Exit the for() loop. */
			}
		}

		if ( j >= wago750_serial->max_attempts ) {
			return mx_error( MXE_TIMED_OUT, fname,
	"Timed out after %g seconds while waiting for space to become "
	"available in the input buffer of Wago serial interface '%s'.",
				wago750_serial->actual_timeout,
				rs232->record->name );
		}

		MX_WAGO_DEBUG(("%s: Wago serial input buffer now has space.",
			fname));

		/* The status byte contains the value of the 
		 * 'transmit_acknowledge' bit as well.
		 */

		transmit_acknowledge = status_byte
				& MXF_WAGO750_SERIAL_STAT_TRANSMIT_ACKNOWLEDGE;

		MX_WAGO_DEBUG(("%s: #1 transmit_acknowledge = %ld",
			fname, transmit_acknowledge));

		/* Get the old value of the control byte. */

		mx_status = mxi_wago750_serial_read_control_byte(wago750_serial,
							&old_control_byte );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Preserve the old value of 'receive_acknowledge'. */

		old_receive_acknowledge = old_control_byte &
				MXF_WAGO750_SERIAL_CTRL_RECEIVE_ACKNOWLEDGE;

		/* Construct the control byte to be sent along with the
		 * first character.  The 'transmit_request' bit in this
		 * control byte should have the same value as the
		 * 'transmit_acknowledge' bit that we just received.
		 */

		new_control_byte = (uint8_t) old_receive_acknowledge;

		if ( transmit_acknowledge != 0 ) {
			new_control_byte
				|= MXF_WAGO750_SERIAL_CTRL_TRANSMIT_REQUEST;
		}

		/*
		 * The upper half of the control byte contains the number
		 * of characters that we are sending in this block.
		 */

		new_control_byte |= ( write_size << 4 );

		MX_WAGO_DEBUG(("%s: #1 new_control_byte = %#x",
			fname, new_control_byte));

		/* Copy the data bytes to the local output buffer.
		 *
		 * The fallthroughs to the next case in the switch() statement
		 * below due to the absence of 'break' statements _is_ 
		 * intentional.  This style allows me to write the code
		 * without lots of if...then...else logic.
		 */

		for ( j = 0; j < sizeof(output_buffer); j++ ) {
			output_buffer[j] = 0;
		}

		if ( wago750_serial->interleaved_data_addresses ) {
			switch( write_size ) {
			case 5:
				output_buffer[2] =
					( (uint16_t) buffer_ptr[4] ) << 8;
			case 4:
				output_buffer[2] |=
					( (uint16_t) buffer_ptr[3] );
			case 3:
				output_buffer[1] =
					( (uint16_t) buffer_ptr[2] ) << 8;
			case 2:
				output_buffer[1] |=
					( (uint16_t) buffer_ptr[1] );
			case 1:
				output_buffer[0] =
					( (uint16_t) buffer_ptr[0] ) << 8;
			}
		} else {
			switch( write_size ) {
			case 5:
				output_buffer[2] =
					( (uint16_t) buffer_ptr[4] );
			case 4:
				output_buffer[2] |=
					( (uint16_t) buffer_ptr[3] ) << 8;
			case 3:
				output_buffer[1] =
					( (uint16_t) buffer_ptr[2] );
			case 2:
				output_buffer[1] |=
					( (uint16_t) buffer_ptr[1] ) << 8;
			case 1:
				output_buffer[0] =
					( (uint16_t) buffer_ptr[0] );
			}
		}

		/* Add the control byte to the output buffer. */

		if ( wago750_serial->interleaved_data_addresses ) {
			output_buffer[0] |= (uint16_t) new_control_byte;
		} else {
			output_buffer[0] |=
				( (uint16_t) new_control_byte ) << 8;
		}

		/* Transmit the output buffer to the Wago serial interface. */

			/* Add 1 for the control byte. */

		real_write_size = write_size + 1;

		if ( ( real_write_size % 2 ) == 0 ) {
			num_registers_to_write = real_write_size / 2;
		} else {
			num_registers_to_write = 1 + real_write_size / 2;
		}

#if 1
		MX_WAGO_DEBUG(("%s: num_registers_to_write = %lu",
			fname, num_registers_to_write));

		for ( j = 0; j < num_registers_to_write; j++ ) {
			MX_WAGO_DEBUG(("%s: output_buffer[%lu] = %#x",
				fname, j, output_buffer[j]));
		}
#endif

		mx_status = mxi_wago750_serial_write_data_registers(
						wago750_serial,
						num_registers_to_write,
						output_buffer );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* The new control byte becomes the old control byte once
		 * we have transmitted it.
		 */

		old_control_byte = new_control_byte;

		/* Now send a new control byte with the 'transmit_request'
		 * inverted relative to the one we just sent.
		 */

		transmit_request = old_control_byte
				& MXF_WAGO750_SERIAL_CTRL_TRANSMIT_REQUEST;

		MX_WAGO_DEBUG(
		("%s: transmit_request = %ld, OLD control_byte = %#x",
			fname, transmit_request, old_control_byte));

		/* Mask off the transmit request bit. */

		new_control_byte = old_control_byte &
				( ~ MXF_WAGO750_SERIAL_CTRL_TRANSMIT_REQUEST );

		MX_WAGO_DEBUG(("%s: Step #1, new_control_byte = %#x",
			fname, new_control_byte));

		/* Invert the transmit request bit. */

		new_transmit_request = ( ~transmit_request )
				& MXF_WAGO750_SERIAL_CTRL_TRANSMIT_REQUEST;

		MX_WAGO_DEBUG(("%s: Step #2, new_transmit_request = %ld",
			fname, new_transmit_request));

		/* Bitwise OR in the new value of transmit request. */

		new_control_byte |= new_transmit_request;

		MX_WAGO_DEBUG(
		("%s: inverting transmit request, NEW control_byte = %#x",
			fname, new_control_byte));

		mx_status = mxi_wago750_serial_write_control_byte(
					wago750_serial, new_control_byte,
					(uint8_t) buffer_ptr[0] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Now we wait for the 'transmit_acknowledge' bit in
		 * the status byte to have the same value as the new 
		 * 'transmit_request' that we just sent.
		 */

		MX_WAGO_DEBUG(("%s: waiting for transmit acknowledge.", fname));

		for ( j = 0; j < wago750_serial->max_attempts; j++ ) {
			mx_status = mxi_wago750_serial_read_status_byte(
						wago750_serial, &status_byte,
						&data_byte_0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			new_transmit_acknowledge = status_byte
				& MXF_WAGO750_SERIAL_STAT_TRANSMIT_ACKNOWLEDGE;

			MX_WAGO_DEBUG(
		    ("%s: status_byte = %#x, new_transmit_acknowledge = %ld",
				fname, status_byte, new_transmit_acknowledge));

			if ( new_transmit_request == new_transmit_acknowledge )
			{
				/* The Wago serial interface has finished
				 * receiving the characters that we just sent
				 * so we can break out of the for() loop now.
				 */

				break;
			}
		}

		if ( j >= wago750_serial->max_attempts ) {
			return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %g seconds for Wago 750 "
			"serial interface '%s' to acknowledge receipt of the "
			"characters that we just transmitted.",
				wago750_serial->actual_timeout,
				rs232->record->name );
		}

		MX_WAGO_DEBUG(("%s: *** Block %lu has been transmitted ***",
			fname, i ));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_wago750_serial_putline( MX_RS232 *rs232,
		char *buffer,
		size_t *bytes_written )
{
	size_t buffer_length, buffer_bytes_written, terminator_bytes_written;
	mx_status_type mx_status;

	/* First, write the contents of the buffer. */

	buffer_length = strlen( buffer );

	mx_status = mxi_wago750_serial_write( rs232,
				buffer, buffer_length,
				&buffer_bytes_written );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Then, write the line terminators. */

	mx_status = mxi_wago750_serial_write( rs232,
				rs232->write_terminator_array,
				rs232->num_write_terminator_chars,
				&terminator_bytes_written );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_written != NULL ) {
		*bytes_written = buffer_bytes_written
				+ terminator_bytes_written;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_wago750_serial_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_wago750_serial_num_input_bytes_available()";

	MX_WAGO750_SERIAL *wago750_serial;
	uint8_t status_byte, control_byte, data_byte_0;
	long receive_request, receive_acknowledge;
	long num_bytes_available;
	unsigned long byte_index, num_bytes_in_buffer;
	mx_status_type mx_status;

	mx_status = mxi_wago750_serial_get_pointers( rs232,
						&wago750_serial, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_WAGO_DEBUG(("*** %s invoked ***", fname));

	/* First see if there are already some unread bytes in the
	 * local input buffer.
	 */

	byte_index = wago750_serial->next_input_byte_index;

	num_bytes_in_buffer = wago750_serial->num_input_bytes_in_buffer;

	MX_WAGO_DEBUG(("%s: byte_index = %ld, num_bytes_in_buffer = %ld",
		fname, byte_index, num_bytes_in_buffer));

	if ( byte_index < num_bytes_in_buffer ) {

		rs232->num_input_bytes_available =
			(unsigned long) ( num_bytes_in_buffer - byte_index );

		MX_WAGO_DEBUG(
	    ("%s: *** bytes are in buffer, num_input_bytes_available = %lu ***",
			fname, rs232->num_input_bytes_available ));

		return MX_SUCCESSFUL_RESULT;
	}
	
	/* Otherwise, we must check the 'receive_request' and 
	 * 'receive_acknowledge' bits to figure out if the Wago
	 * serial interface has any characters to send.
	 */

	/* First, get the 'receive_acknowledge' bit from
	 * the control byte.
	 */

	mx_status = mxi_wago750_serial_read_control_byte( wago750_serial,
							&control_byte );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Then, get the 'receive_request' bit from the status byte. */

	mx_status = mxi_wago750_serial_read_status_byte( wago750_serial,
							&status_byte,
							&data_byte_0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_WAGO_DEBUG(("%s: control_byte = %#x, status_byte = %#x",
		fname, control_byte, status_byte));

	receive_acknowledge = control_byte
			& MXF_WAGO750_SERIAL_CTRL_RECEIVE_ACKNOWLEDGE;

	receive_request = status_byte
			& MXF_WAGO750_SERIAL_STAT_RECEIVE_REQUEST;

	/* If the 'receive_request' bit is not equal to the
	 * 'receive_acknowledge' bit, then the Wago serial
	 * interface has characters available to read.
	 */

	MX_WAGO_DEBUG(("%s: receive_request = %ld, receive_acknowledge = %ld",
		fname, receive_request, receive_acknowledge));

	if ( receive_request == receive_acknowledge ) {
		rs232->num_input_bytes_available = 0;

		MX_WAGO_DEBUG(
    ("%s: *** no bytes available in Wago, num_input_bytes_available = %lu ***",
			fname, rs232->num_input_bytes_available));

		return MX_SUCCESSFUL_RESULT;
	}

	num_bytes_available = ( status_byte >> 4 ) & 0x0f;

	/* Check some potential error conditions. */

	if ( num_bytes_available == 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"The 'receive acknowledge' bit (%#lx) and the 'receive request' bit (%#lx) "
"are different for Wago serial interface '%s', but the status byte also "
"says that there are zero bytes available to read!  This should not happen!",
			receive_acknowledge, receive_request,
			rs232->record->name );
	}

	if ( num_bytes_available > 5 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"The Wago serial interface '%s' reports that it has %ld bytes available "
"to read, but the maximum is supposed to be %ld bytes.  Perhaps you have a "
"different model Wago serial interface than the ones this driver was "
"written for.", rs232->record->name, num_bytes_available,
			wago750_serial->num_data_bytes );
	}

	if ( num_bytes_available > wago750_serial->num_data_bytes ) {
		if ( wago750_serial->num_data_bytes_warning == FALSE ) {
			mx_warning( "Wago serial interface '%s' "
"reports that it has %ld bytes available to read, but you have configured the "
"interface to only transmit %ld bytes.  This will work but does not use the "
"interface in the most efficient manner possible.",
				rs232->record->name,
				num_bytes_available,
				wago750_serial->num_data_bytes );

			wago750_serial->num_data_bytes_warning = TRUE;
		}
	}

	rs232->num_input_bytes_available = num_bytes_available;

	MX_WAGO_DEBUG(("%s: num_input_bytes_available = %lu",
		fname, rs232->num_input_bytes_available));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_wago750_serial_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_wago750_serial_discard_unread_input()";

	MX_WAGO750_SERIAL *wago750_serial;
	mx_status_type mx_status;

	mx_status = mxi_wago750_serial_get_pointers( rs232,
						&wago750_serial, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_WAGO_DEBUG(("*** %s invoked ***", fname));

	wago750_serial->num_input_bytes_in_buffer = 0;

	wago750_serial->next_input_byte_index = 0;

	return MX_SUCCESSFUL_RESULT;
}


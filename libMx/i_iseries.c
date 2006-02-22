/*
 * Name:    i_iseries.c
 *
 * Purpose: MX interface driver for iSeries temperature and process
 *          controllers made by Newport Electronics and distributed
 *          by Omega.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_ISERIES_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_socket.h"
#include "mx_rs232.h"
#include "mx_modbus.h"
#include "i_tcp232.h"
#include "i_iseries.h"

MX_RECORD_FUNCTION_LIST mxi_iseries_record_function_list = {
	NULL,
	mxi_iseries_create_record_structures,
	mxi_iseries_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_iseries_open
};

MX_RECORD_FIELD_DEFAULTS mxi_iseries_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_ISERIES_STANDARD_FIELDS
};

long mxi_iseries_num_record_fields
		= sizeof( mxi_iseries_record_field_defaults )
			/ sizeof( mxi_iseries_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_iseries_rfield_def_ptr
			= &mxi_iseries_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_iseries_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_iseries_create_record_structures()";

	MX_ISERIES *iseries;

	/* Allocate memory for the necessary structures. */

	iseries = (MX_ISERIES *) malloc( sizeof(MX_ISERIES) );

	if ( iseries == (MX_ISERIES *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ISERIES structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = iseries;

	record->record_function_list = &mxi_iseries_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	iseries->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_iseries_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_iseries_finish_record_initialization()";

	MX_ISERIES *iseries;
	char *ptr;

	iseries = (MX_ISERIES *) record->record_type_struct;

	iseries->recognition_character = iseries->bus_address_name[0];

	ptr = &(iseries->bus_address_name[1]);

	iseries->bus_address = (long) mx_hex_string_to_unsigned_long( ptr );

	MX_DEBUG( 2,("%s: '%s' recognition_character = '%c'",
		fname, record->name, iseries->recognition_character));

	MX_DEBUG( 2,("%s: '%s' bus_address = %02lX",
		fname, record->name, iseries->bus_address));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_iseries_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_iseries_open()";

	MX_ISERIES *iseries;
	MX_RS232 *rs232;
	MX_TCP232 *tcp232;
	char command[80];
	char response[80];
	char *response_ptr, *ptr;
	size_t num_bytes_read;
	int i;
	double software_version;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	iseries = (MX_ISERIES *) record->record_type_struct;

	if ( iseries == (MX_ISERIES *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ISERIES pointer for record '%s' is NULL.", record->name);
	}

	if ( iseries->bus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The 'bus_record' pointer for iSeries controller '%s' is NULL.",
			record->name );
	}

	/* Figure out whether iSeries echo mode is on.  Along the way
	 * we also get the iSeries software version.
	 */

	software_version = -1;

	switch( iseries->bus_record->mx_class ) {
	case MXI_MODBUS:
		/* Echo mode is irrelevant for MODBUS communication. */

		iseries->echo_on = FALSE;

		/* Ask for the software version. */

		mx_status = mxi_iseries_command( iseries, 'U', 0x03, 0, 0,
						&software_version, 0,
						MXI_ISERIES_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXI_RS232:
		rs232 = (MX_RS232 *) iseries->bus_record->record_class_struct;

		if ( iseries->bus_record->mx_type == MXI_232_TCP232 ) {
			tcp232 = (MX_TCP232 *)
				iseries->bus_record->record_type_struct;

			if ( (tcp232->tcp232_flags & MXF_TCP232_QUIET) == 0 ) {

				mx_warning(
		"The MXF_TCP232_QUIET bit (%#x) is not set in the "
		"'tcp232_flags' field of record '%s'.  Without this flag set, "
		"you will get an MXE_NETWORK_CONNECTION_LOST message after "
		"each command sent to the iSeries controller.  This is "
		"probably not what you want.", MXF_TCP232_QUIET,
					iseries->bus_record->name );
			}
		}

		/* Ask for the software version and see if the command
		 * is echoed.
		 */

		if ( iseries->bus_address >= 0 ) {
			sprintf( command, "%c%02lxU03",
				iseries->recognition_character,
				iseries->bus_address );
		} else {
			sprintf( command, "%cU03",
				iseries->recognition_character );
		}

#if MXI_ISERIES_DEBUG
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
			fname, command, record->name ));
#endif

		/* The command _must_ be sent as one packet including the
		 * line terminators or else the iSeries controller will
		 * discard the message.  For that reason, we manually add
		 * here the line terminators rather than relying on
		 * mx_rs232_putline() to do the job.
		 */

		ptr = command + strlen( command );

		for ( i = 0; i < rs232->num_write_terminator_chars; i++ ) {
			ptr[i] = rs232->write_terminator_array[i];
		}

		ptr[i] = '\0';	/* Make sure the string is null terminated. */

		/* Send the command. */

		mx_status = mx_rs232_write( iseries->bus_record,
						command,
						strlen( command ),
						NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_rs232_getline( iseries->bus_record,
						response, sizeof(response),
						&num_bytes_read,
						0 );

		if ( ( mx_status.code != MXE_SUCCESS )
		  && ( mx_status.code != MXE_NETWORK_CONNECTION_LOST ) )
		{
			return mx_status;
		}

#if MXI_ISERIES_DEBUG
		MX_DEBUG(-2,("%s: received '%s' from '%s'.",
			fname, response, record->name ));
#endif

		if ( iseries->bus_address >= 0 ) {
			response_ptr = response + 2;
		} else {
			response_ptr = response;
		}

		if ( *response_ptr == 'U' ) {
			iseries->echo_on = TRUE;

			response_ptr += 3;
		} else {
			iseries->echo_on = FALSE;

			response_ptr = response;
		}

		software_version = atof( response_ptr );

		break;

	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Bus record '%s' is not an RS-232 or a MODBUS record.",
			iseries->bus_record->name );
	}

	iseries->software_version = software_version;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_iseries_command( MX_ISERIES *iseries,
			char command_prefix,
			unsigned long command_index,
			int num_command_bytes,
			double command_value,
			double *response_value,
			int precision,
			int debug_flag )
{
	static const char fname[] = "mxi_iseries_command()";

	MX_RS232 *rs232;
	char command_buffer[100];
	char response_buffer[100];
	char *response_ptr, *ptr;
	int put_or_write, current_precision;
	uint16_t current_reading_configuration, new_reading_configuration;
	uint16_t register_read_value, register_write_value;
	double multiplier, divisor, double_read_value, double_write_value;
	long mask, long_read_value, long_write_value;
	int i;
	mx_status_type mx_status;

	if ( iseries == (MX_ISERIES *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_ISERIES pointer passed was NULL." );
	}
	if ( iseries->bus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bus_record' pointer for the MX_ISERIES structure passed "
		"is NULL." );
	}
	if ( ( precision < 0 ) || ( precision > 3 ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested floating point precision %d for iSeries "
	"controller '%s' is outside the allowed range of 0-3.",
			precision, iseries->record->name );
	}

	if ( islower( (int) command_prefix ) ) {
		command_prefix = toupper( (int) command_prefix );
	}

	if ( (command_prefix == 'P') || (command_prefix == 'W') ) {
		put_or_write = TRUE;
	} else {
		put_or_write = FALSE;
	}

	switch( iseries->bus_record->mx_class ) {
	case MXI_RS232:
		rs232 = (MX_RS232 *) iseries->bus_record->record_class_struct;

		if ( iseries->bus_address >= 0 ) {
			/* Multipoint RS-485 mode. */

			if ( put_or_write ) {
				if ( precision != 0 ) {
				    sprintf( command_buffer,
					"%c%02lX%c%02lX%*g",
					iseries->recognition_character,
					iseries->bus_address,
					command_prefix,
					command_index,
					precision,
					command_value );
				} else {
				    sprintf( command_buffer,
					"%c%02lX%c%02lX%*lX",
					iseries->recognition_character,
					iseries->bus_address,
					command_prefix,
					command_index,
					2 * num_command_bytes,
					mx_round( command_value ) );
				}
			} else {
				sprintf( command_buffer,
					"%c%02lX%c%02lX",
					iseries->recognition_character,
					iseries->bus_address,
					command_prefix,
					command_index );
			}
		} else {
			/* Point-to-point mode. */

			if ( put_or_write ) {
				if ( precision != 0 ) {
				    sprintf( command_buffer,
					"%c%c%02lX%*g",
					iseries->recognition_character,
					command_prefix,
					command_index,
					precision,
					command_value );
				} else {
				    sprintf( command_buffer,
					"%c%c%02lX%*lX",
					iseries->recognition_character,
					command_prefix,
					command_index,
					2 * num_command_bytes,
					mx_round( command_value ) );
				}
			} else {
				sprintf( command_buffer,
					"%c%c%02lX",
					iseries->recognition_character,
					command_prefix,
					command_index );
			}
		}

#if MXI_ISERIES_DEBUG
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
			fname, command_buffer, iseries->record->name ));
#endif

		/* The command _must_ be sent as one message including the
		 * line terminators or else the iSeries controller will
		 * discard the message.  For that reason, we manually add
		 * here the line terminators rather than relying on
		 * mx_rs232_putline() to do the job.
		 */

		ptr = command_buffer + strlen( command_buffer );

		for ( i = 0; i < rs232->num_write_terminator_chars; i++ ) {
			ptr[i] = rs232->write_terminator_array[i];
		}

		ptr[i] = '\0';	/* Make sure the string is null terminated. */

		/* Send the command. */

		mx_status = mx_rs232_write( iseries->bus_record,
						command_buffer,
						strlen( command_buffer ),
						NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( iseries->echo_on || ( put_or_write == FALSE ) ) {
			/* Read the response. */

			mx_status = mx_rs232_getline( iseries->bus_record,
						response_buffer,
						sizeof(response_buffer),
						NULL, 0 );

			if ( ( mx_status.code != MXE_SUCCESS )
			  && ( mx_status.code != MXE_NETWORK_CONNECTION_LOST ) )
			{
				return mx_status;
			}

#if MXI_ISERIES_DEBUG
			MX_DEBUG(-2,("%s: received '%s' from '%s'.",
				fname, response_buffer, iseries->record->name));
#endif

			if ( response_value != NULL ) {
				if ( put_or_write ) {
					*response_value = 0;
				} else {
					if ( iseries->echo_on ) {
					    if ( iseries->bus_address >= 0 ) {
					        response_ptr =
							response_buffer + 5;
					    } else {
					        response_ptr =
							response_buffer + 3;
					    }
					} else {
					    response_ptr = response_buffer;
					}

					/* Skip over any leading whitespace. */

					response_ptr += strspn( response_ptr,
								MX_WHITESPACE );

					/* Parse the response. */

					if ( ( precision != 0 )
					  || ( command_prefix == 'X' ) )
					{
					    *response_value =
							atof(response_ptr);
					} else {
					    *response_value = (double)
				  mx_hex_string_to_unsigned_long(response_ptr);
					}
				}
			}
		}
		break;
	case MXI_MODBUS:
		/* Get the current reading configuration. */

		mx_status = mx_modbus_read_holding_registers(
					iseries->bus_record,
					iseries->bus_address + 0x08, 1,
					&current_reading_configuration );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		current_precision = ( current_reading_configuration & 0x7 ) - 1;

		if ( put_or_write ) {
			/* Write a value to the controller. */

			/* If needed, first set the precision. */

			if ( precision != current_precision ) {

				new_reading_configuration =
				    current_reading_configuration & ( ~ 0x7 );

				new_reading_configuration |= ( precision + 1 );

				mx_status = mx_modbus_write_single_register(
					iseries->bus_record,
					iseries->bus_address + 0x08,
					new_reading_configuration );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

			/* Adjust the value to write depending on the
			 * decimal point precision.
			 */

			multiplier = pow( 10.0, precision );

			double_write_value = command_value * multiplier;

			long_write_value = mx_round( double_write_value );

			if ( long_write_value > 32767 ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested value %g to write to iSeries interface '%s' "
	"would exceed the positive limit %g for the requested decimal point "
	"precision of %d.", command_value, iseries->record->name,
					32767.0 / multiplier, precision );
			} else
			if ( long_write_value < -32768 ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested value %g to write to iSeries interface '%s' "
	"would exceed the positive limit %g for the requested decimal point "
	"precision of %d.", command_value, iseries->record->name,
					-32768.0 / multiplier, precision );
			}

			register_write_value = (uint16_t) long_write_value;

			/* Now write the value. */

			mx_status = mx_modbus_write_single_register(
					iseries->bus_record,
					iseries->bus_address + command_index,
					register_write_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} else {
			if ( response_value == NULL ) {
				return mx_error( MXE_NULL_ARGUMENT, fname,
	"'response_value' pointer is NULL for read command prefix '%c' "
	"of command sent to iSeries controller '%s'.",
					command_prefix,
					iseries->bus_record->name );
			}

			/* Read a value from the controller. */

			mx_status = mx_modbus_read_holding_registers(
					iseries->bus_record,
					iseries->bus_address + command_index, 1,
					&register_read_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			long_read_value = (long) register_read_value;

			/* Sign extend values that are supposed
			 * to be negative.
			 */

			if ( register_read_value >= 0x8000 ) {
				/* Set all the high bits to 1. */

				mask = ~ 0xffff;  /* mask = 0x...ffff0000 */

				long_read_value |= mask;
			}

			double_read_value = (double) long_read_value;

			/* Adjust the value just read depending on the
			 * decimal point precision.
			 */

			divisor = pow( 10.0, precision );

			*response_value = double_read_value / divisor;
		}
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}


/*
 * Name:    i_pdi45.c
 *
 * Purpose: MX driver for Prairie Digital, Inc. Model 45 data acquisition
 *          and control module.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_PDI45_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "i_pdi45.h"

MX_RECORD_FUNCTION_LIST mxi_pdi45_record_function_list = {
	NULL,
	mxi_pdi45_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_pdi45_open
};

MX_RECORD_FIELD_DEFAULTS mxi_pdi45_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PDI45_STANDARD_FIELDS
};

mx_length_type mxi_pdi45_num_record_fields
		= sizeof( mxi_pdi45_record_field_defaults )
			/ sizeof( mxi_pdi45_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_pdi45_rfield_def_ptr
			= &mxi_pdi45_record_field_defaults[0];

static mx_status_type mxi_pdi45_checksum( MX_PDI45 *pdi45,
				char *command, char *checksum );

static mx_status_type mxi_pdi45_check_checksum( char *response );

static mx_status_type mxi_pdi45_get_error_code( char *buffer, int *error_code );

MX_EXPORT mx_status_type
mxi_pdi45_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_pdi45_create_record_structures()";

	MX_PDI45 *pdi45;

	/* Allocate memory for the necessary structures. */

	pdi45 = (MX_PDI45 *) malloc( sizeof(MX_PDI45) );

	if ( pdi45 == (MX_PDI45 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_PDI45 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = pdi45;

	record->record_function_list = &mxi_pdi45_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	pdi45->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pdi45_open( MX_RECORD *record )
{
	const char fname[] = "mxi_pdi45_open()";

	MX_PDI45 *pdi45;
	MX_RS232 *rs232;
	char response[250];
	char buffer[80];
	char *communication_configuration;
	int i, num_items, hex_value;
	int exit_loop, length, result;
	int power_up_clear_needed, four_step_mode;
	int pdi45_error_code;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	MX_DEBUG(-2, ("%s invoked for record '%s'.", fname, record->name));

	pdi45 = (MX_PDI45 *) record->record_type_struct;

	if ( pdi45 == (MX_PDI45 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PDI45 pointer for record '%s' is NULL.", record->name);
	}

	if ( pdi45->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"rs232_record pointer for PDI45 record '%s' is NULL.",
			record->name );
	}

	result = mx_verify_driver_type( pdi45->rs232_record,
				MXR_INTERFACE, MXI_RS232, MXT_ANY );

	if ( result != TRUE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"rs232_record '%s' for PDI45 record '%s' is not actually an RS-232 record.",
			pdi45->rs232_record->name, record->name );
	}

	rs232 = (MX_RS232 *) pdi45->rs232_record->record_class_struct;

	if ( ( rs232->read_terminators != MX_CR )
	  || ( rs232->write_terminators != MX_CR ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The read and write terminators for RS-232 record '%s' "
		"used by PDI45 record '%s' must both be "
		"carriage return characters (0x0d).",
			pdi45->rs232_record->name,
			record->name );
	}

	/* Flush any unread or unwritten characters left over. */

	mx_status = mx_rs232_discard_unwritten_output( pdi45->rs232_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( pdi45->rs232_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	exit_loop = FALSE;
	four_step_mode = FALSE;
	power_up_clear_needed = FALSE;
	communication_configuration = NULL;

	/* Start out by trying to read the communication configuration. */

	while ( exit_loop == FALSE ) {

		if ( power_up_clear_needed ) {
			mx_status = mxi_pdi45_command( pdi45, "00A", NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Send the read communication configuration command. */

                mx_status = mx_rs232_putline( pdi45->rs232_record, ">00%0B5",
						NULL, MXI_PDI45_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* See what response we got from the PDI 45 */

		mx_status = mx_rs232_getline( pdi45->rs232_record,
						response, sizeof response,
						NULL, MXI_PDI45_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( response[0] ) {
		case 'N':	/* ===== Error code response. ===== */

			mx_status = mxi_pdi45_get_error_code( response,
							&pdi45_error_code );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			switch( pdi45_error_code ) {
			case MX_PDI45_ERR_POWER_UP_CLEAR_EXPECTED:
				/* Give up if this is a second try. */

				if ( power_up_clear_needed == TRUE ) {
					return mx_error(
					MXE_INTERFACE_IO_ERROR, fname,
			"Power up clear of the PDI 45 was not successful.");
				}

				power_up_clear_needed = TRUE;
				break;

			default:
				return mx_error(MXE_INTERFACE_IO_ERROR, fname,
	"Unexpected error code %d while trying to connect to the PDI 45.",
					pdi45_error_code);
			}
			break;

		case 'A':	/* ===== Acknowledgement response. ===== */

			power_up_clear_needed = FALSE;

			/* Are we in two step mode or four step mode? */

			length = strlen( response );

			switch( length ) {
			case 13:
				four_step_mode = FALSE;
				communication_configuration = &response[0];
				break;

			case 7:
				if ( strcmp( response, "A00%0B5" ) != 0 ) {
					return mx_error(
					MXE_INTERFACE_IO_ERROR, fname,
"Unexpected response to read communication configuration command = '%s'",
						response );
				}
				four_step_mode = TRUE;

				/* Now need to execute the command. */

				mx_status = mx_rs232_putline(
						pdi45->rs232_record, "E",
						NULL, MXI_PDI45_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* Get the response. */

				mx_status = mx_rs232_getline(
						pdi45->rs232_record,
						response, sizeof response,
						NULL, MXI_PDI45_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				communication_configuration = &response[0];
				break;

			default:
				MX_DEBUG(-2,("Response length = %d", length));

				return mx_error(MXE_INTERFACE_IO_ERROR, fname,
"Unexpected response to read communication configuration command = '%s'",
					response );
				break;
			}
			break;
		default:	/* ===== Illegal response. ===== */

			MX_DEBUG(-2, ("Illegal response char = 0x%x '%c'",
				response[0], response[0]));

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Unexpected response to read communication configuration command = '%s'",
				response );
			break;
	
		}

		if ( power_up_clear_needed == TRUE ) {
			exit_loop = FALSE;
		} else {
			exit_loop = TRUE;
		}
	}

	/* Save the four step mode flag. */

	pdi45->four_step_mode = four_step_mode;

	if ( four_step_mode ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Four step mode not yet implemented." );
	}
				
	/*** Read the configuration of the digital I/O types. ***/

	mx_status = mxi_pdi45_command( pdi45, "00!0",
				response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < MX_PDI45_NUM_DIGITAL_CHANNELS; i++ ) {
		strlcpy( buffer, response + 31 - 2 * i, 3 );

		num_items = sscanf( buffer, "%x", &hex_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unrecognizable digital I/O type '%s' in response '%s' "
		"to command '00!0' for PDI45 '%s'.",
				buffer, response, record->name );
		}

		pdi45->io_type[i] = hex_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pdi45_command( MX_PDI45 *pdi45,
			char *command,
			char *response,
			size_t response_length )
{
	const char fname[] = "mxi_pdi45_command()";

	char read_buffer[MX_PDI45_COMMAND_LENGTH+1];
	char write_buffer[MX_PDI45_COMMAND_LENGTH+1];
	char checksum[4];
	int pdi45_error_code;
	mx_status_type mx_status;

	/* Construct checksum field for outgoing PDI45 command. */

	mx_status = mxi_pdi45_checksum( pdi45, command, checksum );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Format the command string. */

	sprintf(write_buffer, ">%s%s", command, checksum);

	/* Send the PDI45 command. */

	mx_status = mx_rs232_putline( pdi45->rs232_record, write_buffer,
					NULL, MXI_PDI45_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the response to the command. */

	mx_status = mx_rs232_getline( pdi45->rs232_record,
					read_buffer, sizeof(read_buffer), 
					NULL, MXI_PDI45_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Was the response an error code? */

	switch( read_buffer[0] ) {
	case 'A':
		/* Was the response only the single character 'A'? */

		if ( read_buffer[1] != '\0' ) {
			/* If not, check the checksum of the response. */

			mx_status = mxi_pdi45_check_checksum( read_buffer );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Copy the response to the output variable if desired. */

		if ( response != NULL ) {
			strlcpy( response, read_buffer, response_length );
		}

		return MX_SUCCESSFUL_RESULT;

	case 'N':
		mx_status = mxi_pdi45_get_error_code( read_buffer,
							&pdi45_error_code );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( pdi45_error_code ) {
		case 0x00:
			return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
			"Error 0x00: Power up clear expected for "
			"PDI45 controller '%s'.",
				pdi45->record->name );
		case 0x01:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error 0x01: Unrecognized command '%s' sent to "
			"PDI45 controller '%s'.",
				command, pdi45->record->name );
		case 0x02:
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error 0x02: received command checksum "
			"does not match the calculated checksum for "
			"command '%s' sent to PDI45 controller '%s'.",
				command, pdi45->record->name );
		case 0x03:
			return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
			"Error 0x03: Input buffer overflow for "
			"PDI45 controller '%s'.",
				pdi45->record->name );
		case 0x04:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error 0x04: Non-printable characters were sent to "
			"PDI45 controller '%s'.",
				pdi45->record->name );
		case 0x05:
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error 0x05: Not enough characters received "
			"by PDI45 controller '%s'.",
				pdi45->record->name );
		case 0x06:
			return mx_error( MXE_TIMED_OUT, fname,
			"Error 0x06: Communication time-out error for "
			"PDI45 controller '%s'.",
				pdi45->record->name );
		case 0x07:
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"Error 0x07: The data value in command '%s' sent to "
			"PDI45 controller '%s' is outside the allowed range.",
				command, pdi45->record->name );
		case 0x10:
			return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR,fname,
			"Error 0x10: Improper interface connection for "
			"current PDI45 firmware for controller '%s'.",
				pdi45->record->name );
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Error #02x: Unrecognized error code received "
			"from PDI45 controller '%s' for command '%s'.",
				pdi45->record->name, command );
		}


	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unexpected response to command '%s'.  Response = '%s'.",
			write_buffer, read_buffer );
	}
#if defined(__BORLANDC__)
	return MX_SUCCESSFUL_RESULT;
#endif
}

static mx_status_type
mxi_pdi45_checksum( MX_PDI45 *pdi45, char *command, char *checksum )
{
	int i, length, value;

	length = strlen( command );

	/* Add up the character values. */

	value = 0;

	for ( i = 0; i < length; i++ ) {
		value += command[i];
	}

	/* Only interested in the low order 8 bits. */

	value &= 0xff;

	/* Convert to a character string. */

	sprintf( checksum, "%02X", value );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_pdi45_check_checksum( char *response )
{
	const char fname[] = "mxi_pdi45_check_checksum()";

	int i, num_chars, num_items;
	int returned_checksum, computed_checksum;
	char *ptr;

	if ( response[0] != 'A' ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The string '%s' is not a valid PDI Model 45 response.",
			response );
	}

	num_chars = strlen( response );

	if ( response[num_chars - 1] == '\r' ) {
		response[num_chars - 1] = '\0';

		num_chars -= 1;
	}

	/* Leave out the checksum field. */

	num_chars -= 2;

	if ( num_chars <= 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"The string '%s' is not long enough to be a valid PDI Model 45 response.",
			response );
	}

	/* Parse the returned checksum. */
		
	ptr = response + num_chars;

	num_items = sscanf( ptr, "%2x", &returned_checksum );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"The checksum field '%c%c' has non-hexadecimal characters in it.",
			ptr[0], ptr[1] );
	}

	/* Compute our own version of the checksum. */

	computed_checksum = 0;

	/* Skip over the 'A' at the beginning. */

	for ( i = 1; i < num_chars; i++ ) {
		computed_checksum += response[i];
	}

	computed_checksum &= 0xff;

	MX_DEBUG( 2, ("computed checksum = 0x%X, returned checksum = 0x%X",
		computed_checksum, returned_checksum));

	if ( computed_checksum == returned_checksum ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Checksum error: computed checksum = 0x%X, returned checksum = 0x%X",
			computed_checksum, returned_checksum );
	}
}

static mx_status_type
mxi_pdi45_get_error_code( char *buffer, int *error_code )
{
	const char fname[] = "mxi_pdi45_get_error_code()";

	char error_string[3];
	int num_items, code;

	if (( buffer[0] != 'N' ) || ( buffer[3] != '\0' )) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"'%s' is not a valid PDI Model 45 error code.", buffer );
	}

	error_string[0] = buffer[1];
	error_string[1] = buffer[2];
	error_string[2] = '\0';

	num_items = sscanf( error_string, "%x", &code );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The error code '%s' has non-hexadecimal characters in it.",
			error_string );
	}

	*error_code = code;

	return MX_SUCCESSFUL_RESULT;
}


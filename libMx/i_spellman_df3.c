/*
 * Name:    i_spellman_df3.c
 *
 * Purpose: MX interface driver for the Spellman DF3/FF3 series of
 *          high voltage power supplies for X-ray generators.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_SPELLMAN_DF3_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "i_spellman_df3.h"

MX_RECORD_FUNCTION_LIST mxi_spellman_df3_record_function_list = {
	NULL,
	mxi_spellman_df3_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_spellman_df3_open,
	NULL,
	NULL,
	mxi_spellman_df3_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxi_spellman_df3_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_SPELLMAN_DF3_STANDARD_FIELDS
};

long mxi_spellman_df3_num_record_fields
		= sizeof( mxi_spellman_df3_record_field_defaults )
			/ sizeof( mxi_spellman_df3_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_spellman_df3_rfield_def_ptr
			= &mxi_spellman_df3_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_spellman_df3_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_spellman_df3_create_record_structures()";

	MX_SPELLMAN_DF3 *spellman_df3;

	/* Allocate memory for the necessary structures. */

	spellman_df3 = (MX_SPELLMAN_DF3 *) malloc( sizeof(MX_SPELLMAN_DF3) );

	if ( spellman_df3 == (MX_SPELLMAN_DF3 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SPELLMAN_DF3 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = spellman_df3;

	record->record_function_list = &mxi_spellman_df3_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	spellman_df3->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_spellman_df3_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_spellman_df3_open()";

	MX_SPELLMAN_DF3 *spellman_df3;
	char response[40];
	int num_items;
	unsigned long software_version;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL.");
	}

	spellman_df3 = (MX_SPELLMAN_DF3 *) record->record_type_struct;

	if ( spellman_df3 == (MX_SPELLMAN_DF3 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SPELLMAN_DF3 pointer for record '%s' is NULL.",
			record->name);
	}

	/* Compute the query command interval in clock ticks. */

	if ( spellman_df3->query_interval > 0 ) {
		spellman_df3->ticks_per_query =
	    mx_convert_seconds_to_clock_ticks( spellman_df3->query_interval );

		spellman_df3->next_query_tick = mx_current_clock_tick();
	}

	/* Clear out any existing trash from the RS-232 line. */

	mx_status = mx_rs232_discard_unread_input( spellman_df3->rs232_record,
						MXI_SPELLMAN_DF3_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the Spellman power supply is present by asking
	 * for its software version.
	 */

	mx_status = mxi_spellman_df3_command( spellman_df3, "V",
						response, sizeof(response),
						MXI_SPELLMAN_DF3_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "B%lu", &software_version );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The response '%s' to the 'V' command for "
		"Spellman DF3/FF3 '%s' was not recognizable.",
			response, record->name );
	}

#if MXI_SPELLMAN_DF3_DEBUG
	MX_DEBUG(-2,("%s: Spellman DF3/FF3 '%s' software version = %lu",
			fname, record->name, software_version ));
#endif

	/* Initialize the monitor arrays by doing a query command. */

	mx_status = mxi_spellman_df3_query_command( spellman_df3 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Copy the voltage and current monitor values to the control array.
	 *
	 * NOTE: The copied values must be rescaled since control values
	 * run from 0 to 0xFFF, while monitor values run from 0 to 0x3FF.
	 * This basically means that the monitor values must be multiplied
	 * by 4.
	 */

	spellman_df3->analog_control[MXF_SPELLMAN_DF3_VOLTAGE_CONTROL] =
	  4 * spellman_df3->analog_monitor[MXF_SPELLMAN_DF3_VOLTAGE_MONITOR];

	spellman_df3->analog_control[MXF_SPELLMAN_DF3_CURRENT_CONTROL] =
	  4 * spellman_df3->analog_monitor[MXF_SPELLMAN_DF3_CURRENT_MONITOR];

	/* Copy in the default power and filament current limits. */

	spellman_df3->analog_control[MXF_SPELLMAN_DF3_POWER_LIMIT]
		= spellman_df3->default_power_limit;

	spellman_df3->analog_control[MXF_SPELLMAN_DF3_FILAMENT_CURRENT_LIMIT]
		= spellman_df3->default_filament_current_limit;

	/* Set all of the digital control fields to 0. */

	memset( spellman_df3->digital_control, 0,
		sizeof(spellman_df3->digital_control) );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_spellman_df3_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_spellman_df3_resynchronize()";

	MX_SPELLMAN_DF3 *spellman_df3;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	spellman_df3 = (MX_SPELLMAN_DF3 *) record->record_type_struct;

	if ( spellman_df3 == (MX_SPELLMAN_DF3 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SPELLMAN_DF3 pointer for record '%s' is NULL.",
			record->name);
	}

	mx_status = mx_resynchronize_record( spellman_df3->rs232_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_spellman_df3_command( MX_SPELLMAN_DF3 *spellman_df3,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_spellman_df3_command()";

	char local_command[100];
	unsigned long i, command_length, response_length;
	unsigned long command_checksum, expected_checksum, actual_checksum;
	char *checksum_ptr, *ptr;
	mx_status_type mx_status;

	if ( spellman_df3 == (MX_SPELLMAN_DF3 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SPELLMAN_DF3 pointer passed was NULL." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, spellman_df3->record->name ));
	}

	/* Compute the command checksum. */

	command_length = strlen(command);

	command_checksum = 0;

	for ( i = 0; i < command_length; i++ ) {
		command_checksum = command_checksum + command[i];
	}

	command_checksum = command_checksum % 256;

	/* Format the command to be sent to the power supply. */

	snprintf( local_command, sizeof(local_command),
		"%c%s%0lX", MX_CTRL_A, command, command_checksum );

#if 0
	{
		unsigned long local_command_length;
		int c;

		local_command_length = strlen(local_command);

		for ( i = 0; i < local_command_length; i++ ) {
			c = local_command[i];

			MX_DEBUG(-2,("%s: local_command[%lu] = %#x '%c'",
				fname, i, c, c));
		}
	}
#endif

	/* Send the command. */

	mx_status = mx_rs232_putline( spellman_df3->rs232_record,
					local_command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the response. */

	mx_status = mx_rs232_getline( spellman_df3->rs232_record,
					response, response_buffer_length,
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the two byte checksum is correct. */

	response_length = strlen(response);

	checksum_ptr = response + response_length - 2;

#if 0
	MX_DEBUG(-2,("%s: response '%s' checksum = '%s'",
		fname, response, checksum_ptr));
#endif

	expected_checksum = 0;

	for ( ptr = response; ptr < checksum_ptr; ptr++ ) {
		expected_checksum = expected_checksum + *ptr;
	}

	expected_checksum = expected_checksum % 256;

	/*---*/

	actual_checksum = 16 * mx_hex_char_to_unsigned_long( checksum_ptr[0] )
			     + mx_hex_char_to_unsigned_long( checksum_ptr[1] );

	if ( actual_checksum != expected_checksum ) {
		mx_warning( "The response '%s' to the command '%s' sent to "
		"Spellman DF3/FF3 '%s' had a checksum %#lX that did not match "
		"the expected checksum of %#lX.  Continuing anyway.",
			response, command, spellman_df3->record->name,
			actual_checksum, expected_checksum );
	}

	/* Strip off the two byte checksum. */

	*checksum_ptr = '\0';

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, spellman_df3->record->name ));
	}

	/* If the response was not an error response, then we are done now. */

	if ( response[0] != 'E' ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* We received an error code.  Return an approprate error
	 * to the caller.
	 */

	switch( response[1] ) {
	case '1':
		return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
		"Local Mode Error for '%s': A set command was attempted "
		"while the power supply was set to Local Mode.",
			spellman_df3->record->name );
		break;
	case '2':
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Undefined Command Code for '%s': The command character '%c' "
		"received was not an S, Q, or V.",
			spellman_df3->record->name, command[0] );
		break;
	case '3':
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Checksum Error for '%s': The transmitted checksum received "
		"in the command packet did not match the checksum calculated "
		"on the received bytes.",
			spellman_df3->record->name );
		break;
	case '4':
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Extra Byte(s) Received for '%s':  A byte other than the "
		"carriage return character was received in the last expected "
		"byte position of the command.",
			spellman_df3->record->name );
		break;
	case '5':
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"Illegal Digital Control Byte in Set Command for '%s': "
		"Only one of the conditions 'X-ray On', 'X-ray Off', "
		"and 'Power Supply Reset' can be set in the digital control "
		"byte of the Set command at any one time.",
			spellman_df3->record->name );
		break;
	case '6':
		return mx_error( MXE_NOT_READY, fname,
		"Illegal Set Command Received While a Fault is Active "
		"for '%s': You must send a Power Supply Reset before "
		"sending any other commands.",
			spellman_df3->record->name );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Unrecognized error code '%c' was seen in the response '%s' "
		"to command '%s' for Spellman power supply '%s'.",
			response[1], response, command,
			spellman_df3->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_spellman_df3_query_command( MX_SPELLMAN_DF3 *spellman_df3 )
{
	static const char fname[] = "mxi_spellman_df3_query_command()";

	char response[40];
	unsigned long monitor_byte;
	mx_status_type mx_status;

	MX_CLOCK_TICK current_tick;
	int tick_comparison;

	/* If the 'query_interval' field for the 'spellman_df3' record
	 * is greater than 0, then this driver has been configured to
	 * require a minimum separation in time between query commands
	 * sent to the hardware device.  If the next query time has
	 * not been reached, then we just return and the caller will
	 * use the values already found in the 'analog_monitor' and
	 * 'digital_monitor' arrays of the MX_SPELLMAN_DF3 structure.
	 */

	if ( spellman_df3->query_interval > 0 ) {
		current_tick = mx_current_clock_tick();

		tick_comparison = mx_compare_clock_ticks( current_tick,
					spellman_df3->next_query_tick );

		if ( tick_comparison < 0 ) {
			/* It is not yet time for the next query command. */

#if MXI_SPELLMAN_DF3_DEBUG
			MX_DEBUG(-2,("%s: Using cached monitor values.",fname));
#endif

			return MX_SUCCESSFUL_RESULT;
		}

		spellman_df3->next_query_tick = mx_add_clock_ticks(
			current_tick, spellman_df3->ticks_per_query );
	}

	/* Send the query command. */

	mx_status = mxi_spellman_df3_command( spellman_df3, "Q",
					response, sizeof(response),
					MXI_SPELLMAN_DF3_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The response should be 13 characters long and start with 
	 * the letter 'R'.  The checksum and carriage return characters
	 * will already have been stripped off by the time that we
	 * get here.
	 */

	if ( response[0] != 'R' ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The first character of the response '%s' to the command 'Q' "
		"was not the letter 'R' for Spellman DF3/FF3 '%s'.",
			response, spellman_df3->record->name );
	}

	spellman_df3->analog_monitor[MXF_SPELLMAN_DF3_VOLTAGE_MONITOR] =
		 256 * mx_hex_char_to_unsigned_long( response[1] )
		+ 16 * mx_hex_char_to_unsigned_long( response[2] )
		+      mx_hex_char_to_unsigned_long( response[3] );

	spellman_df3->analog_monitor[MXF_SPELLMAN_DF3_CURRENT_MONITOR] =
		 256 * mx_hex_char_to_unsigned_long( response[4] )
		+ 16 * mx_hex_char_to_unsigned_long( response[5] )
		+      mx_hex_char_to_unsigned_long( response[6] );

	spellman_df3->analog_monitor[
				MXF_SPELLMAN_DF3_FILAMENT_CURRENT_MONITOR] =
		 256 * mx_hex_char_to_unsigned_long( response[7] )
		+ 16 * mx_hex_char_to_unsigned_long( response[8] )
		+      mx_hex_char_to_unsigned_long( response[9] );

	monitor_byte = mx_hex_char_to_unsigned_long( response[10] );

	if ( monitor_byte & 0x8 ) {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_INTERLOCK_STATUS]
							    	= TRUE;
	} else {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_INTERLOCK_STATUS]
	    							= FALSE;
	}

	if ( monitor_byte & 0x4 ) {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_XRAY_ON_INDICATOR]
							    	= TRUE;
	} else {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_XRAY_ON_INDICATOR]
	    							= FALSE;
	}

	if ( monitor_byte & 0x2 ) {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_POWER_SUPPLY_FAULT]
							    	= TRUE;
	} else {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_POWER_SUPPLY_FAULT]
	    							= FALSE;
	}

	if ( monitor_byte & 0x1 ) {
	    spellman_df3->digital_monitor[
	    	MXF_SPELLMAN_DF3_FILAMENT_CURRENT_LIMIT_FAULT] = TRUE;
	} else {
	    spellman_df3->digital_monitor[
	    	MXF_SPELLMAN_DF3_FILAMENT_CURRENT_LIMIT_FAULT] = FALSE;
	}

	monitor_byte = mx_hex_char_to_unsigned_long( response[11] );

	if ( monitor_byte & 0x8 ) {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_OVERVOLTAGE_FAULT]
							    	= TRUE;
	} else {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_OVERVOLTAGE_FAULT]
	    							= FALSE;
	}

	if ( monitor_byte & 0x4 ) {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_OVERPOWER_FAULT]
							    	= TRUE;
	} else {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_OVERPOWER_FAULT]
	    							= FALSE;
	}

	if ( monitor_byte & 0x2 ) {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_OVERCURRENT_FAULT]
							    	= TRUE;
	} else {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_OVERCURRENT_FAULT]
	    							= FALSE;
	}

	if ( monitor_byte & 0x1 ) {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_KV_MINIMUM_FAULT]
							    	= TRUE;
	} else {
	    spellman_df3->digital_monitor[MXF_SPELLMAN_DF3_KV_MINIMUM_FAULT]
	    							= FALSE;
	}

	monitor_byte = mx_hex_char_to_unsigned_long( response[12] );

	if ( monitor_byte & 0x1 ) {
	    spellman_df3->digital_monitor[
	    	MXF_SPELLMAN_DF3_CONTROL_MODE_INDICATOR] = TRUE;
	} else {
	    spellman_df3->digital_monitor[
	    	MXF_SPELLMAN_DF3_CONTROL_MODE_INDICATOR] = FALSE;
	}

#if 0
	{
		int i, num_digital_monitors;

		num_digital_monitors = sizeof(spellman_df3->digital_monitor)
				/ sizeof(spellman_df3->digital_monitor[0]);

		for ( i = 0; i < num_digital_monitors; i++ ) {
			MX_DEBUG(-2,("%s: digital_monitor[%d] = %d",
				fname, i, spellman_df3->digital_monitor[i]));
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_spellman_df3_set_command( MX_SPELLMAN_DF3 *spellman_df3 )
{
	static const char fname[] = "mxi_spellman_df3_set_command()";

	char command[40];
	char response[40];
	int *digital_monitor, *digital_control;
	int digital_control_data;
	mx_bool_type fault;
	int raw_voltage_command, raw_current_command;
	int raw_power_limit, raw_filament_current_limit;
	mx_status_type mx_status;

	/* Start by using a query command to see if any faults are present. */

	mx_status = mxi_spellman_df3_query_command( spellman_df3 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	digital_monitor = spellman_df3->digital_monitor;

	fault = digital_monitor[MXF_SPELLMAN_DF3_KV_MINIMUM_FAULT]
	    | digital_monitor[MXF_SPELLMAN_DF3_OVERCURRENT_FAULT]
	    | digital_monitor[MXF_SPELLMAN_DF3_OVERPOWER_FAULT]
	    | digital_monitor[MXF_SPELLMAN_DF3_OVERVOLTAGE_FAULT]
	    | digital_monitor[MXF_SPELLMAN_DF3_FILAMENT_CURRENT_LIMIT_FAULT]
	    | digital_monitor[MXF_SPELLMAN_DF3_POWER_SUPPLY_FAULT]
	    | digital_monitor[MXF_SPELLMAN_DF3_INTERLOCK_STATUS];

	/* Construct the command to be sent to the power supply.
	 *
	 * The leading ^A character and trailing checksum and ^M will be
	 * added by the function mxi_spellman_df3_command().
	 */

	command[0] = 'S';

	raw_voltage_command =
	    spellman_df3->analog_control[MXF_SPELLMAN_DF3_VOLTAGE_CONTROL];

	snprintf( &(command[1]), 4, "%0X", raw_voltage_command );

	raw_current_command =
	    spellman_df3->analog_control[MXF_SPELLMAN_DF3_CURRENT_CONTROL];

	snprintf( &(command[4]), 4, "%0X", raw_current_command );

	raw_power_limit =
	    spellman_df3->analog_control[MXF_SPELLMAN_DF3_POWER_LIMIT];

	snprintf( &(command[7]), 4, "%0X", raw_power_limit );

	raw_filament_current_limit =
    spellman_df3->analog_control[MXF_SPELLMAN_DF3_FILAMENT_CURRENT_LIMIT];

	snprintf( &(command[10]), 4, "%0X", raw_filament_current_limit );

	digital_control = spellman_df3->digital_control;

	if ( fault ) {
		digital_control_data = 0x4;
	} else
	if ( digital_control[1] ) {
		digital_control_data = 0x2;
	} else
	if ( digital_control[0] ) {
		digital_control_data = 0x1;
	} else {
		digital_control_data = 0;
	}

	snprintf( &(command[13]), 2, "%0X", digital_control_data );

#if 0
	{
		int i;

		for ( i = 0; i < 14; i++ ) {
			MX_DEBUG(-2,("%s: command[%d] = %#x '%c'",
				fname, i, command[i], command[i]));
		}
	}
#endif

	/* Now send the command. */

	mx_status = mxi_spellman_df3_command( spellman_df3, command,
						response, sizeof(response),
						MXI_SPELLMAN_DF3_DEBUG );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the command was successful, we should get an 'A' character
	 * back as the response.
	 */

	if ( response[0] == 'A' ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, then some sort of error occurred. */

	return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Received an unexpected response '%s' to command '%s' "
	"for Spellman DF3/FF3 '%s'.", response, command,
		spellman_df3->record->name );
}


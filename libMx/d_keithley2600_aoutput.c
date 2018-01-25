/*
 * Name:    d_keithley2600_aoutput.c
 *
 * Purpose: MX driver to control Keithley 2600 series analog outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY2600_AOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "i_keithley.h"
#include "i_keithley2600.h"
#include "d_keithley2600_aoutput.h"

/* Initialize the aoutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_keithley2600_aoutput_record_function_list = {
	NULL,
	mxd_keithley2600_aoutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_keithley2600_aoutput_open
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_keithley2600_aoutput_analog_output_function_list =
{
	NULL,
	mxd_keithley2600_aoutput_write
};

/* Keithley 2600 analog output data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_keithley2600_aoutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_KEITHLEY2600_AOUTPUT_STANDARD_FIELDS
};

long mxd_keithley2600_aoutput_num_record_fields
		= sizeof( mxd_keithley2600_aoutput_rf_defaults )
		  / sizeof( mxd_keithley2600_aoutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keithley2600_aoutput_rfield_def_ptr
			= &mxd_keithley2600_aoutput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_keithley2600_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
				MX_KEITHLEY2600_AOUTPUT **keithley2600_aoutput,
				MX_KEITHLEY2600 **keithley2600,
				const char *calling_fname )
{
	static const char fname[] = "mxd_keithley2600_aoutput_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_KEITHLEY2600_AOUTPUT *keithley2600_aoutput_ptr;
	MX_KEITHLEY2600 *keithley2600_ptr;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = aoutput->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_ANALOG_OUTPUT pointer passed was NULL." );
	}

	keithley2600_aoutput_ptr = (MX_KEITHLEY2600_AOUTPUT *)
					record->record_type_struct;

	if ( keithley2600_aoutput_ptr == (MX_KEITHLEY2600_AOUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2600_AOUTPUT pointer for aoutput '%s' is NULL.",
			record->name );
	}

	if ( keithley2600_aoutput != (MX_KEITHLEY2600_AOUTPUT **) NULL ) {
		*keithley2600_aoutput = keithley2600_aoutput_ptr;
	}

	controller_record = keithley2600_aoutput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for aoutput '%s' is NULL.",
				record->name );
	}

	keithley2600_ptr = (MX_KEITHLEY2600 *)
					controller_record->record_type_struct;

	if ( keithley2600_ptr == (MX_KEITHLEY2600 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_KEITHLEY2600 pointer for controller record '%s' is NULL.",
				controller_record->name );
	}

	if ( keithley2600 != (MX_KEITHLEY2600 **) NULL ) {
		*keithley2600 = keithley2600_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_keithley2600_aoutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_keithley2600_aoutput_create_record_structures()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_KEITHLEY2600_AOUTPUT *keithley2600_aoutput;

	/* Allocate memory for the necessary structures. */

	aoutput = (MX_ANALOG_OUTPUT *) malloc( sizeof(MX_ANALOG_OUTPUT) );

	if ( aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_OUTPUT structure." );
	}

	keithley2600_aoutput = (MX_KEITHLEY2600_AOUTPUT *)
				malloc( sizeof(MX_KEITHLEY2600_AOUTPUT) );

	if ( keithley2600_aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY2600_AOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = aoutput;
	record->record_type_struct = keithley2600_aoutput;
	record->class_specific_function_list
			= &mxd_keithley2600_aoutput_analog_output_function_list;

	aoutput->record = record;

	aoutput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2600_aoutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2600_aoutput_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_KEITHLEY2600_AOUTPUT *keithley2600_aoutput;
	MX_KEITHLEY2600 *keithley2600;
	char command[MXU_KEITHLEY2600_COMMAND_LENGTH+1];
	int i, length;
	mx_status_type mx_status;

	aoutput = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2600_aoutput_get_pointers( aoutput,
		&keithley2600_aoutput, &keithley2600, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( islower( (int) keithley2600_aoutput->channel_name ) ) {
		keithley2600_aoutput->channel_name
			= toupper( (int) keithley2600_aoutput->channel_name );
	}

	keithley2600_aoutput->lowercase_channel_name
		= tolower( (int) keithley2600_aoutput->channel_name );

	switch( keithley2600_aoutput->channel_name ) {
	case 'A':
	    keithley2600_aoutput->channel_number = 1;
	    break;
	case 'B':
	    keithley2600_aoutput->channel_number = 2;
	    break;
	default:
	    return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Illegal channel name '%c' specified for analog output '%s'",
	    	keithley2600_aoutput->channel_name, record->name );
	    break;
	}

	length = strlen( keithley2600_aoutput->signal_type );

	if ( mx_strncasecmp( "voltage",
			keithley2600_aoutput->signal_type, length ) == 0 )
	{
		keithley2600_aoutput->lowercase_signal_type = 'v';
	} else
	if ( mx_strncasecmp( "current",
			keithley2600_aoutput->signal_type, length ) == 0 )
	{
		keithley2600_aoutput->lowercase_signal_type = 'i';
	} else
	if ( mx_strncasecmp( "i",
			keithley2600_aoutput->signal_type, length ) == 0 )
	{
		keithley2600_aoutput->lowercase_signal_type = 'i';
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized signal type '%s' for analog output '%s'.  "
		"The allowed signal types are 'voltage', 'current', "
		"'v' or 'i'.",
			keithley2600_aoutput->signal_type,
			aoutput->record->name );
	}

	/*======== Initialize the controller. ========*/

	/* Turn on autoranging of current and voltage. */

	snprintf( command, sizeof(command),
		"smu%c.measure_autorangei = smu%c.AUTORANGE_ON",
		keithley2600_aoutput->lowercase_channel_name,
		keithley2600_aoutput->lowercase_channel_name );

	mx_status = mxi_keithley2600_command( keithley2600,
				command, NULL, 0,
				keithley2600->keithley2600_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"smu%c.measure_autorangev = smu%c.AUTORANGE_ON",
		keithley2600_aoutput->lowercase_channel_name,
		keithley2600_aoutput->lowercase_channel_name );

	mx_status = mxi_keithley2600_command( keithley2600,
				command, NULL, 0,
				keithley2600->keithley2600_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear the buffers. */

	for ( i = 1; i < 3; i++ ) {

		snprintf( command, sizeof(command),
			"smu%c.nvbuffer%d.clear()",
			keithley2600_aoutput->lowercase_channel_name, i );

		mx_status = mxi_keithley2600_command( keithley2600,
				command, NULL, 0,
				keithley2600->keithley2600_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley2600_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_keithley2600_aoutput_write()";

	MX_KEITHLEY2600_AOUTPUT *keithley2600_aoutput;
	MX_KEITHLEY2600 *keithley2600;
	char command[MXU_KEITHLEY2600_COMMAND_LENGTH+1];
	char response[MXU_KEITHLEY2600_RESPONSE_LENGTH+1];
	int num_outputs;
	mx_status_type mx_status;

	mx_status = mxd_keithley2600_aoutput_get_pointers( aoutput,
		&keithley2600_aoutput, &keithley2600, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the controller to take single measurements. */

	snprintf( command, sizeof(command),
		"smu%c.measure_count = 1",
		keithley2600_aoutput->lowercase_channel_name );

	mx_status = mxi_keithley2600_command( keithley2600,
				command, NULL, 0,
				keithley2600->keithley2600_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"smu%c.measure.interval = 0.1",
		keithley2600_aoutput->lowercase_channel_name );

	mx_status = mxi_keithley2600_command( keithley2600,
				command, NULL, 0,
				keithley2600->keithley2600_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"print(smu%c.measure.%c())",
		keithley2600_aoutput->lowercase_channel_name,
		keithley2600_aoutput->lowercase_signal_type );

	mx_status = mxi_keithley2600_command( keithley2600, command,
				response, sizeof(response),
				keithley2600->keithley2600_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_outputs = sscanf( response, "%lg",
			&(aoutput->raw_value.double_value) );

	if ( num_outputs != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find a number in the response '%s' to command '%s' "
		"for analog output '%s'.",
			response, command, aoutput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}


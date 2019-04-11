/*
 * Name:    d_numato_gpio_ainput.c
 *
 * Purpose: MX driver to control Keithley 2600 series analog inputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define NUMATO_GPIO_AINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_analog_input.h"
#include "i_keithley.h"
#include "i_numato_gpio.h"
#include "d_numato_gpio_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_numato_gpio_ainput_record_function_list = {
	NULL,
	mxd_numato_gpio_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_numato_gpio_ainput_open,
	NULL,
	NULL,
	NULL,
	mxd_numato_gpio_ainput_special_processing_setup
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_numato_gpio_ainput_analog_input_function_list =
{
	mxd_numato_gpio_ainput_read
};

/* Keithley 2600 analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_numato_gpio_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_NUMATO_GPIO_AINPUT_STANDARD_FIELDS
};

long mxd_numato_gpio_ainput_num_record_fields
		= sizeof( mxd_numato_gpio_ainput_rf_defaults )
		  / sizeof( mxd_numato_gpio_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_numato_gpio_ainput_rfield_def_ptr
			= &mxd_numato_gpio_ainput_rf_defaults[0];

/*----*/

static mx_status_type
mxd_numato_gpio_ainput_process_function( void *, void *, void *, int );

/* A private function for the use of the driver. */

static mx_status_type
mxd_numato_gpio_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_NUMATO_GPIO_AINPUT **numato_gpio_ainput,
				MX_NUMATO_GPIO **numato_gpio,
				const char *calling_fname )
{
	static const char fname[] = "mxd_numato_gpio_ainput_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_NUMATO_GPIO_AINPUT *numato_gpio_ainput_ptr;
	MX_NUMATO_GPIO *numato_gpio_ptr;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = ainput->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_ANALOG_INPUT pointer passed was NULL." );
	}

	numato_gpio_ainput_ptr = (MX_NUMATO_GPIO_AINPUT *)
					record->record_type_struct;

	if ( numato_gpio_ainput_ptr == (MX_NUMATO_GPIO_AINPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NUMATO_GPIO_AINPUT pointer for ainput '%s' is NULL.",
			record->name );
	}

	if ( numato_gpio_ainput != (MX_NUMATO_GPIO_AINPUT **) NULL ) {
		*numato_gpio_ainput = numato_gpio_ainput_ptr;
	}

	controller_record = numato_gpio_ainput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for ainput '%s' is NULL.",
				record->name );
	}

	numato_gpio_ptr = (MX_NUMATO_GPIO *)
					controller_record->record_type_struct;

	if ( numato_gpio_ptr == (MX_NUMATO_GPIO *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_NUMATO_GPIO pointer for controller record '%s' is NULL.",
				controller_record->name );
	}

	if ( numato_gpio != (MX_NUMATO_GPIO **) NULL ) {
		*numato_gpio = numato_gpio_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_numato_gpio_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_numato_gpio_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_NUMATO_GPIO_AINPUT *numato_gpio_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	numato_gpio_ainput = (MX_NUMATO_GPIO_AINPUT *)
				malloc( sizeof(MX_NUMATO_GPIO_AINPUT) );

	if ( numato_gpio_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NUMATO_GPIO_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = numato_gpio_ainput;
	record->class_specific_function_list
			= &mxd_numato_gpio_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_numato_gpio_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_numato_gpio_ainput_open()";

	MX_ANALOG_INPUT *ainput;
	MX_NUMATO_GPIO_AINPUT *numato_gpio_ainput;
	MX_NUMATO_GPIO *numato_gpio;
	char command[MXU_NUMATO_GPIO_COMMAND_LENGTH+1];
	int i, length;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_numato_gpio_ainput_get_pointers( ainput,
		&numato_gpio_ainput, &numato_gpio, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( islower( (int) numato_gpio_ainput->channel_name ) ) {
		numato_gpio_ainput->channel_name
			= toupper( (int) numato_gpio_ainput->channel_name );
	}

	numato_gpio_ainput->lowercase_channel_name
		= tolower( (int) numato_gpio_ainput->channel_name );

	switch( numato_gpio_ainput->channel_name ) {
	case 'A':
	    numato_gpio_ainput->channel_number = 1;
	    break;
	case 'B':
	    numato_gpio_ainput->channel_number = 2;
	    break;
	default:
	    return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Illegal channel name '%c' specified for analog input '%s'",
	    	numato_gpio_ainput->channel_name, record->name );
	    break;
	}

	length = strlen( numato_gpio_ainput->signal_type );

	if ( mx_strncasecmp( "voltage",
			numato_gpio_ainput->signal_type, length ) == 0 )
	{
		numato_gpio_ainput->lowercase_signal_type = 'v';
	} else
	if ( mx_strncasecmp( "current",
			numato_gpio_ainput->signal_type, length ) == 0 )
	{
		numato_gpio_ainput->lowercase_signal_type = 'i';
	} else
	if ( mx_strncasecmp( "i",
			numato_gpio_ainput->signal_type, length ) == 0 )
	{
		numato_gpio_ainput->lowercase_signal_type = 'i';
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized signal type '%s' for analog input '%s'.  "
		"The allowed signal types are 'voltage', 'current', "
		"'v' or 'i'.",
			numato_gpio_ainput->signal_type,
			ainput->record->name );
	}

	/*======== Initialize the controller. ========*/

	/* Turn on autoranging of current and voltage. */

	snprintf( command, sizeof(command),
		"smu%c.measure_autorangei = smu%c.AUTORANGE_ON",
		numato_gpio_ainput->lowercase_channel_name,
		numato_gpio_ainput->lowercase_channel_name );

	mx_status = mxi_numato_gpio_command( numato_gpio,
				command, NULL, 0,
				numato_gpio->numato_gpio_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"smu%c.measure_autorangev = smu%c.AUTORANGE_ON",
		numato_gpio_ainput->lowercase_channel_name,
		numato_gpio_ainput->lowercase_channel_name );

	mx_status = mxi_numato_gpio_command( numato_gpio,
				command, NULL, 0,
				numato_gpio->numato_gpio_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear the buffers. */

	for ( i = 1; i < 3; i++ ) {

		snprintf( command, sizeof(command),
			"smu%c.nvbuffer%d.clear()",
			numato_gpio_ainput->lowercase_channel_name, i );

		mx_status = mxi_numato_gpio_command( numato_gpio,
				command, NULL, 0,
				numato_gpio->numato_gpio_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_numato_gpio_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_numato_gpio_ainput_read()";

	MX_NUMATO_GPIO_AINPUT *numato_gpio_ainput;
	MX_NUMATO_GPIO *numato_gpio;
	char command[MXU_NUMATO_GPIO_COMMAND_LENGTH+1];
	char response[MXU_NUMATO_GPIO_RESPONSE_LENGTH+1];
	int num_inputs;
	mx_status_type mx_status;

	mx_status = mxd_numato_gpio_ainput_get_pointers( ainput,
		&numato_gpio_ainput, &numato_gpio, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the controller to take single measurements. */

	snprintf( command, sizeof(command),
		"smu%c.measure_count = 1",
		numato_gpio_ainput->lowercase_channel_name );

	mx_status = mxi_numato_gpio_command( numato_gpio,
				command, NULL, 0,
				numato_gpio->numato_gpio_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"smu%c.measure.interval = 0.1",
		numato_gpio_ainput->lowercase_channel_name );

	mx_status = mxi_numato_gpio_command( numato_gpio,
				command, NULL, 0,
				numato_gpio->numato_gpio_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"print(smu%c.measure.%c())",
		numato_gpio_ainput->lowercase_channel_name,
		numato_gpio_ainput->lowercase_signal_type );

	mx_status = mxi_numato_gpio_command( numato_gpio, command,
				response, sizeof(response),
				numato_gpio->numato_gpio_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_inputs = sscanf( response, "%lg",
			&(ainput->raw_value.double_value) );

	if ( num_inputs != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find a number in the response '%s' to command '%s' "
		"for analog input '%s'.",
			response, command, ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_numato_gpio_ainput_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_numato_gpio_ainput_special_processing_setup()";

	MX_RECORD_FIELD *record_field = NULL;
	MX_RECORD_FIELD *record_field_array = NULL;
	long i;

	MX_DEBUG(-2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value )  {
		case MXLV_NUMATO_GPIO_AINPUT_SIGNAL_TYPE:
			record_field->process_function
				= mxd_numato_gpio_ainput_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_numato_gpio_ainput_process_function( void *record_ptr,
					void *record_field_ptr,
					void *socket_handler_ptr,
					int operation )
{
	static const char fname[] =
		"mxd_numato_gpio_ainput_special_processing_setup()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		default:
			MX_DEBUG( 1,
			("%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value ));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_NUMATO_GPIO_AINPUT_SIGNAL_TYPE:
			/* Re-executing the open() routine does
			 * all that is needed.
			 */
			mx_status = mxd_numato_gpio_ainput_open( record );
			break;
		default:
			MX_DEBUG( 1,
			("%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value ));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
		break;
	}

	return mx_status;
}


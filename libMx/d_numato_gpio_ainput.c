/*
 * Name:    d_numato_gpio_ainput.c
 *
 * Purpose: MX driver for Numato Lab GPIO analog inputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define NUMATO_GPIO_AINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_numato_gpio.h"
#include "d_numato_gpio_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_numato_gpio_ainput_record_function_list = {
	NULL,
	mxd_numato_gpio_ainput_create_record_structures
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

/* A private function for the use of the driver. */

static mx_status_type
mxd_numato_gpio_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_NUMATO_GPIO_AINPUT **numato_gpio_ainput,
				MX_NUMATO_GPIO **numato_gpio,
				const char *calling_fname )
{
	static const char fname[] = "mxd_numato_gpio_ainput_get_pointers()";

	MX_RECORD *record, *numato_gpio_record;
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

	numato_gpio_record = numato_gpio_ainput_ptr->numato_gpio_record;

	if ( numato_gpio_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'numato_gpio_record' pointer for ainput '%s' is NULL.",
				record->name );
	}

	numato_gpio_ptr = (MX_NUMATO_GPIO *)
					numato_gpio_record->record_type_struct;

	if ( numato_gpio_ptr == (MX_NUMATO_GPIO *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_NUMATO_GPIO pointer for controller record '%s' is NULL.",
				numato_gpio_record->name );
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
	numato_gpio_ainput->record = record;

	ainput->subclass = MXT_AIN_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_numato_gpio_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_numato_gpio_ainput_read()";

	MX_NUMATO_GPIO_AINPUT *numato_gpio_ainput = NULL;
	MX_NUMATO_GPIO *numato_gpio = NULL;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_numato_gpio_ainput_get_pointers( ainput,
		&numato_gpio_ainput, &numato_gpio, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"adc read %ld", numato_gpio_ainput->channel_number );

	mx_status = mxi_numato_gpio_command( numato_gpio, command,
						response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &(ainput->raw_value.long_value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Did not see the status of analog input '%s' in the "
		"response '%s' to command '%s'.",
			ainput->record->name, response, command );
	}

	return MX_SUCCESSFUL_RESULT;
}


/*
 * Name:    d_numato_gpio_doutput.c
 *
 * Purpose: MX driver for Numato Lab GPIO digital outputs.
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

#define NUMATO_GPIO_DOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_numato_gpio.h"
#include "d_numato_gpio_doutput.h"

/* Initialize the doutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_numato_gpio_doutput_record_function_list = {
	NULL,
	mxd_numato_gpio_doutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_numato_gpio_doutput_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_numato_gpio_doutput_digital_output_function_list =
{
	NULL,
	mxd_numato_gpio_doutput_write
};

/* Numato GPIO digital output data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_numato_gpio_doutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_NUMATO_GPIO_DOUTPUT_STANDARD_FIELDS
};

long mxd_numato_gpio_doutput_num_record_fields
		= sizeof( mxd_numato_gpio_doutput_rf_defaults )
		  / sizeof( mxd_numato_gpio_doutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_numato_gpio_doutput_rfield_def_ptr
			= &mxd_numato_gpio_doutput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_numato_gpio_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
				MX_NUMATO_GPIO_DOUTPUT **numato_gpio_doutput,
				MX_NUMATO_GPIO **numato_gpio,
				const char *calling_fname )
{
	static const char fname[] = "mxd_numato_gpio_doutput_get_pointers()";

	MX_RECORD *record = NULL;
	MX_RECORD *numato_gpio_record = NULL;
	MX_NUMATO_GPIO_DOUTPUT *numato_gpio_doutput_ptr;
	MX_NUMATO_GPIO *numato_gpio_ptr;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = doutput->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}

	numato_gpio_doutput_ptr = (MX_NUMATO_GPIO_DOUTPUT *)
					record->record_type_struct;

	if ( numato_gpio_doutput_ptr == (MX_NUMATO_GPIO_DOUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_NUMATO_GPIO_DOUTPUT pointer for digital output '%s' is NULL.",
			record->name );
	}

	if ( numato_gpio_doutput != (MX_NUMATO_GPIO_DOUTPUT **) NULL ) {
		*numato_gpio_doutput = numato_gpio_doutput_ptr;
	}

	numato_gpio_record = numato_gpio_doutput_ptr->numato_gpio_record;

	if ( numato_gpio_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The 'numato_gpio_record' pointer for digital output '%s' is NULL.",
							record->name );
	}

	numato_gpio_ptr = (MX_NUMATO_GPIO *)
					numato_gpio_record->record_type_struct;

	if ( numato_gpio != (MX_NUMATO_GPIO **) NULL ) {
		*numato_gpio = numato_gpio_ptr;

		if ( (*numato_gpio) == (MX_NUMATO_GPIO *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NUMATO_GPIO pointer for Numato GPIO "
			"record '%s' is NULL.",
				numato_gpio_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_numato_gpio_doutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_numato_gpio_doutput_create_record_structures()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_NUMATO_GPIO_DOUTPUT *numato_gpio_doutput;

	/* Allocate memory for the necessary structures. */

	doutput = (MX_DIGITAL_OUTPUT *) malloc( sizeof(MX_DIGITAL_OUTPUT) );

	if ( doutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
	}

	numato_gpio_doutput = (MX_NUMATO_GPIO_DOUTPUT *)
				malloc( sizeof(MX_NUMATO_GPIO_DOUTPUT) );

	if ( numato_gpio_doutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NUMATO_GPIO_DOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = doutput;
	record->record_type_struct = numato_gpio_doutput;
	record->class_specific_function_list
		= &mxd_numato_gpio_doutput_digital_output_function_list;

	doutput->record = record;
	numato_gpio_doutput->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_numato_gpio_doutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_numato_gpio_doutput_open()";

	MX_DIGITAL_OUTPUT *doutput = NULL;
	MX_NUMATO_GPIO_DOUTPUT *numato_gpio_doutput = NULL;
	MX_NUMATO_GPIO *numato_gpio = NULL;
	long channel;
	unsigned long iomask, iodir;
	char command[80];
	mx_status_type mx_status;

	doutput = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	mx_status = mxd_numato_gpio_doutput_get_pointers( doutput,
				&numato_gpio_doutput, &numato_gpio, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel = numato_gpio_doutput->channel_number;

	if ( channel < 0 ) {
		iomask = 0xff;
		iodir = 0;
	} else {
		iomask = (1 << channel);
		iodir = 0;
	}

	snprintf( command, sizeof(command), "gpio iomask %02lx", iomask );

	mx_status = mxi_numato_gpio_command( numato_gpio, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "gpio iodir %02lx", iodir );

	mx_status = mxi_numato_gpio_command( numato_gpio, command, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_numato_gpio_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_numato_gpio_doutput_write()";

	MX_NUMATO_GPIO_DOUTPUT *numato_gpio_doutput;
	MX_NUMATO_GPIO *numato_gpio;
	long channel;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	mx_status = mxd_numato_gpio_doutput_get_pointers( doutput,
				&numato_gpio_doutput, &numato_gpio, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel = numato_gpio_doutput->channel_number;

	if ( channel < 0 ) {
		strlcpy( command, "gpio writeall", sizeof(command) );
	} else {
		if ( doutput->value == 0 ) {
			snprintf( command, sizeof(command),
				"gpio clear %ld", channel );
		} else {
			doutput->value = 1;

			snprintf( command, sizeof(command),
				"gpio set %ld", channel );
		}
	}

	mx_status = mxi_numato_gpio_command( numato_gpio, command,
						response, sizeof(response) );

	return mx_status;
}


/*
 * Name:    d_keithley2400_ainput.c
 *
 * Purpose: MX driver to control Keithley 2400 series analog inputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2005, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY2400_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_keithley.h"
#include "i_keithley2400.h"
#include "d_keithley2400_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_keithley2400_ainput_record_function_list = {
	NULL,
	mxd_keithley2400_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_keithley2400_ainput_open
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_keithley2400_ainput_analog_input_function_list =
{
	mxd_keithley2400_ainput_read
};

/* Keithley 2400 analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_keithley2400_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_KEITHLEY2400_AINPUT_STANDARD_FIELDS
};

long mxd_keithley2400_ainput_num_record_fields
		= sizeof( mxd_keithley2400_ainput_rf_defaults )
		  / sizeof( mxd_keithley2400_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keithley2400_ainput_rfield_def_ptr
			= &mxd_keithley2400_ainput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_keithley2400_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_KEITHLEY2400_AINPUT **keithley2400_ainput,
				MX_KEITHLEY2400 **keithley2400,
				MX_INTERFACE **interface,
				const char *calling_fname )
{
	static const char fname[] = "mxd_keithley2400_ainput_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_KEITHLEY2400_AINPUT *keithley2400_ainput_ptr;
	MX_KEITHLEY2400 *keithley2400_ptr;

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

	keithley2400_ainput_ptr = (MX_KEITHLEY2400_AINPUT *)
					record->record_type_struct;

	if ( keithley2400_ainput_ptr == (MX_KEITHLEY2400_AINPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2400_AINPUT pointer for ainput '%s' is NULL.",
			record->name );
	}

	if ( keithley2400_ainput != (MX_KEITHLEY2400_AINPUT **) NULL ) {
		*keithley2400_ainput = keithley2400_ainput_ptr;
	}

	controller_record = keithley2400_ainput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for ainput '%s' is NULL.",
				record->name );
	}

	keithley2400_ptr = (MX_KEITHLEY2400 *)
					controller_record->record_type_struct;

	if ( keithley2400_ptr == (MX_KEITHLEY2400 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_KEITHLEY2400 pointer for controller record '%s' is NULL.",
				controller_record->name );
	}

	if ( keithley2400 != (MX_KEITHLEY2400 **) NULL ) {
		*keithley2400 = keithley2400_ptr;
	}

	if ( interface != (MX_INTERFACE **) NULL ) {
		*interface = &(keithley2400_ptr->port_interface);

		if ( *interface == (MX_INTERFACE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERFACE pointer for Keithley 2400 '%s' is NULL.",
				keithley2400_ptr->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_keithley2400_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_keithley2400_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_KEITHLEY2400_AINPUT *keithley2400_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	keithley2400_ainput = (MX_KEITHLEY2400_AINPUT *)
				malloc( sizeof(MX_KEITHLEY2400_AINPUT) );

	if ( keithley2400_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY2400_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = keithley2400_ainput;
	record->class_specific_function_list
			= &mxd_keithley2400_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2400_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2400_ainput_open()";

	MX_ANALOG_INPUT *ainput;
	MX_KEITHLEY2400_AINPUT *keithley2400_ainput;
	MX_KEITHLEY2400 *keithley2400;
	MX_INTERFACE *interface;
	long gpib_address;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2400_ainput_get_pointers( ainput,
		&keithley2400_ainput, &keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( interface->record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by Keithley 2400 ainput '%s' is not "
		"an interface record.", interface->record->name, record->name );
	}

	switch( interface->record->mx_class ) {
	case MXI_RS232:
		break;
	case MXI_GPIB:
		gpib_address = interface->address;

		/* Check that the GPIB address is valid. */

		if ( gpib_address < 0 || gpib_address > 30 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"GPIB address %ld for record '%s' is out of allowed range 0-30.",
				gpib_address, record->name );
		}
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' is not an RS-232 or GPIB record.",
			interface->record->name );
	}

	mx_status = mxi_keithley2400_set_measurement_type( keithley2400,
			interface, keithley2400_ainput->measurement_type,
			KEITHLEY2400_AINPUT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley2400_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_keithley2400_ainput_read()";

	MX_KEITHLEY2400_AINPUT *keithley2400_ainput;
	MX_KEITHLEY2400 *keithley2400;
	MX_INTERFACE *interface;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_keithley2400_ainput_get_pointers( ainput,
		&keithley2400_ainput, &keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( keithley2400_ainput->measurement_type
		!= keithley2400->last_measurement_type )
	{
		mx_status = mxi_keithley2400_set_measurement_type(
				keithley2400, interface,
				keithley2400_ainput->measurement_type,
				KEITHLEY2400_AINPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	switch( keithley2400_ainput->measurement_type ) {
	case MXT_KEITHLEY2400_VOLT:
		strlcpy( command, ":MEAS:VOLT?", sizeof(command) );
		break;
	case MXT_KEITHLEY2400_CURR:
		strlcpy( command, ":MEAS:CURR?", sizeof(command) );
		break;
	case MXT_KEITHLEY2400_RES:
		strlcpy( command, ":MEAS:RES?", sizeof(command) );
		break;
	}

	mx_status = mxi_keithley_command( ainput->record, interface, command,
						response, sizeof(response),
						KEITHLEY2400_AINPUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the measured value. */

	if ( strlen( response ) == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No response was returned for command '%s' for record '%s'.",
			command, ainput->record->name );
	}

	num_items = sscanf(response, "%lg", &(ainput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cannot parse response to command '%s' for record '%s'.  "
		"response = '%s'",
			command, ainput->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}


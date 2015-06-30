/*
 * Name:    d_keithley2700_ainput.c
 *
 * Purpose: MX driver to control Keithley 2700 analog inputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY2700_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_keithley.h"
#include "i_keithley2700.h"
#include "d_keithley2700_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_keithley2700_ainput_record_function_list = {
	NULL,
	mxd_keithley2700_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_keithley2700_ainput_open,
	mxd_keithley2700_ainput_close
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_keithley2700_ainput_analog_input_function_list =
{
	mxd_keithley2700_ainput_read
};

/* Keithley 2700 analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_keithley2700_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_KEITHLEY2700_AINPUT_STANDARD_FIELDS
};

long mxd_keithley2700_ainput_num_record_fields
		= sizeof( mxd_keithley2700_ainput_rf_defaults )
		  / sizeof( mxd_keithley2700_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keithley2700_ainput_rfield_def_ptr
			= &mxd_keithley2700_ainput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_keithley2700_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_KEITHLEY2700_AINPUT **keithley2700_ainput,
				MX_KEITHLEY2700 **keithley2700,
				MX_INTERFACE **interface,
				const char *calling_fname )
{
	static const char fname[] = "mxd_keithley2700_ainput_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_KEITHLEY2700_AINPUT *keithley2700_ainput_ptr;
	MX_KEITHLEY2700 *keithley2700_ptr;

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

	keithley2700_ainput_ptr = (MX_KEITHLEY2700_AINPUT *)
					record->record_type_struct;

	if ( keithley2700_ainput_ptr == (MX_KEITHLEY2700_AINPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2700_AINPUT pointer for ainput '%s' is NULL.",
			record->name );
	}

	if ( keithley2700_ainput != (MX_KEITHLEY2700_AINPUT **) NULL ) {
		*keithley2700_ainput = keithley2700_ainput_ptr;
	}

	controller_record = keithley2700_ainput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for ainput '%s' is NULL.",
				record->name );
	}

	keithley2700_ptr = (MX_KEITHLEY2700 *)
					controller_record->record_type_struct;

	if ( keithley2700_ptr == (MX_KEITHLEY2700 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_KEITHLEY2700 pointer for controller record '%s' is NULL.",
				controller_record->name );
	}

	if ( keithley2700 != (MX_KEITHLEY2700 **) NULL ) {
		*keithley2700 = keithley2700_ptr;
	}

	if ( interface != (MX_INTERFACE **) NULL ) {
		*interface = &(keithley2700_ptr->port_interface);

		if ( *interface == (MX_INTERFACE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERFACE pointer for Keithley 2700 '%s' is NULL.",
				keithley2700_ptr->record->name );
		}
	}

	if ( keithley2700_ptr->channel_type == (long **) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The channel_type array for Keithley 2700 '%s' "
		"used by record '%s' has not been set up.",
			keithley2700_ptr->record->name,
			ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_keithley2700_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_keithley2700_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_KEITHLEY2700_AINPUT *keithley2700_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	keithley2700_ainput = (MX_KEITHLEY2700_AINPUT *)
				malloc( sizeof(MX_KEITHLEY2700_AINPUT) );

	if ( keithley2700_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY2700_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = keithley2700_ainput;
	record->class_specific_function_list
			= &mxd_keithley2700_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2700_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2700_ainput_open()";

	MX_ANALOG_INPUT *ainput;
	MX_KEITHLEY2700_AINPUT *keithley2700_ainput;
	MX_KEITHLEY2700 *keithley2700;
	MX_INTERFACE *interface;
	long gpib_address;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2700_ainput_get_pointers( ainput,
		&keithley2700_ainput, &keithley2700, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( interface->record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by Keithley 2700 ainput '%s' is not "
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

	keithley2700_ainput->slot =
		mxi_keithley2700_slot( keithley2700_ainput->system_channel );

	keithley2700_ainput->channel =
		mxi_keithley2700_channel( keithley2700_ainput->system_channel );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2700_ainput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2700_ainput_close()";

	MX_ANALOG_INPUT *ainput;
	MX_KEITHLEY2700_AINPUT *keithley2700_ainput;
	MX_KEITHLEY2700 *keithley2700;
	MX_INTERFACE *interface;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2700_ainput_get_pointers( ainput,
		&keithley2700_ainput, &keithley2700, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley2700_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_keithley2700_ainput_read()";

	MX_KEITHLEY2700_AINPUT *keithley2700_ainput;
	MX_KEITHLEY2700 *keithley2700;
	MX_INTERFACE *interface;
	char command[80];
	char response[80];
	int num_items;
	long slot, channel;
	mx_status_type mx_status;

	mx_status = mxd_keithley2700_ainput_get_pointers( ainput,
		&keithley2700_ainput, &keithley2700, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the Keithley range. */

	slot = keithley2700_ainput->slot;
	channel = keithley2700_ainput->channel;

	/* Close this record's system channel.  In plainer terms, this
	 * selects the channel for reading.
	 */

	sprintf( command, "ROUT:CLOS (@%ld%02ld)", slot, channel );

	mx_status = mxi_keithley_command( ainput->record, interface, command,
					NULL, 0, KEITHLEY2700_AINPUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now trigger the channel and readout the recorded value. */

	mx_status = mxi_keithley_command( ainput->record, interface, "READ?",
						response, sizeof(response),
						KEITHLEY2700_AINPUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%lg", &(ainput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cannot parse response to command '%s' for record '%s'.  "
		"response = '%s'",
			command, ainput->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}


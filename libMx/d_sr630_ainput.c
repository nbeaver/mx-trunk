/*
 * Name:    d_sr630_ainput.c
 *
 * Purpose: MX driver to control Stanford Research Systems model SR630
 *          thermocouple input channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define SR630_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_sr630.h"
#include "d_sr630_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_sr630_ainput_record_function_list = {
	NULL,
	mxd_sr630_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_sr630_ainput_open,
	mxd_sr630_ainput_close
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_sr630_ainput_analog_input_function_list =
{
	mxd_sr630_ainput_read
};

/* SR630 analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_sr630_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_SR630_AINPUT_STANDARD_FIELDS
};

long mxd_sr630_ainput_num_record_fields
		= sizeof( mxd_sr630_ainput_rf_defaults )
		  / sizeof( mxd_sr630_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sr630_ainput_rfield_def_ptr
			= &mxd_sr630_ainput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_sr630_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_SR630_AINPUT **sr630_ainput,
				MX_SR630 **sr630,
				const char *calling_fname )
{
	static const char fname[] = "mxd_sr630_ainput_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_SR630_AINPUT *sr630_ainput_ptr;
	MX_SR630 *sr630_ptr;

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

	sr630_ainput_ptr = (MX_SR630_AINPUT *)
					record->record_type_struct;

	if ( sr630_ainput_ptr == (MX_SR630_AINPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SR630_AINPUT pointer for ainput '%s' is NULL.",
			record->name );
	}

	if ( sr630_ainput != (MX_SR630_AINPUT **) NULL ) {
		*sr630_ainput = sr630_ainput_ptr;
	}

	controller_record = sr630_ainput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for ainput '%s' is NULL.",
				record->name );
	}

	sr630_ptr = (MX_SR630 *) controller_record->record_type_struct;

	if ( sr630_ptr == (MX_SR630 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SR630 pointer for controller record '%s' is NULL.",
				controller_record->name );
	}

	if ( sr630 != (MX_SR630 **) NULL ) {
		*sr630 = sr630_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_sr630_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_sr630_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_SR630_AINPUT *sr630_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	sr630_ainput = (MX_SR630_AINPUT *)
				malloc( sizeof(MX_SR630_AINPUT) );

	if ( sr630_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SR630_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = sr630_ainput;
	record->class_specific_function_list
			= &mxd_sr630_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sr630_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sr630_ainput_open()";

	MX_ANALOG_INPUT *ainput;
	MX_SR630_AINPUT *sr630_ainput;
	MX_SR630 *sr630;
	MX_INTERFACE *interface;
	long gpib_address;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_sr630_ainput_get_pointers( ainput,
				&sr630_ainput, &sr630, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	interface = &(sr630->port_interface);

	if ( interface->record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by SR630 ainput '%s' is not "
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

	if ( ( sr630_ainput->channel_number < 1 )
	  || ( sr630_ainput->channel_number > 16 ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal channel number %d for SR630 analog input '%s'.  "
		"The allowed channel numbers are from 1 to 16.",
			sr630_ainput->channel_number,
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sr630_ainput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_sr630_ainput_close()";

	MX_ANALOG_INPUT *ainput;
	MX_SR630_AINPUT *sr630_ainput;
	MX_SR630 *sr630;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_sr630_ainput_get_pointers( ainput,
					&sr630_ainput, &sr630, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sr630_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_sr630_ainput_read()";

	MX_SR630_AINPUT *sr630_ainput;
	MX_SR630 *sr630;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_sr630_ainput_get_pointers( ainput,
					&sr630_ainput, &sr630, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "MEAS? %d", sr630_ainput->channel_number );

	mx_status = mxi_sr630_command( sr630, command,
					response, sizeof(response),
					SR630_AINPUT_DEBUG );

	num_items = sscanf(response, "%lg", &(ainput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cannot parse response to command '%s' for record '%s'.  "
		"response = '%s'",
			command, ainput->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}


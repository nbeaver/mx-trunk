/*
 * Name:    d_sr630_aoutput.c
 *
 * Purpose: MX driver to control Stanford Research Systems SR630 voltage
 *          analog outputs.
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

#define SR630_AOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "i_sr630.h"
#include "d_sr630_aoutput.h"

/* Initialize the aoutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_sr630_aoutput_record_function_list = {
	NULL,
	mxd_sr630_aoutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_sr630_aoutput_open
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_sr630_aoutput_analog_output_function_list =
{
	NULL,
	mxd_sr630_aoutput_write
};

/* SR630 analog output data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_sr630_aoutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_SR630_AOUTPUT_STANDARD_FIELDS
};

long mxd_sr630_aoutput_num_record_fields
		= sizeof( mxd_sr630_aoutput_rf_defaults )
		  / sizeof( mxd_sr630_aoutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sr630_aoutput_rfield_def_ptr
			= &mxd_sr630_aoutput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_sr630_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
				MX_SR630_AOUTPUT **sr630_aoutput,
				MX_SR630 **sr630,
				const char *calling_fname )
{
	static const char fname[] = "mxd_sr630_aoutput_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_SR630_AOUTPUT *sr630_aoutput_ptr;
	MX_SR630 *sr630_ptr;

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

	sr630_aoutput_ptr = (MX_SR630_AOUTPUT *)
					record->record_type_struct;

	if ( sr630_aoutput_ptr == (MX_SR630_AOUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SR630_AOUTPUT pointer for aoutput '%s' is NULL.",
			record->name );
	}

	if ( sr630_aoutput != (MX_SR630_AOUTPUT **) NULL ) {
		*sr630_aoutput = sr630_aoutput_ptr;
	}

	controller_record = sr630_aoutput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for aoutput '%s' is NULL.",
				record->name );
	}

	sr630_ptr = (MX_SR630 *)
					controller_record->record_type_struct;

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
mxd_sr630_aoutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_sr630_aoutput_create_record_structures()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_SR630_AOUTPUT *sr630_aoutput;

	/* Allocate memory for the necessary structures. */

	aoutput = (MX_ANALOG_OUTPUT *) malloc( sizeof(MX_ANALOG_OUTPUT) );

	if ( aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_OUTPUT structure." );
	}

	sr630_aoutput = (MX_SR630_AOUTPUT *)
				malloc( sizeof(MX_SR630_AOUTPUT) );

	if ( sr630_aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SR630_AOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = aoutput;
	record->record_type_struct = sr630_aoutput;
	record->class_specific_function_list
			= &mxd_sr630_aoutput_analog_output_function_list;

	aoutput->record = record;

	aoutput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sr630_aoutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sr630_aoutput_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_SR630_AOUTPUT *sr630_aoutput;
	MX_SR630 *sr630;
	MX_INTERFACE *interface;
	long gpib_address;
	char command[40];
	mx_status_type mx_status;

	aoutput = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_sr630_aoutput_get_pointers( aoutput,
					&sr630_aoutput, &sr630, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	interface = &(sr630->port_interface);

	if ( interface->record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by SR630 aoutput '%s' is not "
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

	if ( ( sr630_aoutput->channel_number < 1 )
	  || ( sr630_aoutput->channel_number > 4 ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal channel number %d for SR630 analog output '%s'.  "
		"The allowed range of channel numbers is 1 to 4.",
			sr630_aoutput->channel_number, record->name );
	}

	/* Configure the channel as a voltage output. */

	sprintf( command, "VOUT %d,1", sr630_aoutput->channel_number );

	mx_status = mxi_sr630_command( sr630, command,
				NULL, 0, SR630_AOUTPUT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sr630_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_sr630_aoutput_write()";

	MX_SR630_AOUTPUT *sr630_aoutput;
	MX_SR630 *sr630;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_sr630_aoutput_get_pointers( aoutput,
					&sr630_aoutput, &sr630, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "VOUT %d,%f", sr630_aoutput->channel_number,
				aoutput->raw_value.double_value );

	mx_status = mxi_sr630_command( sr630, command,
				NULL, 0, SR630_AOUTPUT_DEBUG );

	return mx_status;
}


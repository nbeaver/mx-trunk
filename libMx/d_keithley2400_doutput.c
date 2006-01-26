/*
 * Name:    d_keithley2400_doutput.c
 *
 * Purpose: MX driver to control Keithley 2400 series digital outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY2400_DOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_keithley.h"
#include "i_keithley2400.h"
#include "d_keithley2400_doutput.h"

/* Initialize the doutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_keithley2400_doutput_record_function_list = {
	NULL,
	mxd_keithley2400_doutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_keithley2400_doutput_open,
	mxd_keithley2400_doutput_close
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_keithley2400_doutput_digital_output_function_list =
{
	NULL,
	mxd_keithley2400_doutput_write
};

/* Keithley 2400 digital output data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_keithley2400_doutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_KEITHLEY2400_DOUTPUT_STANDARD_FIELDS
};

long mxd_keithley2400_doutput_num_record_fields
		= sizeof( mxd_keithley2400_doutput_rf_defaults )
		  / sizeof( mxd_keithley2400_doutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keithley2400_doutput_rfield_def_ptr
			= &mxd_keithley2400_doutput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_keithley2400_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
				MX_KEITHLEY2400_DOUTPUT **keithley2400_doutput,
				MX_KEITHLEY2400 **keithley2400,
				MX_INTERFACE **interface,
				const char *calling_fname )
{
	static const char fname[] = "mxd_keithley2400_doutput_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_KEITHLEY2400_DOUTPUT *keithley2400_doutput_ptr;
	MX_KEITHLEY2400 *keithley2400_ptr;

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

	keithley2400_doutput_ptr = (MX_KEITHLEY2400_DOUTPUT *)
					record->record_type_struct;

	if ( keithley2400_doutput_ptr == (MX_KEITHLEY2400_DOUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2400_DOUTPUT pointer for doutput '%s' is NULL.",
			record->name );
	}

	if ( keithley2400_doutput != (MX_KEITHLEY2400_DOUTPUT **) NULL ) {
		*keithley2400_doutput = keithley2400_doutput_ptr;
	}

	controller_record = keithley2400_doutput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for doutput '%s' is NULL.",
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
mxd_keithley2400_doutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_keithley2400_doutput_create_record_structures()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_KEITHLEY2400_DOUTPUT *keithley2400_doutput;

	/* Allocate memory for the necessary structures. */

	doutput = (MX_DIGITAL_OUTPUT *) malloc( sizeof(MX_DIGITAL_OUTPUT) );

	if ( doutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
	}

	keithley2400_doutput = (MX_KEITHLEY2400_DOUTPUT *)
				malloc( sizeof(MX_KEITHLEY2400_DOUTPUT) );

	if ( keithley2400_doutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY2400_DOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = doutput;
	record->record_type_struct = keithley2400_doutput;
	record->class_specific_function_list
		= &mxd_keithley2400_doutput_digital_output_function_list;

	doutput->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2400_doutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2400_doutput_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_KEITHLEY2400_DOUTPUT *keithley2400_doutput;
	MX_KEITHLEY2400 *keithley2400;
	MX_INTERFACE *interface;
	long gpib_address;
	mx_status_type mx_status;

	doutput = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2400_doutput_get_pointers( doutput,
		&keithley2400_doutput, &keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( interface->record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by Keithley 2400 doutput '%s' is not "
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
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2400_doutput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2400_doutput_close()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_KEITHLEY2400_DOUTPUT *keithley2400_doutput;
	MX_KEITHLEY2400 *keithley2400;
	MX_INTERFACE *interface;
	mx_status_type mx_status;

	doutput = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2400_doutput_get_pointers( doutput,
		&keithley2400_doutput, &keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley2400_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_keithley2400_doutput_write()";

	MX_KEITHLEY2400_DOUTPUT *keithley2400_doutput;
	MX_KEITHLEY2400 *keithley2400;
	MX_INTERFACE *interface;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_keithley2400_doutput_get_pointers( doutput,
		&keithley2400_doutput, &keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "SOUR2:TTL %lu", doutput->value );

	mx_status = mxi_keithley_command( doutput->record, interface, command,
					NULL, 0, KEITHLEY2400_DOUTPUT_DEBUG );

	return mx_status;
}


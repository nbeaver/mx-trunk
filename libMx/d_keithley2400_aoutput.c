/*
 * Name:    d_keithley2400_aoutput.c
 *
 * Purpose: MX driver to control Keithley 2400 series analog outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY2400_AOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "i_keithley.h"
#include "i_keithley2400.h"
#include "d_keithley2400_aoutput.h"

/* Initialize the aoutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_keithley2400_aoutput_record_function_list = {
	NULL,
	mxd_keithley2400_aoutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_keithley2400_aoutput_open,
	mxd_keithley2400_aoutput_close
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_keithley2400_aoutput_analog_output_function_list =
{
	NULL,
	mxd_keithley2400_aoutput_write
};

/* Keithley 2400 analog output data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_keithley2400_aoutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_KEITHLEY2400_AOUTPUT_STANDARD_FIELDS
};

long mxd_keithley2400_aoutput_num_record_fields
		= sizeof( mxd_keithley2400_aoutput_rf_defaults )
		  / sizeof( mxd_keithley2400_aoutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keithley2400_aoutput_rfield_def_ptr
			= &mxd_keithley2400_aoutput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_keithley2400_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
				MX_KEITHLEY2400_AOUTPUT **keithley2400_aoutput,
				MX_KEITHLEY2400 **keithley2400,
				MX_INTERFACE **interface,
				const char *calling_fname )
{
	static const char fname[] = "mxd_keithley2400_aoutput_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_KEITHLEY2400_AOUTPUT *keithley2400_aoutput_ptr;
	MX_KEITHLEY2400 *keithley2400_ptr;

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

	keithley2400_aoutput_ptr = (MX_KEITHLEY2400_AOUTPUT *)
					record->record_type_struct;

	if ( keithley2400_aoutput_ptr == (MX_KEITHLEY2400_AOUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2400_AOUTPUT pointer for aoutput '%s' is NULL.",
			record->name );
	}

	if ( keithley2400_aoutput != (MX_KEITHLEY2400_AOUTPUT **) NULL ) {
		*keithley2400_aoutput = keithley2400_aoutput_ptr;
	}

	controller_record = keithley2400_aoutput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for aoutput '%s' is NULL.",
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
mxd_keithley2400_aoutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_keithley2400_aoutput_create_record_structures()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_KEITHLEY2400_AOUTPUT *keithley2400_aoutput;

	/* Allocate memory for the necessary structures. */

	aoutput = (MX_ANALOG_OUTPUT *) malloc( sizeof(MX_ANALOG_OUTPUT) );

	if ( aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_OUTPUT structure." );
	}

	keithley2400_aoutput = (MX_KEITHLEY2400_AOUTPUT *)
				malloc( sizeof(MX_KEITHLEY2400_AOUTPUT) );

	if ( keithley2400_aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY2400_AOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = aoutput;
	record->record_type_struct = keithley2400_aoutput;
	record->class_specific_function_list
			= &mxd_keithley2400_aoutput_analog_output_function_list;

	aoutput->record = record;

	aoutput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2400_aoutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2400_aoutput_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_KEITHLEY2400_AOUTPUT *keithley2400_aoutput;
	MX_KEITHLEY2400 *keithley2400;
	MX_INTERFACE *interface;
	long gpib_address;
	mx_status_type mx_status;

	aoutput = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2400_aoutput_get_pointers( aoutput,
		&keithley2400_aoutput, &keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( interface->record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by Keithley 2400 aoutput '%s' is not "
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

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2400_aoutput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2400_aoutput_close()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_KEITHLEY2400_AOUTPUT *keithley2400_aoutput;
	MX_KEITHLEY2400 *keithley2400;
	MX_INTERFACE *interface;
	mx_status_type mx_status;

	aoutput = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2400_aoutput_get_pointers( aoutput,
		&keithley2400_aoutput, &keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley2400_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_keithley2400_aoutput_write()";

	MX_KEITHLEY2400_AOUTPUT *keithley2400_aoutput;
	MX_KEITHLEY2400 *keithley2400;
	MX_INTERFACE *interface;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_keithley2400_aoutput_get_pointers( aoutput,
		&keithley2400_aoutput, &keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( keithley2400_aoutput->source_type ) {
	case MXT_KEITHLEY2400_VOLT:
		snprintf( command, sizeof(command),
			":SOUR:VOLT:LEV %g", aoutput->raw_value.double_value );
		break;
	case MXT_KEITHLEY2400_CURR:
		snprintf( command, sizeof(command),
			":SOUR:CURR:LEV %g", aoutput->raw_value.double_value );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported source type %ld for Keithley 2400 "
		"analog output '%s' used by record '%s'.",
			keithley2400_aoutput->source_type,
			keithley2400->record->name,
			aoutput->record->name );
	}

	mx_status = mxi_keithley_command( aoutput->record, interface, command,
					NULL, 0, KEITHLEY2400_AOUTPUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	keithley2400->last_source_type = keithley2400_aoutput->source_type;

	return MX_SUCCESSFUL_RESULT;
}


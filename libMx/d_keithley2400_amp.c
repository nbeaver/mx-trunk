/*
 * Name:    d_keithley2400_amp.c
 *
 * Purpose: MX amplifier driver for controlling the range of a 
 *          Keithley 2400 multimeter.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_KEITHLEY2400_AMP_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_amplifier.h"

#include "i_keithley.h"
#include "i_keithley2400.h"
#include "d_keithley2400_amp.h"

MX_RECORD_FUNCTION_LIST mxd_keithley2400_amp_record_function_list = {
	NULL,
	mxd_keithley2400_amp_create_record_structures
};

MX_AMPLIFIER_FUNCTION_LIST mxd_keithley2400_amp_amplifier_function_list = {
	mxd_keithley2400_amp_get_gain,
	mxd_keithley2400_amp_set_gain
};

MX_RECORD_FIELD_DEFAULTS mxd_keithley2400_amp_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_KEITHLEY2400_AMP_STANDARD_FIELDS
};

long mxd_keithley2400_amp_num_record_fields
		= sizeof( mxd_keithley2400_amp_record_field_defaults )
		    / sizeof( mxd_keithley2400_amp_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keithley2400_amp_rfield_def_ptr
			= &mxd_keithley2400_amp_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_keithley2400_amp_get_pointers( MX_AMPLIFIER *amplifier,
				MX_KEITHLEY2400_AMP **keithley2400_amp,
				MX_KEITHLEY2400 **keithley2400,
				MX_INTERFACE **interface,
				const char *calling_fname )
{
	static const char fname[] = "mxd_keithley2400_amp_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_KEITHLEY2400_AMP *keithley2400_amp_ptr;
	MX_KEITHLEY2400 *keithley2400_ptr;

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = amplifier->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_AMPLIFIER pointer passed was NULL." );
	}

	keithley2400_amp_ptr = (MX_KEITHLEY2400_AMP *)
					record->record_type_struct;

	if ( keithley2400_amp_ptr == (MX_KEITHLEY2400_AMP *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2400_AMP pointer for amplifier '%s' is NULL.",
			record->name );
	}

	if ( keithley2400_amp != (MX_KEITHLEY2400_AMP **) NULL ) {
		*keithley2400_amp = keithley2400_amp_ptr;
	}

	controller_record = keithley2400_amp_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for amplifier '%s' is NULL.",
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

/* ====== */

MX_EXPORT mx_status_type
mxd_keithley2400_amp_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_keithley2400_amp_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_KEITHLEY2400_AMP *keithley2400_amp;

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AMPLIFIER structure.");
	}

	keithley2400_amp = (MX_KEITHLEY2400_AMP *)
				malloc( sizeof(MX_KEITHLEY2400_AMP) );

	if ( keithley2400_amp == (MX_KEITHLEY2400_AMP *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_KEITHLEY2400_AMP structure." );
	}

	record->record_class_struct = amplifier;
	record->record_type_struct = keithley2400_amp;

	record->class_specific_function_list
			= &mxd_keithley2400_amp_amplifier_function_list;

	amplifier->record = record;
	keithley2400_amp->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2400_amp_get_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_keithley2400_amp_get_gain()";

	MX_KEITHLEY2400_AMP *keithley2400_amp;
	MX_KEITHLEY2400 *keithley2400;
	MX_INTERFACE *interface;
	char command[80];
	char response[80];
	int measurement_type, num_items;
	double gain_value;
	mx_status_type mx_status;

	mx_status = mxd_keithley2400_amp_get_pointers( amplifier,
		&keithley2400_amp, &keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( keithley2400_amp->io_type ) {
	case MXT_KEITHLEY2400_AMP_INPUT:
		mx_status = mxi_keithley2400_get_measurement_type(
					keithley2400, interface,
					&measurement_type, 
					MXD_KEITHLEY2400_AMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( keithley2400->last_measurement_type ) {
		case MXT_KEITHLEY2400_VOLT:
			snprintf( command, sizeof(command), ":SENS:VOLT:RANG?");
			break;
		case MXT_KEITHLEY2400_CURR:
			snprintf( command, sizeof(command), ":SENS:CURR:RANG?");
			break;
		case MXT_KEITHLEY2400_RES:
			snprintf( command, sizeof(command), ":SENS:RES:RANG?" );
			break;
		}
		break;

	case MXT_KEITHLEY2400_AMP_OUTPUT:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Getting the output gain is not yet implemented." );

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"io_type %ld for amplifier '%s' is not supported.  "
		"Only 1 for input and 2 for output are supported.",
			keithley2400_amp->io_type,
			amplifier->record->name );
	}

	mx_status = mxi_keithley_command( keithley2400->record,interface,
					command, response, sizeof(response),
					MXD_KEITHLEY2400_AMP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lg", &gain_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Received an unparseable response '%s' to command '%s' "
		"from Keithley 2400 '%s'.",
			response, command, keithley2400->record->name );
	}

	amplifier->gain = gain_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2400_amp_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_keithley2400_amp_set_gain()";

	MX_KEITHLEY2400_AMP *keithley2400_amp;
	MX_KEITHLEY2400 *keithley2400;
	MX_INTERFACE *interface;
	char command[80];
	int measurement_type;
	mx_status_type mx_status;

	mx_status = mxd_keithley2400_amp_get_pointers( amplifier,
		&keithley2400_amp, &keithley2400, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( keithley2400_amp->io_type ) {
	case MXT_KEITHLEY2400_AMP_INPUT:
		mx_status = mxi_keithley2400_get_measurement_type(
					keithley2400, interface,
					&measurement_type, 
					MXD_KEITHLEY2400_AMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( fabs( amplifier->gain ) < 1.0e-12 ) {

			/* Turn on autoscaling. */

			switch( keithley2400->last_measurement_type ) {
			case MXT_KEITHLEY2400_VOLT:
				snprintf( command, sizeof(command),
						":SENS:VOLT:RANG:AUTO ON" );
				break;
			case MXT_KEITHLEY2400_CURR:
				snprintf( command, sizeof(command),
						":SENS:CURR:RANG:AUTO ON" );
				break;
			case MXT_KEITHLEY2400_RES:
				snprintf( command, sizeof(command),
						":SENS:RES:RANG:AUTO ON" );
				break;
			}
		} else {
			/* Turn off autoscaling. */

			switch( keithley2400->last_measurement_type ) {
			case MXT_KEITHLEY2400_VOLT:
				snprintf( command, sizeof(command),
					":SENS:VOLT:RANG %g", amplifier->gain );
				break;
			case MXT_KEITHLEY2400_CURR:
				snprintf( command, sizeof(command),
					":SENS:CURR:RANG %g", amplifier->gain );
				break;
			case MXT_KEITHLEY2400_RES:
				snprintf( command, sizeof(command),
					":SENS:RES:RANG %g", amplifier->gain );
				break;
			}
		}
		break;

	case MXT_KEITHLEY2400_AMP_OUTPUT:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Setting the output gain is not yet implemented." );

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"io_type %ld for amplifier '%s' is not supported.  "
		"Only 1 for input and 2 for output are supported.",
			keithley2400_amp->io_type,
			amplifier->record->name );
	}

	mx_status = mxi_keithley_command( keithley2400->record,interface,
						command, NULL, 0,
						MXD_KEITHLEY2400_AMP_DEBUG );

	return mx_status;
}


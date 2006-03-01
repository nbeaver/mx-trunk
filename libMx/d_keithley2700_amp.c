/*
 * Name:    d_keithley2700_amp.c
 *
 * Purpose: MX driver to control Keithley 2700 measurement ranges.
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

#define KEITHLEY2700_AMP_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_amplifier.h"
#include "i_keithley.h"
#include "i_keithley2700.h"
#include "d_keithley2700_amp.h"

/* Initialize the amplifier driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_keithley2700_amp_record_function_list = {
	NULL,
	mxd_keithley2700_amp_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_keithley2700_amp_open,
	mxd_keithley2700_amp_close
};

MX_AMPLIFIER_FUNCTION_LIST mxd_keithley2700_amp_amplifier_function_list = {
	mxd_keithley2700_amp_get_gain,
	mxd_keithley2700_amp_set_gain,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_amplifier_default_get_parameter_handler,
	mx_amplifier_default_set_parameter_handler
};

/* Keithley 428 amplifier data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_keithley2700_amp_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_KEITHLEY2700_AMP_STANDARD_FIELDS
};

long mxd_keithley2700_amp_num_record_fields
		= sizeof( mxd_keithley2700_amp_rf_defaults )
		  / sizeof( mxd_keithley2700_amp_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keithley2700_amp_rfield_def_ptr
			= &mxd_keithley2700_amp_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_keithley2700_amp_get_pointers( MX_AMPLIFIER *amplifier,
				MX_KEITHLEY2700_AMP **keithley2700_amp,
				MX_KEITHLEY2700 **keithley2700,
				MX_INTERFACE **interface,
				const char *calling_fname )
{
	static const char fname[] = "mxd_keithley2700_amp_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_KEITHLEY2700_AMP *keithley2700_amp_ptr;
	MX_KEITHLEY2700 *keithley2700_ptr;

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

	keithley2700_amp_ptr = (MX_KEITHLEY2700_AMP *)
					record->record_type_struct;

	if ( keithley2700_amp_ptr == (MX_KEITHLEY2700_AMP *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2700_AMP pointer for amplifier '%s' is NULL.",
			record->name );
	}

	if ( keithley2700_amp != (MX_KEITHLEY2700_AMP **) NULL ) {
		*keithley2700_amp = keithley2700_amp_ptr;
	}

	controller_record = keithley2700_amp_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for amplifier '%s' is NULL.",
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
			amplifier->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_keithley2700_amp_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_keithley2700_amp_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_KEITHLEY2700_AMP *keithley2700_amp;

	/* Allocate memory for the necessary structures. */

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AMPLIFIER structure." );
	}

	keithley2700_amp = (MX_KEITHLEY2700_AMP *)
				malloc( sizeof(MX_KEITHLEY2700_AMP) );

	if ( keithley2700_amp == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY2700_AMP structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = amplifier;
	record->record_type_struct = keithley2700_amp;
	record->class_specific_function_list
			= &mxd_keithley2700_amp_amplifier_function_list;

	amplifier->record = record;

	amplifier->gain_range[0] = 1.0;
	amplifier->gain_range[1] = 1.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2700_amp_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2700_amp_open()";

	MX_AMPLIFIER *amplifier;
	MX_KEITHLEY2700_AMP *keithley2700_amp;
	MX_KEITHLEY2700 *keithley2700;
	MX_INTERFACE *interface;
	long gpib_address;
	mx_status_type mx_status;

	amplifier = (MX_AMPLIFIER *) record->record_class_struct;

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_AMPLIFIER pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2700_amp_get_pointers( amplifier,
			&keithley2700_amp, &keithley2700, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( interface->record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by Keithley 2700 amplifier '%s' is not "
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

	keithley2700_amp->slot =
		mxi_keithley2700_slot( keithley2700_amp->system_channel );

	keithley2700_amp->channel =
		mxi_keithley2700_channel( keithley2700_amp->system_channel );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2700_amp_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2700_amp_close()";

	MX_AMPLIFIER *amplifier;
	MX_KEITHLEY2700_AMP *keithley2700_amp;
	MX_KEITHLEY2700 *keithley2700;
	MX_INTERFACE *interface;
	mx_status_type mx_status;

	amplifier = (MX_AMPLIFIER *) (record->record_class_struct);

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_AMPLIFIER pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2700_amp_get_pointers( amplifier,
			&keithley2700_amp, &keithley2700, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley2700_amp_get_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_keithley2700_amp_get_gain()";

	MX_KEITHLEY2700_AMP *keithley2700_amp;
	MX_KEITHLEY2700 *keithley2700;
	MX_INTERFACE *interface;
	char command[80];
	char response[80];
	int num_items, fast_mode;
	long slot, channel, channel_type;
	mx_status_type mx_status;

	mx_status = mxd_keithley2700_amp_get_pointers( amplifier,
			&keithley2700_amp, &keithley2700, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Skip communicating with the Keithley if the database
	 * is in fast mode.
	 */

	mx_status = mx_get_fast_mode( amplifier->record, &fast_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fast_mode ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the Keithley range. */

	slot = keithley2700_amp->slot;
	channel = keithley2700_amp->channel;

	channel_type = keithley2700->channel_type[slot-1][channel-1];

	switch( channel_type ) {
	case MXT_KEITHLEY2700_DCV:
		sprintf( command, "SENSE:VOLT:DC:RANG? (@%ld%02ld)",
						slot, channel );
		break;
	default:
		mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal channel type %d for Keithley 2700 amplifier '%s'.",
			channel_type, amplifier->record->name );
	}

	mx_status = mxi_keithley_command( amplifier->record, interface, command,
						response, sizeof(response),
						KEITHLEY2700_AMP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lg", &(amplifier->gain) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cannot parse response to command '%s' for record '%s'.  "
		"response = '%s'",
			command, amplifier->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2700_amp_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_keithley2700_amp_set_gain()";

	MX_KEITHLEY2700_AMP *keithley2700_amp;
	MX_KEITHLEY2700 *keithley2700;
	MX_INTERFACE *interface;
	char command[80];
	long slot, channel, channel_type;
	mx_status_type mx_status;

	mx_status = mxd_keithley2700_amp_get_pointers( amplifier,
			&keithley2700_amp, &keithley2700, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Keithley 2700 does not require you to round off values
	 * before sending them, so we just send it the raw value requested
	 * by the user.
	 */

	slot = keithley2700_amp->slot;
	channel = keithley2700_amp->channel;

	channel_type = keithley2700->channel_type[slot-1][channel-1];

	switch( channel_type ) {
	case MXT_KEITHLEY2700_DCV:
		sprintf( command, "SENSE:VOLT:DC:RANG %g, (@%ld%02ld)",
				amplifier->gain, slot, channel );
		break;
	case MXT_KEITHLEY2700_ACV:
		sprintf( command, "SENSE:VOLT:AC:RANG %g, (@%ld%02ld)",
				amplifier->gain, slot, channel );
		break;
	case MXT_KEITHLEY2700_DCI:
		sprintf( command, "SENSE:CURR:DC:RANG %g, (@%ld%02ld)",
				amplifier->gain, slot, channel );
		break;
	case MXT_KEITHLEY2700_ACI:
		sprintf( command, "SENSE:CURR:AC:RANG %g, (@%ld%02ld)",
				amplifier->gain, slot, channel );
		break;
	case MXT_KEITHLEY2700_OHMS_2:
		sprintf( command, "SENSE:RES:RANG %g, (@%ld%02ld)",
				amplifier->gain, slot, channel );
		break;
	case MXT_KEITHLEY2700_OHMS_4:
		sprintf( command, "SENSE:FRES:RANG %g, (@%ld%02ld)",
				amplifier->gain, slot, channel );
		break;
	default:
		mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal channel type %ld for Keithley 2700 amplifier '%s'.",
			channel_type, amplifier->record->name );
	}

	mx_status = mxi_keithley_command( amplifier->record, interface, command,
					NULL, 0, KEITHLEY2700_AMP_DEBUG );

	return mx_status;
}


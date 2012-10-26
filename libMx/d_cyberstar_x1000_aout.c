/*
 * Name:    d_cyberstar_x1000_aout.c
 *
 * Purpose: MX analog output driver to control the Oxford Danfysik
 *          Cyberstar X1000 high voltage and delay.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_CYBERSTAR_X1000_AOUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_sca.h"
#include "mx_analog_output.h"
#include "d_cyberstar_x1000.h"
#include "d_cyberstar_x1000_aout.h"

/* Initialize the CYBERSTAR_X1000 analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_cyberstar_x1000_aout_record_function_list = {
	NULL,
	mxd_cyberstar_x1000_aout_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
    mxd_cyberstar_x1000_aout_analog_output_function_list = {
	mxd_cyberstar_x1000_aout_read,
	mxd_cyberstar_x1000_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_cyberstar_x1000_aout_recfield_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_CYBERSTAR_X1000_AOUTPUT_STANDARD_FIELDS
};

long mxd_cyberstar_x1000_aout_num_record_fields
		= sizeof( mxd_cyberstar_x1000_aout_recfield_defaults )
		/ sizeof( mxd_cyberstar_x1000_aout_recfield_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_cyberstar_x1000_aout_rfield_def_ptr
			= &mxd_cyberstar_x1000_aout_recfield_defaults[0];

static mx_status_type
mxd_cyberstar_x1000_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_CYBERSTAR_X1000_AOUTPUT **cyberstar_x1000_aoutput,
			MX_CYBERSTAR_X1000 **cyberstar_x1000,
			const char *calling_fname )
{
	static const char fname[] = "mxd_cyberstar_x1000_aout_get_pointers()";

	MX_RECORD *cyberstar_x1000_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( cyberstar_x1000_aoutput == (MX_CYBERSTAR_X1000_AOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_CYBERSTAR_X1000_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( cyberstar_x1000 == (MX_CYBERSTAR_X1000 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_CYBERSTAR_X1000 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*cyberstar_x1000_aoutput = (MX_CYBERSTAR_X1000_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *cyberstar_x1000_aoutput == (MX_CYBERSTAR_X1000_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_CYBERSTAR_X1000_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	cyberstar_x1000_record =
		(*cyberstar_x1000_aoutput)->cyberstar_x1000_record;

	if ( cyberstar_x1000_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CYBERSTAR_X1000 pointer for CYBERSTAR_X1000 analog input "
		"record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( cyberstar_x1000_record->mx_type != MXT_SCA_CYBERSTAR_X1000 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"cyberstar_x1000_record '%s' for Cyberstar X1000 analog input '%s' "
	"is not a Cyberstar X1000 record.  Instead, it is a '%s' record.",
			cyberstar_x1000_record->name, aoutput->record->name,
			mx_get_driver_name( cyberstar_x1000_record ) );
	}

	*cyberstar_x1000 = (MX_CYBERSTAR_X1000 *)
				cyberstar_x1000_record->record_type_struct;

	if ( *cyberstar_x1000 == (MX_CYBERSTAR_X1000 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_CYBERSTAR_X1000 pointer for Cyberstar X1000 record '%s' used by "
"Cyberstar X1000 analog input record '%s' and passed by '%s' is NULL.",
			cyberstar_x1000_record->name,
			aoutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_cyberstar_x1000_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_cyberstar_x1000_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_CYBERSTAR_X1000_AOUTPUT *cyberstar_x1000_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *)
					malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        cyberstar_x1000_aoutput = (MX_CYBERSTAR_X1000_AOUTPUT *)
				malloc( sizeof(MX_CYBERSTAR_X1000_AOUTPUT) );

        if ( cyberstar_x1000_aoutput == (MX_CYBERSTAR_X1000_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_CYBERSTAR_X1000_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = cyberstar_x1000_aoutput;
        record->class_specific_function_list
		= &mxd_cyberstar_x1000_aout_analog_output_function_list;

        analog_output->record = record;
	cyberstar_x1000_aoutput->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cyberstar_x1000_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_cyberstar_x1000_aout_read()";

	MX_CYBERSTAR_X1000_AOUTPUT *cyberstar_x1000_aoutput;
	MX_CYBERSTAR_X1000 *cyberstar_x1000;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_cyberstar_x1000_aout_get_pointers( aoutput,
			&cyberstar_x1000_aoutput, &cyberstar_x1000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( cyberstar_x1000_aoutput->output_type
			== MXT_CYBERSTAR_X1000_HIGH_VOLTAGE )
	{
		snprintf( command, sizeof(command),
				":SOUR%ld:VOLT?", cyberstar_x1000->address );
	} else {
		snprintf( command, sizeof(command),
				":TRIG%ld:ECO?", cyberstar_x1000->address );
	}

	mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000, command,
				response, sizeof( response ),
				MXD_CYBERSTAR_X1000_AOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%lg", &(aoutput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Could not find a numerical value in the response to an '%s' "
		"command to Cyberstar X1000 controller '%s' for "
		"analog output record '%s'.  Response = '%s'",
			command,
			cyberstar_x1000->record->name,
			aoutput->record->name,
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cyberstar_x1000_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_cyberstar_x1000_aout_write()";

	MX_CYBERSTAR_X1000_AOUTPUT *cyberstar_x1000_aoutput;
	MX_CYBERSTAR_X1000 *cyberstar_x1000;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_cyberstar_x1000_aout_get_pointers( aoutput,
			&cyberstar_x1000_aoutput, &cyberstar_x1000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( cyberstar_x1000_aoutput->output_type
			== MXT_CYBERSTAR_X1000_HIGH_VOLTAGE )
	{
		snprintf( command, sizeof(command), ":SOUR%ld:VOLT %g",
					cyberstar_x1000->address,
					aoutput->raw_value.double_value );
	} else {
		snprintf( command, sizeof(command), ":TRIG%ld:ECO %g",
					cyberstar_x1000->address,
					aoutput->raw_value.double_value );
	}

	mx_status = mxd_cyberstar_x1000_command( cyberstar_x1000, command,
				NULL, 0, MXD_CYBERSTAR_X1000_AOUT_DEBUG );

	return mx_status;
}


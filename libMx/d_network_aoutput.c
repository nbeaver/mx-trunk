/*
 * Name:    d_network_aoutput.c
 *
 * Purpose: MX network analog output device driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2005, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_analog_output.h"
#include "d_network_aoutput.h"

MX_RECORD_FUNCTION_LIST mxd_network_aoutput_record_function_list = {
	NULL,
	mxd_network_aoutput_create_record_structures,
	mxd_network_aoutput_finish_record_initialization
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_network_aoutput_analog_output_function_list = {
	mxd_network_aoutput_read,
	mxd_network_aoutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_network_aoutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_NETWORK_AOUTPUT_STANDARD_FIELDS
};

long mxd_network_aoutput_num_record_fields
		= sizeof( mxd_network_aoutput_record_field_defaults )
		    / sizeof( mxd_network_aoutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_aoutput_rfield_def_ptr
			= &mxd_network_aoutput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_network_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_NETWORK_AOUTPUT **network_aoutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_aoutput_get_pointers()";

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( network_aoutput == (MX_NETWORK_AOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_AOUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*network_aoutput = (MX_NETWORK_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *network_aoutput == (MX_NETWORK_AOUTPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_AOUTPUT pointer for aoutput record '%s' passed by '%s' is NULL",
				aoutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */


MX_EXPORT mx_status_type
mxd_network_aoutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_network_aoutput_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_NETWORK_AOUTPUT *network_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        network_aoutput = (MX_NETWORK_AOUTPUT *)
				malloc( sizeof(MX_NETWORK_AOUTPUT) );

        if ( network_aoutput == (MX_NETWORK_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_NETWORK_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = network_aoutput;
        record->class_specific_function_list
			= &mxd_network_aoutput_analog_output_function_list;

        analog_output->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_aoutput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_aoutput_finish_record_initialization()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_NETWORK_AOUTPUT *network_aoutput;
	char *name;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	network_aoutput = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_network_aoutput_get_pointers(
				aoutput, &network_aoutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	name = network_aoutput->remote_record_field_name;

	if ( strchr( name, '.' ) == NULL ) {
		mx_network_field_init( &(network_aoutput->value_nf),
			network_aoutput->server_record,
			"%s.value", network_aoutput->remote_record_field_name );
	} else {
		mx_network_field_init( &(network_aoutput->value_nf),
			network_aoutput->server_record,
			network_aoutput->remote_record_field_name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_aoutput_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_network_aoutput_read()";

	MX_NETWORK_AOUTPUT *network_aoutput;
	double raw_value;
	mx_status_type mx_status;

	network_aoutput = (MX_NETWORK_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( network_aoutput == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_AOUTPUT pointer for analog output '%s' is NULL",
			aoutput->record->name );
	}

	mx_status = mx_get( &(network_aoutput->value_nf),
				MXFT_DOUBLE, &raw_value );

	aoutput->raw_value.double_value = (double) raw_value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_network_aoutput_read()";

	MX_NETWORK_AOUTPUT *network_aoutput;
	double raw_value;
	mx_status_type mx_status;

	network_aoutput = (MX_NETWORK_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( network_aoutput == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_AOUTPUT pointer for analog output '%s' is NULL",
			aoutput->record->name );
	}

	raw_value = aoutput->raw_value.double_value;

	mx_status = mx_put( &(network_aoutput->value_nf),
				MXFT_DOUBLE, &raw_value );

	return mx_status;
}


/*
 * Name:    d_network_dinput.c
 *
 * Purpose: MX network digital input device driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2004, 2006-2007, 2011
 *   Illinois Institute of Technology
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
#include "mx_digital_input.h"
#include "d_network_dinput.h"

MX_RECORD_FUNCTION_LIST mxd_network_dinput_record_function_list = {
	NULL,
	mxd_network_dinput_create_record_structures,
	mxd_network_dinput_finish_record_initialization
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_network_dinput_digital_input_function_list = {
	mxd_network_dinput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_network_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_NETWORK_DINPUT_STANDARD_FIELDS
};

long mxd_network_dinput_num_record_fields
		= sizeof( mxd_network_dinput_record_field_defaults )
			/ sizeof( mxd_network_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_dinput_rfield_def_ptr
			= &mxd_network_dinput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_network_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_NETWORK_DINPUT **network_dinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_dinput_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_DIGITAL_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( network_dinput == (MX_NETWORK_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_NETWORK_DINPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*network_dinput = (MX_NETWORK_DINPUT *)
				dinput->record->record_type_struct;

	if ( *network_dinput == (MX_NETWORK_DINPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_DINPUT pointer for dinput record '%s' passed by '%s' is NULL",
				dinput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_network_dinput_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_network_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_NETWORK_DINPUT *network_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        network_dinput = (MX_NETWORK_DINPUT *)
				malloc( sizeof(MX_NETWORK_DINPUT) );

        if ( network_dinput == (MX_NETWORK_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_NETWORK_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = network_dinput;
        record->class_specific_function_list
			= &mxd_network_dinput_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_dinput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_dinput_finish_record_initialization()";

	MX_DIGITAL_INPUT *dinput;
	MX_NETWORK_DINPUT *network_dinput;
	char *name;
	mx_status_type mx_status;

	network_dinput = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_network_dinput_get_pointers(
				dinput, &network_dinput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	name = network_dinput->remote_record_field_name;

	if ( strchr( name, '.' ) == NULL ) {
		mx_network_field_init( &(network_dinput->value_nf),
			network_dinput->server_record,
			"%s.value", network_dinput->remote_record_field_name );
	} else {
		mx_network_field_init( &(network_dinput->value_nf),
			network_dinput->server_record,
			network_dinput->remote_record_field_name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_network_dinput_read()";

	MX_NETWORK_DINPUT *network_dinput;
	unsigned long value;
	mx_status_type mx_status;

	network_dinput = NULL;

	mx_status = mxd_network_dinput_get_pointers(
				dinput, &network_dinput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_dinput->value_nf), MXFT_HEX, &value );

	dinput->value = value;

	return mx_status;
}


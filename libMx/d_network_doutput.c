/*
 * Name:    d_network_doutput.c
 *
 * Purpose: MX network digital output device driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2004 Illinois Institute of Technology
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
#include "mx_digital_output.h"
#include "d_network_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_network_doutput_record_function_list = {
	NULL,
	mxd_network_doutput_create_record_structures,
	mxd_network_doutput_finish_record_initialization
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_network_doutput_digital_output_function_list = {
	mxd_network_doutput_read,
	mxd_network_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_network_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_NETWORK_DOUTPUT_STANDARD_FIELDS
};

mx_length_type mxd_network_doutput_num_record_fields
		= sizeof( mxd_network_doutput_record_field_defaults )
			/ sizeof( mxd_network_doutput_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_network_doutput_rfield_def_ptr
			= &mxd_network_doutput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_network_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_NETWORK_DOUTPUT **network_doutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_doutput_get_pointers()";

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( network_doutput == (MX_NETWORK_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_DOUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*network_doutput = (MX_NETWORK_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *network_doutput == (MX_NETWORK_DOUTPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_DOUTPUT pointer for doutput record '%s' passed by '%s' is NULL",
				doutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_network_doutput_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_network_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_NETWORK_DOUTPUT *network_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
				malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        network_doutput = (MX_NETWORK_DOUTPUT *)
				malloc( sizeof(MX_NETWORK_DOUTPUT) );

        if ( network_doutput == (MX_NETWORK_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_NETWORK_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = network_doutput;
        record->class_specific_function_list
			= &mxd_network_doutput_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_doutput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_doutput_finish_record_initialization()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_NETWORK_DOUTPUT *network_doutput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_network_doutput_get_pointers(
				doutput, &network_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_doutput->value_nf),
		network_doutput->server_record,
		"%s.value", network_doutput->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_doutput_read( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_network_doutput_read()";

	MX_NETWORK_DOUTPUT *network_doutput;
	mx_hex_type value;
	mx_status_type mx_status;

	mx_status = mxd_network_doutput_get_pointers(
				doutput, &network_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_doutput->value_nf), MXFT_HEX, &value );

	doutput->value = value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_network_doutput_write()";

	MX_NETWORK_DOUTPUT *network_doutput;
	mx_hex_type value;
	mx_status_type mx_status;

	mx_status = mxd_network_doutput_get_pointers(
				doutput, &network_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value = doutput->value;

	mx_status = mx_put( &(network_doutput->value_nf), MXFT_HEX, &value );

	return mx_status;
}


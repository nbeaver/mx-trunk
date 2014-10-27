/*
 * Name:    d_network_ainput.c
 *
 * Purpose: MX network analog input device driver.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2004, 2007, 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_analog_input.h"
#include "d_network_ainput.h"

MX_RECORD_FUNCTION_LIST mxd_network_ainput_record_function_list = {
	NULL,
	mxd_network_ainput_create_record_structures,
	mxd_network_ainput_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_network_ainput_analog_input_function_list = {
	mxd_network_ainput_read,
	mxd_network_ainput_get_dark_current,
	mxd_network_ainput_set_dark_current
};

MX_RECORD_FIELD_DEFAULTS mxd_network_ainput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_NETWORK_AINPUT_STANDARD_FIELDS
};

long mxd_network_ainput_num_record_fields
		= sizeof( mxd_network_ainput_record_field_defaults )
			/ sizeof( mxd_network_ainput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_ainput_rfield_def_ptr
			= &mxd_network_ainput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_network_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_NETWORK_AINPUT **network_ainput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_ainput_get_pointers()";

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ANALOG_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( network_ainput == (MX_NETWORK_AINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_NETWORK_AINPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*network_ainput = (MX_NETWORK_AINPUT *)
				ainput->record->record_type_struct;

	if ( *network_ainput == (MX_NETWORK_AINPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_AINPUT pointer for ainput record '%s' passed by '%s' is NULL",
				ainput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_network_ainput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_network_ainput_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_NETWORK_AINPUT *network_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        network_ainput = (MX_NETWORK_AINPUT *)
				malloc( sizeof(MX_NETWORK_AINPUT) );

        if ( network_ainput == (MX_NETWORK_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_NETWORK_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = network_ainput;
        record->class_specific_function_list
			= &mxd_network_ainput_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_ainput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_ainput_finish_record_initialization()";

	MX_ANALOG_INPUT *ainput;
	MX_NETWORK_AINPUT *network_ainput;
	char *name;
	unsigned long flags;
	mx_status_type mx_status;

	network_ainput = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_network_ainput_get_pointers(
				ainput, &network_ainput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy(record->network_type_name, "mx", MXU_NETWORK_TYPE_NAME_LENGTH);

	mx_status = mx_analog_input_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	name = network_ainput->remote_record_field_name;

	if ( strchr( name, '.' ) == NULL ) {
		mx_network_field_init( &(network_ainput->value_nf),
			network_ainput->server_record, "%s.value",
				network_ainput->remote_record_field_name );

		mx_network_field_init( &(network_ainput->dark_current_nf),
			network_ainput->server_record, "%s.dark_current",
				network_ainput->remote_record_field_name );
	} else {
		mx_network_field_init( &(network_ainput->value_nf),
			network_ainput->server_record,
			network_ainput->remote_record_field_name );

		mx_network_field_init(
			&(network_ainput->dark_current_nf), NULL, "" );

		flags = ainput->analog_input_flags;

		if ( flags & MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT ) {

			/* If the 'server subtracts dark current' flag is set,
			 * turn the flag off and generate a warning.
			 */

			flags &= (~MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT);

			mx_warning(
		"Turned off the MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT flag "
		"for analog input '%s'.  Server dark current subtraction "
		"can only be supported if the remote record field name '%s' "
		"does _not_ include a period '.' character in the name.",
				record->name,
				network_ainput->remote_record_field_name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_network_ainput_read()";

	MX_NETWORK_AINPUT *network_ainput;
	double raw_value;
	mx_status_type mx_status;

	network_ainput = NULL;

	mx_status = mxd_network_ainput_get_pointers(
				ainput, &network_ainput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_ainput->value_nf),
				MXFT_DOUBLE, &raw_value );

	ainput->raw_value.double_value = raw_value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_ainput_get_dark_current( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_network_ainput_get_dark_current()";

	MX_NETWORK_AINPUT *network_ainput;
	double dark_current;
	mx_status_type mx_status;

	/* If the database is configured such that the server maintains
	 * the dark current rather than the client, then ask the server
	 * for the value.  Otherwise, just return our locally cached value.
	 */

	if (ainput->analog_input_flags & MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT)
	{
		mx_status = mxd_network_ainput_get_pointers(
				ainput, &network_ainput, fname);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get( &(network_ainput->dark_current_nf),
					MXFT_DOUBLE, &dark_current );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ainput->dark_current = dark_current;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_ainput_set_dark_current( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_network_ainput_set_dark_current()";

	MX_NETWORK_AINPUT *network_ainput;
	double dark_current;
	mx_status_type mx_status;

	/* If the database is configured such that the server maintains
	 * the dark current rather than the client, then send the value
	 * to the server.  Otherwise, just cache it locally.
	 */

	if (ainput->analog_input_flags & MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT)
	{
		mx_status = mxd_network_ainput_get_pointers(
				ainput, &network_ainput, fname);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dark_current = ainput->dark_current;

		mx_status = mx_put( &(network_ainput->dark_current_nf),
					MXFT_DOUBLE, &dark_current );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}


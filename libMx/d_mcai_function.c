/*
 * Name:    d_mcai_function.c
 *
 * Purpose: MX driver to compute a linear function of the values from a
 *          multichannel analog input record.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MCAI_FUNCTION_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_mcai.h"
#include "d_mcai_function.h"

MX_RECORD_FUNCTION_LIST mxd_mcai_function_record_function_list = {
	mxd_mcai_function_initialize_type,
	mxd_mcai_function_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_mcai_function_analog_input_function_list = {
	mxd_mcai_function_read
};

MX_RECORD_FIELD_DEFAULTS mxd_mcai_function_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_MCAI_FUNCTION_STANDARD_FIELDS
};

long mxd_mcai_function_num_record_fields
		= sizeof( mxd_mcai_function_record_field_defaults )
			/ sizeof( mxd_mcai_function_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mcai_function_rfield_def_ptr
			= &mxd_mcai_function_record_field_defaults[0];

static mx_status_type
mxd_mcai_function_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_MCAI_FUNCTION **mcai_function,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mcai_function_get_pointers()";

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (mcai_function == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCAI_FUNCTION pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mcai_function = (MX_MCAI_FUNCTION *)
				ainput->record->record_type_struct;

	if ( *mcai_function == (MX_MCAI_FUNCTION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCAI_FUNCTION pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcai_function_initialize_type( long type )
{
        static const char fname[] = "mxs_mcai_function_initialize_type()";

        MX_DRIVER *driver;
        MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
        MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
        MX_RECORD_FIELD_DEFAULTS *field;
        long num_record_fields;
	long referenced_field_index;
        long num_channels_varargs_cookie;
        mx_status_type mx_status;

        driver = mx_get_driver_by_type( type );

        if ( driver == (MX_DRIVER *) NULL ) {
                return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
                        "Record type %ld not found.", type );
        }

        record_field_defaults_ptr = driver->record_field_defaults_ptr;

        if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'record_field_defaults_ptr' for record type '%s' is NULL.",
                        driver->name );
        }

        record_field_defaults = *record_field_defaults_ptr;

        if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'record_field_defaults_ptr' for record type '%s' is NULL.",
                        driver->name );
        }

        if ( driver->num_record_fields == (long *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'num_record_fields' pointer for record type '%s' is NULL.",
                        driver->name );
        }

	num_record_fields = *(driver->num_record_fields);

        mx_status = mx_find_record_field_defaults_index(
                        record_field_defaults, num_record_fields,
                        "num_channels", &referenced_field_index );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        mx_status = mx_construct_varargs_cookie(
		referenced_field_index, 0, &num_channels_varargs_cookie );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        MX_DEBUG( 2,("%s: num_channels varargs cookie = %ld",
                        fname, num_channels_varargs_cookie));

	mx_status = mx_find_record_field_defaults(
		record_field_defaults, num_record_fields,
		"real_scale", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_channels_varargs_cookie;

	mx_status = mx_find_record_field_defaults(
		record_field_defaults, num_record_fields,
		"real_offset", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_channels_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcai_function_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_mcai_function_create_record_structures()";

        MX_ANALOG_INPUT *ainput;
        MX_MCAI_FUNCTION *mcai_function;

        /* Allocate memory for the necessary structures. */

        ainput = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        mcai_function = (MX_MCAI_FUNCTION *)
				malloc( sizeof(MX_MCAI_FUNCTION) );

        if ( mcai_function == (MX_MCAI_FUNCTION *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MCAI_FUNCTION structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = ainput;
        record->record_type_struct = mcai_function;
        record->class_specific_function_list
                                = &mxd_mcai_function_analog_input_function_list;

        ainput->record = record;
	mcai_function->record = record;

	/* Raw analog input values are stored as doubles. */

	ainput->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcai_function_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_mcai_function_read()";

	MX_MCAI_FUNCTION *mcai_function;
	MX_MCAI *mcai;
	double raw_value, channel_value;
	double *scale, *offset, *channel_array;
	long i;
	mx_status_type mx_status;

	mx_status = mxd_mcai_function_get_pointers( ainput,
						&mcai_function, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcai_function->mcai_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mcai_record pointer for record '%s' is NULL.",
			ainput->record->name );
	}

	mcai = (MX_MCAI *)
			mcai_function->mcai_record->record_class_struct;

	if ( mcai == (MX_MCAI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCAI pointer for record '%s' "
		"used by record '%s' is NULL.",
			mcai_function->mcai_record->name,
			ainput->record->name );
	}

	/* Read the multichannel analog input. */

	mx_status = mx_mcai_read( mcai_function->mcai_record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now compute the linear function from the values just read. */

	channel_array = mcai->channel_array;

	scale = mcai_function->real_scale;
	offset = mcai_function->real_offset;

	raw_value = 0.0;

	for ( i = 0; i < mcai_function->num_channels; i++ ) {
		channel_value = offset[i] + scale[i] * channel_array[i];

		raw_value += channel_value;
	}

	ainput->raw_value.double_value = raw_value;

	return mx_status;
}


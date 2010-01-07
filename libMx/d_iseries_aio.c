/*
 * Name:    d_iseries_aio.c
 *
 * Purpose: MX drivers to control iSeries functions as if they were
 *          analog I/O ports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ISERIES_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "i_iseries.h"
#include "d_iseries_aio.h"

/* Initialize the iSeries analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_iseries_ain_record_function_list = {
	NULL,
	mxd_iseries_ain_create_record_structures,
	mxd_iseries_ain_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_iseries_ain_analog_input_function_list = {
	mxd_iseries_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_iseries_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_ISERIES_AINPUT_STANDARD_FIELDS
};

long mxd_iseries_ain_num_record_fields
		= sizeof( mxd_iseries_ain_record_field_defaults )
			/ sizeof( mxd_iseries_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_iseries_ain_rfield_def_ptr
			= &mxd_iseries_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_iseries_aout_record_function_list = {
	NULL,
	mxd_iseries_aout_create_record_structures,
	mxd_iseries_aout_finish_record_initialization
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_iseries_aout_analog_output_function_list = {
	mxd_iseries_aout_read,
	mxd_iseries_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_iseries_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_ISERIES_AOUTPUT_STANDARD_FIELDS
};

long mxd_iseries_aout_num_record_fields
		= sizeof( mxd_iseries_aout_record_field_defaults )
			/ sizeof( mxd_iseries_aout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_iseries_aout_rfield_def_ptr
			= &mxd_iseries_aout_record_field_defaults[0];

static mx_status_type
mxd_iseries_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_ISERIES_AINPUT **iseries_ainput,
			MX_ISERIES **iseries,
			const char *calling_fname )
{
	static const char fname[] = "mxd_iseries_ain_get_pointers()";

	MX_RECORD *iseries_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (iseries_ainput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ISERIES_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (iseries == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ISERIES pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*iseries_ainput = (MX_ISERIES_AINPUT *)
				ainput->record->record_type_struct;

	if ( *iseries_ainput == (MX_ISERIES_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ISERIES_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	iseries_record = (*iseries_ainput)->iseries_record;

	if ( iseries_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ISERIES pointer for iSeries analog input "
		"record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( iseries_record->mx_type != MXI_GEN_ISERIES ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"iseries_record '%s' for iSeries analog input '%s' is "
		"not an 'iseries' record.  Instead, it is a '%s' record.",
			iseries_record->name, ainput->record->name,
			mx_get_driver_name( iseries_record ) );
	}

	*iseries = (MX_ISERIES *) iseries_record->record_type_struct;

	if ( *iseries == (MX_ISERIES *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ISERIES pointer for 'iseries' record '%s' used by "
	"iSeries analog input record '%s' and passed by '%s' is NULL.",
			iseries_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_iseries_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_ISERIES_AOUTPUT **iseries_aoutput,
			MX_ISERIES **iseries,
			const char *calling_fname )
{
	static const char fname[] = "mxd_iseries_aout_get_pointers()";

	MX_RECORD *iseries_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (iseries_aoutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ISERIES_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (iseries == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ISERIES pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*iseries_aoutput = (MX_ISERIES_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *iseries_aoutput == (MX_ISERIES_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ISERIES_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	iseries_record = (*iseries_aoutput)->iseries_record;

	if ( iseries_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ISERIES pointer for iSeries analog output "
		"record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( iseries_record->mx_type != MXI_GEN_ISERIES ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"iseries_record '%s' for iSeries analog output '%s' is "
		"not an 'iseries' record.  Instead, it is a '%s' record.",
			iseries_record->name, aoutput->record->name,
			mx_get_driver_name( iseries_record ) );
	}

	*iseries = (MX_ISERIES *) iseries_record->record_type_struct;

	if ( *iseries == (MX_ISERIES *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ISERIES pointer for 'iseries' record '%s' used by "
	"iSeries analog output record '%s' and passed by '%s' is NULL.",
			iseries_record->name,
			aoutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_iseries_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_iseries_ain_create_record_structures()";

        MX_ANALOG_INPUT *ainput;
        MX_ISERIES_AINPUT *iseries_ainput;

        /* Allocate memory for the necessary structures. */

        ainput = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        iseries_ainput = (MX_ISERIES_AINPUT *)
				malloc( sizeof(MX_ISERIES_AINPUT) );

        if ( iseries_ainput == (MX_ISERIES_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ISERIES_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = ainput;
        record->record_type_struct = iseries_ainput;
        record->class_specific_function_list
                                = &mxd_iseries_ain_analog_input_function_list;

        ainput->record = record;
	iseries_ainput->record = record;

	/* Raw analog input values are stored as doubles. */

	ainput->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_iseries_ain_finish_record_initialization( MX_RECORD *record )
{
	MX_ISERIES_AINPUT *iseries_ainput;
	char *ptr;
	mx_status_type mx_status;

	iseries_ainput = (MX_ISERIES_AINPUT *) record->record_type_struct;

	record->precision = (int) iseries_ainput->default_precision;

	iseries_ainput->command_prefix = iseries_ainput->command[0];

	ptr = &(iseries_ainput->command[1]);

	iseries_ainput->command_index = mx_hex_string_to_unsigned_long( ptr );

	mx_status = mx_analog_input_finish_record_initialization( record );

        return mx_status;
}

MX_EXPORT mx_status_type
mxd_iseries_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_iseries_ain_read()";

	MX_ISERIES_AINPUT *iseries_ainput;
	MX_ISERIES *iseries;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	iseries = NULL;
	iseries_ainput = NULL;

	mx_status = mxd_iseries_ain_get_pointers( ainput,
					&iseries_ainput, &iseries, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_iseries_command( iseries,
					iseries_ainput->command_prefix,
					iseries_ainput->command_index,
					iseries_ainput->num_command_bytes,
					0,
					&(ainput->raw_value.double_value),
					ainput->record->precision,
					MXD_ISERIES_AIO_DEBUG );
	return mx_status;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_iseries_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_iseries_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *aoutput;
        MX_ISERIES_AOUTPUT *iseries_aoutput;

        /* Allocate memory for the necessary structures. */

        aoutput = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        iseries_aoutput = (MX_ISERIES_AOUTPUT *)
				malloc( sizeof(MX_ISERIES_AOUTPUT) );

        if ( iseries_aoutput == (MX_ISERIES_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ISERIES_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = aoutput;
        record->record_type_struct = iseries_aoutput;
        record->class_specific_function_list
                                = &mxd_iseries_aout_analog_output_function_list;

        aoutput->record = record;
	iseries_aoutput->record = record;

	/* Raw analog output values are stored as doubles. */

	aoutput->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_iseries_aout_finish_record_initialization( MX_RECORD *record )
{
	MX_ISERIES_AOUTPUT *iseries_aoutput;
	char *ptr;

	iseries_aoutput = (MX_ISERIES_AOUTPUT *) record->record_type_struct;

	record->precision = (int) iseries_aoutput->default_precision;

	iseries_aoutput->command_prefix = iseries_aoutput->command[0];

	ptr = &(iseries_aoutput->command[1]);

	iseries_aoutput->command_index = mx_hex_string_to_unsigned_long( ptr );

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_iseries_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_iseries_aout_read()";

	MX_ISERIES_AOUTPUT *iseries_aoutput;
	MX_ISERIES *iseries;
	char command_prefix;
	mx_status_type mx_status;

	mx_status = mxd_iseries_aout_get_pointers( aoutput,
					&iseries_aoutput, &iseries, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( iseries_aoutput->command_prefix ) {
	case 'W':
		command_prefix = 'R';
		break;
	case 'P':
		command_prefix = 'G';
		break;
	default:
		command_prefix = iseries_aoutput->command_prefix;
		break;
	}

	mx_status = mxi_iseries_command( iseries,
					command_prefix,
					iseries_aoutput->command_index,
					iseries_aoutput->num_command_bytes,
					0,
					&(aoutput->raw_value.double_value),
					aoutput->record->precision,
					MXD_ISERIES_AIO_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_iseries_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_iseries_aout_write()";

	MX_ISERIES_AOUTPUT *iseries_aoutput;
	MX_ISERIES *iseries;
	double raw_value;
	mx_status_type mx_status;

	mx_status = mxd_iseries_aout_get_pointers( aoutput,
					&iseries_aoutput, &iseries, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_value = aoutput->raw_value.double_value;

	if ( raw_value < 0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested raw value %g for iSeries analog "
		"output '%s' is less than zero.  Allowed values must be "
		"greater than or equal to zero.",
			raw_value, aoutput->record->name );
	}

	mx_status = mxi_iseries_command( iseries,
					iseries_aoutput->command_prefix,
					iseries_aoutput->command_index,
					iseries_aoutput->num_command_bytes,
					aoutput->raw_value.double_value,
					NULL,
					aoutput->record->precision,
					MXD_ISERIES_AIO_DEBUG );

	return mx_status;
}


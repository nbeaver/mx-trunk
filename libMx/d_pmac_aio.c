/*
 * Name:    d_pmac_aio.c
 *
 * Purpose: MX input and output drivers to control PMAC variables as if they
 *          were analog inputs and outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2006, 2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PMAC_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "i_pmac.h"
#include "d_pmac_aio.h"

/* Initialize the PMAC analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_pmac_ain_record_function_list = {
	NULL,
	mxd_pmac_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_pmac_ain_analog_input_function_list = {
	mxd_pmac_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_pmac_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_PMAC_AINPUT_STANDARD_FIELDS
};

long mxd_pmac_ain_num_record_fields
		= sizeof( mxd_pmac_ain_record_field_defaults )
			/ sizeof( mxd_pmac_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmac_ain_rfield_def_ptr
			= &mxd_pmac_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_pmac_aout_record_function_list = {
	NULL,
	mxd_pmac_aout_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_pmac_aout_analog_output_function_list = {
	mxd_pmac_aout_read,
	mxd_pmac_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_pmac_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_PMAC_AOUTPUT_STANDARD_FIELDS
};

long mxd_pmac_aout_num_record_fields
		= sizeof( mxd_pmac_aout_record_field_defaults )
			/ sizeof( mxd_pmac_aout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmac_aout_rfield_def_ptr
			= &mxd_pmac_aout_record_field_defaults[0];

static mx_status_type
mxd_pmac_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_PMAC_AINPUT **pmac_ainput,
			MX_PMAC **pmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmac_ain_get_pointers()";

	MX_RECORD *pmac_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pmac_ainput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PMAC_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pmac == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pmac_ainput = (MX_PMAC_AINPUT *) ainput->record->record_type_struct;

	if ( *pmac_ainput == (MX_PMAC_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PMAC_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	pmac_record = (*pmac_ainput)->pmac_record;

	if ( pmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_PMAC pointer for PMAC analog input record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( pmac_record->mx_type != MXI_CTRL_PMAC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"pmac_record '%s' for PMAC analog input '%s' is not a PMAC record.  "
"Instead, it is a '%s' record.",
			pmac_record->name, ainput->record->name,
			mx_get_driver_name( pmac_record ) );
	}

	*pmac = (MX_PMAC *) pmac_record->record_type_struct;

	if ( *pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PMAC pointer for PMAC record '%s' used by PMAC analog input "
	"record '%s' and passed by '%s' is NULL.",
			pmac_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pmac_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_PMAC_AOUTPUT **pmac_aoutput,
			MX_PMAC **pmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmac_aout_get_pointers()";

	MX_RECORD *pmac_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pmac_aoutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PMAC_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pmac == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pmac_aoutput = (MX_PMAC_AOUTPUT *) aoutput->record->record_type_struct;

	if ( *pmac_aoutput == (MX_PMAC_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PMAC_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	pmac_record = (*pmac_aoutput)->pmac_record;

	if ( pmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_PMAC pointer for PMAC analog output record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( pmac_record->mx_type != MXI_CTRL_PMAC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"pmac_record '%s' for PMAC analog output '%s' is not a PMAC record.  "
"Instead, it is a '%s' record.",
			pmac_record->name, aoutput->record->name,
			mx_get_driver_name( pmac_record ) );
	}

	*pmac = (MX_PMAC *) pmac_record->record_type_struct;

	if ( *pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PMAC pointer for PMAC record '%s' used by PMAC analog output "
	"record '%s' and passed by '%s' is NULL.",
			pmac_record->name,
			aoutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_pmac_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_pmac_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_PMAC_AINPUT *pmac_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        pmac_ainput = (MX_PMAC_AINPUT *) malloc( sizeof(MX_PMAC_AINPUT) );

        if ( pmac_ainput == (MX_PMAC_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PMAC_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = pmac_ainput;
        record->class_specific_function_list
                                = &mxd_pmac_ain_analog_input_function_list;

        analog_input->record = record;
	pmac_ainput->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_pmac_ain_read()";

	MX_PMAC_AINPUT *pmac_ainput;
	MX_PMAC *pmac;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	pmac = NULL;
	pmac_ainput = NULL;

	mx_status = mxd_pmac_ain_get_pointers( ainput,
					&pmac_ainput, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pmac->num_cards > 1 ) {
		snprintf( command, sizeof(command),
			"@%lx%s",
			pmac_ainput->card_number,
			pmac_ainput->pmac_variable_name );
	} else {
		strlcpy( command,
			pmac_ainput->pmac_variable_name,
			sizeof(command) );
	}

	mx_status = mxi_pmac_command( pmac, command,
				response, sizeof( response ),
				MXD_PMAC_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%lg", &(ainput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"PMAC variable value not found in response '%s' for "
		"PMAC analog input record '%s'.",
			response, ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_pmac_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_pmac_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_PMAC_AOUTPUT *pmac_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *)
					malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        pmac_aoutput = (MX_PMAC_AOUTPUT *) malloc( sizeof(MX_PMAC_AOUTPUT) );

        if ( pmac_aoutput == (MX_PMAC_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PMAC_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = pmac_aoutput;
        record->class_specific_function_list
                                = &mxd_pmac_aout_analog_output_function_list;

        analog_output->record = record;
	pmac_aoutput->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_pmac_aout_read()";

	MX_PMAC_AOUTPUT *pmac_aoutput;
	MX_PMAC *pmac;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_pmac_aout_get_pointers( aoutput,
					&pmac_aoutput, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pmac->num_cards > 1 ) {
		snprintf( command, sizeof(command),
			"@%lx%s",
			pmac_aoutput->card_number,
			pmac_aoutput->pmac_variable_name );
	} else {
		strlcpy( command,
			pmac_aoutput->pmac_variable_name,
			sizeof(command) );
	}

	mx_status = mxi_pmac_command( pmac, command,
				response, sizeof( response ),
				MXD_PMAC_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%lg", &(aoutput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"PMAC variable value not found in response '%s' for "
		"PMAC analog output record '%s'.",
			response, aoutput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_pmac_aout_write()";

	MX_PMAC_AOUTPUT *pmac_aoutput;
	MX_PMAC *pmac;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_pmac_aout_get_pointers( aoutput,
					&pmac_aoutput, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pmac->num_cards > 1 ) {
		snprintf( command, sizeof(command),
			"@%lx%s=%g",
			pmac_aoutput->card_number,
			pmac_aoutput->pmac_variable_name,
			aoutput->raw_value.double_value );
	} else {
		snprintf( command, sizeof(command),
			"%s=%g",
			pmac_aoutput->pmac_variable_name,
			aoutput->raw_value.double_value );
	}

	mx_status = mxi_pmac_command( pmac, command,
					NULL, 0, MXD_PMAC_AIO_DEBUG );

	return mx_status;
}


/*
 * Name:    d_mclennan_aio.c
 *
 * Purpose: MX input and output drivers to control Mclennan analog I/O ports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MCLENNAN_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "d_mclennan.h"
#include "d_mclennan_aio.h"

/* Initialize the MCLENNAN analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_mclennan_ain_record_function_list = {
	NULL,
	mxd_mclennan_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_mclennan_ain_analog_input_function_list = {
	mxd_mclennan_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_mclennan_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_MCLENNAN_AINPUT_STANDARD_FIELDS
};

long mxd_mclennan_ain_num_record_fields
		= sizeof( mxd_mclennan_ain_record_field_defaults )
			/ sizeof( mxd_mclennan_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mclennan_ain_rfield_def_ptr
			= &mxd_mclennan_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_mclennan_aout_record_function_list = {
	NULL,
	mxd_mclennan_aout_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_mclennan_aout_analog_output_function_list = {
	mxd_mclennan_aout_read,
	mxd_mclennan_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_mclennan_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_MCLENNAN_AOUTPUT_STANDARD_FIELDS
};

long mxd_mclennan_aout_num_record_fields
		= sizeof( mxd_mclennan_aout_record_field_defaults )
			/ sizeof( mxd_mclennan_aout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mclennan_aout_rfield_def_ptr
			= &mxd_mclennan_aout_record_field_defaults[0];

static mx_status_type
mxd_mclennan_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_MCLENNAN_AINPUT **mclennan_ainput,
			MX_MCLENNAN **mclennan,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mclennan_ain_get_pointers()";

	MX_RECORD *mclennan_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mclennan_ainput == (MX_MCLENNAN_AINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCLENNAN_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mclennan == (MX_MCLENNAN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCLENNAN pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mclennan_ainput = (MX_MCLENNAN_AINPUT *)
				ainput->record->record_type_struct;

	if ( *mclennan_ainput == (MX_MCLENNAN_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCLENNAN_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	mclennan_record = (*mclennan_ainput)->mclennan_record;

	if ( mclennan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MCLENNAN pointer for MCLENNAN analog input "
		"record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( mclennan_record->mx_type != MXT_MTR_MCLENNAN ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"mclennan_record '%s' for Mclennan analog input '%s' "
		"is not a Mclennan record.  Instead, it is a '%s' record.",
			mclennan_record->name, ainput->record->name,
			mx_get_driver_name( mclennan_record ) );
	}

	*mclennan = (MX_MCLENNAN *) mclennan_record->record_type_struct;

	if ( *mclennan == (MX_MCLENNAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_MCLENNAN pointer for Mclennan record '%s' used by "
	"Mclennan analog input record '%s' and passed by '%s' is NULL.",
			mclennan_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_mclennan_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_MCLENNAN_AOUTPUT **mclennan_aoutput,
			MX_MCLENNAN **mclennan,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mclennan_aout_get_pointers()";

	MX_RECORD *mclennan_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mclennan_aoutput == (MX_MCLENNAN_AOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCLENNAN_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mclennan == (MX_MCLENNAN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCLENNAN pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mclennan_aoutput = (MX_MCLENNAN_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *mclennan_aoutput == (MX_MCLENNAN_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCLENNAN_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	mclennan_record = (*mclennan_aoutput)->mclennan_record;

	if ( mclennan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MCLENNAN pointer for MCLENNAN analog input "
		"record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( mclennan_record->mx_type != MXT_MTR_MCLENNAN ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"mclennan_record '%s' for Mclennan analog input '%s' "
		"is not a Mclennan record.  Instead, it is a '%s' record.",
			mclennan_record->name, aoutput->record->name,
			mx_get_driver_name( mclennan_record ) );
	}

	*mclennan = (MX_MCLENNAN *) mclennan_record->record_type_struct;

	if ( *mclennan == (MX_MCLENNAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_MCLENNAN pointer for Mclennan record '%s' used by "
	"Mclennan analog input record '%s' and passed by '%s' is NULL.",
			mclennan_record->name,
			aoutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_mclennan_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_mclennan_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_MCLENNAN_AINPUT *mclennan_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        mclennan_ainput = (MX_MCLENNAN_AINPUT *)
				malloc( sizeof(MX_MCLENNAN_AINPUT) );

        if ( mclennan_ainput == (MX_MCLENNAN_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MCLENNAN_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = mclennan_ainput;
        record->class_specific_function_list
			= &mxd_mclennan_ain_analog_input_function_list;

        analog_input->record = record;
	mclennan_ainput->record = record;

	/* Raw analog input values are stored as longs. */

	analog_input->subclass = MXT_AIN_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_mclennan_ain_read()";

	MX_MCLENNAN_AINPUT *mclennan_ainput;
	MX_MCLENNAN *mclennan;
	char command[80];
	char response[80];
	int num_items;
	long port_number;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	mclennan = NULL;
	mclennan_ainput = NULL;

	mx_status = mxd_mclennan_ain_get_pointers( ainput,
					&mclennan_ainput, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mclennan->num_ainput_ports == 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Mclennan controller '%s' used by analog input record '%s' "
		"does not support analog input ports.",
			mclennan->record->name,
			ainput->record->name );
	}

	port_number = mclennan_ainput->port_number;

	if ((port_number < 1) || (port_number > mclennan->num_ainput_ports)) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %ld used by analog input record '%s' is outside "
	"the legal range of 1 to %ld.", port_number,
			ainput->record->name,
			mclennan->num_ainput_ports );
	}

	sprintf( command, "AI%ld", port_number );

	mx_status = mxd_mclennan_command( mclennan, command,
				response, sizeof( response ),
				MXD_MCLENNAN_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%ld", &(ainput->raw_value.long_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Could not find a numerical value in the response to an 'AI' "
		"command to Mclennan controller '%s' for "
		"analog input record '%s'.  Response = '%s'",
			mclennan->record->name,
			ainput->record->name,
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_mclennan_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_mclennan_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_MCLENNAN_AOUTPUT *mclennan_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *)
					malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        mclennan_aoutput = (MX_MCLENNAN_AOUTPUT *)
				malloc( sizeof(MX_MCLENNAN_AOUTPUT) );

        if ( mclennan_aoutput == (MX_MCLENNAN_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MCLENNAN_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = mclennan_aoutput;
        record->class_specific_function_list
			= &mxd_mclennan_aout_analog_output_function_list;

        analog_output->record = record;
	mclennan_aoutput->record = record;

	/* Raw analog output values are stored as long. */

	analog_output->subclass = MXT_AOU_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	/* Just report back the value most recently written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_mclennan_aout_write()";

	MX_MCLENNAN_AOUTPUT *mclennan_aoutput;
	MX_MCLENNAN *mclennan;
	char command[80];
	long port_number;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	mclennan = NULL;
	mclennan_aoutput = NULL;

	mx_status = mxd_mclennan_aout_get_pointers( aoutput,
					&mclennan_aoutput, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mclennan->num_aoutput_ports == 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Mclennan controller '%s' used by analog output record '%s' "
		"does not support analog output ports.",
			mclennan->record->name,
			aoutput->record->name );
	}

	port_number = mclennan_aoutput->port_number;

	if ((port_number < 1) || (port_number > mclennan->num_aoutput_ports)) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %ld used by analog output record '%s' is outside "
	"the legal range of 1 to %ld.", port_number,
			aoutput->record->name,
			mclennan->num_aoutput_ports );
	}

	sprintf( command, "AO%ld/%ld", port_number,
				aoutput->raw_value.long_value );

	mx_status = mxd_mclennan_command( mclennan, command,
					NULL, 0, MXD_MCLENNAN_AIO_DEBUG );

	return mx_status;
}


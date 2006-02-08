/*
 * Name:    d_tracker_aio.c
 *
 * Purpose: MX drivers to control Data Track Tracker locations as
 *          analog I/O ports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_TRACKER_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "d_tracker_aio.h"

/* Initialize the Tracker analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_tracker_ain_record_function_list = {
	NULL,
	mxd_tracker_ain_create_record_structures,
	mxd_tracker_ain_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_tracker_ain_analog_input_function_list = {
	mxd_tracker_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_tracker_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_TRACKER_AINPUT_STANDARD_FIELDS
};

mx_length_type mxd_tracker_ain_num_record_fields
		= sizeof( mxd_tracker_ain_record_field_defaults )
			/ sizeof( mxd_tracker_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_tracker_ain_rfield_def_ptr
			= &mxd_tracker_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_tracker_aout_record_function_list = {
	NULL,
	mxd_tracker_aout_create_record_structures,
	mxd_tracker_aout_finish_record_initialization
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_tracker_aout_analog_output_function_list = {
	mxd_tracker_aout_read,
	mxd_tracker_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_tracker_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_TRACKER_AOUTPUT_STANDARD_FIELDS
};

mx_length_type mxd_tracker_aout_num_record_fields
		= sizeof( mxd_tracker_aout_record_field_defaults )
			/ sizeof( mxd_tracker_aout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_tracker_aout_rfield_def_ptr
			= &mxd_tracker_aout_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_tracker_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_TRACKER_AINPUT **tracker_ainput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_tracker_ain_get_pointers()";

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( tracker_ainput == (MX_TRACKER_AINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_TRACKER_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*tracker_ainput = (MX_TRACKER_AINPUT *)
				ainput->record->record_type_struct;

	if ( *tracker_ainput == (MX_TRACKER_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_TRACKER_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_tracker_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_TRACKER_AOUTPUT **tracker_aoutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_tracker_aout_get_pointers()";

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( tracker_aoutput == (MX_TRACKER_AOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_TRACKER_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*tracker_aoutput = (MX_TRACKER_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *tracker_aoutput == (MX_TRACKER_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_TRACKER_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_tracker_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_tracker_ain_create_record_structures()";

        MX_ANALOG_INPUT *ainput;
        MX_TRACKER_AINPUT *tracker_ainput;

        /* Allocate memory for the necessary structures. */

        ainput = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        tracker_ainput = (MX_TRACKER_AINPUT *)
				malloc( sizeof(MX_TRACKER_AINPUT) );

        if ( tracker_ainput == (MX_TRACKER_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_TRACKER_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = ainput;
        record->record_type_struct = tracker_ainput;
        record->class_specific_function_list
                                = &mxd_tracker_ain_analog_input_function_list;

        ainput->record = record;
	tracker_ainput->record = record;

	/* Raw analog input values are stored as doubles. */

	ainput->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tracker_ain_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_tracker_ain_finish_record_initialization()";

	MX_ANALOG_INPUT *ainput;
	MX_TRACKER_AINPUT *tracker_ainput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_tracker_ain_get_pointers( ainput,
						&tracker_ainput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_verify_configuration(tracker_ainput->rs232_record,
				MXF_232_DONT_CARE, MXF_232_DONT_CARE,
				(char) MXF_232_DONT_CARE, MXF_232_DONT_CARE,
				(char) MXF_232_DONT_CARE, 0x0d0a, 0x0d0a );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_analog_input_finish_record_initialization( record );

        return mx_status;
}

MX_EXPORT mx_status_type
mxd_tracker_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_tracker_ain_read()";

	MX_TRACKER_AINPUT *tracker_ainput;
	char command[80], response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_tracker_ain_get_pointers( ainput,
						&tracker_ainput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, ";%03d RA %d 1", tracker_ainput->address,
						tracker_ainput->location );

	mx_status = mxd_tracker_command( ainput->record,
					tracker_ainput->rs232_record,
					command,
					response, sizeof(response),
					MXD_TRACKER_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%lg", &(ainput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"A numerical value was not found in the response "
		"to command '%s' for analog input '%s'.  Response = '%s'",
			command, ainput->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_tracker_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_tracker_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *aoutput;
        MX_TRACKER_AOUTPUT *tracker_aoutput;

        /* Allocate memory for the necessary structures. */

        aoutput = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        tracker_aoutput = (MX_TRACKER_AOUTPUT *)
				malloc( sizeof(MX_TRACKER_AOUTPUT) );

        if ( tracker_aoutput == (MX_TRACKER_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_TRACKER_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = aoutput;
        record->record_type_struct = tracker_aoutput;
        record->class_specific_function_list
                                = &mxd_tracker_aout_analog_output_function_list;

        aoutput->record = record;
	tracker_aoutput->record = record;

	/* Raw analog output values are stored as doubles. */

	aoutput->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tracker_aout_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_tracker_aout_finish_record_initialization()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_TRACKER_AOUTPUT *tracker_aoutput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_tracker_aout_get_pointers( aoutput,
						&tracker_aoutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_verify_configuration(tracker_aoutput->rs232_record,
				MXF_232_DONT_CARE, MXF_232_DONT_CARE,
				(char) MXF_232_DONT_CARE, MXF_232_DONT_CARE,
				(char) MXF_232_DONT_CARE, 0x0d0a, 0x0d0a );
        return mx_status;
}

MX_EXPORT mx_status_type
mxd_tracker_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_tracker_aout_read()";

	MX_TRACKER_AOUTPUT *tracker_aoutput;
	char command[80], response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_tracker_aout_get_pointers( aoutput,
						&tracker_aoutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, ";%03d RA %d 1", tracker_aoutput->address,
						tracker_aoutput->location );

	mx_status = mxd_tracker_command( aoutput->record,
					tracker_aoutput->rs232_record,
					command,
					response, sizeof(response),
					MXD_TRACKER_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%lg", &(aoutput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"A numerical value was not found out the response "
		"to command '%s' for analog output '%s'.  Response = '%s'",
			command, aoutput->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tracker_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_tracker_aout_write()";

	MX_TRACKER_AOUTPUT *tracker_aoutput;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_tracker_aout_get_pointers( aoutput,
						&tracker_aoutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, ";%03d SA %d %g", tracker_aoutput->address,
					tracker_aoutput->location,
					aoutput->raw_value.double_value );

	mx_status = mxd_tracker_command( aoutput->record,
					tracker_aoutput->rs232_record,
					command,
					NULL, 0,
					MXD_TRACKER_AIO_DEBUG );
	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxd_tracker_command( MX_RECORD *record,
			MX_RECORD *rs232_record,
			char *command,
			char *response,
			size_t max_response_length,
			int debug_flag )
{
	static const char fname[] = "mxd_tracker_command()";

	mx_status_type mx_status;

	mx_status = mx_rs232_putline( rs232_record, command,
					NULL, MXD_TRACKER_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( rs232_record,
					response, sizeof(response),
					NULL, MXD_TRACKER_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response == (char *) NULL ) {

		/* The returned value is expected to be "OK". */

		if ( strcmp( response, "OK" ) != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The response for command '%s' to record '%s' "
			"was not the string 'OK'.  Instead, it responded '%s'",
				command, record->name, response );
		}
	} else {
		if ( response[0] == '?' ) {
			if ( strcmp( response+1, "99999" ) == 0 ) {
				return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
				"Record '%s' is over range.",
					record->name );
			} else
			if ( strcmp( response+1, "19999" ) == 0 ) {
				return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
				"Record '%s' is under range.",
					record->name );
			} else {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
				"Unexpected error response '%s' for command "
				"'%s' sent to record '%s'.",
			    		response, command, record->name );
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}



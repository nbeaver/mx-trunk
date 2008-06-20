/*
 * Name:    d_bkprecision_912x_aio.c
 *
 * Purpose: MX analog I/O drivers for the BK Precision 912x series of
 *          programmable power supplies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_BKPRECISION_912X_AIO_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "i_bkprecision_912x.h"
#include "d_bkprecision_912x_aio.h"

/* Initialize the analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_ain_record_function_list = {
	NULL,
	mxd_bkprecision_912x_ain_create_record_structures,
	mx_analog_input_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bkprecision_912x_ain_open
};

MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_bkprecision_912x_ain_analog_input_function_list =
{
	mxd_bkprecision_912x_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_bkprecision_912x_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_BKPRECISION_912X_AINPUT_STANDARD_FIELDS
};

long mxd_bkprecision_912x_ain_num_record_fields
		= sizeof( mxd_bkprecision_912x_ain_record_field_defaults )
		  / sizeof( mxd_bkprecision_912x_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_ain_rfield_def_ptr
			= &mxd_bkprecision_912x_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_aout_record_function_list = {
	NULL,
	mxd_bkprecision_912x_aout_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bkprecision_912x_aout_open
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_bkprecision_912x_aout_analog_output_function_list =
{
	mxd_bkprecision_912x_aout_read,
	mxd_bkprecision_912x_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_bkprecision_912x_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_BKPRECISION_912X_AOUTPUT_STANDARD_FIELDS
};

long mxd_bkprecision_912x_aout_num_record_fields
		= sizeof( mxd_bkprecision_912x_aout_record_field_defaults )
		  / sizeof( mxd_bkprecision_912x_aout_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_aout_rfield_def_ptr
			= &mxd_bkprecision_912x_aout_record_field_defaults[0];

static mx_status_type
mxd_bkprecision_912x_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_BKPRECISION_912X_AINPUT **bkprecision_912x_ainput,
			MX_BKPRECISION_912X **bkprecision_912x,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bkprecision_912x_ain_get_pointers()";

	MX_RECORD *bkprecision_912x_record;
	MX_BKPRECISION_912X_AINPUT *bkprecision_912x_ainput_ptr;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bkprecision_912x_ainput_ptr = (MX_BKPRECISION_912X_AINPUT *)
					ainput->record->record_type_struct;

	if (bkprecision_912x_ainput_ptr == (MX_BKPRECISION_912X_AINPUT *) NULL)
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BKPRECISION_912X_AINPUT pointer for record '%s' passed "
		"by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( bkprecision_912x_ainput != (MX_BKPRECISION_912X_AINPUT **) NULL ) {
		*bkprecision_912x_ainput = bkprecision_912x_ainput_ptr;
	}

	if ( bkprecision_912x != (MX_BKPRECISION_912X **) NULL ) {
		bkprecision_912x_record =
			bkprecision_912x_ainput_ptr->bkprecision_912x_record;

		if ( bkprecision_912x_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_BKPRECISION_912X pointer for analog "
			"input record '%s' passed by '%s' is NULL.",
				ainput->record->name, calling_fname );
		}

		*bkprecision_912x = (MX_BKPRECISION_912X *)
			bkprecision_912x_record->record_type_struct;

		if ( *bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BKPRECISION_912X pointer for BK Precision "
			"power supply '%s' used by analog input record '%s'.",
				bkprecision_912x_record->name,
				ainput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_bkprecision_912x_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_BKPRECISION_912X_AOUTPUT **bkprecision_912x_aoutput,
			MX_BKPRECISION_912X **bkprecision_912x,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bkprecision_912x_aout_get_pointers()";

	MX_RECORD *bkprecision_912x_record;
	MX_BKPRECISION_912X_AOUTPUT *bkprecision_912x_aoutput_ptr;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bkprecision_912x_aoutput_ptr = (MX_BKPRECISION_912X_AOUTPUT *)
					aoutput->record->record_type_struct;

	if (bkprecision_912x_aoutput_ptr == (MX_BKPRECISION_912X_AOUTPUT *)NULL)	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BKPRECISION_912X_AOUTPUT pointer for record '%s' passed "
		"by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if (bkprecision_912x_aoutput != (MX_BKPRECISION_912X_AOUTPUT **) NULL) {
		*bkprecision_912x_aoutput = bkprecision_912x_aoutput_ptr;
	}

	if ( bkprecision_912x != (MX_BKPRECISION_912X **) NULL ) {
		bkprecision_912x_record =
			bkprecision_912x_aoutput_ptr->bkprecision_912x_record;

		if ( bkprecision_912x_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_BKPRECISION_912X pointer for analog "
			"output record '%s' passed by '%s' is NULL.",
				aoutput->record->name, calling_fname );
		}

		*bkprecision_912x = (MX_BKPRECISION_912X *)
			bkprecision_912x_record->record_type_struct;

		if ( *bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BKPRECISION_912X pointer for BK Precision "
			"power supply '%s' used by analog output record '%s'.",
				bkprecision_912x_record->name,
				aoutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_bkprecision_912x_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_bkprecision_912x_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_BKPRECISION_912X_AINPUT *bkprecision_912x_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        bkprecision_912x_ainput = (MX_BKPRECISION_912X_AINPUT *)
				malloc( sizeof(MX_BKPRECISION_912X_AINPUT) );

        if ( bkprecision_912x_ainput == (MX_BKPRECISION_912X_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Cannot allocate memory for MX_BKPRECISION_912X_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = bkprecision_912x_ainput;
        record->class_specific_function_list
			= &mxd_bkprecision_912x_ain_analog_input_function_list;

        analog_input->record = record;
	bkprecision_912x_ainput->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_ain_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_bkprecision_912x_ain_open()";

	MX_ANALOG_INPUT *ainput;
	MX_BKPRECISION_912X_AINPUT *bkprecision_912x_ainput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_bkprecision_912x_ain_get_pointers( ainput,
			&bkprecision_912x_ainput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_strncasecmp("VOLTAGE", bkprecision_912x_ainput->input_type_name,
		strlen(bkprecision_912x_ainput->input_type_name) ) == 0 )
	{
		bkprecision_912x_ainput->input_type
					= MXT_BKPRECISION_912X_VOLTAGE;
	} else
	if ( mx_strncasecmp("CURRENT", bkprecision_912x_ainput->input_type_name,
		strlen(bkprecision_912x_ainput->input_type_name) ) == 0 )
	{
		bkprecision_912x_ainput->input_type
					= MXT_BKPRECISION_912X_CURRENT;
	} else
	if ( mx_strncasecmp("POWER", bkprecision_912x_ainput->input_type_name,
		strlen(bkprecision_912x_ainput->input_type_name) ) == 0 )
	{
		bkprecision_912x_ainput->input_type
					= MXT_BKPRECISION_912X_POWER;
	} else
	if ( mx_strncasecmp("DVM", bkprecision_912x_ainput->input_type_name,
		strlen(bkprecision_912x_ainput->input_type_name) ) == 0 )
	{
		bkprecision_912x_ainput->input_type
					= MXT_BKPRECISION_912X_DVM;
	} else
	if ( mx_strncasecmp("RESISTANCE",
		bkprecision_912x_ainput->input_type_name,
		strlen(bkprecision_912x_ainput->input_type_name) ) == 0 )
	{
		bkprecision_912x_ainput->input_type
					= MXT_BKPRECISION_912X_RESISTANCE;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized input type '%s' specified for "
		"BK Precision analog input '%s'.",
			bkprecision_912x_ainput->input_type_name,
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_bkprecision_912x_ain_read()";

	MX_BKPRECISION_912X_AINPUT *bkprecision_912x_ainput;
	MX_BKPRECISION_912X *bkprecision_912x;
	int num_items;
	char response[40];
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_ain_get_pointers( ainput,
			&bkprecision_912x_ainput, &bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bkprecision_912x_ainput->input_type ) {
	case MXT_BKPRECISION_912X_VOLTAGE:
		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
						"MEASURE:VOLTAGE?",
						response, sizeof(response),
						MXD_BKPRECISION_912X_AIO_DEBUG);
		break;
	case MXT_BKPRECISION_912X_CURRENT:
		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
						"MEASURE:CURRENT?",
						response, sizeof(response),
						MXD_BKPRECISION_912X_AIO_DEBUG);
		break;
	case MXT_BKPRECISION_912X_POWER:
		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
						"MEASURE:POWER?",
						response, sizeof(response),
						MXD_BKPRECISION_912X_AIO_DEBUG);
		break;
	case MXT_BKPRECISION_912X_DVM:
		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
						"MEASURE:DVM?",
						response, sizeof(response),
						MXD_BKPRECISION_912X_AIO_DEBUG);
		break;
	case MXT_BKPRECISION_912X_RESISTANCE:
		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
						"MEASURE:RESISTANCE?",
						response, sizeof(response),
						MXD_BKPRECISION_912X_AIO_DEBUG);
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal input type %lu requested for analog input '%s'.",
			bkprecision_912x_ainput->input_type,
			ainput->record->name );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%lg", &(ainput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"A numerical value was not found in the response '%s' "
		"from analog input '%s'.",
			response, ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_bkprecision_912x_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_bkprecision_912x_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_BKPRECISION_912X_AOUTPUT *bkprecision_912x_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        bkprecision_912x_aoutput = (MX_BKPRECISION_912X_AOUTPUT *)
				malloc( sizeof(MX_BKPRECISION_912X_AOUTPUT) );

        if ( bkprecision_912x_aoutput == (MX_BKPRECISION_912X_AOUTPUT *) NULL )
	{
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Cannot allocate memory for MX_BKPRECISION_912X_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = bkprecision_912x_aoutput;
        record->class_specific_function_list
		= &mxd_bkprecision_912x_aout_analog_output_function_list;

        analog_output->record = record;
	bkprecision_912x_aoutput->record = record;

	/* Raw analog output values are stored as double. */

	analog_output->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_aout_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_bkprecision_912x_aout_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_BKPRECISION_912X_AOUTPUT *bkprecision_912x_aoutput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_bkprecision_912x_aout_get_pointers( aoutput,
			&bkprecision_912x_aoutput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_strncasecmp( "VOLTAGE",
		bkprecision_912x_aoutput->output_type_name,
		strlen(bkprecision_912x_aoutput->output_type_name) ) == 0 )
	{
		bkprecision_912x_aoutput->output_type
			= MXT_BKPRECISION_912X_VOLTAGE;
	} else
	if ( mx_strncasecmp( "CURRENT",
		bkprecision_912x_aoutput->output_type_name,
		strlen(bkprecision_912x_aoutput->output_type_name) ) == 0 )
	{
		bkprecision_912x_aoutput->output_type
			= MXT_BKPRECISION_912X_CURRENT;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized output type '%s' specified for "
		"BK Precision analog output '%s'.",
			bkprecision_912x_aoutput->output_type_name,
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_bkprecision_912x_aout_read()";

	MX_BKPRECISION_912X_AOUTPUT *bkprecision_912x_aoutput;
	MX_BKPRECISION_912X *bkprecision_912x;
	int num_items;
	char response[40];
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_aout_get_pointers( aoutput,
			&bkprecision_912x_aoutput, &bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bkprecision_912x_aoutput->output_type ) {
	case MXT_BKPRECISION_912X_VOLTAGE:
		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
						"VOLTAGE?",
						response, sizeof(response),
						MXD_BKPRECISION_912X_AIO_DEBUG);
		break;
	case MXT_BKPRECISION_912X_CURRENT:
		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
						"CURRENT?",
						response, sizeof(response),
						MXD_BKPRECISION_912X_AIO_DEBUG);
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output type %lu requested for analog output '%s'.",
			bkprecision_912x_aoutput->output_type,
			aoutput->record->name );
		break;
	}

	num_items = sscanf(response, "%lg", &(aoutput->raw_value.double_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"A numerical value was not found in the response '%s' "
		"from analog output '%s'.",
			response, aoutput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_bkprecision_912x_aout_write()";

	MX_BKPRECISION_912X_AOUTPUT *bkprecision_912x_aoutput;
	MX_BKPRECISION_912X *bkprecision_912x;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_aout_get_pointers( aoutput,
			&bkprecision_912x_aoutput, &bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bkprecision_912x_aoutput->output_type ) {
	case MXT_BKPRECISION_912X_VOLTAGE:
		snprintf( command, sizeof(command),
			"VOLTAGE %f", aoutput->raw_value.double_value );
		break;
	case MXT_BKPRECISION_912X_CURRENT:
		snprintf( command, sizeof(command),
			"CURRENT %f", aoutput->raw_value.double_value );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output type %lu requested for analog output '%s'.",
			bkprecision_912x_aoutput->output_type,
			aoutput->record->name );
		break;
	}

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, NULL, 0,
					MXD_BKPRECISION_912X_AIO_DEBUG );

	return mx_status;
}


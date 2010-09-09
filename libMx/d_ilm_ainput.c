/*
 * Name:    d_ilm_ainput.c
 *
 * Purpose: Analog input driver for values returned by the READ command for
 *          Oxford Instruments ILM (Intelligent Level Meter) controllers.
 *
 * Author:  William Lavender / Henry Bellamy
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ILM_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_analog_input.h"
#include "i_isobus.h"
#include "i_ilm.h"
#include "d_ilm_ainput.h"

/* Initialize the ILM analog input driver function tables. */

MX_RECORD_FUNCTION_LIST mxd_ilm_ainput_record_function_list = {
	NULL,
	mxd_ilm_ainput_create_record_structures,
	mx_analog_input_finish_record_initialization,
	NULL,
	NULL,
	mxd_ilm_ainput_open
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_ilm_ainput_analog_input_function_list = {
	mxd_ilm_ainput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_ilm_ainput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_ILM_AINPUT_STANDARD_FIELDS
};

long mxd_ilm_ainput_num_record_fields
		= sizeof( mxd_ilm_ainput_record_field_defaults )
			/ sizeof( mxd_ilm_ainput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ilm_ainput_rfield_def_ptr
		= &mxd_ilm_ainput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_ilm_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_ILM_AINPUT **ilm_ainput,
				MX_ILM **ilm,
				MX_ISOBUS **isobus,
				const char *calling_fname )
{
	static const char fname[] = "mxd_ilm_ainput_get_pointers()";

	MX_ILM_AINPUT *ilm_ainput_ptr;
	MX_RECORD *ilm_record, *isobus_record;
	MX_ILM *ilm_ptr;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed was NULL." );
	}

	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_ANALOG_INPUT pointer passed was NULL." );
	}

	ilm_ainput_ptr = (MX_ILM_AINPUT *)
				ainput->record->record_type_struct;

	if ( ilm_ainput_ptr == (MX_ILM_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ILM_AINPUT pointer for analog input '%s' is NULL",
			ainput->record->name );
	}

	if ( ilm_ainput != (MX_ILM_AINPUT **) NULL ) {
		*ilm_ainput = ilm_ainput_ptr;
	}

	ilm_record = ilm_ainput_ptr->ilm_record;

	if ( ilm_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The ilm_record pointer for analog input '%s' is NULL.",
			ainput->record->name );
	}

	if ( ilm_record->mx_type != MXI_CTRL_ILM ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"ilm_record '%s' for ILM analog input '%s' "
		"is not an 'ilm' record.  Instead, it is of type '%s'.",
			ilm_record->name, ainput->record->name,
			mx_get_driver_name( ilm_record ) );
	}

	ilm_ptr = (MX_ILM *) ilm_record->record_type_struct;

	if ( ilm_ptr == (MX_ILM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ILM pointer for ILM record '%s' "
		"used by ILM analog input '%s' is NULL.",
			ilm_record->name,
			ainput->record->name );
	}

	if ( ilm != (MX_ILM **) NULL ) {
		*ilm = ilm_ptr;
	}

	if ( isobus != (MX_ISOBUS **) NULL ) {
		isobus_record = ilm_ptr->isobus_record;

		if ( isobus_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The isobus_record pointer for ILM "
			"record '%s' is NULL.", ilm_record->name );
		}

		*isobus = isobus_record->record_type_struct;

		if ( (*isobus) == (MX_ISOBUS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_ISOBUS pointer for ISOBUS record '%s' "
			"is NULL.", isobus_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_ilm_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_ilm_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_ILM_AINPUT *ilm_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_ANALOG_INPUT structure." );
	}

	ilm_ainput = (MX_ILM_AINPUT *) malloc( sizeof(MX_ILM_AINPUT) );

	if ( ilm_ainput == (MX_ILM_AINPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_ILM_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = ilm_ainput;
	record->class_specific_function_list =
		&mxd_ilm_ainput_analog_input_function_list;

	ainput->record = record;
	ilm_ainput->record = record;

	/* Raw analog input values are stored as doubles. */

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ilm_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ilm_ainput_open()";

	MX_ANALOG_INPUT *ainput;
	MX_ILM_AINPUT *ilm_ainput = NULL;
	MX_ILM *ilm = NULL;
	MX_ISOBUS *isobus = NULL;
	char response[80];
	int channel, channel_usage;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXD_ILM_AINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	ainput = record->record_class_struct;

	mx_status = mxd_ilm_ainput_get_pointers( ainput,
					&ilm_ainput, &ilm, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (ilm_ainput->parameter_type < 1)
	  || (ilm_ainput->parameter_type > 13) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal parameter number %ld specified for ILM ainput "
		"record '%s'.  The allowed values are from 1 to 13.",
			ilm_ainput->parameter_type,
			ilm_ainput->record->name );
	}

	/* Get the ILM status. */

	mx_status = mxi_isobus_command( isobus, ilm->isobus_address,
					"X", response, sizeof(response),
					ilm->maximum_retries,
					MXD_ILM_AINPUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ilm_ainput->parameter_type ) {
	case 1:
	case 6:
	case 11:
		channel = 1;
		channel_usage = (int)
			mx_hex_char_to_unsigned_long( response[1] );
		break;
	case 2:
	case 7:
	case 12:
		channel = 2;
		channel_usage = (int)
			mx_hex_char_to_unsigned_long( response[2] );
		break;
	case 3:
	case 13:
		channel = 3;
		channel_usage = (int)
			mx_hex_char_to_unsigned_long( response[2] );
		break;
	default:
		channel = -1;
		channel_usage = -1;
		break;
	}

	switch( channel_usage ) {
	case 1:
	case 2:
	case 3:
		break;
	case 0:
		mx_warning( "Channel %d for ILM ainput record '%s' "
		"is marked as 'not in use'.",
			channel, ilm_ainput->record->name );
		break;
	case 9:
		mx_warning( "Channel %d for ILM ainput record '%s' "
		"has an error (usually means probe unplugged).",
			channel, ilm_ainput->record->name );
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognized channel usage value %x reported "
		"for channel %d of ILM ainput record '%s'.",
			channel_usage, channel,
			ilm_ainput->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ilm_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_ilm_ainput_read()";

	MX_ILM_AINPUT *ilm_ainput = NULL;
	MX_ILM *ilm = NULL;
	MX_ISOBUS *isobus = NULL;
	char command[20];
	char response[80];
	int num_items;
	double raw_value;
	mx_status_type mx_status;

	mx_status = mxd_ilm_ainput_get_pointers( ainput,
					&ilm_ainput, &ilm, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
			"R%ld", ilm_ainput->parameter_type );

	mx_status = mxi_isobus_command( isobus, ilm->isobus_address,
					command, response, sizeof(response),
					ilm->maximum_retries,
					MXD_ILM_AINPUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "R%lf", &raw_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"ILM controller '%s' sent an unrecognizable response '%s' "
		"in response to the command '%s'.",
			ilm->record->name, response, command );
	}

	ainput->raw_value.double_value = raw_value;

	return MX_SUCCESSFUL_RESULT;
}


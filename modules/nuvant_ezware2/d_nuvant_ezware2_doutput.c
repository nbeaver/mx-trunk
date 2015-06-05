/*
 * Name:    d_nuvant_ezware2_doutput.c
 *
 * Purpose: MX driver for NuVant EZWare2 digital output channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NUVANT_EZWARE2_DOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_nuvant_ezware2.h"
#include "d_nuvant_ezware2_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_nuvant_ezware2_doutput_record_function_list = {
	NULL,
	mxd_nuvant_ezware2_doutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_nuvant_ezware2_doutput_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_nuvant_ezware2_doutput_digital_output_function_list = {
	mxd_nuvant_ezware2_doutput_read,
	mxd_nuvant_ezware2_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_nuvant_ezware2_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_NUVANT_EZWARE2_DOUTPUT_STANDARD_FIELDS
};

long mxd_nuvant_ezware2_doutput_num_record_fields
	= sizeof( mxd_nuvant_ezware2_doutput_record_field_defaults )
		/ sizeof( mxd_nuvant_ezware2_doutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_nuvant_ezware2_doutput_rfield_def_ptr
			= &mxd_nuvant_ezware2_doutput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_nuvant_ezware2_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_NUVANT_EZWARE2_DOUTPUT **ezware2_doutput,
			MX_NUVANT_EZWARE2 **ezware2,
			const char *calling_fname )
{
	static const char fname[] = "mxd_nuvant_ezware2_doutput_get_pointers()";

	MX_NUVANT_EZWARE2_DOUTPUT *ezware2_doutput_ptr;
	MX_RECORD *ezware2_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	  "the MX_RECORD pointer for the MX_DIGITAL_OUTPUT pointer %p is NULL.",
			doutput );
	}

	ezware2_doutput_ptr = (MX_NUVANT_EZWARE2_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( ezware2_doutput_ptr == (MX_NUVANT_EZWARE2_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NUVANT_EZWARE2_DOUTPUT pointer for "
		"digital output '%s' passed by '%s' is NULL",
			doutput->record->name, calling_fname );
	}

	if ( ezware2_doutput != (MX_NUVANT_EZWARE2_DOUTPUT **) NULL ) {
		*ezware2_doutput = ezware2_doutput_ptr;
	}

	if ( ezware2 != (MX_NUVANT_EZWARE2 **) NULL ) {
		ezware2_record = ezware2_doutput_ptr->nuvant_ezware2_record;

		if ( ezware2_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The ezware2_record pointer for record '%s' is NULL.",
				doutput->record->name );
		}

		*ezware2 = (MX_NUVANT_EZWARE2 *)ezware2_record->record_type_struct;

		if ( (*ezware2) == (MX_NUVANT_EZWARE2 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NUVANT_EZWARE2 pointer for EZstat record '%s' "
			"used by record '%s' is NULL.",
				ezware2_record->name,
				doutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_nuvant_ezware2_doutput_create_record_structures( MX_RECORD *record )
{
        const char fname[] =
		"mxd_nuvant_ezware2_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_NUVANT_EZWARE2_DOUTPUT *nuvant_ezware2_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        nuvant_ezware2_doutput = (MX_NUVANT_EZWARE2_DOUTPUT *)
				malloc( sizeof(MX_NUVANT_EZWARE2_DOUTPUT) );

        if ( nuvant_ezware2_doutput == (MX_NUVANT_EZWARE2_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_NUVANT_EZWARE2_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = nuvant_ezware2_doutput;
        record->class_specific_function_list
		= &mxd_nuvant_ezware2_doutput_digital_output_function_list;

        digital_output->record = record;
	nuvant_ezware2_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezware2_doutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_nuvant_ezware2_doutput_open()";

	MX_DIGITAL_OUTPUT *doutput = NULL;
	MX_NUVANT_EZWARE2_DOUTPUT *ezware2_doutput = NULL;
	MX_NUVANT_EZWARE2 *ezware2 = NULL;
	char *type_name = NULL;
	size_t len;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_nuvant_ezware2_doutput_get_pointers( doutput,
					&ezware2_doutput, &ezware2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	type_name = ezware2_doutput->output_type_name;

	len = strlen( type_name );

	if ( mx_strncasecmp( type_name, "cell_enable", len ) == 0 ) {
		ezware2_doutput->output_type =
				MXT_NUVANT_EZWARE2_DOUTPUT_CELL_ENABLE;
	} else
	if ( mx_strncasecmp( type_name, "external_switch", len ) == 0 ) {
		ezware2_doutput->output_type =
				MXT_NUVANT_EZWARE2_DOUTPUT_EXTERNAL_SWITCH;
	} else
	if ( mx_strncasecmp( type_name, "mode_select", len ) == 0 ) {
		ezware2_doutput->output_type =
				MXT_NUVANT_EZWARE2_DOUTPUT_MODE_SELECT;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Output type '%s' is not supported for record '%s'.",
			ezware2_doutput->output_type_name,
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezware2_doutput_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_nuvant_ezware2_doutput_read()";

	MX_NUVANT_EZWARE2_DOUTPUT *ezware2_doutput = NULL;
	MX_NUVANT_EZWARE2 *ezware2 = NULL;
	LVBoolean cell_status;
	mx_status_type mx_status;

	mx_status = mxd_nuvant_ezware2_doutput_get_pointers( doutput,
					&ezware2_doutput, &ezware2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezware2_doutput->output_type ) {
	case MXT_NUVANT_EZWARE2_DOUTPUT_CELL_ENABLE:
		GetCellState( &cell_status );

		doutput->value = cell_status;
		break;
	case MXT_NUVANT_EZWARE2_DOUTPUT_MODE_SELECT:
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output type %lu requested for record '%s'.",
			doutput->value, doutput->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezware2_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_nuvant_ezware2_doutput_write()";

	MX_NUVANT_EZWARE2_DOUTPUT *ezware_doutput = NULL;
	MX_NUVANT_EZWARE2 *ezware = NULL;
	mx_status_type mx_status;

	mx_status = mxd_nuvant_ezware2_doutput_get_pointers( doutput,
					&ezware_doutput, &ezware, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezware_doutput->output_type ) {
	case MXT_NUVANT_EZWARE2_DOUTPUT_CELL_ENABLE:
		if ( (doutput->value < 0) || (doutput->value > 1) ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The value %ld is not valid for '%s'.  "
			"The allowed values are 0 and 1.",
				doutput->value,
				doutput->record->name );
		}

		EnableDisableCell2( doutput->value, ezware->device_number );
		break;
	case MXT_NUVANT_EZWARE2_DOUTPUT_MODE_SELECT:
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output type %lu requested for record '%s'.",
			doutput->value, doutput->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}


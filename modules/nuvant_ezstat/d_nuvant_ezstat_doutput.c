/*
 * Name:    d_nuvant_ezstat_doutput.c
 *
 * Purpose: MX driver for NuVant EZstat digital output channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NUVANT_EZSTAT_DOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_nuvant_ezstat.h"
#include "d_nuvant_ezstat_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_nuvant_ezstat_doutput_record_function_list = {
	NULL,
	mxd_nuvant_ezstat_doutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_nuvant_ezstat_doutput_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_nuvant_ezstat_doutput_digital_output_function_list = {
	mxd_nuvant_ezstat_doutput_read,
	mxd_nuvant_ezstat_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_nuvant_ezstat_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_NUVANT_EZSTAT_DOUTPUT_STANDARD_FIELDS
};

long mxd_nuvant_ezstat_doutput_num_record_fields
	= sizeof( mxd_nuvant_ezstat_doutput_record_field_defaults )
		/ sizeof( mxd_nuvant_ezstat_doutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_nuvant_ezstat_doutput_rfield_def_ptr
			= &mxd_nuvant_ezstat_doutput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_nuvant_ezstat_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_NUVANT_EZSTAT_DOUTPUT **ezstat_doutput,
			MX_NUVANT_EZSTAT **ezstat,
			const char *calling_fname )
{
	static const char fname[] = "mxd_nuvant_ezstat_doutput_get_pointers()";

	MX_NUVANT_EZSTAT_DOUTPUT *ezstat_doutput_ptr;
	MX_RECORD *ezstat_record;

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

	ezstat_doutput_ptr = (MX_NUVANT_EZSTAT_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( ezstat_doutput_ptr == (MX_NUVANT_EZSTAT_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NUVANT_EZSTAT_DOUTPUT pointer for "
		"digital output '%s' passed by '%s' is NULL",
			doutput->record->name, calling_fname );
	}

	if ( ezstat_doutput != (MX_NUVANT_EZSTAT_DOUTPUT **) NULL ) {
		*ezstat_doutput = ezstat_doutput_ptr;
	}

	if ( ezstat != (MX_NUVANT_EZSTAT **) NULL ) {
		ezstat_record = ezstat_doutput_ptr->nuvant_ezstat_record;

		if ( ezstat_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The ezstat_record pointer for record '%s' is NULL.",
				doutput->record->name );
		}

		*ezstat = (MX_NUVANT_EZSTAT *)ezstat_record->record_type_struct;

		if ( (*ezstat) == (MX_NUVANT_EZSTAT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NUVANT_EZSTAT pointer for EZstat record '%s' "
			"used by record '%s' is NULL.",
				ezstat_record->name,
				doutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_doutput_create_record_structures( MX_RECORD *record )
{
        const char fname[] =
		"mxd_nuvant_ezstat_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_NUVANT_EZSTAT_DOUTPUT *nuvant_ezstat_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        nuvant_ezstat_doutput = (MX_NUVANT_EZSTAT_DOUTPUT *)
				malloc( sizeof(MX_NUVANT_EZSTAT_DOUTPUT) );

        if ( nuvant_ezstat_doutput == (MX_NUVANT_EZSTAT_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_NUVANT_EZSTAT_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = nuvant_ezstat_doutput;
        record->class_specific_function_list
		= &mxd_nuvant_ezstat_doutput_digital_output_function_list;

        digital_output->record = record;
	nuvant_ezstat_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_doutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_nuvant_ezstat_doutput_open()";

	MX_DIGITAL_OUTPUT *doutput = NULL;
	MX_NUVANT_EZSTAT_DOUTPUT *ezstat_doutput = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	char *type_name = NULL;
	size_t len;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_nuvant_ezstat_doutput_get_pointers( doutput,
					&ezstat_doutput, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	type_name = ezstat_doutput->output_type_name;

	len = strlen( type_name );

	if ( mx_strncasecmp( type_name, "cell_enable", len ) == 0 ) {
		ezstat_doutput->output_type =
				MXT_NUVANT_EZSTAT_DOUTPUT_CELL_ENABLE;
	} else
	if ( mx_strncasecmp( type_name, "external_switch", len ) == 0 ) {
		ezstat_doutput->output_type =
				MXT_NUVANT_EZSTAT_DOUTPUT_EXTERNAL_SWITCH;
	} else
	if ( mx_strncasecmp( type_name, "mode_select", len ) == 0 ) {
		ezstat_doutput->output_type =
				MXT_NUVANT_EZSTAT_DOUTPUT_MODE_SELECT;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Output type '%s' is not supported for record '%s'.",
			ezstat_doutput->output_type_name,
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_doutput_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_nuvant_ezstat_doutput_read()";

	MX_NUVANT_EZSTAT_DOUTPUT *ezstat_doutput = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	mx_status_type mx_status;

	mx_status = mxd_nuvant_ezstat_doutput_get_pointers( doutput,
					&ezstat_doutput, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezstat_doutput->output_type ) {
	case MXT_NUVANT_EZSTAT_DOUTPUT_CELL_ENABLE:
		mx_status = mx_digital_output_read( ezstat->p00_record,
							&(doutput->value) );
		break;
	case MXT_NUVANT_EZSTAT_DOUTPUT_EXTERNAL_SWITCH:
		mx_status = mx_digital_output_read( ezstat->p01_record,
							&(doutput->value) );
		break;
	case MXT_NUVANT_EZSTAT_DOUTPUT_MODE_SELECT:
		mx_status = mx_digital_output_read( ezstat->p10_record,
							&(doutput->value) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( doutput->value ) {
		case 0:
			ezstat->mode = MXF_NUVANT_EZSTAT_POTENTIOSTAT_MODE;
			break;
		case 1:
			ezstat->mode = MXF_NUVANT_EZSTAT_GALVANOSTAT_MODE;
			break;
		default:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The mode %lu reported by device '%s' is illegal.  "
			"The allowed mode values are 0 and 1.",
				doutput->value, doutput->record->name );
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output type %lu requested for record '%s'.",
			ezstat_doutput->output_type,
			doutput->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_nuvant_ezstat_doutput_write()";

	MX_NUVANT_EZSTAT_DOUTPUT *ezstat_doutput = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	mx_status_type mx_status;

	mx_status = mxd_nuvant_ezstat_doutput_get_pointers( doutput,
					&ezstat_doutput, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezstat_doutput->output_type ) {
	case MXT_NUVANT_EZSTAT_DOUTPUT_CELL_ENABLE:
		mx_status = mx_digital_output_write( ezstat->p00_record,
							doutput->value );
		break;
	case MXT_NUVANT_EZSTAT_DOUTPUT_EXTERNAL_SWITCH:
		mx_status = mx_digital_output_write( ezstat->p01_record,
							doutput->value );
		break;
	case MXT_NUVANT_EZSTAT_DOUTPUT_MODE_SELECT:
		switch( doutput->value ) {
		case 0:
			ezstat->mode = MXF_NUVANT_EZSTAT_POTENTIOSTAT_MODE;
			break;
		case 1:
			ezstat->mode = MXF_NUVANT_EZSTAT_GALVANOSTAT_MODE;
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The mode %lu specified for device '%s' is illegal.  "
			"The allowed mode values are 0 and 1.",
				doutput->value, doutput->record->name );
			break;
		}

		mx_status = mx_digital_output_write( ezstat->p10_record,
							doutput->value );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output type %lu requested for record '%s'.",
			doutput->value, doutput->record->name );
		break;
	}

	return mx_status;
}


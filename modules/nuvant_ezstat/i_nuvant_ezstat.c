/*
 * Name:    i_nuvant_ezstat.c
 *
 * Purpose: MX driver for NuVant EZstat controllers.
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

#define MXI_NUVANT_EZSTAT_DEBUG		TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_digital_output.h"
#include "i_nuvant_ezstat.h"

MX_RECORD_FUNCTION_LIST mxi_nuvant_ezstat_record_function_list = {
	NULL,
	mxi_nuvant_ezstat_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_nuvant_ezstat_open
};

MX_RECORD_FIELD_DEFAULTS mxi_nuvant_ezstat_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NUVANT_EZSTAT_STANDARD_FIELDS
};

long mxi_nuvant_ezstat_num_record_fields
		= sizeof( mxi_nuvant_ezstat_record_field_defaults )
		/ sizeof( mxi_nuvant_ezstat_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_nuvant_ezstat_rfield_def_ptr
			= &mxi_nuvant_ezstat_record_field_defaults[0];

static mx_status_type
mxi_nuvant_ezstat_get_pointers( MX_RECORD *record,
				MX_NUVANT_EZSTAT **nuvant_ezstat,
				const char *calling_fname )
{
	static const char fname[] = "mxi_nuvant_ezstat_get_pointers()";

	MX_NUVANT_EZSTAT *nuvant_ezstat_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	nuvant_ezstat_ptr = (MX_NUVANT_EZSTAT *) record->record_type_struct;

	if ( nuvant_ezstat_ptr == (MX_NUVANT_EZSTAT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NUVANT_EZSTAT pointer for record '%s' is NULL.",
			record->name );
	}

	if ( nuvant_ezstat != (MX_NUVANT_EZSTAT **) NULL ) {
		*nuvant_ezstat = nuvant_ezstat_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_nuvant_ezstat_create_record_structures()";

	MX_NUVANT_EZSTAT *nuvant_ezstat;

	/* Allocate memory for the necessary structures. */

	nuvant_ezstat = (MX_NUVANT_EZSTAT *) malloc( sizeof(MX_NUVANT_EZSTAT) );

	if ( nuvant_ezstat == (MX_NUVANT_EZSTAT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_NUVANT_EZSTAT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = nuvant_ezstat;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	nuvant_ezstat->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_nuvant_ezstat_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_nuvant_ezstat_open()";

	MX_NUVANT_EZSTAT *ezstat = NULL;
	unsigned long ezstat_mode;
	mx_status_type mx_status;

	mx_status = mxi_nuvant_ezstat_get_pointers( record, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NUVANT_EZSTAT_DEBUG
	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name));
#endif

	mx_status = mx_digital_output_read( ezstat->p10_record, &ezstat_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezstat_mode ) {
	case 0:
		ezstat->mode = MXF_NUVANT_EZSTAT_POTENTIOSTAT_MODE;
		break;
	case 1:
		ezstat->mode = MXF_NUVANT_EZSTAT_GALVANOSTAT_MODE;
		break;
	default:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The current EZstat mode %lu for device '%s' is illegal.  "
		"The allowed modes are 0 and 1.",
			ezstat_mode, record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}


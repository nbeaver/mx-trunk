/*
 * Name:    i_nuvant_ezware2.c
 *
 * Purpose: MX driver for NuVant EZstat controllers via EZWare2.DLL.
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

#define MXI_NUVANT_EZWARE2_DEBUG		TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_nuvant_ezware2.h"

#if 0
typedef uint8_t LVBoolean;

#include "EZWare2.h"

#else
int32_t __cdecl GetEzWareVersion(void);
#endif

MX_RECORD_FUNCTION_LIST mxi_nuvant_ezware2_record_function_list = {
	NULL,
	mxi_nuvant_ezware2_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_nuvant_ezware2_open
};

MX_RECORD_FIELD_DEFAULTS mxi_nuvant_ezware2_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NUVANT_EZWARE2_STANDARD_FIELDS
};

long mxi_nuvant_ezware2_num_record_fields
		= sizeof( mxi_nuvant_ezware2_record_field_defaults )
		/ sizeof( mxi_nuvant_ezware2_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_nuvant_ezware2_rfield_def_ptr
			= &mxi_nuvant_ezware2_record_field_defaults[0];

static mx_status_type
mxi_nuvant_ezware2_get_pointers( MX_RECORD *record,
				MX_NUVANT_EZWARE2 **nuvant_ezware2,
				const char *calling_fname )
{
	static const char fname[] = "mxi_nuvant_ezware2_get_pointers()";

	MX_NUVANT_EZWARE2 *nuvant_ezware2_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	nuvant_ezware2_ptr = (MX_NUVANT_EZWARE2 *) record->record_type_struct;

	if ( nuvant_ezware2_ptr == (MX_NUVANT_EZWARE2 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NUVANT_EZWARE2 pointer for record '%s' is NULL.",
			record->name );
	}

	if ( nuvant_ezware2 != (MX_NUVANT_EZWARE2 **) NULL ) {
		*nuvant_ezware2 = nuvant_ezware2_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_nuvant_ezware2_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_nuvant_ezware2_create_record_structures()";

	MX_NUVANT_EZWARE2 *nuvant_ezware2;

	/* Allocate memory for the necessary structures. */

	nuvant_ezware2 = (MX_NUVANT_EZWARE2 *)
				malloc( sizeof(MX_NUVANT_EZWARE2) );

	if ( nuvant_ezware2 == (MX_NUVANT_EZWARE2 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_NUVANT_EZWARE2 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = nuvant_ezware2;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	nuvant_ezware2->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_nuvant_ezware2_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_nuvant_ezware2_open()";

	MX_NUVANT_EZWARE2 *ezware2 = NULL;
	int32_t ezware_version;
	mx_status_type mx_status;

	mx_status = mxi_nuvant_ezware2_get_pointers( record, &ezware2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ezware_version = GetEzWareVersion();

#if MXI_NUVANT_EZWARE2_DEBUG
	MX_DEBUG(-2,("%s invoked for '%s', EZware version = %ld.",
		fname, record->name, (long) ezware_version ));
#endif

	return mx_status;
}

/*-------------------------------------------------------------------------*/

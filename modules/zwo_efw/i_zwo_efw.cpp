/*
 * Name:    i_zwo_efw.c
 *
 * Purpose: MX driver for ZWO Electronic Filter Wheels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_ZWO_EFW_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_motor.h"
#include "i_zwo_efw.h"
#include "d_zwo_efw_motor.h"

MX_RECORD_FUNCTION_LIST mxi_zwo_efw_record_function_list = {
	NULL,
	mxi_zwo_efw_create_record_structures,
	mxi_zwo_efw_finish_record_initialization,
	NULL,
	NULL,
	mxi_zwo_efw_open,
};

MX_RECORD_FIELD_DEFAULTS mxi_zwo_efw_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_ZWO_EFW_STANDARD_FIELDS
};

long mxi_zwo_efw_num_record_fields
		= sizeof( mxi_zwo_efw_record_field_defaults )
			/ sizeof( mxi_zwo_efw_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_zwo_efw_rfield_def_ptr
			= &mxi_zwo_efw_record_field_defaults[0];

/*---*/

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_zwo_efw_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_zwo_efw_create_record_structures()";

	MX_ZWO_EFW *zwo_efw = NULL;

	/* Allocate memory for the necessary structures. */

	zwo_efw = (MX_ZWO_EFW *) malloc( sizeof(MX_ZWO_EFW) );

	if ( zwo_efw == (MX_ZWO_EFW *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for an MX_ZWO_EFW structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = zwo_efw;

	record->record_function_list = &mxi_zwo_efw_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	zwo_efw->record = record;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_zwo_efw_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_zwo_efw_finish_record_initialization()";

	MX_ZWO_EFW *zwo_efw = NULL;
	unsigned long flags;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	zwo_efw = (MX_ZWO_EFW *) record->record_type_struct;

	if ( zwo_efw == (MX_ZWO_EFW *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ZWO_EFW pointer for record '%s' is NULL.",
			record->name );
	}

	flags = zwo_efw->zwo_efw_flags;

	flags = flags;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_zwo_efw_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_zwo_efw_open()";

	MX_ZWO_EFW *zwo_efw = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	zwo_efw = (MX_ZWO_EFW *) record->record_type_struct;

	if ( zwo_efw == (MX_ZWO_EFW *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ZWO_EFW pointer for record '%s' is NULL.",
			record->name );
	}

#if MXI_ZWO_EFW_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/


/*
 * Name:    i_k8055.c
 *
 * Purpose: MX interface driver for the Velleman K805
 *          USB Experiment Interface Board.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_K8055_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_k8055.h"

MX_RECORD_FUNCTION_LIST mxi_k8055_record_function_list = {
	NULL,
	mxi_k8055_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_k8055_open,
	mxi_k8055_close
};

MX_RECORD_FIELD_DEFAULTS mxi_k8055_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_K8055_STANDARD_FIELDS
};

long mxi_k8055_num_record_fields
		= sizeof( mxi_k8055_record_field_defaults )
			/ sizeof( mxi_k8055_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_k8055_rfield_def_ptr
			= &mxi_k8055_record_field_defaults[0];

static mx_status_type
mxi_k8055_get_pointers( MX_RECORD *record,
				MX_K8055 **k8055,
				const char *calling_fname )
{
	static const char fname[] = "mxi_k8055_get_pointers()";

	MX_K8055 *k8055_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	k8055_ptr = (MX_K8055 *) record->record_type_struct;

	if ( k8055_ptr == (MX_K8055 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_K8055 pointer for record '%s' is NULL.",
			record->name );
	}

	if ( k8055 != (MX_K8055 **) NULL ) {
		*k8055 = k8055_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

static long mxp_num_k8055_cards_loaded = 0;

/*------*/

MX_EXPORT mx_status_type
mxi_k8055_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_k8055_create_record_structures()";

	MX_K8055 *k8055;

	/* Allocate memory for the necessary structures. */

	k8055 = (MX_K8055 *) malloc( sizeof(MX_K8055) );

	if ( k8055 == (MX_K8055 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_K8055 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = k8055;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	k8055->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_k8055_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_k8055_open()";

	MX_K8055 *k8055;
	int devices_found, result;
	char *dll_version;
	mx_status_type mx_status;

	mx_status = mxi_k8055_get_pointers( record, &k8055, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dll_version = Version();

	MX_DEBUG(-2,("%s: K8055 DLL version = '%s'", fname, dll_version));

	devices_found = SearchDevices();

	MX_DEBUG(-2,("%s: K8055 devices found = %#x", fname, devices_found ));

	result = OpenDevice( k8055->card_address );

	MX_DEBUG(-2,("%s: K8055 OpenDevice( %lu ) = %d",
		fname, k8055->card_address, result ));

	if ( result == -1 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"K8055 card address %lu for record '%s' was not found.  "
		"Check to verify that it is plugged into a USB port.",
			k8055->card_address, record->name );
	}

	mxp_num_k8055_cards_loaded++;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_k8055_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_k8055_close()";

	MX_K8055 *k8055;
	mx_status_type mx_status;

	mx_status = mxi_k8055_get_pointers( record, &k8055, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mxp_num_k8055_cards_loaded--;

	if ( mxp_num_k8055_cards_loaded <= 0 ) {
		CloseDevice();
	}

	return mx_status;
}


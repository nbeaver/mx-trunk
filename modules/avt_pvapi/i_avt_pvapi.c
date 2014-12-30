/*
 * Name:    i_avt_pvapi.c
 *
 * Purpose: MX interface driver for the PvAPI C API used by
 *          cameras from Allied Vision Technologies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_AVT_PVAPI_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"

#include "i_avt_pvapi.h"

MX_RECORD_FUNCTION_LIST mxi_avt_pvapi_record_function_list = {
	NULL,
	mxi_avt_pvapi_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_avt_pvapi_open,
	mxi_avt_pvapi_close
};

MX_RECORD_FIELD_DEFAULTS mxi_avt_pvapi_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_AVT_PVAPI_STANDARD_FIELDS
};

long mxi_avt_pvapi_num_record_fields
		= sizeof( mxi_avt_pvapi_record_field_defaults )
			/ sizeof( mxi_avt_pvapi_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_avt_pvapi_rfield_def_ptr
			= &mxi_avt_pvapi_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_avt_pvapi_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_avt_pvapi_create_record_structures()";

	MX_AVT_PVAPI *avt_pvapi;

	/* Allocate memory for the necessary structures. */

	avt_pvapi = (MX_AVT_PVAPI *) malloc( sizeof(MX_AVT_PVAPI) );

	if ( avt_pvapi == (MX_AVT_PVAPI *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AVT_PVAPI structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = avt_pvapi;

	record->record_function_list = &mxi_avt_pvapi_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	avt_pvapi->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_avt_pvapi_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_avt_pvapi_open()";

	MX_AVT_PVAPI *avt_pvapi = NULL;
	unsigned long major_version, minor_version;
	unsigned long pvapi_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	avt_pvapi = (MX_AVT_PVAPI *) record->record_type_struct;

	if ( avt_pvapi == (MX_AVT_PVAPI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AVT_PVAPI pointer for record '%s' is NULL.", record->name);
	}

#if MXI_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	/* Show the version number of the PvApi library we are using. */

	PvVersion( &major_version, &minor_version );

#if MXI_AVT_PVAPI_DEBUG
	MX_DEBUG(-2,("%s: Using PvAPI version %lu.%lu",
		fname, major_version, minor_version));
#endif

	avt_pvapi->pvapi_version = 1000 * major_version + minor_version;

	/* Initialize the PvAPI module. */

	pvapi_status = PvInitialize();

	MX_DEBUG(-2,("%s: PvInitialize() status = %lu", fname, pvapi_status));

	if ( pvapi_status != ePvErrSuccess ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to initialize the PvAPI module failed "
		"with error code %lu", pvapi_status );
	}

	/* Give PvAPI a while to find the cameras. */

	mx_sleep(5);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_avt_pvapi_close( MX_RECORD *record )
{
#if MXI_AVT_PVAPI_DEBUG
	static const char fname[] = "mxi_avt_pvapi_close()";

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}


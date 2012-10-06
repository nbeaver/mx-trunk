/*
 * Name:    i_fastcam_pcclib.c
 *
 * Purpose: MX driver for Photron's FASTCAM PccLib library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_FASTCAM_PCCLIB_DEBUG	FALSE

#define MXI_FASTCAM_PCCLIB_DEBUG_OPEN	FALSE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_fastcam_pcclib.h"

MX_RECORD_FUNCTION_LIST mxi_fastcam_pcclib_record_function_list = {
	NULL,
	mxi_fastcam_pcclib_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_fastcam_pcclib_open
};

MX_RECORD_FIELD_DEFAULTS mxi_fastcam_pcclib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_FASTCAM_PCCLIB_STANDARD_FIELDS
};

long mxi_fastcam_pcclib_num_record_fields
		= sizeof( mxi_fastcam_pcclib_record_field_defaults )
			/ sizeof( mxi_fastcam_pcclib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_fastcam_pcclib_rfield_def_ptr
			= &mxi_fastcam_pcclib_record_field_defaults[0];

static mx_status_type
mxi_fastcam_pcclib_get_pointers( MX_RECORD *record,
				MX_FASTCAM_PCCLIB **fastcam_pcclib,
				const char *calling_fname )
{
	static const char fname[] = "mxi_fastcam_pcclib_get_pointers()";

	MX_FASTCAM_PCCLIB *fastcam_pcclib_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	fastcam_pcclib_ptr = (MX_FASTCAM_PCCLIB *) record->record_type_struct;

	if ( fastcam_pcclib_ptr == (MX_FASTCAM_PCCLIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_FASTCAM_PCCLIB pointer for record '%s' is NULL.",
			record->name );
	}

	if ( fastcam_pcclib != (MX_FASTCAM_PCCLIB **) NULL ) {
		*fastcam_pcclib = fastcam_pcclib_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_fastcam_pcclib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_fastcam_pcclib_create_record_structures()";

	MX_FASTCAM_PCCLIB *fastcam_pcclib;

	/* Allocate memory for the necessary structures. */

	fastcam_pcclib = (MX_FASTCAM_PCCLIB *)
				malloc( sizeof(MX_FASTCAM_PCCLIB) );

	if ( fastcam_pcclib == (MX_FASTCAM_PCCLIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_FASTCAM_PCCLIB structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = fastcam_pcclib;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	fastcam_pcclib->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_fastcam_pcclib_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_fastcam_pcclib_open()";

	MX_FASTCAM_PCCLIB *fastcam_pcclib;
	mx_status_type mx_status;

	mx_status = mxi_fastcam_pcclib_get_pointers( record,
						&fastcam_pcclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_FASTCAM_PCCLIB_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name));
#endif

	/* Create an empty array of camera records, which will be filled
	 * in by the individual camera drivers.
	 */

	fastcam_pcclib->num_cameras = 0;

	fastcam_pcclib->camera_record_array =
		(MX_RECORD **) calloc( fastcam_pcclib->max_cameras,
				sizeof(MX_RECORD *) );

	if ( fastcam_pcclib->camera_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of camera record pointers for record '%s'.",
			fastcam_pcclib->max_cameras, record->name );
	}

#if MXI_FASTCAM_PCCLIB_DEBUG_OPEN
	MX_DEBUG(-2,("%s complete for '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}


/*
 * Name:    v_rdi_mbc_save_frame.c
 *
 * Purpose: MX variable driver the ALS MBC beamline that overrides the
 *          default save_averaged_correction_frame function for the
 *          specified area detector.
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

#define MXV_RDI_MBC_SAVE_FRAME_DEBUG			TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_cfn.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_variable.h"

#include "v_rdi_mbc_save_frame.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxv_rdi_mbc_save_frame_record_function_list = {
	mx_variable_initialize_driver,
	mxv_rdi_mbc_save_frame_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxv_rdi_mbc_save_frame_open
};

MX_VARIABLE_FUNCTION_LIST mxv_rdi_mbc_save_frame_variable_function_list = {
	mxv_rdi_mbc_save_frame_send_variable,
	mxv_rdi_mbc_save_frame_receive_variable
};

/* Record field defaults for 'rdi_mbc_save_frame'. */

MX_RECORD_FIELD_DEFAULTS mxv_rdi_mbc_save_frame_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_RDI_MBC_SAVE_FRAME_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_rdi_mbc_save_frame_num_record_fields
	= sizeof( mxv_rdi_mbc_save_frame_record_field_defaults )
	/ sizeof( mxv_rdi_mbc_save_frame_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_rdi_mbc_save_frame_rfield_def_ptr
		= &mxv_rdi_mbc_save_frame_record_field_defaults[0];

/*---*/

static mx_status_type
mxv_rdi_mbc_save_frame_get_pointers( MX_VARIABLE *variable,
			MX_RDI_MBC_SAVE_FRAME **rdi_mbc_save_frame,
			MX_AREA_DETECTOR **ad,
			const char *calling_fname )
{
	static const char fname[] =
			"mxv_rdi_mbc_save_frame_get_pointers()";

	MX_RDI_MBC_SAVE_FRAME *rdi_mbc_save_frame_ptr;
	MX_RECORD *ad_record;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_VARIABLE %p is NULL.",
			variable );
	}

	rdi_mbc_save_frame_ptr = (MX_RDI_MBC_SAVE_FRAME *)
				variable->record->record_type_struct;

	if ( rdi_mbc_save_frame_ptr == (MX_RDI_MBC_SAVE_FRAME *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_RDI_MBC_SAVE_FRAME pointer for record '%s' is NULL.",
			variable->record->name );
	}

	if ( rdi_mbc_save_frame != (MX_RDI_MBC_SAVE_FRAME **) NULL ) {
		*rdi_mbc_save_frame = rdi_mbc_save_frame_ptr;
	}

	if ( ad != (MX_AREA_DETECTOR **) NULL ) {
		ad_record = rdi_mbc_save_frame_ptr->area_detector_record;

		if ( ad_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The area_detector_record pointer for "
			"variable record '%s' is NULL.",
				variable->record->name );
		}

		*ad = (MX_AREA_DETECTOR *) ad_record->record_class_struct;

		if ( (*ad) == (MX_AREA_DETECTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_AREA_DETECTOR pointer for record '%s' "
			"used by variable '%s' is NULL.",
				ad_record->name,
				variable->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxv_rdi_mbc_save_frame_override_fn( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxv_rdi_mbc_save_frame_override_fn()";

	MX_RDI_MBC_SAVE_FRAME *rdi_mbc_save_frame;
	MX_IMAGE_FRAME *dark_current_frame;
	unsigned long exposure_time_sec, exposure_time_nsec;
	double exposure_time;
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));

	/* If this is not a dark current measurement, then use the
	 * generic version of the function.
	 */

	if ( ad->correction_measurement_type != MXFT_AD_DARK_CURRENT_FRAME ) {

		mx_status =
		    mx_area_detector_default_save_averaged_correction_frame(ad);

		return mx_status;
	}

	/* Find the pointer to the MX_RDI_MBC_SAVE_FRAME object. */

	rdi_mbc_save_frame = (MX_RDI_MBC_SAVE_FRAME *) ad->application_ptr;

	if ( rdi_mbc_save_frame == (MX_RDI_MBC_SAVE_FRAME *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The application_ptr for area detector '%s' is NULL.",
			ad->record->name );
	}

	/* Get the exposure time for the dark current frame. */

	dark_current_frame = ad->dark_current_frame;

	if ( dark_current_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"It appears that a dark current frame has not been loaded "
		"or measured yet for this area detector '%s'.",
			ad->record->name );
	}

	exposure_time_sec = MXIF_EXPOSURE_TIME_SEC( dark_current_frame );

	exposure_time_nsec = MXIF_EXPOSURE_TIME_NSEC( dark_current_frame );

	exposure_time = exposure_time_sec + 1.0e-9 * exposure_time_nsec;

	MX_DEBUG(-2,("%s: exposure_time = (%lu,%lu) = %f seconds",
		fname, exposure_time_sec, exposure_time_nsec, exposure_time));

	/* For dark current frames, we use a special filename format. */

	snprintf( ad->saved_dark_current_filename,
		sizeof( ad->saved_dark_current_filename ),
		"%s/%.02f_avg.smv",
			rdi_mbc_save_frame->dark_current_directory_name,
			exposure_time );

	/* Now that we have set the filename, invoke the default function. */

	mx_status = mx_area_detector_default_save_averaged_correction_frame(ad);

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxv_rdi_mbc_save_frame_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_rdi_mbc_save_frame_create_record_structures()";

	MX_VARIABLE *variable;
	MX_RDI_MBC_SAVE_FRAME *rdi_mbc_save_frame = NULL;

	variable = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VARIABLE structure." );
	}

	rdi_mbc_save_frame = (MX_RDI_MBC_SAVE_FRAME *)
				malloc( sizeof(MX_RDI_MBC_SAVE_FRAME));

	if ( rdi_mbc_save_frame == (MX_RDI_MBC_SAVE_FRAME *) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_RDI_MBC_SAVE_FRAME structure.");
	}

	record->record_superclass_struct = variable;
	record->record_class_struct = NULL;
	record->record_type_struct = rdi_mbc_save_frame;
	record->superclass_specific_function_list = 
			&mxv_rdi_mbc_save_frame_variable_function_list;
	record->class_specific_function_list = NULL;

	variable->record = record;
	rdi_mbc_save_frame->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_save_frame_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_rdi_mbc_save_frame_open()";

	MX_VARIABLE *variable;
	MX_RDI_MBC_SAVE_FRAME *rdi_mbc_save_frame;
	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_rdi_mbc_save_frame_get_pointers( variable,
					&rdi_mbc_save_frame, &ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Try to save a pointer to the MX_RDI_MBC_SAVE_FRAME structure
	 * so that we can find it later in the save_averaged_correction_frame
	 * method.
	 */

	if ( ad->application_ptr != NULL ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"The application_ptr for area detector '%s' is "
		"already in use by something else.", record->name );
	}

	ad->application_ptr = rdi_mbc_save_frame;

	/* Find the MX_AREA_DETECTOR_FUNCTION_LIST pointer in the
	 * specified area detector record.
	 */

	flist = (MX_AREA_DETECTOR_FUNCTION_LIST *)
			ad->record->class_specific_function_list;

	if ( flist == (MX_AREA_DETECTOR_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR_FUNCTION_LIST pointer for record '%s' "
		"used by record '%s' is NULL.",
			ad->record->name, record->name );
	}

	/* Now that we have found the function list, override the
	 * save_averaged_correction_frame method in it.
	 */

	flist->save_averaged_correction_frame =
				mxv_rdi_mbc_save_frame_override_fn;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_save_frame_send_variable( MX_VARIABLE *variable )
{
	/* This is just a stub. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_save_frame_receive_variable( MX_VARIABLE *variable )
{
	/* This is just another stub. */

	return MX_SUCCESSFUL_RESULT;
}


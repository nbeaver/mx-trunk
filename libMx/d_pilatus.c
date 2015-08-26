/*
 * Name:    d_pilatus.c
 *
 * Purpose: MX area detector driver for Dectris Pilatus detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PILATUS_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_hrt.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_pilatus.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_pilatus_record_function_list = {
	mxd_pilatus_initialize_driver,
	mxd_pilatus_create_record_structures,
	mxd_pilatus_finish_record_initialization,
	NULL,
	NULL,
	mxd_pilatus_open,
	mxd_pilatus_close,
	NULL,
	mxd_pilatus_resynchronize
};

MX_AREA_DETECTOR_FUNCTION_LIST
mxd_pilatus_ad_function_list = {
	mxd_pilatus_arm,
	mxd_pilatus_trigger,
	NULL,
	mxd_pilatus_stop,
	mxd_pilatus_abort,
	mxd_pilatus_get_last_frame_number,
	mxd_pilatus_get_total_num_frames,
	mxd_pilatus_get_status,
	mxd_pilatus_get_extended_status,
	mxd_pilatus_readout_frame,
	mxd_pilatus_correct_frame,
	mxd_pilatus_transfer_frame,
	mxd_pilatus_load_frame,
	mxd_pilatus_save_frame,
	mxd_pilatus_copy_frame,
	mxd_pilatus_get_roi_frame,
	mxd_pilatus_get_parameter,
	mxd_pilatus_set_parameter,
	mxd_pilatus_measure_correction,
	NULL,
	mxd_pilatus_setup_exposure,
	mxd_pilatus_trigger_exposure
};

MX_RECORD_FIELD_DEFAULTS mxd_pilatus_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_PILATUS_STANDARD_FIELDS
};

long mxd_pilatus_num_record_fields
		= sizeof( mxd_pilatus_rf_defaults )
		  / sizeof( mxd_pilatus_rf_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_pilatus_rfield_def_ptr
			= &mxd_pilatus_rf_defaults[0];

/*---*/

static mx_status_type
mxd_pilatus_get_pointers( MX_AREA_DETECTOR *ad,
			MX_PILATUS **pilatus,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pilatus_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pilatus == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PILATUS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pilatus = (MX_PILATUS *)
				ad->record->record_type_struct;

	if ( *pilatus == (MX_PILATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_PILATUS pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pilatus_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pilatus_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_PILATUS *pilatus = NULL;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	pilatus = (MX_PILATUS *)
				malloc( sizeof(MX_PILATUS) );

	if ( pilatus == (MX_PILATUS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_PILATUS structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = pilatus;
	record->class_specific_function_list = 
			&mxd_pilatus_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	pilatus->record = record;

	ad->trigger_mode = 0;
	ad->initial_correction_flags = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pilatus_finish_record_initialization()";

	MX_AREA_DETECTOR *ad;
	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	pilatus = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pilatus_open()";

	MX_AREA_DETECTOR *ad;
	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	pilatus = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_pilatus_resynchronize()";

	MX_AREA_DETECTOR *ad;
	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	pilatus = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = record->record_class_struct;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_arm()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	pilatus = NULL;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_trigger()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	pilatus = NULL;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_stop()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_abort()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_get_last_frame_number( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_get_last_frame_number()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s: area detector '%s', last_frame_number = %ld",
		fname, ad->record->name, ad->last_frame_number ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_get_total_num_frames( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_get_total_num_frames()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s: area detector '%s', total_num_frames = %ld",
		fname, ad->record->name, ad->total_num_frames ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_get_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_get_status()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_pilatus_get_extended_status()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
	    fname, ad->last_frame_number, ad->total_num_frames, ad->status));
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_readout_frame()"; 
	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad,
						&pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', frame %ld.",
		fname, ad->record->name, ad->readout_frame ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_correct_frame()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,
		("%s invoked for area detector '%s', correction_flags=%#lx.",
		fname, ad->record->name, ad->correction_flags ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_pilatus_transfer_frame()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', transfer_frame = %ld.",
		fname, ad->record->name, ad->transfer_frame ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_load_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_load_frame()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	pilatus = NULL;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_save_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_save_frame()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_copy_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_copy_frame()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,
	("%s invoked for area detector '%s', source = %#lx, destination = %#lx",
		fname, ad->record->name, ad->copy_frame[0], ad->copy_frame[1]));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_get_roi_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_get_roi_frame()";

	MX_PILATUS *pilatus = NULL;
	MX_IMAGE_FRAME *roi_frame;
	mx_status_type mx_status;

	pilatus = NULL;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	roi_frame = ad->roi_frame;

	if ( roi_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The ROI MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pilatus_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_get_parameter()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		break;

	case MXLV_AD_MAXIMUM_FRAMESIZE:
		break;

	case MXLV_AD_FRAMESIZE:
		break;

	case MXLV_AD_BINSIZE:
		break;

	case MXLV_AD_CORRECTION_FLAGS:
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
#if MXD_PILATUS_DEBUG
		MX_DEBUG(-2,("%s: image format = %ld, format name = '%s'",
		    fname, ad->image_format, ad->image_format_name));
#endif
		break;

	case MXLV_AD_BYTE_ORDER:
		break;

	case MXLV_AD_TRIGGER_MODE:
		break;

	case MXLV_AD_EXPOSURE_MODE:
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		break;

	case MXLV_AD_BITS_PER_PIXEL:
		break;

	case MXLV_AD_DETECTOR_READOUT_TIME:
		break;

	case MXLV_AD_SEQUENCE_START_DELAY:
		break;

	case MXLV_AD_TOTAL_ACQUISITION_TIME:
		break;

	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		break;

	case MXLV_AD_SEQUENCE_TYPE:

#if MXD_PILATUS_DEBUG
		MX_DEBUG(-2,("%s: GET sequence_type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif

#if MXD_PILATUS_DEBUG
		MX_DEBUG(-2,("%s: sequence type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		break;

	case MXLV_AD_ROI:
		break;

	case MXLV_AD_SUBFRAME_SIZE:
		break;

	case MXLV_AD_USE_SCALED_DARK_CURRENT:
		break;

	case MXLV_AD_REGISTER_VALUE:
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		break;

	case MXLV_AD_CORRECTION_LOAD_FORMAT:
	case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		break;

	case MXLV_AD_CORRECTION_SAVE_FORMAT:
	case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_LOAD_FORMAT:
	case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_SAVE_FORMAT:
	case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		break;

	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:
		break;

	case MXLV_AD_DATAFILE_NAME:
		break;

	case MXLV_AD_DATAFILE_NUMBER:
		break;

	case MXLV_AD_DATAFILE_PATTERN:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		break;

	case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
		break;

	case MXLV_AD_FRAME_FILENAME:
		break;

	default:
		mx_status =
			mx_area_detector_default_get_parameter_handler( ad );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pilatus_set_parameter()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		break;

	case MXLV_AD_BINSIZE:
		break;

	case MXLV_AD_CORRECTION_FLAGS:
		break;

	case MXLV_AD_TRIGGER_MODE:
		break;

	case MXLV_AD_EXPOSURE_MODE:
		break;

	case MXLV_AD_SEQUENCE_TYPE:
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		break;

	case MXLV_AD_SEQUENCE_ONE_SHOT:
		break;

	case MXLV_AD_SEQUENCE_CONTINUOUS:
		break;

	case MXLV_AD_SEQUENCE_MULTIFRAME:
		break;

	case MXLV_AD_SEQUENCE_STROBE:
		break;

	case MXLV_AD_SEQUENCE_DURATION:
		break;

	case MXLV_AD_SEQUENCE_GATED:
		break;

	case MXLV_AD_ROI:
		break;

	case MXLV_AD_SUBFRAME_SIZE:
		break;

	case MXLV_AD_USE_SCALED_DARK_CURRENT:
		break;

	case MXLV_AD_GEOM_CORR_AFTER_FLOOD:
		break;

	case MXLV_AD_CORRECTION_FRAME_GEOM_CORR_LAST:
		break;

	case MXLV_AD_CORRECTION_FRAME_NO_GEOM_CORR:
		break;

	case MXLV_AD_REGISTER_VALUE:
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		break;

	case MXLV_AD_CORRECTION_LOAD_FORMAT:
		break;

	case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		break;

	case MXLV_AD_CORRECTION_SAVE_FORMAT:
		break;

	case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_LOAD_FORMAT:
		break;

	case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_SAVE_FORMAT:
		break;

	case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		break;

	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		break;

	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:
		break;

	case MXLV_AD_DATAFILE_NAME:
		break;

	case MXLV_AD_DATAFILE_NUMBER:
		break;

	case MXLV_AD_DATAFILE_PATTERN:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		break;

	case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		break;

	case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
		break;

	case MXLV_AD_FRAME_FILENAME:
		break;

	case MXLV_AD_RAW_LOAD_FRAME:
		break;

	case MXLV_AD_RAW_SAVE_FRAME:
		break;

	case MXLV_AD_BYTES_PER_FRAME:
	case MXLV_AD_BYTES_PER_PIXEL:
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Changing the parameter '%s' for area detector '%s' "
		"is not supported.", mx_get_field_label_string( ad->record,
			ad->parameter_type ), ad->record->name );
	default:
		mx_status =
			mx_area_detector_default_set_parameter_handler( ad );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_measure_correction()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
	MX_DEBUG(-2,("%s: type = %ld, time = %g, num_measurements = %ld",
		fname, ad->correction_measurement_type,
		ad->correction_measurement_time,
		ad->num_correction_measurements ));
#endif

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
	case MXFT_AD_FLOOD_FIELD_FRAME:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %ld is not supported "
		"for area detector '%s'.",
			ad->correction_measurement_type, ad->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_setup_exposure( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_setup_exposure()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', motor '%s', "
	"shutter '%s', trigger '%s', "
	"exposure distance = %f, exposure time = %f",
		fname, ad->record->name, ad->exposure_motor_name,
		ad->shutter_name, ad->exposure_trigger_name,
		ad->exposure_distance, ad->exposure_time ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pilatus_trigger_exposure( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_pilatus_trigger_exposure()";

	MX_PILATUS *pilatus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pilatus_get_pointers( ad, &pilatus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PILATUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	return mx_status;
}


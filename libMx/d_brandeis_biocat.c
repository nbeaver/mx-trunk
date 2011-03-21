/*
 * Name:    d_brandeis_biocat.c
 *
 * Purpose: MX driver for the Brandeis detector at the BioCAT sector
 *          of the Advanced Photon Source.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007, 2009-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_BRANDEIS_BIOCAT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_brandeis_biocat.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_brandeis_biocat_record_function_list = {
	mxd_brandeis_biocat_initialize_driver,
	mxd_brandeis_biocat_create_record_structures,
	mxd_brandeis_biocat_finish_record_initialization,
	NULL,
	NULL,
	mxd_brandeis_biocat_open,
	mxd_brandeis_biocat_close
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_brandeis_biocat_ad_function_list = {
	mxd_brandeis_biocat_arm,
	mxd_brandeis_biocat_trigger,
	mxd_brandeis_biocat_stop,
	mxd_brandeis_biocat_abort,
	NULL,
	NULL,
	NULL,
	mxd_brandeis_biocat_get_extended_status,
	mxd_brandeis_biocat_readout_frame,
	mxd_brandeis_biocat_correct_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_brandeis_biocat_get_parameter,
	mxd_brandeis_biocat_set_parameter,
	mx_area_detector_default_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_brandeis_biocat_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_BRANDEIS_BIOCAT_STANDARD_FIELDS
};

long mxd_brandeis_biocat_num_record_fields
		= sizeof( mxd_brandeis_biocat_record_field_defaults )
			/ sizeof( mxd_brandeis_biocat_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_brandeis_biocat_rfield_def_ptr
			= &mxd_brandeis_biocat_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_brandeis_biocat_get_pointers( MX_AREA_DETECTOR *ad,
			MX_BRANDEIS_BIOCAT **brandeis_biocat,
			const char *calling_fname )
{
	static const char fname[] = "mxd_brandeis_biocat_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (brandeis_biocat == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_BRANDEIS_BIOCAT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*brandeis_biocat = (MX_BRANDEIS_BIOCAT *)
				ad->record->record_type_struct;

	if ( *brandeis_biocat == (MX_BRANDEIS_BIOCAT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_BRANDEIS_BIOCAT pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_brandeis_biocat_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_brandeis_biocat_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_BRANDEIS_BIOCAT *brandeis_biocat;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	brandeis_biocat = (MX_BRANDEIS_BIOCAT *)
				malloc( sizeof(MX_BRANDEIS_BIOCAT) );

	if ( brandeis_biocat == (MX_BRANDEIS_BIOCAT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_BRANDEIS_BIOCAT structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = brandeis_biocat;
	record->class_specific_function_list = 
			&mxd_brandeis_biocat_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	brandeis_biocat->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_finish_record_initialization( MX_RECORD *record )
{
	return mx_area_detector_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_brandeis_biocat_open()";

	MX_AREA_DETECTOR *ad;
	MX_BRANDEIS_BIOCAT *brandeis_biocat;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_brandeis_biocat_get_pointers( ad,
						&brandeis_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BRANDEIS_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_brandeis_biocat_arm()";

	MX_BRANDEIS_BIOCAT *brandeis_biocat;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_brandeis_biocat_get_pointers( ad,
						&brandeis_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BRANDEIS_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	sp = &(ad->sequence_parameters);

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_brandeis_biocat_trigger()";

	MX_BRANDEIS_BIOCAT *brandeis_biocat;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_brandeis_biocat_get_pointers( ad,
						&brandeis_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BRANDEIS_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

#if MXD_BRANDEIS_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_brandeis_biocat_stop()";

	MX_BRANDEIS_BIOCAT *brandeis_biocat;
	mx_status_type mx_status;

	mx_status = mxd_brandeis_biocat_get_pointers( ad,
						&brandeis_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BRANDEIS_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_brandeis_biocat_abort()";

	MX_BRANDEIS_BIOCAT *brandeis_biocat;
	mx_status_type mx_status;

	mx_status = mxd_brandeis_biocat_get_pointers( ad,
						&brandeis_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BRANDEIS_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_brandeis_biocat_get_extended_status()";

	MX_BRANDEIS_BIOCAT *brandeis_biocat;
	mx_status_type mx_status;

	mx_status = mxd_brandeis_biocat_get_pointers( ad,
						&brandeis_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BRANDEIS_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	ad->last_frame_number = -1;

	ad->total_num_frames = 0;

	ad->status = 0;

	if ( 0 ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_brandeis_biocat_readout_frame()";

	MX_BRANDEIS_BIOCAT *brandeis_biocat;
	mx_status_type mx_status;

	mx_status = mxd_brandeis_biocat_get_pointers( ad,
						&brandeis_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BRANDEIS_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_brandeis_biocat_correct_frame()";

	MX_BRANDEIS_BIOCAT *brandeis_biocat;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_brandeis_biocat_get_pointers( ad,
						&brandeis_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BRANDEIS_BIOCAT_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	sp = &(ad->sequence_parameters);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_brandeis_biocat_get_parameter()";

	MX_BRANDEIS_BIOCAT *brandeis_biocat;
	MX_SEQUENCE_PARAMETERS seq;
	mx_status_type mx_status;

	mx_status = mxd_brandeis_biocat_get_pointers( ad,
						&brandeis_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BRANDEIS_BIOCAT_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		break;

	case MXLV_AD_FRAMESIZE:
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_image_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		break;

	case MXLV_AD_DETECTOR_READOUT_TIME:
		break;

	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		mx_status = mx_area_detector_get_sequence_parameters(
							ad->record, &seq );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch ( seq.sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_CONTINUOUS:
		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
		case MXT_SQ_STROBE:
		case MXT_SQ_DURATION:
			/* For these cases, use the default calculation. */

			ad->parameter_type = MXLV_AD_TOTAL_SEQUENCE_TIME;

			mx_status =
			   mx_area_detector_default_get_parameter_handler( ad );
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Area detector '%s' is configured for unsupported "
			"sequence type %ld.",
				ad->record->name,
				seq.sequence_type );
		}

#if MXD_BRANDEIS_BIOCAT_DEBUG
		MX_DEBUG(-2,("%s: ad->total_sequence_time = %g",
			fname, ad->total_sequence_time));
#endif
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		break;

	case MXLV_AD_TRIGGER_MODE:
		break;

	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_brandeis_biocat_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_brandeis_biocat_set_parameter()";

	MX_BRANDEIS_BIOCAT *brandeis_biocat;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_brandeis_biocat_get_pointers( ad,
						&brandeis_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BRANDEIS_BIOCAT_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif

	sp = &(ad->sequence_parameters);

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:

		/* Do not send any changes to the hardware when these two
		 * values are changed since the contents of the sequence
		 * parameter array may not match the change.  For example,
		 * if we are changing from one-shot mode to multiframe mode,
		 * parameter_array[0] is the exposure time in one-shot mode,
		 * but parameter_array[0] is the number of frames in
		 * multiframe mode.  Thus, the only quasi-safe time for
		 * sending changes to the hardware is when the contents
		 * of the sequence parameter array has been changed.
		 */
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		break; 

	case MXLV_AD_TRIGGER_MODE:
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Changing parameter '%s' for area detector '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
		break;

	default:
		mx_status = mx_area_detector_default_set_parameter_handler(ad);
		break;
	}

	return mx_status;
}


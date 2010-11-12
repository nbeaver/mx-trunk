/*
 * Name:    d_epics_area_detector.c
 *
 * Purpose: MX driver for the EPICS Area Detector record.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EPICS_AREA_DETECTOR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_epics.h"
#include "d_epics_area_detector.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_epics_ad_record_function_list = {
	mxd_epics_ad_initialize_driver,
	mxd_epics_ad_create_record_structures,
	mxd_epics_ad_finish_record_initialization,
	NULL,
	NULL,
	mxd_epics_ad_open
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_epics_ad_ad_function_list = {
	NULL,
	mxd_epics_ad_trigger,
	mxd_epics_ad_abort,
	mxd_epics_ad_abort,
	NULL,
	NULL,
	NULL,
	mxd_epics_ad_get_extended_status,
	mxd_epics_ad_readout_frame,
	mxd_epics_ad_correct_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_epics_ad_get_parameter,
	mxd_epics_ad_set_parameter,
	mx_area_detector_default_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_epics_ad_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_EPICS_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_epics_ad_num_record_fields
		= sizeof( mxd_epics_ad_record_field_defaults )
			/ sizeof( mxd_epics_ad_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_ad_rfield_def_ptr
			= &mxd_epics_ad_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_epics_ad_get_pointers( MX_AREA_DETECTOR *ad,
			MX_EPICS_AREA_DETECTOR **epics_ad,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_ad_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (epics_ad == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_EPICS_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*epics_ad = (MX_EPICS_AREA_DETECTOR *) ad->record->record_type_struct;

	if ( *epics_ad == (MX_EPICS_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_EPICS_AREA_DETECTOR pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_epics_ad_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_ad_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_epics_ad_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_EPICS_AREA_DETECTOR *epics_ad;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	epics_ad = (MX_EPICS_AREA_DETECTOR *)
				malloc( sizeof(MX_EPICS_AREA_DETECTOR) );

	if ( epics_ad == (MX_EPICS_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for an MX_EPICS_AREA_DETECTOR structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = epics_ad;
	record->class_specific_function_list = &mxd_epics_ad_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	epics_ad->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_epics_ad_finish_record_initialization()";

	MX_AREA_DETECTOR *ad;
	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	mx_status_type mx_status;

	mx_status = mx_area_detector_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(epics_ad->abort_pv),
			"%sAbort", epics_ad->epics_prefix );

	mx_epics_pvname_init( &(epics_ad->acquire_pv),
			"%sAcquire", epics_ad->epics_prefix );

	mx_epics_pvname_init( &(epics_ad->acquire_time_pv),
			"%sAcquireTime", epics_ad->epics_prefix );

	mx_epics_pvname_init( &(epics_ad->array_data_pv),
			"%sArrayData", epics_ad->epics_prefix );

	mx_epics_pvname_init( &(epics_ad->binx_pv),
			"%sBinX", epics_ad->epics_prefix );

	mx_epics_pvname_init( &(epics_ad->biny_pv),
			"%sBinY", epics_ad->epics_prefix );

	mx_epics_pvname_init( &(epics_ad->detector_state_pv),
			"%sDetectorState_RBV", epics_ad->epics_prefix );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_ad_open()";

	MX_AREA_DETECTOR *ad;
	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	char pvname[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char epics_string[41];
	int32_t data_type;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));

	mx_epics_set_debug_flag( TRUE );
#endif

	snprintf( pvname, sizeof(pvname),
		"%sManufacturer_RBV", epics_ad->epics_prefix );

	mx_status = mx_caget_by_name( pvname, MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: CCD manufacturer = '%s'", fname, epics_string));
#endif
	snprintf( pvname, sizeof(pvname),
		"%sModel_RBV", epics_ad->epics_prefix );

	mx_status = mx_caget_by_name( pvname, MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: CCD model = '%s'", fname, epics_string));
#endif


	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	epics_ad->acquisition_in_progress = FALSE;

	snprintf( pvname, sizeof(pvname),
		"%sDataType_RBV", epics_ad->epics_prefix );

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1, &data_type );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: bits per pixel = %ld", fname, ad->bits_per_pixel))
#endif

	switch( data_type ) {
	case 0:				/* Int8 */
	case 1:				/* UInt8 */
		ad->bits_per_pixel = 8;
		ad->bytes_per_pixel = 1;
		ad->image_format = MXT_IMAGE_FORMAT_GREY8;
		break;

	case 2:				/* Int16 */
	case 3:				/* UInt16 */
		ad->bits_per_pixel = 16;
		ad->bytes_per_pixel = 2;
		ad->image_format = MXT_IMAGE_FORMAT_GREY16;
		break;

	case 4:				/* Int32 */
	case 5:				/* UInt32 */
		ad->bits_per_pixel = 32;
		ad->bytes_per_pixel = 4;
		ad->image_format = MXT_IMAGE_FORMAT_GREY32;
		break;

	case 6:				/* Float32 */
	case 7:				/* Float64 */
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support for Float32 and Float64 data types is not yet "
		"implemented for EPICS Area Detector '%s'.",
			record->name );

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized data type %ld was requested "
		"for EPICS Area Detector '%s'.",
			(long) data_type, record->name );
		
	}

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						MXU_IMAGE_FORMAT_NAME_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: Apparently, the EPICS CCD driver _assumes_ that the
	 * maximum framesize is 2048 by 2048.
	 */

	ad->maximum_framesize[0] = 2048;
	ad->maximum_framesize[1] = 2048;

	/* Get the current binsize.  This will update the current framesize
	 * as well as a side effect.
	 */

	mx_status = mx_area_detector_get_binsize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: framesize = (%lu, %lu), binsize = (%lu, %lu)",
		fname, ad->framesize[0], ad->framesize[1],
		ad->binsize[0], ad->binsize[1]));
#endif

	/* Does the area detector support reading out the image data?
	 *
	 * Begin by testing for the presence of the $(P)$(R)ArrayData PV.
	 */

	mx_status = mx_epics_pv_connect( &(epics_ad->array_data_pv),
			MXF_EPVC_QUIET | MXF_EPVC_WAIT_FOR_CONNECTION );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		epics_ad->array_data_available = TRUE;
		break;
	case MXE_TIMED_OUT:
		epics_ad->array_data_available = FALSE;
		break;
	default:
		epics_ad->array_data_available = FALSE;

		return mx_error( mx_status.code,
				mx_status.location,
				mx_status.message );
		break;
	}

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: array_data_available = %d",
		fname, epics_ad->array_data_available));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_trigger()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	int32_t acquire;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	acquire = 1;

	mx_status = mx_caput_nowait( &(epics_ad->acquire_pv),
					MX_CA_LONG, 1, &acquire );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_ad->acquisition_in_progress = TRUE;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_abort()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	int32_t abort_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	abort_value = 1;

	mx_status = mx_caput( &(epics_ad->abort_pv),
					MX_CA_LONG, 1, &abort_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_ad->acquisition_in_progress = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_get_extended_status()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	int32_t busy;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_caget( &(epics_ad->detector_state_pv),
					MX_CA_LONG, 1, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
	} else {
		ad->status = 0;
	}

	if ( epics_ad->acquisition_in_progress ) {
		if ( busy == 0 ) {
			ad->last_frame_number = 0;
			ad->total_num_frames++;
		}
	} else {
		if ( busy ) {
			epics_ad->acquisition_in_progress = TRUE;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_readout_frame()";

	MX_EPICS_AREA_DETECTOR *epics_ad;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_ad_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_correct_frame()";

	MX_EPICS_AREA_DETECTOR *epics_ad;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	sp = &(ad->sequence_parameters);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_get_parameter()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	MX_SEQUENCE_PARAMETERS seq;
	int32_t x_binsize, y_binsize;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
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
	case MXLV_AD_BINSIZE:
		mx_status = mx_caget( &(epics_ad->binx_pv),
					MX_CA_LONG, 1, &x_binsize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_ad->biny_pv),
					MX_CA_LONG, 1, &y_binsize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->binsize[0] = x_binsize;
		ad->binsize[1] = y_binsize;

		if ( ad->binsize[0] > 0 ) {
			ad->framesize[0] =
				ad->maximum_framesize[0] / ad->binsize[0];
		}
		if ( ad->binsize[1] > 0 ) {
			ad->framesize[1] =
				ad->maximum_framesize[1] / ad->binsize[1];
		}
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
		case MXT_SQ_BULB:
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

#if MXD_EPICS_AREA_DETECTOR_DEBUG
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
mxd_epics_ad_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_set_parameter()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	unsigned long saved_value;
	double seconds;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
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
		if ( sp->sequence_type != MXT_SQ_ONE_SHOT ) {
			saved_value = sp->sequence_type;

			sp->sequence_type = MXT_SQ_ONE_SHOT;

			return mx_error( MXE_UNSUPPORTED, fname,
			"Sequence type %lu is not supported for "
			"EPICS Area Detector '%s'.  "
			"Only one-shot sequences are supported.",
				saved_value, ad->record->name );
		}
		break;
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		if ( sp->num_parameters != 1 ) {
			saved_value = sp->num_parameters;

			sp->num_parameters = 0;

			return mx_error( MXE_UNSUPPORTED, fname,
			"# sequence parameters = %lu is not supported for "
			"EPICS Area Detector '%s'.  "
			"Only a value of 1 is supported.",
				saved_value, ad->record->name );
		}
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		seconds = sp->parameter_array[0];

		mx_status = mx_caput( &(epics_ad->acquire_time_pv),
					MX_CA_DOUBLE, 1, &seconds );
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

#endif /* HAVE_EPICS */


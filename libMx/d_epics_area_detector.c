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
#include "mx_bit.h"
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
	mxd_epics_ad_get_last_frame_number,
	mxd_epics_ad_get_total_num_frames,
	mxd_epics_ad_get_status,
	NULL,
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

	mx_epics_pvname_init( &(epics_ad->acquire_pv),
			"%s%sAcquire",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->acquire_rbv_pv),
			"%s%sAcquire_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->acquire_period_pv),
			"%s%sAcquirePeriod",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->acquire_period_rbv_pv),
			"%s%sAcquirePeriod_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->acquire_time_pv),
			"%s%sAcquireTime",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->acquire_time_rbv_pv),
			"%s%sAcquireTime_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->array_counter_rbv_pv),
			"%s%sArrayCounter_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->array_data_pv),
			"%s%sArrayData",
			epics_ad->prefix_name, epics_ad->image_name );

	mx_epics_pvname_init( &(epics_ad->binx_pv),
			"%s%sBinX",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->binx_rbv_pv),
			"%s%sBinX_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->biny_pv),
			"%s%sBinY",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->biny_rbv_pv),
			"%s%sBinY_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->detector_state_pv),
			"%s%sDetectorState_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->image_mode_pv),
			"%s%sImageMode",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->image_mode_rbv_pv),
			"%s%sImageMode_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->num_images_pv),
			"%s%sNumImages",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->num_images_rbv_pv),
			"%s%sNumImages_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->num_images_counter_rbv_pv),
			"%s%sNumImagesCounter_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->trigger_mode_pv),
			"%s%sTriggerMode",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->trigger_mode_rbv_pv),
			"%s%sTriggerMode_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_ad_open()";

	MX_AREA_DETECTOR *ad;
	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	char pvname[ MXU_EPICS_PVNAME_LENGTH+1 ];
	unsigned long ad_flags, mask, max_size_value;
	char epics_string[41];
	int32_t data_type, color_mode;
	char *max_array_bytes_string;
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

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	epics_ad->acquisition_is_starting = FALSE;
	epics_ad->array_data_available = FALSE;
	epics_ad->max_array_bytes = 0;

	/* Detect the presence of the detector by asking it for
	 * its manufacturer name.
	 */

	snprintf( pvname, sizeof(pvname), "%s%sManufacturer_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: CCD manufacturer = '%s'", fname, epics_string));
#endif
	snprintf( pvname, sizeof(pvname), "%s%sModel_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: CCD model = '%s'", fname, epics_string));
#endif

	/* Set the default file format.
	 *
	 * FIXME - Are both of these necessary?
	 */

	ad->frame_file_format = MXT_IMAGE_FILE_SMV;
	ad->datafile_format = ad->frame_file_format;

	ad->correction_frames_to_skip = 0;

	/* Do we need automatic saving and/or loading of image frames by MX? */

	ad_flags = ad->area_detector_flags;

	mask = MXF_AD_LOAD_FRAME_AFTER_ACQUISITION
		| MXF_AD_SAVE_FRAME_AFTER_ACQUISITION;

	if ( ad_flags & mask ) {

#if MXD_EPICS_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
	  ("%s: Enabling automatic datafile management.  ad_flags & mask = %lx",
			fname, (ad_flags & mask) ));
#endif
		mx_status =
		    mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*---*/

	snprintf( pvname, sizeof(pvname), "%s%sDataType_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1, &data_type );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( pvname, sizeof(pvname), "%s%sColorMode_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1, &color_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: data_type = %ld, color_mode = %ld",
		fname, (long) data_type, (long) color_mode ));
#endif

	if ( color_mode != 0 ) {
		mx_warning(
		"EPICS Area Detector '%s' is using color mode %ld, "
		"which is not yet supported by the '%s' MX driver.",
			ad->record->name, (long) color_mode,
			mx_get_driver_name( ad->record ) );
	}

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
		ad->bits_per_pixel = 32;
		ad->bytes_per_pixel = 4;
		ad->image_format = MXT_IMAGE_FORMAT_FLOAT;
		break;

	case 7:				/* Float64 */
		ad->bits_per_pixel = 64;
		ad->bytes_per_pixel = 8;
		ad->image_format = MXT_IMAGE_FORMAT_DOUBLE;
		break;

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

	ad->correction_calc_format = ad->image_format;

	/* Get the maximum frame size from the EPICS server. */

	snprintf( pvname, sizeof(pvname), "%s%sMaxSizeX_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1, &max_size_value );
	
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->maximum_framesize[0] = max_size_value;

	snprintf( pvname, sizeof(pvname), "%s%sMaxSizeY_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1, &max_size_value );
	
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->maximum_framesize[1] = max_size_value;

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

	/* Initialize area detector parameters. */

	ad->byte_order = (long) mx_native_byteorder();
	ad->header_length = 0;

	mx_status = mx_area_detector_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Does the area detector support reading out the image data? */

	if ( strlen( epics_ad->image_name ) == 0 ) {
		epics_ad->array_data_available = FALSE;
	} else {
		/* Begin by looking for the $(P)$(R)ArrayData PV. */

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
	}

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: array_data_available = %d",
		fname, epics_ad->array_data_available));
#endif

	/* Figure out the maximum number of array data elements that can
	 * be transferred.
	 */

	max_array_bytes_string = getenv( "EPICS_CA_MAX_ARRAY_BYTES" );

	if ( max_array_bytes_string == NULL ) {
		epics_ad->max_array_bytes = 16000;
	} else {
		epics_ad->max_array_bytes = atol( max_array_bytes_string );
	}

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

	epics_ad->acquisition_is_starting = TRUE;

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
	int32_t acquire_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	acquire_value = 0;

	mx_status = mx_caput( &(epics_ad->acquire_pv),
					MX_CA_LONG, 1, &acquire_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_ad->acquisition_is_starting = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_ad_get_last_frame_number( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_get_last_frame_number()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	int32_t num_images;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_caget( &(epics_ad->num_images_counter_rbv_pv),
				MX_CA_LONG, 1, &num_images );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->last_frame_number = num_images - 1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_get_total_num_frames( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_get_total_num_frames()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	int32_t array_counter;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_caget( &(epics_ad->array_counter_rbv_pv),
				MX_CA_LONG, 1, &array_counter );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->total_num_frames = array_counter;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_get_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_get_status()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	int32_t detector_state, acquire_rbv;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Get details of the detector state from the DetectorState_RBV PV. */

	mx_status = mx_caget( &(epics_ad->detector_state_pv),
					MX_CA_LONG, 1, &detector_state );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( detector_state ) {
	case 0:			/* Idle */
		ad->status = 0;
		break;
	case 1:			/* Acquire */
		ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
		break;
	case 2:			/* Readout */
	case 4:			/* Saving */
	case 5:			/* Aborting */
	case 7:			/* Waiting */
	case 8:			/* Initializing */
		ad->status = MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
		break;
	case 3:			/* Correct */
		ad->status = MXSF_AD_CORRECTION_IN_PROGRESS;
		break;
	case 6:			/* Error */
		ad->status = MXSF_AD_ERROR;
		break;
	default:
		ad->status = MXSF_AD_ERROR;

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unexpected detector state %ld returned by "
		"EPICS Area Detector '%s'.",
			(long) detector_state,
			ad->record->name );
		break;
	}

	/* The Acquire_RBV PV must also be checked since it is possible for
	 * the detector state variable to return 0, even though an imaging
	 * sequence is in process.
	 */

	mx_status = mx_caget( &(epics_ad->acquire_rbv_pv),
				MX_CA_LONG, 1, &acquire_rbv );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( acquire_rbv != 0 ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

	/* Just after an acquisition sequence has started, the status flags
	 * from the EPICS Area Detector system may still be 0 for a while.
	 * The acquisition_is_starting flag allows us to check for this
	 * possibility and turns on the MXSF_AD_ACQUISITION_IN_PROGRESS
	 * bit during this time interval.  After the area detector system
	 * starts returning non-zero status values, then we turn off our
	 * local acquisition_is_starting flag.
	 */

	if ( epics_ad->acquisition_is_starting ) {
		if ( ad->status & MXSF_AD_IS_BUSY ) {
			epics_ad->acquisition_is_starting = FALSE;
		} else
		if ( ad->status & MXSF_AD_ERROR ) {
			epics_ad->acquisition_is_starting = FALSE;
		} else {
			ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
		}
	}

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: acquisition_is_starting = %ld",
		fname, (long) epics_ad->acquisition_is_starting ));
	MX_DEBUG(-2,("%s: detector '%s' status = %#lx",
		fname, ad->record->name, (unsigned long) ad->status ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_readout_frame()";

	MX_EPICS_AREA_DETECTOR *epics_ad;
	long epics_data_type;
	unsigned long num_array_elements;
	void *image_data;
	double acquisition_time;
	struct timespec acquisition_timespec;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	/* If we do not have direct access to the array data, then
	 * there is nothing for us to do here.
	 */

	if ( epics_ad->array_data_available == FALSE ) {

#if MXD_EPICS_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: image data is not available.", fname));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* How big is the frame? */

	ad->parameter_type = MXLV_AD_BYTES_PER_FRAME;

	mx_status = mxd_epics_ad_get_parameter( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is EPICS configured so that we can read this frame out? */

	if ( ad->bytes_per_frame > epics_ad->max_array_bytes ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Cannot readout the image frame since the image size "
		"of %ld bytes for EPICS Area Detector '%s' is larger than "
		"the maximum size (%ld bytes) configured for EPICS.  "
		"To fix this, you must set the environment "
		"variable EPICS_CA_MAX_ARRAY_BYTES to %ld and then "
		"restart your MX client.",  
			ad->bytes_per_frame,
			ad->record->name,
			epics_ad->max_array_bytes,
			ad->bytes_per_frame );
	}

	/* Yes, we can read the frame.  Make sure that the local image frame
	 * data structure is the right size to read the frame into.
	 */

	num_array_elements = ad->framesize[0] * ad->framesize[1];

	mx_status = mx_image_alloc( &(ad->image_frame),
				ad->framesize[0],
				ad->framesize[1],
				ad->image_format,
				ad->byte_order,
				ad->bytes_per_pixel,
				ad->header_length,
				ad->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	image_data = ad->image_frame->image_data;

	/* Get the EPICS data type corresponding to the MX image format. */

	switch( ad->image_format ) {
	case MXT_IMAGE_FORMAT_GREY8:
		epics_data_type = MX_CA_CHAR;
		break;

	case MXT_IMAGE_FORMAT_GREY16:
		epics_data_type = MX_CA_SHORT;
		break;

	case MXT_IMAGE_FORMAT_GREY32:
		epics_data_type = MX_CA_LONG;
		break;

	case MXT_IMAGE_FORMAT_FLOAT:
		epics_data_type = MX_CA_FLOAT;
		break;

	case MXT_IMAGE_FORMAT_DOUBLE:
		epics_data_type = MX_CA_DOUBLE;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported MX image format %ld requested for "
		"EPICS Area Detector '%s'.",
			ad->image_format, ad->record->name );
		break;
	}

	/* Readout the frame. */

	mx_status = mx_caget( &(epics_ad->array_data_pv),
				epics_data_type,
				num_array_elements,
				image_data );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the binsize in the header.
	 * 
	 * Our earlier read of MXLV_AD_BYTES_PER_FRAME will have set
	 * the binsize correctly as a side effect.
	 */

	MXIF_ROW_BINSIZE(ad->image_frame)    = ad->binsize[0];
	MXIF_COLUMN_BINSIZE(ad->image_frame) = ad->binsize[1];

	/* Set the exposure time in the header. */

	mx_status = mx_caget( &(epics_ad->acquire_time_rbv_pv),
				MX_CA_DOUBLE, 1, &acquisition_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	acquisition_timespec =
		mx_convert_seconds_to_high_resolution_time( acquisition_time );

	MXIF_EXPOSURE_TIME_SEC(ad->image_frame)  = acquisition_timespec.tv_sec;
	MXIF_EXPOSURE_TIME_NSEC(ad->image_frame) = acquisition_timespec.tv_nsec;

	/* Set the timestamp in the header. */

	MXIF_TIMESTAMP_SEC(ad->image_frame)  = 0;
	MXIF_TIMESTAMP_NSEC(ad->image_frame) = 0;

	/* We do not know what the area detector bias is, so set it to 0. */

	MXIF_BIAS_OFFSET_MILLI_ADUS(ad->image_frame) = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_correct_frame()";

	MX_EPICS_AREA_DETECTOR *epics_ad;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* The EPICS Area Detector record will already have done the
	 * image correction, so there is nothing for us to do here.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_get_parameter()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	int32_t x_binsize, y_binsize, trigger_mode, image_mode, num_images;
	double acquire_time, acquire_period;
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
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		break;

	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
	case MXLV_AD_BYTES_PER_FRAME:
		mx_status = mx_caget( &(epics_ad->binx_rbv_pv),
					MX_CA_LONG, 1, &x_binsize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_ad->biny_rbv_pv),
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

		ad->bytes_per_frame = mx_round( ad->bytes_per_pixel
			* ad->framesize[0] * ad->framesize[1] );
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_image_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		break;

	case MXLV_AD_DETECTOR_READOUT_TIME:
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		mx_status = mx_caget( &(epics_ad->image_mode_rbv_pv),
					MX_CA_LONG, 1, &image_mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( image_mode ) {
		case 0:			/* Single */
			sp->sequence_type = MXT_SQ_ONE_SHOT;
			sp->num_parameters = 1;
			break;
		case 1:			/* Multiple */
			sp->sequence_type = MXT_SQ_MULTIFRAME;
			sp->num_parameters = 3;
			break;
		case 2:			/* Continuous */
			sp->sequence_type = MXT_SQ_CONTINUOUS;
			sp->num_parameters = 1;
			break;
		default:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Unrecognized image mode %ld returned for "
			"EPICS Area Detector '%s'",
				(long) image_mode, ad->record->name );
			break;
		}
		break;
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		switch( sp->sequence_type ) {
		case MXT_SQ_MULTIFRAME:
			mx_status = mx_caget(
					&(epics_ad->acquire_period_rbv_pv),
					MX_CA_DOUBLE, 1, &acquire_period );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_caget( &(epics_ad->num_images_rbv_pv),
					MX_CA_LONG, 1, &num_images );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Fall through to the next case. */

		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_CONTINUOUS:
			mx_status = mx_caget( &(epics_ad->acquire_time_rbv_pv),
					MX_CA_DOUBLE, 1, &acquire_time );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
				sp->parameter_array[0] = num_images;
				sp->parameter_array[1] = acquire_time;
				sp->parameter_array[2] =
					acquire_time + acquire_period;
			} else {
				sp->parameter_array[0] = acquire_time;
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Sequence type %ld is not supported for "
			"EPICS Area Detector '%s'.",
				sp->sequence_type, ad->record->name );
			break;
		}
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_caget( &(epics_ad->trigger_mode_rbv_pv),
					MX_CA_LONG, 1, &trigger_mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( trigger_mode ) {
		case 0:			/* Internal */
			ad->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;
			break;
		case 1:			/* External */
			ad->trigger_mode = MXT_IMAGE_EXTERNAL_TRIGGER;
			break;
		default:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Unrecognized trigger mode %ld returned for "
			"EPICS Area Detector '%s'.",
				(long) trigger_mode, ad->record->name );
			break;
		}
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
	double acquire_time, acquire_period;
	int32_t trigger_mode, image_mode, num_images;
	int32_t x_binsize, y_binsize;
	double raw_x_binsize, raw_y_binsize;
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
		if ( ad->parameter_type == MXLV_AD_FRAMESIZE ) {
			raw_x_binsize =
				mx_divide_safely( ad->maximum_framesize[0],
							ad->framesize[0] );
			raw_y_binsize =
				mx_divide_safely( ad->maximum_framesize[1],
							ad->framesize[1] );

			x_binsize = mx_round( raw_x_binsize );
			y_binsize = mx_round( raw_y_binsize );
		} else {
			x_binsize = ad->binsize[0];
			y_binsize = ad->binsize[1];
		}

		mx_status = mx_caput( &(epics_ad->binx_pv),
				MX_CA_LONG, 1, &x_binsize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caput( &(epics_ad->biny_pv),
				MX_CA_LONG, 1, &y_binsize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_AD_SEQUENCE_TYPE:
		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			image_mode = 0;
			break;
		case MXT_SQ_CONTINUOUS:
			image_mode = 2;
			break;
		case MXT_SQ_MULTIFRAME:
			image_mode = 1;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Area detector sequence mode %ld is not supported for "
			"EPICS Area Detector '%s'.",
				(long) image_mode, ad->record->name );
			break;
		}

		mx_status = mx_caput( &(epics_ad->image_mode_pv),
				MX_CA_LONG, 1, &image_mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
			acquire_time = sp->parameter_array[1];
		} else {
			acquire_time = sp->parameter_array[0];
		}

		mx_status = mx_caput( &(epics_ad->acquire_time_pv),
					MX_CA_DOUBLE, 1, &acquire_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
			num_images = sp->parameter_array[0];
		} else {
			num_images = 1;
		}

		mx_status = mx_caput( &(epics_ad->num_images_pv),
					MX_CA_LONG, 1, &num_images );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
			acquire_period = sp->parameter_array[2] - acquire_time;

			mx_status = mx_caput( &(epics_ad->acquire_period_pv),
					MX_CA_DOUBLE, 1, &acquire_period );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break; 

	case MXLV_AD_TRIGGER_MODE:
		switch( ad->trigger_mode ) {
		case MXT_IMAGE_INTERNAL_TRIGGER:
			trigger_mode = 0;
			break;
		case MXT_IMAGE_EXTERNAL_TRIGGER:
			trigger_mode = 1;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported trigger mode %ld requested for "
			"EPICS Area Detector '%s'.",
				(long) trigger_mode, ad->record->name );
		}

		mx_status = mx_caput( &(epics_ad->trigger_mode_pv),
					MX_CA_LONG, 1, &trigger_mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
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


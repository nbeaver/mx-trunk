/*
 * Name:    d_epics_area_detector.c
 *
 * Purpose: MX driver for the EPICS Area Detector record.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011, 2013-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EPICS_AREA_DETECTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

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
	mxd_epics_ad_arm,
	mxd_epics_ad_trigger,
	NULL,
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

	epics_ad->epics_roi_array = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_epics_ad_finish_record_initialization()";

	MX_AREA_DETECTOR *ad;
	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	MX_EPICS_AREA_DETECTOR_ROI *epics_ad_roi = NULL;
	long i;
	mx_status_type mx_status;

	mx_status = mx_area_detector_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

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

	mx_epics_pvname_init( &(epics_ad->array_callbacks_pv),
			"%s%sArrayCallbacks",
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

	mx_epics_pvname_init( &(epics_ad->file_format_pv),
			"%s%sFileFormat",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->file_format_rbv_pv),
			"%s%sFileFormat_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->file_name_pv),
			"%s%sFileName",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->file_name_rbv_pv),
			"%s%sFileName_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->file_number_pv),
			"%s%sFileNumber",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->file_number_rbv_pv),
			"%s%sFileNumber_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->file_path_pv),
			"%s%sFilePath",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->file_path_rbv_pv),
			"%s%sFilePath_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->file_path_exists_rbv_pv),
			"%s%sFilePathExists_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->file_template_pv),
			"%s%sFileTemplate",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->file_template_rbv_pv),
			"%s%sFileTemplate_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->full_file_name_rbv_pv),
			"%s%sFullFileName_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->image_mode_pv),
			"%s%sImageMode",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->image_mode_rbv_pv),
			"%s%sImageMode_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->num_acquisitions_pv),
			"%s%sNumAcquisitions",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->num_acquisitions_rbv_pv),
			"%s%sNumAcquisitions_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_epics_pvname_init( &(epics_ad->num_acquisitions_counter_rbv_pv),
			"%s%sNumAcquisitionsCounter_RBV",
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

	/* If configured, set up the EPICS-specific ROI data structures. */

	if ( ad->maximum_num_rois > 0 ) {
		epics_ad->epics_roi_array = (MX_EPICS_AREA_DETECTOR_ROI *)
			malloc( ad->maximum_num_rois
				* sizeof(MX_EPICS_AREA_DETECTOR_ROI) );

		if ( epics_ad->epics_roi_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %lu element "
			"array of MX_EPICS_AREA_DETECTOR_ROI structures.",
				ad->maximum_num_rois );
		}
	}

	for ( i = 0; i < ad->maximum_num_rois; i++ ) {
		epics_ad_roi = &(epics_ad->epics_roi_array[i]);

		mx_epics_pvname_init( &(epics_ad_roi->nd_array_port_pv),
			"%sROI%d:NDArrayPort", epics_ad->prefix_name, i+1 );

		mx_epics_pvname_init( &(epics_ad_roi->nd_array_port_rbv_pv),
			"%sROI%d:NDArrayPort_RBV", epics_ad->prefix_name, i+1 );

		mx_epics_pvname_init( &(epics_ad_roi->min_x_pv),
			"%sROI%d:MinX", epics_ad->prefix_name, i+1 );

		mx_epics_pvname_init( &(epics_ad_roi->min_x_rbv_pv),
			"%sROI%d:MinX_RBV", epics_ad->prefix_name, i+1 );

		mx_epics_pvname_init( &(epics_ad_roi->size_x_pv),
			"%sROI%d:SizeX", epics_ad->prefix_name, i+1 );

		mx_epics_pvname_init( &(epics_ad_roi->size_x_rbv_pv),
			"%sROI%d:SizeX_RBV", epics_ad->prefix_name, i+1 );

		mx_epics_pvname_init( &(epics_ad_roi->min_y_pv),
			"%sROI%d:MinY", epics_ad->prefix_name, i+1 );

		mx_epics_pvname_init( &(epics_ad_roi->min_y_rbv_pv),
			"%sROI%d:MinY_RBV", epics_ad->prefix_name, i+1 );

		mx_epics_pvname_init( &(epics_ad_roi->size_y_pv),
			"%sROI%d:SizeY", epics_ad->prefix_name, i+1 );

		mx_epics_pvname_init( &(epics_ad_roi->size_y_rbv_pv),
			"%sROI%d:SizeY_RBV", epics_ad->prefix_name, i+1 );

	};

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_ad_open()";

	MX_AREA_DETECTOR *ad;
	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	char pvname[ MXU_EPICS_PVNAME_LENGTH+1 ];
	unsigned long ad_flags, mask, epics_ad_flags;
	int32_t epics_datatype, epics_colormode;
	int32_t num_exposures, num_images;
	uint32_t max_size_value;
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

	ad->image_data_available = FALSE;

	epics_ad->num_images_counter_is_implemented = FALSE;
	epics_ad->old_total_num_frames = 0;

	epics_ad->acquisition_is_starting = FALSE;
	epics_ad->max_array_bytes = 0;

	/* Detect the presence of the detector by asking it for
	 * its manufacturer name.
	 */

	snprintf( pvname, sizeof(pvname), "%s%sManufacturer_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caget_by_name( pvname,
			MX_CA_STRING, 1, epics_ad->manufacturer_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Detector manufacturer = '%s'",
			fname, epics_ad->manufacturer_name));
#endif
	snprintf( pvname, sizeof(pvname), "%s%sModel_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caget_by_name( pvname,
			MX_CA_STRING, 1, epics_ad->model_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Detector model = '%s'", fname, epics_ad->model_name));
#endif

	/* Set the default file formats. */

	ad->datafile_load_format   = MXT_IMAGE_FILE_SMV;
	ad->datafile_save_format   = MXT_IMAGE_FILE_SMV;
	ad->correction_load_format = MXT_IMAGE_FILE_SMV;
	ad->correction_save_format = MXT_IMAGE_FILE_SMV;

	ad->correction_frames_to_skip = 0;

	/* Do we need automatic saving and/or readout of image frames by MX? */

	ad_flags = ad->area_detector_flags;

	mask = MXF_AD_READOUT_FRAME_AFTER_ACQUISITION
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

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1, &epics_datatype );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( pvname, sizeof(pvname), "%s%sColorMode_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1, &epics_colormode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: data_type = %ld, color_mode = %ld",
		fname, (long) data_type, (long) color_mode ));
#endif

	if ( epics_colormode != 0 ) {
		mx_warning(
		"EPICS Area Detector '%s' is using color mode %ld, "
		"which is not yet supported by the '%s' MX driver.",
			ad->record->name, (long) epics_colormode,
			mx_get_driver_name( ad->record ) );
	}

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: bits per pixel = %ld", fname, ad->bits_per_pixel))
#endif

	epics_ad_flags = epics_ad->epics_area_detector_flags;

	if ( ( epics_ad_flags & MXF_EPICS_AD_USE_DATATYPE_PV ) == 0 ) {
		char *epics_datatype_name;

		/* If requested, we override the datatype reported by EPICS. */

		epics_datatype_name = epics_ad->epics_datatype_name;

		if ( mx_strcasecmp( epics_datatype_name, "int8" ) == 0 ) {
			epics_datatype = 0;
		} else
		if ( mx_strcasecmp( epics_datatype_name, "uint8" ) == 0 ) {
			epics_datatype = 1;
		} else
		if ( mx_strcasecmp( epics_datatype_name, "int16" ) == 0 ) {
			epics_datatype = 2;
		} else
		if ( mx_strcasecmp( epics_datatype_name, "uint16" ) == 0 ) {
			epics_datatype = 3;
		} else
		if ( mx_strcasecmp( epics_datatype_name, "int32" ) == 0 ) {
			epics_datatype = 4;
		} else
		if ( mx_strcasecmp( epics_datatype_name, "uint32" ) == 0 ) {
			epics_datatype = 5;
		} else
		if ( mx_strcasecmp( epics_datatype_name, "float32" ) == 0 ) {
			epics_datatype = 6;
		} else
		if ( mx_strcasecmp( epics_datatype_name, "float64" ) == 0 ) {
			epics_datatype = 7;
		} else {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"The specified EPICS datatype '%s' for area detector "
			"'%s' is not valid.  The valid datatypes are "
			"Int8, UInt8, Int16, UInt16, Int32, UInt32, "
			"Float32, and Float64.",
				epics_datatype_name, record->name );
		}
	}

	epics_ad->epics_datatype = epics_datatype;

	switch( epics_datatype ) {
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
			(long) epics_datatype, record->name );
		
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

	/* What asyn port does this detector use? */

	snprintf( pvname, sizeof(pvname), "%s%sPortName_RBV",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_STRING, 1,
					&(epics_ad->asyn_port_name) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Does this version use the NumAcquisitionsCounter_RBV PV?
	 */

	mx_status = mx_epics_pv_connect(
			&(epics_ad->num_acquisitions_counter_rbv_pv),
			MXF_EPVC_QUIET | MXF_EPVC_WAIT_FOR_CONNECTION );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		epics_ad->use_num_acquisitions = TRUE;
		break;
	case MXE_TIMED_OUT:
		epics_ad->use_num_acquisitions = FALSE;
		break;
	default:
		epics_ad->use_num_acquisitions = FALSE;

		return mx_error( mx_status.code,
			mx_status.location,
			"%s", mx_status.message );
		break;
	}

#if 1
	MX_DEBUG(-2,("%s: '%s' use_num_acquisitions = %d",
		fname, record->name, epics_ad->use_num_acquisitions));
#endif

	/* Set all of the lower level counter PVs to ratios of 1. */

	num_exposures = 1;

	snprintf( pvname, sizeof(pvname), "%s%sNumExposures",
			epics_ad->prefix_name, epics_ad->camera_name );

	mx_status = mx_caput_by_name( pvname, MX_CA_LONG, 1, &num_exposures );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( epics_ad->use_num_acquisitions ) {
		num_images = 1;

		mx_status = mx_caput( &(epics_ad->num_images_pv),
					MX_CA_LONG, 1, &num_images );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Does the area detector support reading out the image data? */

	if ( strlen( epics_ad->image_name ) == 0 ) {
		ad->image_data_available = FALSE;
	} else {
		/* Look for the $(P)$(R)ArrayData PV. */

		mx_status = mx_epics_pv_connect( &(epics_ad->array_data_pv),
			MXF_EPVC_QUIET | MXF_EPVC_WAIT_FOR_CONNECTION );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			ad->image_data_available = TRUE;
			break;
		case MXE_TIMED_OUT:
			ad->image_data_available = FALSE;
			break;
		default:
			ad->image_data_available = FALSE;

			return mx_error( mx_status.code,
					mx_status.location,
					"%s", mx_status.message );
			break;
		}
	}

#if 1 || MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: ad->image_data_avaliable = %d",
		fname, ad->image_data_available));
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

	/* Initialize the raw image frame-related variables to invalid
	 * values of 0 or NULL, since we may not need them.  If these
	 * variables _are_ needed, then they will be initialized in the
	 * readout_frame() method the first time that it is called.
	 */

	epics_ad->raw_data_type = 0;
	epics_ad->raw_frame_bytes = 0;
	epics_ad->raw_frame_array = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_arm()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	int32_t enable_array_callbacks;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	if ( epics_ad->num_images_counter_is_implemented == FALSE ) {

		/* If needed, save a snapshot of the value of total_num_frames
		 * so that it can be used later to compute the value of
		 * last_frame_number.  This kludge will not work if this
		 * detector sequence was not started by MX.
		 */

		mx_status = mx_area_detector_get_trigger_mode(
							ad->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (ad->trigger_mode) & MXT_IMAGE_EXTERNAL_TRIGGER ) {
			mx_status = mx_area_detector_get_total_num_frames(
							ad->record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			epics_ad->old_total_num_frames = ad->total_num_frames;
		}
	}

	/* Fully correct behavior of the MX drivers requires that the
	 * EPICS array callbacks be turned on.  Nevertheless, we do
	 * provide a way to turn this off.  If you turn it off, then
	 * the MX 'total_num_frames' field will not return valid values,
	 * and some parts of MX that depend on 'total_num_frames', will
	 * not work correctly.
	 */

	flags = epics_ad->epics_area_detector_flags;

	if ( flags & MXF_EPICS_AD_DO_NOT_ENABLE_ARRAY_CALLBACKS ) {
		/* Return without enabling array callbacks, even though
		 * this is a bad idea.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	/* Turn on array callbacks. */

	enable_array_callbacks = TRUE;

	mx_status = mx_caput( &(epics_ad->array_callbacks_pv),
				MX_CA_LONG, 1, &enable_array_callbacks );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s complete for area detector '%s'",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ad_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_epics_ad_trigger()";

	MX_EPICS_AREA_DETECTOR *epics_ad = NULL;
	int32_t acquire;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	if ( epics_ad->num_images_counter_is_implemented == FALSE ) {

		/* If needed, save a snapshot of the value of total_num_frames
		 * so that it can be used later to compute the value of
		 * last_frame_number.  This kludge will not work if this
		 * detector sequence was not started by MX.
		 */

		mx_status = mx_area_detector_get_trigger_mode(
							ad->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (ad->trigger_mode) & MXT_IMAGE_INTERNAL_TRIGGER ) {
			mx_status = mx_area_detector_get_total_num_frames(
							ad->record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			epics_ad->old_total_num_frames = ad->total_num_frames;
		}
	}

	/* Manually start the acquisition. */

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
	MX_EPICS_PV *next_frame_pv = NULL;
	int32_t next_frame_number;
	mx_status_type mx_status;

	mx_status = mxd_epics_ad_get_pointers( ad, &epics_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	if ( epics_ad->num_images_counter_is_implemented ) {
		if ( epics_ad->use_num_acquisitions ) {
			next_frame_pv =
				&(epics_ad->num_acquisitions_counter_rbv_pv);
		} else {
			next_frame_pv = &(epics_ad->num_images_counter_rbv_pv);
		}

		mx_status = mx_caget( next_frame_pv,
				MX_CA_LONG, 1, &next_frame_number );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		/* The following kludge only does the right thing if
		 * the currently running acquisition was started by MX.
		 */

		mx_status = mx_area_detector_get_total_num_frames(
							ad->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		next_frame_number =
	    (int32_t)( ad->total_num_frames - epics_ad->old_total_num_frames );

	}

	ad->last_frame_number = next_frame_number - 1;

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

	if ( ad->image_data_available == FALSE ) {

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
				ad->bytes_per_frame,
				ad->dictionary,
				ad->record );

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
	MX_EPICS_AREA_DETECTOR_ROI *epics_ad_roi = NULL;
	MX_EPICS_PV *num_frames_pv = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	int32_t x_binsize, y_binsize, trigger_mode, image_mode, num_frames;
	int32_t x_start, x_size, y_start, y_size;
	uint32_t next_datafile_number;
	double acquire_time, acquire_period;
	char filename_prefix[MXU_FILENAME_LENGTH+1];
	char filename_template[MXU_FILENAME_LENGTH+1];
	char *ptr;
	unsigned long i, num_digits;
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

		if ( x_binsize == 0 ) {
			x_binsize = 1;
		}
		if ( y_binsize == 0 ) {
			y_binsize = 1;
		}

		ad->binsize[0] = x_binsize;
		ad->binsize[1] = y_binsize;

		ad->framesize[0] = ad->maximum_framesize[0] / ad->binsize[0];
		ad->framesize[1] = ad->maximum_framesize[1] / ad->binsize[1];

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

	case MXLV_AD_DATAFILE_DIRECTORY:
		mx_status = mx_caget( &(epics_ad->file_path_rbv_pv),
					MX_CA_CHAR, MXU_FILENAME_LENGTH,
					ad->datafile_directory );
		break;

	case MXLV_AD_DATAFILE_NAME:
		mx_status = mx_caget( &(epics_ad->full_file_name_rbv_pv),
					MX_CA_CHAR, MXU_FILENAME_LENGTH,
					ad->datafile_name );
		break;

	case MXLV_AD_DATAFILE_NUMBER:
		mx_status = mx_caget( &(epics_ad->file_number_rbv_pv),
					MX_CA_LONG, 1, &next_datafile_number );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->datafile_number = next_datafile_number - 1;
		break;

	case MXLV_AD_DATAFILE_PATTERN:
		ad->datafile_pattern[0] = '\0';

		mx_status = mx_caget( &(epics_ad->file_name_rbv_pv),
					MX_CA_CHAR, MXU_FILENAME_LENGTH,
					filename_prefix );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: filename_prefix = '%s'",
			fname, filename_prefix));
#endif

		mx_status = mx_caget( &(epics_ad->file_template_rbv_pv),
					MX_CA_CHAR, MXU_FILENAME_LENGTH,
					filename_template );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_EPICS_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: filename_template = '%s'",
			fname, filename_template));
#endif
		/* Here we attempt to turn an EPICS filename template
		 * into an MX filename pattern.  The current implementation
		 * is extremely fragile and kludgy.
		 */

		/* FIXME - This is an ugly, ugly kludge.
		 * 
		 * We _assume_ that the first 5 characters of the
		 * EPICS filename template are %s%s%
		 */

		if ( strncmp( filename_template, "%s%s%", 5 ) != 0 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The filename template '%s' currently used by "
			"area detector '%s' is not compatible with "
			"MX datafile pattern strings.  Try setting a "
			"datafile pattern from MX to fix this.",
				filename_template,
				ad->record->name );
		}

		ptr = filename_template + 5;

		num_digits = atol( ptr );

		/* FIXME - Ugly kludge, the Next Generation 
		 *
		 * The current string is _assumed_ to be part of a 'd' format
		 * printf element.  We look for the character after this
		 * hypothetical 'd' character for the start of the filename
		 * trailer.
		 */

		ptr = strchr( ptr, 'd' );

		if ( ptr == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The filename template '%s' currently used by "
			"area detector '%s' is not compatible with "
			"MX datafile pattern strings.  Try setting a "
			"datafile pattern from MX to fix this.",
				filename_template,
				ad->record->name );
		}

		ptr++;

		strlcpy( ad->datafile_pattern, filename_prefix,
			sizeof( ad->datafile_pattern ) );

		for ( i = 0; i < num_digits; i++ ) {
			strlcat( ad->datafile_pattern, "#",
				sizeof( ad->datafile_pattern ) );
		}

		strlcat( ad->datafile_pattern, ptr,
			sizeof( ad->datafile_pattern ) );

		/* FIXME - And now the horror is over ... or is it? */
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
			sp->sequence_type = MXT_SQ_STREAM;
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

		if ( epics_ad->use_num_acquisitions ) {
			num_frames_pv = &(epics_ad->num_acquisitions_rbv_pv);
		} else {
			num_frames_pv = &(epics_ad->num_images_rbv_pv);
		}

		switch( sp->sequence_type ) {
		case MXT_SQ_MULTIFRAME:
			mx_status = mx_caget(
					&(epics_ad->acquire_period_rbv_pv),
					MX_CA_DOUBLE, 1, &acquire_period );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_caget( num_frames_pv,
					MX_CA_LONG, 1, &num_frames );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Fall through to the next case. */

		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_STREAM:
			mx_status = mx_caget( &(epics_ad->acquire_time_rbv_pv),
					MX_CA_DOUBLE, 1, &acquire_time );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
				sp->parameter_array[0] = num_frames;
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

	case MXLV_AD_ROI:
		epics_ad_roi = &(epics_ad->epics_roi_array[ ad->roi_number ]);

		/* First, see if the ROI is assigned to this camera. */

		mx_status = mx_caget( &(epics_ad_roi->nd_array_port_rbv_pv),
			MX_CA_STRING, 1, epics_ad_roi->array_port_name );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( strcmp( epics_ad_roi->array_port_name,
				epics_ad->asyn_port_name ) != 0 )
		{
			/* If the ROI is not assigned to this camera, then
			 * set the ROI boundaries to (0,0) and return.
			 */

			ad->roi[0] = 0;
			ad->roi[1] = 0;
			ad->roi[2] = 0;
			ad->roi[3] = 0;

			return MX_SUCCESSFUL_RESULT;
		}

		/* Get the ROI boundaries. */

		mx_status = mx_caget( &(epics_ad_roi->min_x_rbv_pv),
					MX_CA_LONG, 1, &x_start );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_ad_roi->size_x_rbv_pv),
					MX_CA_LONG, 1, &x_size );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->roi[0] = x_start;
		ad->roi[1] = x_start + x_size;

		mx_status = mx_caget( &(epics_ad_roi->min_y_rbv_pv),
					MX_CA_LONG, 1, &y_start );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_ad_roi->size_y_rbv_pv),
					MX_CA_LONG, 1, &y_size );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->roi[2] = y_start;
		ad->roi[3] = y_start + y_size;

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
	MX_EPICS_AREA_DETECTOR_ROI *epics_ad_roi = NULL;
	MX_EPICS_PV *num_frames_pv = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double acquire_time, acquire_period;
	int32_t trigger_mode, image_mode, num_frames;
	int32_t x_binsize, y_binsize;
	long x_start, x_size, y_start, y_size;
	double raw_x_binsize, raw_y_binsize;
	char *ptr, *hash_ptr, *suffix_ptr;
	char filename_prefix[MXU_FILENAME_LENGTH+1];
	char filename_pattern[MXU_FILENAME_LENGTH+1];
	unsigned long num_prefix_chars, num_hash_chars;
	uint32_t file_path_exists, next_file_number, next_datafile_number;
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

	case MXLV_AD_DATAFILE_DIRECTORY:
		mx_status = mx_caput( &(epics_ad->file_path_pv),
					MX_CA_CHAR, MXU_FILENAME_LENGTH,
					ad->datafile_directory );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Reset the EPICS next file number to 0. */

		next_file_number = 0;

		mx_status = mx_caput( &(epics_ad->file_number_pv),
					MX_CA_LONG, 1, &next_file_number );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Check to see if the specified directory actually exists. */

		mx_status = mx_caget( &(epics_ad->file_path_exists_rbv_pv),
					MX_CA_LONG, 1, &file_path_exists );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( file_path_exists ) {
		case 1: /* The file path _does_ exist. */

			return MX_SUCCESSFUL_RESULT;
			break;

		case 0: /* The file path does _not_ exist. */

			return mx_error( MXE_NOT_FOUND, fname,
			"The datafile directory '%s' requested for "
			"area detector '%s' does not exist on the "
			"detector computer or is too long.",
				ad->datafile_directory,
				ad->record->name );
			break;

		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"An unexpected value %lu was returned for "
			"EPICS PV '%s' used by area detector '%s'.",
				(unsigned long) file_path_exists,
				epics_ad->file_path_exists_rbv_pv.pvname,
				ad->record->name );
			break;
		}
		break;

	case MXLV_AD_DATAFILE_NUMBER:
		next_datafile_number = ad->datafile_number + 1;

		mx_status = mx_caput( &(epics_ad->file_number_pv),
					MX_CA_LONG, 1, &next_datafile_number );
		break;

	case MXLV_AD_DATAFILE_PATTERN:
		/* Convert an MX datafile pattern like mytest####.img
		 * into an EPICS area detector file template and name
		 * such as name = 'mytest' and template = '%s%s%4.4d.img'.
		 */

		/* Look for the first # character in the MX template. */

		hash_ptr = strchr( ad->datafile_pattern, '#' );

		if ( hash_ptr == NULL ) {
			/* If there are no # characters in the pattern,
			 * then we set the filename PV to the entire
			 * contents of the datafile_pattern string
			 * and set the filename template PV to '%s%s'
			 * since the filename is now a fixed string.
			 */

			strlcpy( filename_prefix, ad->datafile_pattern,
				sizeof( filename_prefix ) );

			strlcpy( filename_pattern, "%s%s",
				sizeof( filename_pattern ) );
		} else {
			num_prefix_chars = hash_ptr - (ad->datafile_pattern);

			num_hash_chars = 0;

			ptr = hash_ptr;

			while (TRUE) {
				if ( *ptr == '#' ) {
					num_hash_chars++;
					ptr++;
				} else {
					break;
				}
			}

			suffix_ptr = ptr;

			strlcpy( filename_prefix, ad->datafile_pattern,
					num_prefix_chars + 1 );

			snprintf( filename_pattern, sizeof(filename_pattern),
				"%%s%%s%%%lu.%lud%s",
				num_hash_chars, num_hash_chars, suffix_ptr );
		}

		mx_status = mx_caput( &(epics_ad->file_name_pv),
					MX_CA_CHAR, MXU_FILENAME_LENGTH,
					filename_prefix );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caput( &(epics_ad->file_template_pv),
					MX_CA_CHAR, MXU_FILENAME_LENGTH,
					filename_pattern );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Reset the EPICS next file number to 0. */

		next_file_number = 0;

		mx_status = mx_caput( &(epics_ad->file_number_pv),
					MX_CA_LONG, 1, &next_file_number );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			image_mode = 0;
			break;
		case MXT_SQ_STREAM:
			image_mode = 2;
			break;
		case MXT_SQ_MULTIFRAME:
			image_mode = 1;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Area detector sequence type %ld is not supported for "
			"EPICS Area Detector '%s'.",
				(long) sp->sequence_type, ad->record->name );
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

		if ( epics_ad->use_num_acquisitions ) {
			num_frames_pv = &(epics_ad->num_acquisitions_pv);
		} else {
			num_frames_pv = &(epics_ad->num_images_pv);
		}

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
			num_frames = sp->parameter_array[0];
		} else {
			num_frames = 1;
		}

		mx_status = mx_caput( num_frames_pv,
					MX_CA_LONG, 1, &num_frames );

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
				(long) ad->trigger_mode, ad->record->name );
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

	case MXLV_AD_ROI:
		epics_ad_roi = &(epics_ad->epics_roi_array[ ad->roi_number ]);

		/* Assign the ROI to this camera. */

		mx_status = mx_caput( &(epics_ad_roi->nd_array_port_pv),
				MX_CA_STRING, 1, epics_ad->asyn_port_name );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		x_start = ad->roi[0];
		x_size  = ad->roi[1] - x_start;
		y_start = ad->roi[2];
		y_size  = ad->roi[3] - y_start;

		mx_status = mx_caput( &(epics_ad_roi->min_x_pv),
					MX_CA_LONG, 1, &x_start );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caput( &(epics_ad_roi->size_x_pv),
					MX_CA_LONG, 1, &x_size );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caput( &(epics_ad_roi->min_y_pv),
					MX_CA_LONG, 1, &y_start );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caput( &(epics_ad_roi->size_y_pv),
					MX_CA_LONG, 1, &y_size );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	default:
		mx_status = mx_area_detector_default_set_parameter_handler(ad);
		break;
	}

	return mx_status;
}


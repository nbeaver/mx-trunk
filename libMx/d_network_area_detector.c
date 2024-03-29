/*
 * Name:    d_network_area_detector.c
 *
 * Purpose: MX network area detector device driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2011, 2013-2018, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_AREA_DETECTOR_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_network_area_detector.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_network_area_detector_record_function_list = {
	mxd_network_area_detector_initialize_driver,
	mxd_network_area_detector_create_record_structures,
	mxd_network_area_detector_finish_record_initialization,
	NULL,
	NULL,
	mxd_network_area_detector_open,
	NULL,
	NULL,
	mxd_network_area_detector_resynchronize
};

MX_AREA_DETECTOR_FUNCTION_LIST
mxd_network_area_detector_ad_function_list = {
	mxd_network_area_detector_arm,
	mxd_network_area_detector_trigger,
	NULL,
	mxd_network_area_detector_stop,
	mxd_network_area_detector_abort,
	mxd_network_area_detector_get_last_frame_number,
	mxd_network_area_detector_get_total_num_frames,
	mxd_network_area_detector_get_status,
	mxd_network_area_detector_get_extended_status,
	mxd_network_area_detector_readout_frame,
	mxd_network_area_detector_correct_frame,
	mxd_network_area_detector_transfer_frame,
	mxd_network_area_detector_load_frame,
	mxd_network_area_detector_save_frame,
	mxd_network_area_detector_copy_frame,
	mxd_network_area_detector_get_roi_frame,
	mxd_network_area_detector_get_parameter,
	mxd_network_area_detector_set_parameter,
	mxd_network_area_detector_measure_correction,
	NULL,
	mxd_network_area_detector_setup_oscillation,
	mxd_network_area_detector_trigger_oscillation
};

MX_RECORD_FIELD_DEFAULTS mxd_network_area_detector_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_NETWORK_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_network_area_detector_num_record_fields
		= sizeof( mxd_network_area_detector_rf_defaults )
		  / sizeof( mxd_network_area_detector_rf_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_network_area_detector_rfield_def_ptr
			= &mxd_network_area_detector_rf_defaults[0];

/*---*/

static mx_status_type
mxd_network_area_detector_get_pointers( MX_AREA_DETECTOR *ad,
			MX_NETWORK_AREA_DETECTOR **network_area_detector,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_area_detector_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (network_area_detector == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_NETWORK_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*network_area_detector = (MX_NETWORK_AREA_DETECTOR *)
				ad->record->record_type_struct;

	if ( *network_area_detector == (MX_NETWORK_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_NETWORK_AREA_DETECTOR pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_network_area_detector_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_area_detector_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	network_area_detector = (MX_NETWORK_AREA_DETECTOR *)
				malloc( sizeof(MX_NETWORK_AREA_DETECTOR) );

	if ( network_area_detector == (MX_NETWORK_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_NETWORK_AREA_DETECTOR structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = network_area_detector;
	record->class_specific_function_list = 
			&mxd_network_area_detector_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	network_area_detector->record = record;

	ad->trigger_mode = 0;
	ad->initial_correction_flags = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_area_detector_finish_record_initialization()";

	MX_AREA_DETECTOR *ad;
	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	strlcpy(record->network_type_name, "mx", MXU_NETWORK_TYPE_NAME_LENGTH);

	mx_status = mx_area_detector_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_area_detector->abort_nf),
		network_area_detector->server_record,
		"%s.abort", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->arm_nf),
		network_area_detector->server_record,
		"%s.arm", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->binsize_nf),
		network_area_detector->server_record,
		"%s.binsize", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->bits_per_pixel_nf),
		network_area_detector->server_record,
	    "%s.bits_per_pixel", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->byte_order_nf),
		network_area_detector->server_record,
		"%s.byte_order", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->bytes_per_frame_nf),
		network_area_detector->server_record,
	    "%s.bytes_per_frame", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->bytes_per_pixel_nf),
		network_area_detector->server_record,
	    "%s.bytes_per_pixel", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->copy_frame_nf),
		network_area_detector->server_record,
		"%s.copy_frame", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->correct_frame_nf),
		network_area_detector->server_record,
		"%s.correct_frame", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->correction_flags_nf),
		network_area_detector->server_record,
	    "%s.correction_flags", network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->correction_frame_geom_corr_last_nf),
		network_area_detector->server_record,
			"%s.correction_frame_geom_corr_last",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->correction_frame_no_geom_corr_nf),
		network_area_detector->server_record,
			"%s.correction_frame_no_geom_corr",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->correction_load_format_name_nf),
		network_area_detector->server_record,
			"%s.correction_load_format_name",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->correction_measurement_time_nf),
		network_area_detector->server_record,
			"%s.correction_measurement_time",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->correction_measurement_type_nf),
		network_area_detector->server_record,
			"%s.correction_measurement_type",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->correction_save_format_name_nf),
		network_area_detector->server_record,
			"%s.correction_save_format_name",
			network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->current_num_rois_nf),
		network_area_detector->server_record,
	    "%s.current_num_rois", network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->datafile_allow_overwrite_nf),
		network_area_detector->server_record,
    "%s.datafile_allow_overwrite", network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->datafile_autoselect_number_nf),
		network_area_detector->server_record,
    "%s.datafile_autoselect_number", network_area_detector->remote_record_name);

	mx_network_field_init(
		&(network_area_detector->datafile_directory_nf),
		network_area_detector->server_record,
			"%s.datafile_directory",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->datafile_load_format_name_nf),
		network_area_detector->server_record,
			"%s.datafile_load_format_name",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->datafile_name_nf),
		network_area_detector->server_record,
			"%s.datafile_name",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->datafile_number_nf),
		network_area_detector->server_record,
			"%s.datafile_number",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->datafile_pattern_nf),
		network_area_detector->server_record,
			"%s.datafile_pattern",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->datafile_save_format_name_nf),
		network_area_detector->server_record,
			"%s.datafile_save_format_name",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->detector_readout_time_nf),
		network_area_detector->server_record,
			"%s.detector_readout_time",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->disk_space_nf),
		network_area_detector->server_record,
			"%s.disk_space",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->exposure_mode_nf),
		network_area_detector->server_record,
		"%s.exposure_mode", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->extended_status_nf),
		network_area_detector->server_record,
	    "%s.extended_status", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->framesize_nf),
		network_area_detector->server_record,
		"%s.framesize", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->frame_filename_nf),
		network_area_detector->server_record,
	    "%s.frame_filename", network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->geom_corr_after_flat_field_nf),
		network_area_detector->server_record,
			"%s.geom_corr_after_flat_field",
			network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->get_roi_frame_nf),
		network_area_detector->server_record,
		"%s.get_roi_frame", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->image_format_name_nf),
		network_area_detector->server_record,
	    "%s.image_format_name", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->image_format_nf),
		network_area_detector->server_record,
		"%s.image_format", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->image_frame_data_nf),
		network_area_detector->server_record,
	    "%s.image_frame_data", network_area_detector->remote_record_name);

	mx_network_field_init( &(network_area_detector->image_frame_header_nf),
		network_area_detector->server_record,
	    "%s.image_frame_header", network_area_detector->remote_record_name);

	mx_network_field_init(
		&(network_area_detector->image_frame_header_length_nf),
		network_area_detector->server_record,
	    "%s.image_frame_header_length",
				network_area_detector->remote_record_name);

	mx_network_field_init( &(network_area_detector->last_frame_number_nf),
		network_area_detector->server_record,
		"%s.last_frame_number",
				network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->load_frame_nf),
		network_area_detector->server_record,
	    "%s.load_frame", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->register_name_nf),
		network_area_detector->server_record,
	    "%s.register_name",
	    			network_area_detector->remote_record_name );

	mx_network_field_init(&(network_area_detector->register_value_nf),
		network_area_detector->server_record,
	    "%s.register_value",
	    			network_area_detector->remote_record_name );

	mx_network_field_init(&(network_area_detector->maximum_frame_number_nf),
		network_area_detector->server_record,
	  "%s.maximum_frame_number", network_area_detector->remote_record_name);

	mx_network_field_init( &(network_area_detector->maximum_framesize_nf),
		network_area_detector->server_record,
	    "%s.maximum_framesize", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->maximum_num_rois_nf),
		network_area_detector->server_record,
	    "%s.maximum_num_rois", network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->num_correction_measurements_nf),
		network_area_detector->server_record,
			"%s.num_correction_measurements",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->oscillation_distance_nf),
		network_area_detector->server_record,
			"%s.oscillation_distance",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->oscillation_motor_name_nf),
		network_area_detector->server_record,
			"%s.oscillation_motor_name",
			network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->oscillation_trigger_name_nf),
		network_area_detector->server_record,
			"%s.oscillation_trigger_name",
			network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->readout_frame_nf),
		network_area_detector->server_record,
		"%s.readout_frame", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->resolution_nf),
		network_area_detector->server_record,
		"%s.resolution", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->resynchronize_nf),
		network_area_detector->server_record,
		"%s.resynchronize", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->roi_nf),
		network_area_detector->server_record,
		"%s.roi", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->roi_array_nf),
		network_area_detector->server_record,
		"%s.roi_array", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->roi_bytes_per_frame_nf),
		network_area_detector->server_record,
	  "%s.roi_bytes_per_frame", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->roi_frame_buffer_nf),
		network_area_detector->server_record,
	    "%s.roi_frame_buffer", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->roi_number_nf),
		network_area_detector->server_record,
		"%s.roi_number", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->save_frame_nf),
		network_area_detector->server_record,
	    "%s.save_frame", network_area_detector->remote_record_name );

	mx_network_field_init(&(network_area_detector->sequence_start_delay_nf),
		network_area_detector->server_record,
	    "%s.sequence_start_delay",
	    		network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->setup_oscillation_nf),
		network_area_detector->server_record,
		"%s.setup_oscillation",
			network_area_detector->remote_record_name);

	mx_network_field_init( &(network_area_detector->shutter_enable_nf),
		network_area_detector->server_record,
		"%s.shutter_enable",
			network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->shutter_name_nf),
		network_area_detector->server_record,
		"%s.shutter_name", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->shutter_time_nf),
			network_area_detector->server_record,
			"%s.shutter_time",
			network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->status_nf),
		network_area_detector->server_record,
		"%s.status", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->stop_nf),
		network_area_detector->server_record,
		"%s.stop", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->subframe_size_nf),
		network_area_detector->server_record,
		"%s.subframe_size", network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->total_acquisition_time_nf),
		network_area_detector->server_record,
			"%s.total_acquisition_time",
			network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->total_num_frames_nf),
		network_area_detector->server_record,
		"%s.total_num_frames",
				network_area_detector->remote_record_name );

	mx_network_field_init(
		&(network_area_detector->total_sequence_time_nf),
		network_area_detector->server_record,
			"%s.total_sequence_time",
			network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->transfer_frame_nf),
		network_area_detector->server_record,
		"%s.transfer_frame", network_area_detector->remote_record_name);

	mx_network_field_init( &(network_area_detector->trigger_nf),
		network_area_detector->server_record,
		"%s.trigger", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->trigger_mode_nf),
		network_area_detector->server_record,
		"%s.trigger_mode", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->trigger_oscillation_nf),
		network_area_detector->server_record,
		"%s.trigger_oscillation",
			network_area_detector->remote_record_name);

	mx_network_field_init(
			&(network_area_detector->use_scaled_dark_current_nf),
		network_area_detector->server_record,
      "%s.use_scaled_dark_current", network_area_detector->remote_record_name );

	/*---*/

	mx_network_field_init( &(network_area_detector->sequence_type_nf),
		network_area_detector->server_record,
		"%s.sequence_type", network_area_detector->remote_record_name );

	mx_network_field_init(
			&(network_area_detector->num_sequence_parameters_nf),
		network_area_detector->server_record,
		"%s.num_sequence_parameters",
				network_area_detector->remote_record_name );

	mx_network_field_init(
			&(network_area_detector->sequence_parameter_array_nf),
		network_area_detector->server_record,
		"%s.sequence_parameter_array",
				network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->sequence_stream_nf),
		network_area_detector->server_record,
		"%s.sequence_stream",
				network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->sequence_duration_nf),
		network_area_detector->server_record,
		"%s.sequence_duration",
				network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->sequence_gated_nf),
		network_area_detector->server_record,
		"%s.sequence_gated",
				network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->sequence_multiframe_nf),
		network_area_detector->server_record,
		"%s.sequence_multiframe",
				network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->sequence_one_shot_nf),
		network_area_detector->server_record,
		"%s.sequence_one_shot",
				network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->sequence_strobe_nf),
		network_area_detector->server_record,
		"%s.sequence_strobe",
				network_area_detector->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_area_detector_open()";

	MX_AREA_DETECTOR *ad;
	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	long dimension[1];
	mx_status_type mx_status;

	network_area_detector = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* FIXME: We need to get the header length from the server. */

	ad->header_length = 0;

	/* Get the pixel pitch (in mm/pixel) from the server. */

	dimension[0] = 2;

	mx_status = mx_get_array(&(network_area_detector->resolution_nf),
				MXFT_DOUBLE, 1, dimension,
				ad->resolution );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the maximum framesize from the server. */

	dimension[0] = 2;

	mx_status = mx_get_array(&(network_area_detector->maximum_framesize_nf),
				MXFT_LONG, 1, dimension,
				ad->maximum_framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', maximum_framesize = (%ld, %ld)",
		fname, record->name,
		ad->maximum_framesize[0],
		ad->maximum_framesize[1]));
#endif

	/* Get the image framesize from the server. */

	dimension[0] = 2;

	mx_status = mx_get_array( &(network_area_detector->framesize_nf),
				MXFT_LONG, 1, dimension,
				ad->framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', framesize = (%ld, %ld)",
	    fname, record->name, ad->framesize[0], ad->framesize[1]));
#endif

	/* Get the image binsize from the server. */

	dimension[0] = 2;

	mx_status = mx_get_array( &(network_area_detector->binsize_nf),
				MXFT_LONG, 1, dimension,
				ad->binsize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', binsize = (%ld, %ld)",
	    fname, record->name, ad->binsize[0], ad->binsize[1]));
#endif

	/* Get the image format name from the server. */

	dimension[0] = MXU_IMAGE_FORMAT_NAME_LENGTH;

	mx_status = mx_get_array(&(network_area_detector->image_format_name_nf),
				MXFT_STRING, 1, dimension,
				ad->image_format_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the image format type. */

	mx_status = mx_image_get_image_format_type_from_name(
					ad->image_format_name,
					&(ad->image_format) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s: record '%s', image format name = '%s', image format = %ld",
		fname, record->name, ad->image_format_name,
		ad->image_format));
#endif

	/* Get the byte order. */

	mx_status = mx_get( &(network_area_detector->byte_order_nf),
				MXFT_LONG, &(ad->byte_order) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the bits per pixel. */

	mx_status = mx_get( &(network_area_detector->bits_per_pixel_nf),
				MXFT_LONG, &(ad->bits_per_pixel) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the bytes per pixel. */

	mx_status = mx_get( &(network_area_detector->bytes_per_pixel_nf),
				MXFT_DOUBLE, &(ad->bytes_per_pixel) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the bytes per frame. */

	mx_status = mx_get( &(network_area_detector->bytes_per_frame_nf),
				MXFT_LONG, &(ad->bytes_per_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the trigger mode. */

	mx_status = mx_get( &(network_area_detector->trigger_mode_nf),
				MXFT_LONG, &(ad->trigger_mode) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Setup the image frame data structure. */

	mx_status = mx_area_detector_setup_frame( record, &(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the datafile save format to SMV. */

	strlcpy( ad->datafile_save_format_name, "SMV",
				sizeof( ad->datafile_save_format_name ) );

	mx_status = mx_image_get_file_format_type_from_name(
				ad->datafile_save_format_name,
				(long *) &(ad->datafile_save_format) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the MXF_AD_SAVE_FRAME_AFTER_ACQUISITION is set and this is
	 * an MX server, then we configure automatic saving of image frames.
	 * If this process is _not_ an MX server, then we ignore the flag.
	 */

	if ( ad->area_detector_flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION ) {

		if ( mx_database_is_server( record ) ) {
			/* Fetch the current value of total_num_frames from
			 * the remote MX server.
			 */

			mx_status =
			  mxd_network_area_detector_get_total_num_frames( ad );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* We need to initialize the local copy of the
			 * datafile_total_num_frames field to the remote
			 * MX server's value of total_num_frames.  Otherwise,
			 * the datafile management handler will attempt to
			 * write out image frames for each and every image
			 * acquired since the remote MX server started,
			 * which could easily be thousands of frames.
			 */

			ad->datafile_total_num_frames = ad->total_num_frames;

			/* Now it is safe to turn on automatic datafile
			 * management.
			 */

			MX_DEBUG(-2,
				("%s: Network area detector '%s' is setting up "
				"automatic datafile management.",
				fname, record->name ));

			mx_status =
		    mx_area_detector_setup_datafile_management( record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_area_detector_resynchronize()";

	MX_AREA_DETECTOR *ad;
	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_bool_type resynchronize;
	mx_status_type mx_status;

	network_area_detector = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = record->record_class_struct;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	resynchronize = 1;

	mx_status = mx_put( &(network_area_detector->resynchronize_nf),
				MXFT_BOOL, &resynchronize );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_arm()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	mx_status = mx_put( &(network_area_detector->arm_nf),
				MXFT_BOOL, &(ad->arm) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_trigger()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	mx_status = mx_put( &(network_area_detector->trigger_nf),
				MXFT_BOOL, &(ad->trigger) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_stop()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_put( &(network_area_detector->stop_nf),
				MXFT_BOOL, &(ad->stop) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_abort()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_put( &(network_area_detector->abort_nf),
				MXFT_BOOL, &(ad->abort) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_get_last_frame_number( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_network_area_detector_get_last_frame_number()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_area_detector->last_frame_number_nf),
				MXFT_LONG, &(ad->last_frame_number) );

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: area detector '%s', last_frame_number = %ld",
		fname, ad->record->name, ad->last_frame_number ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_get_total_num_frames( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_network_area_detector_get_total_num_frames()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_area_detector->total_num_frames_nf),
				MXFT_LONG, &(ad->total_num_frames) );

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: area detector '%s', total_num_frames = %ld",
		fname, ad->record->name, ad->total_num_frames ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_get_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_get_status()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_get( &(network_area_detector->status_nf),
				MXFT_HEX, &(ad->status) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_network_area_detector_get_extended_status()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	long dimension[1];
	int num_items;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	dimension[0] = MXU_AD_EXTENDED_STATUS_STRING_LENGTH;

	mx_status = mx_get_array( &(network_area_detector->extended_status_nf),
			MXFT_STRING, 1, dimension, &(ad->extended_status) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: ad->extended_status = '%s'",
		fname, ad->extended_status));
#endif

	num_items = sscanf( ad->extended_status, "%ld %ld %lx",
				&(ad->last_frame_number),
				&(ad->total_num_frames),
				&(ad->status) );

	if ( num_items != 3 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The string returned by server '%s' for record field '%s' "
		"was not parseable as an extended status string.  "
		"Returned string = '%s'",
			network_area_detector->server_record->name,
			"extended_status", ad->extended_status );
	}

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
	    fname, ad->last_frame_number, ad->total_num_frames, ad->status));
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_readout_frame()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', frame %ld.",
		fname, ad->record->name, ad->readout_frame ));
#endif
	/* Tell the server to read out the frame from the imaging card
	 * into the server's local frame buffer.
	 */

	mx_status = mx_put( &(network_area_detector->readout_frame_nf),
				MXFT_LONG, &(ad->readout_frame) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_correct_frame()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
		("%s invoked for area detector '%s', correction_flags=%#lx.",
		fname, ad->record->name, ad->correction_flags ));
#endif

	mx_status = mx_put( &(network_area_detector->correct_frame_nf),
				MXFT_BOOL, &(ad->correct_frame) );
	return mx_status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxp_network_ad_transfer_from_local_file( MX_AREA_DETECTOR *ad,
					MX_NETWORK_AREA_DETECTOR *network_ad )
{
	static const char fname[] = "mxp_network_ad_transfer_from_local_file()";

	MX_IMAGE_FRAME *destination_frame;
	long dimension[1];
	char server_datafile_save_format_name[MXU_IMAGE_FORMAT_NAME_LENGTH+1];
	long server_datafile_save_format;
	char remote_image_filename[2*MXU_FILENAME_LENGTH+3];
	char local_image_filename[MXU_FILENAME_LENGTH+1];
	char *filename_ptr;
	mx_status_type mx_status;

#if 1
	MX_DEBUG(-2,("%s invoked for area detector '%s', transfer_frame = %ld.",
		fname, ad->record->name, ad->transfer_frame ));
#endif

	destination_frame = *(ad->transfer_destination_frame_ptr);

	if ( destination_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The destination frame pointer for detector '%s' is NULL.",
			ad->record->name );
	}

	/* Ask the server what file format it is currently using to save
	 * images to disk.  If the MX server is acting as a front end to
	 * third party software, then it should have the name of the type
	 * of file format that the third party software is using.
	 */

	dimension[0] = MXU_IMAGE_FORMAT_NAME_LENGTH;

	mx_status = mx_get_array( &(network_ad->datafile_save_format_name_nf),
				MXFT_STRING, 1, dimension,
				server_datafile_save_format_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: server_datafile_save_format_name = '%s'",
		fname, server_datafile_save_format_name));

	mx_status = mx_image_get_file_format_type_from_name(
				server_datafile_save_format_name,
				&server_datafile_save_format );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	mx_breakpoint();

	/*---*/

	/* Update the datafile directory and filename from the server. */

	dimension[0] = MXU_FILENAME_LENGTH;

	mx_status = mx_get_array( &(network_ad->datafile_directory_nf),
				MXFT_STRING, 1, dimension,
				&(ad->datafile_directory) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: remote datafile_directory = '%s'",
		fname, ad->datafile_directory));

	dimension[0] = MXU_FILENAME_LENGTH;

	mx_status = mx_get_array( &(network_ad->datafile_name_nf),
				MXFT_STRING, 1, dimension,
				&(ad->datafile_name) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: remote datafile_name = '%s'",
		fname, ad->datafile_name));

	/*---*/

	snprintf( remote_image_filename, sizeof(remote_image_filename),
		"%s/%s", ad->datafile_directory, ad->datafile_name );

	if ( strlen( network_ad->local_datafile_directory ) == 0 ) {
		filename_ptr = remote_image_filename;
	} else {
		mx_status = mx_change_filename_prefix(
				remote_image_filename,
				ad->datafile_directory,
				network_ad->local_datafile_directory,
				local_image_filename,
				sizeof(local_image_filename) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		filename_ptr = local_image_filename;
	}

	/*---*/

	mx_status = mx_image_read_file( &destination_frame, NULL,
					server_datafile_save_format,
					filename_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: '%s' file '%s' successfully read.",
		fname, server_datafile_save_format_name, filename_ptr ));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxp_network_ad_transfer_from_remote_server( MX_AREA_DETECTOR *ad,
					MX_NETWORK_AREA_DETECTOR *network_ad )
{
	static const char fname[] =
		"mxp_network_ad_transfer_from_remote_server()";

	MX_IMAGE_FRAME *destination_frame;
	unsigned long local_frame_header_length, remote_frame_header_length;
	long dimension[1];
	mx_status_type mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', transfer_frame = %ld.",
		fname, ad->record->name, ad->transfer_frame ));
#endif
	destination_frame = *(ad->transfer_destination_frame_ptr);

	if ( destination_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The destination frame pointer for detector '%s' is NULL.",
			ad->record->name );
	}

	/* Tell the server to copy the frame to the image frame buffer. */

	mx_status = mx_put( &(network_ad->transfer_frame_nf),
				MXFT_LONG, &(ad->transfer_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure that the destination frame is configured correctly
	 * by reading the image header in the server.
	 */

	/* First, we must get the length of the header. */

	mx_status = mx_get( &(network_ad->image_frame_header_length_nf),
			MXFT_ULONG, &remote_frame_header_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: remote_frame_header_length = %lu",
		fname, remote_frame_header_length));
#endif

	/* Make sure the array is big enough for the client's version
	 * of MX.
	 */

	if ( remote_frame_header_length < MXT_IMAGE_HEADER_LENGTH_IN_BYTES ) {
		local_frame_header_length = MXT_IMAGE_HEADER_LENGTH_IN_BYTES;
	} else {
		local_frame_header_length = remote_frame_header_length;
	}

	/* If the header is too long to fit into the client's image header,
	 * then increase the size of the client's image header.
	 */

	if ( local_frame_header_length >
			destination_frame->allocated_header_length )
	{
		destination_frame->header_data =
			realloc( destination_frame->header_data,
				local_frame_header_length );

		if ( destination_frame->header_data == NULL ) {
			destination_frame->allocated_header_length = 0;
			destination_frame->header_length = 0;

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to increase the size "
			"of the image frame header array for record '%s' "
			"to %ld bytes.", ad->record->name,
				local_frame_header_length );
		}

		destination_frame->allocated_header_length
				= local_frame_header_length;
	}

	destination_frame->header_length = local_frame_header_length;

	memset( destination_frame->header_data, 0,
			destination_frame->allocated_header_length );

	/* Now transfer the header. */

	dimension[0] = (long) remote_frame_header_length;

	mx_status = mx_get_array( &(network_ad->image_frame_header_nf),
			MXFT_CHAR, 1, dimension,
			destination_frame->header_data );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	{
		uint32_t *uint32_header_data;
		long n, num_header_words;

		uint32_header_data = destination_frame->header_data;

		num_header_words = local_frame_header_length / sizeof(uint32_t);

		for ( n = 0; n < num_header_words; n++ ) {
			MX_DEBUG(-2,("%s: uint32_header_data[%ld] = %lu",
			fname, n, (unsigned long) uint32_header_data[n]));
		}
	}
#endif

	/* Copy the header values to the MX_AREA_DETECTOR structure. */

	ad->framesize[0]    = (long) MXIF_ROW_FRAMESIZE(destination_frame);
	ad->framesize[1]    = (long) MXIF_COLUMN_FRAMESIZE(destination_frame);
	ad->binsize[0]      = (long) MXIF_ROW_BINSIZE(destination_frame);
  	ad->binsize[1]      = (long) MXIF_COLUMN_BINSIZE(destination_frame);
	ad->image_format    = (long) MXIF_IMAGE_FORMAT(destination_frame);
	ad->byte_order      = (long) MXIF_BYTE_ORDER(destination_frame);
	ad->bytes_per_pixel = MXIF_BYTES_PER_PIXEL(destination_frame);
	ad->header_length   = (long) local_frame_header_length;

	ad->bytes_per_frame = mx_round( ad->bytes_per_pixel
		* (double) MXIF_ROW_FRAMESIZE(destination_frame)
		* (double) MXIF_COLUMN_FRAMESIZE(destination_frame) );

	/* Reconfigure the destination_frame data structure to match
	 * the image frame that it is about to receive.
	 */

	mx_status = mx_image_alloc( &destination_frame,
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

	/* Now read the frame into the MX_IMAGE_FRAME structure. */

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: reading a %ld byte image frame.",
			fname, (long) destination_frame->image_length ));

	{
		char *image_data;
		long image_length;

		image_data = destination_frame->image_data;
		image_length = destination_frame->image_length;

		MX_DEBUG(-2,("%s: about to read image_data[%ld]",
				fname, image_length - 1));

		MX_DEBUG(-2,("%s: image_data[%ld] = %u",
			fname, image_length - 1,
			image_data[ image_length - 1 ] ));
	}
#endif

	dimension[0] = (long) destination_frame->image_length;

	mx_status = mx_get_array( &(network_ad->image_frame_data_nf),
				MXFT_CHAR, 1, dimension,
				destination_frame->image_data);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
    ("%s: successfully read a %lu byte image frame from area detector '%s'.",
		fname, (unsigned long) destination_frame->image_length,
		ad->record->name ));

	{
		int i;
		unsigned char c;
		unsigned char *frame_buffer;

		frame_buffer = destination_frame->image_data;

		for ( i = 0; i < 10; i++ ) {
			c = frame_buffer[i];

			MX_DEBUG(-2,("%s: frame_buffer[%d] = %u", fname, i, c));
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_network_area_detector_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_network_area_detector_transfer_frame()";

	MX_NETWORK_AREA_DETECTOR *network_ad = NULL;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = network_ad->network_area_detector_flags;

	if ( flags & MXF_NETWORK_AREA_DETECTOR_READ_IMAGE_LOCALLY ) {
		mx_status = 
		    mxp_network_ad_transfer_from_local_file( ad, network_ad );
	} else {
		mx_status =
		    mxp_network_ad_transfer_from_remote_server( ad, network_ad);
	}

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_network_area_detector_load_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_load_frame()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	long dimension[1];
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* NOTE: For a network_area_detector, the specified filename actually
	 * refers to the remote server's filesystem.  
	 */

	/* Tell the server the name of the file to load the frame from. */

	dimension[0] = MXU_FILENAME_LENGTH;

	mx_status = mx_put_array( &(network_area_detector->frame_filename_nf),
				MXFT_STRING, 1, dimension, ad->frame_filename );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the server to load the frame. */

	mx_status = mx_put( &(network_area_detector->load_frame_nf),
					MXFT_LONG, &(ad->load_frame) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_save_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_save_frame()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	long dimension[1];
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* NOTE: For a network_area_detector, the specified filename actually
	 * refers to the remote server's filesystem.  
	 */

	/* Tell the server the name of the file to save the frame to. */

	dimension[0] = MXU_FILENAME_LENGTH;

	mx_status = mx_put_array( &(network_area_detector->frame_filename_nf),
				MXFT_STRING, 1, dimension, ad->frame_filename );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the server to save the frame. */

	mx_status = mx_put( &(network_area_detector->save_frame_nf),
					MXFT_LONG, &(ad->save_frame) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_copy_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_copy_frame()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	long dimension[1];
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s invoked for area detector '%s', source = %#lx, destination = %#lx",
		fname, ad->record->name, ad->copy_frame[0], ad->copy_frame[1]));
#endif
	dimension[0] = 2;

	mx_status = mx_put_array( &(network_area_detector->copy_frame_nf),
				MXFT_LONG, 1, dimension, ad->copy_frame );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_get_roi_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_get_roi_frame()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	MX_IMAGE_FRAME *roi_frame;
	long dimension[1];
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	roi_frame = ad->roi_frame;

	if ( roi_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The ROI MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	/* Tell the server to prepare the frame for being read. */

	mx_status = mx_put( &(network_area_detector->get_roi_frame_nf),
				MXFT_LONG, &(ad->roi_number) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the size of the ROI image. */

	mx_status = mx_get( &(network_area_detector->roi_bytes_per_frame_nf),
				MXFT_LONG, &(ad->roi_bytes_per_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ad->roi_bytes_per_frame <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The reported number of bytes per frame for ROI %ld is %ld "
		"for area detector '%s'.  The minimum legal value is 1 byte.",
			ad->roi_number, ad->roi_bytes_per_frame,
			ad->record->name );
	}

#if 0
	MXIF_SET_BYTES_PER_PIXEL(roi_frame, ad->bytes_per_pixel);

	roi_frame->image_length    = ad->roi_bytes_per_frame;
#endif

	/* Now read the ROI frame into the MX_IMAGE_FRAME structure. */

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: reading a %ld byte ROI image frame.",
			fname, (long) roi_frame->image_length ));

	{
		char *image_data;
		long image_length;

		image_data = roi_frame->image_data;
		image_length = roi_frame->image_length;

		MX_DEBUG(-2,("%s: about to read image_data[%ld]",
				fname, image_length - 1));

		MX_DEBUG(-2,("%s: image_data[%ld] = %u",
			fname, image_length - 1,
			image_data[ image_length - 1 ] ));
	}
#endif

	dimension[0] = (long) roi_frame->image_length;

	mx_status = mx_get_array( &(network_area_detector->roi_frame_buffer_nf),
			MXFT_CHAR, 1, dimension, roi_frame->image_data );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
   ("%s: successfully read a %lu byte ROI image frame from area detector '%s'.",
		fname, (unsigned long) roi_frame->image_length,
		ad->record->name ));

	{
		int i;
		unsigned char c;
		unsigned char *frame_buffer;

		frame_buffer = roi_frame->image_data;

		for ( i = 0; i < 10; i++ ) {
			c = frame_buffer[i];

			MX_DEBUG(-2,("%s: frame_buffer[%d] = %u", fname, i, c));
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_get_parameter()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	long dimension[1];
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		mx_status = mx_get(
			&(network_area_detector->maximum_frame_number_nf),
			MXFT_ULONG, &(ad->maximum_frame_number) );
		break;

	case MXLV_AD_MAXIMUM_FRAMESIZE:
		dimension[0] = 2;

		mx_status = mx_get_array(
			&(network_area_detector->maximum_framesize_nf),
			MXFT_LONG, 1, dimension, &(ad->maximum_framesize) );
		break;

	case MXLV_AD_FRAMESIZE:
		dimension[0] = 2;

		mx_status = mx_get_array(&(network_area_detector->framesize_nf),
				MXFT_LONG, 1, dimension, &(ad->framesize) );
		break;

	case MXLV_AD_BINSIZE:
		dimension[0] = 2;

		mx_status = mx_get_array( &(network_area_detector->binsize_nf),
				MXFT_LONG, 1, dimension, &(ad->binsize) );
		break;

	case MXLV_AD_CORRECTION_FLAGS:
		mx_status = mx_get(
				&(network_area_detector->correction_flags_nf),
				MXFT_HEX, &(ad->correction_flags) );
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_get( &(network_area_detector->image_format_nf),
					MXFT_LONG, &(ad->image_format) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
#if MXD_NETWORK_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: image format = %ld, format name = '%s'",
		    fname, ad->image_format, ad->image_format_name));
#endif
		break;

	case MXLV_AD_BYTE_ORDER:
		mx_status = mx_get( &(network_area_detector->byte_order_nf),
					MXFT_LONG, &(ad->byte_order) );
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_get( &(network_area_detector->trigger_mode_nf),
					MXFT_LONG, &(ad->trigger_mode) );
		break;

	case MXLV_AD_EXPOSURE_MODE:
		mx_status = mx_get( &(network_area_detector->exposure_mode_nf),
					MXFT_LONG, &(ad->exposure_mode) );
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		mx_status =
			mx_get( &(network_area_detector->bytes_per_frame_nf),
					MXFT_LONG, &(ad->bytes_per_frame) );
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		mx_status =
			mx_get( &(network_area_detector->bytes_per_pixel_nf),
					MXFT_DOUBLE, &(ad->bytes_per_pixel) );
		break;
	case MXLV_AD_BITS_PER_PIXEL:
		mx_status =
			mx_get( &(network_area_detector->bits_per_pixel_nf),
					MXFT_LONG, &(ad->bits_per_pixel) );
		break;
	case MXLV_AD_DETECTOR_READOUT_TIME:
		mx_status =
		    mx_get( &(network_area_detector->detector_readout_time_nf),
				MXFT_DOUBLE, &(ad->detector_readout_time) );
		break;
	case MXLV_AD_DISK_SPACE:
		dimension[0] = 2;

		mx_status = mx_get_array(
				&(network_area_detector->disk_space_nf),
				MXFT_UINT64, 1, dimension, &(ad->disk_space) );
		break;
	case MXLV_AD_SEQUENCE_START_DELAY:
		mx_status =
		    mx_get( &(network_area_detector->sequence_start_delay_nf),
				MXFT_DOUBLE, &(ad->sequence_start_delay) );
		break;
	case MXLV_AD_TOTAL_ACQUISITION_TIME:
		mx_status =
		    mx_get( &(network_area_detector->total_acquisition_time_nf),
				MXFT_DOUBLE, &(ad->total_acquisition_time) );
		break;
	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		mx_status =
		    mx_get( &(network_area_detector->total_sequence_time_nf),
				MXFT_DOUBLE, &(ad->total_sequence_time) );
		break;

	case MXLV_AD_SEQUENCE_TYPE:

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: GET sequence_type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif
		mx_status = mx_get( &(network_area_detector->sequence_type_nf),
		    MXFT_LONG, &(ad->sequence_parameters.sequence_type) );

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: sequence type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		mx_status = mx_get(
			&(network_area_detector->num_sequence_parameters_nf),
			MXFT_LONG, &(ad->sequence_parameters.num_parameters) );
				
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		dimension[0] = MXU_MAX_SEQUENCE_PARAMETERS;

		mx_status = mx_get_array(
			&(network_area_detector->sequence_parameter_array_nf),
			MXFT_DOUBLE, 1, dimension,
			&(ad->sequence_parameters.parameter_array));
		break;

	case MXLV_AD_ROI:
		mx_status = mx_put( &(network_area_detector->roi_number_nf),
					MXFT_LONG, &(ad->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension[0] = 4;

		mx_status = mx_get_array( &(network_area_detector->roi_nf),
				MXFT_LONG, 1, dimension, &(ad->roi) );
		break;
	case MXLV_AD_SUBFRAME_SIZE:
		mx_status = mx_get( &(network_area_detector->subframe_size_nf),
					MXFT_ULONG, &(ad->subframe_size) );
		break;
	case MXLV_AD_USE_SCALED_DARK_CURRENT:
		mx_status = mx_get(
			&(network_area_detector->use_scaled_dark_current_nf),
				MXFT_ULONG, &(ad->use_scaled_dark_current) );
		break;
	case MXLV_AD_REGISTER_VALUE:
		dimension[0] = MXU_FIELD_NAME_LENGTH;

		mx_status = mx_put_array(
			&(network_area_detector->register_name_nf),
				MXFT_STRING, 1, dimension,
				ad->register_name );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get( &(network_area_detector->register_value_nf),
					MXFT_LONG, &(ad->register_value) );
		break;
	case MXLV_AD_SHUTTER_ENABLE:
		mx_status = mx_get( &(network_area_detector->shutter_enable_nf),
				MXFT_BOOL, &(ad->shutter_enable) );
		break;
	case MXLV_AD_CORRECTION_LOAD_FORMAT:
	case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_get_array(
		    &(network_area_detector->correction_load_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->correction_load_format_name);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_file_format_type_from_name(
					ad->correction_load_format_name,
					(long *) &(ad->correction_load_format));
		break;
	case MXLV_AD_CORRECTION_SAVE_FORMAT:
	case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_get_array(
		    &(network_area_detector->correction_save_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->correction_save_format_name);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_file_format_type_from_name(
					ad->correction_save_format_name,
					(long *) &(ad->correction_save_format));
		break;
	case MXLV_AD_DATAFILE_LOAD_FORMAT:
	case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_get_array(
			&(network_area_detector->datafile_load_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->datafile_load_format_name);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_file_format_type_from_name(
					ad->datafile_load_format_name,
					(long *) &(ad->datafile_load_format) );
		break;
	case MXLV_AD_DATAFILE_SAVE_FORMAT:
	case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_get_array(
			&(network_area_detector->datafile_save_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->datafile_save_format_name);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_file_format_type_from_name(
					ad->datafile_save_format_name,
					(long *) &(ad->datafile_save_format) );
		break;
	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		mx_status = mx_get(
			&(network_area_detector->datafile_allow_overwrite_nf),
			MXFT_BOOL, &(ad->datafile_allow_overwrite) );
		break;
	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		mx_status = mx_get(
		    &(network_area_detector->datafile_autoselect_number_nf),
			MXFT_BOOL, &(ad->datafile_autoselect_number) );
		break;
	case MXLV_AD_DATAFILE_DIRECTORY:
		dimension[0] = MXU_FILENAME_LENGTH;

		mx_status = mx_get_array(
			&(network_area_detector->datafile_directory_nf),
			MXFT_STRING, 1, dimension, ad->datafile_directory );
		break;
	case MXLV_AD_DATAFILE_NAME:
		dimension[0] = MXU_FILENAME_LENGTH;

		mx_status = mx_get_array(
			&(network_area_detector->datafile_name_nf),
			MXFT_STRING, 1, dimension, ad->datafile_name );
		break;
	case MXLV_AD_DATAFILE_NUMBER:
		mx_status = mx_get(
			&(network_area_detector->datafile_number_nf),
			MXFT_ULONG, &(ad->datafile_number) );
		break;
	case MXLV_AD_DATAFILE_PATTERN:
		dimension[0] = MXU_FILENAME_LENGTH;

		mx_status = mx_get_array(
			&(network_area_detector->datafile_pattern_nf),
			MXFT_STRING, 1, dimension, ad->datafile_pattern );
		break;
	case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		mx_status = mx_get(
		    &(network_area_detector->correction_measurement_time_nf),
		    MXFT_DOUBLE, &(ad->correction_measurement_time) );
		break;
	case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		mx_status = mx_get(
		    &(network_area_detector->correction_measurement_type_nf),
		    MXFT_LONG, &(ad->correction_measurement_type) );
		break;
	case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
		mx_status = mx_get(
		    &(network_area_detector->num_correction_measurements_nf),
		    MXFT_LONG, &(ad->num_correction_measurements) );
		break;
	case MXLV_AD_FRAME_FILENAME:
		dimension[0] = MXU_FILENAME_LENGTH;

		mx_status = mx_get_array(
			&(network_area_detector->frame_filename_nf),
			MXFT_STRING, 1, dimension, ad->frame_filename );
		break;
	default:
		mx_status =
			mx_area_detector_default_get_parameter_handler( ad );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_set_parameter()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	long dimension[1];
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:

		dimension[0] = 2;

		mx_status = mx_put_array(&(network_area_detector->framesize_nf),
				MXFT_LONG, 1, dimension, &(ad->framesize) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Update the local binsize to match. */

		mx_status = mx_get_array(&(network_area_detector->binsize_nf),
				MXFT_LONG, 1, dimension, &(ad->binsize) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Update the local bytes per frame to match. */

		mx_status = mx_get_array(
			&(network_area_detector->bytes_per_frame_nf),
			MXFT_LONG, 1, dimension, &(ad->bytes_per_frame) );
		break;

	case MXLV_AD_BINSIZE:
		dimension[0] = 2;

		mx_status = mx_put_array( &(network_area_detector->binsize_nf),
				MXFT_LONG, 1, dimension, &(ad->binsize) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Update the local framesize to match. */

		mx_status = mx_get_array(&(network_area_detector->framesize_nf),
				MXFT_LONG, 1, dimension, &(ad->framesize) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Update the local bytes per frame to match. */

		mx_status = mx_get_array(
			&(network_area_detector->bytes_per_frame_nf),
			MXFT_LONG, 1, dimension, &(ad->bytes_per_frame) );
		break;

	case MXLV_AD_CORRECTION_FLAGS:
		mx_status = mx_put(
				&(network_area_detector->correction_flags_nf),
				MXFT_HEX, &(ad->correction_flags) );
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_put( &(network_area_detector->trigger_mode_nf),
				MXFT_LONG, &(ad->trigger_mode) );
		break;

	case MXLV_AD_EXPOSURE_MODE:
		mx_status = mx_put( &(network_area_detector->exposure_mode_nf),
					MXFT_LONG, &(ad->exposure_mode) );
		break;

	case MXLV_AD_SEQUENCE_TYPE:

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: PUT sequence_type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif
		mx_status = mx_put( &(network_area_detector->sequence_type_nf),
		    MXFT_LONG, &(ad->sequence_parameters.sequence_type) );

		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		mx_status = mx_put(
			&(network_area_detector->num_sequence_parameters_nf),
			MXFT_LONG, &(ad->sequence_parameters.num_parameters) );
				
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		dimension[0] = ad->sequence_parameters.num_parameters;

		mx_status = mx_put_array(
			&(network_area_detector->sequence_parameter_array_nf),
			MXFT_DOUBLE, 1, dimension,
			&(ad->sequence_parameters.parameter_array));
		break;

	case MXLV_AD_SEQUENCE_ONE_SHOT:
		dimension[0] = 1;

		mx_status = mx_put_array(
			&(network_area_detector->sequence_one_shot_nf),
			MXFT_DOUBLE, 1, dimension, ad->sequence_one_shot );
		break;

	case MXLV_AD_SEQUENCE_STREAM:
		dimension[0] = 1;

		mx_status = mx_put_array(
			&(network_area_detector->sequence_stream_nf),
			MXFT_DOUBLE, 1, dimension, ad->sequence_stream );
		break;

	case MXLV_AD_SEQUENCE_MULTIFRAME:
		dimension[0] = 3;

		mx_status = mx_put_array(
			&(network_area_detector->sequence_multiframe_nf),
			MXFT_DOUBLE, 1, dimension, ad->sequence_multiframe );
		break;

	case MXLV_AD_SEQUENCE_STROBE:
		dimension[0] = 2;

		mx_status = mx_put_array(
			&(network_area_detector->sequence_strobe_nf),
			MXFT_DOUBLE, 1, dimension, ad->sequence_strobe );
		break;

	case MXLV_AD_SEQUENCE_DURATION:
		dimension[0] = 1;

		mx_status = mx_put_array(
			&(network_area_detector->sequence_duration_nf),
			MXFT_DOUBLE, 1, dimension, ad->sequence_duration );
		break;

	case MXLV_AD_SEQUENCE_GATED:
		dimension[0] = 1;

		mx_status = mx_put_array(
			&(network_area_detector->sequence_gated_nf),
			MXFT_DOUBLE, 1, dimension, ad->sequence_gated );
		break;

	case MXLV_AD_ROI:
		mx_status = mx_put( &(network_area_detector->roi_number_nf),
					MXFT_LONG, &(ad->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension[0] = 4;

		mx_status = mx_put_array( &(network_area_detector->roi_nf),
				MXFT_LONG, 1, dimension, &(ad->roi) );
		break;
	case MXLV_AD_SUBFRAME_SIZE:
		mx_status = mx_put( &(network_area_detector->subframe_size_nf),
					MXFT_ULONG, &(ad->subframe_size) );
		break;
	case MXLV_AD_USE_SCALED_DARK_CURRENT:
		mx_status = mx_put(
			&(network_area_detector->use_scaled_dark_current_nf),
				MXFT_ULONG, &(ad->use_scaled_dark_current) );
		break;
	case MXLV_AD_GEOM_CORR_AFTER_FLAT_FIELD:
		mx_status = mx_put(
		    &(network_area_detector->geom_corr_after_flat_field_nf),
		    	MXFT_BOOL, &(ad->geom_corr_after_flat_field) );
		break;
	case MXLV_AD_CORRECTION_FRAME_GEOM_CORR_LAST:
		mx_status = mx_put(
		  &(network_area_detector->correction_frame_geom_corr_last_nf),
		    	MXFT_BOOL, &(ad->correction_frame_geom_corr_last) );
		break;
	case MXLV_AD_CORRECTION_FRAME_NO_GEOM_CORR:
		mx_status = mx_put(
		  &(network_area_detector->correction_frame_no_geom_corr_nf),
		    	MXFT_BOOL, &(ad->correction_frame_no_geom_corr) );
		break;
	case MXLV_AD_REGISTER_VALUE:
		dimension[0] = MXU_FIELD_NAME_LENGTH;

		mx_status = mx_put_array(
			&(network_area_detector->register_name_nf),
				MXFT_STRING, 1, dimension,
				ad->register_name );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_put( &(network_area_detector->register_value_nf),
					MXFT_LONG, &(ad->register_value) );
		break;
	case MXLV_AD_SHUTTER_ENABLE:
		mx_status = mx_put( &(network_area_detector->shutter_enable_nf),
				MXFT_BOOL, &(ad->shutter_enable) );
		break;
	case MXLV_AD_CORRECTION_LOAD_FORMAT:
		mx_status = mx_image_get_file_format_name_from_type(
					ad->correction_load_format,
					ad->correction_load_format_name,
					MXU_AD_DATAFILE_FORMAT_NAME_LENGTH );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_put_array(
		    &(network_area_detector->correction_load_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->correction_load_format_name);
		break;
	case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_put_array(
		    &(network_area_detector->correction_load_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->correction_load_format_name);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_file_format_type_from_name(
					ad->correction_load_format_name,
					(long *) &(ad->correction_load_format));
		break;
	case MXLV_AD_CORRECTION_SAVE_FORMAT:
		mx_status = mx_image_get_file_format_name_from_type(
					ad->correction_save_format,
					ad->correction_save_format_name,
					MXU_AD_DATAFILE_FORMAT_NAME_LENGTH );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_put_array(
		    &(network_area_detector->correction_save_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->correction_save_format_name);
		break;
	case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_put_array(
		    &(network_area_detector->correction_save_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->correction_save_format_name);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_file_format_type_from_name(
					ad->correction_save_format_name,
					(long *) &(ad->correction_save_format));
		break;
	case MXLV_AD_DATAFILE_LOAD_FORMAT:
		mx_status = mx_image_get_file_format_name_from_type(
					ad->datafile_load_format,
					ad->datafile_load_format_name,
					MXU_AD_DATAFILE_FORMAT_NAME_LENGTH );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_put_array(
		    &(network_area_detector->datafile_load_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->datafile_load_format_name);
		break;
	case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_put_array(
		    &(network_area_detector->datafile_load_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->datafile_load_format_name);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_file_format_type_from_name(
					ad->datafile_load_format_name,
					(long *) &(ad->datafile_load_format) );
		break;
	case MXLV_AD_DATAFILE_SAVE_FORMAT:
		mx_status = mx_image_get_file_format_name_from_type(
					ad->datafile_save_format,
					ad->datafile_save_format_name,
					MXU_AD_DATAFILE_FORMAT_NAME_LENGTH );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_put_array(
		    &(network_area_detector->datafile_save_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->datafile_save_format_name);
		break;
	case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		dimension[0] = MXU_AD_DATAFILE_FORMAT_NAME_LENGTH;

		mx_status = mx_put_array(
		    &(network_area_detector->datafile_save_format_name_nf),
			MXFT_STRING, 1, dimension,
			ad->datafile_save_format_name);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_file_format_type_from_name(
					ad->datafile_save_format_name,
					(long *) &(ad->datafile_save_format) );
		break;
	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		mx_status = mx_put(
			&(network_area_detector->datafile_allow_overwrite_nf),
			MXFT_BOOL, &(ad->datafile_allow_overwrite) );
		break;
	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		mx_status = mx_put(
		    &(network_area_detector->datafile_autoselect_number_nf),
			MXFT_BOOL, &(ad->datafile_autoselect_number) );
		break;
	case MXLV_AD_DATAFILE_DIRECTORY:
		dimension[0] = MXU_FILENAME_LENGTH;

		mx_status = mx_put_array(
			&(network_area_detector->datafile_directory_nf),
			MXFT_STRING, 1, dimension, ad->datafile_directory );
		break;
	case MXLV_AD_DATAFILE_NAME:
		dimension[0] = MXU_FILENAME_LENGTH;

		mx_status = mx_put_array(
			&(network_area_detector->datafile_name_nf),
			MXFT_STRING, 1, dimension, ad->datafile_name );
		break;
	case MXLV_AD_DATAFILE_NUMBER:
		mx_status = mx_put(
			&(network_area_detector->datafile_number_nf),
			MXFT_ULONG, &(ad->datafile_number) );
		break;
	case MXLV_AD_DATAFILE_PATTERN:
		dimension[0] = MXU_FILENAME_LENGTH;

		mx_status = mx_put_array(
			&(network_area_detector->datafile_pattern_nf ),
			MXFT_STRING, 1, dimension, ad->datafile_pattern );
		break;
	case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		mx_status = mx_put(
		    &(network_area_detector->correction_measurement_time_nf),
		    MXFT_DOUBLE, &(ad->correction_measurement_time) );
		break;
	case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		mx_status = mx_put(
		    &(network_area_detector->correction_measurement_type_nf),
		    MXFT_LONG, &(ad->correction_measurement_type) );
		break;
	case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
		mx_status = mx_put(
		    &(network_area_detector->num_correction_measurements_nf),
		    MXFT_LONG, &(ad->num_correction_measurements) );
		break;
	case MXLV_AD_FRAME_FILENAME:
		dimension[0] = MXU_FILENAME_LENGTH;

		mx_status = mx_put_array(
			&(network_area_detector->frame_filename_nf),
			MXFT_STRING, 1, dimension, ad->frame_filename );
		break;
	case MXLV_AD_RAW_LOAD_FRAME:
		mx_status = mx_put( &(network_area_detector->load_frame_nf),
						MXFT_LONG, &(ad->load_frame) );
		break;
	case MXLV_AD_RAW_SAVE_FRAME:
		mx_status = mx_put( &(network_area_detector->save_frame_nf),
						MXFT_LONG, &(ad->save_frame) );
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
mxd_network_area_detector_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_network_area_detector_measure_correction()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	network_area_detector = NULL;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
	MX_DEBUG(-2,("%s: type = %ld, time = %g, num_measurements = %ld",
		fname, ad->correction_measurement_type,
		ad->correction_measurement_time,
		ad->num_correction_measurements ));
#endif

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
	case MXFT_AD_FLAT_FIELD_FRAME:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %ld is not supported "
		"for area detector '%s'.",
			ad->correction_measurement_type, ad->record->name );
	}

	/* Setting the measurement time and number of measurements
	 * comes first.
	 */

	mx_status = mx_put(
		&(network_area_detector->correction_measurement_time_nf),
		MXFT_DOUBLE, &(ad->correction_measurement_time) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put(
		&(network_area_detector->num_correction_measurements_nf),
		MXFT_LONG, &(ad->num_correction_measurements) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now start the correction by setting the measurement type. */

	mx_status = mx_put(
		&(network_area_detector->correction_measurement_type_nf),
		MXFT_LONG, &(ad->correction_measurement_type) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_setup_oscillation( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_network_area_detector_setup_oscillation()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', motor '%s', "
	"shutter '%s', trigger '%s', "
	"oscillation distance = %f, oscillation time = %f",
		fname, ad->record->name, ad->oscillation_motor_name,
		ad->shutter_name, ad->oscillation_trigger_name,
		ad->oscillation_distance, ad->exposure_time ));
#endif

	dimension[0] = MXU_RECORD_NAME_LENGTH;

	mx_status = mx_put_array(
		&(network_area_detector->oscillation_motor_name_nf),
		MXFT_STRING, 1, dimension, ad->oscillation_motor_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put_array( &(network_area_detector->shutter_name_nf),
				MXFT_STRING, 1, dimension, ad->shutter_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put_array(
		&(network_area_detector->oscillation_trigger_name_nf),
		MXFT_STRING, 1, dimension, ad->oscillation_trigger_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_area_detector->oscillation_distance_nf),
				MXFT_DOUBLE, &(ad->oscillation_distance) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_area_detector->shutter_time_nf),
				MXFT_DOUBLE, &(ad->shutter_time) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Setup the oscillation. */

	ad->setup_oscillation = TRUE;

	mx_status = mx_put( &(network_area_detector->setup_oscillation_nf),
				MXFT_BOOL, &(ad->setup_oscillation) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_trigger_oscillation( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_network_area_detector_trigger_oscillation()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector = NULL;
	mx_status_type mx_status;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	ad->trigger_oscillation = TRUE;

	mx_status = mx_put( &(network_area_detector->trigger_oscillation_nf),
				MXFT_BOOL, &(ad->trigger_oscillation) );

	return mx_status;
}


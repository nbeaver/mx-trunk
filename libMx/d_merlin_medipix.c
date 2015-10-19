/*
 * Name:    d_merlin_medipix.c
 *
 * Purpose: MX area detector driver for the Merlin Medipix series of
 *          detectors from Quantum Detectors.
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

#define MXD_MERLIN_MEDIPIX_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_merlin_medipix.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_merlin_medipix_record_function_list = {
	mxd_merlin_medipix_initialize_driver,
	mxd_merlin_medipix_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_merlin_medipix_open,
};

MX_AREA_DETECTOR_FUNCTION_LIST
mxd_merlin_medipix_ad_function_list = {
	mxd_merlin_medipix_arm,
	mxd_merlin_medipix_trigger,
	NULL,
	mxd_merlin_medipix_stop,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_merlin_medipix_get_extended_status,
	mxd_merlin_medipix_readout_frame,
	mxd_merlin_medipix_correct_frame,
	mxd_merlin_medipix_transfer_frame,
	mxd_merlin_medipix_load_frame,
	mxd_merlin_medipix_save_frame,
	mxd_merlin_medipix_copy_frame,
	NULL,
	mxd_merlin_medipix_get_parameter,
	mxd_merlin_medipix_set_parameter,
	mxd_merlin_medipix_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_merlin_medipix_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_MERLIN_MEDIPIX_STANDARD_FIELDS
};

long mxd_merlin_medipix_num_record_fields
		= sizeof( mxd_merlin_medipix_rf_defaults )
		  / sizeof( mxd_merlin_medipix_rf_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_merlin_medipix_rfield_def_ptr
			= &mxd_merlin_medipix_rf_defaults[0];

/*---*/

static mx_status_type
mxd_merlin_medipix_get_pointers( MX_AREA_DETECTOR *ad,
			MX_MERLIN_MEDIPIX **merlin_medipix,
			const char *calling_fname )
{
	static const char fname[] = "mxd_merlin_medipix_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (merlin_medipix == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MERLIN_MEDIPIX pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*merlin_medipix = (MX_MERLIN_MEDIPIX *)
				ad->record->record_type_struct;

	if ( *merlin_medipix == (MX_MERLIN_MEDIPIX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_MERLIN_MEDIPIX pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_merlin_medipix_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_merlin_medipix_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	merlin_medipix = (MX_MERLIN_MEDIPIX *)
				malloc( sizeof(MX_MERLIN_MEDIPIX) );

	if ( merlin_medipix == (MX_MERLIN_MEDIPIX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_MERLIN_MEDIPIX structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = merlin_medipix;
	record->class_specific_function_list = 
			&mxd_merlin_medipix_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	merlin_medipix->record = record;

	ad->trigger_mode = 0;
	ad->initial_correction_flags = 0;

	merlin_medipix->merlin_debug_flag = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_merlin_medipix_open()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	unsigned long mask;
	char response[100];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	/* Make the connections to the detector controller. */

	mx_status = mx_tcp_socket_open_as_client(
				&(merlin_medipix->command_socket),
				merlin_medipix->hostname,
				merlin_medipix->command_port,
				0, 1000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_tcp_socket_open_as_client(
				&(merlin_medipix->data_socket),
				merlin_medipix->hostname,
				merlin_medipix->data_port,
				0, 100000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the controller is there by asking for
	 * its software version.
	 */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"GET,SOFTWAREVERSION",
					response, sizeof(response),
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: Merlin software version = '%s'",
		fname, response ));

	/*=== Configure some default parameters for the detector. ===*/

	/* 12-bit counting with both counters */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,COUNTERDEPTH,12",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,ENABLECOUNTER,0",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off continuous acquisition. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,CONTINUOUSRW,0",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off colour mode. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,COLOURMODE,0",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off charge summing. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,CHARGESUMMING,0",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Num frames per trigger. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,NUMFRAMESPERTRIGGER,1",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Select high gain. */

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"SET,GAIN,2",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set generic area detector parameters. */

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	ad->bytes_per_pixel = 2;
	ad->bits_per_pixel = 16;

	ad->bytes_per_frame =
	  mx_round( ad->bytes_per_pixel * ad->framesize[0] * ad->framesize[1] );

	ad->image_format = MXT_IMAGE_FORMAT_GREY16;

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						MXU_IMAGE_FORMAT_NAME_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure automatic saving or readout of image frames. */

	mask = MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
		| MXF_AD_READOUT_FRAME_AFTER_ACQUISITION;

	if ( ad->area_detector_flags & mask ) {
		mx_status =
		  mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_arm()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	char command[80];
	double exposure_time, frame_time;
	unsigned long num_frames;
	mx_status_type mx_status;

	merlin_medipix = NULL;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	/* Set the exposure sequence parameters. */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		frame_time = 1.01 * exposure_time;
		break;
	case MXT_SQ_MULTIFRAME:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = sp->parameter_array[1];
		frame_time = sp->parameter_array[2];
		break;
	case MXT_SQ_DURATION:
		num_frames = mx_round( sp->parameter_array[0] );
		exposure_time = 0;
		frame_time = 0;
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Detector '%s' sequence types other than 'one-shot' "
		"are not yet implemented.", ad->record->name );
		break;
	}

	/* Configure the type of trigger mode we want. */

	switch( ad->trigger_mode ) {
	case MXT_IMAGE_INTERNAL_TRIGGER:
		mx_status = mxd_merlin_medipix_command(
					merlin_medipix,
					"SET,TRIGGERSTART,0",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_merlin_medipix_command(
					merlin_medipix,
					"SET,TRIGGERSTOP,0",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXT_IMAGE_EXTERNAL_TRIGGER:
		mx_status = mxd_merlin_medipix_command(
					merlin_medipix,
					"SET,TRIGGERSTART,1",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_merlin_medipix_command(
					merlin_medipix,
					"SET,TRIGGERSTOP,2",
					NULL, 0,
					merlin_medipix->merlin_debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Unsupported trigger mode %#lx requested for area detector '%s'.",
			ad->trigger_mode, ad->record->name );
		break;
	}

	/* Setup the sequence parameters. */

	snprintf( command, sizeof(command),
		"SET,NUMFRAMESTOACQUIRE,%lu", num_frames );

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					command, NULL, 0,
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"SET,ACQUISITIONTIME,%g", exposure_time );

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					command, NULL, 0,
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"SET,ACQUISITIONPERIOD,%g", frame_time );

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					command, NULL, 0,
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we are configured for external triggering, tell the
	 * acquisition sequence to start.
	 */

	if ( ad->trigger_mode == MXT_IMAGE_EXTERNAL_TRIGGER ) {
		mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"CMD,STARTACQUISITION",
 					NULL, 0,
					merlin_medipix->merlin_debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_trigger()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	merlin_medipix = NULL;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	/* If we are configured for internal triggering, tell the
	 * acquisition sequence to start.
	 */

	if ( ad->trigger_mode == MXT_IMAGE_INTERNAL_TRIGGER ) {
		mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"CMD,STARTACQUISITION",
 					NULL, 0,
					merlin_medipix->merlin_debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_stop()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"CMD,STOPACQUISITION",
 					NULL, 0,
					merlin_medipix->merlin_debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_get_extended_status()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	char response[80];
	int detector_status;
	long num_data_bytes_available;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG( 2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mxd_merlin_medipix_command( merlin_medipix,
					"GET,DETECTORSTATUS",
					response, sizeof(response),
					merlin_medipix->merlin_debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		
	detector_status = atoi( response );

	switch( detector_status ) {
	case 0:
		ad->status = 0;
	case 1:
		ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
		break;
	case 2:
		ad->status = MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
		break;
	default:
		ad->status = MXSF_AD_ERROR;
		break;
	}

	/* Check for any new data from the data_socket. */

	mx_status = mx_socket_num_input_bytes_available(
					merlin_medipix->data_socket,
					&num_data_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_data_bytes_available > 0 ) {
		ad->status |= MXSF_AD_UNSAVED_IMAGE_FRAMES;
	}

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s: ad->status = %#lx, num_data_bytes_available = %lu",
		fname, ad->status, num_data_bytes_available));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_readout_frame()"; 
	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', frame %ld.",
		fname, ad->record->name, ad->readout_frame ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_correct_frame()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,
		("%s invoked for area detector '%s', correction_flags=%#lx.",
		fname, ad->record->name, ad->correction_flags ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_merlin_medipix_transfer_frame()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', transfer_frame = %ld.",
		fname, ad->record->name, ad->transfer_frame ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_load_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_load_frame()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	merlin_medipix = NULL;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_save_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_save_frame()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_copy_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_copy_frame()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
	MX_DEBUG(-2,
	("%s invoked for area detector '%s', source = %#lx, destination = %#lx",
		fname, ad->record->name, ad->copy_frame[0], ad->copy_frame[1]));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_merlin_medipix_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_get_parameter()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
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
#if MXD_MERLIN_MEDIPIX_DEBUG
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

#if MXD_MERLIN_MEDIPIX_DEBUG
		MX_DEBUG(-2,("%s: GET sequence_type = %ld",
			fname, ad->sequence_parameters.sequence_type));
#endif

#if MXD_MERLIN_MEDIPIX_DEBUG
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
mxd_merlin_medipix_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_merlin_medipix_set_parameter()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
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

	case MXLV_AD_GEOM_CORR_AFTER_FLAT_FIELD:
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
mxd_merlin_medipix_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_merlin_medipix_measure_correction()";

	MX_MERLIN_MEDIPIX *merlin_medipix = NULL;
	mx_status_type mx_status;

	mx_status = mxd_merlin_medipix_get_pointers( ad,
						&merlin_medipix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MERLIN_MEDIPIX_DEBUG
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

	return mx_status;
}

/*==========================================================================*/

MX_EXPORT mx_status_type
mxd_merlin_medipix_command( MX_MERLIN_MEDIPIX *merlin_medipix,
			char *command,
			char *response,
			size_t response_buffer_length,
			unsigned long debug_flag )
{
	static const char fname[] = "mxd_merlin_medipix_command()";

	MX_AREA_DETECTOR *ad = NULL;
	char command_buffer[100];
	char response_buffer[500];
	size_t command_length, prefix_length;
	size_t num_integer_characters;
	mx_bool_type is_get_command;
	double timeout_in_seconds;
	int i, status_code;
	char *value_ptr, *status_code_ptr;
	mx_status_type mx_status;

	if ( merlin_medipix == (MX_MERLIN_MEDIPIX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MERLIN_MEDIPIX pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}

	ad = merlin_medipix->record->record_class_struct;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
		fname, command, merlin_medipix->record->name ));
	}

	if ( mx_strncasecmp( command, "GET,", 4 ) == 0 ) {
		is_get_command = TRUE;
	}

	/* The prefix length includes 4 characters for "MPX,", another
	 * 10 characters for the message length, and an additional 1
	 * character for the trailing "," character.
	 */

	prefix_length = 4 + 10 + 1;

	/* First construct the raw command string. */

	command_length = strlen( command );

	command_length++;		/* For the leading ',' separator. */

	snprintf( command_buffer, sizeof(command_buffer),
		"MPX,%010ld,%s", (long) command_length, command );

	mx_status = mx_socket_putline( merlin_medipix->command_socket,
					command_buffer, "\r\n" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now read back the response line. */

	timeout_in_seconds = 5.0;

	mx_status = mx_socket_wait_for_event( merlin_medipix->command_socket,
						timeout_in_seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_getline( merlin_medipix->data_socket,
			response_buffer, sizeof(response_buffer), "\r\n" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If this was a GET command, find the returned value in
	 * the response.  It should be located after the fourth
	 * comma (,) character in the response.
	 */

	if ( is_get_command == FALSE ) {
		value_ptr = NULL;
	} else {
		value_ptr = response_buffer;

		for ( i = 0; i < 4; i++ ) {
			value_ptr = strchr( value_ptr, ',' );

			if ( value_ptr == NULL ) {
				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"Did not find 4 ',' characters in the "
				"response '%s' to the command '%s' sent to "
				"Merlin Medipix detector '%s'.",
					response_buffer, command_buffer,
					ad->record->name );
			}
		}

		value_ptr++;	/* Skip over the comma ',' character. */

		MX_DEBUG(-2,("%s: value_ptr = '%s'", fname, value_ptr));
	}

	/* Get the status code for the message. */

	status_code_ptr = strrchr( response_buffer, ',' );

	if ( status_code_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not find any ',' characters in the response '%s' "
		"to command '%s' sent to Merlin Medipix detector '%s'.",
			response_buffer, command_buffer, ad->record->name );
	}

	/* Null terminate the value string here (if it exists). */

	*status_code_ptr = '\0';

	status_code_ptr++;

	/* Parse the status code. */

	num_integer_characters = strspn( status_code_ptr, "0123456789" );

	if ( num_integer_characters != strlen( status_code_ptr ) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not find a valid status code in the string '%s' "
		"following the response '%s' to the command '%s' sent "
		"to Merlin Medipix detector '%s'.",
			status_code_ptr, response_buffer, 
			command_buffer, ad->record->name );
	}

	status_code = atoi( status_code_ptr );

	switch( status_code ) {
	case 0:			/* 0 means OK */
		break;
	case 1:			/* 1 means busy */

		return mx_error( MXE_NOT_READY, fname,
		"The Merlin Medipix detector '%s' was not ready for "
		"the command '%s' sent to it.",
			ad->record->name, command_buffer );
		break;
	case 2:			/* 2 means not recognized */

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Command '%s' sent to Merlin Medipix detector '%s' was not "
		"a valid command.", command_buffer, ad->record->name );
		break;
	case 3:			/* 3 means parameter out of range */

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Command '%s' sent to Merlin Medipix detector '%s' attempted "
		"to set a parameter to outside its range.",
			command_buffer, ad->record->name );
		break;
	default:
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Merlin Medipix detector '%s' sent unrecognized status "
		"code %d in response to the command '%s' sent to it.",
			ad->record->name, status_code, command_buffer );
		break;
	}

	/* Copy the response to the caller's buffer. */

	if ( is_get_command ) {
		strlcpy( response, value_ptr, response_buffer_length );
	} else {
		if ( response != NULL ) {
			response[0] = '\0';
		}
	}

	/*---*/

	if (debug_flag) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'.",
			fname, response, merlin_medipix->record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}

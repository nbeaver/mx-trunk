/*
 * Name:    d_radicon_taurus.c
 *
 * Purpose: MX driver for the Radicon Taurus series of CMOS detectors.
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

#define MXD_RADICON_TAURUS_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_ascii.h"
#include "mx_motor.h"
#include "mx_image.h"
#include "mx_rs232.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "d_radicon_taurus.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_radicon_taurus_record_function_list = {
	mxd_radicon_taurus_initialize_driver,
	mxd_radicon_taurus_create_record_structures,
	mx_area_detector_finish_record_initialization,
	NULL,
	NULL,
	mxd_radicon_taurus_open
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_radicon_taurus_ad_function_list = {
	mxd_radicon_taurus_arm,
	mxd_radicon_taurus_trigger,
	mxd_radicon_taurus_stop,
	mxd_radicon_taurus_abort,
	NULL,
	NULL,
	NULL,
	mxd_radicon_taurus_get_extended_status,
	mxd_radicon_taurus_readout_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_radicon_taurus_get_parameter,
	mxd_radicon_taurus_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_radicon_taurus_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_RADICON_TAURUS_STANDARD_FIELDS
};

long mxd_radicon_taurus_num_record_fields
		= sizeof( mxd_radicon_taurus_rf_defaults )
		/ sizeof( mxd_radicon_taurus_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_radicon_taurus_rfield_def_ptr
			= &mxd_radicon_taurus_rf_defaults[0];

/*---*/

static mx_status_type
mxd_radicon_taurus_get_pointers( MX_AREA_DETECTOR *ad,
			MX_RADICON_TAURUS **radicon_taurus,
			const char *calling_fname )
{
	static const char fname[] = "mxd_radicon_taurus_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (radicon_taurus == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RADICON_TAURUS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*radicon_taurus = (MX_RADICON_TAURUS *)
				ad->record->record_type_struct;

	if ( *radicon_taurus == (MX_RADICON_TAURUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_RADICON_TAURUS pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_radicon_taurus_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_radicon_taurus_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_RADICON_TAURUS *radicon_taurus;

	/* Use calloc() here instead of malloc(), so that a bunch of
	 * fields that are never touched will be initialized to 0.
	 */

	ad = (MX_AREA_DETECTOR *) calloc( 1, sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	radicon_taurus = (MX_RADICON_TAURUS *)
				calloc( 1, sizeof(MX_RADICON_TAURUS) );

	if ( radicon_taurus == (MX_RADICON_TAURUS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_RADICON_TAURUS structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = radicon_taurus;
	record->class_specific_function_list = 
			&mxd_radicon_taurus_ad_function_list;

	ad->record = record;
	radicon_taurus->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_radicon_taurus_open()";

	MX_AREA_DETECTOR *ad;
	MX_RADICON_TAURUS *radicon_taurus = NULL;
	MX_RECORD *video_input_record, *serial_port_record;
	long i;
	char c;
	unsigned long mask, num_bytes_available;
	char response[100];
	char *string_value_ptr;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	video_input_record = radicon_taurus->video_input_record;

	serial_port_record = radicon_taurus->serial_port_record;

	/* Configure the serial port as needed by the Taurus driver. */

	mx_status = mx_rs232_set_configuration( serial_port_record,
						115200, 8, 'N', 1, 'S',
						0x0d0a, 0x0d );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any extraneous characters in the serial port buffers. */

	mx_status = mx_rs232_discard_unwritten_output( serial_port_record,
						MXD_RADICON_TAURUS_DEBUG );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( serial_port_record,
						MXD_RADICON_TAURUS_DEBUG );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Request camera parameters from the detector. */

	mx_status = mxd_radicon_taurus_command( radicon_taurus, "GCP",
					NULL, 0, MXD_RADICON_TAURUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_msleep(500);

	/* Loop through the responses from the detector to the GCP command. */

	while (1) {
		mx_status = mx_rs232_num_input_bytes_available(
					radicon_taurus->serial_port_record,
					&num_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_bytes_available <= 1 ) {
			break;		/* Exit the while() loop. */
		}

		mx_status = mx_rs232_getline(radicon_taurus->serial_port_record,
					response, sizeof(response), NULL,
					MXD_RADICON_TAURUS_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Discard any leading LF character. */

		if ( response[0] == MX_LF ) {
			size_t length = strlen( response );

			memmove( response, response + 1, length );
		}

		string_value_ptr = strchr( response, ':' );

		if ( string_value_ptr == NULL ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not find the value separator char ':' "
			"in the response line '%s' for the detector '%s'.",
				response, record->name );
		}

		/* Skip over the colon and a trailing blank. */

		string_value_ptr++;
		string_value_ptr++;

		if ( strncmp( response, "Detector Model", 14 ) == 0 ) {
			if ( strcmp( string_value_ptr, "TAURUS" ) == 0 ) {

				radicon_taurus->detector_model
					= MXT_RADICON_TAURUS;
			} else {
				return mx_error( MXE_UNSUPPORTED, fname,
				"Unrecognized detector model '%s' seen "
				"for detector '%s'.",
					string_value_ptr, record->name );
			}
		} else
		if ( strncmp( response, "Camera Model", 12 ) == 0 ) {
			if ( strncmp(string_value_ptr, "SkiaGraph", 9) == 0 ) {
				radicon_taurus->detector_model
					= MXT_RADICON_XINEOS;
			} else {
				return mx_error( MXE_UNSUPPORTED, fname,
				"Unrecognized camera model '%s' seen "
				"for detector '%s'.",
					string_value_ptr, record->name );
			}
		} else
		if ( strncmp( response, "Detector S/N", 12 ) == 0 ) {
			radicon_taurus->serial_number =
				mx_string_to_unsigned_long( string_value_ptr );
		} else
		if ( strncmp( response, "Camera S/N", 10 ) == 0 ) {
			radicon_taurus->serial_number =
				mx_string_to_unsigned_long( string_value_ptr );
		} else
		if ( strncmp(response, "Detector FW Build Version", 25) == 0 ) {
			radicon_taurus->firmware_version =
				mx_string_to_unsigned_long( string_value_ptr );
		} else
		if ( strncmp( response, "Camera FW Build Version", 23 ) == 0 ) {
			/* Skip over leading FW_ characters. */

			string_value_ptr += 3;

			radicon_taurus->firmware_version =
				mx_string_to_unsigned_long( string_value_ptr );
		} else {
			/* Ignore this line. */
		}
	}

	/* If present, delete the camera's ">" prompt for the next command. */

	if ( num_bytes_available == 1 ) {
		mx_status = mx_rs232_getchar(
				radicon_taurus->serial_port_record,
				&c, MXD_RADICON_TAURUS_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*---*/

	ad->header_length = 0;

	ad->sequence_parameters.sequence_type = MXT_SQ_ONE_SHOT;
	ad->sequence_parameters.num_parameters = 1;
	ad->sequence_parameters.parameter_array[0] = 1.0;

	/* Set the default file formats. */

	ad->datafile_load_format   = MXT_IMAGE_FILE_SMV;
	ad->datafile_save_format   = MXT_IMAGE_FILE_SMV;
	ad->correction_load_format = MXT_IMAGE_FILE_SMV;
	ad->correction_save_format = MXT_IMAGE_FILE_SMV;

	/* Set the maximum framesize to the initial framesize of the
	 * video input.
	 */

	mx_status = mx_video_input_get_framesize( video_input_record,
					&(ad->maximum_framesize[0]),
					&(ad->maximum_framesize[1]) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->framesize[0] = ad->maximum_framesize[0];
	ad->framesize[1] = ad->maximum_framesize[1];

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	radicon_taurus->readout_mode = -1;

	/* Copy other needed parameters from the video input record to
	 * the area detector record.
	 */

	mx_status = mx_video_input_get_image_format( video_input_record,
							&(ad->image_format) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						sizeof(ad->image_format_name) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_byte_order( video_input_record,
							&(ad->byte_order) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_pixel( video_input_record,
							&(ad->bytes_per_pixel));
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bits_per_pixel( video_input_record,
							&(ad->bits_per_pixel));
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( video_input_record,
							&(ad->bytes_per_frame));
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The detector will default to internal triggering. */

	mx_status = mx_area_detector_set_trigger_mode( record,
						MXT_IMAGE_INTERNAL_TRIGGER );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the video card that the camera head is in charge of timing. */

	mx_status = mx_video_input_set_master_clock( video_input_record,
							MXF_VIN_MASTER_CAMERA );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_set_trigger_mode( video_input_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Zero out the ROI boundaries. */

	for ( i = 0; i < ad->maximum_num_rois; i++ ) {
		ad->roi_array[i][0] = 0;
		ad->roi_array[i][1] = 0;
		ad->roi_array[i][2] = 0;
		ad->roi_array[i][3] = 0;
	}

	/* Load the image correction files. */

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure automatic saving or loading of image frames. */

	mask = MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
		| MXF_AD_LOAD_FRAME_AFTER_ACQUISITION;

	if ( ad->area_detector_flags & mask ) {
		mx_status =
		    mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

#define TAURUS_MINIMUM_EXPOSURE_TIME	4
#define TAURUS_SECONDS_PER_STEP		(33.0e-9)

MX_EXPORT mx_status_type
mxd_radicon_taurus_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_arm()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time, exposure_time_offset;
	unsigned long raw_exposure_time_32;
	uint64_t raw_exposure_time_64;
	unsigned long low_order, middle_order, high_order;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	/* Set the exposure time for this sequence. */

	sp = &(ad->sequence_parameters);

	mx_status = mx_sequence_get_exposure_time( sp, 0, &exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( radicon_taurus->detector_model ) {
	case MXT_RADICON_TAURUS:
		/* Set the first integration time. */

		if ( exposure_time < 0.0 ) {
		    raw_exposure_time_64 = TAURUS_MINIMUM_EXPOSURE_TIME;
		} else {
		    raw_exposure_time_64 = (uint64_t)( 0.5
			+ (exposure_time / TAURUS_SECONDS_PER_STEP) );

		    if ( raw_exposure_time_64 < TAURUS_MINIMUM_EXPOSURE_TIME ) {
			raw_exposure_time_64 = TAURUS_MINIMUM_EXPOSURE_TIME;
		    }
		}

		low_order    = raw_exposure_time_64 & 0xffff;
		middle_order = (raw_exposure_time_64 >> 16) & 0xffff;
		high_order   = (raw_exposure_time_64 >> 32) & 0xf;

		snprintf( command, sizeof(command),
			"SI1 %lu %lu %lu",
			low_order, middle_order, high_order );

		mx_status = mxd_radicon_taurus_command( radicon_taurus, command,
					NULL, 0, MXD_RADICON_TAURUS_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the second integration time to a value
		 * that is 8 times smaller to get the lower
		 * order bits of the signal.
		 */

		raw_exposure_time_64 /= 8;

		if ( raw_exposure_time_64 < TAURUS_MINIMUM_EXPOSURE_TIME ) {
			raw_exposure_time_64 = TAURUS_MINIMUM_EXPOSURE_TIME;
		}

		low_order    = raw_exposure_time_64 & 0xffff;
		middle_order = (raw_exposure_time_64 >> 16) & 0xffff;
		high_order   = (raw_exposure_time_64 >> 32) & 0xf;

		snprintf( command, sizeof(command),
			"SI2 %lu %lu %lu",
			low_order, middle_order, high_order );

		mx_status = mxd_radicon_taurus_command( radicon_taurus, command,
					NULL, 0, MXD_RADICON_TAURUS_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/**** Set the Taurus Readout Mode. ****/
		
		/* The correct value for the readout mode depends on
		 * the sequence type and the trigger type.
		 */

		if ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {
			strlcpy( command, "SRO 4", sizeof(command) );
		} else
		if ( ad->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) {
			switch( sp->sequence_type ) {
			case MXT_SQ_ONE_SHOT:
			case MXT_SQ_MULTIFRAME:
			case MXT_SQ_STROBE:
				strlcpy( command, "SRO 3", sizeof(command) );
				break;
			case MXT_SQ_DURATION:
				strlcpy( command, "SRO 2", sizeof(command) );
				break;
			default:
				return mx_error( MXE_UNSUPPORTED, fname,
				"MX imaging sequence type %lu is not "
				"supported for external triggering with "
				"detector '%s'.",
					sp->sequence_type,
					ad->record->name );
				break;
			}
		} else {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported trigger mode %#lx requested for "
			"detector '%s'.",
				ad->trigger_mode,
				ad->record->name );
				
		}

		mx_status = mxd_radicon_taurus_command( radicon_taurus, command,
					NULL, 0, MXD_RADICON_TAURUS_DEBUG );
		break;
	case MXT_RADICON_XINEOS:
		/* FIXME: The scale factor is wrong for the Xineos. */

		raw_exposure_time_32 = mx_round( 40000.0 * exposure_time );

		snprintf( command, sizeof(command),
			"SIT %lu", raw_exposure_time_32 );

		mx_status = mxd_radicon_taurus_command( radicon_taurus, command,
					NULL, 0, MXD_RADICON_TAURUS_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ad->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) {

			strlcpy( command, "STM 2137", sizeof(command) );
		} else
		if ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {

			strlcpy( command, "STM 64", sizeof(command) );
		} else {
			mx_warning( "Unsupported trigger mode %#lx "
			"requested for detector '%s'.  "
			"Setting trigger to internal mode.",
				ad->trigger_mode, ad->record->name );

			strlcpy( command, "STM 64", sizeof(command) );

			ad->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;
		}

		mx_status = mxd_radicon_taurus_command( radicon_taurus, command,
					NULL, 0, MXD_RADICON_TAURUS_DEBUG );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported detector type %lu for detector '%s'",
			radicon_taurus->detector_model,
			ad->record->name );
		break;
	}


	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the video capture card to get ready for a frame. */

	mx_status = mx_video_input_arm( radicon_taurus->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_trigger()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_trigger(
			radicon_taurus->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_stop()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_stop(radicon_taurus->video_input_record);

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_abort()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_abort( radicon_taurus->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_radicon_taurus_get_extended_status()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	long vinput_last_frame_number;
	long vinput_total_num_frames;
	unsigned long vinput_status;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_get_extended_status(
				radicon_taurus->video_input_record,
				&vinput_last_frame_number,
				&vinput_total_num_frames,
				&vinput_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->last_frame_number = vinput_last_frame_number;
	ad->total_num_frames = vinput_total_num_frames;

	ad->status = 0;

	if ( vinput_status & MXSF_VIN_IS_BUSY ) {

		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s: last_frame_number = %ld, "
	"total_num_frames = %ld, status = %#lx",
		fname, ad->last_frame_number,
		ad->total_num_frames, ad->status));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_readout_frame()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_get_frame(
		radicon_taurus->video_input_record,
		ad->readout_frame, &(ad->image_frame) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_get_parameter()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	MX_RECORD *video_input_record;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif
	video_input_record = radicon_taurus->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		mx_status = mx_video_input_get_framesize( video_input_record,
				&(ad->framesize[0]), &(ad->framesize[1]) );
		break;
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_video_input_get_image_format( video_input_record,
						&(ad->image_format) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		mx_status = mx_video_input_get_bytes_per_frame(
				video_input_record, &(ad->bytes_per_frame) );
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		mx_status = mx_video_input_get_bytes_per_pixel(
				video_input_record, &(ad->bytes_per_pixel) );
		break;

	case MXLV_AD_TRIGGER_MODE:
		/* The video capture card should always be in 'free run'
		 * mode, so we do not check its status here.
		 */
		break;

	case MXLV_AD_BITS_PER_PIXEL:
		mx_status = mx_video_input_get_bits_per_pixel(
				video_input_record, &(ad->bits_per_pixel) );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		break;

	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_set_parameter()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	MX_RECORD *video_input_record;
	unsigned long readout_mode, trigger_mask;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif
	video_input_record = radicon_taurus->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		switch( radicon_taurus->detector_model ) {
		case MXT_RADICON_TAURUS:
			/* The Taurus detector does not support binning. */

			ad->binsize[0] = 1;
			ad->binsize[1] = 1;

			mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
			break;

		case MXT_RADICON_XINEOS:
			/* For the Xineos detector, if you bin in one
			 * dimension, then you must bin in _both_ dimensions.
			 * The detector does not allow you to specify them
			 * independently.
			 */

			if (( ad->binsize[0] > 1 ) || ( ad->binsize[1] > 1 )) {
				ad->binsize[0] = 2;
				ad->binsize[1] = 2;
				readout_mode = 1;
			} else {
				readout_mode = 2;
			}

			mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
	
			/* Tell the video input to change its framesize. */
	
			mx_status = mx_video_input_set_framesize(
					video_input_record,
					ad->framesize[0], ad->framesize[1] );
	
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
	
			/* Tell the camera head to switch readout modes. */
	
			if ( readout_mode != radicon_taurus->readout_mode ) {
				if ( ad->binsize[0] == 1 ) {
					strlcpy( command, "SVM 1",
							sizeof(command) );
				} else {
					strlcpy( command, "SVM 2",
							sizeof(command) );
				}
				mx_status = mxd_radicon_taurus_command(
						radicon_taurus,
						command, NULL, 0,
						MXD_RADICON_TAURUS_DEBUG );
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported detector model %lu for detector '%s'.",
				radicon_taurus->detector_model,
				ad->record->name );
			break;
		}
		break;

	case MXLV_AD_TRIGGER_MODE:
		/* All of the trigger mode handling logic is actually done
		 * at 'arm' time.  Look in mxd_radicon_taurus_arm() for it.
		 */
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		mx_status = mx_video_input_set_sequence_parameters(
				video_input_record,
				&(ad->sequence_parameters) );
		break; 

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Changing parameter '%s' for area detector '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
	default:
		mx_status = mx_area_detector_default_set_parameter_handler(ad);
		break;
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_radicon_taurus_command( MX_RADICON_TAURUS *radicon_taurus, char *command,
			char *response, size_t response_buffer_length,
			mx_bool_type debug_flag )
{
	static const char fname[] = "mxd_radicon_taurus_command()";

	MX_RECORD *serial_port_record;
	unsigned long num_bytes_available;
	char c;
	mx_status_type mx_status;

	if ( radicon_taurus == (MX_RADICON_TAURUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RADICON_TAURUS pointer passed was NULL." );
	}

	serial_port_record = radicon_taurus->serial_port_record;

	if ( serial_port_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The serial_port_record pointer for record '%s' is NULL.",
			radicon_taurus->record->name );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, radicon_taurus->record->name ));
	}

	mx_status = mx_rs232_putline( serial_port_record, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (response == NULL) || (response_buffer_length == 0) ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = mx_rs232_getline( serial_port_record,
				response, response_buffer_length, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, response, radicon_taurus->record->name));
		}
	}

	/* If there is a single character left to be read from the
	 * serial port, then that should be a LF character.  Read
	 * that character out and discard it.
	 */

	mx_status = mx_rs232_num_input_bytes_available( serial_port_record,
							&num_bytes_available );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_bytes_available == 1 ) {
		mx_status = mx_rs232_getchar( serial_port_record,
						&c, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( c != MX_LF ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"A single character %#x was discarded for '%s', "
			"but it was not a LF character.",
				c, radicon_taurus->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}


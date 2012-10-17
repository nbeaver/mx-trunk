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

#define MXD_RADICON_TAURUS_DEBUG				FALSE

#define MXD_RADICON_TAURUS_DEBUG_RS232				TRUE

#define MXD_RADICON_TAURUS_DEBUG_RS232_SRO_SI			TRUE

#define MXD_RADICON_TAURUS_DEBUG_RS232_DELAY			FALSE

#define MXD_RADICON_TAURUS_DEBUG_EXTENDED_STATUS		FALSE

#define MXD_RADICON_TAURUS_DEBUG_EXTENDED_STATUS_WHEN_BUSY	FALSE

#define MXD_RADICON_TAURUS_DEBUG_EXTENDED_STATUS_WHEN_CHANGED	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_inttypes.h"
#include "mx_hrt.h"
#include "mx_ascii.h"
#include "mx_motor.h"
#include "mx_image.h"
#include "mx_rs232.h"
#include "mx_pulse_generator.h"
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
	mxd_radicon_taurus_open,
	NULL,
	NULL,
	mxd_radicon_taurus_resynchronize,
	mxd_radicon_taurus_special_processing_setup
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_radicon_taurus_ad_function_list = {
	mxd_radicon_taurus_arm,
	mxd_radicon_taurus_trigger,
	NULL,
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

static mx_status_type mxd_radicon_taurus_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );

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
	long vinput_framesize[2];
	long raw_framesize[2];
	long i;
	char c;
	double serial_delay_in_seconds;
	unsigned long mask, num_bytes_available;
	char command[100];
	char response[100];
	char *string_value_ptr;
	unsigned long flags;
	mx_bool_type response_seen;
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
	/* If a pulse generator record name has been specified in the 
	 * record description, then try to get a pointer to that record.
	 */

	radicon_taurus->poll_pulser_status = FALSE;

	if ( strlen( radicon_taurus->pulser_record_name ) <= 0 ) {
		radicon_taurus->pulser_record = NULL;

	} else {
		radicon_taurus->pulser_record = mx_get_record( ad->record,
					radicon_taurus->pulser_record_name );

		if ( radicon_taurus->pulser_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"The pulse generator record '%s' specified for "
			"area detector '%s' was not found in the database.",
				radicon_taurus->pulser_record_name,
				ad->record->name );
		}

		if ( radicon_taurus->pulser_record->mx_class
			!= MXC_PULSE_GENERATOR )
		{
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The record '%s' specified as a pulse generator "
			"for area detector '%s' is not actually a "
			"pulse generator.",
				radicon_taurus->pulser_record->name,
				ad->record->name );

			radicon_taurus->pulser_record = NULL;

			return mx_status;
		}

		radicon_taurus->poll_pulser_status = TRUE;
	}

	if ( radicon_taurus->pulser_record == (MX_RECORD *) NULL ) {
		/* If there is no pulser record, then we must set the
		 * poll_pulser_status field to read only.  If we allowed
		 * the field to be writeable and someone set the field
		 * to TRUE, then the MX process would crash the next time
		 * the get_extended_status field was read.
		 */

		MX_RECORD_FIELD *poll_field;

		mx_status = mx_find_record_field( record,
					"poll_pulser_status", &poll_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_record_field_set_read_only_flag( poll_field,
								TRUE );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		/* If there _is_ a pulser record, but we are running in
		 * an MX server, then we turn off the poll_pulser_status
		 * flag because the pulse generator record should provide
		 * a callback (if needed) that handles any necessary
		 * background processing.  The most relevant example of this
		 * is the 'digital_output_pulser' driver, which provides a
		 * callback to perform background processing that turns on
		 * or off the digital output value as necessary.
		 */

		MX_LIST_HEAD *list_head;

		list_head = mx_get_record_list_head_struct( record );

		if ( list_head->is_server == TRUE ) {
			radicon_taurus->poll_pulser_status = FALSE;
		}
	}

	MX_DEBUG(-2,("%s: radicon_taurus->poll_pulser_status = %d",
		fname, (int) radicon_taurus->poll_pulser_status ));

	/* Verify that the video input and serial port records
	 * have been found.
	 */

	video_input_record = radicon_taurus->video_input_record;

	if ( video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The video_input_record pointer for detector '%s' is NULL.",
			record->name );
	}

	serial_port_record = radicon_taurus->serial_port_record;

	if ( serial_port_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The serial_port_record pointer for detector '%s' is NULL.",
			record->name );
	}

	/* Configure the serial port as needed by the Taurus driver. */

	mx_status = mx_rs232_set_configuration( serial_port_record,
						115200, 8, 'N', 1, 'S',
						0x0d0a, 0x0d );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any leftover garbage characters in the serial port buffers.*/

	mx_status = mx_rs232_discard_unwritten_output( serial_port_record,
					MXD_RADICON_TAURUS_DEBUG_RS232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( serial_port_record,
					MXD_RADICON_TAURUS_DEBUG_RS232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Setup some variables to limit the rate at which commands are
	 * sent to the Taurus serial port.
	 */

	serial_delay_in_seconds = 0.5;

	radicon_taurus->serial_delay_ticks =
		mx_convert_seconds_to_clock_ticks( serial_delay_in_seconds );

	radicon_taurus->next_serial_command_tick = mx_current_clock_tick();

	/* Request camera parameters from the detector. */

	mx_status = mxd_radicon_taurus_command( radicon_taurus, "gcp",
				NULL, 0, MXD_RADICON_TAURUS_DEBUG_RS232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Give the detector a little while to respond to the GCP command. */

	mx_msleep(500);

	radicon_taurus->detector_model = 0;
	radicon_taurus->serial_number[0] = '\0';
	radicon_taurus->firmware_version = 0;
	response_seen = FALSE;

	/* Loop through the responses from the detector to the GCP command. */

	while (1) {
		mx_status = mx_rs232_num_input_bytes_available(
					radicon_taurus->serial_port_record,
					&num_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_bytes_available <= 1 ) {
			if ( response_seen == FALSE ) {
				return mx_error( MXE_NOT_READY, fname,
				"Area detector '%s' did not respond to a 'gcp' "
				"command.  Perhaps the detector is turned off?",
					record->name );
			}

			break;		/* Exit the while() loop. */
		}

		response_seen = TRUE;

		mx_status = mx_rs232_getline(radicon_taurus->serial_port_record,
					response, sizeof(response), NULL,
					MXD_RADICON_TAURUS_DEBUG_RS232 );

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
			strlcpy( radicon_taurus->serial_number,
				string_value_ptr,
				MXU_RADICON_TAURUS_SERIAL_NUMBER_LENGTH );
		} else
		if ( strncmp( response, "Camera S/N", 10 ) == 0 ) {
			strlcpy( radicon_taurus->serial_number,
				string_value_ptr,
				MXU_RADICON_TAURUS_SERIAL_NUMBER_LENGTH );
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

		if ( radicon_taurus->detector_model == 0 ) {
			mx_warning( "Did not see the detector model in the "
			"response to a GCP command sent to detector '%s'.",
				record->name );
		}
	}

#if 0
	/* If present, delete the camera's ">" prompt for the next command. */

	if ( num_bytes_available == 1 ) {
		mx_status = mx_rs232_getchar(
				radicon_taurus->serial_port_record,
				&c, MXD_RADICON_TAURUS_DEBUG_RS232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
#endif

	/* Do this firmware have the getxxx commands for reading back
	 * the sro, si1, and si2 settings?
	 */

	if ( radicon_taurus->firmware_version >= 105 ) {
		radicon_taurus->have_get_commands = TRUE;
	} else {
		radicon_taurus->have_get_commands = FALSE;
	}

	/* Set the detector gain to 1. */

	if ( radicon_taurus->firmware_version >= 105 ) {
		mx_status = mxd_radicon_taurus_command( radicon_taurus,
				"i2caw 72 1 0", NULL, 0, 
				MXD_RADICON_TAURUS_DEBUG_RS232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*--- Initialize the detector by putting it into free-run mode. ---*/

	radicon_taurus->sro_mode = 4;

	mx_status = mxd_radicon_taurus_set_sro( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off vertical flipping of the image by default. */

	flags = radicon_taurus->radicon_taurus_flags;

	if ( flags & MXF_RADICON_TAURUS_FLIP_IMAGE ) {
		radicon_taurus->flip_image = TRUE;
	} else {
		radicon_taurus->flip_image = FALSE;
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

	/* The maximum framesize is smaller than the raw frame from the
	 * video card, since the outermost pixels in the raw image do not 
	 * have real data in them.
	 */

	raw_framesize[0] = 2848;
	raw_framesize[1] = 2964;

	if ( radicon_taurus->use_raw_frames ) {
		ad->maximum_framesize[0] = raw_framesize[0];
		ad->maximum_framesize[1] = raw_framesize[1];
	} else {
		ad->maximum_framesize[0] = 2820;
		ad->maximum_framesize[1] = 2952;
	}

	ad->framesize[0] = ad->maximum_framesize[0];
	ad->framesize[1] = ad->maximum_framesize[1];

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	/* Compute and set the linetime. */

	if ( radicon_taurus->firmware_version < 105 ) {
		radicon_taurus->linetime = 946;
	} else {
		/* 677 gives us a 30 Hz frame rate. */

		radicon_taurus->linetime = 677;

		snprintf( command, sizeof(command),
			"linetime %lu", radicon_taurus->linetime );

		mx_status = mxd_radicon_taurus_command( radicon_taurus,
					command, NULL, 0,
					MXD_RADICON_TAURUS_DEBUG_RS232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Compute the readout time from the linetime and the framesize. */

	radicon_taurus->clock_frequency = 30.0e6;

	radicon_taurus->readout_time = ( raw_framesize[1] / 2 )
		* radicon_taurus->linetime / radicon_taurus->clock_frequency;

	/*---*/

	radicon_taurus->si1_si2_ratio = 0.1;

	radicon_taurus->use_different_si2_value = FALSE;

	/* Seed the registers with the minimum legal values. */

	radicon_taurus->si1_register = 4;
	radicon_taurus->si2_register = 4;

	/* If possible, fetch the current settings from the real registers. */

	mx_status = mxd_radicon_taurus_get_si1( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_radicon_taurus_get_si2( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	radicon_taurus->bypass_arm = FALSE;

	radicon_taurus->old_total_num_frames = -1;
	radicon_taurus->old_status = 0xffffffff;

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

	ad->bytes_per_frame = mx_round( ad->framesize[0] * ad->framesize[1]
						* ad->bytes_per_pixel );

	/* The detector will default to external triggering. */

	mx_status = mx_area_detector_set_trigger_mode( record,
						MXT_IMAGE_EXTERNAL_TRIGGER );
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

	/* raw_frame is used to contain the data returned by
	 * the video capture card, so raw_frame must have the
	 * same dimensions as the video capture card's frame.
	 */

	mx_status = mx_video_input_get_framesize( video_input_record,
						&vinput_framesize[0],
						&vinput_framesize[1] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate space for the raw frame. */

	mx_status = mx_image_alloc( &(radicon_taurus->raw_frame),
					vinput_framesize[0],
					vinput_framesize[1],
					ad->image_format,
					ad->byte_order,
					ad->bytes_per_pixel,
					ad->header_length,
					ad->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MXIF_ROW_BINSIZE(radicon_taurus->raw_frame) = 1;
	MXIF_COLUMN_BINSIZE(radicon_taurus->raw_frame) = 1;
	MXIF_BITS_PER_PIXEL(radicon_taurus->raw_frame) = ad->bits_per_pixel;

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

/*----*/

MX_EXPORT mx_status_type
mxd_radicon_taurus_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_radicon_taurus_resynchronize()";

	MX_AREA_DETECTOR *ad;
	MX_RADICON_TAURUS *radicon_taurus;
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
	/* Putting the detector back into free-run mode will reset it. */

	radicon_taurus->sro_mode = 4;

	mx_status = mxd_radicon_taurus_set_sro( ad );

	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mxd_radicon_taurus_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_radicon_taurus_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_RADICON_TAURUS_SRO:
		case MXLV_RADICON_TAURUS_SI1:
		case MXLV_RADICON_TAURUS_SI2:
			record_field->process_function
					= mxd_radicon_taurus_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

static mx_status_type
mxd_radicon_taurus_generate_throwaway_frame( MX_AREA_DETECTOR *ad,
					MX_RADICON_TAURUS *radicon_taurus )
{
	static const char fname[] =
			"mxd_radicon_taurus_generate_throwaway_frame()";

	unsigned long i, max_number_of_loops, sleep_milliseconds;
	mx_bool_type busy;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	/* This function should only be called in internal trigger mode. */

	/* Reprogram SI1 and SI2 to generate a short frame. */

	radicon_taurus->si1_register = 500;

	mx_status = mxd_radicon_taurus_set_si1( ad ); 

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	radicon_taurus->si2_register = 500;

	mx_status = mxd_radicon_taurus_set_si2( ad ); 

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Program the pulse generator to send a short pulse. */

	mx_status = mx_pulse_generator_set_mode( radicon_taurus->pulser_record,
							MXF_PGN_PULSE );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_pulse_period(
					radicon_taurus->pulser_record, 0.1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_pulse_width(
					radicon_taurus->pulser_record, 0.05 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the pulse. */

	mx_status = mx_pulse_generator_start( radicon_taurus->pulser_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the pulse to complete. */

	max_number_of_loops = 100;
	sleep_milliseconds = 10;

	for ( i = 0; i < max_number_of_loops; i++ ) {

		mx_status = mx_pulse_generator_is_busy(
						radicon_taurus->pulser_record,
						&busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy == FALSE )
			break;		/* Exit the for() loop. */

		mx_msleep( sleep_milliseconds );
	}

	mx_status = mx_pulse_generator_stop( radicon_taurus->pulser_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_stop( ad->record );

	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mxd_radicon_taurus_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_arm()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	MX_RECORD *video_input_record = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time, exposure_time_offset, raw_exposure_time;
	unsigned long raw_exposure_time_32;
	uint64_t long_exposure_time_as_uint64;
	double long_exposure_time_as_double;
	double short_exposure_time_as_double;
	uint64_t si1_register, si2_register;
	char command[80];
	mx_bool_type set_exposure_times;
	mx_bool_type use_different_si2_value;
	unsigned long old_sro_mode;
	unsigned long ad_flags;
	mx_bool_type enable_overrun_checking;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	video_input_record = radicon_taurus->video_input_record;

	if ( video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The video_input_record pointer for detector '%s' is NULL.",
			ad->record->name );
	}

	/* First see if we need to turn on buffer overrun checking.  We
	 * only do this if this is a server that is automatically saving
	 * files to disk, since otherwise it is impossible to guarantee
	 * saving of the files on a timely basis.
	 */

	ad_flags = ad->area_detector_flags;

	if ( ad_flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION ) {
		enable_overrun_checking = TRUE;
	} else {
		enable_overrun_checking = FALSE;
	}

	mx_status = mx_video_input_check_for_buffer_overrun(
						video_input_record,
						enable_overrun_checking );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

        /*****************************************************************
	 * If the bypass_arm flag is set, we just tell the video capture *
	 * card to get ready to receive frames and skip reprogramming of *
	 * the detector head.                                            *
         *****************************************************************/

	if ( radicon_taurus->bypass_arm ) {
		mx_status = mx_video_input_arm( video_input_record );

		return mx_status;
	}

	/*--- Otherwise, we continue on to reprogram the detector head. ---*/

	/* Set the exposure time for this sequence. */

	sp = &(ad->sequence_parameters);

	mx_status = mx_sequence_get_exposure_time( sp, 0, &exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( radicon_taurus->detector_model ) {
	case MXT_RADICON_TAURUS:

		old_sro_mode = radicon_taurus->sro_mode;
		
		/**** Set the Taurus Readout Mode. ****/
		
		/* The correct value for the readout mode depends on
		 * the sequence type.
		 */

		set_exposure_times = FALSE;

		use_different_si2_value =
			radicon_taurus->use_different_si2_value;

		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			set_exposure_times = TRUE;
			use_different_si2_value = FALSE;

			radicon_taurus->sro_mode = 3;
			break;
		case MXT_SQ_MULTIFRAME:
			set_exposure_times = TRUE;
			use_different_si2_value = FALSE;

			radicon_taurus->sro_mode = 4;
			break;
		case MXT_SQ_STROBE:
			set_exposure_times = TRUE;

			if ( use_different_si2_value ) {
				radicon_taurus->sro_mode = 0;
			} else {
				radicon_taurus->sro_mode = 1;
			}
			break;
		case MXT_SQ_DURATION:
			set_exposure_times = FALSE;
			use_different_si2_value = FALSE;

			radicon_taurus->sro_mode = 2;
			break;
		case MXT_SQ_GATED:
			set_exposure_times = TRUE;

			/* Note: This mode does not change the value of
			 * the use_different_si2_value flag, since both
			 * possible values are valid.
			 */

			radicon_taurus->sro_mode = 0;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"MX imaging sequence type %lu is not supported "
			"for external triggering with detector '%s'.",
				sp->sequence_type, ad->record->name );
		}

		if ( radicon_taurus->sro_mode != old_sro_mode ) {

			mx_status = mxd_radicon_taurus_set_sro( ad );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* When you change SRO modes, the first sequence
			 * after the change sometimes does not operate
			 * correctly.  With an external trigger, there
			 * is little that we can do to fix this.  However,
			 * with an internal trigger, we can generate a
			 * throwaway frame that we can then discard.
			 */

#if 1
			if ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {
				mx_status =
				    mxd_radicon_taurus_generate_throwaway_frame(
					ad, radicon_taurus );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
#endif
		}

		/* Do we need to set the exposure times?  If not, then
		 * we can skip the rest of this case.
		 */

		if ( set_exposure_times == FALSE ) {
			break;	/* Exit the switch() detector model block. */
		}

		/* If we get here, we have to set up the exposure times. */

		/* If use_different_si2_value is FALSE, then we compute
		 * a single raw exposure time as a 64-bit integer which
		 * is stored below in the variable long_exposure_time_as_uint64.
		 *
		 * If use_different_si2_value is TRUE, then we compute
		 * both a long exposure time and a short exposure time
		 * using a settable ratio of the two.  Then, we assign
		 * the short exposure time to si1 and the long exposure
		 * time to si2.
		 */

		long_exposure_time_as_double = 
			(exposure_time - radicon_taurus->readout_time)
				 * radicon_taurus->clock_frequency;

		if ( long_exposure_time_as_double <
			radicon_taurus->minimum_exposure_ticks )
		{
			long_exposure_time_as_double =
				radicon_taurus->minimum_exposure_ticks;
		}

		long_exposure_time_as_uint64 =
			(int64_t) ( 0.5 + long_exposure_time_as_double );

		if ( use_different_si2_value ) {
			short_exposure_time_as_double =
				radicon_taurus->si1_si2_ratio
					* long_exposure_time_as_double;

			si1_register =
				(uint64_t)(short_exposure_time_as_double + 0.5);

			si2_register = long_exposure_time_as_uint64;
		} else {
			si1_register = long_exposure_time_as_uint64;
			si2_register = long_exposure_time_as_uint64;
		}

		/* Set the value of the si1 register. */

		radicon_taurus->si1_register = si1_register;

		mx_status = mxd_radicon_taurus_set_si1( ad );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the value of the si2 register. */

		radicon_taurus->si2_register = si2_register;

		mx_status = mxd_radicon_taurus_set_si2( ad );

		break;

	case MXT_RADICON_XINEOS:
		/* FIXME: The scale factor is wrong for the Xineos. */

		raw_exposure_time_32 = mx_round( 40000.0 * exposure_time );

		snprintf( command, sizeof(command),
			"SIT %lu", raw_exposure_time_32 );

		mx_status = mxd_radicon_taurus_command( radicon_taurus, command,
				NULL, 0, MXD_RADICON_TAURUS_DEBUG_RS232 );

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
				NULL, 0, MXD_RADICON_TAURUS_DEBUG_RS232 );
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

	/* Tell the video capture card to get ready for frames. */

	mx_status = mx_video_input_arm( video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_trigger()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;
	double pulse_period, pulse_width;
	unsigned long num_pulses;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	/* FIXME: The following sleep exists to insert a delay time
	 * between the trigger call and the first get extended status.
	 *
	 * There should not be a need for such a delay, but it does
	 * need to be there for some reason, at least on the new detector.
	 */

	mx_msleep(1000);

	if ( ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {

		/* If internal triggering is not enabled,
		 * return without doing anything.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	/* We need to have a pulse generator to perform internal triggering. */

	if ( radicon_taurus->pulser_record == (MX_RECORD *) NULL ) {

		mx_warning( "Internal triggering was requested for "
		"area detector '%s', but no pulse generator is configured "
		"for it.", ad->record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the exposure time for this sequence. */

	sp = &(ad->sequence_parameters);

	mx_status = mx_sequence_get_exposure_time( sp, 0, &exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Calculate the pulse width, pulse period, and number of pulses. */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		/* sro 3 - single pulse */

		pulse_width = 1.05 * exposure_time;
		pulse_period = 1.1 * pulse_width;
		num_pulses = 1;
		break;

	case MXT_SQ_MULTIFRAME:
		/* sro 4 - this is a free run mode */

		pulse_width = pulse_period = 0.0;
		num_pulses = 0;
		break;

	case MXT_SQ_STROBE:
		/* sro 1 - double pulses */

		pulse_width = 0.1;
		pulse_period = 1.1 * pulse_width;
		num_pulses = sp->parameter_array[0] / 2L;
		break;

	case MXT_SQ_DURATION:
		/* sro 2 - duration depends on width of external trigger. */

		/* Since duration mode scans are intended for use with
		 * external triggers, specifying the pulse width and
		 * period are not really provided for.  However, if you
		 * set the necessary pulse width and period parameters
		 * directly using the pulse generator record, then you
		 * can effectively generate an "external" trigger using
		 * the internal trigger hardware.
		 */

		/* The sequence does give us the number of pulses to send. */

		num_pulses = sp->parameter_array[0];
		break;

	case MXT_SQ_GATED:
		/* sro 0 - one long gate pulse for all of the frames */

		pulse_width = sp->parameter_array[2];
		pulse_period = 1.001 * pulse_width;
		num_pulses = 1;
		break;
	}

	/* If num_pulses is 0, then we will not need to start
	 * the pulse generator.
	 */

	if ( num_pulses == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Configure the pulse generator for this sequence. */

	mx_status = mx_pulse_generator_set_mode( radicon_taurus->pulser_record,
							MXF_PGN_PULSE );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if( sp->sequence_type != MXT_SQ_DURATION ) {
		mx_status = mx_pulse_generator_set_pulse_period(
						radicon_taurus->pulser_record,
						pulse_period );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_pulse_generator_set_pulse_width(
						radicon_taurus->pulser_record,
						pulse_width );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_pulse_generator_set_num_pulses(
						radicon_taurus->pulser_record,
						num_pulses );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now start the pulse generator to start the imaging sequence. */

	mx_status = mx_pulse_generator_start( radicon_taurus->pulser_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif
	return MX_SUCCESSFUL_RESULT;
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

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( radicon_taurus->pulser_record != (MX_RECORD *) NULL ) {
		mx_status = mx_pulse_generator_stop(
				radicon_taurus->pulser_record );
	}

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

	if ( vinput_status & MXSF_VIN_OVERRUN ) {
		ad->latched_status |= MXSF_AD_BUFFER_OVERRUN;
	}

	/* If a pulse generator is in use and we are _not_ in an MX server,
	 * then poll its status.  For some pulse generator drivers, this is
	 * necessary for the driver to operate correctly, since otherwise
	 * their background processing doesn't get to happen.
	 */

	if ( radicon_taurus->poll_pulser_status ) {
		mx_bool_type pulser_busy;

		mx_status = mx_pulse_generator_is_busy(
						radicon_taurus->pulser_record,
						&pulser_busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( pulser_busy ) {
			ad->status |= MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS;
		}
	}

#if MXD_RADICON_TAURUS_DEBUG_EXTENDED_STATUS

	MX_DEBUG(-2,("%s: last_frame_number = %ld, "
			"total_num_frames = %ld, status = %#lx",
			fname, ad->last_frame_number,
			ad->total_num_frames,
			ad->status));

#elif MXD_RADICON_TAURUS_DEBUG_EXTENDED_STATUS_WHEN_BUSY

	if ( ad->status & MXSF_AD_ACQUISITION_IN_PROGRESS ) {
		MX_DEBUG(-2,("%s: last_frame_number = %ld, "
			"total_num_frames = %ld, status = %#lx",
			fname, ad->last_frame_number,
			ad->total_num_frames,
			ad->status));
	}
#elif MXD_RADICON_TAURUS_DEBUG_EXTENDED_STATUS_WHEN_CHANGED

	if ( (ad->total_num_frames != radicon_taurus->old_total_num_frames)
	  || (ad->status           != radicon_taurus->old_status) )
	{
		MX_DEBUG(-2,("%s: last_frame_number = %ld, "
			"total_num_frames = %ld, status = %#lx",
			fname, ad->last_frame_number,
			ad->total_num_frames,
			ad->status));
	}
#endif

	radicon_taurus->old_total_num_frames = ad->total_num_frames;
	radicon_taurus->old_status           = ad->status;

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

	if ( radicon_taurus->use_raw_frames ) {

		/* If we get here, we will use the raw frames
		 * from the capture card directly.
		 */

		mx_status = mx_video_input_get_frame(
			radicon_taurus->video_input_record,
			ad->readout_frame, &(ad->image_frame) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		/* We generate trimmed frames instead. */

		uint16_t *raw_frame_buffer, *trimmed_frame_buffer;
		uint16_t *raw_ptr, *trimmed_ptr;
		long raw_row_framesize, raw_column_framesize;
		long trimmed_row_framesize, trimmed_column_framesize;
		long trimmed_row_bytesize;
		long trimmed_row, row_offset, column_offset;
		long flipped_trimmed_row;

		/*
		 * First, read the video capture card's frame into
		 * the raw_frame buffer.
		 */

		mx_status = mx_video_input_get_frame(
			radicon_taurus->video_input_record,
			ad->readout_frame, &(radicon_taurus->raw_frame) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		raw_row_framesize =
			MXIF_ROW_FRAMESIZE(radicon_taurus->raw_frame);

		raw_column_framesize =
			MXIF_COLUMN_FRAMESIZE(radicon_taurus->raw_frame);

		trimmed_row_framesize    = ad->framesize[0];
		trimmed_column_framesize = ad->framesize[1];

		trimmed_row_bytesize = mx_round( 
			trimmed_row_framesize * ad->bytes_per_pixel );

		column_offset =
			(raw_row_framesize - trimmed_row_framesize) / 2;

		row_offset =
			(raw_column_framesize - trimmed_column_framesize) / 2;

		raw_frame_buffer = radicon_taurus->raw_frame->image_data;

		trimmed_frame_buffer = ad->image_frame->image_data;

		/* Note: In the following section, we have two slightly
		 * different versions of the code inside the for() loops,
		 * depending on whether or not we want to flip the image.
		 * We have written the code this way since putting an if()
		 * test in the body of a loop can be a performance killer.
		 */

		if ( radicon_taurus->flip_image ) {

		    /* While copying the image to the trimmed frame buffer,
		     * we _flip_ the image vertically.
		     */

		    for ( trimmed_row = 0;
			trimmed_row < trimmed_column_framesize;
			trimmed_row++ )
		    {
			trimmed_ptr = trimmed_frame_buffer
				+ trimmed_row * trimmed_row_framesize;

			flipped_trimmed_row = trimmed_column_framesize
							- trimmed_row - 1;
			
			raw_ptr = raw_frame_buffer
				+ row_offset * raw_row_framesize
				+ flipped_trimmed_row * raw_row_framesize
				+ column_offset;

			memcpy( trimmed_ptr, raw_ptr, trimmed_row_bytesize );
		    }
		} else {
		    /* Do _not_ flip the image vertically. */

		    for ( trimmed_row = 0;
			trimmed_row < trimmed_column_framesize;
			trimmed_row++ )
		    {
			trimmed_ptr = trimmed_frame_buffer
				+ trimmed_row * trimmed_row_framesize;
			
			raw_ptr = raw_frame_buffer
				+ row_offset * raw_row_framesize
				+ trimmed_row * raw_row_framesize
				+ column_offset;

			memcpy( trimmed_ptr, raw_ptr, trimmed_row_bytesize );
		    }
		}

		/* Copy the exposure time and timestamp from the raw frame. */

		MXIF_EXPOSURE_TIME_SEC( ad->image_frame )
			= MXIF_EXPOSURE_TIME_SEC( radicon_taurus->raw_frame );

		MXIF_EXPOSURE_TIME_NSEC( ad->image_frame )
			= MXIF_EXPOSURE_TIME_NSEC( radicon_taurus->raw_frame );

		MXIF_TIMESTAMP_SEC( ad->image_frame )
			= MXIF_TIMESTAMP_SEC( radicon_taurus->raw_frame );

		MXIF_TIMESTAMP_NSEC( ad->image_frame )
			= MXIF_TIMESTAMP_NSEC( radicon_taurus->raw_frame );
	}

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s complete for area detector '%s'.",
		fname, ad->record->name ));
#endif

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
		if ( radicon_taurus->use_raw_frames ) {
			mx_status = mx_video_input_get_framesize(
				video_input_record,
				&(ad->framesize[0]),
				&(ad->framesize[1]) );
		} else {
			ad->framesize[0] = ad->maximum_framesize[0];
			ad->framesize[1] = ad->maximum_framesize[1];
		}
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
		ad->bytes_per_frame =
			mx_round( ad->framesize[0] * ad->framesize[1]
					* ad->bytes_per_pixel );
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
	unsigned long sro_mode, trigger_mask;
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
				sro_mode = 1;
			} else {
				sro_mode = 2;
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
	
			if ( sro_mode != radicon_taurus->sro_mode ) {
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
					MXD_RADICON_TAURUS_DEBUG_RS232 );
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

	case MXLV_AD_MARK_FRAME_AS_SAVED:
		mx_status = mx_video_input_mark_frame_as_saved(
				video_input_record,
				ad->mark_frame_as_saved );
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

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_radicon_taurus_get_sro( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_get_sro()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	char response[100];
	char *response_ptr;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	if ( radicon_taurus->have_get_commands == FALSE ) {

		/* If this is an old detector that does not support
		 * the 'getsro' command, then all we can do is to
		 * return the value currently in the variable
		 * radicon_taurus->sro_mode.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mxd_radicon_taurus_command( radicon_taurus, "getsro",
					response, sizeof(response),
					MXD_RADICON_TAURUS_DEBUG_RS232_SRO_SI );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The current SRO mode is found after an '=' byte in the response. */

	response_ptr = strchr( response, '=' );

	if ( response_ptr == (char *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see the SRO mode in the response '%s' to the "
		"'getsro' command sent to detector '%s'.",
			response, ad->record->name );
	}

	response_ptr++;		/* Skip over the '='. */
	response_ptr++;		/* Skip over a trailing blank. */

	radicon_taurus->sro_mode = mx_string_to_unsigned_long( response_ptr );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_set_sro( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_taurus_set_sro()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	char command[100];
	int i, max_attempts;
	unsigned long new_sro_mode;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	new_sro_mode = radicon_taurus->sro_mode;

	/* If our caller has asked for an illegal SRO mode, then
	 * we must send back an error.
	 */

	if ( new_sro_mode > 4 ) {
		radicon_taurus->sro_mode = 4;

		/* Attempt to restore the existing hardware value of SRO. */

		mx_status = mxd_radicon_taurus_get_sro( ad );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Attempted to set detector SRO mode to %lu for detector '%s'.  "
		"The allowed range of values is from 0 to 4.",
			new_sro_mode, ad->record->name );
	}

	/*---*/

	max_attempts = 3;

	snprintf( command, sizeof(command), "sro %lu", new_sro_mode );

	for ( i = 0; i < max_attempts; i++ ) {
		mx_status = mxd_radicon_taurus_command( radicon_taurus,
					command, NULL, 0,
					MXD_RADICON_TAURUS_DEBUG_RS232_SRO_SI );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( radicon_taurus->have_get_commands == FALSE ) {
			/* If this is an old detector that does not support
			 * the 'getsro' command, then we cannot verify that
			 * the SRO command worked, so we just return.
			 */

			return MX_SUCCESSFUL_RESULT;
		}

		/* See if the SRO command worked. */

		mx_status = mxd_radicon_taurus_get_sro( ad );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( radicon_taurus->sro_mode == new_sro_mode ) {
			/* If we get here, we have successfully changed
			 * the SRO mode to the new requested SRO, so we
			 * can now return.
			 */

			return MX_SUCCESSFUL_RESULT;
		}
	}

	/* If we get here, then we have exceeded the maximum number of
	 * allowed attempts and must return an error to the caller.
	 */

	return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to change the sro mode to %lu for detector '%s' "
		"failed after %d attempts.",
		new_sro_mode, ad->record->name, max_attempts );
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_radicon_taurus_get_si_register( MX_AREA_DETECTOR *ad, long si_type )
{
	static const char fname[] = "mxd_radicon_taurus_get_si_register()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	MX_RECORD *serial_port_record = NULL;
	char command[100];
	char response[100];
	char *response_ptr;
	uint64_t new_si_value;
	unsigned long high_order, middle_order, low_order, temp_value;
	int i;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	serial_port_record = radicon_taurus->serial_port_record;

	if ( serial_port_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The serial_port_record pointer for record '%s' is NULL.",
			radicon_taurus->record->name );
	}

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	if ( radicon_taurus->have_get_commands == FALSE ) {

		/* If this is an old detector that does not support
		 * the 'getsi' commands, then all we can do is to
		 * return the value currently in the variable
		 * radicon_taurus->si1_register or
		 * radicon_taurus->si2_register.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	switch( si_type ) {
	case MXLV_RADICON_TAURUS_SI1:
		strlcpy( command, "getsi1", sizeof(command) );
		break;
	case MXLV_RADICON_TAURUS_SI2:
		strlcpy( command, "getsi2", sizeof(command) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Invoked with illegal si_type value %ld", si_type );
		break;
	}

	mx_status = mxd_radicon_taurus_command( radicon_taurus, command,
					NULL, 0,
					MXD_RADICON_TAURUS_DEBUG_RS232_SRO_SI );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* There should be three lines returned for this command. */

	for ( i = 0; i < 3; i++ ) {

		mx_status = mxd_radicon_taurus_command( radicon_taurus, NULL,
					response, sizeof(response),
					MXD_RADICON_TAURUS_DEBUG_RS232_SRO_SI );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		response_ptr = strchr( response, '=' );

		if ( response_ptr == (char *) NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The response '%s' to command '%s' for detector '%s' "
			"did not have an equals sign '=' in it.",
				response, command, ad->record->name );
		}

		response_ptr++;		/* Skip over the '=' character. */
		response_ptr++;		/* Skip over the trailing space char. */

		temp_value = mx_string_to_unsigned_long( response_ptr );

		switch( i ) {
		case 0:
			high_order = temp_value;
			break;
		case 1:
			middle_order = temp_value;
			break;
		case 2:
			low_order = temp_value;
			break;
		}
	}

	/* Construct the value of the register from its parts. */

	new_si_value  = (low_order & 0xffff);
	new_si_value |= (( middle_order & 0xffff ) << 16);
	new_si_value |= ( ((uint64_t) ( high_order & 0xf )) << 32);

	switch( si_type ) {
	case MXLV_RADICON_TAURUS_SI1:
		radicon_taurus->si1_register = new_si_value;
		break;
	case MXLV_RADICON_TAURUS_SI2:
		radicon_taurus->si2_register = new_si_value;
		break;
	}

#if MXD_RADICON_TAURUS_DEBUG_RS232_SRO_SI
	MX_DEBUG(-2,("%s: SI value read = %" PRIu64,
		fname, new_si_value));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_radicon_taurus_set_si_register( MX_AREA_DETECTOR *ad, long si_type )
{
	static const char fname[] = "mxd_radicon_taurus_set_si_register()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	char command[100];
	int i, max_attempts;
	uint64_t new_si_value;
	unsigned long high_order, middle_order, low_order;
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_pointers( ad,
						&radicon_taurus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_TAURUS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	switch( si_type ) {
	case MXLV_RADICON_TAURUS_SI1:
		new_si_value = radicon_taurus->si1_register;
		break;
	case MXLV_RADICON_TAURUS_SI2:
		new_si_value = radicon_taurus->si2_register;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Invoked with illegal si_type value %ld", si_type );
		break;
	}

#if MXD_RADICON_TAURUS_DEBUG_RS232_SRO_SI
	MX_DEBUG(-2,("%s: New SI value = %" PRIu64,
		fname, new_si_value));
#endif

	/*---*/

	low_order    = new_si_value & 0xffff;
	middle_order = (new_si_value >> 16) & 0xffff;
	high_order   = (new_si_value >> 32) & 0xf;

	switch( si_type ) {
	case MXLV_RADICON_TAURUS_SI1:
		snprintf( command, sizeof(command),
			"si1 %lu %lu %lu",
			high_order, middle_order, low_order );
		break;
	case MXLV_RADICON_TAURUS_SI2:
		snprintf( command, sizeof(command),
			"si2 %lu %lu %lu",
			high_order, middle_order, low_order );
		break;
	}

	/*---*/

	max_attempts = 3;

	for ( i = 0; i < max_attempts; i++ ) {
		mx_status = mxd_radicon_taurus_command( radicon_taurus,
					command, NULL, 0,
					MXD_RADICON_TAURUS_DEBUG_RS232_SRO_SI );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( radicon_taurus->have_get_commands == FALSE ) {
			/* If this is an old detector that does not support
			 * the 'getsi' commands, then we cannot verify that
			 * the SI command worked, so we just return.
			 */

			return MX_SUCCESSFUL_RESULT;
		}

		/* See if the SI command worked. */

		mx_status = mxd_radicon_taurus_get_si_register( ad, si_type );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If one of the following tests succeeds, then we have
		 * successfully changed the SI register to the new requested
		 * SI value and can now return.
		 */

		switch( si_type ) {
		case MXLV_RADICON_TAURUS_SI1:
			if ( radicon_taurus->si1_register == new_si_value ) {
				return MX_SUCCESSFUL_RESULT;
			}
			break;
		case MXLV_RADICON_TAURUS_SI2:
			if ( radicon_taurus->si2_register == new_si_value ) {
				return MX_SUCCESSFUL_RESULT;
			}
			break;
		}
	}

	/* If we get here, then we have exceeded the maximum number of
	 * allowed attempts and must return an error to the caller.
	 */

	return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to change an SI register with the command '%s' "
		"for detector '%s' failed after %d attempts.",
		command, ad->record->name, max_attempts );
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_radicon_taurus_get_si1( MX_AREA_DETECTOR *ad )
{
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_si_register( ad,
						MXLV_RADICON_TAURUS_SI1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_set_si1( MX_AREA_DETECTOR *ad )
{
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_set_si_register( ad,
						MXLV_RADICON_TAURUS_SI1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_get_si2( MX_AREA_DETECTOR *ad )
{
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_get_si_register( ad,
						MXLV_RADICON_TAURUS_SI2 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_taurus_set_si2( MX_AREA_DETECTOR *ad )
{
	mx_status_type mx_status;

	mx_status = mxd_radicon_taurus_set_si_register( ad,
						MXLV_RADICON_TAURUS_SI2 );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_radicon_taurus_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	static const char fname[] = "mxd_radicon_taurus_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AREA_DETECTOR *ad;
	MX_RADICON_TAURUS *radicon_taurus;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	ad = (MX_AREA_DETECTOR *) record->record_class_struct;
	radicon_taurus = (MX_RADICON_TAURUS *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_RADICON_TAURUS_SRO:
			mx_status = mxd_radicon_taurus_get_sro( ad );
			break;
		case MXLV_RADICON_TAURUS_SI1:
			mx_status = mxd_radicon_taurus_get_si1( ad );
			break;
		case MXLV_RADICON_TAURUS_SI2:
			mx_status = mxd_radicon_taurus_get_si2( ad );
			break;
		default:
			break;
		}
		break;

	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_RADICON_TAURUS_SRO:
			mx_status = mxd_radicon_taurus_set_sro( ad );
			break;
		case MXLV_RADICON_TAURUS_SI1:
			mx_status = mxd_radicon_taurus_set_si1( ad );
			break;
		case MXLV_RADICON_TAURUS_SI2:
			mx_status = mxd_radicon_taurus_set_si2( ad );
			break;
		default:
			break;
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unknown operation code = %d", operation );
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
	size_t num_to_discard;
	char c;
	MX_CLOCK_TICK current_tick;
	int comparison;
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

	/* Are we sending a command? */

	if ( command != NULL ) {
		/* Yes, we are sending a command. */

		/* See if we need to wait before sending the next command. */

#if MXD_RADICON_TAURUS_DEBUG_RS232_DELAY
		MX_DEBUG(-2,("%s: next_serial_command_tick = (%lu,%lu)", fname,
	  (unsigned long) radicon_taurus->next_serial_command_tick.high_order,
	  (unsigned long) radicon_taurus->next_serial_command_tick.low_order));
#endif

		while (1) {
			current_tick = mx_current_clock_tick();

			comparison = mx_compare_clock_ticks( current_tick,
				radicon_taurus->next_serial_command_tick );

#if MXD_RADICON_TAURUS_DEBUG_RS232_DELAY
			MX_DEBUG(-2,
			("%s: current_tick = (%lu,%lu), comparison = %d", fname,
				(unsigned long) current_tick.high_order,
				(unsigned long) current_tick.low_order,
				comparison));
#endif

			if ( comparison >= 0 ) {

				/* It is now time for the next command.
				 * Before breaking out of this while() loop,
				 * we must compute the new value of
				 * next_serial_command_tick.
				 */

				radicon_taurus->next_serial_command_tick =
					mx_add_clock_ticks( current_tick,
					  radicon_taurus->serial_delay_ticks );

#if MXD_RADICON_TAURUS_DEBUG_RS232_DELAY
				MX_DEBUG(-2,
			("%s: NEW next_serial_command_tick = (%lu,%lu)", fname,
	  (unsigned long) radicon_taurus->next_serial_command_tick.high_order,
	  (unsigned long) radicon_taurus->next_serial_command_tick.low_order));
#endif

				break;	/* Exit the while() loop. */
			}

			mx_msleep(10);
		}

		/* Now we can send the command. */

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: sending '%s' to '%s'",
				fname, command, radicon_taurus->record->name ));
		}

		mx_status = mx_rs232_putline( serial_port_record,
						command, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Are we reading a response? */

	if ( (response == NULL) || (response_buffer_length == 0) ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		/* Yes, we are reading a response. */

		mx_status = mx_rs232_getline( serial_port_record,
				response, response_buffer_length, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Are there any leading characters that we want to discard? */

		num_to_discard = strspn( response, ">\n" );

#if 0
		MX_DEBUG(-2,("%s: num_to_discard = %lu",
				fname, (unsigned long) num_to_discard ));
#endif

		if ( num_to_discard > 0 ) {
			memmove( response, response + num_to_discard,
				response_buffer_length - num_to_discard );
		}

		/*---*/

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, response, radicon_taurus->record->name));
		}
	}

#if 0
	/* If there is a single character left to be read from the
	 * serial port, then that should be a LF character or a '>'
	 * character.  Read that character out and discard it.
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

		if ( ( c != MX_LF )
		  && ( c != '>' ) )
		{
			(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"A single character %#x was discarded for '%s', "
			"but it was not a LF character or a '>' character.",
				c, radicon_taurus->record->name );
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}


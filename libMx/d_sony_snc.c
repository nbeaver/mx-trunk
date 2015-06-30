/*
 * Name:    d_sony_snc.c
 *
 * Purpose: MX driver for Sony Network Camera video inputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SONY_SNC_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_socket.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "d_sony_snc.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_sony_snc_record_function_list = {
	NULL,
	mxd_sony_snc_create_record_structures,
	mxd_sony_snc_finish_record_initialization,
	NULL,
	NULL,
	mxd_sony_snc_open
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_sony_snc_video_input_function_list = {
	mxd_sony_snc_arm,
	mxd_sony_snc_trigger,
	mxd_sony_snc_stop,
	mxd_sony_snc_abort,
	mxd_sony_snc_asynchronous_capture,
	NULL,
	NULL,
	NULL,
	mxd_sony_snc_get_extended_status,
	mxd_sony_snc_get_frame,
	mxd_sony_snc_get_parameter,
	mxd_sony_snc_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_sony_snc_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_SONY_SNC_STANDARD_FIELDS
};

long mxd_sony_snc_num_record_fields
		= sizeof( mxd_sony_snc_record_field_defaults )
			/ sizeof( mxd_sony_snc_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sony_snc_rfield_def_ptr
			= &mxd_sony_snc_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_sony_snc_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_SONY_SNC **sony_snc,
			const char *calling_fname )
{
	static const char fname[] = "mxd_sony_snc_get_pointers()";

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (sony_snc == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_SONY_SNC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*sony_snc = (MX_SONY_SNC *) vinput->record->record_type_struct;

	if ( *sony_snc == (MX_SONY_SNC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_SONY_SNC pointer for record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_sony_snc_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_sony_snc_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_SONY_SNC *sony_snc = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	sony_snc = (MX_SONY_SNC *)
				malloc( sizeof(MX_SONY_SNC) );

	if ( sony_snc == (MX_SONY_SNC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_SONY_SNC structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = sony_snc;
	record->class_specific_function_list = 
			&mxd_sony_snc_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	sony_snc->record = record;

	vinput->trigger_mode = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sony_snc_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_sony_snc_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_SONY_SNC *sony_snc = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_sony_snc_get_pointers( vinput, &sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_video_input_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_snc_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sony_snc_open()";

	MX_VIDEO_INPUT *vinput;
	MX_SONY_SNC *sony_snc = NULL;
	long pixels_per_frame;
	char buffer[100];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_sony_snc_get_pointers( vinput, &sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* Initialize a bunch of driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = (long) mx_native_byteorder();
	vinput->trigger_mode   = MXT_IMAGE_NO_TRIGGER;

	vinput->total_num_frames = 0;

	sony_snc->old_total_num_frames = 0;

	switch( vinput->image_format ) {
	case MXT_IMAGE_FORMAT_RGB565:
	case MXT_IMAGE_FORMAT_YUYV:
		vinput->bytes_per_pixel = 2;
		vinput->bits_per_pixel  = 16;
		break;

	case MXT_IMAGE_FORMAT_RGB:
		vinput->bytes_per_pixel = 3;
		vinput->bits_per_pixel  = 24;
		break;
	case MXT_IMAGE_FORMAT_GREY8:
		vinput->bytes_per_pixel = 1;
		vinput->bits_per_pixel  = 8;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		vinput->bytes_per_pixel = 2;
		vinput->bits_per_pixel  = 16;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image format %ld '%s' is not a supported image format "
		"for software-emulated video input '%s'.",
			vinput->image_format, vinput->image_format_name,
			record->name );
	}

	pixels_per_frame = vinput->framesize[0] * vinput->framesize[1];

	vinput->bytes_per_frame = pixels_per_frame
					* mx_round( vinput->bytes_per_pixel );

	sony_snc->camera_socket = NULL;


	/* Verify that the controller is there by asking for
	 * its serial number.
	 */

	mx_status = mxd_sony_snc_send_command( sony_snc,
					"POST /command/sysinfo.cgi?Serial",
					NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_getline( sony_snc->camera_socket,
					buffer, sizeof(buffer), "\r\n" );

	MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sony_snc_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sony_snc_arm()";

	MX_SONY_SNC *sony_snc = NULL;
	MX_SEQUENCE_PARAMETERS *seq;
	mx_status_type mx_status;

	mx_status = mxd_sony_snc_get_pointers( vinput, &sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	sony_snc->old_total_num_frames = vinput->total_num_frames;

	seq = &(vinput->sequence_parameters);

#if 1
	MXW_UNUSED( seq );
#else
	switch( seq->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
		sony_snc->seconds_per_frame = seq->parameter_array[0];
		sony_snc->num_frames_in_sequence = 1;
		break;
	case MXT_SQ_MULTIFRAME:
		sony_snc->seconds_per_frame = seq->parameter_array[2];
		sony_snc->num_frames_in_sequence =
					mx_round( seq->parameter_array[0] );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported sequence type %lu requested for video input '%s'.",
			seq->sequence_type, vinput->record->name );
		break;
	}

	switch( vinput->trigger_mode ) {
	case MXT_IMAGE_INTERNAL_TRIGGER:
		sony_snc->sequence_in_progress = FALSE;
		break;
	case MXT_IMAGE_EXTERNAL_TRIGGER:
		sony_snc->sequence_in_progress = TRUE;
		sony_snc->start_tick = mx_current_clock_tick();
		break;
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_snc_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sony_snc_trigger()";

	MX_SONY_SNC *sony_snc = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sony_snc_get_pointers( vinput, &sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	sony_snc->sequence_in_progress = TRUE;
	sony_snc->start_tick = mx_current_clock_tick();

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_snc_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sony_snc_stop()";

	MX_SONY_SNC *sony_snc = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sony_snc_get_pointers( vinput, &sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	sony_snc->sequence_in_progress = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_snc_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sony_snc_abort()";

	MX_SONY_SNC *sony_snc = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sony_snc_get_pointers( vinput, &sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	sony_snc->sequence_in_progress = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_snc_asynchronous_capture( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sony_snc_asynchronous_capture()";

	MX_SONY_SNC *sony_snc = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sony_snc_get_pointers( vinput, &sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	sony_snc->sequence_in_progress = TRUE;
	sony_snc->start_tick = mx_current_clock_tick();

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_snc_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sony_snc_get_extended_status()";

	MX_SONY_SNC *sony_snc = NULL;
	MX_CLOCK_TICK clock_ticks_since_start;
	double seconds_since_start, frames_since_start_dbl;
	long frames_since_start;
	mx_status_type mx_status;

	mx_status = mxd_sony_snc_get_pointers( vinput,
						&sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sony_snc->sequence_in_progress == FALSE ) {
		vinput->status = 0;
	} else {
		clock_ticks_since_start = mx_subtract_clock_ticks(
			mx_current_clock_tick(), sony_snc->start_tick );

		seconds_since_start = mx_convert_clock_ticks_to_seconds(
						clock_ticks_since_start );

		frames_since_start_dbl = mx_divide_safely( seconds_since_start,
						sony_snc->seconds_per_frame);

		frames_since_start = mx_round( frames_since_start_dbl );

		switch( vinput->sequence_parameters.sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			if ( frames_since_start < 1 ) {
				vinput->status = MXSF_VIN_IS_BUSY;
				vinput->last_frame_number = -1;
				vinput->total_num_frames =
					sony_snc->old_total_num_frames;
			} else {
				vinput->status = 0;
				vinput->last_frame_number = 0;
				vinput->total_num_frames =
					sony_snc->old_total_num_frames + 1;

				sony_snc->sequence_in_progress = FALSE;
			}
			break;

		case MXT_SQ_CONTINUOUS:
			vinput->status = MXSF_VIN_IS_BUSY;
			vinput->last_frame_number = 0;
			vinput->total_num_frames =
		    sony_snc->old_total_num_frames + frames_since_start;

		    	break;

		case MXT_SQ_MULTIFRAME:
			if ( frames_since_start <
				sony_snc->num_frames_in_sequence )
			{
				vinput->status = MXSF_VIN_IS_BUSY;
			} else {
				vinput->status = 0;
				sony_snc->sequence_in_progress = FALSE;
			}

			vinput->last_frame_number = frames_since_start - 1;

			vinput->total_num_frames =
					sony_snc->old_total_num_frames
						+ frames_since_start;
			break;
		}
	}

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
		fname, vinput->last_frame_number, vinput->total_num_frames,
		vinput->status));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_snc_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sony_snc_get_frame()";

	MX_SONY_SNC *sony_snc = NULL;
	MX_IMAGE_FRAME *frame;
	mx_status_type mx_status;

	sony_snc = NULL;

	mx_status = mxd_sony_snc_get_pointers( vinput,
						&sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	frame->image_length    = vinput->bytes_per_frame;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,
	("%s: successfully read a %lu byte image frame from video input '%s'.",
		fname, (unsigned long) frame->image_length,
		vinput->record->name ));

	{
		int n;
		unsigned char c;
		unsigned char *frame_buffer;

		frame_buffer = frame->image_data;

		MX_DEBUG(-2,("%s: frame = %p, frame_buffer = %p",
			fname, frame, frame_buffer));

		for ( n = 0; n < 10; n++ ) {
			c = frame_buffer[n];

			MX_DEBUG(-2,("%s: frame_buffer[%d] = %u", fname, n, c));
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sony_snc_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sony_snc_get_parameter()";

	MX_SONY_SNC *sony_snc = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sony_snc_get_pointers( vinput,
						&sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
#if MXD_SONY_SNC_DEBUG
		MX_DEBUG(-2,("%s: video format = %ld, format name = '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
#endif
		break;

	case MXLV_VIN_BYTE_ORDER:
		vinput->byte_order = (long) mx_native_byteorder();
		break;

	case MXLV_VIN_TRIGGER_MODE:
		vinput->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel
			* vinput->framesize[0] * vinput->framesize[1] );
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		switch( vinput->image_format ) {
		case MXT_IMAGE_FORMAT_RGB:
			vinput->bytes_per_pixel = 3;
			vinput->bits_per_pixel  = 24;
			break;
		case MXT_IMAGE_FORMAT_GREY8:
			vinput->bytes_per_pixel = 1;
			vinput->bits_per_pixel  = 8;
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			vinput->bytes_per_pixel = 2;
			vinput->bits_per_pixel  = 16;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported image format %ld for video input '%s'.",
				vinput->image_format, vinput->record->name );
		}
		break;

	case MXLV_VIN_BUSY:
		vinput->busy = 0;
		break;

	case MXLV_VIN_STATUS:
		vinput->status = 0;
		break;

	case MXLV_VIN_SEQUENCE_TYPE:
		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		break;
	default:
		mx_status =
			mx_video_input_default_get_parameter_handler( vinput );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_snc_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_sony_snc_set_parameter()";

	MX_SONY_SNC *sony_snc = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sony_snc_get_pointers( vinput,
						&sony_snc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the image format is not supported for "
			"video input '%s'.", vinput->record->name );

	case MXLV_VIN_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the byte order for video input '%s' "
			"is not supported.", vinput->record->name );

	case MXLV_VIN_TRIGGER_MODE:
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Directly changing the number of bytes per frame "
			"for video input '%s' is not supported.",
				vinput->record->name );

	case MXLV_VIN_SEQUENCE_TYPE:
		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		break;

	default:
		mx_status =
			mx_video_input_default_set_parameter_handler( vinput );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_sony_snc_send_command( MX_SONY_SNC *sony_snc,
				char *command,
				char *data )
{
	static const char fname[] = "mxd_sony_snc_send_command()";

	char buffer[80];
	mx_status_type mx_status;

	if ( sony_snc == (MX_SONY_SNC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SONY_SNC pointer passed was NULL." );
	}

	mx_status = mxd_sony_snc_update_socket( sony_snc );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_SNC_DEBUG
	MX_DEBUG(-2,("%s: sending command = '%s', data = '%s'",
		fname, command, data));
#endif

	/* Send the request to the socket. */

	mx_status = mx_socket_putline( sony_snc->camera_socket,
						command, "\r\n" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( buffer, sizeof(buffer), "Host: %s", sony_snc->hostname );

	mx_status = mx_socket_putline( sony_snc->camera_socket,
						buffer, "\r\n" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_putline( sony_snc->camera_socket,
					"Connection: Keep-Alive", "\r\n" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_putline( sony_snc->camera_socket,
					"Cache-Control: no-cache", "\r\n" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_putline( sony_snc->camera_socket,
					"Content-Length: 16", "\r\n" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_putline( sony_snc->camera_socket,
					"", "\r\n" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( data != NULL ) {
		mx_status = mx_socket_putline( sony_snc->camera_socket,
						data, "\r\n" );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_sony_snc_update_socket( MX_SONY_SNC *sony_snc )
{
	static const char fname[] = "mxd_sony_snc_update_socket()";

	mx_status_type mx_status;

	if ( sony_snc == (MX_SONY_SNC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SONY_SNC pointer passed was NULL." );
	}

	if ( mx_socket_is_open( sony_snc->camera_socket ) ) {
		return MX_SUCCESSFUL_RESULT;
	}

	(void) mx_socket_close( sony_snc->camera_socket );

	mx_status = mx_tcp_socket_open_as_client( &(sony_snc->camera_socket),
						sony_snc->hostname,
						sony_snc->port_number,
						0x0, 0 );

	return mx_status;
}


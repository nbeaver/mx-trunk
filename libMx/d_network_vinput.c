/*
 * Name:    d_network_vinput.c
 *
 * Purpose: MX network video input device driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_VINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_hrt.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "d_network_vinput.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_network_vinput_record_function_list = {
	NULL,
	mxd_network_vinput_create_record_structures,
	mxd_network_vinput_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_network_vinput_open,
	mxd_network_vinput_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_network_vinput_video_input_function_list = {
	mxd_network_vinput_arm,
	mxd_network_vinput_trigger,
	mxd_network_vinput_stop,
	mxd_network_vinput_abort,
	mxd_network_vinput_continuous_capture,
	mxd_network_vinput_get_last_frame_number,
	mxd_network_vinput_get_total_num_frames,
	mxd_network_vinput_get_status,
	mxd_network_vinput_get_extended_status,
	mxd_network_vinput_get_frame,
	mxd_network_vinput_get_parameter,
	mxd_network_vinput_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_network_vinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_NETWORK_VINPUT_STANDARD_FIELDS
};

long mxd_network_vinput_num_record_fields
		= sizeof( mxd_network_vinput_record_field_defaults )
			/ sizeof( mxd_network_vinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_vinput_rfield_def_ptr
			= &mxd_network_vinput_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_network_vinput_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_NETWORK_VINPUT **network_vinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_vinput_get_pointers()";

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (network_vinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_NETWORK_VINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*network_vinput = (MX_NETWORK_VINPUT *)
				vinput->record->record_type_struct;

	if ( *network_vinput == (MX_NETWORK_VINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_NETWORK_VINPUT pointer for record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_network_vinput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_vinput_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_NETWORK_VINPUT *network_vinput;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	network_vinput = (MX_NETWORK_VINPUT *)
				malloc( sizeof(MX_NETWORK_VINPUT) );

	if ( network_vinput == (MX_NETWORK_VINPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_NETWORK_VINPUT structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = network_vinput;
	record->class_specific_function_list = 
			&mxd_network_vinput_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	network_vinput->record = record;

	vinput->trigger_mode = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_vinput_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_video_input_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_vinput->abort_nf),
			network_vinput->server_record,
			"%s.abort", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->arm_nf),
			network_vinput->server_record,
			"%s.arm", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->bits_per_pixel_nf),
			network_vinput->server_record,
		"%s.bits_per_pixel", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->byte_order_nf),
			network_vinput->server_record,
			"%s.byte_order", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->bytes_per_frame_nf),
			network_vinput->server_record,
		"%s.bytes_per_frame", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->bytes_per_pixel_nf),
			network_vinput->server_record,
		"%s.bytes_per_pixel", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->camera_trigger_polarity_nf),
			network_vinput->server_record,
	    "%s.camera_trigger_polarity", network_vinput->remote_record_name);

	mx_network_field_init( &(network_vinput->continuous_capture_nf),
			network_vinput->server_record,
		"%s.continuous_capture", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->extended_status_nf),
			network_vinput->server_record,
		"%s.extended_status", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->external_trigger_polarity_nf),
			network_vinput->server_record,
	    "%s.external_trigger_polarity", network_vinput->remote_record_name);

	mx_network_field_init( &(network_vinput->framesize_nf),
			network_vinput->server_record,
			"%s.framesize", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->frame_buffer_nf),
			network_vinput->server_record,
			"%s.frame_buffer", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->get_frame_nf),
			network_vinput->server_record,
			"%s.get_frame", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->image_format_name_nf),
			network_vinput->server_record,
		"%s.image_format_name", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->image_format_nf),
			network_vinput->server_record,
			"%s.image_format", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->last_frame_number_nf),
			network_vinput->server_record,
		"%s.last_frame_number", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->maximum_frame_number_nf),
			network_vinput->server_record,
		"%s.maximum_frame_number", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->pixel_clock_frequency_nf),
			network_vinput->server_record,
	    "%s.pixel_clock_frequency", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->status_nf),
			network_vinput->server_record,
			"%s.status", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->stop_nf),
			network_vinput->server_record,
			"%s.stop", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->total_num_frames_nf),
			network_vinput->server_record,
		"%s.total_num_frames", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->trigger_nf),
			network_vinput->server_record,
			"%s.trigger", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->trigger_mode_nf),
			network_vinput->server_record,
			"%s.trigger_mode", network_vinput->remote_record_name );

	/*---*/

	mx_network_field_init( &(network_vinput->sequence_type_nf),
			network_vinput->server_record,
		"%s.sequence_type", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->num_sequence_parameters_nf),
			network_vinput->server_record,
			"%s.num_sequence_parameters",
				network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->sequence_parameter_array_nf),
			network_vinput->server_record,
			"%s.sequence_parameter_array",
				network_vinput->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_vinput_open()";

	MX_VIDEO_INPUT *vinput;
	MX_NETWORK_VINPUT *network_vinput;
	long dimension[1];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* Get the image framesize from the server. */

	dimension[0] = 2;

	mx_status = mx_get_array( &(network_vinput->framesize_nf),
				MXFT_LONG, 1, dimension,
				vinput->framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', framesize = (%ld, %ld)",
	    fname, record->name, vinput->framesize[0], vinput->framesize[1]));
#endif

	/* Get the image format name from the server. */

	dimension[0] = MXU_IMAGE_FORMAT_NAME_LENGTH;

	mx_status = mx_get_array( &(network_vinput->image_format_name_nf),
				MXFT_STRING, 1, dimension,
				vinput->image_format_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the image format type. */

	mx_status = mx_image_get_format_type_from_name(
					vinput->image_format_name,
					&(vinput->image_format) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: record '%s', image format name = '%s', image format = %ld",
		fname, record->name, vinput->image_format_name,
		vinput->image_format));
#endif

	/* Get the byte order. */

	mx_status = mx_get( &(network_vinput->byte_order_nf),
				MXFT_LONG, &(vinput->byte_order) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_arm()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	mx_status = mx_put( &(network_vinput->arm_nf),
				MXFT_BOOL, &(vinput->arm) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_trigger()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	mx_status = mx_put( &(network_vinput->trigger_nf),
				MXFT_BOOL, &(vinput->trigger) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_stop()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	mx_status = mx_put( &(network_vinput->stop_nf),
				MXFT_BOOL, &(vinput->stop) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_abort()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	mx_status = mx_put( &(network_vinput->abort_nf),
				MXFT_BOOL, &(vinput->abort) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_continuous_capture( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_continuous_capture()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	mx_status = mx_put( &(network_vinput->continuous_capture_nf),
				MXFT_LONG, &(vinput->continuous_capture) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_last_frame_number( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_network_vinput_get_last_frame_number()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	mx_status = mx_get( &(network_vinput->last_frame_number_nf),
				MXFT_HEX, &(vinput->last_frame_number) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_total_num_frames( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_network_vinput_get_total_num_frames()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	mx_status = mx_get( &(network_vinput->total_num_frames_nf),
				MXFT_HEX, &(vinput->total_num_frames) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_get_status()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	mx_status = mx_get( &(network_vinput->status_nf),
				MXFT_HEX, &(vinput->status) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_network_vinput_get_extended_status()";

	MX_NETWORK_VINPUT *network_vinput;
	long dimension[1];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VIDEO_INPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, ad->record->name ));
#endif
	dimension[0] = MXU_VIN_EXTENDED_STATUS_STRING_LENGTH;

	mx_status = mx_get_array( &(network_vinput->extended_status_nf),
			MXFT_STRING, 1, dimension, &(vinput->extended_status) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1 || MXD_NETWORK_VIDEO_INPUT_DEBUG
	MX_DEBUG(-2,("%s: vinput->extended_status = '%s'",
		fname, vinput->extended_status));
#endif

	num_items = sscanf( vinput->extended_status, "%ld %ld %lx",
				&(vinput->last_frame_number),
				&(vinput->total_num_frames),
				&(vinput->status) );

	if ( num_items != 3 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The string returned by server '%s' for record field '%s' "
		"was not parseable as an extended status string.  "
		"Returned string = '%s'",
			network_vinput->server_record->name,
			"extended_status", vinput->extended_status );
	}

#if 1 || MXD_NETWORK_VIDEO_INPUT_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
		fname, vinput->last_frame_number,
		vinput->total_num_frames, vinput->status));
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_get_frame()";

	MX_NETWORK_VINPUT *network_vinput;
	MX_IMAGE_FRAME *frame;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	/* Tell the server to prepare the frame for being read. */

	mx_status = mx_put( &(network_vinput->get_frame_nf),
				MXFT_LONG, &(vinput->frame_number) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Ask for the number of bytes per pixel. */

	mx_status = mx_get( &(network_vinput->bytes_per_pixel_nf),
				MXFT_DOUBLE, &(vinput->bytes_per_pixel) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame->bytes_per_pixel = vinput->bytes_per_pixel;

	/* Ask for the size of the image. */

	mx_status = mx_get( &(network_vinput->bytes_per_frame_nf),
				MXFT_LONG, &(vinput->bytes_per_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame->image_length = vinput->bytes_per_frame;

	/* Now read the frame into the MX_IMAGE_FRAME structure. */

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: reading a %ld byte image frame.",
			fname, (long) frame->image_length ));

	{
		char *image_data;
		long image_length;

		image_data = frame->image_data;
		image_length = frame->image_length;

		MX_DEBUG(-2,("%s: about to read image_data[%ld]",
				fname, image_length - 1));

		MX_DEBUG(-2,("%s: image_data[%ld] = %u",
			fname, image_length - 1,
			image_data[ image_length - 1 ] ));
	}
#endif

	dimension[0] = frame->image_length;

	mx_status = mx_get_array( &(network_vinput->frame_buffer_nf),
			MXFT_CHAR, 1, dimension, frame->image_data );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: successfully read a %lu byte image frame from video input '%s'.",
		fname, (unsigned long) frame->image_length,
		vinput->record->name ));

	{
		int i;
		unsigned char c;
		unsigned char *frame_buffer;

		frame_buffer = frame->image_data;

		for ( i = 0; i < 10; i++ ) {
			c = frame_buffer[i];

			MX_DEBUG(-2,("%s: frame_buffer[%d] = %u", fname, i, c));
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_get_parameter()";

	MX_NETWORK_VINPUT *network_vinput;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_MAXIMUM_FRAME_NUMBER:
		mx_status = mx_get( &(network_vinput->maximum_frame_number_nf),
				MXFT_ULONG, &(vinput->maximum_frame_number) );
		break;

	case MXLV_VIN_BYTE_ORDER:
		mx_status = mx_get( &(network_vinput->byte_order_nf),
					MXFT_LONG, &(vinput->byte_order) );
		break;

	case MXLV_VIN_CAMERA_TRIGGER_POLARITY:
		mx_status = 
		    mx_get( &(network_vinput->camera_trigger_polarity_nf),
				MXFT_LONG, &(vinput->camera_trigger_polarity) );
		break;

	case MXLV_VIN_EXTERNAL_TRIGGER_POLARITY:
		mx_status = 
		    mx_get( &(network_vinput->external_trigger_polarity_nf),
			    MXFT_LONG, &(vinput->external_trigger_polarity) );
		break;

	case MXLV_VIN_FRAMESIZE:
		dimension[0] = 2;

		mx_status = mx_get_array( &(network_vinput->framesize_nf),
				MXFT_LONG, 1, dimension, &(vinput->framesize) );
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		mx_status = mx_get( &(network_vinput->image_format_nf),
					MXFT_LONG, &(vinput->image_format) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
#if MXD_NETWORK_VINPUT_DEBUG
		MX_DEBUG(-2,("%s: video format = %ld, format name = '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
#endif
		break;

	case MXLV_VIN_TRIGGER_MODE:
		mx_status = mx_get( &(network_vinput->trigger_mode_nf),
					MXFT_LONG, &(vinput->trigger_mode) );
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		mx_status = mx_get( &(network_vinput->bytes_per_frame_nf),
					MXFT_LONG, &(vinput->bytes_per_frame) );
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		mx_status = mx_get( &(network_vinput->bytes_per_pixel_nf),
				    MXFT_DOUBLE, &(vinput->bytes_per_pixel) );
		break;

	case MXLV_VIN_BITS_PER_PIXEL:
		mx_status = mx_get( &(network_vinput->bits_per_pixel_nf),
				    MXFT_LONG, &(vinput->bits_per_pixel) );
		break;

	case MXLV_VIN_PIXEL_CLOCK_FREQUENCY:
		mx_status = mx_get( &(network_vinput->pixel_clock_frequency_nf),
				MXFT_DOUBLE, &(vinput->pixel_clock_frequency) );
		break;

	case MXLV_VIN_STATUS:
		mx_status = mx_get( &(network_vinput->status_nf),
					MXFT_HEX, &(vinput->status) );
		break;

	case MXLV_VIN_SEQUENCE_TYPE:
		mx_status = mx_get( &(network_vinput->sequence_type_nf),
		    MXFT_LONG, &(vinput->sequence_parameters.sequence_type) );

		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		mx_status = mx_get(
				&(network_vinput->num_sequence_parameters_nf),
		    MXFT_LONG, &(vinput->sequence_parameters.num_parameters) );
				
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension[0] = vinput->sequence_parameters.num_parameters;

		mx_status = mx_get_array(
				&(network_vinput->sequence_parameter_array_nf),
				MXFT_DOUBLE, 1, dimension,
				&(vinput->sequence_parameters.parameter_array));
		break;
	default:
		mx_status =
			mx_video_input_default_get_parameter_handler( vinput );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_set_parameter()";

	MX_NETWORK_VINPUT *network_vinput;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_CAMERA_TRIGGER_POLARITY:
		mx_status = 
		    mx_put( &(network_vinput->camera_trigger_polarity_nf),
				MXFT_LONG, &(vinput->camera_trigger_polarity) );
		break;

	case MXLV_VIN_EXTERNAL_TRIGGER_POLARITY:
		mx_status = 
		    mx_put( &(network_vinput->external_trigger_polarity_nf),
			    MXFT_LONG, &(vinput->external_trigger_polarity) );
		break;

	case MXLV_VIN_FRAMESIZE:

		dimension[0] = 2;

		mx_status = mx_put_array( &(network_vinput->framesize_nf),
				MXFT_LONG, 1, dimension, &(vinput->framesize) );
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the image format is not supported for "
			"video input '%s'.", vinput->record->name );

	case MXLV_VIN_PIXEL_CLOCK_FREQUENCY:
		mx_status = mx_put( &(network_vinput->pixel_clock_frequency_nf),
				MXFT_DOUBLE, &(vinput->pixel_clock_frequency) );
		break;

	case MXLV_VIN_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the byte order for video input '%s' "
			"is not supported.", vinput->record->name );

	case MXLV_VIN_TRIGGER_MODE:
		mx_status = mx_put( &(network_vinput->trigger_mode_nf),
				MXFT_LONG, &(vinput->trigger_mode) );
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Directly changing the number of bytes per frame "
			"for video input '%s' is not supported.",
				vinput->record->name );

	case MXLV_VIN_SEQUENCE_TYPE:
		mx_status = mx_put( &(network_vinput->sequence_type_nf),
		    MXFT_LONG, &(vinput->sequence_parameters.sequence_type) );

		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		mx_status = mx_put(
				&(network_vinput->num_sequence_parameters_nf),
		    MXFT_LONG, &(vinput->sequence_parameters.num_parameters) );
				
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension[0] = vinput->sequence_parameters.num_parameters;

		mx_status = mx_put_array(
				&(network_vinput->sequence_parameter_array_nf),
				MXFT_DOUBLE, 1, dimension,
				&(vinput->sequence_parameters.parameter_array));
		break;

	default:
		mx_status =
			mx_video_input_default_set_parameter_handler( vinput );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}


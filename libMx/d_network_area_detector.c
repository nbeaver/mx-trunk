/*
 * Name:    d_network_area_detector.c
 *
 * Purpose: MX network area detector device driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_AREA_DETECTOR_DEBUG	TRUE

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
	mxd_network_area_detector_initialize_type,
	mxd_network_area_detector_create_record_structures,
	mxd_network_area_detector_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_network_area_detector_open,
	mxd_network_area_detector_close
};

MX_AREA_DETECTOR_FUNCTION_LIST
mxd_network_area_detector_area_detector_function_list = {
	mxd_network_area_detector_arm,
	mxd_network_area_detector_trigger,
	mxd_network_area_detector_stop,
	mxd_network_area_detector_abort,
	mxd_network_area_detector_busy,
	mxd_network_area_detector_get_status,
	mxd_network_area_detector_get_frame,
	NULL,
	mxd_network_area_detector_get_roi_frame,
	mxd_network_area_detector_get_parameter,
	mxd_network_area_detector_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_network_area_detector_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MXD_NETWORK_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_network_area_detector_num_record_fields
		= sizeof( mxd_network_area_detector_record_field_defaults )
			/ sizeof( mxd_network_area_detector_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_area_detector_rfield_def_ptr
			= &mxd_network_area_detector_record_field_defaults[0];

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
mxd_network_area_detector_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_area_detector_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_NETWORK_AREA_DETECTOR *network_area_detector;

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
			&mxd_network_area_detector_area_detector_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	network_area_detector->record = record;

	ad->trigger_mode = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_area_detector_finish_record_initialization()";

	MX_AREA_DETECTOR *ad;
	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	mx_status_type mx_status;

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

	mx_network_field_init( &(network_area_detector->busy_nf),
		network_area_detector->server_record,
		"%s.busy", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->bytes_per_frame_nf),
		network_area_detector->server_record,
	    "%s.bytes_per_frame", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->bytes_per_pixel_nf),
		network_area_detector->server_record,
	    "%s.bytes_per_pixel", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->current_num_rois_nf),
		network_area_detector->server_record,
	    "%s.current_num_rois", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->framesize_nf),
		network_area_detector->server_record,
		"%s.framesize", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->frame_buffer_nf),
		network_area_detector->server_record,
		"%s.frame_buffer", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->get_frame_nf),
		network_area_detector->server_record,
		"%s.get_frame", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->get_roi_frame_nf),
		network_area_detector->server_record,
		"%s.get_roi_frame", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->image_format_name_nf),
		network_area_detector->server_record,
	    "%s.image_format_name", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->image_format_nf),
		network_area_detector->server_record,
		"%s.image_format", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->maximum_framesize_nf),
		network_area_detector->server_record,
	    "%s.maximum_framesize", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->maximum_num_rois_nf),
		network_area_detector->server_record,
	    "%s.maximum_num_rois", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->pixel_order_nf),
		network_area_detector->server_record,
		"%s.pixel_order", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->roi_nf),
		network_area_detector->server_record,
		"%s.roi", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->roi_array_nf),
		network_area_detector->server_record,
		"%s.roi_array", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->roi_frame_buffer_nf),
		network_area_detector->server_record,
	    "%s.roi_frame_buffer", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->roi_number_nf),
		network_area_detector->server_record,
		"%s.roi_number", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->status_nf),
		network_area_detector->server_record,
		"%s.status", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->stop_nf),
		network_area_detector->server_record,
		"%s.stop", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->trigger_nf),
		network_area_detector->server_record,
		"%s.trigger", network_area_detector->remote_record_name );

	mx_network_field_init( &(network_area_detector->trigger_mode_nf),
		network_area_detector->server_record,
		"%s.trigger_mode", network_area_detector->remote_record_name );

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

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_area_detector_open()";

	MX_AREA_DETECTOR *ad;
	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	long dimension[1];
	mx_status_type mx_status;

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
	/* Get the maximum framesize from the server. */

	dimension[0] = 2;

	mx_status = mx_get_array(&(network_area_detector->maximum_framesize_nf),
				MXFT_LONG, 1, dimension,
				ad->maximum_framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: record '%s', maximum_framesize = (%ld, %ld)",
		fname, record->name,
		ad->maximum_framesize[0],
		ad->maximum_framesize[1]));

	/* Get the image framesize from the server. */

	dimension[0] = 2;

	mx_status = mx_get_array( &(network_area_detector->framesize_nf),
				MXFT_LONG, 1, dimension,
				ad->framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: record '%s', framesize = (%ld, %ld)",
	    fname, record->name, ad->framesize[0], ad->framesize[1]));

	/* Get the image binsize from the server. */

	dimension[0] = 2;

	mx_status = mx_get_array( &(network_area_detector->binsize_nf),
				MXFT_LONG, 1, dimension,
				ad->binsize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: record '%s', binsize = (%ld, %ld)",
	    fname, record->name, ad->binsize[0], ad->binsize[1]));

	/* Get the image format name from the server. */

	dimension[0] = MXU_IMAGE_FORMAT_NAME_LENGTH;

	mx_status = mx_get_array(&(network_area_detector->image_format_name_nf),
				MXFT_STRING, 1, dimension,
				ad->image_format_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the image format type. */

	mx_status = mx_get_image_format_type_from_name(
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

	/* Get the pixel order. */

	mx_status = mx_get( &(network_area_detector->pixel_order_nf),
				MXFT_LONG, &(ad->pixel_order) );

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

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_arm()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	mx_status_type mx_status;

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

	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	mx_status_type mx_status;

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

	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	mx_status_type mx_status;

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

	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	mx_status_type mx_status;

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
mxd_network_area_detector_busy( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_busy()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	mx_status_type mx_status;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_area_detector->busy_nf),
				MXFT_BOOL, &(ad->busy) );

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: area detector '%s', busy = %d",
		fname, ad->record->name, (int) ad->busy ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_area_detector_get_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_get_status()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	mx_status_type mx_status;

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
mxd_network_area_detector_get_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_get_frame()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	MX_IMAGE_FRAME *frame;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = ad->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	/* Tell the server to prepare the frame for being read. */

	mx_status = mx_put( &(network_area_detector->get_frame_nf),
				MXFT_LONG, &(ad->frame_number) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Ask for the bytes per pixel of the image. */

	mx_status = mx_get( &(network_area_detector->bytes_per_pixel_nf),
				MXFT_DOUBLE, &(ad->bytes_per_pixel) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame->bytes_per_pixel = ad->bytes_per_pixel;

	/* Ask for the size of the image. */

	mx_status = mx_get( &(network_area_detector->bytes_per_frame_nf),
				MXFT_LONG, &(ad->bytes_per_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame->image_length = ad->bytes_per_frame;

	/* Now read the frame into the MX_IMAGE_FRAME structure. */

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
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

	mx_status = mx_get_array( &(network_area_detector->frame_buffer_nf),
			MXFT_CHAR, 1, dimension, frame->image_data );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
    ("%s: successfully read a %lu byte image frame from area detector '%s'.",
		fname, (unsigned long) frame->image_length,
		ad->record->name ));

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
mxd_network_area_detector_get_roi_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_network_area_detector_get_roi_frame()";

	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	MX_IMAGE_FRAME *roi_frame;
	long dimension[1];
	mx_status_type mx_status;

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

	roi_frame->bytes_per_pixel = ad->bytes_per_pixel;
	roi_frame->image_length    = ad->roi_bytes_per_frame;

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

	dimension[0] = roi_frame->image_length;

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

	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_area_detector_get_pointers( ad,
						&network_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
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

	case MXLV_AD_FORMAT:
	case MXLV_AD_FORMAT_NAME:
		mx_status = mx_get( &(network_area_detector->image_format_nf),
					MXFT_LONG, &(ad->image_format) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
#if MXD_NETWORK_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: image format = %ld, format name = '%s'",
		    fname, ad->image_format, ad->image_format_name));
#endif
		break;

	case MXLV_AD_PIXEL_ORDER:
		mx_status = mx_get( &(network_area_detector->pixel_order_nf),
					MXFT_LONG, &(ad->pixel_order) );
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_get( &(network_area_detector->trigger_mode_nf),
					MXFT_LONG, &(ad->trigger_mode) );
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

	case MXLV_AD_BUSY:
		mx_status = mx_get( &(network_area_detector->busy_nf),
					MXFT_BOOL, &(ad->busy) );
		break;

	case MXLV_AD_STATUS:
		mx_status = mx_get( &(network_area_detector->status_nf),
					MXFT_HEX, &(ad->status) );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
		mx_status = mx_get( &(network_area_detector->sequence_type_nf),
		    MXFT_LONG, &(ad->sequence_parameters.sequence_type) );

		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		mx_status = mx_get(
			&(network_area_detector->num_sequence_parameters_nf),
			MXFT_LONG, &(ad->sequence_parameters.num_parameters) );
				
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension[0] = ad->sequence_parameters.num_parameters;

		mx_status = mx_get_array(
			&(network_area_detector->sequence_parameter_array_nf),
			MXFT_DOUBLE, 1, dimension,
			&(ad->sequence_parameters.parameter_array));
		break;

	case MXLV_AD_ROI_NUMBER:
		mx_status = mx_get( &(network_area_detector->roi_number_nf),
					MXFT_LONG, &(ad->roi_number) );
		break;
	case MXLV_AD_ROI:
		dimension[0] = 4;

		mx_status = mx_get_array( &(network_area_detector->roi_nf),
				MXFT_LONG, 1, dimension, &(ad->roi) );
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

	MX_NETWORK_AREA_DETECTOR *network_area_detector;
	long dimension[1];
	mx_status_type mx_status;

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
		break;

	case MXLV_AD_BINSIZE:
		dimension[0] = 2;

		mx_status = mx_put_array( &(network_area_detector->binsize_nf),
				MXFT_LONG, 1, dimension, &(ad->binsize) );
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_put( &(network_area_detector->trigger_mode_nf),
				MXFT_LONG, &(ad->trigger_mode) );
		break;

		break;

	case MXLV_AD_SEQUENCE_TYPE:
		mx_status = mx_put( &(network_area_detector->sequence_type_nf),
		    MXFT_LONG, &(ad->sequence_parameters.sequence_type) );

		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		mx_status = mx_put(
			&(network_area_detector->num_sequence_parameters_nf),
			MXFT_LONG, &(ad->sequence_parameters.num_parameters) );
				
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension[0] = ad->sequence_parameters.num_parameters;

		mx_status = mx_put_array(
			&(network_area_detector->sequence_parameter_array_nf),
			MXFT_DOUBLE, 1, dimension,
			&(ad->sequence_parameters.parameter_array));
		break;

	case MXLV_AD_ROI_NUMBER:
		mx_status = mx_put( &(network_area_detector->roi_number_nf),
					MXFT_LONG, &(ad->roi_number) );
		break;
	case MXLV_AD_ROI:
		dimension[0] = 4;

		mx_status = mx_put_array( &(network_area_detector->roi_nf),
				MXFT_LONG, 1, dimension, &(ad->roi) );
		break;
	case MXLV_AD_FORMAT:
	case MXLV_AD_FORMAT_NAME:
	case MXLV_AD_PIXEL_ORDER:
	case MXLV_AD_BYTES_PER_FRAME:
	case MXLV_AD_BYTES_PER_PIXEL:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Changing the parameter '%s' for area detector '%s' "
		"is not supported.", mx_get_field_label_string( ad->record,
			ad->parameter_type ), ad->record->name );
		break;

	default:
		mx_status =
			mx_area_detector_default_set_parameter_handler( ad );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}


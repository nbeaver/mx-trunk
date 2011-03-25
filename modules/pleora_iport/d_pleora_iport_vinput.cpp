/*
 * Name:    d_pleora_iport_vinput.c
 *
 * Purpose: MX video input driver for a Pleora iPORT video capture device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PLEORA_IPORT_VINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_array.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_pleora_iport.h"
#include "d_pleora_iport_vinput.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_pleora_iport_vinput_record_function_list = {
	NULL,
	mxd_pleora_iport_vinput_create_record_structures,
	mxd_pleora_iport_vinput_finish_record_initialization,
	NULL,
	NULL,
	mxd_pleora_iport_vinput_open,
	mxd_pleora_iport_vinput_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_pleora_iport_vinput_video_input_function_list = {
	mxd_pleora_iport_vinput_arm,
	mxd_pleora_iport_vinput_trigger,
	mxd_pleora_iport_vinput_stop,
	mxd_pleora_iport_vinput_abort,
	mxd_pleora_iport_vinput_asynchronous_capture,
	NULL,
	NULL,
	NULL,
	mxd_pleora_iport_vinput_get_extended_status,
	mxd_pleora_iport_vinput_get_frame,
	mxd_pleora_iport_vinput_get_parameter,
	mxd_pleora_iport_vinput_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_pleora_iport_vinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_PLEORA_IPORT_VINPUT_STANDARD_FIELDS
};

long mxd_pleora_iport_vinput_num_record_fields
		= sizeof( mxd_pleora_iport_vinput_record_field_defaults )
		/ sizeof( mxd_pleora_iport_vinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pleora_iport_vinput_rfield_def_ptr
			= &mxd_pleora_iport_vinput_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_pleora_iport_vinput_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_PLEORA_IPORT_VINPUT **pleora_iport_vinput,
			MX_PLEORA_IPORT **pleora_iport,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pleora_iport_vinput_get_pointers()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput_ptr;
	MX_RECORD *pleora_iport_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pleora_iport_vinput_ptr = (MX_PLEORA_IPORT_VINPUT *)
				vinput->record->record_type_struct;

	if ( pleora_iport_vinput_ptr == (MX_PLEORA_IPORT_VINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT_VINPUT pointer for record '%s' "
			"passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}

	if ( pleora_iport_vinput != (MX_PLEORA_IPORT_VINPUT **) NULL ) {
		*pleora_iport_vinput = pleora_iport_vinput_ptr;
	}

	if ( pleora_iport != (MX_PLEORA_IPORT **) NULL ) {
		pleora_iport_record =
			pleora_iport_vinput_ptr->pleora_iport_record;

		if ( pleora_iport_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The pleora_iport_record pointer for record '%s' "
			"is NULL.",
			vinput->record->name, calling_fname );
		}

		*pleora_iport = (MX_PLEORA_IPORT *)
					pleora_iport_record->record_type_struct;

		if ( *pleora_iport == (MX_PLEORA_IPORT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT pointer for record '%s' used "
			"by record '%s' is NULL.",
				vinput->record->name,
				pleora_iport_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

#if MXD_PLEORA_IPORT_VINPUT_DEBUG

static void
mxd_pleora_iport_vinput_display_extensions( MX_RECORD *record,
					CyGrabber *grabber )
{
	static const char fname[] =
		"mxd_pleora_iport_vinput_display_extensions()";

	int i;

	fprintf( stderr, "Camera '%s' device extensions:\n", record->name );

	struct {
		char extension_name[80];
		unsigned long extension_id;
	} device_extensions[] = {

	{"CY_DEVICE_EXT_COUNTER", CY_DEVICE_EXT_COUNTER},
	{"CY_DEVICE_EXT_DEBOUNCING", CY_DEVICE_EXT_DEBOUNCING},
	{"CY_DEVICE_EXT_DELAYER", CY_DEVICE_EXT_DELAYER},
	{"CY_DEVICE_EXT_FLOW_CONTROL", CY_DEVICE_EXT_FLOW_CONTROL},
	{"CY_DEVICE_EXT_GPIO_CONTROL_BITS", CY_DEVICE_EXT_GPIO_CONTROL_BITS},
	{"CY_DEVICE_EXT_GPIO_FUNCTION_SELECT",
					CY_DEVICE_EXT_GPIO_FUNCTION_SELECT},
	{"CY_DEVICE_EXT_GPIO_INTERRUPTS", CY_DEVICE_EXT_GPIO_INTERRUPTS},
	{"CY_DEVICE_EXT_GPIO_LUT", CY_DEVICE_EXT_GPIO_LUT},
	{"CY_DEVICE_EXT_PULSE_GENERATOR", CY_DEVICE_EXT_PULSE_GENERATOR},
	{"CY_DEVICE_EXT_RESCALER", CY_DEVICE_EXT_RESCALER},
	{"CY_DEVICE_EXT_STREAMING_INT_SELECTION",
					CY_DEVICE_EXT_STREAMING_INT_SELECTION},
	{"CY_DEVICE_EXT_TIMESTAMP_COUNTER", CY_DEVICE_EXT_TIMESTAMP_COUNTER},
	{"CY_DEVICE_EXT_TIMESTAMP_TRIGGERS", CY_DEVICE_EXT_TIMESTAMP_TRIGGERS},
	{"", 0} };

	CyDevice &device = grabber->GetDevice();

	for ( i = 0; ; i++ ) {
		if ( strlen( device_extensions[i].extension_name ) == 0 ) {
			break;		/* Exit the for() loop. */
		}

		fprintf( stderr, "%-38s %lu extension(s)\n",
			device_extensions[i].extension_name,
			device.GetExtensionCountByType(
				device_extensions[i].extension_id ) );
	}

	fprintf( stderr, "Camera '%s' grabber extensions:\n", record->name );

	struct {
		char extension_name[80];
		unsigned long extension_id;
	} grabber_extensions[] = {

	{"CY_GRABBER_EXT_CAMERA_LINK", CY_GRABBER_EXT_CAMERA_LINK},
	{"CY_GRABBER_EXT_CAMERA_LVDS", CY_GRABBER_EXT_CAMERA_LVDS},
	{"CY_GRABBER_EXT_ANALOG_SOURCE", CY_GRABBER_EXT_ANALOG_SOURCE},
	{"", 0} };

	for ( i = 0; ; i++ ) {
		if ( strlen( grabber_extensions[i].extension_name ) == 0 ) {
			break;		/* Exit the for() loop. */
		}

		fprintf( stderr, "%-38s %lu extension(s)\n",
			grabber_extensions[i].extension_name,
			grabber->GetExtensionCountByType(
				grabber_extensions[i].extension_id ) );
	}

	return;
}

#endif /* MXD_PLEORA_IPORT_VINPUT_DEBUG */

/*---*/

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pleora_iport_vinput_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	pleora_iport_vinput = (MX_PLEORA_IPORT_VINPUT *)
				malloc( sizeof(MX_PLEORA_IPORT_VINPUT) );

	if ( pleora_iport_vinput == (MX_PLEORA_IPORT_VINPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_PLEORA_IPORT_VINPUT structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = pleora_iport_vinput;
	record->class_specific_function_list = 
			&mxd_pleora_iport_vinput_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	pleora_iport_vinput->record = record;

	vinput->trigger_mode = 0;

	pleora_iport_vinput->grabber = NULL;
	pleora_iport_vinput->grab_finished_event = NULL;
	pleora_iport_vinput->grab_in_progress = FALSE;

	pleora_iport_vinput->user_buffer = NULL;

	vinput->trigger_mode = MXT_IMAGE_EXTERNAL_TRIGGER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pleora_iport_vinput_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_PLEORA_IPORT *pleora_iport = NULL;
	long i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
				&pleora_iport_vinput, &pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_video_input_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We need to add this record to the device_record_array
	 * in the pleora_iport record.
	 */

	if ( pleora_iport->max_devices <= 0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"max_devices for Pleora iPORT record '%s' is %d "
		"which leaves no space for record '%s' to insert itself "
		"into pleora_iport->device_record_array",
			pleora_iport->record->name,
			pleora_iport->max_devices,
			record->name );
	}

	if ( pleora_iport->device_record_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The pleora_iport->device_record_array pointer is NULL "
		"for record '%s' used by record '%s'.",
			pleora_iport->record->name, record->name );
	}

	/* Look for an empty slot in the device_record_array. */

	for ( i = 0; i < pleora_iport->max_devices; i++ ) {
		if ( pleora_iport->device_record_array[i] == NULL ) {
			pleora_iport->device_record_array[i] = record;

			break;	/* Exit the for() loop. */
		}
	}

	if ( i >= pleora_iport->max_devices ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"There is no unused space available in the device_record_array "
		"of Pleora iPORT record '%s' used by video input '%s'.  "
		"You should increase the value of the 'max_devices' field "
		"( currently %ld ) in record '%s' to make room.",
			pleora_iport->record->name,
			record->name,
			pleora_iport->max_devices,
			pleora_iport->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pleora_iport_vinput_open()";

	MX_VIDEO_INPUT *vinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_PLEORA_IPORT *pleora_iport = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
				&pleora_iport_vinput, &pleora_iport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	if ( pleora_iport_vinput->grabber == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No grabber has been connected for record '%s'.",
			record->name );
	}

	CyGrabber *grabber = pleora_iport_vinput->grabber;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: grabber = %p", fname, grabber));
#endif

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	{
		CyDevice &device = grabber->GetDevice();

		unsigned char dev_id, mod_id, sub_id;
		unsigned char vendor_id, mac1, mac2, mac3, mac4;
		unsigned char mac5, mac6, ip1, ip2, ip3, ip4;
		unsigned char version_major, version_minor;
		unsigned char channel_count, version_sub;

		device.GetDeviceInfo( &dev_id, &mod_id, &sub_id,
			&vendor_id, &mac1, &mac2, &mac3, &mac4,
			&mac5, &mac6, &ip1, &ip2, &ip3, &ip4,
			&version_major, &version_minor,
			&channel_count, &version_sub );

		MX_DEBUG(-2,("%s: grabber IP address = %d.%d.%d.%d",
		fname, ip1, ip2, ip3, ip4));
	}
#endif

#if MXD_PLEORA_IPORT_VINPUT_DEBUG

	/* For debugging purposes, print out information about
	 * the extensions that we need.
	 */

	if ( 0 ) {
		mxd_pleora_iport_vinput_display_extensions( record, grabber );
	}
#endif

	/* Initialize the image properties. */

	mx_status = mx_video_input_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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
	case MXT_IMAGE_FORMAT_GREY32:
		vinput->bytes_per_pixel = 4;
		vinput->bits_per_pixel  = 32;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image format %ld for video input '%s'.",
			vinput->image_format, vinput->record->name );
		break;
	}

	/* We set the bits per pixel here so that
	 * CY_GRABBER_PARAM_PIXEL_DEPTH will be set
	 * under the hood.
	 */

	mx_status = mx_video_input_set_bits_per_pixel( record,
						vinput->bits_per_pixel );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We set the image frame size here so that CY_GRABBER_PARAM_SIZE_X
	 * and CY_GRABBER_PARAM_SIZE_Y will be set under the hood.
	 */

	mx_status = mx_video_input_set_framesize( record,
						vinput->framesize[0],
						vinput->framesize[1] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel
			* vinput->framesize[0] * vinput->framesize[1] );

	/* We set the bytes per frame here so that
	 * CY_GRABBER_PARAM_IMAGE_SIZE will be set
	 * under the hood.
	 */

	mx_status = mx_video_input_set_bytes_per_frame( record,
						vinput->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*
	 * NOTE: The _real_ value for CY_GRABBER_PARAM_OFFSET_X is
	 * set in the open() routine of the area detector driver,
	 * rather than here in the video input driver, since this
	 * part of the software does not know which area detector
	 * driver is being used.
	 */

	grabber->SetParameter( CY_GRABBER_PARAM_OFFSET_X, 0 );

	grabber->SetParameter( CY_GRABBER_PARAM_OFFSET_Y, 0 );

	grabber->SetParameter( CY_GRABBER_PARAM_TAP_QUANTITY, 1 );

	grabber->SetParameter( CY_GRABBER_PARAM_PACKED, false );

	grabber->SetParameter( CY_GRABBER_PARAM_NORMALIZED, false );

	/* Send the cached values to the grabber. */

	grabber->SaveConfig();

	/* Initialize a bunch of MX driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = (long) mx_native_byteorder();
	vinput->trigger_mode   = MXT_IMAGE_NO_TRIGGER;

	vinput->last_frame_number = -1;
	vinput->total_num_frames = 0;
	vinput->status = 0x0;

	vinput->maximum_frame_number = 0;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: vinput->framesize[0] = %ld, vinput->framesize[1] = %ld",
		fname, vinput->framesize[0], vinput->framesize[1] ));

	MX_DEBUG(-2,("%s: vinput->image_format_name = '%s'",
		fname, vinput->image_format_name));

	MX_DEBUG(-2,("%s: vinput->image_format = %ld",
		fname, vinput->image_format));

	MX_DEBUG(-2,("%s: vinput->byte_order = %ld",
		fname, vinput->byte_order));

	MX_DEBUG(-2,("%s: vinput->trigger_mode = %ld",
		fname, vinput->trigger_mode));

	MX_DEBUG(-2,("%s: vinput->bits_per_pixel = %ld",
		fname, vinput->bits_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_pixel = %g",
		fname, vinput->bytes_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_frame = %ld",
		fname, vinput->bytes_per_frame));
#endif

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_pleora_iport_vinput_close()";

	MX_VIDEO_INPUT *vinput = NULL;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	if ( pleora_iport_vinput->grabber != NULL ) {
		delete pleora_iport_vinput->grabber;
	}

	pleora_iport_vinput->grabber = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_arm()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_SEQUENCE_PARAMETERS *seq;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	/* For this video input, the input frame buffer must be set up
	 * before we can execute the grabber->Grab() method.  For that
	 * reason, we do mx_image_alloc() here to make sure that the
	 * frame is set up.
	 */

	mx_status = mx_image_alloc( &(vinput->frame),
					vinput->framesize[0],
					vinput->framesize[1],
					vinput->image_format,
					vinput->byte_order,
					vinput->bytes_per_pixel,
					MXT_IMAGE_HEADER_LENGTH_IN_BYTES,
					vinput->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create a CyUserBuffer() structure to use in receiving frames. */

	if ( pleora_iport_vinput->user_buffer != NULL ) {
		delete pleora_iport_vinput->user_buffer;
	}

	pleora_iport_vinput->user_buffer = new CyUserBuffer(
				(unsigned char *) vinput->frame->image_data,
				vinput->bytes_per_frame, 0 );

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: Created pleora_iport_vinput->user_buffer = %p",
			fname, pleora_iport_vinput->user_buffer));

	MX_DEBUG(-2,("%s:   This CyUserBuffer control buffer %p of length %lu.",
		fname, vinput->frame->image_data, vinput->bytes_per_frame));

	MX_DEBUG(-2,("%s:   CyUserBuffer::GetBuffer() = %p",
		fname, pleora_iport_vinput->user_buffer->GetBuffer() ));

	MX_DEBUG(-2,("%s:   CyUserBuffer::GetBufferSize() = %lu",
		fname, pleora_iport_vinput->user_buffer->GetBufferSize() ));
#endif

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: Prepare for trigger mode %d",
		fname, vinput->trigger_mode ));
#endif

	if ( (vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) == 0 ) {

		/* If external triggering is not enabled,
		 * return without doing anything further.
		 */

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
		MX_DEBUG(-2,
		("%s: external trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we are doing external triggering. */

	seq = &(vinput->sequence_parameters);

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: Prepare for sequence type %d",
		fname, seq->sequence_type));
#endif

	pleora_iport_vinput->grab_in_progress = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_trigger()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_IMAGE_FRAME *frame;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	if ( ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {

		/* If internal triggering is not enabled,
		 * return without doing anything.
		 */

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
		MX_DEBUG(-2,
		("%s: internal trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we are doing internal triggering. */

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: Sending internal trigger for '%s'.",
		fname, vinput->record->name));
#endif
	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"No image frame has been allocated for video input '%s'.",
			vinput->record->name );
	}

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	CyResult cy_result = grabber->Grab(
				CyChannel(0),
				(unsigned char *) frame->image_data,
				vinput->bytes_per_frame,
				pleora_iport_vinput->grab_finished_event,
				NULL,
				CY_GRABBER_FLAG_NO_WAIT,
				NULL );

	if ( cy_result != CY_RESULT_OK ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to start acquiring a frame for '%s' failed.  "
		"cy_result = %d",
			vinput->record->name );
	}

	pleora_iport_vinput->grab_in_progress = TRUE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_stop()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_abort()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_asynchronous_capture( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_pleora_iport_vinput_asynchronous_capture()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mxd_pleora_iport_vinput_get_extended_status()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	unsigned long timeout_ms = 1L;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Poll for the event status. */

	CyUserBuffer *user_buffer = pleora_iport_vinput->user_buffer;

	if ( user_buffer == NULL ) {

		/* If we have never started an acquisition sequence, then
		 * the detector is idle.
		 */

		vinput->status = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	CyResultEvent &grab_finished_event = user_buffer->GetCompletionEvent();

	CyResult cy_result =
		grab_finished_event.WaitUntilSignaled( timeout_ms );

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: WaitUntilSignaled() returned %d",
		fname, cy_result ));
#endif

	vinput->status = 0;

	switch( cy_result ) {
	case CY_RESULT_OK:
	case CY_RESULT_IMAGE_ERROR:
		if ( pleora_iport_vinput->grab_in_progress ) {
			vinput->last_frame_number = 0;
			vinput->total_num_frames++;

			pleora_iport_vinput->grab_in_progress = FALSE;
		}

		if ( 1 ) {
			CyImageInfo &image_info = user_buffer->GetImageInfo();

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
			MX_DEBUG(-2,("%s: image ID = %lu",
				fname, image_info.GetImageID() ));
			MX_DEBUG(-2,("%s: image size = %lu",
				fname, image_info.GetSize() ));
			MX_DEBUG(-2,("%s: image timestamp = %lu",
				fname, image_info.GetTimestamp() ));
#endif

			CyBufferQueue::ImageStatus &image_status
				= image_info.GetImageStatus();

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
			MX_DEBUG(-2,("%s: mFrameOverrun = %d",
				fname, image_status.mFrameOverrun));
			MX_DEBUG(-2,("%s: mGrabberFIFOOverrun = %d",
				fname, image_status.mGrabberFIFOOverrun));
			MX_DEBUG(-2,("%s: mImageDropped = %d",
				fname, image_status.mImageDropped));
			MX_DEBUG(-2,("%s: mPartialLineMissing = %d",
				fname, image_status.mPartialLineMissing));
			MX_DEBUG(-2,("%s: mFullLineMissing = %d",
				fname, image_status.mFullLineMissing));
			MX_DEBUG(-2,("%s: mInterlaced = %d",
				fname, image_status.mInterlaced));
			MX_DEBUG(-2,("%s: mFirstGrabbedField = %d",
				fname, image_status.mFirstGrabbedField));
			MX_DEBUG(-2,("%s: mFieldFirstLine = %d",
				fname, image_status.mFieldFirstLine));
			MX_DEBUG(-2,("%s: mHorizontalUnlocked = %d",
				fname, image_status.mHorizontalUnlocked));
			MX_DEBUG(-2,("%s: mVerticalUnlocked = %d",
				fname, image_status.mVerticalUnlocked));
			MX_DEBUG(-2,("%s: mEOFByLineCount = %d",
				fname, image_status.mEOFByLineCount));
#endif
		}
		break;

	case CY_RESULT_TIMEOUT:
		if ( pleora_iport_vinput->grab_in_progress ) {
			vinput->status |= MXSF_VIN_IS_BUSY;

			vinput->last_frame_number = -1;
		}
		break;

	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"WaitUntilSignaled(%lu) returned an unexpected "
		"value (%d) for record '%s'.",
			timeout_ms, cy_result, vinput->record->name );
		break;
	}

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
		fname, vinput->last_frame_number, vinput->total_num_frames,
		vinput->status));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_get_frame()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_IMAGE_FRAME *frame;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_get_parameter()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
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
#if MXD_PLEORA_IPORT_VINPUT_DEBUG
		MX_DEBUG(-2,("%s: video format = %ld, format name = '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
#endif
		break;

	case MXLV_VIN_BYTE_ORDER:
		vinput->byte_order = (long) mx_native_byteorder();
		break;

	case MXLV_VIN_TRIGGER_MODE:
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel
			* vinput->framesize[0] * vinput->framesize[1] );
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
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
mxd_pleora_iport_vinput_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_set_parameter()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	unsigned long bytes_per_frame;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		grabber->SetParameter( CY_GRABBER_PARAM_SIZE_X,
						vinput->framesize[0] );
		grabber->SetParameter( CY_GRABBER_PARAM_SIZE_Y,
						vinput->framesize[1] );
		grabber->SaveConfig();
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
		/* The documentation for the CyUserBuffer class states that
		 * the image buffer size must be a multiple of 4 bytes.
		 */

		bytes_per_frame = vinput->bytes_per_frame;

		if ( (bytes_per_frame % 4) != 0 ) {
			bytes_per_frame += ( 4 - (bytes_per_frame % 4) );

			mx_warning( "The bytes per frame of video input '%s' "
			"was rounded up to %lu so that it is a multiple of 4.",
				vinput->record->name,
				bytes_per_frame );
		}

		grabber->SetParameter( CY_GRABBER_PARAM_IMAGE_SIZE,
						bytes_per_frame );
		grabber->SaveConfig();
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		grabber->SetParameter( CY_GRABBER_PARAM_PIXEL_DEPTH,
				mx_round(8.0 * vinput->bytes_per_pixel) );
		grabber->SaveConfig();
		break;

	case MXLV_VIN_BITS_PER_PIXEL:
		grabber->SetParameter( CY_GRABBER_PARAM_PIXEL_DEPTH,
						vinput->bits_per_pixel );
		grabber->SaveConfig();
		break;

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


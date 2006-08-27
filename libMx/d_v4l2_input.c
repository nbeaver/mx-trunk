/*
 * Name:    d_v4l2_input.c
 *
 * Purpose: MX driver for Video4Linux2 video input.
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

#define MXD_V4L2_INPUT_DEBUG	TRUE

#include <stdio.h>

#include "mxconfig.h"

#if defined(OS_LINUX) && HAVE_VIDEO_4_LINUX_2

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "d_v4l2_input.h"

/* The following defines are used for detecting a V4L version 1 device.
 * V4L version 1 devices are not supported by this driver, but their
 * presence is detected.  The following defines were copied from a
 * Linux 2.4.18 system.
 */

struct video_capability {
	char name[32];
	int type;
	int channels;
	int audios;
	int maxwidth;
	int maxheight;
	int minwidth;
	int minheight;
};

#define VIDIOCGCAP	_IOR('v',1,struct video_capability)

/* End of V4L version 1 defines. */

/*---*/

MX_RECORD_FUNCTION_LIST mxd_v4l2_input_record_function_list = {
	NULL,
	mxd_v4l2_input_create_record_structures,
	mxd_v4l2_input_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_v4l2_input_open,
	mxd_v4l2_input_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_v4l2_input_video_input_function_list = {
	mxd_v4l2_input_arm,
	mxd_v4l2_input_trigger,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_v4l2_input_get_frame,
	NULL,
	mxd_v4l2_input_get_parameter,
	mxd_v4l2_input_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_v4l2_input_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_V4L2_INPUT_STANDARD_FIELDS
};

long mxd_v4l2_input_num_record_fields
		= sizeof( mxd_v4l2_input_record_field_defaults )
			/ sizeof( mxd_v4l2_input_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_v4l2_input_rfield_def_ptr
			= &mxd_v4l2_input_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_v4l2_input_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_V4L2_INPUT **v4l2_input,
			const char *calling_fname )
{
	static const char fname[] = "mxd_v4l2_input_get_pointers()";

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (v4l2_input == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_V4L2_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*v4l2_input = (MX_V4L2_INPUT *) vinput->record->record_type_struct;

	if ( *v4l2_input == (MX_V4L2_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_V4L2_INPUT pointer for record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_v4l2_input_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_v4l2_input_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_V4L2_INPUT *v4l2_input;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	v4l2_input = (MX_V4L2_INPUT *) malloc( sizeof(MX_V4L2_INPUT) );

	if ( v4l2_input == (MX_V4L2_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_V4L2_INPUT structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = v4l2_input;
	record->class_specific_function_list = 
			&mxd_v4l2_input_video_input_function_list;

	memset( &(vinput->sequence_info), 0, sizeof(vinput->sequence_info) );

	vinput->record = record;
	v4l2_input->record = record;

	v4l2_input->fd = -1;
	v4l2_input->num_inputs = -1;

	v4l2_input->armed = FALSE;

	v4l2_input->frame_buffer_length = 0;
	v4l2_input->frame_buffer = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_v4l2_input_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_v4l2_input_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_v4l2_input_open()";

	MX_VIDEO_INPUT *vinput;
	MX_V4L2_INPUT *v4l2_input;
	struct video_capability cap1;
	struct v4l2_input input;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	int i, saved_errno, os_status;
	long input_number;
	long requested_x_framesize, requested_y_framesize;
	long requested_image_format;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	v4l2_input = NULL;  /* Suppress GCC 4 uninitialized variable warning. */

	mx_status = mxd_v4l2_input_get_pointers( vinput, &v4l2_input, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	v4l2_input->fd = open(v4l2_input->device_name, O_RDWR | O_NONBLOCK, 0);

	if ( v4l2_input->fd == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to open V4L2 video input device '%s' for record '%s'.  "
		"Errno = %d, error message = '%s'",
			v4l2_input->device_name, record->name,
			saved_errno, strerror(saved_errno) );
	}

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s: opened video device '%s'.",
		fname, v4l2_input->device_name));
#endif

	/* Get the capabilities for this input. */

	os_status = ioctl(v4l2_input->fd, VIDIOC_QUERYCAP, &(v4l2_input->cap));

	if ( os_status == -1 ) {
		saved_errno = errno;

		switch ( saved_errno ) {
		case EINVAL:
		case ENOTTY:

			/* See if this is a V4L version 1 device. */

			os_status = ioctl( v4l2_input->fd, VIDIOCGCAP, &cap1 );

			if ( os_status == 0 ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Device '%s' ('%s') used by record '%s' is a "
				"Video for Linux version 1 video input, "
				"which is not supported by this driver.  "
				"Only Video for Linux version 2 devices "
				"are supported.  "
				"If you get this message, you may need to "
				"upgrade to the Linux 2.6 kernel or beyond "
				"to get Video for Linux version 2.",
					v4l2_input->device_name, cap1.name,
					record->name );
			} else {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Device '%s' for record '%s' is not a video input.",
					v4l2_input->device_name, record->name );
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error getting video capabilities for device '%s' "
			"belonging to record '%s'.  "
			"Errno = %d, error message = '%s'",
				v4l2_input->device_name, record->name,
				saved_errno, strerror(saved_errno) );
		}
	}

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s: driver = '%s', card = '%s'",
		fname, v4l2_input->cap.driver, v4l2_input->cap.card));

	MX_DEBUG(-2,
	("%s: bus_info = '%s', version = %lu, capabilities = %#lx",
		fname, v4l2_input->cap.bus_info,
		(unsigned long) v4l2_input->cap.version,
		(unsigned long) v4l2_input->cap.capabilities));

	/* Display the capabilities. */

	fprintf(stderr,"%s: Capabilities: ", fname);

	if ( v4l2_input->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE ) {
		fprintf(stderr, "video_capture ");
	}
	if ( v4l2_input->cap.capabilities & V4L2_CAP_VIDEO_OUTPUT ) {
		fprintf(stderr, "video_output ");
	}
	if ( v4l2_input->cap.capabilities & V4L2_CAP_VIDEO_OVERLAY ) {
		fprintf(stderr, "video_overlay ");
	}
	if ( v4l2_input->cap.capabilities & V4L2_CAP_VBI_CAPTURE ) {
		fprintf(stderr, "vbi_capture ");
	}
	if ( v4l2_input->cap.capabilities & V4L2_CAP_VBI_OUTPUT ) {
		fprintf(stderr, "vbi_output ");
	}
	if ( v4l2_input->cap.capabilities & V4L2_CAP_RDS_CAPTURE ) {
		fprintf(stderr, "rds_capture ");
	}
	if ( v4l2_input->cap.capabilities & V4L2_CAP_TUNER ) {
		fprintf(stderr, "tuner ");
	}
	if ( v4l2_input->cap.capabilities & V4L2_CAP_AUDIO ) {
		fprintf(stderr, "audio ");
	}
	if ( v4l2_input->cap.capabilities & V4L2_CAP_READWRITE ) {
		fprintf(stderr, "readwrite ");
	}
	if ( v4l2_input->cap.capabilities & V4L2_CAP_ASYNCIO ) {
		fprintf(stderr, "asyncio ");
	}
	if ( v4l2_input->cap.capabilities & V4L2_CAP_STREAMING ) {
		fprintf(stderr, "streaming ");
	}

	fprintf(stderr,"\n");
#endif

	/* Enumerate the inputs. */

	v4l2_input->num_inputs = 0;

	for ( i = 0; ; i++ ) {
		memset( &input, 0, sizeof(input) );

		input.index = i;

		os_status = ioctl( v4l2_input->fd, VIDIOC_ENUMINPUT, &input );

		if ( os_status == -1 ) {
			saved_errno = errno;

			if ( saved_errno == EINVAL ) {
				/* We have reached the end of the list of
				 * inputs, so break out of the for() loop.
				 */

				break;
			}

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error trying to enumerate input %d for "
			"V4L2 video input device '%s' of record '%s'.  "
			"Errno = %d, error message = '%s'",
				i, v4l2_input->device_name, record->name,
				saved_errno, strerror(saved_errno) );
		}

#if MXD_V4L2_INPUT_DEBUG
		MX_DEBUG(-2,("%s: Input %d name = '%s'",
			fname, i, input.name));
#endif

		(v4l2_input->num_inputs)++;
	}

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s: %ld video inputs for device '%s'.",
		fname, v4l2_input->num_inputs, v4l2_input->device_name ));
#endif

	/* Select the requested input. */

	input_number = v4l2_input->input_number;

	if (( input_number < 0 ) || ( input_number >= v4l2_input->num_inputs ))
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested input number %ld for V4L2 record '%s' "
		"is outside the allowed range of 0 to %ld.",
			input_number, record->name, v4l2_input->num_inputs - 1);
	}

	os_status = ioctl( v4l2_input->fd, VIDIOC_S_INPUT, &input_number );

	if ( os_status == -1 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to select input %ld for V4L2 record '%s' failed.  "
		"Errno = %d, error message = '%s'.",
			input_number, record->name,
			saved_errno, strerror(saved_errno) );
	}

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s: Input %ld selected for V4L2 record '%s'.",
		fname, input_number, record->name ));
#endif

	/* Change cropping settings to the defaults. */

	memset( &cropcap, 0, sizeof(cropcap) );

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	(void) ioctl( v4l2_input->fd, VIDIOC_CROPCAP, &cropcap );

	memset( &crop, 0, sizeof(crop) );

	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c = cropcap.defrect;	/* reset to default */

	os_status = ioctl( v4l2_input->fd, VIDIOC_S_CROP, &crop );

	if ( os_status == -1 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EINVAL:
			/* Cropping not supported. */
			break;
		default:
			/* Ignore errors. */
			break;
		}
	}

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s: reset cropping to defaults.", fname));
#endif

	requested_x_framesize = vinput->framesize[0];
	requested_y_framesize = vinput->framesize[1];
	requested_image_format = vinput->image_format;

	/* Initialize the current settings from the hardware. */

	mx_status = mx_video_input_get_framesize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, change the framesize. */

	if ( (requested_x_framesize > 0) && (requested_y_framesize > 0) ) {

		mx_status = mx_video_input_set_framesize( record,
							requested_x_framesize,
							requested_y_framesize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* If requested, set the image format. */

	if ( requested_image_format > 0 ) {

		mx_status = mx_video_input_set_image_format( record,
							requested_image_format);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_v4l2_input_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_v4l2_input_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_v4l2_input_arm()";

	MX_V4L2_INPUT *v4l2_input;
	size_t new_length;
	mx_status_type mx_status;

	mx_status = mxd_v4l2_input_get_pointers( vinput, &v4l2_input, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	v4l2_input->armed = FALSE;

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	/* For Video for Linux 2, we use the arm() routine to change the
	 * length of the local frame buffer.
	 *
	 * First we compute the necessary length of the frame buffer.
	 */

	switch( vinput->image_format ) {
	case MXT_IMAGE_FORMAT_RGB565:
		/* 16 bits (2 bytes) per pixel. */

		new_length = 2 * vinput->framesize[0] * vinput->framesize[1];
		break;
	case MXT_IMAGE_FORMAT_YUYV:
		/* 32 bits (4 bytes) for 2 pixels. */

		new_length = 2 * vinput->framesize[0] * vinput->framesize[1];
		break;
	default:
		v4l2_input->frame_buffer_length = 0;

		if ( v4l2_input->frame_buffer != NULL ) {
			free( v4l2_input->frame_buffer );
		}

		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support for image format %ld is not yet implemented "
		"for video input '%s'",
			vinput->image_format, vinput->record->name );
	}

	if ( ( v4l2_input->frame_buffer_length == new_length )
	  && ( v4l2_input->frame_buffer != NULL ) )
	{
		/* The frame buffer is already the correct length,
		 * so just return.
		 */

		v4l2_input->armed = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	v4l2_input->frame_buffer_length = new_length;

	if ( v4l2_input->frame_buffer != NULL ) {
		free( v4l2_input->frame_buffer );
	}

	v4l2_input->frame_buffer = malloc( new_length );

	if ( v4l2_input->frame_buffer == NULL ) {
		v4l2_input->frame_buffer_length = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu byte frame buffer "
		"for video input '%s'.", (unsigned long) new_length,
			vinput->record->name );
	}

	v4l2_input->armed = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_v4l2_input_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_v4l2_input_trigger()";

	MX_V4L2_INPUT *v4l2_input;
	fd_set fds;
	struct timeval tv;
	int fd, result, saved_errno;
	int seconds_to_wait, microseconds_to_wait;
	double timeout_seconds;
	mx_status_type mx_status;

	mx_status = mxd_v4l2_input_get_pointers( vinput, &v4l2_input, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	if ( v4l2_input->armed == FALSE ) {
		return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
		"Video input '%s' must be armed before it can be triggered.",
			vinput->record->name );
	}

	v4l2_input->armed = FALSE;

	seconds_to_wait      = 2;
	microseconds_to_wait = 0;

	timeout_seconds = (double) seconds_to_wait
				+ 1.0e-6 * (double) microseconds_to_wait;

	fd = v4l2_input->fd;

	for (;;) {
		FD_ZERO( &fds );
		FD_SET( fd, &fds );

		tv.tv_sec  = seconds_to_wait;
		tv.tv_usec = microseconds_to_wait;

		/* Wait for a frame to be available. */

#if MXD_V4L2_INPUT_DEBUG
		MX_DEBUG(-2,("%s: wait for a frame.", fname));
#endif

		result = select( fd+1, &fds, NULL, NULL, &tv );

		if ( result == -1 ) {
			saved_errno = errno;

			if ( saved_errno == EINTR ) {
				/* Go back to the top of the for() loop. */

				continue;
			} else {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
				    "Error in select() for video input '%s'.  "
				    "Errno = %d, error message = '%s'",
					    vinput->record->name,
					    saved_errno, strerror(saved_errno));
			}
		}

		if ( result == 0 ) {
			return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %g seconds for a frame "
			"from video input '%s'.",
				timeout_seconds, vinput->record->name );
		}

#if MXD_V4L2_INPUT_DEBUG
		MX_DEBUG(-2,("%s: read a frame.", fname));
#endif

		errno = 0;

		result = read( v4l2_input->fd,
				v4l2_input->frame_buffer,
				v4l2_input->frame_buffer_length );

		saved_errno = errno;

		MX_DEBUG(-2,("%s: read() errno = %d", fname, saved_errno));

		if ( saved_errno == EAGAIN ) {
			/* If we got an errno of EAGAIN, go back to the top of
			 * the for() loop.
			 */

			continue;
		} else {
			/* Otherwise, we break out of the for() loop. */

			break;
		}
	}


	if ( result < 0 ) {
		/* saved_errno = errno; */

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Error reading an image frame from video input '%s'.  "
			"Errno = %d, error message = '%s'",
				vinput->record->name,
				saved_errno, strerror(saved_errno) );
	}

#if 0
	if ( result < v4l2_input->frame_buffer_length ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"Read %lu bytes from video input '%s' when we were "
			"expecting to read %lu bytes.",
			    (unsigned long) result, vinput->record->name,
			    (unsigned long) v4l2_input->frame_buffer_length );
	}
#endif

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s: %lu bytes read from video input '%s'.",
		fname, (unsigned long) v4l2_input->frame_buffer_length,
		vinput->record->name ));
#endif

#if 1
	{
		int i;
		unsigned char *ptr;
		unsigned int c;

		fprintf(stderr,"\n");

		ptr = v4l2_input->frame_buffer;

		for ( i = 0; i < 300; i++ ) {
			c = (unsigned int)( *ptr );

			fprintf(stderr, "%u ", c);

			ptr++;
		}
		fprintf(stderr,"\n");
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_v4l2_input_get_frame( MX_VIDEO_INPUT *vinput, MX_IMAGE_FRAME **frame )
{
	static const char fname[] = "mxd_v4l2_input_get_frame()";

	MX_V4L2_INPUT *v4l2_input;
	mx_status_type mx_status;

	mx_status = mxd_v4l2_input_get_pointers( vinput, &v4l2_input, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	if ( ( v4l2_input->frame_buffer_length == 0 )
	  || ( v4l2_input->frame_buffer == NULL ) )
	{
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"No image frames have been taken yet for video input '%s'.",
			vinput->record->name );
	}

	*frame = malloc( sizeof(MX_IMAGE_FRAME) );

	if ( (*frame) == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Ran out of memory trying to allocate an MX_IMAGE_FRAME structure." );
	}

	(*frame)->image_type = MXT_IMAGE_LOCAL_1D_ARRAY;
	(*frame)->framesize[0] = vinput->framesize[0];
	(*frame)->framesize[1] = vinput->framesize[1];
	(*frame)->image_format = vinput->image_format;
	(*frame)->pixel_order = vinput->pixel_order;

	(*frame)->header_length = 0;
	(*frame)->header_data = 0;

	(*frame)->image_length = v4l2_input->frame_buffer_length;
	(*frame)->image_data = v4l2_input->frame_buffer;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_v4l2_input_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_v4l2_input_get_parameter()";

	MX_V4L2_INPUT *v4l2_input;
	struct v4l2_format format;
	unsigned long pf;
	char A, B, C, D;
	int os_status, saved_errno;
	mx_status_type mx_status;

	mx_status = mxd_v4l2_input_get_pointers( vinput, &v4l2_input, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
	case MXLV_VIN_FORMAT:
	case MXLV_VIN_PIXEL_ORDER:

		memset( &format, 0, sizeof(format) );

		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		os_status = ioctl( v4l2_input->fd, VIDIOC_G_FMT, &format );

		if ( os_status == -1 ) {
			saved_errno = errno;

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to get the current video format for V4L2 "
			"record '%s' failed.  "
			"Errno = %d, error message = '%s'.",
				vinput->record->name,
				saved_errno, strerror(saved_errno) );
		}
#if MXD_V4L2_INPUT_DEBUG
		MX_DEBUG(-2,("%s: width = %lu, height = %lu", fname,
			(unsigned long) format.fmt.pix.width,
			(unsigned long) format.fmt.pix.height));
		MX_DEBUG(-2,("%s: pixelformat = %lu", fname,
			(unsigned long) format.fmt.pix.pixelformat));
		MX_DEBUG(-2,("%s: field = %d",
			fname, format.fmt.pix.field));
#endif

		vinput->framesize[0] = format.fmt.pix.width;
		vinput->framesize[1] = format.fmt.pix.height;

		/* Decode the pixel format using the algorithm from
		 * the v4l2_fourcc() macro in <linux/videodev2.h>.
		 */

		pf = format.fmt.pix.pixelformat;

		A = pf & 0xff;
		B = (pf >> 8) & 0xff;
		C = (pf >> 16) & 0xff;
		D = (pf >> 24) & 0xff;

#if MXD_V4L2_INPUT_DEBUG
		MX_DEBUG(-2,("%s: pixelformat = %c%c%c%c", fname, A, B, C, D));
#endif
		switch( format.fmt.pix.pixelformat ) {
		case V4L2_PIX_FMT_RGB565:
			vinput->image_format = MXT_IMAGE_FORMAT_RGB565;
			break;
		case V4L2_PIX_FMT_YUYV:
			vinput->image_format = MXT_IMAGE_FORMAT_YUYV;
			break;
		default:
			vinput->image_format = -1;

			mx_warning(
			"Support for image format %c%c%c%c used by "
			"video input '%s' is not yet implemented.",
				A, B, C, D, vinput->record->name );
				
			break;
		}

		/* Save the pixel order. */

		switch( format.fmt.pix.field ) {
		case V4L2_FIELD_NONE:
		case V4L2_FIELD_INTERLACED:
			vinput->pixel_order = MXT_IMAGE_PIXEL_ORDER_STANDARD;
			break;
		default:
			vinput->pixel_order = -1;

			mx_warning(
			"Support for pixel order %d used by "
			"video input '%s' is not yet implemented.",
				format.fmt.pix.field, vinput->record->name );
				
			break;
		}

		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Parameter type %ld not yet implemented for record '%s'.",
			vinput->parameter_type, vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_v4l2_input_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_v4l2_input_set_parameter()";

	MX_V4L2_INPUT *v4l2_input;
	struct v4l2_format format;
	int os_status, saved_errno;
	mx_status_type mx_status;

	mx_status = mxd_v4l2_input_get_pointers( vinput, &v4l2_input, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_V4L2_INPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
	case MXLV_VIN_FORMAT:
	case MXLV_VIN_PIXEL_ORDER:

		/* Get the current settings. */

		memset( &format, 0, sizeof(format) );

		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		os_status = ioctl( v4l2_input->fd, VIDIOC_G_FMT, &format );

		if ( os_status == -1 ) {
			saved_errno = errno;

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to get the current video format for V4L2 "
			"record '%s' failed.  "
			"Errno = %d, error message = '%s'.",
				vinput->record->name,
				saved_errno, strerror(saved_errno) );
		}

		if ( vinput->parameter_type == MXLV_VIN_FRAMESIZE ) {

			format.fmt.pix.width  = vinput->framesize[0];
			format.fmt.pix.height = vinput->framesize[1];

#if MXD_V4L2_INPUT_DEBUG
			MX_DEBUG(-2,("%s: changing framesize to (%lu, %lu)",
			    fname, vinput->framesize[0], vinput->framesize[1]));
#endif
		} else
		if ( vinput->parameter_type == MXLV_VIN_FORMAT ) {

			switch( vinput->image_format ) {
			case MXT_IMAGE_FORMAT_RGB565:
			    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
			    break;
			case MXT_IMAGE_FORMAT_YUYV:
			    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
			    break;
			default:
			    return mx_error( MXE_UNSUPPORTED, fname,
				"Image format type %ld is not supported "
				"by video input '%s'.",
					vinput->image_format,
					vinput->record->name );
			}

#if MXD_V4L2_INPUT_DEBUG
			MX_DEBUG(-2,("%s: changing image format to %lu",
				fname, vinput->image_format ));
#endif
		}

		os_status = ioctl( v4l2_input->fd, VIDIOC_S_FMT, &format );

		if ( os_status == -1 ) {
			saved_errno = errno;

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to set the current video format for V4L2 "
			"record '%s' failed.  "
			"Errno = %d, error message = '%s'.",
				vinput->record->name,
				saved_errno, strerror(saved_errno) );
		}

		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Parameter type %ld not yet implemented for record '%s'.",
			vinput->parameter_type, vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_LINUX && HAVE_VIDEO_4_LINUX_2 */


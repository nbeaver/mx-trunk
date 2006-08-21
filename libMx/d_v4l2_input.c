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

#if defined(OS_LINUX) && HAVE_VIDEO_4_LINUX_2

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
	record->class_specific_function_list = NULL;

	vinput->record = record;
	v4l2_input->record = record;

	v4l2_input->fd = -1;

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
	struct v4l2_capability cap;
	struct v4l2_input input;
	int i, saved_errno, os_status;
	long input_number;
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

	os_status = ioctl( v4l2_input->fd, VIDIOC_QUERYCAP, &cap );

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
		fname, cap.driver, cap.card));

	MX_DEBUG(-2,
	("%s: bus_info = '%s', version = %lu, capabilities = %#lx",
		fname, cap.bus_info,
		(unsigned long) cap.version, (unsigned long) cap.capabilities));

	/* Display the capabilities. */

	fprintf(stderr,"%s: Capabilities: ", fname);

	if ( cap.capabilities & V4L2_CAP_VIDEO_CAPTURE ) {
		fprintf(stderr, "video_capture ");
	}
	if ( cap.capabilities & V4L2_CAP_VIDEO_OUTPUT ) {
		fprintf(stderr, "video_output ");
	}
	if ( cap.capabilities & V4L2_CAP_VIDEO_OVERLAY ) {
		fprintf(stderr, "video_overlay ");
	}
	if ( cap.capabilities & V4L2_CAP_VBI_CAPTURE ) {
		fprintf(stderr, "vbi_capture ");
	}
	if ( cap.capabilities & V4L2_CAP_VBI_OUTPUT ) {
		fprintf(stderr, "vbi_output ");
	}
	if ( cap.capabilities & V4L2_CAP_RDS_CAPTURE ) {
		fprintf(stderr, "rds_capture ");
	}
	if ( cap.capabilities & V4L2_CAP_TUNER ) {
		fprintf(stderr, "tuner ");
	}
	if ( cap.capabilities & V4L2_CAP_AUDIO ) {
		fprintf(stderr, "audio ");
	}
	if ( cap.capabilities & V4L2_CAP_READWRITE ) {
		fprintf(stderr, "readwrite ");
	}
	if ( cap.capabilities & V4L2_CAP_ASYNCIO ) {
		fprintf(stderr, "asyncio ");
	}
	if ( cap.capabilities & V4L2_CAP_STREAMING ) {
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

#endif /* OS_LINUX && HAVE_VIDEO_4_LINUX_2 */


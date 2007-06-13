/*
 * Name:    d_edt.c
 *
 * Purpose: MX driver for video inputs using EDT cameras.
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

#define MXD_EDT_DEBUG	TRUE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EDT

#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"

#include "edtinc.h"	/* Vendor include file. */

#include "i_edt.h"
#include "d_edt.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_edt_record_function_list = {
	NULL,
	mxd_edt_create_record_structures,
	mxd_edt_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_edt_open,
	mxd_edt_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_edt_video_input_function_list = {
	mxd_edt_arm,
	mxd_edt_trigger,
	mxd_edt_stop,
	mxd_edt_abort,
	NULL,
	NULL,
	mxd_edt_get_status,
	NULL,
	mxd_edt_get_frame,
	mxd_edt_get_parameter,
	mxd_edt_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_edt_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_EDT_STANDARD_FIELDS
};

long mxd_edt_num_record_fields
		= sizeof( mxd_edt_record_field_defaults )
			/ sizeof( mxd_edt_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_edt_rfield_def_ptr
			= &mxd_edt_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_edt_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_EDT_VIDEO_INPUT **edt_vinput,
			MX_EDT **edt,
			const char *calling_fname )
{
	static const char fname[] = "mxd_edt_get_pointers()";

	MX_EDT_VIDEO_INPUT *edt_vinput_ptr;
	MX_RECORD *edt_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	edt_vinput_ptr = vinput->record->record_type_struct;

	if ( edt_vinput_ptr == (MX_EDT_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EDT_VIDEO_INPUT pointer for record '%s' is NULL.",
			vinput->record->name );
	}

	if ( edt_vinput != (MX_EDT_VIDEO_INPUT **) NULL ) {
		*edt_vinput = edt_vinput_ptr;
	}

	if ( edt != (MX_EDT **) NULL ) {
		edt_record = edt_vinput_ptr->edt_record;

		if ( edt_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The edt_record pointer for record '%s' is NULL.",
				vinput->record->name );
		}

		*edt = edt_record->record_type_struct;

		if ( (*edt) == (MX_EDT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_EDT pointer for record '%s' is NULL.",
				vinput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_edt_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_edt_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_EDT_VIDEO_INPUT *edt_vinput;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	edt_vinput = (MX_EDT_VIDEO_INPUT *) malloc( sizeof(MX_EDT_VIDEO_INPUT));

	if ( edt_vinput == (MX_EDT_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_EDT_VIDEO_INPUT structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = edt_vinput;
	record->class_specific_function_list = 
			&mxd_edt_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	edt_vinput->record = record;

	vinput->bytes_per_frame = 0;
	vinput->bytes_per_pixel = 0;
	vinput->bits_per_pixel = 0;
	vinput->trigger_mode = 0;

	vinput->pixel_clock_frequency = 0.0;	/* in pixels/second */

	vinput->external_trigger_polarity = MXF_VIN_TRIGGER_NONE;

	vinput->camera_trigger_polarity = MXF_VIN_TRIGGER_NONE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_finish_record_initialization( MX_RECORD *record )
{
	return mx_video_input_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_edt_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_edt_open()";

	MX_VIDEO_INPUT *vinput;
	MX_EDT_VIDEO_INPUT *edt_vinput;
	MX_EDT *edt;
	unsigned int buffer_size;
	int i, edt_status, timeout;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, &edt, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	/* Connect to the EDT imaging board. */

	edt_vinput->pdv_p = pdv_open_channel( edt->device_name,
					edt_vinput->unit_number,
					edt_vinput->channel_number );

	if ( edt_vinput->pdv_p == (PdvDev *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to make a connection to EDT video input '%s'.  "
		"Errno = %u, error = '%s'.", record->name,
			edt_errno(), strerror( edt_errno() ) );
	}

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: EDT board id = %#x",
		fname, edt_vinput->pdv_p->devid));

	MX_DEBUG(-2,("%s: EDT camera type = '%s'.",
		fname, pdv_get_cameratype( edt_vinput->pdv_p ) ));

	MX_DEBUG(-2,("%s: EDT camera class = '%s'.",
		fname, pdv_get_camera_class( edt_vinput->pdv_p ) ));

	MX_DEBUG(-2,("%s: EDT firmware revision = %#x",
		fname, edt_reg_read( edt_vinput->pdv_p, PDV_REV ) ));

	MX_DEBUG(-2,("%s: EDT data path register = %#x",
		fname, edt_reg_read( edt_vinput->pdv_p, PDV_DATA_PATH ) ));

	MX_DEBUG(-2,("%s: EDT utility register = %#x",
		fname, edt_reg_read( edt_vinput->pdv_p, PDV_UTILITY ) ));

	MX_DEBUG(-2,("%s: EDT utility 2 register = %#x",
		fname, edt_reg_read( edt_vinput->pdv_p, PDV_UTIL2 ) ));
#endif

#if 1 /* WML WML WML */

	/* Check to see if this is a Camera Link board. */

	if ( edt_vinput->pdv_p->devid == PDVCL_ID ) {

		uint_t reg_value;

#if MXD_EDT_DEBUG
		MX_DEBUG(-2,("%s: EDT board '%s' is a Camera Link board.",
			fname, record->name ));
#endif
		/* If this is a Camera Link board, make sure that the
		 * EXPOSE signal is tied to CC1.
		 */

		reg_value = edt_reg_read( edt_vinput->pdv_p, PDV_MODE_CNTL );

#if MXD_EDT_DEBUG
		MX_DEBUG(-2,("%s: Mode control register = %#x",
			fname, reg_value));
#endif


		/* Check to see if we have been asked to invert
		 * the EXPOSE signal to the camera.
		 */

#if MXD_EDT_DEBUG
		MX_DEBUG(-2,("%s: Inverting EXPOSE signal for EDT board '%s'",
				fname, record->name ));
#endif
		reg_value = edt_reg_read(edt_vinput->pdv_p, PDV_UTIL3 );

#if MXD_EDT_DEBUG
		MX_DEBUG(-2,("%s: Utility 3 register (before) = %#x",
						fname, reg_value));
#endif
		reg_value &= ~0x1;    /* Mask off the low order PTRIGINV bit. */

		if ( edt_vinput->edt_flags == MXF_EDT_INVERT_EXPOSE_SIGNAL ) {
			reg_value |= 0x1;
		}

		edt_reg_write( edt_vinput->pdv_p, PDV_UTIL3, reg_value );

#if MXD_EDT_DEBUG
		MX_DEBUG(-2,("%s: Utility 3 register (after) = %#x",
						fname, reg_value));
#endif
	}

#endif /* WML WML WML */

	/* Set timeout to 5 seconds. */

	timeout = 5000;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: pdv_get_depth() = %d",
		fname, pdv_get_depth( edt_vinput->pdv_p ) ));
	MX_DEBUG(-2,("%s: pdv_get_extdepth() = %d",
		fname, pdv_get_extdepth( edt_vinput->pdv_p ) ));

	MX_DEBUG(-2,("%s: pdv_get_cam_height() = %d",
		fname, pdv_get_cam_height( edt_vinput->pdv_p ) ));
	MX_DEBUG(-2,("%s: pdv_get_cam_width() = %d",
		fname, pdv_get_cam_width( edt_vinput->pdv_p ) ));

	MX_DEBUG(-2,("%s: pdv_get_cam_height() = %d",
		fname, pdv_get_cam_height( edt_vinput->pdv_p ) ));
	MX_DEBUG(-2,("%s: pdv_get_cam_width() = %d",
		fname, pdv_get_cam_width( edt_vinput->pdv_p ) ));

	MX_DEBUG(-2,("%s: pdv_get_imagesize() = %d",
		fname, pdv_get_imagesize( edt_vinput->pdv_p ) ));

	MX_DEBUG(-2,("%s: pdv_get_exposure() = %d",
		fname, pdv_get_exposure( edt_vinput->pdv_p ) ));

	MX_DEBUG(-2,("%s: Setting timeout to %d milliseconds",
		fname, timeout));
#endif

	edt_status = pdv_set_timeout( edt_vinput->pdv_p, 5000 );

	if ( edt_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to allocate %lu frame buffers for "
			"EDT video input '%s' failed.  "
			"Errno = %u, error = '%s'.",
				edt_vinput->maximum_num_frames, record->name,
				edt_errno(), strerror( edt_errno() ) );
	}

	/* Allocate frame buffer space. */

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: About to allocate %ld frames with pdv_multibuf()",
		fname, edt_vinput->maximum_num_frames));
#endif

	edt_status = pdv_multibuf( edt_vinput->pdv_p,
					edt_vinput->maximum_num_frames );

	if ( edt_status != 0 ) {
		char msg[64];

		sprintf(msg, "pdv_multibuf(0x%p, %lu)",
			edt_vinput->pdv_p, edt_vinput->maximum_num_frames);
		pdv_perror(msg);

		if ( edt_errno() == 0 ) {
			mx_warning("edt_status = %d, but edt_errno() = %d",
				edt_status, edt_errno() );
		} else {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to allocate %lu frame buffers for "
			"EDT video input '%s' failed.  "
			"Errno = %u, error = '%s'.",
				edt_vinput->maximum_num_frames, record->name,
				edt_errno(), strerror( edt_errno() ) );
		}
	}

	for ( i = 0; i < edt_vinput->maximum_num_frames; i++ ) {
		buffer_size = edt_allocated_size( edt_vinput->pdv_p, i );

		MX_DEBUG(-2,("%s: buffer[%d] allocated size = %u bytes.",
			fname, i, buffer_size));
	}

	edt_vinput->last_done_count = edt_done_count( edt_vinput->pdv_p );

	edt_vinput->num_timeouts = pdv_timeouts( edt_vinput->pdv_p );

	/* Initialize a bunch of driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->pixel_order    = MXT_IMAGE_PIXEL_ORDER_STANDARD;
	vinput->trigger_mode   = MXT_IMAGE_INTERNAL_TRIGGER;

	mx_status = mx_video_input_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bits_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_framesize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_edt_close()";

	MX_VIDEO_INPUT *vinput;
	MX_EDT_VIDEO_INPUT *edt_vinput;
	int edt_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	if ( edt_vinput->pdv_p != NULL ) {
		edt_status = pdv_close( edt_vinput->pdv_p );

		if ( edt_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Error closing PdvDev structure for EDT video input '%s'.  "
		"Errno = %u, error = '%s'.", record->name,
			edt_errno(), strerror( edt_errno() ) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_arm()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	/* If external triggering is not enabled,
	 * return without doing anything further.
	 */

	if ( ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) == 0 ) {

#if MXD_EDT_DEBUG
		MX_DEBUG(-2,
		("%s: external trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_trigger()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	MX_SEQUENCE_PARAMETERS *sp;
	int edt_status, num_frames, milliseconds;
	double exposure_time;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	if ( ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {

		/* If internal triggering is not enabled,
		 * return without doing anything
		 */

#if MXD_EDT_DEBUG
		MX_DEBUG(-2,
		("%s: internal trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/*---*/

	sp = &(vinput->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		exposure_time = sp->parameter_array[0];
		num_frames = 1;
		break;
	case MXT_SQ_MULTIFRAME:
		exposure_time = sp->parameter_array[1];
		num_frames = mx_round( sp->parameter_array[0] );
		break;
	default:
		exposure_time = -1;
		num_frames = 0;
		break;
	}

	if ( num_frames > edt_vinput->maximum_num_frames ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested number of sequence frames (%d) for "
		"video input '%s' is larger than the maximum value "
		"of %lu.", num_frames, vinput->record->name,
			edt_vinput->maximum_num_frames );
			
	}

	if ( exposure_time >= 0.0 ) {
		/* Set the exposure time. */

		milliseconds = mx_round( 1000.0 * exposure_time );

#if MXD_EDT_DEBUG
		MX_DEBUG(-2,
	("%s: Setting exposure to %u milliseconds using pdv_set_exposure()",
			fname, milliseconds ));
#endif

		edt_status = pdv_set_exposure(edt_vinput->pdv_p, milliseconds);

		if ( edt_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to set the exposure time to %g for EDT "
			"video input '%s' failed.  Errno = %u, error = '%s'.",
				exposure_time, vinput->record->name,
				edt_errno(), strerror( edt_errno() ) );
		}
	}

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: About to trigger using pdv_start_images()", fname ));
#endif

	edt_vinput->last_done_count = edt_done_count( edt_vinput->pdv_p );

	edt_vinput->num_timeouts = pdv_timeouts( edt_vinput->pdv_p );

	/* Start the actual data acquisition. */

	pdv_start_images( edt_vinput->pdv_p, num_frames );

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: Started taking %d frame(s) using video input '%s'.",
		fname, num_frames, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_stop()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	int edt_status;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	edt_status = edt_abort_dma( edt_vinput->pdv_p );

	if ( edt_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to stop image acquisition failed for record '%s'.",
			vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_abort()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	int edt_status;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	edt_status = edt_abort_dma( edt_vinput->pdv_p );

	if ( edt_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to stop image acquisition failed for record '%s'.",
			vinput->record->name );
	}


	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_get_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_get_status()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	int done_count, busy, num_timeouts, overrun;
	uint status_register;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status_register = edt_reg_read( edt_vinput->pdv_p, PDV_STAT );

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: EDT status register = %#x", fname, status_register));
#endif

	if ( status_register == 0x30 ) {
		busy = FALSE;
	} else {
		busy = TRUE;
	}

	if ( busy ) {
		vinput->busy = TRUE;

		vinput->status |= MXSF_VIN_IS_BUSY;
	} else {
		vinput->busy = FALSE;

		vinput->status &= ~MXSF_VIN_IS_BUSY;
	}

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: busy = %d", fname, vinput->busy ));
#endif

	done_count = edt_done_count( edt_vinput->pdv_p );

	vinput->total_num_frames = done_count;

	if ( vinput->maximum_frame_number == 0 ) {
		vinput->last_frame_number = 0;
	} else {
		vinput->last_frame_number
			= done_count % vinput->maximum_frame_number;
	}

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: edt_done_count() = %ld",
		fname, (long) edt_done_count(edt_vinput->pdv_p ) ));
#endif

	num_timeouts = pdv_timeouts( edt_vinput->pdv_p );

	if ( num_timeouts > edt_vinput->num_timeouts ) {
		vinput->status |= MXSF_VIN_TIMEOUT;
	}

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: num_timeouts = %d", fname, num_timeouts));
#endif

	overrun = pdv_overrun( edt_vinput->pdv_p );

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: overrun = %d", fname, overrun));
#endif

	if ( overrun ) {
		vinput->status |= MXSF_VIN_OVERRUN;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_get_frame()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	MX_IMAGE_FRAME *frame;
	unsigned char **ring_buffer_array;
	unsigned char *image_data;
	u_int time_array[2];
	u_int buffer_number;
	int edt_status, done_count;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	/* Compute the EDT buffer number from the MX frame number. */

	if ( vinput->maximum_frame_number == 0 ) {
		buffer_number = 0;
	} else {
		if ( vinput->frame_number == (-1) ) {
			done_count = edt_done_count( edt_vinput->pdv_p );
		} else {
			done_count = edt_vinput->last_done_count
						+ vinput->frame_number;
		}
		buffer_number
			= ( done_count - 1 ) % vinput->maximum_frame_number;
	}

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: buffer_number = %u", fname, buffer_number));
#endif

	/* Get the address of the image buffer. */

	ring_buffer_array = pdv_buffer_addresses( edt_vinput->pdv_p );

	if ( ring_buffer_array == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The ring_buffer_array for EDT record '%s' has not "
		"been initialized.", vinput->record->name );
	}

	image_data = ring_buffer_array[ buffer_number ];

	if ( image_data == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The image_data pointer for buffer %u of EDT record '%s' "
		"has not been initialized.",
			buffer_number, vinput->record->name);
	}

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: Copying a %lu byte image frame.",
			fname, (unsigned long) frame->image_length));
#endif

	memcpy( frame->image_data, image_data, frame->image_length );

	/* Get the timestamp for the frame. */

	edt_status = edt_get_timestamp( edt_vinput->pdv_p,
					time_array, buffer_number );

#if 0
	if ( edt_status != 0 ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unable to get the timestamp for EDT buffer %u of record '%s'.",
			buffer_number, vinput->record->name );
	}
#endif

	frame->image_time.tv_sec = time_array[0];
	frame->image_time.tv_nsec = 1000L * (long) time_array[1];

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: Frame successfully copied.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_get_parameter()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	int bit_depth;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_PIXEL_ORDER:
	case MXLV_VIN_TRIGGER_MODE:
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:

		break;

	case MXLV_VIN_BYTES_PER_FRAME:
	case MXLV_VIN_BYTES_PER_PIXEL:
		switch( vinput->image_format ) {
		case MXT_IMAGE_FORMAT_RGB:
			vinput->bytes_per_pixel = 3;
			break;
		case MXT_IMAGE_FORMAT_GREY8:
			vinput->bytes_per_pixel = 1;
			break;
	
		case MXT_IMAGE_FORMAT_GREY16:
			vinput->bytes_per_pixel = 2;
			break;
	
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported image format %ld for video input '%s'.",
				vinput->image_format, vinput->record->name );
		}

		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel )
				* vinput->framesize[0] * vinput->framesize[1];
		break;

	case MXLV_VIN_BITS_PER_PIXEL:
		vinput->bits_per_pixel = pdv_get_depth( edt_vinput->pdv_p );
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:

		/* FIXME - The following logic is rather lame. */

		bit_depth = pdv_get_depth( edt_vinput->pdv_p );

		if ( bit_depth <= 8 ) {
			vinput->image_format = MXT_IMAGE_FORMAT_GREY8;
		} else
		if ( bit_depth <= 16 ) {
			vinput->image_format = MXT_IMAGE_FORMAT_GREY16;
		} else {
			vinput->image_format = MXT_IMAGE_FORMAT_RGB;
		}

		mx_status = mx_image_get_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_EDT_DEBUG
		MX_DEBUG(-2,("%s: bit_depth = %d, video format = %ld '%s'",
			fname, bit_depth, vinput->image_format,
			vinput->image_format_name));
		 	
#endif
		break;

	case MXLV_VIN_FRAMESIZE:
		vinput->framesize[0] = pdv_get_cam_width( edt_vinput->pdv_p );
		vinput->framesize[1] = pdv_get_cam_height( edt_vinput->pdv_p );
		break;
	default:
		return mx_video_input_default_get_parameter_handler( vinput );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_set_parameter()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	int edt_status;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:

		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		(void) mxd_edt_get_parameter( vinput );

		return mx_error( MXE_UNSUPPORTED, fname,
		"Changing the video format for video input '%s' via "
		"MX is not supported.", vinput->record->name );
		break;

	case MXLV_VIN_PIXEL_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the pixel order for video input '%s' "
			"is not supported.", vinput->record->name );
		break;

	case MXLV_VIN_FRAMESIZE:

#if MXD_EDT_DEBUG
		MX_DEBUG(-2,("%s: setting '%s' framesize to (%lu, %lu)",
			fname, vinput->record->name,
			vinput->framesize[0], vinput->framesize[1]));
#endif
		edt_status = pdv_setsize( edt_vinput->pdv_p,
			vinput->framesize[0], vinput->framesize[1] );

		if ( edt_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to set the framesize for EDT video "
			"input '%s' to (%lu, %lu) failed.  "
			"Errno = %u, error = '%s'.", vinput->record->name,
				vinput->framesize[0], vinput->framesize[1],
				edt_errno(), strerror( edt_errno() ) );
		}
		break;

	case MXLV_VIN_TRIGGER_MODE:
		break;

	default:
		return mx_video_input_default_set_parameter_handler( vinput );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_EDT */


/*
 * Name:    d_edt.c
 *
 * Purpose: MX driver for video inputs using EDT cameras.
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

#define MXD_EDT_DEBUG	TRUE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EDT

#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"

#include "edtinc.h"

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
	mxd_edt_busy,
	mxd_edt_get_status,
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
			MX_EDT_VIDEO_INPUT **edt,
			const char *calling_fname )
{
	static const char fname[] = "mxd_edt_get_pointers()";

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (edt == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_EDT_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*edt = (MX_EDT_VIDEO_INPUT *)
				vinput->record->record_type_struct;

	if ( *edt == (MX_EDT_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_EDT_VIDEO_INPUT pointer for record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
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
	vinput->trigger_mode = 0;

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
	MX_RECORD *edt_record;
	MX_EDT *edt;
	int edt_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	edt_record = edt_vinput->edt_record;

	if ( edt_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The edt_record pointer for EDT video input '%s' is NULL.",
			record->name );
	}

	edt = (MX_EDT *) edt_record->record_type_struct;

	if ( edt == (MX_EDT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EDT pointer for EDT record '%s' used by "
		"EDT video input '%s' is NULL.",
			edt_record->name, record->name );
	}

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
	MX_DEBUG(-2,("%s: EDT camera type = '%s'.",
		fname, pdv_get_cameratype( edt_vinput->pdv_p ) ));
#endif
	/* Allocate frame buffer space. */

	edt_status = pdv_multibuf( edt_vinput->pdv_p,
					edt_vinput->maximum_num_frames );

	if ( edt_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to allocate %lu frame buffers for "
		"EDT video input '%s' failed.  Errno = %u, error = '%s'.",
			edt_vinput->maximum_num_frames, record->name,
			edt_errno(), strerror( edt_errno() ) );
	}

	/* Initialize a bunch of driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->pixel_order    = MXT_IMAGE_PIXEL_ORDER_STANDARD;
	vinput->trigger_mode   = MXT_IMAGE_NO_TRIGGER;

	mx_status = mx_video_input_get_image_format( record, NULL );

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

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

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

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	if ( ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) == 0 ) {

		/* If external triggering is not enabled,
		 * return without doing anything
		 */

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

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

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

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: sp = %p", fname, sp));

	MX_DEBUG(-2,("%s: sp->sequence_type = %ld", fname, sp->sequence_type));
#endif

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: triggering one shot mode for vinput '%s'.",
		fname, vinput->record->name ));
#endif
		num_frames = 1;
		exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_CONTINUOUS:

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: triggering continuous mode for vinput '%s'.",
		fname, vinput->record->name ));
#endif
		num_frames = 0;
		exposure_time = sp->parameter_array[0];
		break;

	case MXT_SQ_MULTIFRAME:
		if ( sp->num_parameters < 1 ) {
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"The first sequence parameter of video input '%s' "
			"for a sequence of type MXT_SQ_MULTI should be "
			"the number of frames.  "
			"However, the sequence says that it has %ld frames.",
			    vinput->record->name, sp->num_parameters );
				
		}

		num_frames = mx_round( sp->parameter_array[0] );

		if ( num_frames > edt_vinput->maximum_num_frames ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of sequence frames (%d) for "
			"video input '%s' is larger than the maximum value "
			"of %lu.", num_frames, vinput->record->name,
				edt_vinput->maximum_num_frames );
			
		}

		exposure_time = sp->parameter_array[1];

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: triggering multiframe mode for vinput '%s'.",
		fname, vinput->record->name ));
	MX_DEBUG(-2,("%s: num_frames = %d", fname, num_frames));
#endif
		break;

	case MXT_SQ_CIRCULAR_MULTIFRAME:
		if ( sp->num_parameters < 1 ) {
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"The first sequence parameter of video input '%s' "
			"for a sequence of type MXT_SQ_CONTINUOUS_MULTI should "
			"be the number of frames in the circular buffer.  "
			"However, the sequence says that it has %ld frames.",
			    vinput->record->name, sp->num_parameters );
		}

		num_frames = mx_round( sp->parameter_array[0] );

		if ( num_frames > edt_vinput->maximum_num_frames ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of sequence frames (%d) for "
			"video input '%s' is larger than the maximum value "
			"of %lu.", num_frames, vinput->record->name,
				edt_vinput->maximum_num_frames );
			
		}

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: triggering circular multiframe mode for vinput '%s'.",
		fname, vinput->record->name ));
	MX_DEBUG(-2,("%s: num_frames = %d", fname, num_frames));
#endif

		return mx_error( MXE_UNSUPPORTED, fname,
		"Circular multiframe mode is not currently supported "
		"for EDT video input '%s'.", vinput->record->name );
		break;

	case MXT_SQ_STROBE:
	case MXT_SQ_BULB:
		/* These modes use external triggers, so we do nothing here. */
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"video input '%s'.",
			sp->sequence_type, vinput->record->name );
	}

	/* Set the exposure time. */

	milliseconds = mx_round( 1000.0 * exposure_time );

	edt_status = pdv_set_exposure( edt_vinput->pdv_p, milliseconds );

	if ( edt_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to set the exposure time to %g for "
		"EDT video input '%s' failed.  Errno = %u, error = '%s'.",
			exposure_time, vinput->record->name,
			edt_errno(), strerror( edt_errno() ) );
	}

	/* Start the actual data acquisition. */

	pdv_start_images( edt_vinput->pdv_p, num_frames );

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s: Started taking frame(s) using video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_stop()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	pdv_start_images( edt_vinput->pdv_p, 1 );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_abort()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	pdv_start_images( edt_vinput->pdv_p, 1 );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_busy( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_busy()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	int busy;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	busy = FALSE;		/* FIXME!!! */

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

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_get_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_get_status()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	mx_status = mxd_edt_busy( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_edt_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_get_frame()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	MX_IMAGE_FRAME *frame;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

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

	MX_DEBUG(-2,("%s: FIXME! - Frame readout not yet implemented.",fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_edt_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_edt_get_parameter()";

	MX_EDT_VIDEO_INPUT *edt_vinput;
	int bit_depth;
	mx_status_type mx_status;

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

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
		vinput->framesize[0] = pdv_get_width( edt_vinput->pdv_p );
		vinput->framesize[1] = pdv_get_height( edt_vinput->pdv_p );
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Parameter type %ld not yet implemented for record '%s'.",
			vinput->parameter_type, vinput->record->name );
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

	mx_status = mxd_edt_get_pointers( vinput, &edt_vinput, fname );

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
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Parameter type %ld not yet implemented for record '%s'.",
			vinput->parameter_type, vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_EDT */


/*
 * Name:    d_epix_xclib.c
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

#define MXD_EPIX_XCLIB_DEBUG	TRUE

#define USE_CLCCSE_REGISTER	TRUE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPIX_XCLIB

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_epix_xclib.h"
#include "d_epix_xclib.h"

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include "xcliball.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_epix_xclib_record_function_list = {
	NULL,
	mxd_epix_xclib_create_record_structures,
	mxd_epix_xclib_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_epix_xclib_open,
	mxd_epix_xclib_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_epix_xclib_video_input_function_list = {
	mxd_epix_xclib_arm,
	mxd_epix_xclib_trigger,
	mxd_epix_xclib_stop,
	mxd_epix_xclib_abort,
	mxd_epix_xclib_busy,
	mxd_epix_xclib_get_status,
	mxd_epix_xclib_get_frame,
	mxd_epix_xclib_get_sequence,
	mxd_epix_xclib_get_parameter,
	mxd_epix_xclib_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_epix_xclib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_EPIX_XCLIB_STANDARD_FIELDS
};

long mxd_epix_xclib_num_record_fields
		= sizeof( mxd_epix_xclib_record_field_defaults )
			/ sizeof( mxd_epix_xclib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epix_xclib_rfield_def_ptr
			= &mxd_epix_xclib_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_epix_xclib_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_EPIX_XCLIB_VIDEO_INPUT **epix_xclib,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epix_xclib_get_pointers()";

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (epix_xclib == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_EPIX_XCLIB_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*epix_xclib = (MX_EPIX_XCLIB_VIDEO_INPUT *)
				vinput->record->record_type_struct;

	if ( *epix_xclib == (MX_EPIX_XCLIB_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_EPIX_XCLIB_VIDEO_INPUT pointer for record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

#if USE_CLCCSE_REGISTER

static mx_status_type
mxd_epix_xclib_camera_link_set_line( MX_VIDEO_INPUT *vinput,
				MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput,
				int camera_line, int line_state )
{
	static const char fname[] = "mxd_epix_xclib_camera_set_line()";

	uint16 CLCCSE;
	int epix_status;

	struct xclibs *xc;
	xclib_DeclareVidStateStructs(vidstate);

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
			fname, vinput->record->name));

	MX_DEBUG(-2,("%s: camera_line = %d, line_state = %d",
			fname, camera_line, line_state));
#endif

	xc = pxd_xclibEscape(0, 0, 0);

	if ( xc == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The XCLIB library has not yet been initialized with "
		"pxd_PIXCIopen() for video input '%s'.", vinput->record->name );
	}

	xclib_InitVidStateStructs(vidstate);

	xc->pxlib.getState( &(xc->pxlib), 0, PXMODE_DIGI, &vidstate );

	CLCCSE = vidstate.xc.dxxformat->CLCCSE;

	MX_DEBUG(-2,("%s: Old CLCCSE = %#x", fname, CLCCSE));

	switch( line_state ) {
	case 1:   /* High */

		switch( camera_line ) {
		case MX_EPIX_XCLIB_CC1:
			/* CC1 is controlled by bits 0 to 3. */

			/* CLCCSE &= 0xfff0; */
			CLCCSE |= 0x0001;
			break;

		case MX_EPIX_XCLIB_CC2:
			/* CC2 is controlled by bits 4 to 7. */

			CLCCSE &= 0xff0f;
			CLCCSE |= 0x0010;
			break;

		case MX_EPIX_XCLIB_CC3:
			/* CC3 is controlled by bits 8 to 11. */

			CLCCSE &= 0xf0ff;
			CLCCSE |= 0x0100;
			break;

		case MX_EPIX_XCLIB_CC4:
			/* CC4 is controlled by bits 12 to 15. */

			CLCCSE &= 0x0fff;
			CLCCSE |= 0x1000;
			break;

		default:
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal Camera Link line %d was requested "
			"for video input '%s'.", camera_line,
				vinput->record->name );
		}
		break;

	case 0:   /* Low */

		switch( camera_line ) {
		case MX_EPIX_XCLIB_CC1:
			/* CC1 is controlled by bits 0 to 3. */

			CLCCSE &= 0xfff0;
			break;

		case MX_EPIX_XCLIB_CC2:
			/* CC2 is controlled by bits 4 to 7. */

			CLCCSE &= 0xff0f;
			break;

		case MX_EPIX_XCLIB_CC3:
			/* CC3 is controlled by bits 8 to 11. */

			CLCCSE &= 0xf0ff;
			break;

		case MX_EPIX_XCLIB_CC4:
			/* CC4 is controlled by bits 12 to 15. */

			CLCCSE &= 0x0fff;
			break;

		default:
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal Camera Link line %d was requested "
			"for video input '%s'.", camera_line,
				vinput->record->name );
		}
		break;

	default:
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal line state %d was requested for Camera Link line %d "
		"of video input '%s'.", line_state, camera_line,
			vinput->record->name );
	}

	MX_DEBUG(-2,("%s: New CLCCSE = %#x", fname, CLCCSE));

	vidstate.xc.dxxformat->CLCCSE = CLCCSE;

	epix_status = xc->pxlib.defineState(
				&(xc->pxlib), 0, PXMODE_DIGI, &vidstate );

	MX_DEBUG(-2,("%s: epix_status #1 = %d", fname, epix_status));

	epix_status = pxd_xclibEscaped(0, 0, 0);

	MX_DEBUG(-2,("%s: epix_status #2 = %d", fname, epix_status));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_epix_xclib_internal_trigger( MX_VIDEO_INPUT *vinput,
				MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput,
				mx_bool_type trigger_on )
{
	static const char fname[] = "mxd_epix_xclib_internal_trigger()";

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
			fname, vinput->record->name));
#endif

	epix_xclib_vinput->generate_cc1_pulse = trigger_on;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: generate_cc1_pulse = %d",
			fname, epix_xclib_vinput->generate_cc1_pulse));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*...*/

#else /* not USE_CLCCSE_REGISTER */

/*...*/

static mx_status_type
mxd_epix_xclib_internal_trigger( MX_VIDEO_INPUT *vinput,
				MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput,
				mx_bool_type trigger_on )
{
	static const char fname[] = "mxd_epix_xclib_internal_trigger()";

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
			fname, vinput->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

#endif /* not USE_CLCCSE_REGISTER */

/*---*/

static mx_status_type
mxd_epix_xclib_external_trigger( MX_VIDEO_INPUT *vinput,
				MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput,
				mx_bool_type trigger_on )
{
	static const char fname[] = "mxd_epix_xclib_external_trigger()";

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
			fname, vinput->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_epix_xclib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_epix_xclib_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	epix_xclib_vinput = (MX_EPIX_XCLIB_VIDEO_INPUT *)
				malloc( sizeof(MX_EPIX_XCLIB_VIDEO_INPUT) );

	if ( epix_xclib_vinput == (MX_EPIX_XCLIB_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_EPIX_XCLIB_VIDEO_INPUT structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = epix_xclib_vinput;
	record->class_specific_function_list = 
			&mxd_epix_xclib_video_input_function_list;

	memset( &(vinput->sequence_info), 0, sizeof(vinput->sequence_info) );

	vinput->record = record;
	epix_xclib_vinput->record = record;

	vinput->trigger_mode = 0;

#if USE_CLCCSE_REGISTER
	epix_xclib_vinput->generate_cc1_pulse = FALSE;
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epix_xclib_open()";

	MX_VIDEO_INPUT *vinput;
	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	epix_xclib_vinput->unitmap = 1 << (epix_xclib_vinput->unit_number - 1);

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: board model = %#x",
		fname, pxd_infoModel( epix_xclib_vinput->unitmap ) ));

	MX_DEBUG(-2,("%s: board submodel = %#x",
		fname, pxd_infoSubmodel( epix_xclib_vinput->unitmap ) ));

	MX_DEBUG(-2,("%s: board memory = %lu bytes",
		fname, pxd_infoMemsize( epix_xclib_vinput->unitmap ) ));
#endif

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_arm()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	/* FIXME - Nothing here for now. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_trigger()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	MX_SEQUENCE_INFO *sq;
	pxbuffer_t startbuf, endbuf, numbuf;
	char error_message[80];
	int epix_status;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	sq = &(vinput->sequence_info);

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: sq = %p", fname, sq));

	MX_DEBUG(-2,("%s: sq->sequence_type = %ld", fname, sq->sequence_type));
#endif

#if USE_CLCCSE_REGISTER
	if ( epix_xclib_vinput->generate_cc1_pulse ) {

		MX_DEBUG(-2,("%s: CC1 pulse.", fname));

		/* Force CC1 high. */

		mx_status = mxd_epix_xclib_camera_link_set_line(
						vinput, epix_xclib_vinput, 
						MX_EPIX_XCLIB_CC1, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;


		/* Force CC1 low. */

		mx_status = mxd_epix_xclib_camera_link_set_line(
						vinput, epix_xclib_vinput, 
						MX_EPIX_XCLIB_CC1, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
#endif

	switch( sq->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		epix_status = pxd_goSnap( epix_xclib_vinput->unitmap, 1 );

		if ( epix_status != 0 ) {
			mxi_epix_xclib_error_message(
				epix_xclib_vinput->unitmap, epix_status,
				error_message, sizeof(error_message) );

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to take a frame with "
			"video input '%s' failed.  %s",
				vinput->record->name, error_message );
		}
		break;
	case MXT_SQ_CONTINUOUS:
		epix_status = pxd_goLive( epix_xclib_vinput->unitmap, 1 );

		if ( epix_status != 0 ) {
			mxi_epix_xclib_error_message(
				epix_xclib_vinput->unitmap, epix_status,
				error_message, sizeof(error_message) );

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to start continuous single frame "
			"acquisition with video input '%s' failed.  %s",
				vinput->record->name, error_message );
		}
		break;

	case MXT_SQ_MULTI:
		if ( sq->num_sequence_parameters < 1 ) {
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"The first sequence parameter of video input '%s' "
			"for a sequence of type MXT_SQ_MULTI should be "
			"the number of frames.  "
			"However, the sequence says that it has %ld frames.",
			    vinput->record->name, sq->num_sequence_parameters );
				
		}

		numbuf = mx_round( sq->sequence_parameters[0] );

		if ( numbuf > pxd_imageZdim() ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of sequence frames (%ld) for "
			"video input '%s' is larger than the maximum value "
			"of %d.",
			    numbuf, vinput->record->name, pxd_imageZdim());
		}

		startbuf = 1;
		endbuf = startbuf + numbuf - 1;

		epix_status = pxd_goLiveSeq( epix_xclib_vinput->unitmap,
					startbuf, endbuf, 1, numbuf, 1 );

		if ( epix_status != 0 ) {
			mxi_epix_xclib_error_message(
				epix_xclib_vinput->unitmap, epix_status,
				error_message, sizeof(error_message) );

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to start multi frame "
			"acquisition with video input '%s' failed.  %s",
				vinput->record->name, error_message );
		}
		break;

	case MXT_SQ_CONTINUOUS_MULTI:
		if ( sq->num_sequence_parameters < 1 ) {
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"The first sequence parameter of video input '%s' "
			"for a sequence of type MXT_SQ_CONTINUOUS_MULTI should "
			"be the number of frames in the circular buffer.  "
			"However, the sequence says that it has %ld frames.",
			    vinput->record->name, sq->num_sequence_parameters );
		}

		endbuf = mx_round( sq->sequence_parameters[0] );

		if ( endbuf > pxd_imageZdim() ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of sequence frames (%ld) for "
			"video input '%s' is larger than the maximum value "
			"of %d.",
			    endbuf, vinput->record->name, pxd_imageZdim());
		}

		startbuf = 1;

		epix_status = pxd_goLiveSeq( epix_xclib_vinput->unitmap,
					startbuf, endbuf, 1, 0, 1 );

		if ( epix_status != 0 ) {
			mxi_epix_xclib_error_message(
				epix_xclib_vinput->unitmap, epix_status,
				error_message, sizeof(error_message) );

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to start continuous multi frame "
			"acquisition with video input '%s' failed.  %s",
				vinput->record->name, error_message );
		}
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"video input '%s'.",
			sq->sequence_type, vinput->record->name );
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: Successfully took frame using video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_stop()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	char error_message[80];
	int epix_status;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	epix_status = pxd_goUnLive( epix_xclib_vinput->unitmap );

	if ( epix_status != 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, epix_status,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to stop taking frames for "
			"video input '%s' failed.  %s",
				vinput->record->name, error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_abort()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	char error_message[80];
	int epix_status;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	epix_status = pxd_goAbortLive( epix_xclib_vinput->unitmap );

	if ( epix_status != 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, epix_status,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to abort taking frames for "
			"video input '%s' failed.  %s",
				vinput->record->name, error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_busy( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_busy()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	int busy;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	busy = pxd_goneLive( epix_xclib_vinput->unitmap, 0 );

	if ( busy ) {
		vinput->busy = TRUE;

		vinput->status |= MXSF_VIN_IS_BUSY;
	} else {
		vinput->busy = FALSE;

		vinput->status &= ~MXSF_VIN_IS_BUSY;
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: vinput->busy = %d", fname, vinput->busy ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_get_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_get_status()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	int busy;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	mx_status = mxd_epix_xclib_busy( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_get_frame( MX_VIDEO_INPUT *vinput, MX_IMAGE_FRAME **frame )
{
	static const char fname[] = "mxd_epix_xclib_get_frame()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_get_sequence( MX_VIDEO_INPUT *vinput,
				MX_IMAGE_SEQUENCE **sequence )
{
	static const char fname[] = "mxd_epix_xclib_get_sequence()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sequence == (MX_IMAGE_SEQUENCE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_SEQUENCE pointer passed was NULL." );
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_get_parameter()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
	case MXLV_VIN_FORMAT:
	case MXLV_VIN_PIXEL_ORDER:
	case MXLV_VIN_TRIGGER_MODE:
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETERS:

		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Parameter type %ld not yet implemented for record '%s'.",
			vinput->parameter_type, vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_set_parameter()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
	case MXLV_VIN_FORMAT:
	case MXLV_VIN_PIXEL_ORDER:
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETERS:

		break;

	case MXLV_VIN_TRIGGER_MODE:
		if ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {

			mx_status = mxd_epix_xclib_internal_trigger(
					vinput, epix_xclib_vinput, TRUE );
		} else {
			mx_status = mxd_epix_xclib_internal_trigger(
					vinput, epix_xclib_vinput, FALSE );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) {

			mx_status = mxd_epix_xclib_external_trigger(
					vinput, epix_xclib_vinput, TRUE );
		} else {
			mx_status = mxd_epix_xclib_external_trigger(
					vinput, epix_xclib_vinput, FALSE );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Parameter type %ld not yet implemented for record '%s'.",
			vinput->parameter_type, vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_LINUX && HAVE_VIDEO_4_LINUX_2 */


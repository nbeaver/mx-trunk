/*
 * Name:    d_epix_xclib.c
 *
 * Purpose: MX driver for EPIX, Inc. video inputs via XCLIB.
 *
 * Author:  William Lavender
 *
 * WARNING: So far this driver has only been tested with PIXCI E4 cameras
 *          using 16-bit greyscale pixels.  Use of other formats may require
 *          updates to this driver.
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

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include "xcliball.h"

#include "i_epix_xclib.h"
#include "d_epix_xclib.h"

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

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: Old CLCCSE = %#x", fname, CLCCSE));
#endif

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

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: New CLCCSE = %#x", fname, CLCCSE));
#endif

	vidstate.xc.dxxformat->CLCCSE = CLCCSE;

	epix_status = xc->pxlib.defineState(
				&(xc->pxlib), 0, PXMODE_DIGI, &vidstate );

	if ( epix_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Error in xc->pxlib.defineState() for record '%s'.  "
		"Error code = %d", vinput->record->name, epix_status );
	}

	epix_status = pxd_xclibEscaped(0, 0, 0);

	if ( epix_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Error in pxd_xclibEscaped() for record '%s'.  "
		"Error code = %d", vinput->record->name, epix_status );
	}


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

	unsigned int exsync_mode, princ_mode, mask;
	unsigned int epcd, trigneg, exps, cnts, tis;
	int epix_status;
	char error_message[80];

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s', trigger_on = %d",
			fname, vinput->record->name, (int) trigger_on ));
#endif

	exsync_mode = pxd_getExsyncMode( epix_xclib_vinput->unitmap );
	
	princ_mode  = pxd_getPrincMode( epix_xclib_vinput->unitmap );

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: old exsync_mode = %#x, old princ_mode = %#x",
		fname, exsync_mode, princ_mode));
#endif

	/* Construct the parts of the new PRINC mode. */

	/* Mask off the bits used by this imaging board. */

	mask = 0x397;	/* Bits 9-7, 4, 2-0 */

	princ_mode &= ~mask;

	/* Bits 9-7, EPCD, Exposure Pixel Clock Divide. */

	epcd = 0x7 << 7;     /* Pixel clock divide by 512. */

	princ_mode |= epcd;

	/* Bit 4, TRIGNEG, Trigger Input Polarity Select. */

	trigneg = 0x0 << 4;  /* Trigger on positive edge. */

	princ_mode |= trigneg;

	/* Bit 2, EXPS, EXSYNC Polarity Select. */

	exps = 0x0 << 2;     /* Negative CC1 pulse. */

	princ_mode |= exps;

	/* Bit 1, CNTS, Continuous Select. */

	cnts = 0x0 << 1;     /* One shot mode. */

	princ_mode |= cnts;

	/* Bit 0, TIS, Trigger Input Select. */

	tis = 0x0;           /* Ignore external trigger. */

	if ( ( tis != 0 ) & ( cnts == 0 ) ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The combination of TIS=1 and CNTS=0 in the PRINC trigger "
		"control register is not supported for video input '%s'.",
			vinput->record->name );
	}	

	princ_mode |= tis;
	
#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: new exsync_mode = %#x, new princ_mode = %#x",
		fname, exsync_mode, princ_mode));
#endif

	epix_status = pxd_setExsyncPrincMode( epix_xclib_vinput->unitmap,
						exsync_mode, princ_mode );

	if ( epix_status != 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, epix_status,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"pxd_setExsyncPrincMode( %#x, %#x, %#x ) failed.  %s",
			epix_xclib_vinput->unitmap, exsync_mode, princ_mode,
			error_message );
	}

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
	MX_DEBUG(-2,("%s invoked for video input '%s', trigger_on = %d",
			fname, vinput->record->name, (int) trigger_on ));
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

#if MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,("%s: CC1 pulse.", fname));
#endif

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

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: starting buffer count = %d",
		fname, pxd_capturedBuffer( epix_xclib_vinput->unitmap ) ));

	MX_DEBUG(-2,("%s: video fields per frame = %lu",
		fname, pxd_videoFieldsPerFrame() ));

	MX_DEBUG(-2,("%s: video field count = %lu",
		fname, pxd_videoFieldCount( epix_xclib_vinput->unitmap ) ));
#endif

#if ( USE_CLCCSE_REGISTER == FALSE )
	epix_status = pxd_setExsyncPrin( epix_xclib_vinput->unitmap,
						65535, 65535 );

	if ( epix_status != 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, epix_status,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to send an EXSYNC pulse for "
			"video input '%s' failed.  %s",
				vinput->record->name, error_message );
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
	MX_DEBUG(-2,("%s: Started taking a frame using video input '%s'.",
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
	pxbuffer_t last_buffer;
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
	MX_DEBUG(-2,("%s: busy = %d, field count = %lu, last buffer = %d",
		fname, vinput->busy,
		pxd_videoFieldCount( epix_xclib_vinput->unitmap ),
		pxd_capturedBuffer( epix_xclib_vinput->unitmap ) ));
#endif

#if 0
	if ( vinput->busy == FALSE ) {
		char filename[] = "test.tiff";
		int epix_status;

		MX_DEBUG(-2,("%s: saving TIFF file '%s'", fname, filename));

		epix_status = pxd_saveTiff( epix_xclib_vinput->unitmap,
			filename, 1, 0, 0, -1, -1, 0, 0 );

		MX_DEBUG(-2,("%s: saved TIFF file '%s', epix_status = %d",
			fname, filename, epix_status));
	}
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

	char rgb_colorspace[] = "RGB";
	char grey_colorspace[] = "Grey";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	long image_format, bytes_per_image, words_to_read, result;
	long x_framesize, y_framesize;
	char *colorspace;
	char error_message[80];
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

	/* Get the image pixel format. */

	mx_status = mx_video_input_get_image_format( vinput->record,
							&image_format );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: image_format = %ld", fname, image_format));
#endif

	/* Get the dimensions of the image. */

	mx_status = mx_video_input_get_framesize( vinput->record,
						&x_framesize, &y_framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the number of bytes in the image. */

	switch( image_format ) {
	case MXT_IMAGE_FORMAT_RGB:
		bytes_per_image = 3 * x_framesize * y_framesize;
		colorspace = rgb_colorspace;
		break;

	case MXT_IMAGE_FORMAT_GREY8:
		bytes_per_image = x_framesize * y_framesize;
		colorspace = grey_colorspace;
		break;

	case MXT_IMAGE_FORMAT_GREY16:
		bytes_per_image = 2 * x_framesize * y_framesize;
		colorspace = grey_colorspace;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image format %ld for video input '%s'.",
			image_format, vinput->record->name );
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: bytes_per_image = %ld", fname, bytes_per_image));
#endif

	/* At this point, we either reuse an existing MX_IMAGE_FRAME
	 * or create a new one.
	 */

	if ( (*frame) == (MX_IMAGE_FRAME *) NULL ) {

		/* Allocate a new MX_IMAGE_FRAME. */

		*frame = malloc( sizeof(MX_IMAGE_FRAME) );

		if ( (*frame) == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"a new MX_IMAGE_FRAME structure." );
		}

		(*frame)->header_length = 0;
		(*frame)->header_data = NULL;

		(*frame)->image_length = 0;
		(*frame)->image_data = NULL;
	}

	/* Fill in some parameters. */

	(*frame)->image_type = MXT_IMAGE_LOCAL_1D_ARRAY;
	(*frame)->framesize[0] = x_framesize;
	(*frame)->framesize[1] = y_framesize;
	(*frame)->image_format = image_format;

	/* See if the image buffer is already big enough for the image. */

	if ( ( (*frame)->image_data != NULL )
	  && ( (*frame)->image_length >= bytes_per_image ) )
	{
#if MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,
		("%s: The image buffer is already big enough.", fname));
#endif
	} else {
		/* If not, then allocate a new one. */

		if ( (*frame)->image_data != NULL ) {
			free( (*frame)->image_data );
		}

		(*frame)->image_data = malloc( bytes_per_image );

		if ( (*frame)->image_data == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate a %ld byte image buffer for "
			"video input '%s'.",
				bytes_per_image, vinput->record->name );
		}


#if MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,("%s: allocated new frame buffer.", fname));
#endif
	}

#if 1  /* FIXME!!! - This should not be present in the final version. */
	memset( (*frame)->image_data, 0, 50 );
#endif

	(*frame)->image_length = bytes_per_image;

	/* Now read the frame into the MX_IMAGE_FRAME structure. */

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: reading a %lu byte image frame.",
				fname, (*frame)->image_length ));
#endif

	if ( image_format == MXT_IMAGE_FORMAT_GREY16 ) {
		words_to_read = ((*frame)->image_length) / 2;
		
		result = pxd_readushort( epix_xclib_vinput->unitmap, 1,
				0, 0, -1, -1,
				(*frame)->image_data, words_to_read,
				colorspace );

		if ( result >= 0 ) {
			result = result * 2;
		}
	} else {
		result = pxd_readuchar( epix_xclib_vinput->unitmap, 1,
				0, 0, -1, -1,
				(*frame)->image_data, (*frame)->image_length,
				colorspace );
	}

	/* Was the read successful? */

	if ( result < 0 ) {
		mxi_epix_xclib_error_message( epix_xclib_vinput->unitmap,
			result, error_message, sizeof(error_message) ); 

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"An error occurred while reading a %lu byte image frame "
		"from video input '%s'.  Error = '%s'.",
			(*frame)->image_length, vinput->record->name,
			error_message );
	} else
	if ( result < (*frame)->image_length ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"Read only %ld bytes from video input '%s' when we were "
		"expecting to read %lu bytes.",
			result, vinput->record->name,
			(*frame)->image_length );
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,
	("%s: successfully read a %lu byte image frame from video input '%s'.",
		fname, (*frame)->image_length, vinput->record->name ));
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
	int bits_per_component, components_per_pixel;
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
	case MXLV_VIN_PIXEL_ORDER:
	case MXLV_VIN_TRIGGER_MODE:
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETERS:

		break;

	case MXLV_VIN_FORMAT:
		bits_per_component = pxd_imageBdim();
		components_per_pixel = pxd_imageCdim();

		if ( bits_per_component <= 8 ) {
			switch( components_per_pixel ) {
			case 1:
			    vinput->image_format = MXT_IMAGE_FORMAT_GREY8;
			    break;
			case 3:
			    vinput->image_format = MXT_IMAGE_FORMAT_RGB;
			    break;
			default:
			    return mx_error( MXE_UNSUPPORTED, fname,
				"%d-bit video input '%s' reports an "
				"unsupported number of pixel components (%d).",
					bits_per_component,
					vinput->record->name,
					components_per_pixel );
			}
		} else
		if ( bits_per_component <= 16 ) {
			switch( components_per_pixel ) {
			case 1:
			    vinput->image_format = MXT_IMAGE_FORMAT_GREY16;
			    break;
			default:
			    return mx_error( MXE_UNSUPPORTED, fname,
				"%d-bit video input '%s' reports an "
				"unsupported number of pixel components (%d).",
					bits_per_component,
					vinput->record->name,
					components_per_pixel );
			}
		} else {
			return mx_error( MXE_UNSUPPORTED, fname,
				"Video input '%s' reports an unsupported "
				"number of bits per component (%d) "
				"and components per pixel (%d).",
					vinput->record->name,
					bits_per_component,
					components_per_pixel );
		}
#if MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,(
		"%s: bits per component = %d, components per pixel = %d",
			fname, bits_per_component, components_per_pixel));

		MX_DEBUG(-2,("%s: video format = %ld", fname, vinput->image_format));
#endif
		break;

	case MXLV_VIN_FRAMESIZE:
		vinput->framesize[0] = pxd_imageXdim();
		vinput->framesize[1] = pxd_imageYdim();
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
	int epix_status;
	mx_status_type mx_status;

	struct xclibs *xc;
	xclib_DeclareVidStateStructs(vidstate);

	mx_status = mxd_epix_xclib_get_pointers( vinput,
						&epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETERS:

		break;

	case MXLV_VIN_FORMAT:
		(void) mxd_epix_xclib_get_parameter( vinput );

		return mx_error( MXE_UNSUPPORTED, fname,
		"Changing the video format for video input '%s' via "
		"MX is not supported.  In order to change video formats, "
		"you must create a new video configuration in the XCAP "
		"program from EPIX, Inc.", vinput->record->name );
		break;

	case MXLV_VIN_PIXEL_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the pixel order for video input '%s' "
			"is not supported.", vinput->record->name );
		break;

	case MXLV_VIN_FRAMESIZE:

		/* Escape to the Structured Style Interface. */

		xc = pxd_xclibEscape(0, 0, 0);

		if ( xc == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The XCLIB library has not yet been initialized with "
		"pxd_PIXCIopen() for video input '%s'.", vinput->record->name );
		}

		xclib_InitVidStateStructs(vidstate);

		xc->pxlib.getState( &(xc->pxlib), 0, PXMODE_DIGI, &vidstate );

		/* Change the necessary parameters. */

		vidstate.vidres->x.datsamples = vinput->framesize[0];
		vidstate.vidres->x.vidsamples = vinput->framesize[0];
		vidstate.vidres->x.setmidvidoffset = 0;
		vidstate.vidres->x.vidoffset = 0;
		vidstate.vidres->x.setmaxdatsamples = 0;
		vidstate.vidres->x.setmaxvidsamples = 0;

		vidstate.vidres->y.datsamples = vinput->framesize[1];
		vidstate.vidres->y.vidsamples = vinput->framesize[1];
		vidstate.vidres->y.setmidvidoffset = 0;
		vidstate.vidres->y.vidoffset = 0;
		vidstate.vidres->y.setmaxdatsamples = 0;
		vidstate.vidres->y.setmaxvidsamples = 0;

		vidstate.vidres->datfields = 1;
		vidstate.vidres->setmaxdatfields = 0;

		/* Leave the Structured Style Interface. */

		epix_status = xc->pxlib.defineState(
				&(xc->pxlib), 0, PXMODE_DIGI, &vidstate );

		if ( epix_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Error in xc->pxlib.defineState() for record '%s'.  "
			"Error code = %d",
				vinput->record->name, epix_status );
		}

		epix_status = pxd_xclibEscaped(0, 0, 0);

		if ( epix_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Error in pxd_xclibEscaped() for record '%s'.  "
			"Error code = %d",
				vinput->record->name, epix_status );
		}

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

#endif /* OS_LINUX && HAVE_EPIX_XCLIB */


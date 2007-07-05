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
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EPIX_XCLIB_DEBUG		TRUE

#define MXD_EPIX_XCLIB_DEBUG_IMAGE_TIME	TRUE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPIX_XCLIB

#include <stdlib.h>

#if defined(OS_WIN32)
#  include <windows.h>
#else
#  include <sys/times.h>
#endif

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_hrt.h"
#include "mx_image.h"
#include "mx_camera_link.h"
#include "mx_video_input.h"

#include "xcliball.h"	/* Vendor include file */

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
	NULL,
	NULL,
	mxd_epix_xclib_resynchronize
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_epix_xclib_video_input_function_list = {
	mxd_epix_xclib_arm,
	mxd_epix_xclib_trigger,
	mxd_epix_xclib_stop,
	mxd_epix_xclib_abort,
	mxd_epix_xclib_continuous_capture,
	mxd_epix_xclib_get_last_frame_number,
	mxd_epix_xclib_get_total_num_frames,
	mxd_epix_xclib_get_status,
	mxd_epix_xclib_get_extended_status,
	mxd_epix_xclib_get_frame,
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
			MX_EPIX_XCLIB_VIDEO_INPUT **epix_xclib_vinput,
			MX_EPIX_XCLIB **epix_xclib,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epix_xclib_get_pointers()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput_ptr;
	MX_RECORD *xclib_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	epix_xclib_vinput_ptr = vinput->record->record_type_struct;

	if ( epix_xclib_vinput_ptr == (MX_EPIX_XCLIB_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_EPIX_XCLIB_VIDEO_INPUT pointer for record '%s' is NULL.",
			vinput->record->name );
	}

	if ( epix_xclib_vinput != (MX_EPIX_XCLIB_VIDEO_INPUT **) NULL ) {
		*epix_xclib_vinput = epix_xclib_vinput_ptr;
	}

	if ( epix_xclib != (MX_EPIX_XCLIB **) NULL ) {
		xclib_record = epix_xclib_vinput_ptr->xclib_record;

		if ( xclib_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The xclib_record pointer for record '%s' is NULL.",
				vinput->record->name );
		}

		*epix_xclib = xclib_record->record_type_struct;

		if ( (*epix_xclib) == (MX_EPIX_XCLIB *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_EPIX_XCLIB pointer for record '%s' is NULL.",
				vinput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_epix_xclib_set_exsync_princ( MX_VIDEO_INPUT *vinput,
				MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput,
				double trigger_time,
				double frame_time,
				mx_bool_type continuous_select,
				mx_bool_type trigger_input_select )
{
	static const char fname[] = "mxd_epix_xclib_set_exsync_princ()";

	unsigned int exsync_mode, princ_mode, mask;
	unsigned int epcd, trigneg, exps, cnts, tis;
	unsigned int pixel_clock_bitmap;
	int epix_status;
	unsigned long flags;
	char error_message[80];

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
			fname, vinput->record->name));

#if 0
	{
		/* Show the EXSYNC and PRIN values and also the resulting
		 * camera timing.
		 */

		int exsync_value, prin_value;
		double timing_scale, epix_exposure_time, epix_frame_time;

		exsync_value = pxd_getExsync( epix_xclib_vinput->unitmap );
		prin_value   = pxd_getPrin( epix_xclib_vinput->unitmap );

	MX_DEBUG(-2,("%s: EXSYNC value = %#x", fname, exsync_value));
	MX_DEBUG(-2,("%s: PRIN value = %#x", fname, prin_value));
	MX_DEBUG(-2,("%s: pixel clock divisor = %lu",
		fname, epix_xclib_vinput->pixel_clock_divisor));

		timing_scale = mx_divide_safely(
				epix_xclib_vinput->pixel_clock_divisor,
				vinput->pixel_clock_frequency );

		epix_exposure_time = timing_scale * (double) exsync_value;

		epix_frame_time = epix_exposure_time + 2.0e-6
					+ timing_scale * (double) prin_value;

		MX_DEBUG(-2,
		("%s: EPIX exposure time = %g sec, EPIX frame time = %g sec",
			fname, epix_exposure_time, epix_frame_time));
	}
#endif

	MX_DEBUG(-2,("%s: trigger_time = %g, frame_time = %g",
			fname, trigger_time, frame_time));
	MX_DEBUG(-2,("%s: continuous_select = %d, trigger_input_select = %d",
			fname, (int) continuous_select,
			(int) trigger_input_select));
#endif

	/* Read the current values of the EXSYNC and PRINC registers. */

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

	switch( epix_xclib_vinput->pixel_clock_divisor ) {

	case 512: pixel_clock_bitmap = 0x7; break;
	case 256: pixel_clock_bitmap = 0x6; break;
	case 128: pixel_clock_bitmap = 0x5; break;
	case  64: pixel_clock_bitmap = 0x4; break;
	case  32: pixel_clock_bitmap = 0x3; break;
	case  16: pixel_clock_bitmap = 0x2; break;
	case   8: pixel_clock_bitmap = 0x1; break;
	case   4: pixel_clock_bitmap = 0x0; break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	    "Unsupported pixel clock divisor %lu for EPIX imaging board '%s'.",
			epix_xclib_vinput->pixel_clock_divisor,
			vinput->record->name );
	}

	epcd = pixel_clock_bitmap << 7;

	princ_mode |= epcd;

	/* Bit 4, TRIGNEG, Trigger Input Polarity Select. */

	switch( vinput->external_trigger_polarity ) {
	case MXF_VIN_TRIGGER_RISING:
		trigneg = 0x0 << 4;  /* Trigger on rising edge. */
		break;
	case MXF_VIN_TRIGGER_FALLING:
		trigneg = 0x1 << 4;  /* Trigger on falling edge. */
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"External trigger mode %ld is not supported for "
		"EPIX video input '%s'.",
			vinput->external_trigger_polarity,
			vinput->record->name );
	}

	princ_mode |= trigneg;

	/* Bit 2, EXPS, EXSYNC Polarity Select. */

	switch( vinput->camera_trigger_polarity ) {
	case MXF_VIN_TRIGGER_HIGH:
		exps = 0x1 << 2;     /* Positive CC1 pulse. */
		break;
	case MXF_VIN_TRIGGER_LOW:
		exps = 0x0 << 2;     /* Negative CC1 pulse. */
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Camera trigger mode %ld is not supported for "
		"EPIX video input '%s'.",
			vinput->camera_trigger_polarity,
			vinput->record->name );
	}

	princ_mode |= exps;

	/* Bit 1, CNTS, Continuous Select. */

	cnts = (continuous_select & 0x1) << 1;

	princ_mode |= cnts;

	/* Bit 0, TIS, Trigger Input Select. */

	tis = (trigger_input_select & 0x1);

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
		"pxd_setExsyncPrincMode( %#lx, %#x, %#x ) failed.  %s",
			epix_xclib_vinput->unitmap, exsync_mode, princ_mode,
			error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_epix_xclib_set_ready_status( MX_VIDEO_INPUT *vinput,
				MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput,
				mx_bool_type ready_for_trigger )
{
	static const char fname[] = "mxd_epix_xclib_set_ready_status()";

	char error_message[80];
	int output_value, epix_status;

#if 1	/* Not using OUT1 as a ready bit anymore. */

	return MX_SUCCESSFUL_RESULT;
#endif

	/* General Purpose Output 1 is used to indicate the imaging board
	 * is ready to be triggered.
	 */

	output_value = pxd_getGPOut( epix_xclib_vinput->unitmap, 0 );

	if ( output_value < 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, output_value,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to read the status of the General "
			"Purpose Outputs for video input '%s' failed.  %s",
				vinput->record->name, error_message );
	}

	if ( ready_for_trigger ) {
		output_value |= 0x1;
	} else {
		output_value &= ~0x1;
	}

	epix_status = pxd_setGPOut( epix_xclib_vinput->unitmap, output_value );

	if ( epix_status != 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, epix_status,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to set the value of the General "
			"Purpose Output 1 for video input '%s' failed.  %s",
				vinput->record->name, error_message );
	}

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

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	epix_xclib_vinput->record = record;

	vinput->bytes_per_frame = 0;
	vinput->bytes_per_pixel = 0;
	vinput->bits_per_pixel = 0;
	vinput->trigger_mode = 0;

	vinput->pixel_clock_frequency = 0.0;	/* in pixels/second */

	vinput->external_trigger_polarity = MXF_VIN_TRIGGER_NONE;

	vinput->camera_trigger_polarity = MXF_VIN_TRIGGER_NONE;

	epix_xclib_vinput->default_trigger_time = 0.01;	/* in seconds */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_finish_record_initialization( MX_RECORD *record )
{
	return mx_video_input_finish_record_initialization( record );
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
					&epix_xclib_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	epix_xclib_vinput->unitmap = 1 << (epix_xclib_vinput->unit_number - 1);

	epix_xclib_vinput->pixel_clock_divisor = 512;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: board model = %#x",
		fname, pxd_infoModel( epix_xclib_vinput->unitmap ) ));

	MX_DEBUG(-2,("%s: board submodel = %#x",
		fname, pxd_infoSubmodel( epix_xclib_vinput->unitmap ) ));

	MX_DEBUG(-2,("%s: board memory = %lu bytes",
		fname, pxd_infoMemsize( epix_xclib_vinput->unitmap ) ));
#endif
	/* Initialize a bunch of driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = mx_native_byteorder();
	vinput->trigger_mode   = MXT_IMAGE_NO_TRIGGER;

	vinput->maximum_frame_number = pxd_imageZdim() - 1;

	epix_xclib_vinput->num_write_test_array_bytes = 0;
	epix_xclib_vinput->write_test_array = NULL;

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

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_epix_xclib_resynchronize()";

	MX_VIDEO_INPUT *vinput;
	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'", fname, record->name ));
#endif

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
					&epix_xclib_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_resynchronize_record( epix_xclib_vinput->xclib_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_arm()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	MX_SEQUENCE_PARAMETERS *sp;
	pxbuffer_t startbuf, endbuf, numbuf;
	double trigger_time, frame_time;
	mx_bool_type continuous_select;
	char error_message[80];
	int epix_status;
	unsigned long flags, old_array_bytes, new_array_bytes;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
					&epix_xclib_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	flags = epix_xclib_vinput->epix_xclib_vinput_flags;

	/* If the 'write test' feature is enabled, fill
	 * the first frame buffer with a test value.
	 */

#if 0
	MX_DEBUG(-2,("%s: epix_xclib_vinput_flags = %#lx", fname, flags));
#endif

	if ( flags & MXF_EPIX_WRITE_TEST) {

		old_array_bytes = epix_xclib_vinput->num_write_test_array_bytes;

		new_array_bytes = vinput->framesize[0] * vinput->framesize[1]
						* sizeof(uint16_t);

		if ( old_array_bytes == 0 ) {
			if ( new_array_bytes == 0 ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
					"The length requested by record '%s' "
					"for the write test array is 0.",
					vinput->record->name );
			} else {
				epix_xclib_vinput->write_test_array
					= malloc( new_array_bytes );

				epix_xclib_vinput->num_write_test_array_bytes
					= new_array_bytes;
			}
		} else {
			if ( new_array_bytes > old_array_bytes ) {
				epix_xclib_vinput->write_test_array = realloc(
					epix_xclib_vinput->write_test_array,
					new_array_bytes );

				epix_xclib_vinput->num_write_test_array_bytes
					= new_array_bytes;
			}
		}

		if ( epix_xclib_vinput->write_test_array == NULL ) {
			epix_xclib_vinput->num_write_test_array_bytes = 0;

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"The write_test_array pointer for record '%s' is NULL.",
				vinput->record->name );
		}

		memset( epix_xclib_vinput->write_test_array,
			epix_xclib_vinput->write_test_value,
			epix_xclib_vinput->num_write_test_array_bytes );

#if 1
		MX_DEBUG(-2,("%s: Writing a %lu byte array set to "
			"the value %#x for record '%s'.",
			fname, epix_xclib_vinput->num_write_test_array_bytes,
			epix_xclib_vinput->write_test_array[0],
			vinput->record->name ));
#endif
		epix_status = pxd_writeushort( epix_xclib_vinput->unitmap, 1,
					0, 0, -1, -1,
					epix_xclib_vinput->write_test_array,
			    epix_xclib_vinput->num_write_test_array_bytes / 2L,
					"Grey" );
#if 1
		MX_DEBUG(-2,("%s: pxd_writeushort() epix_status = %d",
			fname, epix_status));
#endif

		if ( epix_status < 0 ) {
			mxi_epix_xclib_error_message(
				epix_xclib_vinput->unitmap, epix_status,
				error_message, sizeof(error_message) ); 

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"An error occurred while writing a %lu byte "
			"image frame to video input '%s'.  Error = '%s'.",
		(unsigned long) epix_xclib_vinput->num_write_test_array_bytes,
			vinput->record->name, error_message );
		}
	}

	/* If external triggering is not enabled,
	 * return without doing anything further.
	 */

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: vinput->trigger_mode = %#lx",
		fname, vinput->trigger_mode));
#endif

	if ( ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) == 0 ) {

#if MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,
		("%s: external trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, external triggering has been selected. */

	sp = &(vinput->sequence_parameters);

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: External triggering selected.", fname));

	MX_DEBUG(-2,("%s: sp = %p", fname, sp));
	MX_DEBUG(-2,("%s: sp->sequence_type = %ld", fname, sp->sequence_type));
#endif

	/* Compute all of the parameters needed by the sequence.  One-shot,
	 * strobe, sub-image, and streak camera sequences are supported
	 * with external triggers.
	 */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		startbuf = 1;
		endbuf = 1;
		numbuf = 1;

		frame_time = sp->parameter_array[0];

		trigger_time = epix_xclib_vinput->default_trigger_time;
		break;

	case MXT_SQ_STROBE:
		startbuf = 1;
		endbuf = mx_round( sp->parameter_array[0] );
		numbuf = endbuf;

		frame_time = sp->parameter_array[1];

		trigger_time = epix_xclib_vinput->default_trigger_time;
		break;

	case MXT_SQ_STREAK_CAMERA:
		startbuf = 1;
		endbuf = 1;
		numbuf = 1;

		frame_time = sp->parameter_array[0] * sp->parameter_array[1];

		trigger_time = epix_xclib_vinput->default_trigger_time;
		break;

	case MXT_SQ_SUBIMAGE:
		startbuf = 1;
		endbuf = 1;
		numbuf = 1;

		frame_time = sp->parameter_array[1] * sp->parameter_array[3];

		trigger_time = epix_xclib_vinput->default_trigger_time;
		break;


	case MXT_SQ_CONTINUOUS:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Continuous sequences cannot be used with external "
			"triggers for video input '%s'.",
				vinput->record->name );
		break;

	case MXT_SQ_MULTIFRAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Multiframe sequences cannot be used with external "
			"triggers for video input '%s'.",
				vinput->record->name );
		break;

	case MXT_SQ_CIRCULAR_MULTIFRAME:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Circular multiframe sequences cannot be used with external "
			"triggers for video input '%s'.",
				vinput->record->name );
		break;

	case MXT_SQ_BULB:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Bulb sequences are not supported by video input '%s'.",
			vinput->record->name );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"video input '%s'.",
			sp->sequence_type, vinput->record->name );
	}

	/* Configure the EXSYNC and PRINC registers with trigger input
	 * select (TIS) set to TRUE for all sequence types.  Continuous
	 * select (CNTS) is set to FALSE for one-shot mode and to TRUE
	 * for all other modes.
	 */

#if 0
	if ( sp->sequence_type == MXT_SQ_ONE_SHOT ) {
		continuous_select = FALSE;   /* _Not_ supported by the HW! */
	} else {
		continuous_select = TRUE;
	}
#else
	continuous_select = TRUE;
#endif

	mx_status = mxd_epix_xclib_set_exsync_princ( vinput, epix_xclib_vinput,
							trigger_time,
							frame_time,
							continuous_select,
							TRUE );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable the external trigger on General Purpose Trigger 1. */

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: startbuf = %ld, endbuf = %ld, numbuf = %ld",
		fname, startbuf, endbuf, numbuf));
#endif

	epix_status = pxd_goLiveSeqTrig( epix_xclib_vinput->unitmap,
		startbuf, endbuf, 1, numbuf, 1,
		0, 0,                   /* reserved */
		0x100, 3, 0,    /* Start GP trigger 1, rising edge, no delay */
		0, 0, 0, 0, 0, 0, 0,    /* reserved */
		0, 0, 0,                /* No end trigger */
		0, 0, 0, 0, 0, 0 );     /* yet more reserved */

	if ( epix_status != 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, epix_status,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to enable the external trigger for "
		"video input '%s' failed.  %s",
			vinput->record->name, error_message );
	}

	mx_status = mxd_epix_xclib_set_ready_status( vinput,
						epix_xclib_vinput, TRUE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_trigger()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	MX_EPIX_XCLIB *epix_xclib;
	MX_SEQUENCE_PARAMETERS *sp;
	pxbuffer_t startbuf, endbuf, numbuf;
	double trigger_time, exposure_time;
	mx_bool_type continuous_select;
	char error_message[80];
	int epix_status;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
				&epix_xclib_vinput, &epix_xclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif
	flags = epix_xclib->epix_xclib_flags;

	if ( ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {

		/* If internal triggering is not enabled,
		 * return without doing anything
		 */

#if MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,
		("%s: internal trigger disabled for video input '%s'",
			fname, vinput->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* FIXME - If the video input is already acquiring data,
	 * should we return without doing anything here?  This 
	 * might happen if we enabled both external and internal
	 * triggering.
	 */

	/*---*/

	sp = &(vinput->sequence_parameters);

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: Internal triggering selected.", fname));

	MX_DEBUG(-2,("%s: sp = %p", fname, sp));
	MX_DEBUG(-2,("%s: sp->sequence_type = %ld", fname, sp->sequence_type));
#endif

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: starting buffer count = %d", fname,
		(int) pxd_capturedBuffer( epix_xclib_vinput->unitmap ) ));

	MX_DEBUG(-2,("%s: video fields per frame = %d",
		fname, pxd_videoFieldsPerFrame() ));

	MX_DEBUG(-2,("%s: video field count = %lu",
		fname, pxd_videoFieldCount( epix_xclib_vinput->unitmap ) ));
#endif

	/* Set the EXSYNC and PRINC register values needed. */

	trigger_time = epix_xclib_vinput->default_trigger_time;

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
		exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
	case MXT_SQ_GEOMETRICAL:
		exposure_time = sp->parameter_array[1];
		break;
	case MXT_SQ_STREAK_CAMERA:
		exposure_time = sp->parameter_array[0] * sp->parameter_array[1];
		break;
	case MXT_SQ_SUBIMAGE:
		exposure_time = sp->parameter_array[1] * sp->parameter_array[3];
		break;
	default:
		exposure_time = -1;
		break;
	}

	if ( exposure_time >= 0.0 ) {
		/* Configure the EXSYNC and PRINC registers with trigger
		 * input select (TIS) set to FALSE for all sequence types.
		 * Continuous select (CNTS) is set to FALSE for one-shot
		 * mode and to TRUE for all other modes.
		 */

		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_STREAK_CAMERA:
		case MXT_SQ_SUBIMAGE:
			continuous_select = FALSE;
			break;
		default:
			continuous_select = TRUE;
			break;
		}

		mx_status = mxd_epix_xclib_set_exsync_princ(
						vinput, epix_xclib_vinput,
							trigger_time,
							exposure_time,
							continuous_select,
							FALSE );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Trigger the start of the sequence. */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_STREAK_CAMERA:
	case MXT_SQ_SUBIMAGE:

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: triggering one-shot, streak camera, "
	    "or subimage mode for vinput '%s'.", fname, vinput->record->name ));
#endif
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

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: triggering continuous mode for vinput '%s'.",
		fname, vinput->record->name ));
#endif
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

	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
	case MXT_SQ_GEOMETRICAL:
		if ( sp->num_parameters < 1 ) {
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"The first sequence parameter of video input '%s' "
			"for a multiframe sequence should be "
			"the number of frames.  "
			"However, the sequence says that it has %ld frames.",
			    vinput->record->name, sp->num_parameters );
				
		}

		startbuf = 1;

		endbuf = mx_round( sp->parameter_array[0] );

		if ( endbuf > pxd_imageZdim() ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of sequence frames (%ld) for "
			"video input '%s' is larger than the maximum value "
			"of %d.",
			    endbuf, vinput->record->name, pxd_imageZdim());
		}

		if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
			numbuf = endbuf;
		} else {
			numbuf = 0;
		}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: triggering multiframe sequence for vinput '%s'.",
		fname, vinput->record->name ));
	MX_DEBUG(-2,("%s: startbuf = %ld, endbuf = %ld, numbuf = %ld",
		fname, startbuf, endbuf, numbuf));
#endif
		epix_status = pxd_goLiveSeq( epix_xclib_vinput->unitmap,
					startbuf, endbuf, 1, numbuf, 1 );

		if ( epix_status != 0 ) {
			mxi_epix_xclib_error_message(
				epix_xclib_vinput->unitmap, epix_status,
				error_message, sizeof(error_message) );

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to start multiframe "
			"acquisition with video input '%s' failed.  %s",
				vinput->record->name, error_message );
		}
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

	mx_status = mxd_epix_xclib_set_ready_status( vinput,
						epix_xclib_vinput, TRUE );
#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: Started taking frame(s) using video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
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
					&epix_xclib_vinput, NULL, fname );

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
					&epix_xclib_vinput, NULL, fname );

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
mxd_epix_xclib_continuous_capture( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_continuous_capture()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	char error_message[80];
	long num_frame_buffers;
	int epix_status;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
					&epix_xclib_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	num_frame_buffers = pxd_imageZdim();

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: num_frame_buffers = %ld", fname, num_frame_buffers));
#endif
	vinput->maximum_frame_number = num_frame_buffers - 1;

	if ( vinput->continuous_capture > num_frame_buffers ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested number (%ld) of continuous capture frames "
		"exceeds the maximum allowed value (%ld) for video input '%s'.",
			vinput->continuous_capture,
			num_frame_buffers,
			vinput->record->name );
	}

	/* Configure the capture sequence to continue using the requested
	 * number of frames until explicitly stopped.  
	 */

#if 1
	epix_status = pxd_goLiveSeq( epix_xclib_vinput->unitmap,
			1, num_frame_buffers, 1, 0, 1 );
#else
	epix_status = pxd_goLiveSeq( epix_xclib_vinput->unitmap,
			1, num_frame_buffers, 1, 10, 1 );
#endif

	if ( epix_status != 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, epix_status,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to start continuous capture for "
			"video input '%s' failed.  %s",
				vinput->record->name, error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_get_last_frame_number( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_get_last_frame_number()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	long captured_buffer;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
					&epix_xclib_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	captured_buffer = pxd_capturedBuffer( epix_xclib_vinput->unitmap );

#if 1 && MXD_EPIX_XCLIB_DEBUG	/* WML */

	MX_DEBUG(-2,("%s: pxd_capturedBuffer() = %ld",
		fname, vinput->last_frame_number ));
#endif
	vinput->last_frame_number = captured_buffer - 1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_get_total_num_frames( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_get_total_num_frames()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
					&epix_xclib_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->total_num_frames =
		pxd_capturedFieldCount( epix_xclib_vinput->unitmap );

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: pxd_capturedFieldCount() = %ld",
		fname, vinput->total_num_frames ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_get_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_get_status()";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	int busy;
	pxbuffer_t last_buffer;
	int epix_status;
	char error_message[1024];	/* 1024 is recommended by the manual. */
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
					&epix_xclib_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	epix_status = pxd_mesgFaultText( epix_xclib_vinput->unitmap,
				error_message, sizeof(error_message) );
#else
	epix_status = pxd_mesgFault( epix_xclib_vinput->unitmap );
#endif

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: epix_status = %d", fname, epix_status));
#endif

	if ( epix_status != 0 ) {
#if MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,("%s: EPIX error message = '%s'",
			fname, error_message));
#endif
	}

	busy = pxd_goneLive( epix_xclib_vinput->unitmap, 0 );

	if ( busy ) {
		vinput->busy = TRUE;

		vinput->status |= MXSF_VIN_IS_BUSY;
	} else {
		vinput->busy = FALSE;

		vinput->status &= ~MXSF_VIN_IS_BUSY;
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,
	("%s: pxd_goneLive() = %d, pxd_videoFieldCount() = %lu, "
	"pxd_capturedBuffer() = %d",
		fname, vinput->busy,
		pxd_videoFieldCount( epix_xclib_vinput->unitmap ),
		(int) pxd_capturedBuffer( epix_xclib_vinput->unitmap ) ));
#endif

#if 0
	if ( vinput->busy == FALSE ) {
		mx_status = mxd_epix_xclib_set_ready_status( vinput,
							epix_xclib_vinput,
							FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_get_extended_status()";

	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_get_status( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_epix_xclib_get_last_frame_number( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_epix_xclib_get_total_num_frames( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_epix_xclib_get_frame()";

	char rgb_colorspace[] = "RGB";
	char grey_colorspace[] = "Grey";

	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	MX_EPIX_XCLIB *epix_xclib;
	MX_IMAGE_FRAME *frame;
	pxbuffer_t epix_frame_number;
	long words_to_read, result;
	unsigned long flags;
	char *colorspace;
	char error_message[80];
	mx_status_type mx_status;

	uint16_t *image_data16;
	long i, num_image_words;

	mx_status = mxd_epix_xclib_get_pointers( vinput,
				&epix_xclib_vinput, &epix_xclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	/* Get the colorspace string for this frame. */

	switch( vinput->image_format ) {
	case MXT_IMAGE_FORMAT_RGB:
		colorspace = rgb_colorspace;
		break;

	case MXT_IMAGE_FORMAT_GREY8:
		colorspace = grey_colorspace;
		break;

	case MXT_IMAGE_FORMAT_GREY16:
		colorspace = grey_colorspace;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image format %ld for video input '%s'.",
			vinput->image_format, vinput->record->name );
	}

	/* Which frame are we trying to read? */

	if ( vinput->frame_number >= pxd_imageZdim() ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested frame number %ld for record '%s' is larger "
		"than the maximum allowed frame number of %d.",
			vinput->frame_number, vinput->record->name,
			pxd_imageZdim() - 1 );
	} else
	if ( vinput->frame_number >= 0 ) {
		epix_frame_number = vinput->frame_number + 1;
	} else
	if ( vinput->frame_number == (-1) ) {
		/* Frame number -1 means return the most recently
		 * acquired frame.
		 */

		epix_frame_number =
			pxd_capturedBuffer( epix_xclib_vinput->unitmap );

		vinput->frame_number = epix_frame_number - 1;
	} else {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Frame number %ld for record '%s' is an illegal frame number.",
			vinput->frame_number, vinput->record->name );
	}

	/* Read the frame into the MX_IMAGE_FRAME structure. */

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,
	("%s: reading a %lu byte image frame from EPIX frame number %ld.",
			fname, (unsigned long) frame->image_length,
			(long) epix_frame_number ));
#endif

	if ( vinput->image_format == MXT_IMAGE_FORMAT_GREY16 ) {
		words_to_read = (frame->image_length) / 2;
		
		result = pxd_readushort( epix_xclib_vinput->unitmap,
				epix_frame_number,
				0, 0, -1, -1,
				frame->image_data, words_to_read,
				colorspace );

		if ( result >= 0 ) {
			result = result * 2;
		}
	} else {
		result = pxd_readuchar( epix_xclib_vinput->unitmap,
				epix_frame_number,
				0, 0, -1, -1,
				frame->image_data, frame->image_length,
				colorspace );
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: result = %ld", fname, result));
#endif
	/* Was the read successful? */

	if ( result < 0 ) {
		mxi_epix_xclib_error_message( epix_xclib_vinput->unitmap,
			result, error_message, sizeof(error_message) ); 

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"An error occurred while reading a %lu byte image frame "
		"from video input '%s'.  Error = '%s'.",
			(unsigned long) frame->image_length,
			vinput->record->name, error_message );
	} else
	if ( result < frame->image_length ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"Read only %ld bytes from video input '%s' when we were "
		"expecting to read %lu bytes.",
			result, vinput->record->name,
			(unsigned long) frame->image_length );
	}

	/* Get the timestamp for the frame. */

	frame->image_time = mxi_epix_xclib_get_buffer_timespec( epix_xclib,
						epix_xclib_vinput->unitmap,
						epix_frame_number );

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: EPIX image timespec = (%lu,%ld)",
		fname, frame->image_time.tv_sec, frame->image_time.tv_nsec));
#endif

#if MXD_EPIX_XCLIB_DEBUG_IMAGE_TIME
	{
		char buffer[80];

		mx_os_time_string( frame->image_time,
				buffer, sizeof(buffer) );

		MX_DEBUG(-2,(" "));
		MX_DEBUG(-2,("%s: Image time = '%s'", fname, buffer));
		MX_DEBUG(-2,("%s: OS time    = '%s'", fname,
			mx_current_time_string(buffer, sizeof(buffer)) ));
	}
#endif

	/* If requested, byteswap the image. */

	flags = epix_xclib->epix_xclib_flags;

	if ( flags & MXF_EPIX_BYTESWAP ) {

		image_data16 = frame->image_data;

		num_image_words = frame->image_length / 2L;

#if MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,("%s: byteswapping %ld word image.",
			fname, num_image_words));
#endif
		for ( i = 0; i < num_image_words; i++ ) {
			image_data16[i] = mx_16bit_byteswap( image_data16[i] );
		}
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,
	("%s: successfully read a %lu byte image frame from video input '%s'.",
		fname, (unsigned long) frame->image_length,
		vinput->record->name ));
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
					&epix_xclib_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_BYTE_ORDER:
	case MXLV_VIN_TRIGGER_MODE:
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:

		break;

	case MXLV_VIN_BYTES_PER_FRAME:
	case MXLV_VIN_BYTES_PER_PIXEL:
	case MXLV_VIN_BITS_PER_PIXEL:
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

		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel )
				* vinput->framesize[0] * vinput->framesize[1];
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
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

		mx_status = mx_image_get_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,(
		"%s: bits per component = %d, components per pixel = %d",
			fname, bits_per_component, components_per_pixel));

		MX_DEBUG(-2,("%s: video format = %ld '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
		 	
#endif
		break;

	case MXLV_VIN_MAXIMUM_FRAME_NUMBER:
		vinput->maximum_frame_number = pxd_imageZdim() - 1;
		break;

	case MXLV_VIN_FRAMESIZE:
		vinput->framesize[0] = pxd_imageXdim();
		vinput->framesize[1] = pxd_imageYdim();
		break;
	default:
		return mx_video_input_default_get_parameter_handler( vinput );
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
					&epix_xclib_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPIX_XCLIB_DEBUG
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
		(void) mxd_epix_xclib_get_parameter( vinput );

		return mx_error( MXE_UNSUPPORTED, fname,
		"Changing the video format for video input '%s' via "
		"MX is not supported.  In order to change video formats, "
		"you must create a new video configuration in the XCAP "
		"program from EPIX, Inc.", vinput->record->name );
		break;

	case MXLV_VIN_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the byte order for video input '%s' "
			"is not supported.", vinput->record->name );
		break;

	case MXLV_VIN_FRAMESIZE:

#if 1 || MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,("%s: setting '%s' framesize to (%lu, %lu)",
			fname, vinput->record->name,
			vinput->framesize[0], vinput->framesize[1]));
#endif
		/* Escape to the Structured Style Interface. */

		xc = pxd_xclibEscape(0, 0, 0);

		if ( xc == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"The XCLIB library has not yet been initialized "
			"for video input '%s' with pxd_PIXCIopen().",
				vinput->record->name );
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

		/* Change the state. */

		epix_status = xc->pxlib.defineState(
				&(xc->pxlib), 0, PXMODE_DIGI, &vidstate );

		if ( epix_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Error in xc->pxlib.defineState() for record '%s'.  "
			"Error code = %d",
				vinput->record->name, epix_status );
		}

		/* Leave the Structured Style Interface. */

#if 1    /* WML WML WML */
		epix_status = pxd_xclibEscaped(0, 0, 0);

		if ( epix_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Error in pxd_xclibEscaped() for record '%s'.  "
			"Error code = %d",
				vinput->record->name, epix_status );
		}
#endif   /* WML WML WML */

#if 1 || MXD_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,("%s: finished setting '%s' framesize.",
			fname, vinput->record->name));
#endif
		break;

	case MXLV_VIN_TRIGGER_MODE:
		break;

	default:
		return mx_video_input_default_set_parameter_handler( vinput );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_EPIX_XCLIB */


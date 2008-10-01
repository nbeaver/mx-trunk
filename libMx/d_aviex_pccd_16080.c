/*
 * Name:    d_aviex_pccd_16080.c
 *
 * Purpose: Functions and definitions that are specific to the
 *          Aviex PCCD-16080 detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_AVIEX_PCCD_16080_DEBUG        		TRUE
#define MXD_AVIEX_PCCD_16080_DEBUG_DESCRAMBLING		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "d_aviex_pccd.h"

/*-------------------------------------------------------------------------*/

/* PCCD-16080 data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_aviex_pccd_16080_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_AVIEX_PCCD_STANDARD_FIELDS,
	MXD_AVIEX_PCCD_16080_STANDARD_FIELDS
};

long mxd_aviex_pccd_16080_num_record_fields
		= sizeof( mxd_aviex_pccd_16080_record_field_defaults )
		/ sizeof( mxd_aviex_pccd_16080_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aviex_pccd_16080_rfield_def_ptr
			= &mxd_aviex_pccd_16080_record_field_defaults[0];

/*-------------------------------------------------------------------------*/

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_read_register( struct mx_aviex_pccd *aviex_pccd,
					unsigned long register_address,
					unsigned long *register_value )
{
	static const char fname[] = "mxd_aviex_pccd_16080_read_register()";

	MX_AVIEX_PCCD_REGISTER *reg;
	unsigned long high_byte, low_byte;
	mx_status_type mx_status;

	reg = &(aviex_pccd->register_array[register_address]);

	if ( reg->size == 1 ) {
		mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
					register_address, register_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else
	if ( reg->size == 2 ) {
		mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
					register_address, &low_byte );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
					register_address+1, &high_byte );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*register_value = (high_byte << 8) | low_byte;

		return MX_SUCCESSFUL_RESULT;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported register size %d for register %lu "
		"of Aviex PCCD-16080 detector '%s'.",
			reg->size, register_address,
			aviex_pccd->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_write_register( struct mx_aviex_pccd *aviex_pccd,
					unsigned long register_address,
					unsigned long register_value )
{
	static const char fname[] = "mxd_aviex_pccd_16080_write_register()";

	MX_AVIEX_PCCD_REGISTER *reg;
	unsigned long high_byte, low_byte;
	mx_status_type mx_status;

	reg = &(aviex_pccd->register_array[register_address]);

	if ( reg->size == 1 ) {
		mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
					register_address, register_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else
	if ( reg->size == 2 ) {

		high_byte = (register_value >> 8) & 0xff;

		low_byte  = register_value & 0xff;

		mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
					register_address, low_byte );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
					register_address+1, high_byte );

		return mx_status;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported register size %d for register %lu "
		"of Aviex PCCD-16080 detector '%s'.",
			reg->size, register_address,
			aviex_pccd->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#define INIT_REGISTER( i, s, v, r, t, n, x ) \
	do {                                                             \
		mx_status = mxd_aviex_pccd_init_register(                \
			aviex_pccd, (i), (s), (v), (r), (t), (n), (x) ); \
	                                                                 \
		if ( mx_status.code != MXE_SUCCESS )                     \
			return mx_status;                                \
	} while(0)

#define MXD_MAXIMUM_TOTAL_SUBIMAGE_LINES	2048

/*-------------------------------------------------------------------------*/

/* mxd_aviex_pccd_16080_initialize_detector() is invoked by the function
 * mxd_aviex_pccd_open() and performs initialization steps that are specific
 * to the PCCD-16080 detectors.
 */

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_initialize_detector( MX_RECORD *record,
					MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd,
					MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_aviex_pccd_16080_initialize_detector()";

	size_t array_size;
	unsigned long fpga_version;
	unsigned long control_register_value;
	unsigned long pccd_flags;
	mx_status_type mx_status;

	pccd_flags = aviex_pccd->aviex_pccd_flags;

	/* Initialize data structures used to specify attributes
	 * of each detector head register.
	 */

	aviex_pccd->num_registers =
		MXLV_AVIEX_PCCD_16080_DH_YDOFFS - MXLV_AVIEX_PCCD_DH_BASE + 1;

	array_size = aviex_pccd->num_registers * sizeof(MX_AVIEX_PCCD_REGISTER);

	/* Allocate memory for the simulated register array. */

	aviex_pccd->register_array = malloc( array_size );

	if ( aviex_pccd->register_array == NULL ) {
		mx_status = mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of simulated detector head registers for detector '%s.'",
			aviex_pccd->num_registers,
			aviex_pccd->record->name );

		aviex_pccd->num_registers = 0;

		return mx_status;
	}

	memset( aviex_pccd->register_array, 0, array_size );

	/* Initialize register attributes. */

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_FPGA_VERSION,
					1,  0x0, TRUE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					1,  5, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VIDBIN,
					1,  1, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VSHBIN,
					1,  1, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_ROILINES,
					1,  1, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_NOF,
					1,  1, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VREAD,
					2,  1042, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VDARK,
					1,  2, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HLEAD,
					1,  4, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HPIX,
					2,  1042, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_TPRE,
					2,  100, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_SHUTTER,
					2,  10, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_TPOST,
					2,  100, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HDARK,
					1,  6, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HBIN,
					1,  1, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_ROIOFFS,
					2,  0, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_SPARE1,
					1,  0, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_SPARE2,
					1,  0, FALSE, FALSE, 0,  0xff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XAOFFS,
					2,  1790, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XBOFFS,
					2,  1230, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XCOFFS,
					2,  1220, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XDOFFS,
					2,  1630, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YAOFFS,
					2,  1560, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YBOFFS,
					2,  470, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YCOFFS,
					2,  2155, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YDOFFS,
					2,  1840, FALSE, FALSE, 0,  0xffff );

	/* Check to find out the firmware version that is being used by
	 * the detector head.
	 */

	mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
			MXLV_AVIEX_PCCD_16080_DH_FPGA_VERSION,
			&fpga_version );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;

	case MXE_TIMED_OUT:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The detector controller for area detector '%s' is not "
			"responding to commands.  Please verify that the "
			"detector is turned on and configured correctly.",
				record->name );
		break;

	default:
		return mx_status;
		break;
	}

#if MXD_AVIEX_PCCD_16080_DEBUG
	MX_DEBUG(-2,("%s: FPGA version = %lu", fname, fpga_version ));
#endif
	/* The PCCD-16080 camera generates 16 bit per pixel images. */

	vinput->bits_per_pixel = 16;
	ad->bits_per_pixel = vinput->bits_per_pixel;

	/* Perform camera model specific initialization */

	switch( record->mx_type ) {
	case MXT_AD_PCCD_16080:

		/* The PCCD-16080 camera has two CCD chips with a maximum
		 * image size of 4096 by 2048.  Each line from the detector
		 * contains 1024 groups of pixels with 8 pixels per group.
		 * A full frame image has 1024 lines.  This means that the
		 * maximum resolution of the video card should be 8192 by 1024.
		 */

		ad->maximum_framesize[0] = 4096;
		ad->maximum_framesize[1] = 2048;

		aviex_pccd->horiz_descramble_factor = 2;
		aviex_pccd->vert_descramble_factor  = 2;

		/* FIXME - The following is a guess! */

		aviex_pccd->pixel_clock_frequency = 60.0e6;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Record '%s' has an MX type of %ld which "
			"is not supported by this driver.",
				record->name, record->mx_type );
		break;
	}

	/* Read the control register so that we can change it. */

	mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					&control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_16080_DEBUG
	MX_DEBUG(-2,("%s: OLD control_register_value = %#lx",
		fname, control_register_value));
#endif

#if 0
	/* Put the detector head in full frame mode. */

	control_register_value
		&= (~MXF_AVIEX_PCCD_16080_DETECTOR_READOUT_MASK);

	/* Turn on an initial runt Frame Valid pulse.  This is used to
	 * work around a misfeature of the PIXCI E4 board.  The E4 board
	 * always ignores the first frame sent by the camera after 
	 * starting a new sequence.  EPIX says that this is to protect
	 * against a race condition in their system.  Aviex's solution
	 * is to send an extra runt Frame Valid pulse at the start of
	 * a sequence, so that the frame thrown away by EPIX is a frame
	 * that we do not want anyway.
	 */

	control_register_value |= MXF_AVIEX_PCCD_16080_DUMMY_FRAME_VALID;

	/* If requested, turn on the test mode pattern. */

	if ( pccd_flags & MXF_AVIEX_PCCD_USE_TEST_PATTERN ) {
		control_register_value |= MXF_AVIEX_PCCD_16080_TEST_MODE_ON;

		mx_warning( "Area detector '%s' will return a test image "
			"instead of taking a real image.",
				record->name );
	} else {
		control_register_value &= (~MXF_AVIEX_PCCD_16080_TEST_MODE_ON);
	}

	/* Write out the new control register value. */

#if MXD_AVIEX_PCCD_16080_DEBUG
	MX_DEBUG(-2,("%s: NEW control_register_value = %#lx",
		fname, control_register_value));
#endif

	mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					control_register_value );
#endif
	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_descramble_raw_data( uint16_t *raw_frame_data,
					uint16_t ***image_sector_array,
					long i_framesize,
					long j_framesize )
{
	static const char fname[] =
			"mxd_aviex_pccd_16080_descramble_raw_data()";

	mx_warning(
	"%s: Descrambling is not yet implemented for PCCD-16080 detectors.",
		fname );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_descramble_streak_camera( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *raw_frame )
{
	static const char fname[] =
			"mxd_aviex_pccd_16080_descramble_streak_camera()";

	mx_warning("%s: Streak camera descrambling is not yet implemented "
		"for PCCD-16080 detectors.", fname );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/


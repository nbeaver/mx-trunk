/*
 * Name:    d_aviex_pccd_4824.c
 *
 * Purpose: Functions and definitions that are specific to the
 *          Aviex PCCD-4824 detector.
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

#define MXD_AVIEX_PCCD_4824_DEBUG			TRUE
#define MXD_AVIEX_PCCD_4824_DEBUG_DESCRAMBLING_TIMES	TRUE
#define MXD_AVIEX_PCCD_4824_DEBUG_SEQUENCE_TIMES	TRUE

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

#if MXD_AVIEX_PCCD_4824_DEBUG_DESCRAMBLING_TIMES
#  include "mx_hrt_debug.h"
#endif

/*-------------------------------------------------------------------------*/

/* PCCD-4824 data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_aviex_pccd_4824_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_AVIEX_PCCD_STANDARD_FIELDS,
	MXD_AVIEX_PCCD_4824_STANDARD_FIELDS
};

long mxd_aviex_pccd_4824_num_record_fields
		= sizeof( mxd_aviex_pccd_4824_record_field_defaults )
		/ sizeof( mxd_aviex_pccd_4824_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aviex_pccd_4824_rfield_def_ptr
			= &mxd_aviex_pccd_4824_record_field_defaults[0];

/*-------------------------------------------------------------------------*/

#define INIT_REGISTER( i, s, v, r, t, n, x ) \
	do {                                                             \
		mx_status = mxd_aviex_pccd_init_register( aviex_pccd,    \
		    (i), FALSE, (s), (v), (r), FALSE, (t), (n), (x) );   \
	                                                                 \
		if ( mx_status.code != MXE_SUCCESS )                     \
			return mx_status;                                \
	} while(0)

#define MXD_MAXIMUM_TOTAL_SUBIMAGE_LINES	2048

/*-------------------------------------------------------------------------*/

/* mxd_aviex_pccd_4824_initialize_detector() is invoked by the function
 * mxd_aviex_pccd_open() and performs initialization steps that are specific
 * to the PCCD-4824 detector.
 */

MX_EXPORT mx_status_type
mxd_aviex_pccd_4824_initialize_detector( MX_RECORD *record,
					MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd,
					MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
			"mxd_aviex_pccd_4824_initialize_detector()";

	size_t array_size;
	unsigned long controller_fpga_version;
	unsigned long control_register_value;
	unsigned long pccd_flags;
	mx_status_type mx_status;

	pccd_flags = aviex_pccd->aviex_pccd_flags;

	/* Initialize data structures used to specify attributes
	 * of each detector head register.
	 */

	aviex_pccd->num_registers =
	    MXLV_AVIEX_PCCD_4824_DH_OFFSET_Z - MXLV_AVIEX_PCCD_DH_BASE + 1;

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

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_CONTROL,
					2,  0x184, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_OVERSCANNED_PIXELS_PER_LINE,
					2,  4,     FALSE, FALSE, 1,  2048 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_PHYSICAL_LINES_IN_QUADRANT,
					2,  1046,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_PHYSICAL_PIXELS_IN_QUADRANT,
					2,  1050,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_LINES_READ_IN_QUADRANT,
					2,  1024,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_PIXELS_READ_IN_QUADRANT,
					2,  1024,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_INITIAL_DELAY_TIME,
					2,  0,     FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_EXPOSURE_TIME,
					2,  0,     FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_READOUT_DELAY_TIME,
					2,  0,     FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_FRAMES_PER_SEQUENCE,
					2,  1,     FALSE, FALSE, 1,  69999 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_GAP_TIME,
					2,  1,     FALSE, FALSE, 1,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_CONTROLLER_FPGA_VERSION,
					2,  17010, TRUE,  FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_LINE_BINNING,
					2,  1,     FALSE, TRUE,  1,  128 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_PIXEL_BINNING,
					2,  1,     FALSE, TRUE,  1,  128 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_SUBFRAME_SIZE,
					2,  1024,  FALSE, TRUE,  16, 1024 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_SUBIMAGES_PER_READ,
					2,  1,     FALSE, FALSE, 1,  128 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_STREAK_MODE_LINES,
					2,  1,     FALSE, FALSE, 1,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_OFFSET_W,
					2,  0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_OFFSET_X,
					2,  0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_OFFSET_Y,
					2,  0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_4824_DH_OFFSET_Z,
					2,  0,     FALSE, FALSE, 0,  65534 );

	/* Check to find out the firmware versions that are being used by
	 * the detector head.
	 */

	mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
			MXLV_AVIEX_PCCD_4824_DH_CONTROLLER_FPGA_VERSION,
			&controller_fpga_version );

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

#if MXD_AVIEX_PCCD_4824_DEBUG
	MX_DEBUG(-2,("%s: controller FPGA version = %lu",
			fname, controller_fpga_version ));
#endif
	/* The PCCD-4824 camera generates 16 bit per pixel images. */

	vinput->bits_per_pixel = 16;
	ad->bits_per_pixel = vinput->bits_per_pixel;

	/* Perform camera model specific initialization */

	switch( record->mx_type ) {
	case MXT_AD_PCCD_4824:

		/* The PCCD-4824 camera is similar to the PCCD-4824 above,
		 * but it only has a single CCD chip which has a maximum
		 * image size of 3584 by 2048.  Each line from the detector
		 * contains 1792 groups of pixels with 4 pixels per group.
		 * A full frame image has 1024 lines.  This means that the
		 * maximum resolution of the video card should be 7168 by 1024
		 * and that the descramble factors should be 2.
		 */

		ad->maximum_framesize[0] = 3584;
		ad->maximum_framesize[1] = 2048;

		aviex_pccd->horiz_descramble_factor = 2;
		aviex_pccd->vert_descramble_factor  = 2;

		/* For the real detector. */

		aviex_pccd->pixel_clock_frequency = 45.4545e6;

		aviex_pccd->num_ccd_taps = 4;
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
					MXLV_AVIEX_PCCD_4824_DH_CONTROL,
					&control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_4824_DEBUG
	MX_DEBUG(-2,("%s: OLD control_register_value = %#lx",
		fname, control_register_value));
#endif

	/* Put the detector head in full frame mode. */

	control_register_value
		&= (~MXF_AVIEX_PCCD_4824_DETECTOR_READOUT_MASK);

	/* Turn on an initial runt Frame Valid pulse.  This is used to
	 * work around a misfeature of the PIXCI E4 board.  The E4 board
	 * always ignores the first frame sent by the camera after 
	 * starting a new sequence.  EPIX says that this is to protect
	 * against a race condition in their system.  Aviex's solution
	 * is to send an extra runt Frame Valid pulse at the start of
	 * a sequence, so that the frame thrown away by EPIX is a frame
	 * that we do not want anyway.
	 */

	control_register_value |= MXF_AVIEX_PCCD_4824_DUMMY_FRAME_VALID;

	/* If requested, turn on the test mode pattern. */

	if ( pccd_flags & MXF_AVIEX_PCCD_USE_TEST_PATTERN ) {
		control_register_value |= MXF_AVIEX_PCCD_4824_TEST_MODE_ON;

		mx_warning( "Area detector '%s' will return a test image "
			"instead of taking a real image.",
				record->name );
	} else {
		control_register_value &= (~MXF_AVIEX_PCCD_4824_TEST_MODE_ON);
	}

	/* Write out the new control register value. */

#if MXD_AVIEX_PCCD_4824_DEBUG
	MX_DEBUG(-2,("%s: NEW control_register_value = %#lx",
		fname, control_register_value));
#endif

	mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_4824_DH_CONTROL,
					control_register_value );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_4824_get_pseudo_register( MX_AVIEX_PCCD *aviex_pccd,
					long parameter_type,
					unsigned long *pseudo_reg_value )
{
	static const char fname[] =
			"mxd_aviex_pccd_4824_get_pseudo_register()";

	unsigned long control_register;
	mx_status_type mx_status;

	/* For pseudo registers, we must extract the value from
	 * the control register.
	 */

	mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
					MXLV_AVIEX_PCCD_4824_DH_CONTROL,
					&control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( parameter_type ) {
	case MXLV_AVIEX_PCCD_4824_DH_DETECTOR_READOUT_MODE:
		*pseudo_reg_value = (control_register >> 5) & 0x3;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_READOUT_SPEED:
		*pseudo_reg_value = (control_register >> 1) & 0x1;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_TEST_MODE:
		*pseudo_reg_value = control_register & 0x1;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_OFFSET_CORRECTION:
		*pseudo_reg_value = (control_register >> 2) & 0x1;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_EXPOSURE_MODE:
		*pseudo_reg_value = (control_register >> 3) & 0x3;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_LINEARIZATION:
		*pseudo_reg_value = (control_register >> 7) & 0x1;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_DUMMY_FRAME_VALID:
		*pseudo_reg_value = (control_register >> 9) & 0x1;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal pseudoregister %lu requested for area detector '%s'.",
			parameter_type, aviex_pccd->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_4824_set_pseudo_register( MX_AVIEX_PCCD *aviex_pccd,
					long parameter_type,
					unsigned long register_value )
{
	unsigned long control_register, pseudo_reg_value;
	mx_status_type mx_status;

	/* For pseudo registers, we must change specific bits in
	 * the control register.
	 */

	mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
					MXLV_AVIEX_PCCD_4824_DH_CONTROL,
					&control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( parameter_type ) {
	case MXLV_AVIEX_PCCD_4824_DH_DETECTOR_READOUT_MODE:
		pseudo_reg_value = ( register_value & 0x3 ) << 5;

		control_register &= ~0x60;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_READOUT_SPEED:
		pseudo_reg_value = ( register_value & 0x1 ) << 0x1;

		control_register &= ~0x2;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_TEST_MODE:
		pseudo_reg_value = register_value & 0x1;

		control_register &= ~0x1;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_OFFSET_CORRECTION:
		pseudo_reg_value = ( register_value & 0x1 ) << 2;

		control_register &= ~0x4;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_EXPOSURE_MODE:
		pseudo_reg_value = ( register_value & 0x3 ) << 3;

		control_register &= ~0x18;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_LINEARIZATION:
		pseudo_reg_value = ( register_value & 0x1 ) << 7;

		control_register &= ~0x80;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_AVIEX_PCCD_4824_DH_DUMMY_FRAME_VALID:
		pseudo_reg_value = ( register_value & 0x1 ) << 9;

		control_register &= ~0x200;

		control_register |= pseudo_reg_value;
		break;
	}

	mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_4824_DH_CONTROL,
					control_register );
	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_4824_set_external_trigger_mode( MX_AVIEX_PCCD *aviex_pccd,
						mx_bool_type external_trigger )
{
	unsigned long control_register_value;
	mx_status_type mx_status;

	mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
					MXLV_AVIEX_PCCD_4824_DH_CONTROL,
					&control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( external_trigger ) {
		control_register_value
			|= MXF_AVIEX_PCCD_4824_EXTERNAL_TRIGGER;
	} else {
		control_register_value
			&= (~MXF_AVIEX_PCCD_4824_EXTERNAL_TRIGGER);
	}

	mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_4824_DH_CONTROL,
					control_register_value );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_4824_set_binsize( MX_AREA_DETECTOR *ad,
				MX_AVIEX_PCCD *aviex_pccd )
{
	mx_status_type mx_status;

	/* Tell the detector head to change its binsize. */

	mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_PIXEL_BINNING,
				ad->binsize[0] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_LINE_BINNING,
				ad->binsize[1] );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_4824_descramble( uint16_t *raw_frame_data,
					uint16_t ***image_sector_array,
					long i_framesize,
					long j_framesize )
{
#if MXD_AVIEX_PCCD_4824_DEBUG_DESCRAMBLING_TIMES
	static const char fname[] = "mxd_aviex_pccd_4824_descramble()";

	MX_HRT_TIMING measurement;
#endif

	long i, j;

#if MXD_AVIEX_PCCD_4824_DEBUG_DESCRAMBLING_TIMES
	MX_HRT_START( measurement );
#endif

	for ( i = 0; i < i_framesize; i++ ) {
	    for ( j = 0; j < j_framesize; j++ ) {

		image_sector_array[0][i][j] = raw_frame_data[2];

		image_sector_array[1][i][j_framesize-j-1] = raw_frame_data[3];

		image_sector_array[2][i_framesize-i-1][j] = raw_frame_data[1];

		image_sector_array[3][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[0];
		raw_frame_data += 4;
	    }
	}

#if MXD_AVIEX_PCCD_4824_DEBUG_DESCRAMBLING_TIMES
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_4824_linearity_descramble( uint16_t *raw_frame_data,
						uint16_t ***image_sector_array,
						long i_framesize,
						long j_framesize,
						uint16_t *lookup_table )
{
#if MXD_AVIEX_PCCD_4824_DEBUG_DESCRAMBLING_TIMES
	static const char fname[] =
		"mxd_aviex_pccd_4824_linearity_descramble()";

	MX_HRT_TIMING measurement;
#endif

	long i, j;

#if MXD_AVIEX_PCCD_4824_DEBUG_DESCRAMBLING_TIMES
	MX_HRT_START( measurement );
#endif

	for ( i = 0; i < i_framesize; i++ ) {
	    for ( j = 0; j < j_framesize; j++ ) {

		image_sector_array[0][i][j]
		    = MXD_AVIEX_PCCD_LOOKUP( lookup_table, raw_frame_data, 2 );

		image_sector_array[1][i][j_framesize-j-1]
		    = MXD_AVIEX_PCCD_LOOKUP( lookup_table, raw_frame_data, 3 );

		image_sector_array[2][i_framesize-i-1][j]
		    = MXD_AVIEX_PCCD_LOOKUP( lookup_table, raw_frame_data, 1 );

		image_sector_array[3][i_framesize-i-1][j_framesize-j-1]
		    = MXD_AVIEX_PCCD_LOOKUP( lookup_table, raw_frame_data, 0 );

		raw_frame_data += 4;
	    }
	}

#if MXD_AVIEX_PCCD_4824_DEBUG_DESCRAMBLING_TIMES
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_4824_streak_camera_descramble( MX_AREA_DETECTOR *ad,
				MX_AVIEX_PCCD *aviex_pccd,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *raw_frame )
{
#if 0
	static const char fname[] =
			"mxd_aviex_pccd_4824_streak_camera_descramble()";
#endif
	uint16_t *image_data, *raw_data;
	uint16_t *image_ptr, *raw_ptr;
	long i, j, row_framesize, column_framesize, total_raw_pixels;
	long rfs;
	mx_status_type mx_status;

	/* First, we figure out how many pixels are in each line
	 * of the raw data by asking for the framesize.
	 */

	mx_status = mx_area_detector_get_framesize( ad->record,
					&row_framesize, &column_framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	total_raw_pixels = row_framesize * column_framesize;

	raw_data   = raw_frame->image_data;
	image_data = image_frame->image_data;

#if 0
	MX_DEBUG(-2,("%s: row_framesize = %ld, column_framesize = %ld",
		fname, row_framesize, column_framesize));
#endif

	/* Loop through the lines of the raw image. */

	for ( i = 0; i < (column_framesize/2L); i++ ) {
		/* The raw data arrives in groups of 4 pixels that need
		 * to be appropriately copied to the final image frame.
		 */

		raw_ptr = raw_data + i * row_framesize * 2L;

		image_ptr = image_data + i * row_framesize * 2L;

		/* Copy the pixels. */

		for ( j = 0; j < (row_framesize/4L); j++ ) {

			rfs = row_framesize;

			image_ptr[j]                   = raw_ptr[16*j + 2];
			image_ptr[rfs/2 - 1 - j]       = raw_ptr[16*j + 3];
			image_ptr[rfs + j]             = raw_ptr[16*j + 1];
			image_ptr[rfs + rfs/2 - 1 - j] = raw_ptr[16*j + 0];
		}
	}

	/* Patch the column framesize and the image length so that
	 * it matches the total size of the streak camera image.
	 */

	MXIF_COLUMN_FRAMESIZE(image_frame) = column_framesize / 2L;

	image_frame->image_length = ( total_raw_pixels / 2L )
			* mx_round( MXIF_BYTES_PER_PIXEL(raw_frame) );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_4824_set_sequence_start_delay( MX_AVIEX_PCCD *aviex_pccd,
						unsigned long new_delay_time )
{
	mx_status_type mx_status;

	/* Tell the detector head to change its binsize. */

	mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_INITIAL_DELAY_TIME,
				new_delay_time );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_4824_configure_for_sequence( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd )
{
	static const char fname[] =
		"mxd_aviex_pccd_4824_configure_for_sequence()";

	MX_SEQUENCE_PARAMETERS *sp;
	unsigned long old_control_register_value, new_control_register_value;
	unsigned long old_detector_readout_mode;
	long vinput_horiz_framesize, vinput_vert_framesize;
	long num_streak_mode_lines;
	long num_frames, exposure_steps, gap_steps;
	long total_num_subimage_lines;
	double exposure_time, frame_time, gap_time, subimage_time, line_time;
	mx_status_type mx_status;

	sp = &(ad->sequence_parameters);

	/* Get the old detector readout mode, since that will be
	 * used by the following code.
	 */

	mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_CONTROL,
				&old_control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	old_detector_readout_mode = old_control_register_value
			& MXF_AVIEX_PCCD_4824_DETECTOR_READOUT_MASK;

#if MXD_AVIEX_PCCD_4824_DEBUG
	MX_DEBUG(-2,("%s: old_control_register_value = %#lx",
		fname, old_control_register_value));
	MX_DEBUG(-2,("%s: old_detector_readout_mode = %#lx",
		fname, old_detector_readout_mode));
#endif
	/* Reprogram the detector head for the requested sequence. */

	new_control_register_value = old_control_register_value;

	/* Mask off the duration bit, since it is only used
	 * by MXT_SQ_BULB sequences.
	 */

	new_control_register_value
		&= (~MXF_AVIEX_PCCD_4824_EXTERNAL_DURATION_TRIGGER);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
	case MXT_SQ_STROBE:
	case MXT_SQ_BULB:

		/* If this is bulb mode, turn on the duration bit. */

		if ( sp->sequence_type == MXT_SQ_BULB ) {
			new_control_register_value
		    |= MXF_AVIEX_PCCD_4824_EXTERNAL_DURATION_TRIGGER;
		}

		/* Get the detector readout time. */

		mx_status = mx_area_detector_get_detector_readout_time(
						ad->record, NULL );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( old_detector_readout_mode != 0 ) {
			/* We must switch to full frame mode. */

			new_control_register_value
		      &= (~MXF_AVIEX_PCCD_4824_DETECTOR_READOUT_MASK);

			/* If we were previously in streak camera mode,
			 * we must restore the normal imaging board
			 * framesize.
			 */

			if ( old_detector_readout_mode
			   == MXF_AVIEX_PCCD_4824_STREAK_CAMERA_MODE )
			{
			    mx_status = mx_video_input_set_framesize(
				aviex_pccd->video_input_record,
				aviex_pccd->vinput_normal_framesize[0],
			       aviex_pccd->vinput_normal_framesize[1]);

			    if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			    mx_status = mx_area_detector_set_binsize(
				aviex_pccd->record,
				aviex_pccd->normal_binsize[0],
				aviex_pccd->normal_binsize[1]);

			    if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			}
		}

		/* Configure the detector head for the requested 
		 * number of frames, exposure time, and gap time.
		 */

		if ( sp->sequence_type == MXT_SQ_ONE_SHOT ) {
			num_frames = 1;
			exposure_time = sp->parameter_array[0];
			frame_time = exposure_time;
		} else
		if ( sp->sequence_type == MXT_SQ_CONTINUOUS ) {
			num_frames = 1;
			exposure_time = sp->parameter_array[0];
			frame_time = exposure_time;
		} else
		if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = sp->parameter_array[1];
			frame_time = sp->parameter_array[2];
		} else
		if ( sp->sequence_type == MXT_SQ_CIRCULAR_MULTIFRAME ) {
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = sp->parameter_array[1];
			frame_time = sp->parameter_array[2];
		} else
		if ( sp->sequence_type == MXT_SQ_STROBE ) {
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = sp->parameter_array[1];

			frame_time = exposure_time
					+ ad->detector_readout_time;
		} else
		if ( sp->sequence_type == MXT_SQ_BULB ) {
			num_frames = mx_round( sp->parameter_array[0] );

			if ( num_frames > 1 ) {
				return mx_error(
				MXE_WOULD_EXCEED_LIMIT, fname,
			"For a 'bulb' sequence, the maximum number of "
			"images for detector '%s' is 1.  You requested "
			"%ld images, which is not supported.",
					ad->record->name, num_frames );
			}

			exposure_time =
				aviex_pccd->exposure_and_gap_step_size;

			frame_time = exposure_time;
		} else
		if ( sp->sequence_type == MXT_SQ_GEOMETRICAL ) {
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = sp->parameter_array[1];
			frame_time    = sp->parameter_array[2];

			gap_time = frame_time - exposure_time
					- ad->detector_readout_time;

#if MXD_AVIEX_PCCD_4824_DEBUG
			MX_DEBUG(-2,("%s: num_frames = %ld",
				fname, num_frames));
			MX_DEBUG(-2,("%s: exposure_time = %g",
				fname, exposure_time));
			MX_DEBUG(-2,("%s: frame_time = %g",
				fname, frame_time)); 
			MX_DEBUG(-2,("%s: detector_readout_time = %g",
				fname, ad->detector_readout_time));
			MX_DEBUG(-2,("%s: gap_time = %g",
				fname, gap_time));
#endif

		} else {
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Inconsistent control structures for "
			"sequence type.  Sequence type = %lu",
				sp->sequence_type );
		}

		mx_status = mxd_aviex_pccd_write_register(
				aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_FRAMES_PER_SEQUENCE,
				num_frames );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exposure_steps = mx_round_down( exposure_time
			/ aviex_pccd->exposure_and_gap_step_size );

		mx_status = mxd_aviex_pccd_write_register(
				aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_EXPOSURE_TIME,
				exposure_steps );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( sp->sequence_type == MXT_SQ_CONTINUOUS ) {
			/* For continuous sequences, we want the
			 * gap time to be one, so that the camera
			 * will take frames as fast as it can.
			 */

			mx_status = mxd_aviex_pccd_write_register(
				aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_GAP_TIME,
				1 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} else
		if ( num_frames > 1 ) {
			/* For sequence types other than
			 * MXT_SQ_CONTINUOUS.
			 */

			gap_time = frame_time - exposure_time
					- ad->detector_readout_time;

#if MXD_AVIEX_PCCD_4824_DEBUG
			MX_DEBUG(-2,
			("%s: num_frames = %ld, gap_time = %g",
				fname, num_frames, gap_time));
#endif

			if ( gap_time < 0.0 ) {
				return mx_error(
					MXE_ILLEGAL_ARGUMENT, fname,
			"The requested time per frame (%g seconds) "
			"is less than the sum of the requested "
			"exposure time (%g seconds) and the detector "
			"readout time (%g seconds) for detector '%s'.",
					frame_time, exposure_time,
					ad->detector_readout_time,
					ad->record->name );
			}

			gap_steps = mx_round_down( gap_time
			    / aviex_pccd->exposure_and_gap_step_size );

			if ( gap_steps > 65535 ) {
				return mx_error(
					MXE_ILLEGAL_ARGUMENT, fname,
			"The computed gap time (%g seconds) is "
			"greater than the maximum allowed gap time "
			"(%g seconds) for detector '%s'.",
					gap_time,
		aviex_pccd->exposure_and_gap_step_size * (double)65535,
					ad->record->name );
			}

			if ( gap_steps < 1 ) {
				return mx_error(
					MXE_ILLEGAL_ARGUMENT, fname,
			"The computed gap time (%g seconds) is "
			"less than the minimum allowed gap time "
			"(%g seconds) for detector '%s'.",
				gap_time,
				aviex_pccd->exposure_and_gap_step_size,
				ad->record->name );
			}

			mx_status = mxd_aviex_pccd_write_register(
				aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_GAP_TIME,
				gap_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;
	case MXT_SQ_STREAK_CAMERA:
		/* See if the user has requested too few streak
		 * camera lines.
		 */

		num_streak_mode_lines
			= mx_round_down( sp->parameter_array[0] );

#if 0
		if ( num_streak_mode_lines < 1050 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The number of streak camera lines (%ld) requested "
		"for area detector '%s' is less than the minimum "
		"of 1050 lines.  If you need less than 1050 lines, "
		"then use subimage mode instead.",
				num_streak_mode_lines,
				ad->record->name );
		}
#endif

		/* Get the current framesize. */

		mx_status = mx_video_input_get_framesize(
				aviex_pccd->video_input_record,
				&vinput_horiz_framesize,
				&vinput_vert_framesize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( old_detector_readout_mode
			!= MXF_AVIEX_PCCD_4824_STREAK_CAMERA_MODE )
		{
			/* Before switching to streak camera mode,
			 * save the video board's current framesize
			 * for later restoration.
			 */

			aviex_pccd->vinput_normal_framesize[0]
				= vinput_horiz_framesize;

			aviex_pccd->vinput_normal_framesize[1]
				= vinput_vert_framesize;

			aviex_pccd->normal_binsize[0] = ad->binsize[0];

			aviex_pccd->normal_binsize[1] = ad->binsize[1];

			/* Now switch to streak camera mode. */

			new_control_register_value
			    |= MXF_AVIEX_PCCD_4824_STREAK_CAMERA_MODE;
		}

		/* Set the number of frames in sequence to 1. */

		mx_status = mxd_aviex_pccd_write_register(
				aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_FRAMES_PER_SEQUENCE,
				1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the number of streak camera mode lines.*/

		mx_status = mxd_aviex_pccd_write_register(
				aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_STREAK_MODE_LINES,
				num_streak_mode_lines );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the exposure time per line. */

		exposure_time = sp->parameter_array[1];

		exposure_steps = mx_round_down( exposure_time
			/ aviex_pccd->exposure_and_gap_step_size );

		mx_status = mxd_aviex_pccd_write_register(
			aviex_pccd,
			MXLV_AVIEX_PCCD_4824_DH_EXPOSURE_TIME,
			exposure_steps );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Compute the gap time per line from the 
		 * total time per line.
		 */

		line_time = sp->parameter_array[2];

		gap_time = line_time - exposure_time;

		gap_steps = mx_round_down( gap_time
			/ aviex_pccd->exposure_and_gap_step_size );

		if ( gap_steps < 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The computed gap time (%g seconds) is "
			"less than the minimum allowed gap time "
			"(%g seconds) for detector '%s'.",
				gap_time,
				aviex_pccd->exposure_and_gap_step_size,
				ad->record->name );
		}

		/* Set the gap time. */

		mx_status = mxd_aviex_pccd_write_register(
			aviex_pccd,
			MXLV_AVIEX_PCCD_4824_DH_GAP_TIME,
			gap_steps );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the streak-mode framesize. */

		mx_status = mx_video_input_set_framesize(
			aviex_pccd->video_input_record,
			vinput_horiz_framesize,
			num_streak_mode_lines / 2L );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get the detector readout time. */

		mx_status = mx_area_detector_get_detector_readout_time(
						ad->record, NULL );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXT_SQ_SUBIMAGE:
		total_num_subimage_lines =
				mx_round( sp->parameter_array[0]
					* sp->parameter_array[1] );

		if ( total_num_subimage_lines >
				MXD_MAXIMUM_TOTAL_SUBIMAGE_LINES )
		{
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested number of lines per subimage (%g) "
		"and the requested number of subimages (%g) results "
		"in a total number of image lines (%ld) that is "
		"larger than the maximum number of subimage lines (%d) "
		"allowed by area detector '%s'.",
				sp->parameter_array[0],
				sp->parameter_array[1],
				total_num_subimage_lines,
				MXD_MAXIMUM_TOTAL_SUBIMAGE_LINES,
				ad->record->name );
		}

		if ( old_detector_readout_mode
			!= MXF_AVIEX_PCCD_4824_SUBIMAGE_MODE )
		{
			/* Switch to sub-image mode. */

			new_control_register_value
		    &= (~MXF_AVIEX_PCCD_4824_DETECTOR_READOUT_MASK);

			new_control_register_value
				|= MXF_AVIEX_PCCD_4824_SUBIMAGE_MODE;

			/* If we were previously in streak camera mode,
			 * we must also restore the normal imaging
			 * board framesize.
			 */

			if ( old_detector_readout_mode
			   == MXF_AVIEX_PCCD_4824_STREAK_CAMERA_MODE )
			{
			    mx_status = mx_video_input_set_framesize(
				aviex_pccd->video_input_record,
				aviex_pccd->vinput_normal_framesize[0],
			       aviex_pccd->vinput_normal_framesize[1]);

			    if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			    mx_status = mx_area_detector_set_binsize(
				aviex_pccd->record,
				aviex_pccd->normal_binsize[0],
			       aviex_pccd->normal_binsize[1]);

			    if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			}
		}

		/* Set the number of frames in sequence to 1. */

		mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_FRAMES_PER_SEQUENCE,
				1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the number of lines per subimage.
		 * The number of subimage lines is twice
		 * the value of the "subframe size".
		 */

		mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_SUBFRAME_SIZE,
			    mx_round_down(sp->parameter_array[0]/2.0) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the number of subimages per frame. */

		mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
				MXLV_AVIEX_PCCD_4824_DH_SUBIMAGES_PER_READ,
				mx_round_down(sp->parameter_array[1]) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the exposure time. */

		exposure_time = sp->parameter_array[2];

		exposure_steps = mx_round_down( exposure_time
			/ aviex_pccd->exposure_and_gap_step_size );

		mx_status = mxd_aviex_pccd_write_register(
			aviex_pccd,
			MXLV_AVIEX_PCCD_4824_DH_EXPOSURE_TIME,
			exposure_steps );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Compute the gap time from the subimage time. */

		subimage_time = sp->parameter_array[3];

		gap_time = subimage_time - exposure_time;

		gap_steps = mx_round_down( gap_time
			/ aviex_pccd->exposure_and_gap_step_size );

		if ( gap_steps < 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The computed gap time (%g seconds) is "
			"less than the minimum allowed gap time "
			"(%g seconds) for detector '%s'.",
				gap_time,
				aviex_pccd->exposure_and_gap_step_size,
				ad->record->name );
		}

		/* Set the gap time. */

		mx_status = mxd_aviex_pccd_write_register(
			aviex_pccd,
			MXLV_AVIEX_PCCD_4824_DH_GAP_TIME,
			gap_steps );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
				
		/* For subimage mode, getting the detector readout
		 * time is the last thing we do, since the value
		 * of the detector readout time depends on some of
		 * the values set above.
		 */

		/* Get the detector readout time. */

		mx_status = mx_area_detector_get_detector_readout_time(
						ad->record, NULL );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal sequence type %ld requested "
		"for detector '%s'.",
			sp->sequence_type, ad->record->name );
	}

	/* Reprogram the control register for the new mode. */

#if MXD_AVIEX_PCCD_4824_DEBUG
	MX_DEBUG(-2, ("%s: old_control_register_value = %#lx",
			fname, old_control_register_value));
	MX_DEBUG(-2, ("%s: new_control_register_value = %#lx",
			fname, new_control_register_value));
#endif

	if (new_control_register_value != old_control_register_value) {

		mx_status = mxd_aviex_pccd_write_register(
					aviex_pccd,
					MXLV_AVIEX_PCCD_4824_DH_CONTROL,
					new_control_register_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

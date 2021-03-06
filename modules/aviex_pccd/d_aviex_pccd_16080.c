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
 * Copyright 2006-2009, 2011-2012, 2015, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_AVIEX_PCCD_16080_DEBUG        		FALSE
#define MXD_AVIEX_PCCD_16080_DEBUG_DESCRAMBLING		FALSE
#define MXD_AVIEX_PCCD_16080_DEBUG_SEQUENCE_TIMES	FALSE

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

#define MXD_MAXIMUM_TOTAL_SUBIMAGE_LINES	2048

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

	if ( register_address >= MXLV_AVIEX_PCCD_DH_BASE ) {
		register_address -= MXLV_AVIEX_PCCD_DH_BASE;
	}

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

	if ( register_address >= MXLV_AVIEX_PCCD_DH_BASE ) {
		register_address -= MXLV_AVIEX_PCCD_DH_BASE;
	}

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

#define INIT_REGISTER( i, s, v, r, w, t, n, x ) \
        do {								    \
                mx_status = mxd_aviex_pccd_16080_init_register( aviex_pccd, \
			(i), (s), (v), (r), (w), (t), (n), (x) );	    \
									    \
                if ( mx_status.code != MXE_SUCCESS )			    \
                        return mx_status;				    \
        } while(0)

static mx_status_type
mxd_aviex_pccd_16080_init_register( MX_AVIEX_PCCD *aviex_pccd,
					long register_index,
					int register_size,	/* in bytes */
					unsigned long register_value,
					mx_bool_type read_only,
					mx_bool_type write_only,
					mx_bool_type power_of_two,
					unsigned long minimum,
					unsigned long maximum )
{
	unsigned long low_byte, high_byte;
	unsigned long low_minimum, low_maximum;
	unsigned long high_minimum, high_maximum;
	mx_status_type mx_status;

	switch( register_size ) {
	case 1:
		mx_status = mxd_aviex_pccd_init_register( aviex_pccd,
					register_index, FALSE,
					register_size, register_value,
					read_only, write_only, power_of_two,
					minimum, maximum );
		break;

	case 2:
		low_byte = register_value & 0xff;
		high_byte = ( register_value >> 8 ) & 0xff;

		if ( maximum <= 0xff ) {
			low_minimum = minimum & 0xff;
			low_maximum = maximum & 0xff;
			high_minimum = 0;
			high_maximum = 0;
		} else {
			low_minimum = 0x0;
			low_maximum = 0xff;
			high_minimum = ( minimum >> 8 ) & 0xff;
			high_maximum = ( maximum >> 8 ) & 0xff;
		}

		mx_status = mxd_aviex_pccd_init_register( aviex_pccd,
					register_index, FALSE,
					register_size, low_byte,
					read_only, write_only, power_of_two,
					low_minimum, low_maximum );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_aviex_pccd_init_register( aviex_pccd,
					register_index + 1, TRUE,
					register_size, high_byte,
					read_only, write_only, power_of_two,
					high_minimum, high_maximum );
		break;
	}

	return mx_status;
}

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

	MX_AVIEX_PCCD_REGISTER *reg;
	int i, min_register, max_register;

	size_t array_size;
	unsigned long fpga_version;
	unsigned long control_register_value;
	unsigned long pccd_flags;
	unsigned long delay_microseconds;
	mx_status_type mx_status;

	pccd_flags = aviex_pccd->aviex_pccd_flags;

	/* Initialize data structures used to specify attributes
	 * of each detector head register.
	 */

	aviex_pccd->num_registers =
		MXLV_AVIEX_PCCD_16080_DH_YDOFFS - MXLV_AVIEX_PCCD_DH_BASE + 1;

	/* The last actual register is at MXLV_AVIEX_PCCD_16080_DH_YDOFFS + 1 */

	aviex_pccd->num_registers++;

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
				1,  100, TRUE, FALSE, FALSE, 0,  255 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_CONTROL,
				1,  2, FALSE, FALSE, FALSE, 0,  63 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VIDBIN,
				1,  1, FALSE, FALSE, FALSE, 1,  8 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VSHBIN,
				1,  1, FALSE, FALSE, TRUE, 1,  128 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_ROILINES,
				1,  1, FALSE, FALSE, FALSE, 1,  8 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_NOF,
				1,  1, FALSE, FALSE, FALSE, 1,  128 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VREAD,
				2,  1042, FALSE, FALSE, FALSE, 1,  1042 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VDARK,
				1,  2, FALSE, FALSE, FALSE, 0,  7 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HLEAD,
				1,  4, FALSE, FALSE, FALSE, 0,  7 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HPIX,
				2,  1042, FALSE, FALSE, FALSE, 1,  1042 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_TPRE,
				2,  100, FALSE, FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_SHUTTER,
				2,  10, FALSE, FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_TPOST,
				2,  100, FALSE, FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HDARK,
				1,  6, FALSE, FALSE, FALSE, 0,  7 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HBIN,
				1,  1, FALSE, FALSE, TRUE, 1,  128 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_ROIOFFS,
				2,  0, FALSE, FALSE, FALSE, 0,  2047 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_NOE,
				2,  0, FALSE, FALSE, FALSE, 1,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XAOFFS,
				2,  1620, FALSE, TRUE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XBOFFS,
				2,  1000, FALSE, TRUE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XCOFFS,
				2,  1100, FALSE, TRUE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XDOFFS,
				2,  1260, FALSE, TRUE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YAOFFS,
				2,  1520, FALSE, TRUE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YBOFFS,
				2,  264, FALSE, TRUE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YCOFFS,
				2,  1968, FALSE, TRUE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YDOFFS,
				2,  1648, FALSE, TRUE, FALSE, 0,  4095 );

	/* Check to find out the firmware version that is being used by
	 * the detector head.
	 */

	mx_status = mxd_aviex_pccd_16080_read_register( aviex_pccd,
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

	switch( aviex_pccd->aviex_pccd_type ) {
	case MXT_AD_PCCD_16080:

		/* The PCCD-16080 camera has two CCD chips with a maximum
		 * image size of 4168 by 2084.  Each line from the detector
		 * contains 1024 groups of pixels with 8 pixels per group.
		 * A full frame image has 1024 lines.  This means that the
		 * maximum resolution of the video card should be 8192 by 1024.
		 */

		ad->maximum_framesize[0] = 4168;
		ad->maximum_framesize[1] = 2084;

		ad->correction_measurement_sequence_type = MXT_SQ_ONE_SHOT;

		aviex_pccd->horiz_descramble_factor = 2;
		aviex_pccd->vert_descramble_factor  = 2;

		aviex_pccd->pixel_clock_frequency = 40.0e6;

		aviex_pccd->num_ccd_taps = 8;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Record '%s' has an MX type of %ld which "
			"is not supported by this driver.",
				record->name, aviex_pccd->aviex_pccd_type );
		break;
	}

	/* Initialize all of the output offset registers, since the
	 * FPGA firmware does not do that.
	 *
	 * FIXME! - Should this be read from a file?
	 */

	min_register =
		MXLV_AVIEX_PCCD_16080_DH_XAOFFS - MXLV_AVIEX_PCCD_DH_BASE;

	max_register =
		MXLV_AVIEX_PCCD_16080_DH_YDOFFS - MXLV_AVIEX_PCCD_DH_BASE + 1;

	for ( i = min_register; i <= max_register; i++ ) {
		reg = &(aviex_pccd->register_array[i]);

		mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
							i, reg->value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Set the control register to internal trigger with high speed
	 * readout and automatic offset adjustment on.
	 */

	control_register_value = MXF_AVIEX_PCCD_16080_CCD_ON
			| MXF_AVIEX_PCCD_16080_HIGH_SPEED
			| MXF_AVIEX_PCCD_16080_AUTOMATIC_OFFSET_CORRECTION_ON
			| MXF_AVIEX_PCCD_16080_EDGE_TRIGGER;

	/* If requested, turn on the test mode pattern. */

	if ( pccd_flags & MXF_AVIEX_PCCD_USE_TEST_PATTERN ) {
		control_register_value |= MXF_AVIEX_PCCD_16080_TEST_MODE_ON;

		mx_warning( "Area detector '%s' will return a test image "
			"instead of taking a real image.",
				record->name );
	}

	/* Write the control register. */

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Unconditionally set VIDBIN to 1, since it isn't really used. */

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_VIDBIN, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the horizontal and vertical dark pixel lines. */

	/* These values for HDARK, VDARK, and HLEAD are for the detector
	 * at BioCAT sector 18-ID.
	 */

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_HDARK, 6 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_VDARK, 2 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_HLEAD, 4 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the pre and post shutter delay times (in microseconds). */

	delay_microseconds = 100;

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_TPRE,
					delay_microseconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_TPOST,
					delay_microseconds );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_get_pseudo_register( MX_AVIEX_PCCD *aviex_pccd,
					long parameter_type,
					unsigned long *pseudo_reg_value )
{
	static const char fname[] =
			"mxd_aviex_pccd_16080_get_pseudo_register()";

	unsigned long control_register;
	mx_status_type mx_status;

	/* For pseudo registers, we must extract the value from
	 * the control register.
	 */

	mx_status = mxd_aviex_pccd_16080_read_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					&control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( parameter_type ) {
	case MXLV_AVIEX_PCCD_16080_DH_CCD_ON:
		*pseudo_reg_value = control_register & 0x1;
		break;
	case MXLV_AVIEX_PCCD_16080_DH_READOUT_SPEED:
		*pseudo_reg_value = (control_register >> 1) & 0x1;
		break;
	case MXLV_AVIEX_PCCD_16080_DH_OFFSET_CORRECTION:
		*pseudo_reg_value = (control_register >> 2) & 0x1;
		break;
	case MXLV_AVIEX_PCCD_16080_DH_EXPOSURE_MODE:
		*pseudo_reg_value = (control_register >> 3) & 0x1;
		break;
	case MXLV_AVIEX_PCCD_16080_DH_EDGE_TRIGGERED:
		*pseudo_reg_value = (control_register >> 4) & 0x1;
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
mxd_aviex_pccd_16080_set_pseudo_register( MX_AVIEX_PCCD *aviex_pccd,
					long parameter_type,
					unsigned long register_value )
{
	unsigned long control_register, pseudo_reg_value;
	mx_status_type mx_status;

	/* For pseudo registers, we must change specific bits in
	 * the control register.
	 */

	mx_status = mxd_aviex_pccd_16080_read_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					&control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( parameter_type ) {
	case MXLV_AVIEX_PCCD_16080_DH_CCD_ON:
		pseudo_reg_value = register_value & 0x1;

		control_register &= ~0x1;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_AVIEX_PCCD_16080_DH_READOUT_SPEED:
		pseudo_reg_value = ( register_value & 0x1 ) << 0x1;

		control_register &= ~0x2;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_AVIEX_PCCD_16080_DH_OFFSET_CORRECTION:
		pseudo_reg_value = ( register_value & 0x1 ) << 2;

		control_register &= ~0x4;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_AVIEX_PCCD_16080_DH_EXPOSURE_MODE:
		pseudo_reg_value = ( register_value & 0x1 ) << 3;

		control_register &= ~0x8;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_AVIEX_PCCD_16080_DH_EDGE_TRIGGERED:
		pseudo_reg_value = ( register_value & 0x1 ) << 4;

		control_register &= ~0x10;

		control_register |= pseudo_reg_value;
		break;
	}

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					control_register );
	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_set_trigger_mode( MX_AVIEX_PCCD *aviex_pccd,
					mx_bool_type external_trigger,
					mx_bool_type edge_triggered )
{
	static const char fname[] = "mxd_aviex_pccd_16080_set_trigger_mode()";

	MX_AREA_DETECTOR *ad;
        unsigned long control_register_value;
        mx_status_type mx_status;

	ad = (MX_AREA_DETECTOR *) aviex_pccd->record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for '%s' is NULL.",
			aviex_pccd->record->name );
	}

        mx_status = mxd_aviex_pccd_16080_read_register( aviex_pccd,
                                        MXLV_AVIEX_PCCD_16080_DH_CONTROL,
                                        &control_register_value );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

	ad->trigger_mode = 0;

        if ( external_trigger ) {
                control_register_value
                        |= MXF_AVIEX_PCCD_16080_EXTERNAL_TRIGGER;

		ad->trigger_mode |= MXF_DEV_EXTERNAL_TRIGGER;
        } else {
                control_register_value
                        &= (~MXF_AVIEX_PCCD_16080_EXTERNAL_TRIGGER);

		ad->trigger_mode |= MXF_DEV_INTERNAL_TRIGGER;
        }

        if ( edge_triggered ) {
                control_register_value
                        |= MXF_AVIEX_PCCD_16080_EDGE_TRIGGER;

		ad->trigger_mode |= MXF_DEV_EDGE_TRIGGER;
        } else {
                control_register_value
                        &= (~MXF_AVIEX_PCCD_16080_EDGE_TRIGGER);
        }

        mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
                                        MXLV_AVIEX_PCCD_16080_DH_CONTROL,
                                        control_register_value );

        return mx_status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_aviex_pccd_16080_write_binning_register( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd,
					unsigned long register_address,
					unsigned long binsize_value )
{
	static const char fname[] =
		"mxd_aviex_pccd_16080_write_binning_register()";

	MX_AVIEX_PCCD_REGISTER *reg;
	char command[20], response[20];
	unsigned long binsize_value_read, binsize_code;
	mx_status_type mx_status;

	if ( register_address >= MXLV_AVIEX_PCCD_DH_BASE ) {
		register_address -= MXLV_AVIEX_PCCD_DH_BASE;
	}

	reg = &(aviex_pccd->register_array[register_address]);

	MXW_UNUSED( reg );

	/* Is this a valid value for this binsize register. */

	mx_status = mxd_aviex_pccd_check_value( aviex_pccd,
					register_address, binsize_value, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the binsize code for this binsize value. */

	switch( binsize_value ) {
	case 1:
		binsize_code = 1;
		break;
	case 2:
		binsize_code = 2;
		break;
	case 4:
		binsize_code = 3;
		break;
	case 8:
		binsize_code = 4;
		break;
	case 16:
		binsize_code = 5;
		break;
	case 32:
		binsize_code = 6;
		break;
	case 64:
		binsize_code = 7;
		break;
	case 128:
		binsize_code = 8;
		break;
	default:
		binsize_code = 0;
		break;
	}

	/* Send the new binsize code to the detector head. */

	snprintf( command, sizeof(command),
		"W%02lu%03lu", register_address, binsize_code );

	mx_status = mxd_aviex_pccd_camera_link_command( aviex_pccd,
					command, response, sizeof(response),
					MXD_AVIEX_PCCD_16080_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read back the binsize value from the register.
	 *
	 * WARNING: The value we read back from the register will
	 * be the binsize_value that was passed to this routine and
	 * _NOT_ the binsize_code that we just sent to the detector head.
	 */

	mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
					register_address, &binsize_value_read );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the value read back matches the value that
	 * was passed to this routine.
	 */

	if ( binsize_value_read != binsize_value ) {
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"The attempt to set the binsize for '%s' to %lu appears to "
		"have failed since the value read back from register %lu "
		"is now %lu.",
			aviex_pccd->record->name, binsize_value,
			register_address, binsize_value_read );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_set_binsize( MX_AREA_DETECTOR *ad,
				MX_AVIEX_PCCD *aviex_pccd )
{
#if MXD_AVIEX_PCCD_16080_DEBUG
	static const char fname[] = "mxd_aviex_pccd_16080_set_binsize()";
#endif

	unsigned long roilines, roilines_register;
	mx_status_type mx_status;

	/*=== Set the registers for the horizontal binsize. ===*/

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_HPIX,
					ad->framesize[0] / 4 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aviex_pccd_16080_write_binning_register(
					ad, aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_HBIN,
					ad->binsize[0] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*=== Set the registers for the vertical binsize. ===*/

	mx_status = mxd_aviex_pccd_16080_write_binning_register(
					ad, aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_VSHBIN,
					ad->binsize[1] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the number of frames to 1. */

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_NOF, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the ROILINES register. */

	roilines = ad->framesize[1] / 2;

	roilines_register = 9 - ad->binsize[1];

#if MXD_AVIEX_PCCD_16080_DEBUG
	MX_DEBUG(-2,("%s: roilines = %lu, roilines_register = %lu",
		fname, roilines, roilines_register));
#endif

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_ROILINES,
					roilines_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Since NOF has been set to 1, then VREAD = ROILINES and ROIOFFS = 0 */

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_VREAD,
					roilines );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_ROIOFFS, 0 );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_descramble( uint16_t *raw_frame_data,
					uint16_t ***image_sector_array,
					long i_framesize,
					long j_framesize )
{
#if MXD_AVIEX_PCCD_16080_DEBUG_DESCRAMBLING
	static const char fname[] = "mxd_aviex_pccd_16080_descramble()";
#endif

	long i, j;

#if MXD_AVIEX_PCCD_16080_DEBUG_DESCRAMBLING
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	for ( i = 0; i < i_framesize; i++ ) {
	    for ( j = 0; j < j_framesize; j++ ) {

                image_sector_array[0][i][j] = raw_frame_data[6];

                image_sector_array[1][i][j_framesize-j-1] = raw_frame_data[7];

                image_sector_array[2][i][j] = raw_frame_data[2];

                image_sector_array[3][i][j_framesize-j-1] = raw_frame_data[3];

                image_sector_array[4][i_framesize-i-1][j] = raw_frame_data[5];

                image_sector_array[5][i_framesize-i-1][j_framesize-j-1]
                                                        = raw_frame_data[4];

                image_sector_array[6][i_framesize-i-1][j] = raw_frame_data[1];

                image_sector_array[7][i_framesize-i-1][j_framesize-j-1]
                                                        = raw_frame_data[0];

		raw_frame_data += 8;
	    }
	}

#if MXD_AVIEX_PCCD_16080_DEBUG_DESCRAMBLING
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_configure_for_sequence( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd )
{
	static const char fname[] =
		"mxd_aviex_pccd_16080_configure_for_sequence()";

	MX_SEQUENCE_PARAMETERS *sp;
#if 0
	long vinput_horiz_framesize, vinput_vert_framesize;
#endif
	long num_frames, exposure_steps;
	unsigned long roilines, noe, vread, roioffs;
	double exposure_time;
	long total_num_subimage_lines;
#if 0
	double subimage_time, gap_time;
	long gap_steps;
#endif
	long master_clock;
	mx_bool_type camera_is_master;

	long trigger_mode, original_trigger_mode, mask, multiframe_trigger;
	mx_status_type mx_status;

#if MXD_AVIEX_PCCD_16080_DEBUG
	MX_DEBUG(-2,("%s invoked for '%s'", fname, ad->record->name));
#endif

	sp = &(ad->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_STROBE:
	case MXT_SQ_DURATION:

#if 0
		if ( in_subimage_mode ) {
			/* We must turn off subimage mode. */

			mx_status = mxd_aviex_pccd_16080_write_register(
				aviex_pccd, MXLV_AVIEX_PCCD_16080_DH_NOE, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* We must restore the normal imaging board framesize.*/

			mx_status = mx_video_input_set_framesize(
				aviex_pccd->video_input_record,
				aviex_pccd->vinput_normal_framesize[0],
				aviex_pccd->vinput_normal_framesize[1] );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_area_detector_set_binsize(
				aviex_pccd->record,
				aviex_pccd->normal_binsize[0],
				aviex_pccd->normal_binsize[1] );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
#endif

		/* Configure the detector head for the requested 
		 * number of frames, exposure time, and gap time.
		 */

		original_trigger_mode = ad->trigger_mode;

		mask = MXF_DEV_INTERNAL_TRIGGER
				| MXF_DEV_EXTERNAL_TRIGGER
				| MXF_DEV_EDGE_TRIGGER
				| MXF_DEV_LEVEL_TRIGGER;

		trigger_mode = original_trigger_mode & (~mask);

		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			num_frames = 1;
			exposure_time = sp->parameter_array[0];
			camera_is_master = FALSE;
			trigger_mode |= MXF_DEV_INTERNAL_TRIGGER;
			trigger_mode |= MXF_DEV_EDGE_TRIGGER;
			break;
		case MXT_SQ_MULTIFRAME:
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = sp->parameter_array[1];
			camera_is_master = FALSE;

			/* Internal/external trigger setting is left alone. */

			multiframe_trigger = original_trigger_mode
		  & ( MXF_DEV_INTERNAL_TRIGGER | MXF_DEV_EXTERNAL_TRIGGER );

			trigger_mode |= multiframe_trigger;

			trigger_mode |= MXF_DEV_EDGE_TRIGGER;
			break;
		case MXT_SQ_STROBE:
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = sp->parameter_array[1];
			camera_is_master = TRUE;
			trigger_mode |= MXF_DEV_EXTERNAL_TRIGGER;
			trigger_mode |= MXF_DEV_EDGE_TRIGGER;
			break;
		case MXT_SQ_DURATION:
			num_frames = mx_round( sp->parameter_array[0] );
			exposure_time = -1.0;
			camera_is_master = TRUE;
			trigger_mode |= MXF_DEV_EXTERNAL_TRIGGER;
			trigger_mode |= MXF_DEV_LEVEL_TRIGGER;
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Inconsistent control structures for "
			"sequence type.  Sequence type = %lu",
				sp->sequence_type );
			break;
		}

		MXW_UNUSED( num_frames );

#if MXD_AVIEX_PCCD_16080_DEBUG
		MX_DEBUG(-2,("%s: trigger_mode = %#lx", fname, trigger_mode ));
#endif

		ad->trigger_mode = trigger_mode;

		mx_status = mxd_aviex_pccd_16080_write_register(
				aviex_pccd,
				MXLV_AVIEX_PCCD_16080_DH_NOF,
				1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( sp->sequence_type != MXT_SQ_DURATION ) {
			exposure_steps = mx_round_down( exposure_time
				/ aviex_pccd->exposure_and_gap_step_size );

#if MXD_AVIEX_PCCD_16080_DEBUG
			MX_DEBUG(-2,("%s: exposure_steps = %lu",
				fname, exposure_steps));
#endif

			mx_status = mxd_aviex_pccd_16080_write_register(
					aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_SHUTTER,
					exposure_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( camera_is_master ) {
			master_clock = MXF_VIN_MASTER_CAMERA;

			aviex_pccd->aviex_pccd_flags
				|= MXF_AVIEX_PCCD_CAMERA_IS_MASTER;
		} else {
			master_clock = MXF_VIN_MASTER_VIDEO_BOARD;

			aviex_pccd->aviex_pccd_flags
				&= (~MXF_AVIEX_PCCD_CAMERA_IS_MASTER);
		}

		mx_status = mx_video_input_set_master_clock(
						aviex_pccd->video_input_record,
						master_clock );

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

		/* Set the number of lines per subimage.
		 * The number of subimage lines is twice
		 * the value of the "subframe size".
		 */

		roilines = mx_round_down( sp->parameter_array[0] / 2.0 );

		mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
				MXLV_AVIEX_PCCD_16080_DH_ROILINES,
				roilines );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the number of subimages per frame. */

		noe = mx_round_down( sp->parameter_array[1] );

		mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
				MXLV_AVIEX_PCCD_16080_DH_NOE,
				noe );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* We must set ROIOFFS to match the specified values
		 * of ROILINES, NOE, and VREAD.
		 */

		mx_status = mxd_aviex_pccd_read_register( aviex_pccd,
				MXLV_AVIEX_PCCD_16080_DH_VREAD,
				&vread );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		roioffs = vread - noe * roilines;

		mx_status = mxd_aviex_pccd_write_register( aviex_pccd,
				MXLV_AVIEX_PCCD_16080_DH_ROIOFFS,
				roioffs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the exposure time. */

		exposure_time = sp->parameter_array[2];

		exposure_steps = mx_round_down( exposure_time
			/ aviex_pccd->exposure_and_gap_step_size );

		mx_status = mxd_aviex_pccd_write_register(
			aviex_pccd,
			MXLV_AVIEX_PCCD_16080_DH_SHUTTER,	/* FIXME ? */
			exposure_steps );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 0
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
			MXLV_AVIEX_PCCD_16080_DH_GAP_TIME,
			gap_steps );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif

		/* The PCCD-16080 detector does not have exposure
		 * or gap multipliers.  Overwrite their values
		 * in the parameter array.
		 */

		sp->parameter_array[4] = 1.0;
		sp->parameter_array[5] = 1.0;
				
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal sequence type %ld requested "
		"for detector '%s'.",
			sp->sequence_type, ad->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_terminate_sequence( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd )
{
	unsigned long old_control_register, new_control_register;
	mx_status_type mx_status;

	mx_status = mxd_aviex_pccd_16080_read_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					&old_control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_control_register = old_control_register;

	/* Turn off the external trigger bit. */

	new_control_register &= (~MXF_AVIEX_PCCD_16080_EXTERNAL_TRIGGER);
	

	/* Turn on the edge trigger bit. */

	new_control_register |= MXF_AVIEX_PCCD_16080_EDGE_TRIGGER;

	if ( new_control_register != old_control_register ) {

		mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					new_control_register );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

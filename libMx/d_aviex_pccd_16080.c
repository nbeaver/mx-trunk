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

#define INIT_REGISTER( i, s, v, r, t, n, x ) \
        do {                                                             \
                mx_status = mxd_aviex_pccd_16080_init_register(          \
                        aviex_pccd, (i), (s), (v), (r), (t), (n), (x) ); \
                                                                         \
                if ( mx_status.code != MXE_SUCCESS )                     \
                        return mx_status;                                \
        } while(0)

static mx_status_type
mxd_aviex_pccd_16080_init_register( MX_AVIEX_PCCD *aviex_pccd,
					long register_index,
					int register_size,	/* in bytes */
					unsigned long register_value,
					mx_bool_type read_only,
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
					read_only, power_of_two,
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
					read_only, power_of_two,
					low_minimum, low_maximum );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_aviex_pccd_init_register( aviex_pccd,
					register_index + 1, TRUE,
					register_size, high_byte,
					read_only, power_of_two,
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

#if 0
	MX_AVIEX_PCCD_REGISTER *reg;
	int i, min_register, max_register;
#endif
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
					1,  100, TRUE, FALSE, 0,  255 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					1,  2, FALSE, FALSE, 0,  31 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VIDBIN,
					1,  1, FALSE, FALSE, 1,  8 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VSHBIN,
					1,  1, FALSE, FALSE, 1,  8 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_ROILINES,
					1,  1, FALSE, FALSE, 1,  8 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_NOF,
					1,  1, FALSE, FALSE, 1,  128 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VREAD,
					2,  1042, FALSE, FALSE, 1,  1042 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_VDARK,
					1,  2, FALSE, FALSE, 0,  7 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HLEAD,
					1,  4, FALSE, FALSE, 0,  7 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HPIX,
					2,  1042, FALSE, FALSE, 1,  1042 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_TPRE,
					2,  100, FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_SHUTTER,
					2,  10, FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_TPOST,
					2,  100, FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HDARK,
					1,  6, FALSE, FALSE, 0,  7 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_HBIN,
					1,  1, FALSE, FALSE, 1,  8 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_ROIOFFS,
					2,  0, FALSE, FALSE, 0,  2047 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_NOE,
					2,  0, FALSE, FALSE, 1,  65535 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XAOFFS,
					2,  1620, FALSE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XBOFFS,
					2,  1000, FALSE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XCOFFS,
					2,  1100, FALSE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_XDOFFS,
					2,  1260, FALSE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YAOFFS,
					2,  1520, FALSE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YBOFFS,
					2,  264, FALSE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YCOFFS,
					2,  1968, FALSE, FALSE, 0,  4095 );

	INIT_REGISTER( MXLV_AVIEX_PCCD_16080_DH_YDOFFS,
					2,  1648, FALSE, FALSE, 0,  4095 );

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

	switch( record->mx_type ) {
	case MXT_AD_PCCD_16080:

		/* The PCCD-16080 camera has two CCD chips with a maximum
		 * image size of 4168 by 2084.  Each line from the detector
		 * contains 1024 groups of pixels with 8 pixels per group.
		 * A full frame image has 1024 lines.  This means that the
		 * maximum resolution of the video card should be 8192 by 1024.
		 */

		ad->maximum_framesize[0] = 4168;
		ad->maximum_framesize[1] = 2084;

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

#if 0
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
#endif

	/* Set the control register to internal trigger with high speed
	 * readout and automatic offset adjustment on.
	 */

	control_register_value = MXF_AVIEX_PCCD_16080_CCD_ON
			| MXF_AVIEX_PCCD_16080_HIGH_SPEED
			| MXF_AVIEX_PCCD_16080_AUTOMATIC_OFFSET_CORRECTION_ON;

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_CONTROL,
					control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the horizontal and vertical dark pixel lines. */

#if 0
	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_HDARK, 4 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_VDARK, 4 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

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
		*pseudo_reg_value = (control_register >> 3) & 0x3;
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
		pseudo_reg_value = ( register_value & 0x3 ) << 3;

		control_register &= ~0x18;

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
mxd_aviex_pccd_16080_set_external_trigger_mode( MX_AVIEX_PCCD *aviex_pccd,
                                                mx_bool_type external_trigger )
{
        unsigned long control_register_value;
        mx_status_type mx_status;

        mx_status = mxd_aviex_pccd_16080_read_register( aviex_pccd,
                                        MXLV_AVIEX_PCCD_16080_DH_CONTROL,
                                        &control_register_value );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        if ( external_trigger ) {
                control_register_value
                        |= MXF_AVIEX_PCCD_16080_EXTERNAL_TRIGGER;
        } else {
                control_register_value
                        &= (~MXF_AVIEX_PCCD_16080_EXTERNAL_TRIGGER);
        }

        mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
                                        MXLV_AVIEX_PCCD_16080_DH_CONTROL,
                                        control_register_value );

        return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_set_binsize( MX_AREA_DETECTOR *ad,
				MX_AVIEX_PCCD *aviex_pccd )
{
	unsigned long roilines, roilines_register;
	mx_status_type mx_status;

	/*=== Set the registers for the horizontal binsize. ===*/

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_HPIX,
					ad->framesize[0] / 4 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_HBIN,
					ad->binsize[0] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*=== Set the registers for the vertical binsize. ===*/

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
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

	MX_DEBUG(-2,("xxx: roilines = %lu, roilines_register = %lu",
		roilines, roilines_register));

	mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_ROILINES,
					roilines_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Since NOF has been set to 0, then VREAD = ROILINES and ROIOFFS = 0 */

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

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_configure_for_sequence( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd )
{
	static const char fname[] =
		"mxd_aviex_pccd_16080_configure_for_sequence()";

	MX_SEQUENCE_PARAMETERS *sp;
	unsigned long old_streak_count;
	long vinput_horiz_framesize, vinput_vert_framesize;
	long num_streak_mode_lines;
	long num_frames, exposure_steps;
	double exposure_time;
	mx_status_type mx_status;

	sp = &(ad->sequence_parameters);

	/* Find out whether or not we are currently in streak camera mode. */

	mx_status = mxd_aviex_pccd_16080_read_register( aviex_pccd,
				MXLV_AVIEX_PCCD_16080_DH_NOE,
				&old_streak_count );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_AVIEX_PCCD_16080_DEBUG
	MX_DEBUG(-2,("%s: old_streak_count = %lu", fname, old_streak_count));
#endif

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:

		if ( old_streak_count > 0 ) {
			/* We must turn off streak camera mode. */

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

		/* Configure the detector head for the requested 
		 * number of frames, exposure time, and gap time.
		 */

		if ( sp->sequence_type == MXT_SQ_ONE_SHOT ) {
			num_frames = 1;
			exposure_time = sp->parameter_array[0];
		} else {
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Inconsistent control structures for "
			"sequence type.  Sequence type = %lu",
				sp->sequence_type );
		}

		mx_status = mxd_aviex_pccd_16080_write_register(
				aviex_pccd,
				MXLV_AVIEX_PCCD_16080_DH_NOF,
				num_frames );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exposure_steps = mx_round_down( exposure_time
			/ aviex_pccd->exposure_and_gap_step_size );

		mx_status = mxd_aviex_pccd_16080_write_register(
				aviex_pccd,
				MXLV_AVIEX_PCCD_16080_DH_SHUTTER,
				exposure_steps );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;
	case MXT_SQ_STREAK_CAMERA:
		/* See if the user has requested too few streak
		 * camera lines.
		 */

		num_streak_mode_lines
			= mx_round_down( sp->parameter_array[0] );

		/* Get the current framesize. */

		mx_status = mx_video_input_get_framesize(
				aviex_pccd->video_input_record,
				&vinput_horiz_framesize,
				&vinput_vert_framesize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( old_streak_count == 0 ) {
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
		}

		/* Set the number of frames in the sequence to 1. */

		mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
				MXLV_AVIEX_PCCD_16080_DH_NOF, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the number of streak camera mode lines.*/

		mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
					MXLV_AVIEX_PCCD_16080_DH_NOE,
					num_streak_mode_lines );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the exposure time per line. */

		exposure_time = sp->parameter_array[1];

		exposure_steps = mx_round_down( exposure_time
			/ aviex_pccd->exposure_and_gap_step_size );

		MX_DEBUG(-2,
		("%s: FIXME! It is almost certain that setting the "
		"exposure time per line this way is wrong!!!", fname));

		mx_status = mxd_aviex_pccd_16080_write_register( aviex_pccd,
			MXLV_AVIEX_PCCD_16080_DH_SHUTTER, exposure_steps );

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

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal sequence type %ld requested "
		"for detector '%s'.",
			sp->sequence_type, ad->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

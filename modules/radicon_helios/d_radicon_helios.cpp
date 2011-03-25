/*
 * Name:    d_radicon_helios.cpp
 *
 * Purpose: MX driver for the Radicon Helios 20x25 CMOS detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_RADICON_HELIOS_DEBUG	TRUE

#define MXD_RADICON_HELIOS_BYTESWAP	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_bit.h"
#include "mx_array.h"
#include "mx_pulse_generator.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "mx_digital_input.h"
#include "d_mbc_gsc_trigger.h"
#include "i_pleora_iport.h"
#include "d_pleora_iport_vinput.h"
#include "d_radicon_helios.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_radicon_helios_record_function_list = {
	mxd_radicon_helios_initialize_driver,
	mxd_radicon_helios_create_record_structures,
	mxd_radicon_helios_finish_record_initialization,
	NULL,
	NULL,
	mxd_radicon_helios_open
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_radicon_helios_ad_function_list = {
	mxd_radicon_helios_arm,
	mxd_radicon_helios_trigger,
	mxd_radicon_helios_abort,
	mxd_radicon_helios_abort,
	NULL,
	NULL,
	NULL,
	mxd_radicon_helios_get_extended_status,
	mxd_radicon_helios_readout_frame,
	mxd_radicon_helios_correct_frame,
	mxd_radicon_helios_transfer_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_radicon_helios_get_parameter,
	mxd_radicon_helios_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_radicon_helios_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_RADICON_HELIOS_STANDARD_FIELDS
};

long mxd_radicon_helios_num_record_fields
		= sizeof( mxd_radicon_helios_record_field_defaults )
			/ sizeof( mxd_radicon_helios_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_radicon_helios_rfield_def_ptr
			= &mxd_radicon_helios_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_radicon_helios_get_pointers( MX_AREA_DETECTOR *ad,
			MX_RADICON_HELIOS **radicon_helios,
			MX_PLEORA_IPORT_VINPUT **pleora_iport_vinput,
			MX_PLEORA_IPORT **pleora_iport,
			const char *calling_fname )
{
	static const char fname[] = "mxd_radicon_helios_get_pointers()";

	MX_RADICON_HELIOS *radicon_helios_ptr;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput_ptr;
	MX_PLEORA_IPORT *pleora_iport_ptr;
	MX_RECORD *vinput_record, *iport_record;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	radicon_helios_ptr = (MX_RADICON_HELIOS *)
					ad->record->record_type_struct;

	if ( radicon_helios_ptr == (MX_RADICON_HELIOS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RADICON_HELIOS pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}

	if ( radicon_helios != (MX_RADICON_HELIOS **) NULL ) {
		*radicon_helios = radicon_helios_ptr;
	}

	vinput_record = radicon_helios_ptr->video_input_record;

	if ( vinput_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The video input record pointer for record '%s' is NULL.",
			ad->record->name );
	}

	pleora_iport_vinput_ptr = (MX_PLEORA_IPORT_VINPUT *)
					vinput_record->record_type_struct;

	if ( pleora_iport_vinput_ptr == (MX_PLEORA_IPORT_VINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PLEORA_IPORT_VINPUT pointer for record '%s' "
		"used by record '%s' is NULL.",
			vinput_record->name, ad->record->name );
	}

	if ( pleora_iport_vinput != (MX_PLEORA_IPORT_VINPUT **) NULL ) {
		*pleora_iport_vinput = pleora_iport_vinput_ptr;
	}

	if ( pleora_iport != (MX_PLEORA_IPORT **) NULL ) {
		iport_record = pleora_iport_vinput_ptr->pleora_iport_record;

		if ( iport_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The iport_record pointer for record '%s' used "
			"by record '%s' is NULL.",
				vinput_record->name, ad->record->name );
		}

		*pleora_iport = (MX_PLEORA_IPORT *)
					iport_record->record_type_struct;

		if ( (*pleora_iport) == (MX_PLEORA_IPORT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT pointer for record '%s' "
			"used by records '%s' and '%s' is NULL.",
				iport_record->name,
				vinput_record->name,
				ad->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_radicon_helios_trigger_image_readout(
			MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput )
{
	static const char fname[] =
			"mxd_radicon_helios_trigger_image_readout()";

	/* Tell the iPORT to wait for a frame to arrive. */

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	CyResult cy_result = grabber->Grab( CyChannel(0),
					*pleora_iport_vinput->user_buffer,
					CY_GRABBER_FLAG_NO_WAIT );

	if ( cy_result != CY_RESULT_OK ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to tell the grabber for '%s' to grab a frame.  "
		"cy_result = %d.",
			pleora_iport_vinput->record->name, cy_result );
	}

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s: Grab() started.", fname));
#endif
		
	/* Set the Helios EXSYNC signal to high to trigger image readout. */

	CyString lut_program_high =
		"Q0 = I2\r\n"
		"Q4 = 1\r\n"
		"Q5 = 1\r\n"
		"Q6 = 1\r\n"
		"Q7 = 1\r\n";

	mxi_pleora_iport_send_lookup_table_program( grabber, lut_program_high );

	mx_msleep( 10 );

	/* Return the Helios EXSYNC signal back to low. */

	CyString lut_program_low =
		"Q0 = I2\r\n"
		"Q4 = 1\r\n"
		"Q5 = 1\r\n"
		"Q6 = 1\r\n"
		"Q7 = 0\r\n";

	mxi_pleora_iport_send_lookup_table_program( grabber, lut_program_low );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

#if 0

/* This version reproduces the same picture as ShadoCam or xray2. */

static mx_status_type
mxd_radicon_helios_descramble_10x10( uint16_t **source_2d_array,
					uint16_t **dest_2d_array,
					long *source_framesize,
					long *dest_framesize )
{
	static const char fname[] = "mxd_radicon_helios_descramble_10x10()";

	long i_src, j_src, i_dest, j_dest;

	for ( i_src = 0; i_src < 1024; i_src++ ) {
		i_dest = i_src;

		for ( j_src = 0; j_src < 1024; j_src++ ) {
			if ( j_src & 0x1 ) {
				/* Odd numbered pixels. */

				j_dest = 512 + (j_src - 1) / 2;
			} else {
				/* Even numbered pixels. */

				j_dest = j_src / 2;
			}

			dest_2d_array[i_dest][j_dest]
				= source_2d_array[i_src][j_src];
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

#else

/* This version rotates and transforms the image to the correct layout. */

static mx_status_type
mxd_radicon_helios_descramble_10x10( uint16_t **source_2d_array,
					uint16_t **dest_2d_array,
					long *source_framesize,
					long *dest_framesize )
{
	static const char fname[] = "mxd_radicon_helios_descramble_10x10()";

	long i_src, j_src, i_dest, j_dest, i_dest_temp, j_dest_temp;

	for ( i_src = 0; i_src < 1024; i_src++ ) {
		i_dest_temp = i_src;

		j_dest = i_dest_temp;

		for ( j_src = 0; j_src < 1024; j_src++ ) {
			if ( j_src & 0x1 ) {
				/* Odd numbered pixels. */

				j_dest_temp = 512 + (j_src - 1) / 2;
			} else {
				/* Even numbered pixels. */

				j_dest_temp = j_src / 2;
			}

			if ( j_dest_temp < 512 ) {
				i_dest = j_dest_temp + 512;
			} else {
				i_dest = j_dest_temp - 512;
			}

#if MXD_RADICON_HELIOS_BYTESWAP
			dest_2d_array[i_dest][j_dest]
			  = mx_16bit_byteswap( source_2d_array[i_src][j_src] );
#else
			dest_2d_array[i_dest][j_dest]
				= source_2d_array[i_src][j_src];
#endif
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif

/*---*/

/* Every 10th pixel */

static mx_status_type
mxd_radicon_helios_descramble_25x20( uint16_t **source_2d_array,
					uint16_t **dest_2d_array,
					long *source_framesize,
					long *dest_framesize )
{
	static const char fname[] = "mxd_radicon_helios_descramble_25x20()";

	long i_src, j_src, i_dest, j_dest;

	for ( j_dest = 0; j_dest < 512; j_dest++ ) {

		for ( i_dest = 0; i_dest < 1000; i_dest++ ) {
			i_src = 1000 - i_dest;
			j_src = 5110 - 10 * j_dest + 9;

			dest_2d_array[i_dest][j_dest] =
						source_2d_array[i_src][j_src];
		}
		for ( i_dest = 1000; i_dest < 2000; i_dest++ ) {
			i_src = i_dest - 1000;
			j_src = 10 * j_dest;

			dest_2d_array[i_dest][j_dest] =
						source_2d_array[i_src][j_src];
		}
	}

	for ( j_dest = 512; j_dest < 1024; j_dest++ ) {
		for ( i_dest = 0; i_dest < 1000; i_dest++ ) {
			i_src = 1000 - i_dest;
			j_src = 10230 - 10 * j_dest + 7;

			dest_2d_array[i_dest][j_dest] =
						source_2d_array[i_src][j_src];
		}
		for ( i_dest = 1000; i_dest < 2000; i_dest++ ) {
			i_src = i_dest - 1000;
			j_src = 10 * (j_dest - 512) + 2;

			dest_2d_array[i_dest][j_dest] =
						source_2d_array[i_src][j_src];
		}
	}
	for ( j_dest = 1024; j_dest < 1536; j_dest++ ) {
		for ( i_dest = 0; i_dest < 1000; i_dest++ ) {
			i_src = 1000 - i_dest;
			j_src = 15350 - 10 * j_dest + 5;

			dest_2d_array[i_dest][j_dest] =
						source_2d_array[i_src][j_src];
		}
		for ( i_dest = 1000; i_dest < 2000; i_dest++ ) {
			i_src = i_dest - 1000;
			j_src = 10 * (j_dest - 1024) + 4;

			dest_2d_array[i_dest][j_dest] =
						source_2d_array[i_src][j_src];
		}
	}
	for ( j_dest = 1536; j_dest < 2048; j_dest++ ) {
		for ( i_dest = 0; i_dest < 1000; i_dest++ ) {
			i_src = 1000 - i_dest;
			j_src = 20470 - 10 * j_dest + 3;

			dest_2d_array[i_dest][j_dest] =
						source_2d_array[i_src][j_src];
		}
		for ( i_dest = 1000; i_dest < 2000; i_dest++ ) {
			i_src = i_dest - 1000;
			j_src = 10 * (j_dest - 1536) + 6;

			dest_2d_array[i_dest][j_dest] =
						source_2d_array[i_src][j_src];
		}
	}
	for ( j_dest = 2048; j_dest < 2560; j_dest++ ) {
		for ( i_dest = 0; i_dest < 1000; i_dest++ ) {
			i_src = 1000 - i_dest;
			j_src = 25590 - 10 * j_dest + 1;

			dest_2d_array[i_dest][j_dest] =
						source_2d_array[i_src][j_src];
		}
		for ( i_dest = 1000; i_dest < 2000; i_dest++ ) {
			i_src = i_dest - 1000;
			j_src = 10 * (j_dest - 2048) + 8;

			dest_2d_array[i_dest][j_dest] =
						source_2d_array[i_src][j_src];
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_radicon_helios_set_exposure_mode( MX_AREA_DETECTOR *ad,
					MX_RADICON_HELIOS *radicon_helios,
					long exposure_mode )
{
	static const char fname[] = "mxd_radicon_helios_set_exposure_mode()";

	MX_RECORD *pulser_record;
	MX_MBC_GSC_TRIGGER *mbc_gsc_trigger;

	pulser_record = radicon_helios->pulse_generator_record;

	/* If no pulse generator is configured, then just return. */

	if ( pulser_record == (MX_RECORD *) NULL )
		return MX_SUCCESSFUL_RESULT;

	/* For particular kinds of pulse generators, we store the
	 * area detector's exposure mode in the pulse generator's
	 * data structures.
	 */

	switch( pulser_record->mx_type ) {
	case MXT_PGN_MBC_GSC_TRIGGER:
		mbc_gsc_trigger = (MX_MBC_GSC_TRIGGER *)
					pulser_record->record_type_struct;

		if ( mbc_gsc_trigger == (MX_MBC_GSC_TRIGGER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MBC_GSC_TRIGGER pointer for record '%s' "
			"used by area detector '%s' is NULL.",
				pulser_record->name,
				ad->record->name );
				
		}

		mbc_gsc_trigger->area_detector_exposure_mode = exposure_mode;
		break;
	default:
		return MX_SUCCESSFUL_RESULT;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_radicon_helios_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_radicon_helios_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_RADICON_HELIOS *radicon_helios;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	radicon_helios = (MX_RADICON_HELIOS *)
				malloc( sizeof(MX_RADICON_HELIOS) );

	if ( radicon_helios == (MX_RADICON_HELIOS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for an MX_RADICON_HELIOS structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = radicon_helios;
	record->class_specific_function_list =
			&mxd_radicon_helios_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	radicon_helios->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_radicon_helios_finish_record_initialization()";

	MX_AREA_DETECTOR *ad;
	MX_RADICON_HELIOS *radicon_helios = NULL;
	char *dtname;
	mx_status_type mx_status;

	mx_status = mx_area_detector_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_radicon_helios_get_pointers( ad,
					&radicon_helios, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dtname = radicon_helios->detector_type_name;

	if ( mx_strcasecmp("10x10", radicon_helios->detector_type_name) == 0 ) {

		radicon_helios->detector_type = MXT_RADICON_HELIOS_10x10;
	} else
	if ( mx_strcasecmp("25x20", radicon_helios->detector_type_name) == 0 ) {

		radicon_helios->detector_type = MXT_RADICON_HELIOS_25x20;
	} else
	if ( mx_strcasecmp("30x30", radicon_helios->detector_type_name) == 0 ) {

		radicon_helios->detector_type = MXT_RADICON_HELIOS_30x30;
	} else {
		radicon_helios->detector_type = -1;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Unrecognized detector type '%s' requested for area detector '%s'.",
			radicon_helios->detector_type_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* NOTE:
 *
 * The necessary values of the second parameter (y) to calls like this
 *
 *   lut_extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIGx, y );
 *
 * (where CY_GPIP_LUT_PARAM_INPUT_CONFIGx corresponds to Ix) are not
 * documented _anywhere_ in the documentation for the Pleora iPORT.
 * 
 * The _only_ place that they are documented is in one particular dialog
 * of the Coyote configuration program.  Here is how you find that.
 *
 * 1.  Start up the Coyote configuration program.
 *
 * 2.  Press the Detect... button to find the available IP engines.
 *
 * 3.  Select the appropriate engine from the "IP Engine Selection" dialog
 *     that pops up.
 *
 * 4.  Press the Connect button in the lower left of the Coyote window.
 *
 * 5.  Press the Configure... button in the lower right of the Coyote window.
 *
 * 6.  The "Configuration - Advanced" window should pop up.  Make sure that
 *     the "IP Engine" tab is selected.
 *
 * 7.  Press the + button for the "Programmable Logic Controller" entry.
 *
 * 8.  Press the + button for the "Signal Routing Block and Lookup Table" entry.
 *
 * 9.  You will see a table of definitions for the I0 through I7 signals.
 *     Click in the second column of one of those table entries.
 *
 * 10. A list of the possible values for that Ix variable will pop up in
 *     numerical order.  If you want to explicitly see the values, they also
 *     show up in a list in the scrolling window at the bottom of the dialog
 *     listed under "Possible values:".
 *
 * Note that only particular combinations of Ix variables with possible inputs
 * can be done.  You do not have full flexibility over the choice.
 */

MX_EXPORT mx_status_type
mxd_radicon_helios_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_radicon_helios_open()";

	MX_AREA_DETECTOR *ad;
	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	unsigned long ad_flags, mask;
	long dmd, trigger;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_radicon_helios_get_pointers( ad,
					&radicon_helios, &pleora_iport_vinput,
					NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	radicon_helios->arm_signal_present = FALSE;
	radicon_helios->acquisition_in_progress = FALSE;

	/* Set the default file format.
	 *
	 * FIXME - Are both of these necessary?
	 */

	ad->frame_file_format = MXT_IMAGE_FILE_SMV;
	ad->datafile_format = ad->frame_file_format;

	ad->trigger_mode = MXT_IMAGE_EXTERNAL_TRIGGER;

	ad->correction_frames_to_skip = 0;

	/* Do we need automatic saving and/or loading of image frames by MX? */

	ad_flags = ad->area_detector_flags;

	mask = MXF_AD_LOAD_FRAME_AFTER_ACQUISITION
		| MXF_AD_SAVE_FRAME_AFTER_ACQUISITION;

	if ( ad_flags & mask ) {

#if MXD_RADICON_HELIOS_DEBUG
		MX_DEBUG(-2,
	  ("%s: Enabling automatic datafile management.  ad_flags & mask = %lx",
			fname, (ad_flags & mask) ));
#endif
		mx_status =
		    mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*---*/

	ad->bits_per_pixel = 16;
	ad->bytes_per_pixel = 2;
	ad->image_format = MXT_IMAGE_FORMAT_GREY16;

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						MXU_IMAGE_FORMAT_NAME_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->correction_calc_format = ad->image_format;

	switch( radicon_helios->detector_type ) {
	case MXT_RADICON_HELIOS_10x10:
		ad->maximum_framesize[0] = 1024;
		ad->maximum_framesize[1] = 1024;
		break;

	case MXT_RADICON_HELIOS_25x20:
		ad->maximum_framesize[0] = 2560;
		ad->maximum_framesize[1] = 2000;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Unrecognized detector type '%s' requested for area detector '%s'.",
			radicon_helios->detector_type_name, record->name );
		break;
	}

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	/* Update the framesize to match. */

	ad->parameter_type = MXLV_AD_FRAMESIZE;

	mx_status = mxd_radicon_helios_get_parameter( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s: framesize = (%lu, %lu), binsize = (%lu, %lu)",
		fname, ad->framesize[0], ad->framesize[1],
		ad->binsize[0], ad->binsize[1]));
#endif

	/* Initialize area detector parameters. */

	ad->byte_order = (long) mx_native_byteorder();
	ad->header_length = 0;

	mx_status = mx_area_detector_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is this record configured for one-shot mode
	 * with an external pulse generator?
	 */

	if ( strlen( radicon_helios->pulse_generator_record_name ) == 0 ) {
		radicon_helios->pulse_generator_record = NULL;
	} else {
		radicon_helios->pulse_generator_record = mx_get_record( record,
				radicon_helios->pulse_generator_record_name );

		if ( radicon_helios->pulse_generator_record == NULL ) {
			mx_warning( "The requested pulse generator '%s' "
			"was not found.  One-shot mode for detector '%s' "
			"will not be able to directly control "
			"an external trigger.",
				radicon_helios->pulse_generator_record_name,
				record->name );
		}
	}

	/* Initialize this detector to duration mode.
	 *
	 * It will default to waiting for an external trigger, rather than
	 * indirectly generating a trigger itself through a pulse generator.
	 */

	mx_status = mx_area_detector_set_duration_mode( record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the pulse generator to expect 'still' mode exposures. */

	mx_status = mxd_radicon_helios_set_exposure_mode( ad,
					radicon_helios, MXF_AD_STILL_MODE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/***********************************************
	 * Configure the Pleora iPORT PLC to work with *
	 * the Radicon Helios detector.                *
	 ***********************************************/

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s: pleora_iport_vinput = %p",
		fname, pleora_iport_vinput ));

	MX_DEBUG(-2,("%s: pleora_iport_vinput->grabber = %p",
		fname, pleora_iport_vinput->grabber));
#endif

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	if ( grabber == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No grabber has been connected for record '%s'.",
			pleora_iport_vinput->record->name );
	}

	/* Set CY_GRABBER_PARAM_OFFSET_X for this detector type. */

	unsigned long offset_x;

	switch( radicon_helios->detector_type ) {
	case MXT_RADICON_HELIOS_10x10:
		offset_x = 8;
		break;
	case MXT_RADICON_HELIOS_25x20:
		offset_x = 40;
		break;
	case MXT_RADICON_HELIOS_30x30:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"The value of CY_GRABBER_PARAM_OFFSET_X for the 30x30 "
			"Rad-icon detector has not yet been specified." );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Detector type %lu is not supported by the "
			"'%s' driver.", radicon_helios->detector_type,
			mx_get_driver_name( record ) );
		break;
	}

	grabber->SetParameter( CY_GRABBER_PARAM_OFFSET_X, offset_x );

	grabber->SaveConfig();

	/* Configure various device extensions for the Rad-icon detector. */

	CyDevice &device = grabber->GetDevice();

	/* Configure pulse generator 0 to generate a 20 Hz pulse train
	 * that is used to do the CMOS equivalent of continuout clear.
	 */

	CyDeviceExtension *pg_extension = &device.GetExtension(
					CY_DEVICE_EXT_PULSE_GENERATOR );

	/* Pulse generator granularity is expressed in multiples of
	 * 30 nanoseconds.  Here we set the pulse generator granularity
	 * to 33333, which produces a timer quantum of 999990 nanoseconds.
	 * That is the closest we can get to 1 millisecond.
	 *
	 * With this setup, the longest possible width or delay times
	 * are 65.5343 seconds.
	 */

	pg_extension->SetParameter( CY_PULSE_GEN_PARAM_GRANULARITY, 33333 );

	/* Use a 10 millisecond pulse width together with a 40 millisecond
	 * delay to produce a 50 millisecond pulse period.
	 */

	pg_extension->SetParameter( CY_PULSE_GEN_PARAM_WIDTH, 10 );

	pg_extension->SetParameter( CY_PULSE_GEN_PARAM_DELAY, 40 );

	pg_extension->SetParameter( CY_PULSE_GEN_PARAM_PERIODIC, true );

	pg_extension->SaveToDevice();

	/* Get the extension needed for controlling the PLC's
	 * Signal Routing Block and the PLC's Lookup Table.
	 */

	CyDeviceExtension *lut_extension =
				&device.GetExtension( CY_DEVICE_EXT_GPIO_LUT );

	/* NOTE: See the description before the beginning of this _open()
	 * function of how to get the values of the second argument to
	 * SetParameter() from the Coyote program.
	 */

	/* Connect TTL Input 0 to I0. */

	lut_extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG0, 0 );

	/* Connect Camera Frame Valid to I2. */

	lut_extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG2, 4 );

	/* Connect Pulse Generator 0 Output to I7. */

	lut_extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG7, 0 );

	/* Reprogram the PLC to generate a sync pulse for the camera
	 * and to initialize control inputs.
	 *
	 * Initialize the PLC for SCAN mode (Q4=0) and EXSYNC modulated
	 * by TTL_IN0 (aka A0 on I0).
	 */

	CyString lut_program =
                "Q0 = I2\r\n"
                "Q1 = 0\r\n"
                "Q4 = 0\r\n"
                "Q5 = 1\r\n"
                "Q6 = 1\r\n"
                "Q7 = I7 & !I0\r\n";

	/* We do not use mxi_pleora_iport_send_lookup_table_program() here,
	 * since it is probably better to change the I-registers and the
	 * lookup table as a quasi-atomic operation.
	 */

	lut_extension->SetParameter( CY_GPIO_LUT_PARAM_GPIO_LUT_PROGRAM,
							lut_program );

	/* Send the changes to the IP engine. */

	lut_extension->SaveToDevice();

	/* End of the PLC reconfiguration. */

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_radicon_helios_wait_for_external_trigger( MX_AREA_DETECTOR *ad,
				MX_RADICON_HELIOS *radicon_helios,
				MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput )
{
	static const char fname[] =
		"mxd_radicon_helios_wait_for_external_trigger()";

	unsigned long trigger_value;
	mx_bool_type trigger_seen;
	mx_status_type mx_status;

	/* FIXME: This is a poor way of waiting for the external trigger,
	 * since it is guaranteed to have lots of timing jitter.  It should
	 * be replaced by logic in the iPORT PLC.
	 */

	/* Clear any latched trigger signal. */

	mx_status = mx_digital_input_clear(
				radicon_helios->external_trigger_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the external trigger with debouncing. */

	trigger_seen = FALSE;

	while (1) {
		mx_status = mx_digital_input_read(
				radicon_helios->external_trigger_record,
				&trigger_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( trigger_value == 0 ) {
			trigger_seen = FALSE;
		} else
		if ( trigger_value == 1 ) {
			if ( trigger_seen ) {
				/* We have seen the trigger twice, so break
				 * out of the while() loop.
				 */
			
				break;
			} else {
				trigger_seen = TRUE;
			}
		} else {
			return mx_error( MXE_INTERRUPTED, fname,
			"Waiting for the external trigger for '%s' "
			"was interrupted.", ad->record->name );
		}

		mx_msleep(50);
	}

	radicon_helios->arm_signal_present = TRUE;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s: Arm signal present.",fname));
#endif

	/* Stop sending START pulses to EXSYNC (Q7=0). */

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	if ( grabber == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No grabber has been connected for record '%s'.",
			pleora_iport_vinput->record->name );
	}

	CyString lookup_table_program = 
				"Q0 = I2\r\n"
				"Q4 = 1\r\n"
				"Q5 = 1\r\n"
				"Q6 = 1\r\n"
				"Q7 = 0\r\n";

	mxi_pleora_iport_send_lookup_table_program(
					pleora_iport_vinput->grabber,
					lookup_table_program );

#if 1
	mxi_pleora_iport_display_all_parameters( grabber );
#endif

	radicon_helios->acquisition_in_progress = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_arm()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	long exposure_mode;
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
			&radicon_helios, &pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sp = &(ad->sequence_parameters);

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	exposure_mode = MXF_AD_STILL_MODE;

	if ( ( ad->correction_measurement_in_progress )
	  && ( ad->correction_measurement_type == MXFT_AD_DARK_CURRENT_FRAME ) )
	{
		exposure_mode = MXF_AD_DARK_MODE;
	}

	mx_status = mxd_radicon_helios_set_exposure_mode( ad, radicon_helios,
								exposure_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The acceptable combinations of sequence modes and triggering
	 * are limited for this detector, so we must force the detector
	 * to use a supported one.
	 */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		if ( radicon_helios->pulse_generator_record != NULL ) {
			ad->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;

			break;		/* Exit the switch() statement. */
		}

		/* No pulse generator record is available, so we force
		 * the measurement into duration mode.
		 */

		mx_warning( "No pulse generator is configured for "
			"detector '%s', so this measurement will be "
			"forced into duration mode.", ad->record->name );

		sp->sequence_type = MXT_SQ_DURATION;
		sp->num_parameters = 1;
		sp->parameter_array[0] = 1;	/* one pulse */

		/* Fall through to the duration mode case. */

	case MXT_SQ_DURATION:
		ad->trigger_mode = MXT_IMAGE_EXTERNAL_TRIGGER;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Sequence type %ld is not supported for detector '%s'.",
			sp->sequence_type, ad->record->name );
		break;
	}

	/* The Pleora iPORT is unconditionally told to be in external trigger
	 * mode, since (from its point of view) it _will_ be receiving
	 * an external trigger.
	 */

	mx_status = mx_video_input_set_trigger_mode(
					pleora_iport_vinput->record,
					MXT_IMAGE_EXTERNAL_TRIGGER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s: Arming area detector '%s', trigger_mode = %#lx",
		fname, ad->record->name, ad->trigger_mode ));
#endif

	/* Tell the video card to get ready for an incoming frame. */

	mx_status = mx_video_input_arm( radicon_helios->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (ad->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here we are using an external trigger. */

	mx_status = mxd_radicon_helios_wait_for_external_trigger( 
				ad, radicon_helios, pleora_iport_vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_trigger()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	MX_RECORD *pulser_record;
	double exposure_time;
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
			&radicon_helios, &pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	if ( ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, then internal trigger has been requested. */

	if ( sp->sequence_type != MXT_SQ_ONE_SHOT ) {
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"An internal trigger has been requested for detector '%s', "
		"but the detector is not in one-shot mode.  Instead, it is "
		"in mode %ld.", ad->record->name, sp->sequence_type );
	}

	exposure_time = sp->parameter_array[0];

	/* NOTE: The "internal" trigger mode actually uses an external
	 * pulse generator to send the trigger signal to the detector.
	 * However, we call this an "internal" trigger, since we directly
	 * control when the trigger is sent.
	 */

	pulser_record = radicon_helios->pulse_generator_record;

	if ( pulser_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"An internal trigger has been requested for detector '%s', "
		"but no pulse generator has been configured for it.",
			ad->record->name );
	}

	/* Configure the pulse generator for this measurement. */

	mx_status = mx_pulse_generator_set_mode( pulser_record,
						MXF_PGN_SQUARE_WAVE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_num_pulses( pulser_record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_pulse_delay( pulser_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_pulse_period( pulser_record,
							2.0 * exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_pulse_width( pulser_record,
							exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now start the pulse generator. */

	mx_status = mx_pulse_generator_start( pulser_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the pulse generator's pulse to arrive. */

	mx_status = mxd_radicon_helios_wait_for_external_trigger( 
				ad, radicon_helios, pleora_iport_vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_abort()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
					&radicon_helios, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	radicon_helios->arm_signal_present = FALSE;
	radicon_helios->acquisition_in_progress = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_get_extended_status()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	long last_frame_number, total_num_frames;
	unsigned long trigger_value, status_flags;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
			&radicon_helios, &pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* If an arm signal was present, check to see if it is still present. */

	if ( radicon_helios->arm_signal_present ) {
		mx_status = mx_digital_input_read(
				radicon_helios->external_trigger_record,
				&trigger_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( trigger_value == 1 ) {
			/* The trigger signal has not yet ended. */

			ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;

			return MX_SUCCESSFUL_RESULT;
		} else
		if ( trigger_value == 0 ) {
			/* The trigger signal has ended. */

			radicon_helios->arm_signal_present = FALSE;

			/* Start the Grab() and send the EXSYNC pulse
			 * to trigger the readout.
			 */

			mxd_radicon_helios_trigger_image_readout(
						pleora_iport_vinput );
		} else {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unexpected value (%lu) returned from "
			"digital input '%s'.", trigger_value,
			radicon_helios->external_trigger_record->name );
		}
	}

	/* Ask the video board for its current status.
	 * For the Pleora board,
	 */

	mx_status = mx_video_input_get_extended_status(
					radicon_helios->video_input_record,
					&last_frame_number,
					&total_num_frames,
					&status_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->last_frame_number = last_frame_number;

	ad->total_num_frames = total_num_frames;

	ad->status = 0;

	if ( status_flags & MXSF_VIN_IS_BUSY ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

	/* Ask the pulse generator (if present) for its current status. */

	if ( radicon_helios->pulse_generator_record != NULL ) {
		mx_status = mx_pulse_generator_is_busy(
					radicon_helios->pulse_generator_record,
					&busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy ) {
			ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
		}
	}

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s: detector '%s' status = %#lx, "
		"total_num_frames = %ld, last_frame_number = %ld",
			fname, ad->record->name,
			(unsigned long) ad->status,
			ad->total_num_frames,
			ad->last_frame_number ));

	MX_DEBUG(-2,("%s: acquisition_in_progress = %d",
		fname, radicon_helios->acquisition_in_progress));
#endif

	if ( ( ad->status & MXSF_AD_IS_BUSY ) == 0 ) {
		mx_status = mxd_radicon_helios_set_exposure_mode( ad,
					radicon_helios, MXF_AD_STILL_MODE );
	}

	if ( radicon_helios->acquisition_in_progress ) {
		if ( (ad->status & MXSF_AD_ACQUISITION_IN_PROGRESS) == 0 ) {

			radicon_helios->acquisition_in_progress = FALSE;

			CyString lut_program =
				"Q0 = I2\r\n"
				"Q4 = 0\r\n"
				"Q5 = 1\r\n"
				"Q6 = 1\r\n"
				"Q7 = I7 & !I0\r\n";

			mxi_pleora_iport_send_lookup_table_program(
					pleora_iport_vinput->grabber,
					lut_program );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_readout_frame()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_VIDEO_INPUT *vinput = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	uint16_t **source_2d_array, **dest_2d_array;
	size_t element_size[2];
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
			&radicon_helios, &pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s: Started storing a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	vinput = (MX_VIDEO_INPUT *)
		pleora_iport_vinput->record->record_class_struct;

#if MXD_RADICON_HELIOS_DEBUG

	CyUserBuffer *user_buffer = pleora_iport_vinput->user_buffer;

	MX_DEBUG(-2,("%s: vinput->frame->image_data = %p",
		fname, vinput->frame->image_data));

	MX_DEBUG(-2,("%s: CyUserBuffer::GetBuffer() = %p",
		fname, user_buffer->GetBuffer() ));

	MX_DEBUG(-2,("%s: CyUserBuffer::GetBufferSize() = %lu",
		fname, user_buffer->GetBufferSize() ));
#endif


	/* Descramble the video card image to the area detector's buffer. */

	/* Create two dimensional overlay arrays for the source and
	 * destination image frames.
	 */

	element_size[0] = sizeof(uint16_t);
	element_size[1] = sizeof(uint16_t *);

	mx_status = mx_array_add_overlay( vinput->frame->image_data,
					2, vinput->framesize, element_size,
					(void **) &source_2d_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_array_add_overlay( ad->image_frame->image_data,
					2, ad->framesize, element_size,
					(void **) &dest_2d_array );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_array_free_overlay( source_2d_array, 2 );
		return mx_status;
	}

#if 1
	int i, row, offset;
	uint16_t value;

	for ( row = 0; row < 4; row++ ) {
		fprintf( stderr, "\nVinput frame (row %d ): ", row );

		for ( i = 0; i < 10; i++ ) {
			offset = i + row * vinput->framesize[0];

			value = ((uint16_t *)
				vinput->frame->image_data)[offset];

			fprintf( stderr, "%hu ", (unsigned short) value );
		}
	}

	for ( row = 0; row < 4; row++ ) {
		fprintf( stderr, "\n2d array( row %d ): ", row );

		for ( i = 0; i < 10; i++ ) {
			value = source_2d_array[row][i];

			fprintf( stderr, "%hu ", (unsigned short) value );
		}
	}

	fprintf( stderr, "\n" );
#endif

	switch( radicon_helios->detector_type ) {
	case MXT_RADICON_HELIOS_10x10:

		mx_status = mxd_radicon_helios_descramble_10x10(
						source_2d_array,
						dest_2d_array,
						vinput->framesize,
						ad->framesize );
		break;
	case MXT_RADICON_HELIOS_25x20:
		mx_status = mxd_radicon_helios_descramble_25x20(
						source_2d_array,
						dest_2d_array,
						vinput->framesize,
						ad->framesize );
		break;
	case MXT_RADICON_HELIOS_30x30:
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Descrambling by record '%s' for detector type '%s' "
		"is not yet implemented.",
			ad->record->name, radicon_helios->detector_type_name );
		break;
	}

	mx_array_free_overlay( source_2d_array, 2 );
	mx_array_free_overlay( dest_2d_array, 2 );

#if MXD_RADICON_HELIOS_DEBUG
	{
		unsigned long i;
		uint16_t *image_data;

		image_data = (uint16_t *) ad->image_frame->image_data;

		MX_DEBUG(-2,("%s: ad->image_frame->image_data = %p",
			fname, ad->image_frame->image_data));

		fprintf( stderr, "Image data: " );

		for ( i = 0; i < 30; i++ ) {
			fprintf( stderr, "%lu ", image_data[i] );
		}

		fprintf( stderr, "...\n" );
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_correct_frame()";

	MX_RADICON_HELIOS *radicon_helios;
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
					&radicon_helios, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_transfer_frame()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	size_t dirname_length;
	char remote_radicon_helios_filename[(2*MXU_FILENAME_LENGTH) + 20];
	char local_radicon_helios_filename[(2*MXU_FILENAME_LENGTH) + 20];
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
					&radicon_helios, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* We can only handle transferring the image frame. */

#if 0
	if ( ad->transfer_frame != MXFT_AD_IMAGE_FRAME ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Transferring frame type %lu is not supported for "
		"MBC NOIR detector '%s'.  Only the image frame (type 0) "
		"is supported.",
			ad->transfer_frame, ad->record->name );
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_get_parameter()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	MX_RECORD *video_input_record;
	long vinput_horiz_framesize, vinput_vert_framesize;
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
					&radicon_helios, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif

	sp = &(ad->sequence_parameters);

	video_input_record = radicon_helios->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		mx_status = mx_video_input_get_maximum_frame_number(
			video_input_record, &(ad->maximum_frame_number) );
		break;

	case MXLV_AD_FRAMESIZE:
		mx_status = mx_video_input_get_framesize( video_input_record,
			&vinput_horiz_framesize, &vinput_vert_framesize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( radicon_helios->detector_type ) {
		case MXT_RADICON_HELIOS_25x20:
			ad->framesize[0] = vinput_horiz_framesize / 2L;
			ad->framesize[1] = vinput_vert_framesize  * 2L;
			break;
		default:
			ad->framesize[0] = vinput_horiz_framesize;
			ad->framesize[1] = vinput_vert_framesize;
			break;
		}
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_video_input_get_image_format( video_input_record,
						&(ad->image_format) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		mx_status = mx_video_input_get_bytes_per_frame(
				video_input_record, &(ad->bytes_per_frame) );
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		mx_status = mx_video_input_get_bytes_per_pixel(
				video_input_record, &(ad->bytes_per_pixel) );
		break;

	case MXLV_AD_DETECTOR_READOUT_TIME:
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		/* Just report back values that were previously written here. */

		break;

	case MXLV_AD_TRIGGER_MODE:
		break;

	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_set_parameter()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	MX_RECORD *video_input_record;
	long vinput_horiz_framesize, vinput_vert_framesize;
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
			&radicon_helios, &pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sp = &(ad->sequence_parameters);

	video_input_record = radicon_helios->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		switch( radicon_helios->detector_type ) {
		case MXT_RADICON_HELIOS_25x20:
			vinput_horiz_framesize = ad->framesize[0] * 2L;
			vinput_vert_framesize  = ad->framesize[1] / 2L;
			break;
		default:
			vinput_horiz_framesize = ad->framesize[0];
			vinput_vert_framesize  = ad->framesize[1];
			break;
		}

		mx_status = mx_video_input_set_framesize( video_input_record,
					ad->framesize[0], ad->framesize[1] );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:

		/* Wait until MXLV_AD_SEQUENCE_PARAMETER_ARRAY is set
		 * before sending any commands to the video card.
		 */
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		switch( sp->sequence_type ) {
		case MXT_SQ_DURATION:
			/* We rely on the external trigger to remain high
			 * for the duration of the exposure.
			 */
			break;

		case MXT_SQ_ONE_SHOT:
			/* We can only use one-shot mode if a pulse generator
			 * has been configured to generate a gate pulse for
			 * the detector.
			 */

			if ( radicon_helios->pulse_generator_record == NULL ) {
				mx_warning( "No pulse generator has been "
				"configured for detector '%s', so we are "
				"switching to duration mode.",
					ad->record->name );

				sp->sequence_type = MXT_SQ_DURATION;
				sp->num_parameters = 1;
				sp->parameter_array[0] = 1;
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Sequence type %ld is not supported for detector '%s'.",
				sp->sequence_type, ad->record->name );
			break;
		}

		break; 

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_set_trigger_mode( video_input_record,
							ad->trigger_mode );
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Changing parameter '%s' for area detector '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
		break;

	default:
		mx_status = mx_area_detector_default_set_parameter_handler(ad);
		break;
	}

	return mx_status;
}


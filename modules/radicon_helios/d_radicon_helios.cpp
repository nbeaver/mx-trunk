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

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "mx_digital_input.h"
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
	mxd_radicon_helios_set_parameter,
	mx_area_detector_default_measure_correction
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

static mx_status_type
mxd_radicon_helios_trigger_image_readout( MX_RADICON_HELIOS *radicon_helios,
					MX_VIDEO_INPUT *vinput,
				MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput )
{
	static const char fname[] =
			"mxd_radicon_helios_trigger_image_readout()";

	/* Tell the grabber to wait for an incoming frame. */

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	if ( grabber == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No grabber has been connected for record '%s'.",
			pleora_iport_vinput->record->name );
	}

	unsigned char *image_data = (unsigned char *) vinput->frame->image_data;

	unsigned long image_length = vinput->frame->image_length;

	CyResult cy_result = grabber->Grab( CyChannel(0),
					*pleora_iport_vinput->user_buffer,
					CY_GRABBER_FLAG_NO_WAIT );

	if ( cy_result != CY_RESULT_OK ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to tell the grabber for '%s' to grab a frame.  "
		"cy_result = %d.", vinput->record->name, cy_result );
	}

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
	mx_status_type mx_status;

	mx_status = mx_area_detector_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_radicon_helios_get_pointers( ad,
					&radicon_helios, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

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

	radicon_helios->acquisition_in_progress = FALSE;

	/* Set the default file format.
	 *
	 * FIXME - Are both of these necessary?
	 */

	ad->frame_file_format = MXT_IMAGE_FILE_SMV;
	ad->datafile_format = ad->frame_file_format;

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

	ad->maximum_framesize[0] = 2560;
	ad->maximum_framesize[1] = 2000;

	/* Update the framesize and binsize to match. */

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

	/* Configure the Pleora iPORT PLC to work with
	 * the Radicon Helios detector.
	 */

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

	CyDevice &device = grabber->GetDevice();

	/* Get the extension needed for controlling the PLC's
	 * Signal Routing Block and the PLC's Lookup Table.
	 */

	CyDeviceExtension *extension =
				&device.GetExtension( CY_DEVICE_EXT_GPIO_LUT );

	/* FIXME: I do not yet know where the values come from for the
	 * second argument of the calls to SetParameter().
	 */

	/* Connect TTL Input 0 to I0. */

	extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG0, 0 );

	/* Connect Camera Frame Valid to I2. */

	extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG2, 4 );

	/* Connect Pulse Generator 0 Output to I7. */

	extension->SetParameter( CY_GPIO_LUT_PARAM_INPUT_CONFIG7, 0 );

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

	extension->SetParameter( CY_GPIO_LUT_PARAM_GPIO_LUT_PROGRAM,
							lut_program );

	/* Send the changes to the IP engine. */

	extension->SaveToDevice();

	/* End of the PLC reconfiguration. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_arm()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_VIDEO_INPUT *vinput = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	long trigger_mode;
	unsigned long trigger_value;
	mx_bool_type trigger_seen;
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
	/* FIXME: Currently we only support external trigger, so
	 * we force that on.
	 */

	mx_status = mx_area_detector_set_trigger_mode( ad->record,
						MXT_IMAGE_EXTERNAL_TRIGGER );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Are we using external trigger or internal trigger? */

	mx_status = mx_video_input_get_trigger_mode(
					radicon_helios->video_input_record,
					&trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s: Arming area detector '%s', trigger_mode = %#lx",
		fname, ad->record->name, trigger_mode ));
#endif

	/* Tell the video card to get ready for an incoming frame. */

	mx_status = mx_video_input_arm( radicon_helios->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	radicon_helios->acquisition_in_progress = FALSE;

	if ( (trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here we are using an external trigger. */

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

	/* Stop sending START pulses to EXSYNC (Q7=0). */

	CyString lookup_table_program = 
				"Q0 = I2\r\n"
				"Q4 = 1\r\n"
				"Q5 = 1\r\n"
				"Q6 = 1\r\n"
				"Q7 = 0\r\n";

	mxi_pleora_iport_send_lookup_table_program(
					pleora_iport_vinput->grabber,
					lookup_table_program );

	/* Wait for the external trigger to go away. */

	while (1) {
		mx_status = mx_digital_input_read(
				radicon_helios->external_trigger_record,
				&trigger_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( trigger_value == 0 ) {
			/* The trigger signal has ended, so break out
			 * of the while loop.
			 */

			break;
		} else
		if ( trigger_value == 1 ) {
			/* The trigger signal has not yet ended. */
		} else {
			return mx_error( MXE_INTERRUPTED, fname,
			"Waiting for the external trigger for '%s' "
			"was interrupted.", ad->record->name );
		}

		mx_msleep(50);
	}

	vinput = (MX_VIDEO_INPUT *)
			pleora_iport_vinput->record->record_class_struct;

	mx_status = mxd_radicon_helios_trigger_image_readout(
				radicon_helios, vinput, pleora_iport_vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	radicon_helios->acquisition_in_progress = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_trigger()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_VIDEO_INPUT *vinput = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	long trigger_mode;
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

	/* If the video card is in internal trigger mode, then we
	 * need to send a software trigger.
	 */

	mx_status = mx_video_input_get_trigger_mode(
					radicon_helios->video_input_record,
					&trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, then the internal trigger has been enabled. */

	/* FIXME: Internal trigger does not work yet. */

	vinput = (MX_VIDEO_INPUT *)
			pleora_iport_vinput->record->record_class_struct;

	mx_status = mxd_radicon_helios_trigger_image_readout(
				radicon_helios, vinput, pleora_iport_vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	radicon_helios->acquisition_in_progress = TRUE;

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
	radicon_helios->acquisition_in_progress = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_get_extended_status()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	long last_frame_number;
	long total_num_frames;
	unsigned long status_flags;
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
					&radicon_helios, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	/* Ask the video board for its current status. */

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

#if MXD_RADICON_HELIOS_DEBUG
	MX_DEBUG(-2,("%s: detector '%s' status = %#lx, "
		"total_num_frames = %ld, last_frame_number = %ld",
			fname, ad->record->name,
			(unsigned long) ad->status,
			ad->total_num_frames,
			ad->last_frame_number ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_readout_frame()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
					&radicon_helios, NULL, NULL, fname );

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

	{
		int i;
		uint16_t *image_data;

		image_data = (uint16_t *) ad->image_frame->image_data;

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

	if ( ad->transfer_frame != MXFT_AD_IMAGE_FRAME ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Transferring frame type %lu is not supported for "
		"MBC NOIR detector '%s'.  Only the image frame (type 0) "
		"is supported.",
			ad->transfer_frame, ad->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_helios_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_helios_get_parameter()";

	MX_RADICON_HELIOS *radicon_helios = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	MX_RECORD *video_input_record;
	double exposure_time;
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
				&(ad->framesize[0]), &(ad->framesize[1]) );
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
		sp->sequence_type = MXT_SQ_ONE_SHOT;
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		sp->num_parameters = 1;
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		exposure_time = 0;	/* FIXME */

		sp->parameter_array[0] = exposure_time;
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_get_trigger_mode(
				video_input_record, &(ad->trigger_mode) );
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
	long num_frames;
	double exposure_time, frame_time, delay_time;
	mx_status_type mx_status;

	mx_status = mxd_radicon_helios_get_pointers( ad,
			&radicon_helios, &pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sp = &(ad->sequence_parameters);

	video_input_record = radicon_helios->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
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
		case MXT_SQ_ONE_SHOT:
			num_frames = 1;
			exposure_time = sp->parameter_array[0];
			frame_time = exposure_time;
			break;

		case MXT_SQ_MULTIFRAME:
			num_frames = sp->parameter_array[0];
			exposure_time = sp->parameter_array[1];
			frame_time = sp->parameter_array[2];
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Sequence type %ld is not supported for detector '%s'.",
				sp->sequence_type, ad->record->name );
			break;
		}

		delay_time = frame_time - exposure_time;

		/* The timing of imaging sequences is controlled by
		 * pulse generator 0 of the Pleora iPORT grabber.
		 */

		{
			__int64 exposure_ticks, delay_ticks;
			__int64 pg_granularity;
			double timer_quantum;
			bool periodic;
			
			CyGrabber *grabber = pleora_iport_vinput->grabber;
	
			CyDevice &device = grabber->GetDevice();
	
			CyDeviceExtension *extension =
				&device.GetExtension(
					CY_DEVICE_EXT_PULSE_GENERATOR );

			/* Pulse generator granularity is expressed in
			 * multiples of 30 nanoseconds.  Here we set
			 * the pulse generator granularity to 33333,
			 * which produces a timer quantum of 999990
			 * nanoseconds.  That is the closest we can
			 * get to 1 millisecond.
			 *
			 * With this setup, the longest possible exposure
			 * or delay times are 65.5343 seconds.
			 */

			pg_granularity = 33333;

			timer_quantum = 30.0e-9 * (double) pg_granularity;

			extension->SetParameter(
			    CY_PULSE_GEN_PARAM_GRANULARITY, pg_granularity );

			exposure_ticks =
				mx_round( exposure_time / timer_quantum );

			delay_ticks = mx_round( delay_time / timer_quantum );

			if ( exposure_ticks > 65535 ) {
				exposure_ticks = 65535;
			} else
			if ( exposure_ticks < 1 ) {
				exposure_ticks = 1;
			}

			if ( delay_ticks > 65535 ) {
				delay_ticks = 65535;
			} else
			if ( delay_ticks < 1 ) {
				delay_ticks = 1;
			}

			extension->SetParameter(
				CY_PULSE_GEN_PARAM_WIDTH, exposure_ticks );

			extension->SetParameter(
				CY_PULSE_GEN_PARAM_DELAY, delay_ticks );

			if ( sp->sequence_type == MXT_SQ_ONE_SHOT ) {
				periodic = false;
			} else {
				periodic = true;
			}

			extension->SetParameter(
				CY_PULSE_GEN_PARAM_PERIODIC, periodic );

			extension->SaveToDevice();

#if 1
			{
				unsigned long parameter_array[] = {
					CY_PULSE_GEN_PARAM_DELAY,
					CY_PULSE_GEN_PARAM_FREQUENCY,
					CY_PULSE_GEN_PARAM_GRANULARITY,
					CY_PULSE_GEN_PARAM_PERIOD,
					CY_PULSE_GEN_PARAM_PERIODIC,
					CY_PULSE_GEN_PARAM_TRIGGER_MODE,
					CY_PULSE_GEN_PARAM_WIDTH };
	
				unsigned long num_parameters
					= sizeof(parameter_array)
					/ sizeof(parameter_array[0]);
	
				mxi_pleora_iport_display_parameter_array(
				  extension, num_parameters, parameter_array );
			}
#endif

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


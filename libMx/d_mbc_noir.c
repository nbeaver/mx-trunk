/*
 * Name:    d_mbc_noir.c
 *
 * Purpose: MX driver for the Molecular Biology Consortium's NOIR 1 detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MBC_NOIR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_epics.h"
#include "d_mbc_noir.h"

#if 1
#include "mx_key.h"
#endif

/*---*/

MX_RECORD_FUNCTION_LIST mxd_mbc_noir_record_function_list = {
	mxd_mbc_noir_initialize_driver,
	mxd_mbc_noir_create_record_structures,
	mxd_mbc_noir_finish_record_initialization,
	NULL,
	NULL,
	mxd_mbc_noir_open
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_mbc_noir_ad_function_list = {
	NULL,
	mxd_mbc_noir_trigger,
	mxd_mbc_noir_abort,
	mxd_mbc_noir_abort,
	NULL,
	NULL,
	NULL,
	mxd_mbc_noir_get_extended_status,
	mxd_mbc_noir_readout_frame,
	mxd_mbc_noir_correct_frame,
	mxd_mbc_noir_transfer_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_mbc_noir_get_parameter,
	mxd_mbc_noir_set_parameter,
	mx_area_detector_default_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_mbc_noir_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_MBC_NOIR_STANDARD_FIELDS
};

long mxd_mbc_noir_num_record_fields
		= sizeof( mxd_mbc_noir_record_field_defaults )
			/ sizeof( mxd_mbc_noir_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mbc_noir_rfield_def_ptr
			= &mxd_mbc_noir_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_mbc_noir_get_pointers( MX_AREA_DETECTOR *ad,
			MX_MBC_NOIR **mbc_noir,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mbc_noir_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (mbc_noir == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MBC_NOIR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mbc_noir = (MX_MBC_NOIR *) ad->record->record_type_struct;

	if ( *mbc_noir == (MX_MBC_NOIR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_MBC_NOIR pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_mbc_noir_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_mbc_noir_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_MBC_NOIR *mbc_noir;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	mbc_noir = (MX_MBC_NOIR *)
				malloc( sizeof(MX_MBC_NOIR) );

	if ( mbc_noir == (MX_MBC_NOIR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for an MX_MBC_NOIR structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = mbc_noir;
	record->class_specific_function_list = &mxd_mbc_noir_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	mbc_noir->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_mbc_noir_finish_record_initialization()";

	MX_AREA_DETECTOR *ad;
	MX_MBC_NOIR *mbc_noir = NULL;
	mx_status_type mx_status;

	mx_status = mx_area_detector_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_mbc_noir_get_pointers( ad, &mbc_noir, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(mbc_noir->collect_angle_pv),
			"%sCOLLECT:angle", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->collect_delta_pv),
			"%sCOLLECT:delta", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->collect_distance_pv),
			"%sCOLLECT:distance", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->collect_energy_pv),
			"%sCOLLECT:energy", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->collect_expose_pv),
			"%sCOLLECT:expose", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->collect_state_msg_pv),
			"%sCOLLECT:stateMsg", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_command_pv),
			"%sNOIR:command", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_command_trig_pv),
			"%sNOIR:commandTrig", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_dir_pv),
			"%sNOIR:dir", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_dmd_pv),
			"%sNOIR:dmd", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_err_msg_pv),
			"%sNOIR:errMsg", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_file_pv),
			"%sNOIR:file", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_process_time_pv),
			"%sNOIR:processTime", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_response_pv),
			"%sNOIR:response", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_response_trig_pv),
			"%sNOIR:responseTrig", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_snap_time_pv),
			"%sNOIR:snapTime", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_state_msg_pv),
			"%sNOIR:stateMsg", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_store_msg_pv),
			"%sNOIR:storeMsg", mbc_noir->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir->noir_store_time_pv),
			"%sNOIR:storeTime", mbc_noir->epics_prefix );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_mbc_noir_open()";

	MX_AREA_DETECTOR *ad;
	MX_MBC_NOIR *mbc_noir = NULL;
	unsigned long ad_flags, mask;
	char epics_string[ MXU_EPICS_STRING_LENGTH+1 ];
	long dmd, trigger;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_mbc_noir_get_pointers( ad, &mbc_noir, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));

	mx_epics_set_debug_flag( TRUE );
#endif

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	mbc_noir->acquisition_in_progress = FALSE;

	/* Detect the presence of the detector by asking it for
	 * its current state.
	 */

	mx_status = mx_caget( &(mbc_noir->noir_state_msg_pv),
			MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
	MX_DEBUG(-2,("%s: NOIR state = '%s'", fname, epics_string));
#endif

	/* Ask for the CCD mode via the 'dmd' PV (Detector MoDe?). */

	mx_status = mx_caget( &(mbc_noir->noir_dmd_pv), MX_CA_LONG, 1, &dmd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
	MX_DEBUG(-2,("%s: NOIR dmd = %ld", fname, dmd ));
#endif

	/* If noir_dmd_pv has a value of 0, then the detector computer
	 * may just have been powered on.  If so, then we must set the
	 * dmd value to 3 and then initialize the detector.
	 */

	if ( dmd == 0 ) {
		/* MBC always uses noir_dmd_pv set to 3. */

		dmd = 3;

		mx_status = mx_caput( &(mbc_noir->noir_dmd_pv),
					MX_CA_LONG, 1, &dmd );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 0
		/* Set command select to 'position'.
		 *
		 * FIXME: What does this do exactly?
		 */

		strlcpy( epics_string, "position", sizeof(epics_string) );

		mx_status = mx_caput( &(mbc_noir->collect_command_select_pv),
					MX_CA_STRING, 1, epics_string );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif

		/* Tell the detector to initialize itself. */

		strlcpy( epics_string, "init", sizeof(epics_string) );

		mx_status = mx_caput( &(mbc_noir->noir_command_pv),
					MX_CA_STRING, 1, epics_string );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		trigger = 1;

		mx_status = mx_caput_nowait( &(mbc_noir->noir_command_trig_pv),
						MX_CA_LONG, 1, &trigger );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Wait for the initialization to finish. */

		while (1) {
			mx_msleep(100);

			mx_status = mx_caget( &(mbc_noir->noir_state_msg_pv),
					MX_CA_STRING, 1, epics_string );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( strcmp( epics_string, "waitCommand" ) == 0 ) {
				break;	/* Exit the while() loop. */
			}
		}

		/* Did the initialization finish successfully? */

		mx_status = mx_caget( &(mbc_noir->noir_err_msg_pv),
					MX_CA_STRING, 1, epics_string );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( strcmp( epics_string, "SUCCESS_INIT_DLL" ) != 0 ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"Initializing NOIR detector '%s' failed with an "
			"error message of '%s'.",
				record->name, epics_string );
		}
	}

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

#if MXD_MBC_NOIR_DEBUG
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

	ad->maximum_framesize[0] = 2048;
	ad->maximum_framesize[1] = 2048;

	/* Update the framesize and binsize to match. */

	ad->parameter_type = MXLV_AD_FRAMESIZE;

	mx_status = mxd_mbc_noir_get_parameter( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
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

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mbc_noir_trigger()";

	MX_MBC_NOIR *mbc_noir = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	int32_t trigger;
	char epics_string[ MXU_EPICS_STRING_LENGTH+1 ];
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_get_pointers( ad, &mbc_noir, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	strlcpy( epics_string, "snap", sizeof(epics_string) );

	mx_status = mx_caput( &(mbc_noir->noir_command_pv),
				MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trigger = 1;

	mx_status = mx_caput_nowait( &(mbc_noir->noir_command_trig_pv),
					MX_CA_LONG, 1, &trigger );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mbc_noir->acquisition_in_progress = TRUE;

#if MXD_MBC_NOIR_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mbc_noir_abort()";

	MX_MBC_NOIR *mbc_noir = NULL;
	int32_t trigger;
	char epics_string[ MXU_EPICS_STRING_LENGTH+1 ];
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_get_pointers( ad, &mbc_noir, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mbc_noir->acquisition_in_progress = FALSE;

	strlcpy( epics_string, "init", sizeof(epics_string) );

	mx_status = mx_caput( &(mbc_noir->noir_command_pv),
				MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trigger = 1;

	mx_status = mx_caput( &(mbc_noir->noir_command_trig_pv),
					MX_CA_LONG, 1, &trigger );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mbc_noir_get_extended_status()";

	MX_MBC_NOIR *mbc_noir = NULL;
	char epics_string[ MXU_EPICS_STRING_LENGTH+1 ];
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_get_pointers( ad, &mbc_noir, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	ad->status = 0;

	mx_status = mx_caget( &(mbc_noir->collect_state_msg_pv),
					MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( epics_string, "waitCommand" ) != 0 ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

	mx_status = mx_caget( &(mbc_noir->noir_state_msg_pv),
					MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( epics_string, "waitCommand" ) != 0 ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

	if ( mbc_noir->acquisition_in_progress ) {
		if ( ad->status & MXSF_AD_ACQUISITION_IN_PROGRESS ) {
			ad->last_frame_number = -1;
		} else {
			mbc_noir->acquisition_in_progress = FALSE;

			ad->last_frame_number = 0;
			ad->total_num_frames++;
		}
	}

#if MXD_MBC_NOIR_DEBUG
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
mxd_mbc_noir_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mbc_noir_readout_frame()";

	MX_MBC_NOIR *mbc_noir = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	int32_t trigger;
	char epics_string[ MXU_EPICS_STRING_LENGTH+1 ];
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_get_pointers( ad, &mbc_noir, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	strlcpy( epics_string, "store", sizeof(epics_string) );

	mx_status = mx_caput( &(mbc_noir->noir_command_pv),
				MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trigger = 1;

	mx_status = mx_caput_nowait( &(mbc_noir->noir_command_trig_pv),
					MX_CA_LONG, 1, &trigger );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
	MX_DEBUG(-2,("%s: Started storing a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mbc_noir_correct_frame()";

	MX_MBC_NOIR *mbc_noir;
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_get_pointers( ad, &mbc_noir, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* The MBC NOIR 1 detector automatically handles correction. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mbc_noir_transfer_frame()";

	MX_MBC_NOIR *mbc_noir = NULL;
	size_t dirname_length;
	char remote_mbc_noir_filename[(2*MXU_FILENAME_LENGTH) + 20];
	char local_mbc_noir_filename[(2*MXU_FILENAME_LENGTH) + 20];
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_get_pointers( ad, &mbc_noir, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG_FRAME_CORRECTION
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

	/* Fetch the current MBC NOIR directory and file name. */

	mx_status = mx_caget( &(mbc_noir->noir_dir_pv),
			MX_CA_STRING, 1, ad->datafile_directory );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(mbc_noir->noir_file_pv),
			MX_CA_STRING, 1, ad->datafile_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We attempt to read in the most recently saved MBC NOIR image. */

	dirname_length = strlen( ad->datafile_directory );

	if ( dirname_length > 0 ) {
		snprintf( remote_mbc_noir_filename,
			sizeof(remote_mbc_noir_filename),
			"%s/%s", ad->datafile_directory, ad->datafile_name );
	} else {
		snprintf( remote_mbc_noir_filename,
			sizeof(remote_mbc_noir_filename),
			"%s", ad->datafile_name );
	}

#if MXD_MBC_NOIR_SERVER_SOCKET_DEBUG
	MX_DEBUG(-2,("%s: Remote MBC NOIR filename = '%s'",
		fname, remote_mbc_noir_filename));
#endif
	if ( strlen( remote_mbc_noir_filename ) == 0 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MBC NOIR 'datafile_directory' and 'datafile_name' "
		"fields have not been initialized for detector '%s', so "
		"we cannot read in the most recently acquired MBC NOIR image.",
			ad->record->name );
	}

	/* Usually, this driver is not running on the same computer as 
	 * the one running the NOIR EPICS server.  If so, then we can only
	 * load the MBC NOIR file if it has been exported to us via somthing
	 * like NFS or SMB.  If so, the local filename is probably different
	 * from the filename on the 'mbc_noir' server computer.  We handle
	 * this by stripping off the remote prefix and then prepending that
	 * with the local prefix.
	 */

	mx_status = mx_change_filename_prefix( remote_mbc_noir_filename,
					mbc_noir->remote_filename_prefix,
					mbc_noir->local_filename_prefix,
					local_mbc_noir_filename,
					sizeof(local_mbc_noir_filename) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now we can read in the MBC NOIR file. */

	mx_status = mx_image_read_smv_file( &(ad->image_frame),
					local_mbc_noir_filename );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mbc_noir_get_parameter()";

	MX_MBC_NOIR *mbc_noir = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_get_pointers( ad, &mbc_noir, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
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

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		ad->binsize[0] = 1;
		ad->binsize[1] = 1;

		ad->framesize[0] = ad->maximum_framesize[0];
		ad->framesize[1] = ad->maximum_framesize[1];
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		ad->bytes_per_frame = mx_round( ad->bytes_per_pixel
			* ad->framesize[0] * ad->framesize[1] );
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_image_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
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
		mx_status = mx_caget( &(mbc_noir->collect_expose_pv),
					MX_CA_DOUBLE, 1, &exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		sp->parameter_array[0] = exposure_time;
		break;

	case MXLV_AD_TRIGGER_MODE:
		ad->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;
		break;

	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mbc_noir_set_parameter()";

	MX_MBC_NOIR *mbc_noir = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_get_pointers( ad, &mbc_noir, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_DEBUG
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

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		ad->binsize[0] = 1;
		ad->binsize[1] = 1;

		ad->framesize[0] = ad->maximum_framesize[0];
		ad->framesize[1] = ad->maximum_framesize[1];
		break;

	case MXLV_AD_SEQUENCE_TYPE:
		if ( sp->sequence_type != MXT_SQ_ONE_SHOT ) {
			mx_status = mx_error( MXE_UNSUPPORTED, fname,
			"Sequence type %ld is not supported for "
			"area detector '%s'.  "
			"Only single frames are supported.",
				sp->sequence_type, ad->record->name );
		}

		sp->sequence_type = MXT_SQ_ONE_SHOT;
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		sp->num_parameters = 1;
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		exposure_time = sp->parameter_array[0];

		mx_status = mx_caput( &(mbc_noir->collect_expose_pv),
					MX_CA_DOUBLE, 1, &exposure_time );
		break; 

	case MXLV_AD_TRIGGER_MODE:
		ad->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;
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

#endif /* HAVE_EPICS */


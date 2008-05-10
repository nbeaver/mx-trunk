/*
 * Name:    d_bluice_area_detector.c
 *
 * Purpose: MX driver for MarCCD remote control.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_BLUICE_AREA_DETECTOR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_bluice.h"
#include "n_bluice_dcss.h"
#include "n_bluice_dhs.h"
#include "n_bluice_dhs_manager.h"
#include "d_bluice_area_detector.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_bluice_area_detector_record_function_list = {
	mxd_bluice_area_detector_initialize_type,
	mxd_bluice_area_detector_create_record_structures,
	mxd_bluice_area_detector_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bluice_area_detector_open
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_bluice_area_detector_function_list = {
	NULL,
	mxd_bluice_area_detector_trigger,
	mxd_bluice_area_detector_stop,
	mxd_bluice_area_detector_stop,
	NULL,
	NULL,
	NULL,
	mxd_bluice_area_detector_get_extended_status,
	mxd_bluice_area_detector_readout_frame,
	mxd_bluice_area_detector_correct_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bluice_area_detector_get_parameter,
	mxd_bluice_area_detector_set_parameter,
	mx_area_detector_default_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_bluice_dcss_area_detector_record_field_defaults[]
= {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_BLUICE_DCSS_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_bluice_dcss_area_detector_num_record_fields
	= sizeof( mxd_bluice_dcss_area_detector_record_field_defaults )
	    / sizeof( mxd_bluice_dcss_area_detector_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_dcss_area_detector_rfield_def_ptr
		= &mxd_bluice_dcss_area_detector_record_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxd_bluice_dhs_area_detector_record_field_defaults[]
= {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_BLUICE_DHS_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_bluice_dhs_area_detector_num_record_fields
	= sizeof( mxd_bluice_dhs_area_detector_record_field_defaults )
	    / sizeof( mxd_bluice_dhs_area_detector_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_dhs_area_detector_rfield_def_ptr
		= &mxd_bluice_dhs_area_detector_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_bluice_area_detector_get_pointers( MX_AREA_DETECTOR *ad,
			MX_BLUICE_AREA_DETECTOR **bluice_area_detector,
			MX_BLUICE_SERVER **bluice_server,
			void **bluice_type_server,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bluice_area_detector_get_pointers()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector_ptr;
	MX_RECORD *bluice_server_record;
	MX_BLUICE_SERVER *bluice_server_ptr;

	/* In this section, we do standard MX ..._get_pointers() logic. */

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( (ad->record) == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_AREA_DETECTOR pointer %p "
		"passed was NULL.", ad );
	}

	bluice_area_detector_ptr = ad->record->record_type_struct;

	if ( bluice_area_detector_ptr == (MX_BLUICE_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_AREA_DETECTOR pointer for record '%s' is NULL.",
			ad->record->name );
	}

	if ( bluice_area_detector != (MX_BLUICE_AREA_DETECTOR **) NULL ) {
		*bluice_area_detector = bluice_area_detector_ptr;
	}

	bluice_server_record = bluice_area_detector_ptr->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' "
		"is NULL.", ad->record->name );
	}

	bluice_server_ptr = bluice_server_record->record_class_struct;

	if ( bluice_server_ptr == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for Blu-Ice server "
		"record '%s' used by record '%s' is NULL.",
			bluice_server_record->name,
			ad->record->name );
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = bluice_server_ptr;
	}

	if ( bluice_type_server != (void *) NULL ) {
		*bluice_type_server = bluice_server_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_bluice_area_detector_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_bluice_area_detector_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	bluice_area_detector = (MX_BLUICE_AREA_DETECTOR *)
				malloc( sizeof(MX_BLUICE_AREA_DETECTOR) );

	if ( bluice_area_detector == (MX_BLUICE_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Cannot allocate memory for an MX_BLUICE_AREA_DETECTOR structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = bluice_area_detector;
	record->class_specific_function_list = 
			&mxd_bluice_area_detector_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	bluice_area_detector->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_finish_record_initialization( MX_RECORD *record )
{
	return mx_area_detector_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_bluice_area_detector_open()";

	MX_AREA_DETECTOR *ad;
	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	mx_status_type mx_status;

	bluice_area_detector = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
			&bluice_area_detector, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	return mx_status;
}

#define mx_atomic_increment(x)  ((x)++)

MX_EXPORT mx_status_type
mxd_bluice_area_detector_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_bluice_area_detector_trigger()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_BLUICE_SERVER *bluice_server;
	void *type_server;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	MX_BLUICE_DHS_SERVER *bluice_dhs_server;
	MX_BLUICE_DHS_MANAGER *bluice_dhs_manager;
	MX_SEQUENCE_PARAMETERS *sp;
	double exposure_time;
	char command[200];
	unsigned long dark_current_fu;
	char datafile_name[MXU_FILENAME_LENGTH+1];
	char datafile_directory[MXU_FILENAME_LENGTH+1];
	char username[MXU_USERNAME_LENGTH+1];
	unsigned long operation_counter;
	mx_status_type mx_status;

	bluice_area_detector = NULL;

	mx_status = mxd_bluice_area_detector_get_pointers( ad,
		&bluice_area_detector, &bluice_server, &type_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	if ( sp->sequence_type != MXT_SQ_ONE_SHOT ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Sequence type %ld is not supported for MarCCD detector '%s'.  "
		"Only one-shot sequences are supported.",
			sp->sequence_type, ad->record->name );
	}

	exposure_time = sp->parameter_array[0];

	/* Construct the trigger command. */

#if 1
	dark_current_fu = 15;

	strlcpy( datafile_name, "test09_53_00", sizeof(datafile_name) );

	strlcpy( datafile_directory, "/data/lavender/test",
					sizeof(datafile_directory) );
#endif

	mx_username( username, sizeof(username) );

	switch( ad->record->mx_type ) {
	case MXT_AD_BLUICE_DCSS:
		bluice_dcss_server = type_server;

		operation_counter = mx_atomic_increment(
				bluice_dcss_server->operation_counter );

		snprintf( command, sizeof(command),
		"gtos_start_operation collectFrame %lu.%lu %lu %s %s %s "
			"NULL NULL 0 %f 0 1 0",
			bluice_dcss_server->client_number,
			operation_counter,
			dark_current_fu,
			datafile_name,
			datafile_directory,
			username,
			exposure_time );
		break;

	case MXT_AD_BLUICE_DHS:
		bluice_dhs_server = type_server;

		bluice_dhs_manager =
		    bluice_dhs_server->dhs_manager_record->record_type_struct;

		operation_counter = mx_atomic_increment(
				bluice_dhs_manager->operation_counter );

		snprintf( command, sizeof(command),
		"stoh_start_operation detector_collect_image %lu.%lu %lu "
		"%s %s %s NULL %f 0.0 0.0 0.0 0.0 0.0 0.0 0 0",
			1L,
			operation_counter,
			dark_current_fu,
			datafile_name,
			datafile_directory,
			username,
			exposure_time );
		break;
	}
			
#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_stop( MX_AREA_DETECTOR *ad )
{
#if 0
	static const char fname[] = "mxd_bluice_area_detector_stop()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	mx_status_type mx_status;

	bluice_area_detector = NULL;

	mx_status = mxd_bluice_area_detector_get_pointers( ad, &bluice_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	bluice_area_detector->finish_time = mx_current_clock_tick();

	bluice_area_detector->current_command = MXF_BLUICE_AREA_DETECTOR_CMD_ABORT;

	mx_status = mxd_bluice_area_detector_command( ad, bluice_area_detector, "abort", MXD_BLUICE_AREA_DETECTOR_DEBUG );

	return mx_status;
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_get_extended_status( MX_AREA_DETECTOR *ad )
{
#if 0
	static const char fname[] = "mxd_bluice_area_detector_get_extended_status()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_CLOCK_TICK current_time;
	int comparison;
	mx_status_type mx_status;

	bluice_area_detector = NULL;

	mx_status = mxd_bluice_area_detector_get_pointers( ad, &bluice_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mxd_bluice_area_detector_check_for_responses( ad,
						bluice_area_detector, MXD_BLUICE_AREA_DETECTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	current_time = mx_current_clock_tick();

	comparison = mx_compare_clock_ticks(current_time, bluice_area_detector->finish_time);

	if ( comparison < 0 ) {
		ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
		ad->last_frame_number = -1;
	} else {
		if ( ad->status & MXSF_AD_ACQUISITION_IN_PROGRESS ) {

			/* If we just finished an acquisition cycle,
			 * then increment the total number of frames.
			 */

			(ad->total_num_frames)++;
		}

		ad->last_frame_number = 0;

		bluice_area_detector->use_finish_time = FALSE;
	}

#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_readout_frame( MX_AREA_DETECTOR *ad )
{
#if 0
	static const char fname[] = "mxd_bluice_area_detector_readout_frame()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	mx_status_type mx_status;

	bluice_area_detector = NULL;

	mx_status = mxd_bluice_area_detector_get_pointers( ad, &bluice_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	bluice_area_detector->current_command = MXF_BLUICE_AREA_DETECTOR_CMD_READOUT;

	mx_status = mxd_bluice_area_detector_command( ad, bluice_area_detector, "readout,0",
				MXF_BLUICE_AREA_DETECTOR_FORCE_READ | MXD_BLUICE_AREA_DETECTOR_DEBUG );

	return mx_status;
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_correct_frame( MX_AREA_DETECTOR *ad )
{
#if 0
	static const char fname[] = "mxd_bluice_area_detector_correct_frame()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	mx_status_type mx_status;

	bluice_area_detector = NULL;

	mx_status = mxd_bluice_area_detector_get_pointers( ad, &bluice_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	bluice_area_detector->current_command = MXF_BLUICE_AREA_DETECTOR_CMD_CORRECT;
	bluice_area_detector->next_state = MXF_BLUICE_AREA_DETECTOR_STATE_CORRECT;

	mx_status = mxd_bluice_area_detector_command(ad, bluice_area_detector, "correct", MXD_BLUICE_AREA_DETECTOR_DEBUG);

	return mx_status;
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_get_parameter( MX_AREA_DETECTOR *ad )
{
#if 0
	static const char fname[] = "mxd_bluice_area_detector_get_parameter()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	mx_status_type mx_status;

	bluice_area_detector = NULL;

	mx_status = mxd_bluice_area_detector_get_pointers( ad, &bluice_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		mx_status = mxd_bluice_area_detector_command( ad, bluice_area_detector, "get_size",
							MXD_BLUICE_AREA_DETECTOR_DEBUG );
		break;

	case MXLV_AD_BINSIZE:
		mx_status = mxd_bluice_area_detector_command( ad, bluice_area_detector, "get_bin",
							MXD_BLUICE_AREA_DETECTOR_DEBUG );
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		ad->image_format = MXT_IMAGE_FILE_TIFF;

		mx_status = mx_image_get_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxd_bluice_area_detector_set_parameter( MX_AREA_DETECTOR *ad )
{
#if 0
	static const char fname[] = "mxd_bluice_area_detector_set_parameter()";

	MX_BLUICE_AREA_DETECTOR *bluice_area_detector;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_bluice_area_detector_get_pointers( ad, &bluice_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BLUICE_AREA_DETECTOR_DEBUG
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
	case MXLV_AD_SEQUENCE_TYPE:
		if ( sp->sequence_type != MXT_SQ_ONE_SHOT ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Sequence type %ld is not supported for MarCCD "
		      "detector '%s'.  Only one-shot sequences are supported.",
				sp->sequence_type, ad->record->name );
		}
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
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}


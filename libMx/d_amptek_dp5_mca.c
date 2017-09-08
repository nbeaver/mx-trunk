/*
 * Name:    d_amptek_dp5_mca.c
 *
 * Purpose: MX multichannel analyzer driver for Amptek MCAs that use the
 *          DP5 protocol.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_AMPTEK_DP5_DEBUG		FALSE

#define MXD_AMPTEK_DP5_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_hrt.h"
#include "mx_hrt_debug.h"
#include "mx_process.h"
#include "mx_rs232.h"
#include "mx_usb.h"
#include "mx_mca.h"
#include "i_amptek_dp5.h"
#include "d_amptek_dp5_mca.h"

#if MXD_AMPTEK_DP5_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_amptek_dp5_mca_record_function_list = {
	mxd_amptek_dp5_mca_initialize_driver,
	mxd_amptek_dp5_mca_create_record_structures,
	mx_mca_finish_record_initialization,
	NULL,
	NULL,
	mxd_amptek_dp5_mca_open,
	NULL,
	NULL,
	NULL,
	mxd_amptek_dp5_mca_special_processing_setup
};

MX_MCA_FUNCTION_LIST mxd_amptek_dp5_mca_mca_function_list = {
	mxd_amptek_dp5_mca_start,
	mxd_amptek_dp5_mca_stop,
	mxd_amptek_dp5_mca_read,
	mxd_amptek_dp5_mca_clear,
	mxd_amptek_dp5_mca_busy,
	mxd_amptek_dp5_mca_get_parameter,
	mxd_amptek_dp5_mca_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_amptek_dp5_mca_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_AMPTEK_DP5_STANDARD_FIELDS,
};

long mxd_amptek_dp5_mca_num_record_fields
		= sizeof( mxd_amptek_dp5_mca_record_field_defaults )
		  / sizeof( mxd_amptek_dp5_mca_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_amptek_dp5_mca_rfield_def_ptr
			= &mxd_amptek_dp5_mca_record_field_defaults[0];

static mx_status_type mxd_amptek_dp5_mca_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/* Private functions for the use of the driver. */

static mx_status_type
mxd_amptek_dp5_mca_get_pointers( MX_MCA *mca,
			MX_AMPTEK_DP5_MCA **amptek_dp5_mca,
			MX_AMPTEK_DP5 **amptek_dp5,
			const char *calling_fname )
{
	static const char fname[] = "mxd_amptek_dp5_mca_get_pointers()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca_ptr = NULL;
	MX_RECORD *amptek_dp5_record = NULL;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' is NULL.",
			calling_fname );
	}

	amptek_dp5_mca_ptr = (MX_AMPTEK_DP5_MCA *)
				mca->record->record_type_struct;

	if ( amptek_dp5_mca_ptr == (MX_AMPTEK_DP5_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AMPTEK_DP5_MCA pointer for MCA '%s' is NULL.",
			mca->record->name );
	}

	if ( amptek_dp5_mca != (MX_AMPTEK_DP5_MCA **) NULL ) {
		*amptek_dp5_mca = amptek_dp5_mca_ptr;
	}

	if ( amptek_dp5 != (MX_AMPTEK_DP5 **) NULL ) {
		amptek_dp5_record = amptek_dp5_mca_ptr->amptek_dp5_record;

		if ( amptek_dp5_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The amptek_dp5_record pointer for Amptek DP5 "
			"MCA '%s' is NULL.", mca->record->name );
		}

		*amptek_dp5 = (MX_AMPTEK_DP5 *)
				amptek_dp5_record->record_type_struct;

		if ( (*amptek_dp5) == (MX_AMPTEK_DP5 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_AMPTEK_DP5 pointer for Amptek DP5 '%s' "
			"used by MCA record '%s' is NULL.",
				amptek_dp5_record->name,
				mca->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_channels_varargs_cookie;
	long maximum_num_rois_varargs_cookie;
	long num_soft_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mca_initialize_driver( driver,
				&maximum_num_channels_varargs_cookie,
				&maximum_num_rois_varargs_cookie,
				&num_soft_rois_varargs_cookie );

	return mx_status;
}


MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_amptek_dp5_mca_create_record_structures()";

	MX_MCA *mca;
	MX_AMPTEK_DP5_MCA *amptek_dp5_mca;

	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA structure." );
	}

	amptek_dp5_mca = (MX_AMPTEK_DP5_MCA *)
				malloc( sizeof(MX_AMPTEK_DP5_MCA) );

	if ( amptek_dp5_mca == (MX_AMPTEK_DP5_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AMPTEK_DP5_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = amptek_dp5_mca;
	record->class_specific_function_list =
				&mxd_amptek_dp5_mca_mca_function_list;

	mca->record = record;
	amptek_dp5_mca->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_amptek_dp5_mca_open()";

	MX_MCA *mca = NULL;
	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case 0:
			record_field->process_function
				    = mxd_amptek_dp5_mca_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_start( MX_MCA *mca )
{
	static const char fname[] = "mxd_amptek_dp5_mca_start()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_amptek_dp5_mca_stop()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_amptek_dp5_mca_read()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_amptek_dp5_mca_clear()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_amptek_dp5_mca_busy()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_amptek_dp5_mca_get_parameter()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	char ascii_response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_MAXIMUM_NUM_CHANNELS:
		mca->maximum_num_channels = 8192;
		break;
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		mx_status = mxi_amptek_dp5_ascii_command( amptek_dp5,
						"MCAC;",
						ascii_response,
						sizeof(ascii_response),
						FALSE,
						amptek_dp5->amptek_dp5_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( ascii_response,
					"MCAC=%lu;",
					&(mca->current_num_channels) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the current number of MCA channels "
			"in the response '%s' to command 'MCAC;' for "
			"MCA '%s'.", ascii_response, mca->record->name );
		}
		break;
	case MXLV_MCA_REAL_TIME:
		break;
	case MXLV_MCA_LIVE_TIME:
		break;
	case MXLV_MCA_PRESET_REAL_TIME:
		mx_status = mxi_amptek_dp5_ascii_command( amptek_dp5,
						"PRET;",
						ascii_response,
						sizeof(ascii_response),
						FALSE,
						amptek_dp5->amptek_dp5_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( ascii_response,
					"PRET=%lg;",
					&(mca->preset_real_time) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the preset real time in the response '%s' "
			"to command 'PRET;' for MCA '%s'.",
				ascii_response, mca->record->name );
		}
	case MXLV_MCA_PRESET_COUNT:
		mx_status = mxi_amptek_dp5_ascii_command( amptek_dp5,
						"PREC;",
						ascii_response,
						sizeof(ascii_response),
						FALSE,
						amptek_dp5->amptek_dp5_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( ascii_response,
					"PREC=%lu;",
					&(mca->preset_count) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the preset real time in the response '%s' "
			"to command 'PRET;' for MCA '%s'.",
				ascii_response, mca->record->name );
		}
		break;
	case MXLV_MCA_CHANNEL_VALUE:
		break;
	case MXLV_MCA_ROI:
		break;
	case MXLV_MCA_ROI_INTEGRAL:
		break;
	case MXLV_MCA_INPUT_COUNT_RATE:
		break;
	case MXLV_MCA_OUTPUT_COUNT_RATE:
		break;
	default:
		mx_status = mx_mca_default_get_parameter_handler( mca );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_set_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_amptek_dp5_mca_set_parameter()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	char ascii_command[80];
	mx_status_type mx_status, mx_status_2;

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_MAXIMUM_NUM_CHANNELS:
		mca->maximum_num_channels = 8192;
		break;
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		/* Check for valid values for the current number of channels. */
		switch( mca->current_num_channels ) {
		case 256:
		case 512:
		case 1024:
		case 2048:
		case 4096:
		case 8192:
			break;
		default:
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"An illegal number of channels (%lu) was requested "
			"for MCA '%s'.  The allowed values are 256, 512, 1024, "
			"2048, 4096, and 8192.",
				mca->current_num_channels,
				mca->record->name );

			mx_status_2 = mxd_amptek_dp5_mca_get_parameter( mca );
			break;
		}

		/* Now send the command. */
		snprintf( ascii_command, sizeof(ascii_command),
			"MCAC=%lu;", mca->current_num_channels );

		mx_status = mxi_amptek_dp5_ascii_command( amptek_dp5,
						ascii_command, NULL, 0, TRUE,
						amptek_dp5->amptek_dp5_flags );
		break;
	case MXLV_MCA_ROI:
		break;
	case MXLV_MCA_PRESET_REAL_TIME:
		snprintf( ascii_command, sizeof(ascii_command),
			"PREC=OFF;PRER=OFF;PRET=%f;", mca->preset_real_time );

		mx_status = mxi_amptek_dp5_ascii_command( amptek_dp5,
						ascii_command, NULL, 0, TRUE,
						amptek_dp5->amptek_dp5_flags );
		break;
	case MXLV_MCA_PRESET_COUNT:
		snprintf( ascii_command, sizeof(ascii_command),
			"PRER=OFF;PRET=OFF;PREC=%lu;", mca->preset_count );

		mx_status = mxi_amptek_dp5_ascii_command( amptek_dp5,
						ascii_command, NULL, 0, TRUE,
						amptek_dp5->amptek_dp5_flags );
		break;
	default:
		mx_status = mx_mca_default_set_parameter_handler( mca );
		break;
	}

	return mx_status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_amptek_dp5_mca_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxd_amptek_dp5_mca_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AMPTEK_DP5_MCA *amptek_dp5_mca;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	amptek_dp5_mca = (MX_AMPTEK_DP5_MCA *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}


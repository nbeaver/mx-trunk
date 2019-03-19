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
 * Copyright 2017-2019 Illinois Institute of Technology
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
	NULL,
	mxd_amptek_dp5_mca_trigger,
	mxd_amptek_dp5_mca_stop,
	mxd_amptek_dp5_mca_read,
	mxd_amptek_dp5_mca_clear,
	mxd_amptek_dp5_mca_get_status,
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
	char response[80];
	int num_items;
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

	/* Read in the currently configured number of MCA channels. */

	mx_status = mxi_amptek_dp5_ascii_command( amptek_dp5, "MCAC;",
					response, sizeof(response),
					FALSE, amptek_dp5->amptek_dp5_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "MCAC=%ld;", 
				&(mca->current_num_channels) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to parse the response '%s' to command 'MCAC;' "
		"for MCA '%s'.",  response, record->name );
	}

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
		case MXLV_AMPTEK_DP5_STATUS_DATA:
		case MXLV_AMPTEK_DP5_FAST_COUNTS:
		case MXLV_AMPTEK_DP5_SLOW_COUNTS:
		case MXLV_AMPTEK_DP5_GENERAL_PURPOSE_COUNTER:
		case MXLV_AMPTEK_DP5_ACCUMULATION_TIME:
		case MXLV_AMPTEK_DP5_HIGH_VOLTAGE:
		case MXLV_AMPTEK_DP5_TEMPERATURE:
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
mxd_amptek_dp5_mca_trigger( MX_MCA *mca )
{
	static const char fname[] = "mxd_amptek_dp5_mca_trigger()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	char command[200];
	mx_status_type mx_status;

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_LIVE_TIME:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Preset live time is not supported for MCA '%s'.  You must use "
		"either preset real time or preset count instead.\n",
			mca->record->name );
		break;
	case MXF_MCA_PRESET_REAL_TIME:
		snprintf( command, sizeof(command),
			"PREC=OFF;PRET=OFF;PRER=%f;",
			mca->preset_real_time );

		mx_status = mxi_amptek_dp5_ascii_command( amptek_dp5,
						command, NULL, 0, TRUE,
						amptek_dp5->amptek_dp5_flags );
		break;
	case MXF_MCA_PRESET_COUNT:
		/* FIXME: You must specify values for PRCL and PRCH here. */

		snprintf( command, sizeof(command),
			"PRER=OFF;PRET=OFF;PREC=%lu;",
			mca->preset_count );

		mx_status = mxi_amptek_dp5_ascii_command( amptek_dp5,
						command, NULL, 0, TRUE,
						amptek_dp5->amptek_dp5_flags );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Preset type %ld is not recognized for MCA '%s'.",
			mca->preset_type, mca->record->name );
		break;
	}

	/* Enable the MCA. */

	mx_status = mxi_amptek_dp5_binary_command( amptek_dp5, 0xF0, 2,
						NULL, NULL, NULL, 0,
						NULL, 0, NULL, TRUE );

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

	mx_status = mxi_amptek_dp5_binary_command( amptek_dp5, 0xF0, 3,
						NULL, NULL, NULL, 0,
						NULL, 0, NULL, TRUE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_amptek_dp5_mca_read()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	char *raw_mca_spectrum = NULL;
	unsigned long *channel_array = NULL;
	unsigned long i, j;
	long response_pid1, response_pid2;
	mx_status_type mx_status;

#if 1
	unsigned long low_val, mid_val, high_val;
	unsigned long channel_value;
#endif

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	memset( amptek_dp5_mca->raw_mca_spectrum, 0,
		sizeof( amptek_dp5_mca->raw_mca_spectrum ) );

	/* Read out the spectrum into the 24-bit array called
	 * 'amptek_dp5_mca->raw_mca_spectrum' and then copy that
	 * to the 32-bit (or 64-bit) 'mca->channel_array' structure.
	 */

	mx_status = mxi_amptek_dp5_binary_command( amptek_dp5,
						2, 1,
						&response_pid1,
						&response_pid2,
						NULL, 0,
					amptek_dp5_mca->raw_mca_spectrum,
						3 * MXU_AMPTEK_DP5_MAX_BINS,
						NULL,
						amptek_dp5->amptek_dp5_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Copy the 24-bit data from amptek_dp5_mca->raw_mca_spectrum to
	 * the unsigned long mca->channel_array structure.
	 */

	raw_mca_spectrum = amptek_dp5_mca->raw_mca_spectrum;

	channel_array = mca->channel_array;

	for ( i = 0; i < mca->current_num_channels; i++ ) {
		j = 3 * i;

		/* We must be careful here to avoid unwanted sign extension. */

		low_val =
		    ((unsigned long) raw_mca_spectrum[j]) & 0xff;
		mid_val = 
		    (((unsigned long) raw_mca_spectrum[j+1]) << 8) & 0xff00;
		high_val =
		    (((unsigned long) raw_mca_spectrum[j+2]) << 16) & 0xff0000;

		channel_value = low_val + mid_val + high_val;

#if 0
		if ( i < 50 ) {
			MX_DEBUG(-2,("RAW[%lu]     = %#x, %#x, %#x", i,
				(unsigned char) raw_mca_spectrum[j+2],
				(unsigned char) raw_mca_spectrum[j+1],
				(unsigned char) raw_mca_spectrum[j] ));
			MX_DEBUG(-2,("SHIFTED[%lu] = %#lx, %#lx, %#lx", i,
				high_val, mid_val, low_val));
			MX_DEBUG(-2,("RESULT[%lu]  = %#lx, %lu", i,
				channel_value, channel_value));
		}
#endif

		channel_array[i] = channel_value;
	}

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

	mx_status = mxi_amptek_dp5_binary_command( amptek_dp5,
						0xF0, 1,
						NULL, NULL,
						NULL, 0,
						NULL, 0,
						NULL,
						amptek_dp5->amptek_dp5_flags );

	return mx_status;
}

/* FIXME: mxd_amptek_dp5_get_status() should become an API function as
 * mx_mca_get_status() or some such.  Other device classes already do this.
 */

MX_EXPORT mx_status_type
mxd_amptek_dp5_mca_get_status( MX_MCA *mca )
{
	static const char fname[] = "mxd_amptek_dp5_mca_get_status()";

	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	uint8_t status_byte_35;
	char *status_data;
	unsigned long accumulation_time_msec, real_time_msec;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_amptek_dp5_mca_get_pointers( mca,
					&amptek_dp5_mca, &amptek_dp5, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a 'Request Status Packet'. */

	status_data = amptek_dp5_mca->status_data;

	mx_status = mxi_amptek_dp5_binary_command( amptek_dp5, 1, 1,
						NULL, NULL, NULL, 0,
						status_data,
						MXU_AMPTEK_DP5_NUM_STATUS_BYTES,
						NULL, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = amptek_dp5_mca->amptek_dp5_mca_flags;

	if ( flags & MXF_AMPTEK_DP5_MCA_DEBUG_STATUS_PACKET ) {
		fprintf( stderr, "status[35] = %#x, status[36] = %#x\n",
			((unsigned int) status_data[35]) & 0xff,
			((unsigned int) status_data[36]) & 0xff );
	}

	/* Fast counts (bytes 0 to 3) */

	amptek_dp5_mca->fast_counts = ( status_data[3] << 24 )
					+ ( status_data[2] << 16 )
					+ ( status_data[1] << 8 )
					+ status_data[0]; 

	/* Slow counts (bytes 4 to 7) */

	amptek_dp5_mca->slow_counts = ( status_data[7] << 24 )
					+ ( status_data[6] << 16 )
					+ ( status_data[5] << 8 )
					+ status_data[4]; 

	/* General purpose counter (bytes 8 to 11) */

	amptek_dp5_mca->general_purpose_counter = ( status_data[11] << 24 )
						+ ( status_data[10] << 16 )
						+ ( status_data[9] << 8 )
						+ status_data[8]; 

	/* Accumulation time milliseconds (bytes 12 to 15) */

	accumulation_time_msec = ( status_data[15] << 24 )
					+ ( status_data[14] << 16 )
					+ ( status_data[13] << 8 )
					+ status_data[12]; 

	amptek_dp5_mca->accumulation_time = 0.001 * accumulation_time_msec;

	/* Real time milliseconds (bytes 20 to 23) */

	real_time_msec = ( status_data[23] << 24 )
			+ ( status_data[22] << 16 )
			+ ( status_data[21] << 8 )
			+ status_data[20]; 

	mca->real_time = 0.001 * real_time_msec;

	/* High voltage (bytes 30 to 31) */

	amptek_dp5_mca->high_voltage = 0.5 * ( ( status_data[30] << 8 )
						+ status_data[31] );

	/* Detector temperature (bytes 32 to 33) */

	amptek_dp5_mca->temperature = 0.5 * ( (( status_data[32] << 8 ) & 0xf)
						+ status_data[33] );

	/* Update the busy status from byte 35. */

	mca->busy = FALSE;

	status_byte_35 = (uint8_t) status_data[35];

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_LIVE_TIME:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Preset live time is not supported for MCA '%s'.  You must use "
		"either preset real time or preset count instead.\n",
			mca->record->name );
		break;
	case MXF_MCA_PRESET_REAL_TIME:
		if ( (status_byte_35 & 0x20) == 0 ) {
			mca->busy = FALSE;
		} else
		if ( status_byte_35 & 0x80 ) {
			mca->busy = FALSE;
		} else {
			mca->busy = TRUE;
		}
		break;
	case MXF_MCA_PRESET_COUNT:
		if ( (status_byte_35 & 0x20) == 0 ) {
			mca->busy = FALSE;
		} else
		if ( status_byte_35 & 0x10 ) {
			mca->busy = FALSE;
		} else {
			mca->busy = TRUE;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Preset type %ld is not recognized for MCA '%s'.",
			mca->preset_type, mca->record->name );
		break;
	}

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
						"PRER;",
						ascii_response,
						sizeof(ascii_response),
						FALSE,
						amptek_dp5->amptek_dp5_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( ascii_response,
					"PRER=%lg;",
					&(mca->preset_real_time) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the preset real time in the response '%s' "
			"to command 'PRER;' for MCA '%s'.",
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
			"Did not see the preset count in the response '%s' "
			"to command 'PREC;' for MCA '%s'.",
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
	case MXLV_MCA_OUTPUT_COUNT_RATE:
		mx_status = mxd_amptek_dp5_mca_get_status( mca );
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

			MXW_UNUSED( mx_status_2 );
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
			"PREC=OFF;PRET=OFF;PRER=%f;", mca->preset_real_time );

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

	MX_RECORD *record = NULL;
	MX_RECORD_FIELD *record_field = NULL;
	MX_MCA *mca = NULL;
	MX_AMPTEK_DP5_MCA *amptek_dp5_mca = NULL;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	mca = (MX_MCA *) record->record_class_struct;
	amptek_dp5_mca = (MX_AMPTEK_DP5_MCA *) record->record_type_struct;

	MXW_UNUSED( amptek_dp5_mca );

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_AMPTEK_DP5_STATUS_DATA:
		case MXLV_AMPTEK_DP5_FAST_COUNTS:
		case MXLV_AMPTEK_DP5_SLOW_COUNTS:
		case MXLV_AMPTEK_DP5_GENERAL_PURPOSE_COUNTER:
		case MXLV_AMPTEK_DP5_ACCUMULATION_TIME:
		case MXLV_AMPTEK_DP5_HIGH_VOLTAGE:
		case MXLV_AMPTEK_DP5_TEMPERATURE:
			mx_status = mxd_amptek_dp5_mca_get_status( mca );
			break;
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


/*
 * Name:    d_trump.c
 *
 * Purpose: MX multichannel analyzer driver for the EG&G Ortec Trump MCA.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2005, 2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_TRUMP_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"

#if HAVE_ORTEC_UMCBI && defined(OS_WIN32)

#include <windows.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_mca.h"
#include "i_umcbi.h"
#include "d_trump.h"

/* The following header file is supplied by Ortec as part of the UMCBI kit. */

#include "mcbcio32.h"

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_trump_record_function_list = {
	mxd_trump_initialize_type,
	mxd_trump_create_record_structures,
	mxd_trump_finish_record_initialization,
	NULL,
	mxd_trump_print_structure,
	mxd_trump_open,
	mxd_trump_close
};

MX_MCA_FUNCTION_LIST mxd_trump_mca_function_list = {
	mxd_trump_start,
	mxd_trump_stop,
	mxd_trump_read,
	mxd_trump_clear,
	mxd_trump_busy,
	mxd_trump_get_parameter,
	mxd_trump_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_trump_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_TRUMP_STANDARD_FIELDS
};

long mxd_trump_num_record_fields
		= sizeof( mxd_trump_record_field_defaults )
		  / sizeof( mxd_trump_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_trump_rfield_def_ptr
			= &mxd_trump_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_trump_get_pointers( MX_MCA *mca,
			MX_TRUMP_MCA **trump_mca,
			MX_UMCBI_DETECTOR **umcbi_detector,
			const char *calling_fname )
{
	static const char fname[] = "mxd_trump_get_pointers()";

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( trump_mca == (MX_TRUMP_MCA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_TRUMP_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( umcbi_detector == (MX_UMCBI_DETECTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_UMCBI_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*trump_mca = (MX_TRUMP_MCA *) (mca->record->record_type_struct);

	if ( *trump_mca == (MX_TRUMP_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TRUMP_MCA pointer for record '%s' is NULL.",
			mca->record->name );
	}

	*umcbi_detector = (*trump_mca)->detector;

	if ( *umcbi_detector == (MX_UMCBI_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_UMCBI_DETECTOR pointer for record '%s' is NULL.",
			mca->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_trump_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_channels_varargs_cookie;
	long maximum_num_rois_varargs_cookie;
	long num_soft_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mca_initialize_type( record_type,
				&num_record_fields,
				&record_field_defaults,
				&maximum_num_channels_varargs_cookie,
				&maximum_num_rois_varargs_cookie,
				&num_soft_rois_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_trump_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_trump_create_record_structures()";

	MX_MCA *mca;
	MX_TRUMP_MCA *trump_mca;

	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA structure." );
	}

	trump_mca = (MX_TRUMP_MCA *) malloc( sizeof(MX_TRUMP_MCA) );

	if ( trump_mca == (MX_TRUMP_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TRUMP_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = trump_mca;
	record->class_specific_function_list = &mxd_trump_mca_function_list;

	mca->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trump_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_trump_finish_record_initialization()";

	MX_MCA *mca;
	MX_TRUMP_MCA *trump_mca;
	mx_status_type mx_status;

	mca = (MX_MCA *) record->record_class_struct;

	mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;

	mca->busy = FALSE;

	if ( mca->maximum_num_rois != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The 'trump' driver for record '%s' expects that "
		"'maximum_num_rois' will be set to 1.", record->name );
	}

	mca->current_num_rois = mca->maximum_num_rois;

	trump_mca = (MX_TRUMP_MCA *) record->record_type_struct;

	trump_mca->detector = NULL;

	mx_status = mx_mca_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_trump_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_trump_print_structure()";

	MX_MCA *mca;
	MX_TRUMP_MCA *trump_mca;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mca = (MX_MCA *) (record->record_class_struct);

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCA pointer for record '%s' is NULL.", record->name);
	}

	trump_mca = (MX_TRUMP_MCA *) (record->record_type_struct);

	if ( trump_mca == (MX_TRUMP_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_TRUMP_MCA pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "MCA parameters for record '%s':\n", record->name);

	fprintf(file, "  MCA type              = TRUMP_MCA.\n\n");
	fprintf(file, "  UMCBI record          = '%s'\n",
					trump_mca->umcbi_record->name);
	fprintf(file, "  detector number       = %d\n",
					trump_mca->detector_number);
	fprintf(file, "  maximum # of channels = %lu\n",
					mca->maximum_num_channels);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trump_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_trump_open()";

	MX_MCA *mca;
	MX_TRUMP_MCA *trump_mca;
	MX_UMCBI_DETECTOR *detector;
	void *detector_handle;
	char response[80];
	long i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MCA pointer for record '%s' is NULL.",
			record->name);
	}

	trump_mca = (MX_TRUMP_MCA *) (record->record_type_struct);

	if ( trump_mca == (MX_TRUMP_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TRUMP_MCA pointer for scaler record '%s' is NULL.",
			record->name );
	}

	mca->current_num_channels = mca->maximum_num_channels;
	mca->current_num_rois = mca->maximum_num_rois;

	for ( i = 0; i < mca->maximum_num_rois; i++ ) {
		mca->roi_array[i][0] = 0;
		mca->roi_array[i][1] = 0;
	}

	mx_status = mxi_umcbi_get_detector_struct( trump_mca->umcbi_record,
			trump_mca->detector_number, &detector );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trump_mca->detector = detector;

	detector->record = record;

	detector_handle = MIOOpenDetector(trump_mca->detector_number, "", "");

	if ( detector_handle == NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Attempt to open connection to MCA '%s' via MIOOpenDetector() failed.  "
"Reason = '%s'.", record->name, mxi_umcbi_strerror() );
	}

	detector->detector_handle = detector_handle;

	/* Verify that we are talking to a Trump MCA card. */

	mx_status = mxi_umcbi_command( detector, "SHOW_VERSION",
				response, sizeof response, MXD_TRUMP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strncmp( response, "$FTRMP", 6 ) != 0 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Detector '%s' is not a Trump multichannel buffer card.  "
		"Response to 'SHOW_VERSION' was '%s'.",
			record->name, response );
	}

#if MXD_TRUMP_DEBUG
	MX_DEBUG(-2,("%s: Connection to MCA '%s' was successfully opened.",
		fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trump_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_trump_close()";

	MX_TRUMP_MCA *trump_mca;
	BOOL status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	trump_mca = (MX_TRUMP_MCA *) (record->record_type_struct);

	if ( trump_mca == (MX_TRUMP_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TRUMP_MCA pointer for scaler record '%s' is NULL.",
			record->name );
	}

	status = MIOCloseDetector( trump_mca->detector->detector_handle );

	if ( status == FALSE ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Attempt to close connection to MCA '%s' via MIOCloseDetector() failed.  "
"Reason = '%s'.", record->name, mxi_umcbi_strerror() );
	}

#if MXD_TRUMP_DEBUG
	MX_DEBUG(-2,("%s: Connection to MCA '%s' was successfully closed.",
		fname, record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trump_start( MX_MCA *mca )
{
	static const char fname[] = "mxd_trump_start()";

	MX_TRUMP_MCA *trump_mca = NULL;
	MX_UMCBI_DETECTOR *detector = NULL;
	char command[50];
	unsigned long tick_count;
	mx_status_type mx_status;

	mx_status = mxd_trump_get_pointers( mca, &trump_mca, &detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_umcbi_command( detector,
				"CLEAR", NULL, 0, MXD_TRUMP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_TRUMP_DEBUG
	MX_DEBUG(-2,("%s: preset_type = %d", fname, mca->preset_type));
#endif

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_NONE:
		mx_status = mxi_umcbi_command( detector,
				"SET_LIVE_PRESET 0", NULL, 0, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxi_umcbi_command( detector,
				"SET_TRUE_PRESET 0", NULL, 0, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXF_MCA_PRESET_LIVE_TIME:
		tick_count = ( mca->preset_live_time )
					* MXD_TRUMP_TICKS_PER_SECOND;

		sprintf( command, "SET_LIVE_PRESET %lu", tick_count );

		mx_status = mxi_umcbi_command( detector,
				command, NULL, 0, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxi_umcbi_command( detector,
				"SET_TRUE_PRESET 0", NULL, 0, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXF_MCA_PRESET_REAL_TIME:
		tick_count = ( mca->preset_real_time )
					* MXD_TRUMP_TICKS_PER_SECOND;

		sprintf( command, "SET_TRUE_PRESET %lu", tick_count );

		mx_status = mxi_umcbi_command( detector,
				command, NULL, 0, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxi_umcbi_command( detector,
				"SET_LIVE_PRESET 0", NULL, 0, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Preset type %d is not supported for record '%s'",
			mca->preset_type );
		break;
	}

	mx_status = mxi_umcbi_command( detector,
				"START", NULL, 0, MXD_TRUMP_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_trump_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_trump_stop()";

	MX_TRUMP_MCA *trump_mca = NULL;
	MX_UMCBI_DETECTOR *detector = NULL;
	mx_status_type mx_status;

	mx_status = mxd_trump_get_pointers( mca, &trump_mca, &detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_umcbi_command( detector,
				"STOP", NULL, 0, MXD_TRUMP_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_trump_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_trump_read()";

	MX_TRUMP_MCA *trump_mca = NULL;
	MX_UMCBI_DETECTOR *detector = NULL;
	unsigned long i, num_channels;
	mx_status_type mx_status;

	DWORD *data_ptr;
	DWORD data_mask, roi_mask;
	WORD returned_channels;

	mx_status = mxd_trump_get_pointers( mca, &trump_mca, &detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_TRUMP_DEBUG
	MX_DEBUG(-2,("%s: Before mxd_trump_get_num_channels()", fname));
#endif

	mx_status = mx_mca_get_num_channels( mca->record, &num_channels );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_TRUMP_DEBUG
	MX_DEBUG(-2,("%s: mca = %p", fname, mca));
	MX_DEBUG(-2,("%s: detector = %p", fname, detector));
	MX_DEBUG(-2,("%s: detector->detector_handle = %p",
			fname, detector->detector_handle));
	MX_DEBUG(-2,("%s: mca->current_num_channels = %lu",
			fname, mca->current_num_channels));
	MX_DEBUG(-2,("%s: mca->channel_array = %p",
			fname, mca->channel_array));

	MX_DEBUG(-2,("%s: Before MIOGetData()", fname));
#endif

	data_ptr = MIOGetData( detector->detector_handle,
				0, ( WORD ) mca->current_num_channels,
				( LPDWORD ) mca->channel_array,
				&returned_channels, &data_mask,
				&roi_mask, "" );

#if MXD_TRUMP_DEBUG
	MX_DEBUG(-2,("%s: After MIOGetData()", fname));
	MX_DEBUG(-2,("%s: returned channels = %hd", fname, returned_channels));
	MX_DEBUG(-2,("%s: data_mask = %lu 0x%x", fname, data_mask, data_mask));
	MX_DEBUG(-2,("%s: roi_mask = %lu 0x%x", fname, roi_mask, roi_mask));
#endif

	if ( data_ptr == NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Transfer of %lu channels from MCA '%s' failed.  Reason = '%s'",
			mca->current_num_channels,
			mca->record->name,
			mxi_umcbi_strerror() );
	}

	/* The high order bit will be set on channels that are part of the
	 * ROI, so we need to mask this off.
	 */

	for ( i = 0; i < returned_channels; i++ ) {

		mca->channel_array[i] &= ( ~(1<<31) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trump_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_trump_clear()";

	MX_TRUMP_MCA *trump_mca = NULL;
	MX_UMCBI_DETECTOR *detector = NULL;
	mx_status_type mx_status;

	mx_status = mxd_trump_get_pointers( mca, &trump_mca, &detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_umcbi_command( detector,
				"CLEAR", NULL, 0, MXD_TRUMP_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_trump_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_trump_busy()";

	MX_TRUMP_MCA *trump_mca = NULL;
	MX_UMCBI_DETECTOR *detector = NULL;
	char response[20];
	mx_status_type mx_status;

	mx_status = mxd_trump_get_pointers( mca, &trump_mca, &detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_umcbi_command( detector, "SHOW_ACTIVE",
				response, sizeof response, MXD_TRUMP_DEBUG );

	if ( strcmp( response, "$C00000087" ) == 0 ) {
		mca->busy = FALSE;
	} else {
		mca->busy = TRUE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_trump_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_trump_get_parameter()";

	MX_TRUMP_MCA *trump_mca = NULL;
	MX_UMCBI_DETECTOR *detector = NULL;
	char response[80];
	char buffer[80];
	char *ptr;
	int num_items;
	unsigned long num_roi_channels, raw_real_time, raw_live_time;
	mx_status_type mx_status;

	WORD num_channels;
	DWORD *data_ptr;
	DWORD long_value, data_mask, roi_mask;
	WORD returned_channels;

	mx_status = mxd_trump_get_pointers( mca, &trump_mca, &detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->parameter_type == MXLV_MCA_CURRENT_NUM_CHANNELS ) {

		num_channels = MIOGetDetLength( detector->detector_handle );

		mca->current_num_channels = (unsigned long) num_channels;

		if ( mca->current_num_channels > mca->maximum_num_channels ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The detector '%s' is reported by UMCBI to have %lu channels, "
"but the record '%s' is only configured to support up to %lu channels.",
				detector->name, mca->current_num_channels,
				mca->record->name, mca->maximum_num_channels );
		}
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI ) {

		mx_status = mxi_umcbi_command( detector, "SHOW_ROI",
				response, sizeof response, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (response[0] != '$') || (response[1] != 'D') ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unexpected response to 'SHOW_ROI' command.  Response = '%s'",
				response );
		}

		/* Parse the ROI start channel. */

		ptr = response + 2;

		strlcpy( buffer, ptr, 5 );

		num_items = sscanf( buffer, "%lu", &(mca->roi[0]) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"ROI start channel not found in response to 'SHOW_ROI' command.  "
	"Response = '%s'", response );
		}

		/* Parse the ROI number of channels. */

		ptr += 5;

		strlcpy( buffer, ptr, 5 );

		num_items = sscanf( buffer, "%lu", &num_roi_channels );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"ROI end channel not found in response to 'SHOW_ROI' command.  "
	"Response = '%s'", response );
		}

		if ( num_roi_channels == 0 ) {
			mca->roi[0] = 0;
			mca->roi[1] = 0;
		} else {
			mca->roi[1] = mca->roi[0] + num_roi_channels - 1L;
		}
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_INTEGRAL ) {

		mx_status = mxi_umcbi_command( detector, "SHOW_INTEGRAL",
				response, sizeof response, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (response[0] != '$') || (response[1] != 'G') ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unexpected response to 'SHOW_INTEGRAL' command.  Response = '%s'",
				response );
		}

		/* Move to the string response, null terminate it and
		 * then parse it.
		 */

		ptr = response + 2;

		ptr[10] = '\0';

		num_items = sscanf( ptr, "%lu", &(mca->roi_integral) );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot parse ROI integral in response '%s'",
				response );
		}

		/* The ROI integral is reported by UMCBI multiplied by an
		 * extra factor of 2.  We must divide this out.
		 */

		mca->roi_integral /= 2L;

	} else
	if ( mca->parameter_type == MXLV_MCA_CHANNEL_NUMBER ) {

		/* Do nothing.  The necessary value is already in
		 * mca->channel_number.
		 */

	} else
	if ( mca->parameter_type == MXLV_MCA_CHANNEL_VALUE ) {

		if ( mca->channel_number >= mca->maximum_num_channels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"mca->channel_number (%lu) is greater than or equal to "
		"mca->maximum_num_channels (%lu).  This should not be "
		"able to happen, so if you see this message, please "
		"report the program bug to Bill Lavender.",
			mca->channel_number, mca->maximum_num_channels );
		}

		data_ptr = MIOGetData( detector->detector_handle,
				( WORD ) mca->channel_number,
				( WORD ) 1,
				( LPDWORD ) &long_value,
				&returned_channels, &data_mask,
				&roi_mask, "" );

		if ( data_ptr == NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Transfer of channel %lu from MCA '%s' failed.  Reason = '%s'",
				mca->channel_number,
				mca->record->name,
				mxi_umcbi_strerror() );
		}

		mca->channel_value = (unsigned long) long_value;

		/* The high order bit will be set if this channel is
		 * part of the ROI, so we need to mask this off.
		 */

		mca->channel_value &= ( ~(1<<31) );

	} else
	if ( mca->parameter_type == MXLV_MCA_REAL_TIME ) {

		mx_status = mxi_umcbi_command( detector, "SHOW_TRUE",
				response, sizeof response, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (response[0] != '$') || (response[1] != 'G') ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unexpected response to 'SHOW_TRUE' command.  Response = '%s'",
				response );
		}

		/* Move to the string response, null terminate it and
		 * then parse it.
		 */

		ptr = response + 2;

		ptr[10] = '\0';

		num_items = sscanf( ptr, "%lu", &raw_real_time );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot parse raw real time value in response '%s'",
				response );
		}

		mca->real_time = mx_divide_safely( (double) raw_real_time,
						MXD_TRUMP_TICKS_PER_SECOND );
	} else
	if ( mca->parameter_type == MXLV_MCA_LIVE_TIME ) {

		mx_status = mxi_umcbi_command( detector, "SHOW_LIVE",
				response, sizeof response, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (response[0] != '$') || (response[1] != 'G') ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unexpected response to 'SHOW_LIVE' command.  Response = '%s'",
				response );
		}

		/* Move to the string response, null terminate it and
		 * then parse it.
		 */

		ptr = response + 2;

		ptr[10] = '\0';

		num_items = sscanf( ptr, "%lu", &raw_live_time );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot parse raw real time value in response '%s'",
				response );
		}

		mca->live_time = mx_divide_safely( (double) raw_live_time,
						MXD_TRUMP_TICKS_PER_SECOND );
	} else {
		return mx_mca_default_get_parameter_handler( mca );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trump_set_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_trump_set_parameter()";

	MX_TRUMP_MCA *trump_mca = NULL;
	MX_UMCBI_DETECTOR *detector = NULL;
	char command[80];
	unsigned long start, length;
	mx_status_type mx_status;

	mx_status = mxd_trump_get_pointers( mca, &trump_mca, &detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->parameter_type == MXLV_MCA_ROI ) {

		mx_status = mxi_umcbi_command( detector,
				"CLEAR_ROI", NULL, 0, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		start  = mca->roi[0];
		length = mca->roi[1] - mca->roi[0] + 1L;

#if MXD_TRUMP_DEBUG
		MX_DEBUG(-2,
		("%s: roi[0] = %lu, roi[1] = %lu, start = %lu, length = %lu",
			fname, mca->roi[0], mca->roi[1], start, length));
#endif

		sprintf( command, "SET_ROI %lu %lu", start, length );

		mx_status = mxi_umcbi_command( detector,
				command, NULL, 0, MXD_TRUMP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

	} else
	if ( mca->parameter_type == MXLV_MCA_CHANNEL_NUMBER ) {

		/* The relevant number is already in mca->channel_number,
		 * so just check to see if it is a sane value.
		 */

		if ( mca->channel_number >= mca->maximum_num_channels ) {
			unsigned long channel_number;

			channel_number = mca->channel_number;
			mca->channel_number = 0;

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested channel number %lu is outside the allowed range of (0-%lu) "
"for the MCA '%s'.", channel_number, mca->maximum_num_channels,
				mca->record->name );
		}

	} else {
		return mx_mca_default_set_parameter_handler( mca );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_ORTEC_UMCBI */


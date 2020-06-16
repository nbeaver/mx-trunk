/*
 * Name:    d_dante_mca.c
 *
 * Purpose: MX multichannel analyzer driver for the XGLabs DANTE MCA.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/* A summary of the available acquistion modes and the needed API calls.
 *
 * Acquisition modes:
 *   Normal DPP acquisition mode (single spectrum)
 *     configure
 *     start
 *     getData
 *   Waveform acquisition mode
 *     start_waveform
 *     getData
 *   List mode
 *     configure
 *     start_list
 *     getAvailableData
 *     getData
 *   List Wave mode
 *     configure
 *     start_listwave
 *     getAvailableData
 *     getData
 *   Mapping mode (multiple spectra)
 *     configure
 *     start_map
 *     getAvailableData
 *     getData
 *
 */

#define MXD_DANTE_MCA_DEBUG			TRUE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Vendor include file. */

#include "DLL_DPP_Callback.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_mca.h"

#include "i_dante.h"
#include "d_dante_mca.h"

#if MXD_DANTE_MCA_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_dante_mca_record_function_list = {
	mxd_dante_mca_initialize_driver,
	mxd_dante_mca_create_record_structures,
	mxd_dante_mca_finish_record_initialization,
	NULL,
	NULL,
	mxd_dante_mca_open,
	mxd_dante_mca_close,
	NULL,
	NULL,
	mxd_dante_mca_special_processing_setup
};

MX_MCA_FUNCTION_LIST mxd_dante_mca_mca_function_list = {
	mxd_dante_mca_arm,
	mxd_dante_mca_trigger,
	mxd_dante_mca_stop,
	mxd_dante_mca_read,
	mxd_dante_mca_clear,
	mxd_dante_mca_busy,
	mxd_dante_mca_get_parameter,
	mxd_dante_mca_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_dante_mca_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_DANTE_MCA_STANDARD_FIELDS
};

long mxd_dante_mca_num_record_fields
		= sizeof( mxd_dante_mca_record_field_defaults )
		  / sizeof( mxd_dante_mca_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_dante_mca_rfield_def_ptr
			= &mxd_dante_mca_record_field_defaults[0];

static mx_status_type mxd_dante_mca_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/* A private function for the use of the driver. */

static mx_status_type
mxd_dante_mca_get_pointers( MX_MCA *mca,
			MX_DANTE_MCA **dante_mca,
			MX_DANTE **dante,
			const char *calling_fname )
{
	static const char fname[] = "mxd_dante_mca_get_pointers()";

	MX_RECORD *mca_record, *dante_record;
	MX_DANTE_MCA *dante_mca_ptr;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}

	mca_record = mca->record;

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' is NULL.",
			calling_fname );
	}

	dante_mca_ptr = (MX_DANTE_MCA *) mca_record->record_type_struct;

	if ( dante_mca_ptr == (MX_DANTE_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE_MCA pointer for MCA record '%s' is NULL.",
			mca_record->name );
	}

	if ( dante_mca != (MX_DANTE_MCA **) NULL ) {
		*dante_mca = dante_mca_ptr;
	}

	if ( dante != (MX_DANTE **) NULL ) {
		dante_record = dante_mca_ptr->dante_record;

		if ( dante_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The dante_record pointer for record '%s' is NULL.",
				mca_record->name );
		}

		*dante = (MX_DANTE *) dante_record->record_type_struct;

		if ( (*dante) == (MX_DANTE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DANTE pointer for Handel record '%s' is NULL.",
				dante_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_dante_mca_initialize_driver( MX_DRIVER *driver )
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
mxd_dante_mca_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_dante_mca_create_record_structures()";

	MX_MCA *mca;
	MX_DANTE_MCA *dante_mca = NULL;

	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MCA structure." );
	}

	dante_mca = (MX_DANTE_MCA *) malloc( sizeof(MX_DANTE_MCA) );

	if ( dante_mca == (MX_DANTE_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_DANTE_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = dante_mca;
	record->class_specific_function_list =
				&mxd_dante_mca_mca_function_list;

	mca->record = record;
	dante_mca->record = record;

	dante_mca->child_mcs_record = NULL;

	dante_mca->mca_record_array_index = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mca_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_dante_mca_finish_record_initialization()";

	MX_MCA *mca;
	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed is NULL." );
	}

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));

	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_dante_mca_get_pointers( mca, &dante_mca, &dante, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;

	mx_status = mx_mca_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mca_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_dante_mca_open()";

	MX_MCA *mca;
	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	long i;
	mx_status_type mx_status;

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif
	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_dante_mca_get_pointers( mca, &dante_mca, &dante, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	/* Search for an empty slot in the MCA array. */

	for ( i = 0; i < dante->num_mcas; i++ ) {

		if ( dante->mca_record_array[i] == NULL ) {

			dante_mca->mca_record_array_index = i;

			dante->mca_record_array[i] = mca->record;

			break;		/* Exit the for() loop. */
		}
	}

	if ( i >= dante->num_mcas ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"There are too many MCA records for DANTE record '%s'.  "
		"DANTE record '%s' says that there are %ld MCAs, but "
		"MCA record '%s' would be MCA number %ld.",
			dante->record->name,
			dante->record->name,
			dante->num_mcas,
			dante_mca->record->name,
			i+1 );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mca_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_dante_mca_close()";

	MX_MCA *mca;
	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	MX_RECORD **mca_record_array;
	unsigned long i, num_mcas;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Delete our entry in mca_record_array. */

	num_mcas = dante->num_mcas;
	mca_record_array = dante->mca_record_array;

	for ( i = 0; i < num_mcas; i++ ) {

		if ( record == mca_record_array[i] ) {
			mca_record_array[i] = NULL;

			break;		/* Exit the for() loop. */
		}
	}

	/* Don't treat it as an error if we did not find an entry
	 * in mca_record_array.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mca_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxd_dante_mca_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(-2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_DANTE_MCA_CONFIGURE:
			record_field->process_function
					    = mxd_dante_mca_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_dante_mca_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxd_dante_mca_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_DANTE_MCA *dante_mca;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	dante_mca = (MX_DANTE_MCA *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		default:
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_DANTE_MCA_CONFIGURE:
			mx_status = mxd_dante_mca_configure( record );
			break;
		default:
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
		break;
	}

	return mx_status;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_dante_mca_configure( MX_RECORD *record )
{
	static const char fname[] = "mxd_dante_mca_configure()";

	MX_MCA *mca = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: Call into the configuration logic of the interface driver. */

	return mx_status;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_dante_mca_arm( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_arm()";

	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mca_trigger( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_trigger()";

	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mca_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_stop()";

	MX_DANTE_MCA *dante_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mca_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_read()";

	MX_DANTE_MCA *dante_mca;
	MX_DANTE *dante;
	mx_status_type mx_status;

#if MXI_DANTE_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
					&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxd_dante_mca_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_clear()";

	MX_DANTE_MCA *dante_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mca_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_busy()";

	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	mx_status_type mx_status;

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
					&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mca_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_get_parameter()";

	MX_DANTE_MCA *dante_mca;
	MX_DANTE *dante;
	mx_status_type mx_status;

#if MXI_DANTE_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
					&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_DANTE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));
#endif

#if MXI_DANTE_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	switch( mca->parameter_type ) {
	case MXLV_MCA_CHANNEL_NUMBER:

		/* These items are stored in memory and are not retrieved
		 * from the hardware.
		 */

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: mca->channel_number = %lu",
			fname, mca->channel_number));
#endif
		break;

	case MXLV_MCA_CURRENT_NUM_CHANNELS:

		mca->current_num_channels = 0;

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: mca->current_num_channels = %ld",
			fname, mca->current_num_channels));
#endif
		break;

	case MXLV_MCA_PRESET_TYPE:

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: mca->preset_type = %ld",
			fname, mca->preset_type));
#endif
		break;

	case MXLV_MCA_PRESET_REAL_TIME:

#if MXI_DANTE_DEBUG_GET_PRESETS
		MX_DEBUG(-2,("%s: mca->preset_real_time = %g",
			fname, mca->preset_real_time));
#endif
		break;

	case MXLV_MCA_PRESET_LIVE_TIME:

#if MXI_DANTE_DEBUG_GET_PRESETS
		MX_DEBUG(-2,("%s: mca->preset_live_time = %g",
			fname, mca->preset_live_time));
#endif
		break;

	case MXLV_MCA_PRESET_COUNT:

#if MXI_DANTE_DEBUG_GET_PRESETS
		MX_DEBUG(-2,("%s: mca->preset_count = %g",
			fname, mca->preset_count));
#endif
		break;

	case MXLV_MCA_CHANNEL_VALUE:
		mca->channel_value = mca->channel_array[ mca->channel_number ];

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: mca->channel_value = %lu",
			fname, mca->channel_value));
#endif
		break;

	case MXLV_MCA_ROI_ARRAY:
		break;

	case MXLV_MCA_ROI_INTEGRAL_ARRAY:
	case MXLV_MCA_ROI_INTEGRAL:

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: Reading roi_integral_array", fname));
#endif
		break;

	case MXLV_MCA_ROI:

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: mca->roi[0] = %lu", fname, mca->roi[0]));
		MX_DEBUG(-2,("%s: mca->roi[1] = %lu", fname, mca->roi[1]));
#endif
		break;
	
	case MXLV_MCA_REAL_TIME:

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: mca->real_time = %g",
			fname, mca->real_time));
#endif
		break;

	case MXLV_MCA_LIVE_TIME:

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: mca->live_time = %g",
			fname, mca->live_time));
#endif
		break;

	case MXLV_MCA_INPUT_COUNT_RATE:

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: $$$ mca->input_count_rate = %g",
			fname, mca->input_count_rate));
#endif
		break;

	case MXLV_MCA_OUTPUT_COUNT_RATE:

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: $$$ mca->output_count_rate = %g",
			fname, mca->output_count_rate));
#endif
		break;

	case MXLV_MCA_COUNTS:

#if MXI_DANTE_DEBUG
		MX_DEBUG(-2,("%s: mca->counts = %lu", fname, mca->counts));
#endif
		break;

	default:
		return mx_mca_default_get_parameter_handler( mca );
		break;
	}

#if MXI_DANTE_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "parameter type '%s' (%ld)",
		mx_get_field_label_string(mca->record, mca->parameter_type),
		mca->parameter_type );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mca_set_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_set_parameter()";

	MX_DANTE_MCA *dante_mca;
	MX_DANTE *dante;
	mx_status_type mx_status;

#if MXI_DANTE_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
					&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_DANTE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));
#endif

#if MXI_DANTE_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		break;

	case MXLV_MCA_PRESET_TYPE:
		break;

	case MXLV_MCA_PRESET_REAL_TIME:
		break;

	case MXLV_MCA_PRESET_LIVE_TIME:
		break;

	case MXLV_MCA_PRESET_COUNT:
		break;


	case MXLV_MCA_ROI_ARRAY:
		break;

	case MXLV_MCA_ROI:
		break;

	default:
		return mx_mca_default_set_parameter_handler( mca );
		break;
	}

#if MXI_DANTE_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "parameter type '%s' (%ld)",
		mx_get_field_label_string(mca->record, mca->parameter_type),
		mca->parameter_type );
#endif

	return mx_status;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_dante_mca_get_mca_array( MX_RECORD *dante_record,
				unsigned long *num_mcas,
				MX_RECORD ***mca_record_array )
{
	static const char fname[] = "mxd_dante_mca_get_mca_array()";

	MX_DANTE *dante;

	if ( dante_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dante_record pointer passed was NULL." );
	}
	if ( num_mcas == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_mcas pointer passed was NULL." );
	}
	if ( mca_record_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The mca_record_array pointer passed was NULL." );
	}

	dante = (MX_DANTE *) dante_record->record_type_struct;

	if ( dante == (MX_DANTE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE pointer for record '%s' is NULL.",
			dante_record->name );
	}

	*num_mcas = dante->num_mcas;
	*mca_record_array = dante->mca_record_array;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mca_get_rate_corrected_roi_integral( MX_MCA *mca,
			unsigned long roi_number,
			double *corrected_roi_value )
{
	static const char fname[] =
		"mxd_dante_mca_get_rate_corrected_roi_integral()";

	MX_DANTE_MCA *dante_mca = NULL;
	unsigned long mca_value;
	double corrected_value;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( corrected_roi_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The corrected_roi_error pointer passed was NULL." );
	}

	mx_status = mx_mca_get_roi_integral( mca->record,
						roi_number, &mca_value );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	corrected_value = (double) mca_value;

	/* FIXME */

	*corrected_roi_value = corrected_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mca_get_livetime_corrected_roi_integral( MX_MCA *mca,
			unsigned long roi_number,
			double *corrected_roi_value )
{
	static const char fname[] =
		"mxd_dante_mca_get_livetime_corrected_roi_integral()";

	MX_DANTE_MCA *dante_mca = NULL;
	unsigned long mca_value;
	double corrected_value;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( corrected_roi_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The corrected_roi_error pointer passed was NULL." );
	}

	mx_status = mx_mca_get_roi_integral( mca->record,
						roi_number, &mca_value );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	corrected_value = (double) mca_value;

	/* FIXME */

	*corrected_roi_value = corrected_value;

	return MX_SUCCESSFUL_RESULT;
}


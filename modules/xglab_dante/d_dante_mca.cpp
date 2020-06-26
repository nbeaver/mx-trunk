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
#include <string.h>

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
	mxd_dante_mca_finish_delayed_initialization,
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
	unsigned long i;
	mx_status_type mx_status;

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif
	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	MX_DEBUG(-2,("**** %s invoked for record '%s'. ****",
			fname, record->name));

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_dante_mca_get_pointers( mca, &dante_mca, &dante, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	/*---*/

	dante_mca->spectrum_data = (uint64_t *)
		calloc( MXU_DANTE_MCA_MAX_SPECTRUM_BINS, sizeof(uint64_t) );

	if ( dante_mca->spectrum_data == (uint64_t *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element array "
		"of 64-bit integers for MCA '%s'.",
			MXU_DANTE_MCA_MAX_SPECTRUM_BINS, mca->record->name );
	}

	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,
	("%s: spectrum_data = %p", fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: spectrum_data[0] = %lu", fname,
			(unsigned long) dante_mca->spectrum_data[0]));

	/*---*/

	switch( mca->maximum_num_channels ) {
	case 1024:
	case 2048:
	case 4096:
		mca->current_num_channels = mca->maximum_num_channels;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Dante MCA '%s' cannot be configured for %lu channels.  "
		"The only allowed values are 1024, 2048, and 4096.",
			mca->record->name, mca->maximum_num_channels );
		break;
	}

	/* Initialize the configuration parameters to the values that
	 * are set as the defaults in the vendor's header file.
	 */

	struct configuration *test_config = new struct configuration;

	memcpy( &(dante_mca->configuration), test_config,\
			sizeof(struct configuration) );

	delete test_config;

	/* Offset and TimestampDelay are not part of the configuration
	 * structure, so we initialize them separately here.
	 */

	dante_mca->offset = 0;
	dante_mca->timestamp_delay = 0;

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
mxd_dante_mca_finish_delayed_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_dante_mca_finish_delayed_initialization()";

	MX_DANTE_MCA *dante_mca = NULL;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));

	dante_mca = (MX_DANTE_MCA *) record->record_type_struct;

	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,
	("%s: spectrum_data = %p", fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,
	("%s: spectrum_data[0] = %lu", fname, dante_mca->spectrum_data[0]));

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
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	MXW_UNUSED( record );
	MXW_UNUSED( record_field );

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

MX_API mx_status_type
mxd_dante_mca_configure( MX_DANTE_MCA *dante_mca )
{
	static const char fname[] = "mxd_dante_mca_configure()";

	uint32_t call_id;
	uint16_t dante_error_code = DLL_NO_ERROR;
	bool dante_error_status;

	MX_DEBUG(-2,("%s: About to configure DANTE MCA '%s'.",
		fname, dante_mca->record->name ));

	dante_error_status = resetLastError();

	call_id = configure( dante_mca->channel_name,
				dante_mca->board_number,
				dante_mca->configuration );

	MX_DEBUG(-2,("%s: call_id = %lu",
		fname, (unsigned long) call_id));

	if ( call_id == 0 ) {
		dante_error_status = getLastError( dante_error_code );

		if ( dante_error_status == false ) {
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"After a call to configure() failed for "
			"record '%s', a call to getLastError() failed "
			"while trying to find out why configure() "
			"failed.",  dante_mca->record->name );

		}

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The call to configure() for record '%s' failed "
		"with DANTE error code %lu.",
			dante_mca->record->name,
			(unsigned long) dante_error_code );
	}

	mxi_dante_wait_for_answer( call_id );

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_dante_mca_arm( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_arm()";

	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	uint32_t call_id;
	uint16_t error_code;
	uint32_t new_other_param;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));

	/* Configure the gating for this MCA. */

	new_other_param = dante_mca->configuration.other_param;

	/* Replace the old gating bits with new gating bits. */

	new_other_param &= 0xfffffffc;

	if ( mca->trigger & MXF_DEV_EXTERNAL_TRIGGER ) {
		new_other_param &= 0x1;
	}
	if ( mca->trigger & MXF_DEV_TRIGGER_HIGH ) {
		new_other_param &= 0x2;
	}

	dante_mca->configuration.other_param = new_other_param;

	mx_status = mxd_dante_mca_configure( dante_mca );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the MCA in 'normal' mode. */

	(void) resetLastError();

	dante->dante_mode = MXF_DANTE_NORMAL_MODE;

	call_id = start( dante_mca->channel_name,
			mca->preset_real_time, mca->current_num_channels );

	if ( call_id == 0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"start() #1 returned an error for MCA '%s'.",
			mca->record->name );
	}

	mxi_dante_wait_for_answer( call_id );

	if ( mxi_dante_callback_data[0] == 1 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		getLastError( error_code );

		switch( error_code ) {
		case DLL_ARGUMENT_OUT_OF_RANGE:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"One of the arguments for start( '%s', %f, %lu ) "
			"is out of range for MCA '%s'.",
				dante_mca->channel_name,
				mca->preset_real_time,
				mca->current_num_channels,
				mca->record->name );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"start() #2 returned an error for MCA '%s'.  "
			"error_code = %lu",
			mca->record->name, (unsigned long) error_code );
		}
	}

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
	MX_DANTE *dante = NULL;
	uint32_t call_id;
	uint16_t dante_error_code = DLL_NO_ERROR;
	bool dante_error_status;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));

	call_id = stop( dante_mca->channel_name );

	if ( call_id == 0 ) {
		dante_error_status = getLastError( dante_error_code );

		if ( dante_error_status == false ) {
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"After a call to stop() failed for record '%s', "
			"a call to getLastError() failed while trying "
			"to find out why stop() failed.",
				dante_mca->record->name );
		}

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The call to stop() for record '%s' failed "
		"with DANTE error code %lu.",
			mca->record->name,
			(unsigned long) dante_error_code );
	}

	mxi_dante_wait_for_answer( call_id );

	if ( mxi_dante_callback_data[0] == 1 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The callback for stop() for record '%s' returned "
		"a failure statis in the callback data for some reason.",
			mca->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mca_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_read()";

	MX_DANTE_MCA *dante_mca;
	MX_DANTE *dante;
	long i;
	bool dante_status;
	uint64_t *spectrum_array = NULL;
	uint32_t spectrum_id;
	statistics stats;
	uint32_t spectrum_size;
	uint16_t error_code = DLL_NO_ERROR;
	mx_status_type mx_status;

#if MXI_DANTE_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
					&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	(void) resetLastError();

	spectrum_size = mca->current_num_channels;

#if 1
	spectrum_array = dante_mca->spectrum_data;
#endif
	MX_DEBUG(-2,("%s: before getData(), dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: before getData(), spectrum_array = %p",
		fname, spectrum_array));
	MX_DEBUG(-2,("%s: before getData(), spectrum_array[0] = %lu",
		fname, (unsigned long)spectrum_array[0] ));

	dante_status = getData( dante_mca->channel_name,
				dante_mca->board_number,
				spectrum_array,
				spectrum_id,
				stats,
				spectrum_size );

	MX_DEBUG(-2,("%s: after getData().", fname));

	if ( dante_status == false ) {
		getLastError( error_code );

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"getData() failed for MCA '%s'.  Error code = %lu",
			mca->record->name, (unsigned long) error_code );
	}

	mca->current_num_channels = spectrum_size;

	memset( mca->channel_array, 0,
		mca->maximum_num_channels * sizeof(unsigned long) );

	for ( i = 0; i < mca->current_num_channels; i++ ) {
		mca->channel_array[i] = spectrum_array[i];
	}

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxd_dante_mca_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_clear()";

	MX_DANTE_MCA *dante_mca = NULL;
	bool clear_status;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));

	/* FIXME: clear_chain() is not documented. clear() is documented
	 * but does not exist.
	 */

	clear_status = clear_chain( dante_mca->channel_name );

	if ( clear_status == false ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"clear() returned an error." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mca_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_busy()";

	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	uint32_t call_id;
	mx_status_type mx_status;

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
					&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));

	call_id = isRunning_system( dante_mca->channel_name,
					dante_mca->board_number );

	if ( call_id == 0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"isRunning() failed for MCA '%s'.",
			mca->record->name );
	}

	mxi_dante_wait_for_answer( call_id );

	if ( mxi_dante_callback_data[0] == 0 ) {
		mca->busy = FALSE;
	} else {
		mca->busy = TRUE;
	}

	MX_DEBUG(-2,("%s: '%s' busy = %d",
		fname, mca->record->name, (int) mca->busy));

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

	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));

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

	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));

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


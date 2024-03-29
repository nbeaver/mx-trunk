/*
 * Name:    d_dante_mca.c
 *
 * Purpose: MX multichannel analyzer driver for the XGLabs DANTE MCA.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020-2023 Illinois Institute of Technology
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

#define MXD_DANTE_MCA_TRACE_CALLS				FALSE

#define MXD_DANTE_MCA_DEBUG					FALSE

#define MXI_DANTE_DEBUG_PARAMETERS				FALSE

#define MXD_DANTE_MCA_DEBUG_POINTERS				FALSE

#define MXD_DANTE_MCA_DEBUG_FINISH_DELAYED_INITIALIZATION	FALSE

#define MXD_DANTE_MCA_DEBUG_READ				FALSE

#define MXD_DANTE_MCA_DEBUG_CONFIGURE				FALSE

#define MXD_DANTE_MCA_DEBUG_TIMING				FALSE

#define MXD_DANTE_MCA_DEBUG_ARM					FALSE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_inttypes.h"
#include "mx_callback.h"
#include "mx_process.h"
#include "mx_mcs.h"
#include "mx_mca.h"

#include "mx_hrt_debug.h"

#include "mx_dll_dpp_callback.h"

#include "i_dante.h"
#include "d_dante_mcs.h"
#include "d_dante_mca.h"

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

	dante_mca->mx_dante_configuration = NULL;

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

#if 0
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

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
	unsigned long board_number;
	mx_status_type mx_status;

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "vvvv %s invoked for record '%s'. vvvv\n",
			fname, record->name );
#endif

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_dante_mca_get_pointers( mca, &dante_mca, &dante, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->current_num_rois = mca->maximum_num_rois;

	mca->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;

	dante_mca->total_num_measurements = 0;

	/* Initialize a bunch of parameters to zero. */

	mca->roi[0] = 0;
	mca->roi[1] = 0;
	mca->roi_integral = 0;
	mca->channel_number = 0;
	mca->channel_value = 0;
	mca->roi_number = 0;
	mca->real_time = 0.0;
	mca->live_time = 0.0;
	mca->counts = 0;
	mca->preset_real_time = 0.0;
	mca->preset_live_time = 0.0;
	mca->preset_count = 0;
	mca->last_measurement_interval = -1.0;
	mca->preset_count_region[0] = 0;
	mca->preset_count_region[1] = 0;
	mca->energy_scale = 0.0;
	mca->energy_offset = 0.0;
	mca->input_count_rate = 0.0;
	mca->output_count_rate = 0.0;

	if ( dante_mca->dante_mca_flags & MXF_DANTE_MCA_TRACE_CALLS ) {
		dante_mca->trace_calls = TRUE;
	} else {
		dante_mca->trace_calls = FALSE;
	}

	if ( dante->trace_mca_calls ) {
		dante_mca->trace_calls = TRUE;
	} else {
		dante_mca->trace_calls = FALSE;
	}

#if MXD_DANTE_MCA_TRACE_CALLS
	dante_mca->trace_calls = TRUE;
#endif

	if( dante_mca->dante_mca_flags & MXF_DANTE_MCA_SHOW_FIRMWARE_AT_OPEN ) {
		mx_status = mxd_dante_mca_get_firmware_version( mca, TRUE );
	} else {
		mx_status = mxd_dante_mca_get_firmware_version( mca, FALSE );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	dante_mca->spectrum_data = (uint64_t *)
		calloc( MXU_DANTE_MCA_MAX_SPECTRUM_BINS, sizeof(uint64_t) );

	if ( dante_mca->spectrum_data == (uint64_t *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element array "
		"of 64-bit integers for MCA '%s'.",
			MXU_DANTE_MCA_MAX_SPECTRUM_BINS, mca->record->name );
	}

#if MXD_DANTE_MCA_DEBUG_POINTERS
	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,
	("%s: spectrum_data = %p", fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: spectrum_data[0] = %lu", fname,
			(unsigned long) dante_mca->spectrum_data[0]));
#endif

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

	/* Insert the MCA record into the matching slot number in the
	 * mca_record_array of the MX_DANTE structure.
	 */

	if ( dante_mca->board_number >= dante->num_mcas ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested board number (%lu) for Dante MCA '%s' "
		"is outside the allowed range from 0 to %ld.",
			dante_mca->board_number, dante_mca->record->name,
			dante->num_mcas - 1 );
	}

	board_number = dante_mca->board_number;

	if ( dante->mca_record_array[ board_number ] != NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"MCA board index %ld for Dante record '%s' "
		"is already in use by record '%s'.",
			board_number, dante->record->name,
		       dante->mca_record_array[ board_number ]->name );
	}	

	dante->mca_record_array[ board_number ] = mca->record;

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "^^^^ %s complete for record '%s' ^^^^\n",\
			fname, record->name );
#endif

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

#if MXD_DANTE_MCA_DEBUG_FINISH_DELAYED_INITIALIZATION
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
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mca_special_processing_setup( MX_RECORD *record )
{
#if 0
	static const char fname[] = "mxd_dante_mca_special_processing_setup()";
#endif

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

#if 0
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_DANTE_MCA_CONFIGURE:
		case MXLV_DANTE_MCA_FIRMWARE_VERSION:
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

	MX_RECORD *record = NULL;
	MX_RECORD_FIELD *record_field = NULL;
	MX_MCA *mca = NULL;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	mca = (MX_MCA *) record->record_class_struct;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA pointer for DANTE MCA record '%s' is NULL.",
			record->name );
	}

	MXW_UNUSED( record );
	MXW_UNUSED( record_field );

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_DANTE_MCA_FIRMWARE_VERSION:
			mx_status =
			    mxd_dante_mca_get_firmware_version( mca, FALSE );
			break;
		default:
			break;
		}
		break;
	case MX_PROCESS_PUT:
	switch( record_field->label_value ) {
		case MXLV_DANTE_MCA_FIRMWARE_VERSION:
			mx_status =
			    mxd_dante_mca_get_firmware_version( mca, TRUE );
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

MX_API mx_status_type
mxd_dante_mca_configure( MX_DANTE_MCA *dante_mca, MX_DANTE_MCS *dante_mcs )
{
	static const char fname[] = "mxd_dante_mca_configure()";

	MX_MCA *mca = NULL;
	MX_MCS *mcs = NULL;
	MX_RECORD *dante_record = NULL;
	MX_DANTE *dante = NULL;
	unsigned long board_number;
	uint32_t call_id;
	uint16_t dante_error_code = DLL_NO_ERROR;
	bool dante_error_status;
	MX_DANTE_CONFIGURATION *mx_dante_configuration = NULL;
	InputMode input_mode;
	GatingMode gating_mode;
	unsigned long dante_flags;
	mx_status_type mx_status;

	/* FIXME: We should probably test the consistency of the
	 * pointers passed to us, but not now.
	 *
	 * Note: The MX_DANTE_MCA pointer must not be NULL, but it is OK
	 * if the MX_DANTE_MCS pointer is NULL.
	 */

	if ( dante_mca == (MX_DANTE_MCA *) NULL ) {
		if ( dante_mcs == (MX_DANTE_MCS *) NULL ) {
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_DANTE_MCA pointer and the MX_DANTE_MCS "
			"pointer are both NULL.  There is nothing we "
			"can do in this case." );
		}

		dante_mca = (MX_DANTE_MCA *)
				dante_mcs->mca_record->record_type_struct;
	}

#if MXD_DANTE_MCA_TRACE_CALLS
	MX_DEBUG(-2,("%s invoked for record '%s'",
		fname, dante_mca->record->name ));
#endif

	mca = (MX_MCA *) dante_mca->record->record_class_struct;

	if ( dante_mcs == (MX_DANTE_MCS *) NULL ) {
		mcs = NULL;
	} else {
		mcs = (MX_MCS *) dante_mcs->record->record_class_struct;
	}

	dante_record = dante_mca->dante_record;

	if ( dante_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The dante_record pointer for MCA '%s' is NULL.",
			dante_mca->record->name );
	}

	dante = (MX_DANTE *) dante_record->record_type_struct;

	if ( dante == (MX_DANTE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE pointer for Dante record '%s' used by "
		"Dante MCA '%s' is NULL.",
			dante_record->name,
			dante_mca->record->name );
	}

	dante_flags = dante->dante_flags;

#if 0
	if ( dante_flags != 0 ) {
		MX_DEBUG(-2,("%s: '%s',  dante_flags = %#lx",
			fname, dante_mca->record->name, dante_flags));
	}
#endif

	/*---*/

	if ( dante_mca->mx_dante_configuration != NULL ) {
		mx_dante_configuration = dante_mca->mx_dante_configuration;
	} else {
		/* If dante_mca->mx_dante_configuration has not yet been
		 * initialized, then we must try to initialize it now.
		 */

		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The dante_mca->mx_dante_configuration pointer has not yet "
		"been initialized for Dante MCA '%s'.  Perhaps you should "
		"try again after MX has finished initializing itself.",
			dante_mca->record->name );
	}

	if ( mx_dante_configuration->mca_record == (MX_RECORD *) NULL ) {

#if 0
		MX_DEBUG(-2,("%s: Notice: Assigning MCA record '%s' "
		"to mx_dante_configuration %p.", fname,
			dante_mca->record->name, mx_dante_configuration));
#endif

		mx_dante_configuration->mca_record = dante_mca->record;
	}

#if MXD_DANTE_MCA_TRACE_CALLS
	(void) mxi_dante_show_parameters( dante_mca->record );
#endif

	if( dante_mca->dante_mca_flags & MXF_DANTE_MCA_VALIDATE_CONFIGURATION )
	{
	    mx_status = mxd_dante_mca_validate_configuration( mca );

	    if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
	}

#if 0
	MX_DEBUG(-2,("%s: About to configure DANTE MCA '%s'.",
		fname, dante_mca->record->name ));
#endif

	dante_error_status = resetLastError();

	/* Configure the standard acquisition parameters. */

	if ( dante_flags & MXF_DANTE_SET_BOARDS_TO_0xFF ) {
		board_number = 0xff;
	} else {
		board_number= dante_mca->board_number;
	}

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "%s: configure( '%s', %lu, configuration ) = ",
			fname, dante_mca->identifier, board_number );
#endif

	call_id = configure( dante_mca->identifier, board_number,
				mx_dante_configuration->configuration );

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "call_id %lu\n", (unsigned long) call_id );
#endif

#if MXD_DANTE_MCA_DEBUG_CONFIGURE
	MX_DEBUG(-2,("%s: Configuring MCA '%s' with call_id = %lu",
		fname, dante_mca->record->name, (unsigned long) call_id));
#endif

	if ( call_id == 0 ) {
		dante_error_status = getLastError( dante_error_code );

		if ( dante_error_status == false ) {
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"After a call to configure() failed for "
			"record '%s', a call to getLastError() failed "
			"while trying to find out why configure() "
			"failed.",  dante_mca->record->name );

		}

		switch ( dante_error_code ) {
		case DLL_WRONG_ID:
			return mx_error( MXE_NOT_FOUND, fname,
			"The MX configuration for MCA '%s' said that "
			"it had board id %lu.  But Dante itself "
			"did not recognize %lu as a known board id.",
				dante_mca->record->name,
				dante_mca->board_number,
				dante_mca->board_number );
			break;

		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"The call to configure() for record '%s' failed "
			"with DANTE error code %lu.",
				dante_mca->record->name,
				(unsigned long) dante_error_code );
			break;
		}
	}

	mxi_dante_wait_for_answer( call_id, dante );

	/* Tell DANTE that we only want internal triggered mode. */

	if ( mca->trigger_mode & MXF_DEV_EXTERNAL_TRIGGER ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"External triggering mode has been requested for MCA '%s', "
		"but only 'normal' (single spectrum) mode is available "
		"for the 'dante_mca' driver.  If you want a single spectrum "
		"with external triggering, try the 'dante_mcs' driver "
		"instead and configure that driver to take only "
		"one measurement.", mca->record->name );
	}

	/* Configure the front-end stage. */

	if ( mx_strcasecmp( "DC_HighImp", dante_mca->input_mode_name ) == 0 ) {
		input_mode = DC_HighImp;
	} else
	if ( mx_strcasecmp( "DC_LowImp", dante_mca->input_mode_name ) == 0 ) {
		input_mode = DC_LowImp;
	} else
	if ( mx_strcasecmp( "AC_Slow", dante_mca->input_mode_name ) == 0 ) {
		input_mode = AC_Slow;
	} else
	if ( mx_strcasecmp( "AC_Fast", dante_mca->input_mode_name ) == 0 ) {
		input_mode = AC_Fast;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal front-end input mode '%s' requested for MCA '%s'.  "
		"The allowed input modes are 'DC_HighImp', 'DC_LowImp', "
		"'AC_Slow', and 'AC_Fast'.",
			dante_mca->input_mode_name,
			dante_mca->record->name );
	}

	mx_dante_configuration->input_mode = input_mode;

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "%s: configure_input( '%s', %lu, %lu ) = ",
		fname, dante_mca->identifier, board_number, input_mode );
#endif

	call_id = configure_input( dante_mca->identifier,
				board_number, input_mode );

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "call_id %lu\n", call_id );
#endif

	mxi_dante_wait_for_answer( call_id, dante );

	/****** Configure gating. ******/

	/* If we were not passed an MX_DANTE_MCS pointer, then we are
	 * configuring the MCA for acquiring a single spectrum.  In
	 * this situation, the only valid MCA mode is internal trigger
	 * mode with the DANTE gating set to FreeRunning.
	 */

	if ( dante_mcs == (MX_DANTE_MCS *) NULL ) {
	    mca->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;

	    gating_mode = FreeRunning;
	} else {
	    /* MX_DANTE_MCS was not NULL, so we must configure
	     * for an MCS mode.
	     */

	    if ( mcs->trigger_mode & MXF_DEV_INTERNAL_TRIGGER ) {
		gating_mode = FreeRunning;
	    } else
	    if ( mcs->trigger_mode & MXF_DEV_EXTERNAL_TRIGGER ) {
		if ( mcs->trigger_mode & MXF_DEV_EDGE_TRIGGER ) {
		    if ( mcs->trigger_mode & MXF_DEV_TRIGGER_HIGH ) {
			if ( mcs->trigger_mode & MXF_DEV_TRIGGER_LOW ) {
			    gating_mode = TriggerBoth;
			} else {
			    gating_mode = TriggerRising;
			}
		    } else
		    if ( mcs->trigger_mode & MXF_DEV_TRIGGER_LOW ) {
			gating_mode = TriggerFalling;
		    } else {
			/* If edge triggering was requested, but the edge
			 * to use was not specified, then we assume that
			 * we should be using a rising trigger.
			 */
			gating_mode = TriggerRising;
		    }
		} else
		if ( mcs->trigger_mode & MXF_DEV_LEVEL_TRIGGER ) {
		    if ( mcs->trigger_mode & MXF_DEV_TRIGGER_HIGH ) {
			gating_mode = GatedHigh;
		    } else
		    if ( mcs->trigger_mode & MXF_DEV_TRIGGER_LOW ) {
			gating_mode = GatedLow;
		    } else {
			/* If level triggering was requested, but the level
			 * to use was not specified, then we assume that
			 * we should be using a gated high trigger.
			 */
			gating_mode = GatedHigh;
		    }
		} else {
		    return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported trigger mode %#lx (%lu) requested for MCS '%s'.",
		    mcs->trigger_mode, mcs->trigger_mode, mcs->record->name );
		}
	    } else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported trigger mode %#lx (%lu) requested for MCS '%s'.",
		    mcs->trigger_mode, mcs->trigger_mode, mcs->record->name );
	    }
	}

	mx_dante_configuration->gating_mode = gating_mode;

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "%s: configure_gating( '%s', %lu, %lu ) = ",
		fname, dante_mca->identifier, gating_mode, board_number );
#endif

	call_id = configure_gating( dante_mca->identifier,
					gating_mode, board_number );

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "call_id = %lu\n", call_id );
#endif

	mxi_dante_wait_for_answer( call_id, dante );

	/* Finish by setting up the configuration offsets. */

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "%s: configure_offset( '%s', %lu, (%lu, %lu) ) = ",
		fname, dante_mca->identifier, board_number,
		mx_dante_configuration->cfg_offset.offset_val1,
		mx_dante_configuration->cfg_offset.offset_val2 );
#endif

	call_id = configure_offset( dante_mca->identifier,
		board_number, mx_dante_configuration->cfg_offset );

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "call_id = %lu\n", call_id );
#endif

	mxi_dante_wait_for_answer( call_id, dante );

#if MXD_DANTE_MCA_TRACE_CALLS
	MX_DEBUG(-2,("%s complete for record '%s'",
		fname, dante_mca->record->name ));
#endif

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
	unsigned long i;
	mx_status_type mx_status;

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING configure_measurement;
	MX_HRT_TIMING start_measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DANTE_MCA_DEBUG_ARM
	MX_DEBUG(-2,("%s invoked for MCA '%s'.", fname, mca->record->name ));
#endif

#if 0
	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));
#endif

	/* Command the MCA to stop. The Dante documentation states
	 * that you have to do this even if the previous acquisition
	 * sequence has completed.
	 */

#if MXD_DANTE_MCA_DEBUG_ARM
	MX_DEBUG(-2,("%s: before call to mxd_dante_mca_stop()", fname));
#endif

	mx_status = mxd_dante_mca_stop( mca );

#if MXD_DANTE_MCA_DEBUG_ARM
	MX_DEBUG(-2,("%s: after call to mxd_dante_mca_stop()", fname));
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->old_busy = FALSE;

	/* Configure the MCA for 'normal' mode. */

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_START( configure_measurement );
#endif

#if MXD_DANTE_MCA_DEBUG_ARM
	MX_DEBUG(-2,
	    ("%s: before call to mxd_dante_mca_configure()", fname));
#endif

	mx_status = mxd_dante_mca_configure( dante_mca, NULL );

#if MXD_DANTE_MCA_DEBUG_ARM
	MX_DEBUG(-2,
	    ("%s: after call to mxd_dante_mca_configure()", fname));
#endif

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_END( configure_measurement );
	MX_HRT_RESULTS( configure_measurement, fname, "configure" );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the MCA in 'normal' mode. */

	MX_DEBUG(-2,("%s: invoking start( '%s', %f, %ld )",
		fname, dante_mca->identifier,
		mca->preset_real_time,
		mca->current_num_channels ));

	for ( i = 0; i < dante_mca->max_attempts; i++ ) {
		(void) resetLastError();

		dante->dante_mode = MXF_DANTE_NORMAL_MODE;

#if 0 || MXD_DANTE_MCA_TRACE_CALLS
		fprintf( stderr, "%s: start( '%s', %f, %ld ) = ",
			fname, dante_mca->identifier,
			mca->preset_real_time,
			mca->current_num_channels );
#endif

#if MXD_DANTE_MCA_DEBUG_TIMING
		MX_HRT_START( start_measurement );
#endif

		call_id = start( dante_mca->identifier,
				mca->preset_real_time, 
				mca->current_num_channels );

#if MXD_DANTE_MCA_DEBUG_TIMING
		MX_HRT_END( start_measurement );
		MX_HRT_RESULTS( start_measurement, fname,
				"start(), i = %lu", i );
#endif

#if 0 || MXD_DANTE_MCA_TRACE_CALLS
		fprintf( stderr, "call_id %lu\n", (unsigned long) call_id );
#endif

		if ( call_id != 0 )
			break;

		if ( dante_mca->dante_mca_flags
				& MXF_DANTE_MCA_ALLOW_CALLID_ZERO ) {
		    mx_warning(
			"start( '%s', %f, %lu ) for MCA '%s'.  returned "
			"a call_id of 0.  The Dante API manual says this "
			"is an error, but we are continuing anyway.",
				dante_mca->identifier,
				mca->preset_real_time,
				mca->current_num_channels,
				mca->record->name );

		    break;     /* Break out of the for( i ) loop. */
		} else {
		    return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "start() returned a 'call_id == 0 ' error for MCA '%s'.  "
		    "We attempted to run start() %lu times.  "
		    "This means that 'something' is wrong with the parameters "
		    "passed to start().  Those parameters were\n"
		    "    dante_mca->identifier = '%s'\n"
		    "    mca->preset_real_time = %f\n"
		    "    mca->current_num_channels = %lu "
		    "    dante_mca->total_num_measurements = %lu",
			mca->record->name,
			dante_mca->max_attempts,
			dante_mca->identifier,
			mca->preset_real_time,
			mca->current_num_channels,
			dante_mca->total_num_measurements );
		}

		mx_msleep( dante_mca->attempt_delay_ms );
	}

	if ( i > 0 ) {
		MX_DEBUG(-2,("%s: %lu retries were performed.", fname, i));
	}

#if 0
	MX_DEBUG(-2,("%s: start() returned call_id = %lu",
			fname, (unsigned long) call_id ));
#endif

	mxi_dante_wait_for_answer( call_id, dante );

	if ( mxi_dante_callback_data[0] != 1 ) {

		getLastError( error_code );

		switch( error_code ) {
		case DLL_ARGUMENT_OUT_OF_RANGE:
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"start() returned a DLL_ARGUMENT_OUT_OF_RANGE "
			"error for MCA '%s'.  "
			"The parameters for the call were\n"
			"    dante_mca->identifier = '%s'\n"
			"    mca->preset_real_time = %f\n"
			"    mca->current_num_channels = %lu",
				mca->record->name,
				dante_mca->identifier,
				mca->preset_real_time,
				mca->current_num_channels );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"start() returned an unexpected "
			"error code (%d) for MCA '%s'.  "
			"The parameters for the call were\n"
			"    dante_mca->identifier = '%s'\n"
			"    mca->preset_real_time = %f\n"
			"    mca->current_num_channels = %lu",
				error_code,
				mca->record->name,
				dante_mca->identifier,
				mca->preset_real_time,
				mca->current_num_channels );
			break;
		}
	}

	/* Mark all of the MCA channels belonging to this Dante board chain
	 * as having 'new_data_available' == TRUE.
	 */

#if 0
	MX_DEBUG(-2,
	  ("%s: mxi_dante_wait_for_answer() completed successfully", fname));
#endif

	mca->old_busy = -1;
	mca->busy = -1;

#if MXD_DANTE_MCA_DEBUG_ARM
	MX_DEBUG(-2,("%s: '%s' busy = %d",
		fname, mca->record->name, (int) mca->busy));
#endif

	mx_status = mxi_dante_set_data_available_flag_for_chain( mca->record,
									TRUE );

#if MXD_DANTE_MCA_DEBUG_ARM
	MX_DEBUG(-2,("%s complete for MCA '%s'.", fname, mca->record->name ));
#endif
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

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING stop_measurement;
#endif

#if 0
	MX_HRT_TIMING wait_measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));
#endif

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "%s: stop( '%s' ) = ", fname, dante_mca->identifier );
#endif

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_START( stop_measurement );
#endif

	call_id = stop( dante_mca->identifier );

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_END( stop_measurement );
	MX_HRT_RESULTS( stop_measurement, fname, "stop" );
#endif

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "call_id %lu\n", call_id );
#endif

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

#if 0
	MX_DEBUG(-2,("%s: stop() returned call_id = %lu",
				fname, (unsigned long) call_id));

	MX_DEBUG(-2,("%s: Skipping wait for call_id = %lu",
				fname, (unsigned long) call_id));
#endif

#if 0
	MX_HRT_START( wait_measurement );

	mxi_dante_wait_for_answer( call_id, dante );

	MX_HRT_END( wait_measurement );
	MX_HRT_RESULTS( wait_measurement, fname, "mxi_dante_wait_for_answer()");

	if ( mxi_dante_callback_data[0] == 1 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The callback for stop() for record '%s' returned "
		"a failure statis in the callback data for some reason.",
			mca->record->name );
	}
#endif

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

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
					&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	(void) resetLastError();

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "%s: resetLastError()\n", fname );
#endif

	spectrum_size = mca->current_num_channels;

	spectrum_array = dante_mca->spectrum_data;

#if MXD_DANTE_MCA_DEBUG_READ
	MX_DEBUG(-2,("%s: before getData(), dante_mca = '%s'",
				fname, mca->record->name ));
	MX_DEBUG(-2,("%s: before getData(), dante_mca->identifier = '%s'",
				fname, dante_mca->identifier));
	MX_DEBUG(-2,("%s: before getData(), dante_mca->board_number = '%lu'",
			fname, (unsigned long) dante_mca->board_number ));
	MX_DEBUG(-2,("%s: before getData(), spectrum_array = %p",
		fname, spectrum_array));
	MX_DEBUG(-2,("%s: before getData(), spectrum_array[0] = %lu",
		fname, (unsigned long)spectrum_array[0] ));
#if 0
	MX_DEBUG(-2,("%s: before getData(), spectrum_id = %p",
		fname, spectrum_id));
	MX_DEBUG(-2,("%s: before getData(), stats = %p",
		fname, stats));
#endif
	MX_DEBUG(-2,("%s: before getData(), spectrum_size = %lu",
			fname, (unsigned long) spectrum_size ));
#endif /* MXD_DANTE_MCA_DEBUG_READ */

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr,
	"%s: getData( '%s', %lu, spectrum_array, spectrum_id, stats, %lu ) = ",
	fname, dante_mca->identifier, dante_mca->board_number, spectrum_size );
#endif

	dante_status = getData( dante_mca->identifier,
				dante_mca->board_number,
				spectrum_array,
				spectrum_id,
				stats,
				spectrum_size );

#if 0
	MX_DEBUG(-2,("%s: after getData(), dante_status = %d",
		fname, (int) dante_status));
	MX_DEBUG(-2,("%s: after getData(), spectrum_id = %lu",
			fname, (unsigned long) spectrum_id ));
#endif

	if ( dante_status == false ) {

#if MXD_DANTE_MCA_TRACE_CALLS
		fprintf( stderr, "false\n" );
#endif
		getLastError( error_code );

		switch( error_code ) {
		case DLL_ARGUMENT_OUT_OF_RANGE:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"One or more of the arguments to getData() "
			"for DANTE MCA '%s' was out of range, with "
			"error code DLL_ARGUMENT_OUT_OF_RANGE (%lu).",
				mca->record->name,
				(unsigned long) error_code );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"getData() failed for MCA '%s'.  Error code = %lu",
				mca->record->name, (unsigned long) error_code );
			break;
		}
	}
#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "true\n" );
	fprintf( stderr, "%s: spectrum_id = %lu\n", fname, spectrum_id );
	fprintf( stderr, "%s: statistics:\n", fname );
	fprintf( stderr,
		"%s:   real_time = %" PRIu64 ", live_time = %" PRIu64 "\n",
			fname, stats.real_time, stats.live_time );
	fprintf( stderr,
		"%s:   ICR = %f, OCR = %f, last_timestamp = %" PRIu64 "\n",
			fname, stats.ICR, stats.OCR, stats.last_timestamp );
	fprintf( stderr, "%s:   detected = %lu, measured = %lu\n",
			fname, stats.detected, stats.measured );
	fprintf( stderr, "%s:   edge_dt = %lu, filt1_dt = %lu\n",
			fname, stats.edge_dt, stats.filt1_dt );
	fprintf( stderr, "%s:   zerocounts = %lu, baselines_value = %lu\n",
			fname, stats.zerocounts, stats.baselines_value );
	fprintf( stderr, "%s:   pup_value = %lu, pup_f1_value = %lu\n",
			fname, stats.pup_value, stats.pup_f1_value );
	fprintf( stderr,
		"%s:   pup_notf1_value = %lu, reset_counter_value = %lu\n",
		fname, stats.pup_notf1_value, stats.reset_counter_value );
#endif

#if 0
	{
		int i;
		unsigned long data_value;

		fprintf( stderr,
		"\nPrinting spectrum values from bin 450 to 650:\n" );

		for ( i = 450; i <= 650; i++ ) {
			data_value = (unsigned long) spectrum_array[i];
			fprintf( stderr, "%lu  ", data_value );
		}

		fprintf( stderr, "\n" );
	}
#endif

	dante_mca->total_num_measurements++;

	/* Copy the spectrum data to the standard mca->channel_array. */

	mca->current_num_channels = spectrum_size;

	memset( mca->channel_array, 0,
		mca->maximum_num_channels * sizeof(unsigned long) );

	for ( i = 0; i < mca->current_num_channels; i++ ) {
		mca->channel_array[i] = spectrum_array[i];
	}

	/* Save the real time, live time, icr, and ocr that were reported
	 * in the stats structure by the call to getData().
	 */

	mca->real_time = 1.0e-6 * (double) stats.real_time;
	mca->live_time = 1.0e-6 * (double) stats.live_time;
	mca->input_count_rate = 1.0e3 * (double) stats.ICR;
	mca->output_count_rate = 1.0e3 * (double) stats.OCR;

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxd_dante_mca_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_clear()";

	MX_DANTE_MCA *dante_mca = NULL;
	uint16_t dante_error_code = DLL_NO_ERROR;
	bool clear_status;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca,
						&dante_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));
#endif

	/* FIXME: clear_chain() is not documented. clear() is documented
	 * but does not exist.
	 */

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "%s: clear_chain( '%s' ) = ",
		fname, dante_mca->identifier );
#endif

	clear_status = clear_chain( dante_mca->identifier );

	if ( clear_status == false ) {

		(void) getLastError( dante_error_code );

#if MXD_DANTE_MCA_TRACE_CALLS
		fprintf( stderr, "false\n" );
#endif
		switch( dante_error_code ) {
		case DLL_UNSUPPORTED_BY_FIRMWARE:
			return mx_error(MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
			"The firmware for DANTE chain '%s' used by MCA '%s' "
			"is either not loaded or is corrupted.",
				dante_mca->identifier, mca->record->name );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"clear() returned an error.  DANTE error code = %lu",
			(unsigned long) dante_error_code );
			break;
		}
	}

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "true\n" );
#endif

	/* Mark all of the MCA channels belonging to this Dante board chain
	 * as having 'new_data_available' == FALSE.
	 */

	mx_status = mxi_dante_set_data_available_flag_for_chain( mca->record,
									FALSE );
	return MX_SUCCESSFUL_RESULT;
}

#if 0
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

#if 0
	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));
#endif

	call_id = isRunning_system( dante_mca->identifier,
					dante_mca->board_number );

	if ( call_id == 0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"isRunning_system() failed for MCA '%s'.",
			mca->record->name );
	}

	mxi_dante_wait_for_answer( call_id, dante );

	if ( mxi_dante_callback_data[0] == 0 ) {
		mca->busy = FALSE;
	} else {
		mca->busy = TRUE;
	}

#if 0
	MX_DEBUG(-2,("%s: isRunning_system() callback data = %lu, "
	"hrt = %f seconds",
		fname, (unsigned long) mxi_dante_callback_data[0],
		mx_high_resolution_time_as_double() ));
#endif

#if 0
	MX_DEBUG(-2,("%s: '%s' old_busy = %d, busy = %d",
		fname, mca->record->name,
		mca->old_busy, mca->busy));
#endif

	if ( mca->busy != mca->old_busy ) {
		MX_DEBUG(-2,("%s: '%s' busy = %d",
			fname, mca->record->name, (int) mca->busy));
	}

	return MX_SUCCESSFUL_RESULT;
}

#else

MX_EXPORT mx_status_type
mxd_dante_mca_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_busy()";

	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	bool dll_status = false;
	bool last_data_received = false;
	mx_status_type mx_status;

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
					&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));
#endif

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr,
	"%s: isLastDataReceived( '%s', %lu, last_data_received ) = ",
		fname, dante_mca->identifier, dante_mca->board_number );
#endif

	dll_status = isLastDataReceived( dante_mca->identifier,
					dante_mca->board_number,
					last_data_received );

	if ( dll_status == false ) {

#if MXD_DANTE_MCA_TRACE_CALLS
		fprintf( stderr, "false\n" );
#endif
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"isLastDataReceived() failed for MCA '%s'.",
			mca->record->name );
	}

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "true\n" );

	if ( last_data_received ) {
		fprintf( stderr, "last_data_received = true\n" );
	} else {
		fprintf( stderr, "last_data_received = false\n" );
	}
#endif

	mca->old_busy = mca->busy;

	if ( last_data_received ) {
		mca->busy = FALSE;
	} else {
		mca->busy = TRUE;
	}

#if 0
	MX_DEBUG(-2,("%s: isLastDataReceived() callback data = %lu, "
	"hrt = %f seconds",
		fname, (unsigned long) mxi_dante_callback_data[0],
		mx_high_resolution_time_as_double() ));
#endif

#if 0
	MX_DEBUG(-2,("%s: '%s' old_busy = %d, busy = %d",
		fname, mca->record->name,
		mca->old_busy, mca->busy));
#endif

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "%s: mca->busy = %lu\n", fname, mca->busy );
#endif

	if ( mca->busy != mca->old_busy ) {
		MX_DEBUG(-2,("%s: '%s' busy = %d",
			fname, mca->record->name, (int) mca->busy));
	}

	return MX_SUCCESSFUL_RESULT;
}
#endif

MX_EXPORT mx_status_type
mxd_dante_mca_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_get_parameter()";

	MX_DANTE_MCA *dante_mca;
	MX_DANTE *dante;
	mx_status_type mx_status;

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
					&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));
#endif

#if MXI_DANTE_DEBUG_PARAMETERS
	MX_DEBUG(-2,("%s invoked for MCA '%s', parameter '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));
#endif

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

		/* These items are stored in memory and are not retrieved
		 * from the hardware.
		 */

	switch( mca->parameter_type ) {
	case MXLV_MCA_REAL_TIME:
	case MXLV_MCA_LIVE_TIME:
	case MXLV_MCA_INPUT_COUNT_RATE:
	case MXLV_MCA_OUTPUT_COUNT_RATE:

		/* These values are set after getData() is called by
		 * mxd_dante_mca_read(), * so we do not do anything here.
		 */

		break;
	default:
		return mx_mca_default_get_parameter_handler( mca );
		break;
	}

#if MXD_DANTE_MCA_DEBUG_TIMING
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

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_dante_mca_get_pointers( mca,
					&dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data = %p",
				fname, dante_mca->spectrum_data));
	MX_DEBUG(-2,("%s: dante_mca->spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));
#endif

#if MXI_DANTE_DEBUG_PARAMETERS
	MX_DEBUG(-2,("%s invoked for MCA '%s', parameter '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));
#endif

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	switch( mca->parameter_type ) {
	default:
		return mx_mca_default_set_parameter_handler( mca );
		break;
	}

#if MXD_DANTE_MCA_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "parameter type '%s' (%ld)",
		mx_get_field_label_string(mca->record, mca->parameter_type),
		mca->parameter_type );
#endif

	return mx_status;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_dante_mca_validate_configuration( MX_MCA *mca )
{
	static const char fname[] = "mxd_dante_mca_validate_configuration()";

	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	MX_DANTE_CONFIGURATION *mx_dante_configuration = NULL;
	struct configuration *configuration = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca, &dante_mca, &dante, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_dante_configuration = dante_mca->mx_dante_configuration;

	if ( mx_dante_configuration == (MX_DANTE_CONFIGURATION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_DANTE_CONFIGURATION pointer for Dante MCA '%s' is NULL.",
			dante_mca->record->name );
	}

	configuration = &(mx_dante_configuration->configuration);

	if ( configuration->max_peaking_time == 0 ) {
		mx_warning( "The max_peaking_time for Dante MCA '%s' is 0.",
			dante_mca->record->name );
	}
	if ( configuration->peaking_time > configuration->max_peaking_time ) {
		mx_warning( "The peaking_time (%lu) for Dante MCA '%s' "
			"is greater than the max_peaking_time (%lu).",
			(unsigned long) configuration->peaking_time,
			dante_mca->record->name,
			(unsigned long) configuration->max_peaking_time );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_dante_mca_get_firmware_version( MX_MCA *mca,
				mx_bool_type log_firmware_version )
{
	static const char fname[] = "mxd_dante_mca_get_firmware_version()";

	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;

	uint32_t call_id;
	uint16_t dante_error_code = DLL_NO_ERROR;
	mx_status_type mx_status;

	mx_status = mxd_dante_mca_get_pointers( mca, &dante_mca, &dante, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Detect the firmware used by this board. */

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "%s: getFirmware( '%s', 0 ) = ",
		fname, dante_mca->identifier, dante_mca->board_number );
#endif

	call_id = getFirmware( dante_mca->identifier,
				dante_mca->board_number );

#if MXD_DANTE_MCA_TRACE_CALLS
	fprintf( stderr, "call_id %lu\n", call_id );
#endif

	if ( call_id == 0 ) {
		(void) getLastError( dante_error_code );

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Getting the firmware version for MCA '%s' failed "
		"with DANTE error code %lu.",
			mca->record->name, (unsigned long) dante_error_code );
	}

	mxi_dante_wait_for_answer( call_id, dante );

#if 0
	int i;

	fprintf( stderr, "getFirmware() callback data = " );

	for ( i = 0; i < MXU_DANTE_MAX_CALLBACK_DATA_LENGTH; i++ ) {
		fprintf( stderr, "%lu ",
			(unsigned long) mxi_dante_callback_data[i] );
	}

	fprintf( stderr, "\n" );
#endif

	dante_mca->firmware_version = 1000000 * mxi_dante_callback_data[0]
				      + 10000 * mxi_dante_callback_data[1]
				      +   100 * mxi_dante_callback_data[2]
				      +         mxi_dante_callback_data[3];

	if ( log_firmware_version ) {
	    char firmware_type[40];

	    switch( mxi_dante_callback_data[3] ) {
	    case 1:
		    strlcpy( firmware_type, "LE", sizeof(firmware_type) );
		    break;
	    case 2:
		    strlcpy( firmware_type, "HR", sizeof(firmware_type) );
		    break;
	    default:
		    snprintf( firmware_type, sizeof(firmware_type),
			"'Unknown type %lu'",
			(unsigned long) mxi_dante_callback_data[3] );
		    break;
	    }

	    MX_DEBUG(-2,("%s: '%s', firmware_version = (%lu.%lu.%lu %s)",
		fname, dante_mca->record->name,
		(unsigned long) mxi_dante_callback_data[0],
		(unsigned long) mxi_dante_callback_data[1],
		(unsigned long) mxi_dante_callback_data[2],
		firmware_type ));
	}

	return MX_SUCCESSFUL_RESULT;
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


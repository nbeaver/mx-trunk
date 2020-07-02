/*
 * Name:    d_dante_mcs.c 
 *
 * Purpose: MX multichannel scaler driver for XGLab DANTE devices used
 *          in mapping mode.
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

#define MXD_DANTE_MCS_DEBUG_MONITOR_THREAD		TRUE

#define MXD_DANTE_MCS_DEBUG_MONITOR_THREAD_BUFFERS	TRUE

#define MXD_DANTE_MCS_DEBUG_BUSY			TRUE

#define MXD_DANTE_MCS_DEBUG_OPEN			TRUE

#define MXD_DANTE_MCS_DEBUG				TRUE

#define MXD_DANTE_MCS_DEBUG				TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_atomic.h"
#include "mx_mca.h"
#include "mx_mcs.h"

/* Vendor include file. */

#include "DLL_DPP_Callback.h"

#include "i_dante.h"
#include "d_dante_mca.h"
#include "d_dante_mcs.h"

/* Initialize the mcs driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_dante_mcs_record_function_list = {
	mxd_dante_mcs_initialize_driver,
	mxd_dante_mcs_create_record_structures,
	mxd_dante_mcs_finish_record_initialization,
	NULL,
	NULL,
	mxd_dante_mcs_open
};

MX_MCS_FUNCTION_LIST mxd_dante_mcs_mcs_function_list = {
	mxd_dante_mcs_arm,
	mxd_dante_mcs_trigger,
	mxd_dante_mcs_stop,
	mxd_dante_mcs_clear,
	mxd_dante_mcs_busy,
	NULL,
	mxd_dante_mcs_read_all,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_dante_mcs_get_parameter,
	mxd_dante_mcs_set_parameter,
	mxd_dante_mcs_get_last_measurement_number,
	mxd_dante_mcs_get_total_num_measurements
};

MX_RECORD_FIELD_DEFAULTS mxd_dante_mcs_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_DANTE_MCS_STANDARD_FIELDS
};

long mxd_dante_mcs_num_record_fields
		= sizeof( mxd_dante_mcs_record_field_defaults )
			/ sizeof( mxd_dante_mcs_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_dante_mcs_rfield_def_ptr
			= &mxd_dante_mcs_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_dante_mcs_get_pointers( MX_MCS *mcs,
			MX_DANTE_MCS **dante_mcs,
			MX_MCA **mca,
			MX_DANTE_MCA **dante_mca,
			MX_DANTE **dante,
			const char *calling_fname )
{
	static const char fname[] = "mxd_dante_mcs_get_pointers()";

	MX_RECORD *mcs_record = NULL;
	MX_DANTE_MCS *dante_mcs_ptr = NULL;

	MX_RECORD *mca_record = NULL;
	MX_DANTE_MCA *dante_mca_ptr = NULL;

	MX_RECORD *dante_record = NULL;

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	mcs_record = mcs->record;

	if ( mcs_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	dante_mcs_ptr = (MX_DANTE_MCS *) mcs_record->record_type_struct;

	if ( dante_mcs_ptr == (MX_DANTE_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE_MCS pointer for MCS record '%s' is NULL.",
			mcs_record->name );
	}

	if ( dante_mcs != (MX_DANTE_MCS **) NULL ) {
		*dante_mcs = dante_mcs_ptr;
	}

	mca_record = dante_mcs_ptr->mca_record;

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mca_record pointer for DANTE MCS '%s' is NULL.",
			mcs_record->name );
	}

	if ( mca != (MX_MCA **) NULL ) {
		*mca = (MX_MCA *) mca_record->record_class_struct;

		if ( (*mca) == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCA pointer for MCA '%s' used by "
			"DANTE MCS '%s' is NULL.",
			mca_record->name, mcs_record->name );
		}
	}

	dante_mca_ptr = (MX_DANTE_MCA *) mca_record->record_type_struct;

	if ( dante_mca_ptr == (MX_DANTE_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DANTE_MCA pointer for MCA '%s' used by "
			"DANTE MCS '%s' is NULL.",
			mca_record->name, mcs_record->name );
	}

	if ( dante_mca != (MX_DANTE_MCA **) NULL ) {
		*dante_mca = dante_mca_ptr;
	}

	if ( dante != (MX_DANTE **) NULL ) {
		dante_record = dante_mca_ptr->dante_record;

		if ( dante_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The dante_record pointer for MCA '%s' used by "
			"DANTE MCS '%s' is NULL.",
			mca_record->name, mcs_record->name );
		}

		*dante = (MX_DANTE *) dante_record->record_type_struct;

		if ( (*dante) == (MX_DANTE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DANTE pointer for DANTE record '%s' is NULL.",
				dante_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_dante_mcs_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_scalers_varargs_cookie;
	long maximum_num_measurements_varargs_cookie;
	mx_status_type status;

	status = mx_mcs_initialize_driver( driver,
				&maximum_num_scalers_varargs_cookie,
				&maximum_num_measurements_varargs_cookie );

	return status;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_dante_mcs_create_record_structures()";

	MX_MCS *mcs;
	MX_DANTE_MCS *dante_mcs = NULL;

	/* Allocate memory for the necessary structures. */

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MCS structure." );
	}

	dante_mcs = (MX_DANTE_MCS *) malloc( sizeof(MX_DANTE_MCS) );

	if ( dante_mcs == (MX_DANTE_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_DANTE_MCS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mcs;
	record->record_type_struct = dante_mcs;
	record->class_specific_function_list = &mxd_dante_mcs_mcs_function_list;

	mcs->record = record;

	dante_mcs->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_dante_mcs_finish_record_initialization()";

	MX_MCS *mcs;
	mx_status_type mx_status;

	mx_status = mx_mcs_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs = (MX_MCS *) (record->record_class_struct);

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS pointer for MCS record '%s' is NULL.",
			record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_dante_mcs_open()";

	MX_MCS *mcs = NULL;
	MX_DANTE_MCS *dante_mcs = NULL;
	MX_MCA *mca = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mcs = (MX_MCS *) (record->record_class_struct);

	mx_status = mxd_dante_mcs_get_pointers( mcs, &dante_mcs,
					&mca, &dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DANTE_MCS_DEBUG_OPEN
	MX_DEBUG(-2,("%s: MCA = '%s', mca->record = %p",
		fname, mca->record->name, mca->record));
#endif

	/* Most of the work was already done by the MCA record that we
	 * point to.  We just need to set up mapping-specific stuff here
	 * that an ordinary MX DANTE MCA driver will not understand.
	 */

	/* Give the parent MCA record a way of finding the child MCS
	 * that depends on it.
	 */

	dante_mca->child_mcs_record = record;

	/* Default to external trigger. */

	mcs->trigger_mode = MXF_DEV_EXTERNAL_TRIGGER;

#if MXD_DANTE_MCS_DEBUG_OPEN
	MX_DEBUG(-2,("%s: MCS '%s' record = %p", fname, record->name, record));
	MX_DEBUG(-2,("%s: mcs '%s' = %p", fname, mcs->record->name, mcs));
	MX_DEBUG(-2,("%s: mcs->data_array = %p", fname, mcs->data_array));
	MX_DEBUG(-2,("%s: mcs->data_array[0] = %p", fname, mcs->data_array[0]));
	MX_DEBUG(-2,("%s: mcs->data_array[0][0] = %lu",
						fname, mcs->data_array[0][0]));
	MX_DEBUG(-2,("%s: dante_mcs = %p", fname, dante_mcs ));
	MX_DEBUG(-2,("%s: dante_mcs->mca_record = %p, '%s'",
			fname, dante_mcs->mca_record,
			dante_mcs->mca_record->name));

	MX_DEBUG(-2,("%s: mca = %p", fname, mca));
	MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
	MX_DEBUG(-2,("%s: dante_mca->record = %p, '%s'",
				fname, dante_mca->record,
				dante_mca->record->name));
	MX_DEBUG(-2,("%s: dante_mca->child_mcs_record = %p, '%s'",
				fname, dante_mca->child_mcs_record,
				dante_mca->child_mcs_record->name));

#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_dante_mcs_arm( MX_MCS *mcs )
{
	static const char fname[] = "mxd_dante_mcs_arm()";

	MX_DANTE_MCS *dante_mcs = NULL;
	MX_MCA *mca = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	uint32_t call_id;
	uint16_t error_code;
	uint32_t new_other_param;
	mx_status_type mx_status;

	mx_status = mxd_dante_mcs_get_pointers( mcs, &dante_mcs,
					&mca, &dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DANTE_MCS_DEBUG
	MX_DEBUG(-2,("%s: mcs = %p, mcs->record = %p, '%s'",
		fname, mcs, mcs->record, mcs->record->name));
#endif

	mx_status = mxd_dante_mcs_stop( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the gating for the MCA this MCS depends on. */

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

	/* Start the MCS in 'mapping' mode. */

	(void) resetLastError();

	dante->dante_mode = MXF_DANTE_MAPPING_MODE;

	MX_DEBUG(-2,("%s: Calling start_map(), channel_name = '%s', "
	"measurement_time = %f, current_num_measurements = %lu, "
	"current_num_scalers = %lu",
		fname, dante_mca->channel_name, mcs->measurement_time,
		mcs->current_num_measurements, mcs->current_num_scalers ));

	call_id = start_map( dante_mca->channel_name,
				mcs->measurement_time,
				mcs->current_num_measurements,
				mcs->current_num_scalers );

	if ( call_id == 0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"start_map() #1 returned an error for MCS '%s'.",
			mcs->record->name );
	}

	mxi_dante_wait_for_answer( call_id );

	if ( mxi_dante_callback_data[0] == 1 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		getLastError( error_code );

		switch( error_code ) {
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"start_map() #2 returned an error for MCS '%s'.  "
			"error_code = %lu",
				mcs->record->name, (unsigned long) error_code );
			break;
		}
	}

	MX_DEBUG(-2,("%s complete for MCS '%s'.", fname, mcs->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_trigger( MX_MCS *mcs )
{
	static const char fname[] = "mxd_dante_mcs_trigger()";

	MX_DANTE *dante = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dante_mcs_get_pointers( mcs, NULL,
					NULL, NULL, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs->trigger_mode == MXF_DEV_INTERNAL_TRIGGER ) {

		/* Manually advance to the next measurement ("pixel"). */

		/* FIXME: How do we do this for DANTE? */
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_stop( MX_MCS *mcs )
{
	static const char fname[] = "mxd_dante_mcs_stop()";

	MX_MCA *mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dante_mcs_get_pointers( mcs, NULL,
					&mca, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_dante_mca_stop( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_clear( MX_MCS *mcs )
{
	static const char fname[] = "mxd_dante_mcs_clear()";

	MX_DANTE_MCS *dante_mcs = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	bool clear_status;
	mx_status_type mx_status;

	mx_status = mxd_dante_mcs_get_pointers( mcs, &dante_mcs,
					NULL, &dante_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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
mxd_dante_mcs_busy( MX_MCS *mcs )
{
	static const char fname[] = "mxd_dante_mcs_busy()";

	MX_DANTE_MCS *dante_mcs = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	uint32_t call_id;
	mx_status_type mx_status;

	mx_status = mxd_dante_mcs_get_pointers( mcs, &dante_mcs,
					NULL, &dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	call_id = isRunning_system( dante_mca->channel_name,
					dante_mca->board_number );

	if ( call_id == 0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"isRunning_system() failed for MCS '%s'.",
			mcs->record->name );
	}

	mxi_dante_wait_for_answer( call_id );

	if ( mxi_dante_callback_data[0] == 0 ) {
		mcs->busy = FALSE;
	} else {
		mcs->busy = TRUE;
	}

	MX_DEBUG(-2,("%s: '%s' busy = %d",
		fname, mcs->record->name, (int) mcs->busy ));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_read_all( MX_MCS *mcs )
{
	static const char fname[] = "mxd_dante_mcs_read_all()";

	MX_DANTE_MCS *dante_mcs = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
	bool dante_status;
	uint16_t dante_error_code = DLL_NO_ERROR;
	bool dante_error_status;
	uint16_t values_temp;
	uint16_t *values_array;
	uint32_t *id_array;
	double *stats_array;
	uint64_t *advstats_array;
	uint32_t spectra_size, chan;
	uint32_t data_number, meas;
	unsigned long offset;
	mx_status_type mx_status;

	mx_status = mxd_dante_mcs_get_pointers( mcs, &dante_mcs,
					NULL, &dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("'%s' invoked for record '%s'.",
		fname, mcs->record->name ));

	mx_status = mxd_dante_mcs_get_last_measurement_number( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: mcs->last_measurement_number = %ld",
		fname, mcs->last_measurement_number));

	MX_DEBUG(-2,("%s: before getAllData(), dante_mca = %p",
			fname, dante_mca));

	spectra_size = mcs->current_num_scalers;

	data_number = mcs->last_measurement_number;

	values_array = new uint16_t[ data_number * 4096 ]();
	id_array = new uint32_t[ data_number ]();
	stats_array = new double[ data_number * 4 ]();
	advstats_array = new uint64_t[ data_number * 18 ]();

	MX_DEBUG(-2,("%s: before getAllData() channel_name = '%s'",
			fname, dante_mca->channel_name ));
	MX_DEBUG(-2,("%s: before getAllData() board_number = %lu",
			fname, (unsigned long) dante_mca->board_number));
	MX_DEBUG(-2,("%s: before getAllData() values_array = %p",
			fname, values_array));
	MX_DEBUG(-2,("%s: before getAllData() id_array = %p",
			fname, id_array));
	MX_DEBUG(-2,("%s: before getAllData() stats_array = %p",
			fname, stats_array));
	MX_DEBUG(-2,("%s: before getAllData() advstats_array = %p",
			fname, advstats_array));
	MX_DEBUG(-2,("%s: before getAllData() spectra_size = %lu",
			fname, (unsigned long) spectra_size));
	MX_DEBUG(-2,("%s: before getAllData() data_number = %lu",
			fname, (unsigned long) data_number));

	(void) resetLastError();

	dante_status = getAllData( dante_mca->channel_name,
				dante_mca->board_number,
				values_array,
				id_array,
				stats_array,
				advstats_array,
				spectra_size,
				data_number );

	MX_DEBUG(-2,("%s: after getAllData() dante_status = %d",
			fname, (int) dante_status));

	if ( dante_status == false ) {
		dante_error_status = getLastError( dante_error_code );

		if ( dante_error_status == false ) {
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"After a call to getAllData() failed for "
			"record '%s', a call to getLastError() failed "
			"while trying to find out why getAllData() "
			"failed.", mcs->record->name );
		}

		switch( dante_error_code ) {
		case DLL_ARGUMENT_OUT_OF_RANGE:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The call to getAllData() for MCS '%s' failed because "
			"one or more of the parameters to getAllData() were "
			"outside the valid range.  DANTE error code = %lu.",
				mcs->record->name,
				(unsigned long) dante_error_code );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"The call to getAllData() failed for MCS '%s' "
			"with DANTE error code %lu.",
				mcs->record->name,
				(unsigned long) dante_error_code );
			break;
		}
	}

#if 1
	fprintf(stderr, "Showing values array from bin 80 to 109.\n" );

	for ( chan = 80; chan < 110; chan++ ) {
		values_temp = values_array[chan];

		fprintf( stderr, "%hu ", (unsigned short) values_temp );
	}
	fprintf( stderr, "\n" );
#endif

	/* Copy out the MCS spectra. */

	for ( meas = 0; meas < data_number; meas++ ) {
		for ( chan = 0; chan < spectra_size; chan++ ) {
			offset = meas * spectra_size + chan;

			MX_DEBUG(-2,("%s: (%lu, %lu)", fname,
				(unsigned long) meas, (unsigned long) chan));

			values_temp = values_array[offset];

			mcs->data_array[meas][chan] = values_temp;
#if 1
			if ( values_temp > 10 ) {
				MX_DEBUG(-2,("%s: data_array[%lu][%lu] = %lu",
				fname, (unsigned long) meas,
				(unsigned long) chan,
				(unsigned long) mcs->data_array[meas][chan]));
			}
#endif
		}
	}

	mcs->new_data_available = TRUE;

	MX_DEBUG(-2,("'%s' complete for record '%s'.",
		fname, mcs->record->name ));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_get_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_dante_mcs_get_parameter()";

	MX_DANTE_MCS *dante_mcs = NULL;
	MX_MCA *mca = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE *dante = NULL;
#if 0
	uint32_t number_of_spectra = 0;
	uint16_t error_code = DLL_NO_ERROR;
	bool dante_status;
#endif
	mx_status_type mx_status;

	mx_status = mxd_dante_mcs_get_pointers( mcs, &dante_mcs,
					&mca, &dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', parameter type '%s' (%ld)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_LAST_MEASUREMENT_NUMBER:
		mx_status = mxd_dante_mcs_get_last_measurement_number( mcs );
		break;
	case MXLV_MCS_TOTAL_NUM_MEASUREMENTS:
		mx_status = mxd_dante_mcs_get_total_num_measurements( mcs );
		break;
	case MXLV_MCS_MEASUREMENT_NUMBER:
		break;
	case MXLV_MCS_DARK_CURRENT:
		break;
	case MXLV_MCS_TRIGGER_MODE:
		break;
	case MXLV_MCS_CURRENT_NUM_SCALERS:
		break;
	default:
		return mx_mcs_default_get_parameter_handler( mcs );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_set_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_dante_mcs_set_parameter()";

	MX_DANTE_MCS *dante_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dante_mcs_get_pointers( mcs, &dante_mcs,
					NULL, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', parameter type '%s' (%ld)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_COUNTING_MODE:
		break;

	case MXLV_MCS_MEASUREMENT_TIME:
		break;

	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
		break;

	case MXLV_MCS_DARK_CURRENT:
		break;

	case MXLV_MCS_TRIGGER_MODE:
		break;
	default:
		return mx_mcs_default_set_parameter_handler( mcs );
		break;
	}

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_get_last_measurement_number( MX_MCS *mcs )
{
	static const char fname[] =
		"mxd_dante_mcs_get_last_measurement_number()";

	MX_DANTE_MCS *dante_mcs = NULL;
	MX_DANTE *dante = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	uint32_t data_number;
	bool dante_status;
	mx_status_type mx_status;

	mx_status = mxd_dante_mcs_get_pointers( mcs, &dante_mcs,
					NULL, &dante_mca, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	data_number = (uint32_t) ULONG_MAX;

	mcs->last_measurement_number = -1;

	dante_status = getAvailableData( dante_mca->channel_name,
					dante_mca->board_number,
					data_number );

	MXW_UNUSED(dante_status);

	MX_DEBUG(-2,("%s: data_number = %lu",
		fname, (unsigned long) data_number))

	mcs->last_measurement_number = (long) data_number;

	MX_DEBUG(-2,("%s: mcs->last_measurement_number = %ld",
		fname, mcs->last_measurement_number));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dante_mcs_get_total_num_measurements( MX_MCS *mcs )
{
	static const char fname[] =
		"mxd_dante_mcs_get_total_num_measurements()";

	MX_DANTE_MCS *dante_mcs = NULL;
	MX_DANTE *dante = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dante_mcs_get_pointers( mcs, &dante_mcs,
					NULL, NULL, &dante, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: This particular kludge only tells you how many
	 * frames have been acquired since the start of this
	 * measurement sequence.
	 */
#if 1
	mx_status = mxd_dante_mcs_get_last_measurement_number( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs->total_num_measurements = mcs->last_measurement_number + 1;
#endif

	MX_DEBUG(-2,("%s: mcs->total_num_measurements = %ld",
		fname, mcs->total_num_measurements));

	return MX_SUCCESSFUL_RESULT;
}


/*
 * Name:    mx_mcs.c
 *
 * Purpose: MX function library for multichannel scalers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2006, 2009-2010, 2012, 2014-2016, 2018-2020
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_MCS_DEBUG_STATUS		FALSE

#define MX_MCS_ENABLE_CLEAR_DEADBAND	TRUE

#define MX_MCS_DEBUG_CLEAR_DEADBAND	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_array.h"
#include "mx_driver.h"
#include "mx_mcs.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_mcs_...
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_mcs_get_pointers( MX_RECORD *mcs_record,
			MX_MCS **mcs,
			MX_MCS_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_mcs_get_pointers()";

	if ( mcs_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The mcs_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( mcs_record->mx_class != MXC_MULTICHANNEL_SCALER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' passed by '%s' is not an MCS.",
			mcs_record->name, calling_fname );
	}

	if ( mcs != (MX_MCS **) NULL ) {
		*mcs = (MX_MCS *) (mcs_record->record_class_struct);

		if ( *mcs == (MX_MCS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS pointer for record '%s' passed by '%s' is NULL",
				mcs_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_MCS_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_MCS_FUNCTION_LIST *)
				(mcs_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_MCS_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_MCS_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				mcs_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* Please note that the 'scaler_data' field is handled specially and is
 * actually initialized in mx_mcs_finish_record_initialization().  This
 * is because the 'scaler_data' field is just a pointer into the appropriate
 * row of the 'data_array' field.
 */

MX_EXPORT mx_status_type
mx_mcs_initialize_driver( MX_DRIVER *driver,
			long *maximum_num_scalers_varargs_cookie,
			long *maximum_num_measurements_varargs_cookie )
{
	static const char fname[] = "mx_mcs_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}
	if ( maximum_num_scalers_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"maximum_num_scalers_varargs_cookie pointer passed was NULL." );
	}
	if ( maximum_num_measurements_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"maximum_num_measurements_varargs_cookie pointer passed was NULL." );
	}

	/* Set varargs cookies in 'data_array' that depend on the values
	 * of 'maximum_num_scalers' and 'maximum_num_measurements'.
	 */

	mx_status = mx_find_record_field_defaults( driver, "data_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults_index( driver,
			"maximum_num_scalers", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
					maximum_num_scalers_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults_index( driver,
			"maximum_num_measurements", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				maximum_num_measurements_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_scalers_varargs_cookie;
	field->dimension[1] = *maximum_num_measurements_varargs_cookie;

	/* 'timer_data' depends on 'maximum_num_measurements'. */

	mx_status = mx_find_record_field_defaults( driver, "timer_data", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_measurements_varargs_cookie;

	/* 'measurement_data' depends on 'maximum_num_scalers'. */

	mx_status = mx_find_record_field_defaults( driver,
					"measurement_data", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_scalers_varargs_cookie;

	/* 'dark_current_array' depends on 'maximum_num_scalers'. */

	mx_status = mx_find_record_field_defaults( driver,
					"dark_current_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_scalers_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_finish_record_initialization( MX_RECORD *mcs_record )
{
	static const char fname[] = "mx_mcs_finish_record_initialization()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	MX_RECORD_FIELD *scaler_data_field;
	MX_RECORD_FIELD *measurement_data_field;
	MX_RECORD_FIELD *timer_data_field;
	long i;
	int valid_type;
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs->maximum_num_scalers == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The multichannel scaler '%s' is set to have zero scaler channels.  "
	"This is an error since at least one scaler channel is required.",
			mcs_record->name );
	}

	if ( mcs->maximum_num_measurements == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The multichannel scaler '%s' is set to have zero measurements.  "
	"This is an error since at least one measurement is required.",
			mcs_record->name );
	}

	mcs->current_num_scalers = mcs->maximum_num_scalers;
	mcs->current_num_measurements = mcs->maximum_num_measurements;

	mcs->autostart = TRUE;
	mcs->counting_mode = MXM_PRESET_TIME;
	mcs->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;

	mcs->readout_preference = MXF_MCS_PREFER_READ_SCALER;

	mcs->latched_status = 0;

	mcs->busy_start_interval_enabled = FALSE;
	mcs->busy_start_interval = -1;

	mcs->external_next_measurement_record = NULL;
	mcs->timer_record = NULL;

	mcs->scaler_record_array = (MX_RECORD **)
		malloc( mcs->maximum_num_scalers * sizeof(MX_RECORD *) );

	if ( mcs->scaler_record_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array of "
		"MCS scaler record pointers.", mcs->maximum_num_scalers );
	}

	for ( i = 0; i < mcs->maximum_num_scalers; i++ ) {
		mcs->scaler_record_array[i] = NULL;
	}

	/* Initialize the 'scaler_data' pointer to point at the first
	 * row of the 'data_array' field.  We also initialize the length
	 * of the 'scaler_data' field to be 'maximum_num_measurements' long.
	 */

	mx_status = mx_find_record_field( mcs_record, "scaler_data",
					&scaler_data_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler_data_field->dimension[0] = mcs->maximum_num_measurements;

	mcs->scaler_index = 0;

	mcs->scaler_data = mcs->data_array[0];

	mx_status = mx_find_record_field( mcs_record, "measurement_data",
					&measurement_data_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field( mcs_record, "timer_data",
					&timer_data_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the 'external_next_measurement_name' field has a non-zero
	 * length value then make mcs->external_next_measurement_record
	 * point to the record with that name.
	 */

	if ( strlen( mcs->external_next_measurement_name ) > 0 ) {

		mcs->external_next_measurement_record = mx_get_record(mcs_record,
				mcs->external_next_measurement_name );

		if ( mcs->external_next_measurement_record == (MX_RECORD *)NULL){
			return mx_error( MXE_NOT_FOUND, fname,
	"Record '%s' was specified as the external channel advance record "
	"for MCS '%s'.  However, record '%s' does not exist.",
				mcs_record->name,
				mcs->external_next_measurement_name,
				mcs_record->name );
		}
	}

	/* If the 'timer_name' field has a non-zero length value
	 * then make mcs->timer_record point to the record with that name.
	 */

	if ( strlen( mcs->timer_name ) > 0 ) {

		mcs->timer_record = mx_get_record( mcs_record,
					mcs->timer_name );

		if ( mcs->timer_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
	"Record '%s' was specified as the timer record for MCS '%s'.  "
	"However, record '%s' does not exist.", mcs->timer_name,
				mcs_record->name, mcs->timer_name );
		}

		valid_type = mx_verify_driver_type( mcs->timer_record,
				MXR_DEVICE, MXC_TIMER, MXT_ANY );

		if ( valid_type == FALSE ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
	"Record '%s' which was specified as the timer record for MCS '%s' "
	"is not a timer record.", mcs->timer_record->name,
				mcs_record->name );
		}
	}

	/*----*/

	mcs->new_data_available = TRUE;

	mcs->clear_deadband = 0;		/* in seconds */

#if 0
	mx_status = mx_mcs_set_parameter( mcs_record, MXLV_MCS_CLEAR_DEADBAND );
#endif

	return mx_status;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_mcs_arm( MX_RECORD *mcs_record )
{
	static const char fname[] = "mx_mcs_arm()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *arm_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	arm_fn = function_list->arm;

	if ( arm_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"arm function ptr for record '%s' is NULL.",
			mcs_record->name );
	}

	mcs->measurement_number = -1;

	mx_status = (*arm_fn)( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs->trigger_mode == MXF_DEV_EXTERNAL_TRIGGER ) {
		mcs->new_data_available = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_trigger( MX_RECORD *mcs_record )
{
	static const char fname[] = "mx_mcs_trigger()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *trigger_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs->trigger_mode == MXF_DEV_INTERNAL_TRIGGER ) {
		mcs->measurement_number = -1;
	}

	trigger_fn = function_list->trigger;

	if ( trigger_fn != NULL ) {
		mx_status = (*trigger_fn)( mcs );
	} else {
		mx_status = MX_SUCCESSFUL_RESULT;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs->trigger_mode == MXF_DEV_INTERNAL_TRIGGER ) {
		mcs->new_data_available = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_start( MX_RECORD *mcs_record )
{
	mx_status_type mx_status;

	mx_status = mx_mcs_arm( mcs_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_trigger( mcs_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_stop( MX_RECORD *mcs_record )
{
	static const char fname[] = "mx_mcs_stop()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *stop_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop_fn = function_list->stop;

	if ( stop_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"stop function ptr for record '%s' is NULL.",
			mcs_record->name );
	}

	mx_status = (*stop_fn)( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs->stop = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_clear( MX_RECORD *mcs_record )
{
	static const char fname[] = "mx_mcs_clear()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *clear_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	clear_fn = function_list->clear;

	if ( clear_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"clear function ptr for record '%s' is NULL.",
			mcs_record->name );
	}

#if ( MX_MCS_ENABLE_CLEAR_DEADBAND == FALSE )
	mx_status = (*clear_fn)( mcs );

	return mx_status;
#else
	{
		MX_CLOCK_TICK current_tick;
		mx_bool_type do_clear;
		int comparison;

		current_tick = mx_current_clock_tick();

		if ( mcs->new_data_available ) {
			do_clear = TRUE;
		} else {
			comparison = mx_compare_clock_ticks( current_tick,
							mcs->next_clear_tick );

#if MX_MCS_DEBUG_CLEAR_DEADBAND
			MX_DEBUG(-2,
	("%s: current tick = (%lu,%lu), next tick = (%lu,%lu), comparison = %d",
				fname, current_tick.high_order,
				current_tick.low_order,
				mcs->next_clear_tick.high_order,
				mcs->next_clear_tick.low_order,
				comparison ));
#endif

			if ( comparison >= 0 ) {
				do_clear = TRUE;
			} else {
				do_clear = FALSE;
			}
		}

#if MX_MCS_DEBUG_CLEAR_DEADBAND
		MX_DEBUG(-2,("%s: do_clear = %d", fname, (int) do_clear));
#endif

		if ( do_clear ) {
			mx_status = (*clear_fn)( mcs );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mcs->new_data_available = FALSE;

			mcs->next_clear_tick =
				mx_add_clock_ticks( current_tick,
					mcs->clear_deadband_ticks );
#if MX_MCS_DEBUG_CLEAR_DEADBAND
			MX_DEBUG(-2,("%s: clear_deadband = %g sec, "
			"clear_deadband_ticks = (%lu,%lu)",
			fname, mcs->clear_deadband,
			mcs->clear_deadband_ticks.high_order,
			mcs->clear_deadband_ticks.low_order));

			MX_DEBUG(-2,("%s: next_clear_tick = (%lu,%lu)",
			fname, mcs->next_clear_tick.high_order,
			mcs->next_clear_tick.low_order));
#endif
		}
	}

	mcs->clear = 0;

	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mx_mcs_is_busy( MX_RECORD *mcs_record, mx_bool_type *busy )
{
	static const char fname[] = "mx_mcs_is_busy()";

	MX_MCS *mcs = NULL;
	unsigned long status_flags;
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record, &mcs, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_get_status( mcs_record, &status_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy != (mx_bool_type *) NULL ) {
		*busy = mcs->busy;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_get_last_measurement_number( MX_RECORD *mcs_record,
				long *last_measurement_number )
{
	static const char fname[] = "mx_mcs_get_last_measurement_number()";

	MX_MCS *mcs = NULL;
	MX_MCS_FUNCTION_LIST *function_list = NULL;
	mx_status_type ( *get_last_measurement_number_fn ) ( MX_MCS * ) = NULL;
	mx_status_type ( *get_extended_status_fn ) ( MX_MCS * ) = NULL;
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs->last_measurement_number = -1;

	get_last_measurement_number_fn =
		function_list->get_last_measurement_number;
	get_extended_status_fn = function_list->get_extended_status;

	if ( get_last_measurement_number_fn != NULL ) {
		mx_status = (*get_last_measurement_number_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'last_measurement_number' and 'extended_status' "
		"functions are not implemented for the '%s' driver "
		"of MCS '%s'.",
			mx_get_driver_name( mcs_record ),
			mcs_record->name );
	}

	if ( last_measurement_number != (long *) NULL ) {
		*last_measurement_number = mcs->last_measurement_number;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_get_total_num_measurements( MX_RECORD *mcs_record,
				long *total_num_measurements )
{
	static const char fname[] = "mx_mcs_get_total_num_measurements()";

	MX_MCS *mcs = NULL;
	MX_MCS_FUNCTION_LIST *function_list = NULL;
	mx_status_type ( *get_total_num_measurements_fn ) ( MX_MCS * ) = NULL;
	mx_status_type ( *get_extended_status_fn ) ( MX_MCS * ) = NULL;
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs->total_num_measurements = -1;

	get_total_num_measurements_fn =
			function_list->get_total_num_measurements;
	get_extended_status_fn = function_list->get_extended_status;

	if ( get_total_num_measurements_fn != NULL ) {
		mx_status = (*get_total_num_measurements_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'total_num_measurements' and 'extended_status' "
		"functions are not implemented for the '%s' driver "
		"of MCS '%s'.",
			mx_get_driver_name( mcs_record ),
			mcs_record->name );
	}

	if ( total_num_measurements != (long *) NULL ) {
		*total_num_measurements = mcs->total_num_measurements;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_get_status( MX_RECORD *mcs_record, unsigned long *mcs_status )
{
	static const char fname[] = "mx_mcs_get_status()";

	MX_MCS *mcs = NULL;
	MX_MCS_FUNCTION_LIST *function_list = NULL;
	mx_status_type ( *busy_fn ) ( MX_MCS * ) = NULL;
	mx_status_type ( *get_status_fn ) ( MX_MCS * ) = NULL;
	mx_status_type ( *get_extended_status_fn ) ( MX_MCS * ) = NULL;
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs->status = 0;

	busy_fn = function_list->busy;
	get_status_fn = function_list->get_status;
	get_extended_status_fn = function_list->get_extended_status;

	if ( get_status_fn != NULL ) {
		mx_status = (*get_status_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mcs->status & MXSF_MCS_IS_BUSY ) {
			mcs->busy = TRUE;
		} else {
			mcs->busy = FALSE;
		}
	} else
	if ( busy_fn != NULL ) {
		mx_status = (*busy_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mcs->busy ) {
			mcs->status |= MXSF_MCS_IS_BUSY;
		} else {
			mcs->status &= (~MXSF_MCS_IS_BUSY);
		}
	} else 
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mcs->status & MXSF_MCS_IS_BUSY ) {
			mcs->busy = TRUE;
		} else {
			mcs->busy = FALSE;
		}
	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'busy', 'status', and 'extended_status' functions are "
		"not implemented for the '%s' driver of MCS '%s'.",
			mx_get_driver_name( mcs_record ),
			mcs_record->name );
	}

	/*---*/

	if ( mcs->busy_start_interval_enabled ) {
		mx_bool_type busy_start_set;

		mx_status = mx_mcs_check_busy_start_interval( mcs_record,
							&busy_start_set );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy_start_set ) {
			mcs->busy = TRUE;
			mcs->status |= MXSF_MCS_IS_BUSY;
		}
	}

	/*---*/

	if ( mcs_status != NULL ) {
		*mcs_status = mcs->status;
	}

	if ( mcs->status & MXSF_MCS_IS_BUSY ) {
		mcs->new_data_available = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_get_extended_status( MX_RECORD *record,
			long *last_measurement_number,
			long *total_num_measurements,
			unsigned long *status_flags )
{
	static const char fname[] = "mx_mcs_get_extended_status()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *flist;
	mx_status_type ( *get_last_measurement_number_fn ) ( MX_MCS * );
	mx_status_type ( *get_total_num_measurements_fn ) ( MX_MCS * );
	mx_status_type ( *get_status_fn ) ( MX_MCS * );
	mx_status_type ( *get_extended_status_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers(record, &mcs, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_last_measurement_number_fn = flist->get_last_measurement_number;
	get_total_num_measurements_fn  = flist->get_total_num_measurements;
	get_status_fn            = flist->get_status;
	get_extended_status_fn   = flist->get_extended_status;

	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( mcs );
	} else {
		if ( get_last_measurement_number_fn == NULL ) {
			mcs->last_measurement_number = -1;
		} else {
			mx_status = (*get_last_measurement_number_fn)( mcs );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( get_total_num_measurements_fn == NULL ) {
			mcs->total_num_measurements = 0;
		} else {
			mx_status = (*get_total_num_measurements_fn)( mcs );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( get_status_fn == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the status for MCS '%s' is unsupported.",
				record->name );
		} else {
			mx_status = (*get_status_fn)( mcs );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/*---*/

	if ( mcs->busy_start_interval_enabled ) {
		mx_bool_type busy_start_set;

		mx_status = mx_mcs_check_busy_start_interval(
						record, &busy_start_set );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy_start_set ) {
			mcs->busy = TRUE;
			mcs->status |= MXSF_MCS_IS_BUSY;
		}
	}

	/* Set the bits from the latched status word in the main status word. */

	mcs->status |= mcs->latched_status;

#if MX_MCS_DEBUG_STATUS
	MX_DEBUG(-2,
	("%s: last_measurement_number = %ld, total_num_measurements = %ld, "
	 "status = %#lx",
	    fname, mcs->last_measurement_number, mcs->total_num_measurements,
	    mcs->status ));
#endif

	if ( last_measurement_number != NULL ) {
		*last_measurement_number = mcs->last_measurement_number;
	}
	if ( total_num_measurements != NULL ) {
		*total_num_measurements = mcs->total_num_measurements;
	}
	if ( status_flags != NULL ) {
		*status_flags = mcs->status;
	}

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_mcs_set_busy_start_interval( MX_RECORD *record,
				double busy_start_interval_in_seconds )
{
	static const char fname[] = "mx_mcs_set_busy_start_interval()";

	MX_MCS *mcs;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The motor record pointer passed was NULL." );
	}

	mcs = (MX_MCS *) record->record_class_struct;

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS pointer for record '%s' is NULL.",
			record->name );
	}

	mcs->busy_start_interval = busy_start_interval_in_seconds;

	if ( busy_start_interval_in_seconds < 0.0 ) {
		mcs->busy_start_interval_enabled = FALSE;
	} else {
		mcs->busy_start_interval_enabled = TRUE;

		mcs->busy_start_ticks =
	    mx_convert_seconds_to_clock_ticks( busy_start_interval_in_seconds );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* If the difference between the current time and the time of the last start
 * command is less than the value of 'busy_start_interval', then the flag
 * busy_start_set will be set to TRUE.
 *
 * There exist multichannel scalers that have the misfeature that when you
 * tell them to start an acquisition sequence, there may be a short time after
 * the start of the acquisition where the controller reports that the MCS
 * is _not_ busy.
 *
 * While this may technically be true, it is not useful for higher level logic
 * that is trying to figure out when a acquisition sequence has completed.
 * MX handles this by reporting that the multichannel scaler is 'busy' for
 * a period after a start command equal to the value of
 * 'mcs->busy_start_interval' in seconds.  This is only done if the
 * busy start feature is enabled.
 */

MX_EXPORT mx_status_type
mx_mcs_check_busy_start_interval( MX_RECORD *record,
				mx_bool_type *busy_start_set )
{
	static const char fname[] =
			"mx_mcs_check_busy_start_interval()";

	MX_MCS *mcs;
	MX_CLOCK_TICK start_tick, busy_start_ticks;
	MX_CLOCK_TICK finish_tick, current_tick;
	int comparison;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The motor record pointer passed was NULL." );
	}
	if ( busy_start_set == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The busy_start_set pointer passed was NULL." );
	}

	mcs = (MX_MCS *) record->record_class_struct;

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS pointer for record '%s' is NULL.",
			record->name );
	}

	if ( mcs->busy_start_interval_enabled == FALSE ) {
		*busy_start_set = FALSE;
		return MX_SUCCESSFUL_RESULT;
	}

	mcs->last_start_time =
		mx_convert_clock_ticks_to_seconds( mcs->last_start_tick );

	start_tick = mcs->last_start_tick;

	busy_start_ticks = mcs->busy_start_ticks;

	finish_tick = mx_add_clock_ticks( start_tick, busy_start_ticks );

	current_tick = mx_current_clock_tick();

	comparison = mx_compare_clock_ticks( current_tick, finish_tick );

	if ( comparison <= 0 ) {
		*busy_start_set = TRUE;
	} else {
		*busy_start_set = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_mcs_get_current_num_measurements( MX_RECORD *mcs_record,
					long *current_num_measurements )
{
	static const char fname[] = "mx_mcs_get_current_num_measurements()";

	MX_MCS *mcs = NULL;
	MX_MCS_FUNCTION_LIST *function_list = NULL;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_CURRENT_NUM_MEASUREMENTS;

	mx_status = (*get_parameter_fn)( mcs );

	if ( current_num_measurements != NULL ) {
		*current_num_measurements = mcs->current_num_measurements;
	}

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_mcs_read_all( MX_RECORD *mcs_record,
			unsigned long *num_scalers,
			unsigned long *num_measurements,
			long ***mcs_data )
{
	static const char fname[] = "mx_mcs_read_all()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *read_all_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_all_fn = function_list->read_all;

	if ( read_all_fn != NULL ) {
		mx_status = (*read_all_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( num_scalers != NULL ) {
		*num_scalers = mcs->current_num_scalers;
	}
	if ( num_measurements != NULL ) {
		*num_measurements = mcs->current_num_measurements;
	}
	if ( mcs_data != NULL ) {
		*mcs_data = mcs->data_array;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_read_scaler( MX_RECORD *mcs_record,
			unsigned long scaler_index,
			unsigned long *num_measurements,
			long **scaler_data )
{
	static const char fname[] = "mx_mcs_read_scaler()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *read_scaler_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ((long) scaler_index) >= mcs->maximum_num_scalers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Scaler index %lu for MCS record '%s' is outside "
		"the allowed range of 0-%ld.",
			scaler_index, mcs_record->name,
			mcs->maximum_num_scalers - 1L );
	}

	mcs->scaler_index = (long) scaler_index;

	read_scaler_fn = function_list->read_scaler;

	if ( read_scaler_fn != NULL ) {
		mx_status = (*read_scaler_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mcs->scaler_data = (mcs->data_array) [ mcs->scaler_index ];

	if ( num_measurements != NULL ) {
		*num_measurements = mcs->current_num_measurements;
	}
	if ( scaler_data != NULL ) {
		*scaler_data = mcs->scaler_data;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_read_measurement( MX_RECORD *mcs_record,
			unsigned long measurement_index,
			unsigned long *num_scalers,
			long **measurement_data )
{
	static const char fname[] = "mx_mcs_read_measurement()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *read_measurement_fn ) ( MX_MCS * );
	unsigned long i;
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ((long) measurement_index) >= mcs->maximum_num_measurements ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Measurement index %lu for MCS record '%s' is outside "
		"the allowed range of 0-%ld.",
			measurement_index, mcs_record->name,
			mcs->maximum_num_measurements - 1L );
	}

	mcs->measurement_index = (long) measurement_index;

	read_measurement_fn = function_list->read_measurement;

	if ( read_measurement_fn != NULL ) {
		mx_status = (*read_measurement_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		for ( i = 0; i < mcs->current_num_scalers; i++ ) {
			mcs->measurement_data[i] =
				(mcs->data_array)[i][measurement_index];
		}
	}

	if ( num_scalers != NULL ) {
		*num_scalers = mcs->current_num_scalers;
	}
	if ( measurement_data != NULL ) {
		*measurement_data = mcs->measurement_data;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_read_scaler_measurement( MX_RECORD *mcs_record,
			unsigned long scaler_index,
			unsigned long measurement_index,
			long *scaler_measurement )
{
	static const char fname[] = "mx_mcs_read_scaler_measurement()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *read_scaler_measurement_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ((long) scaler_index) >= mcs->maximum_num_scalers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Scaler index %lu for MCS record '%s' is outside "
		"the allowed range of 0-%ld.",
			scaler_index, mcs_record->name,
			mcs->maximum_num_scalers - 1L );
	}

	if ( ((long) measurement_index) >= mcs->maximum_num_measurements ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Measurement index %lu for MCS record '%s' is outside "
		"the allowed range of 0-%ld.",
			measurement_index, mcs_record->name,
			mcs->maximum_num_measurements - 1L );
	}

	mcs->scaler_index = (long) scaler_index;
	mcs->measurement_index = (long) measurement_index;

	read_scaler_measurement_fn = function_list->read_scaler_measurement;

	if ( read_scaler_measurement_fn != NULL ) {
		mx_status = (*read_scaler_measurement_fn)( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		mcs->scaler_measurement = 
			    (mcs->data_array)[scaler_index][measurement_index];
	}

	if ( scaler_measurement != NULL ) {
		*scaler_measurement = mcs->scaler_measurement;
	}

#if 0
	MX_DEBUG(-2,("%s: '%s' (%lu, %lu) = %ld",
		fname, mcs_record->name,
		scaler_index, measurement_index,
		mcs->scaler_measurement));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_read_timer( MX_RECORD *mcs_record,
			unsigned long *num_measurements,
			double **timer_data )
{
	static const char fname [] = "mx_mcs_read_timer()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	unsigned long i;
	mx_status_type ( *read_timer_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_timer_fn = function_list->read_timer;

	if ( read_timer_fn == NULL ) {
		for ( i = 0; i < mcs->current_num_measurements; i++ ) {
			(mcs->timer_data)[i] = 0.0;
		}
	} else {
		mx_status = (*read_timer_fn)( mcs );
	}

	if ( num_measurements != NULL ) {
		*num_measurements = mcs->current_num_measurements;
	}
	if ( timer_data != NULL ) {
		*timer_data = mcs->timer_data;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_counting_mode( MX_RECORD *mcs_record, long *counting_mode )
{
	static const char fname[] = "mx_mcs_get_counting_mode()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_COUNTING_MODE;

	mx_status = (*get_parameter_fn)( mcs );

	if ( counting_mode != NULL ) {
		*counting_mode = mcs->counting_mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_set_counting_mode( MX_RECORD *mcs_record, long counting_mode )
{
	static const char fname[] = "mx_mcs_set_counting_mode()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_COUNTING_MODE;

	mcs->counting_mode = counting_mode;

	mx_status = (*set_parameter_fn)( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_trigger_mode( MX_RECORD *mcs_record, long *trigger_mode )
{
	static const char fname[] = "mx_mcs_get_trigger_mode()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_TRIGGER_MODE;

	mx_status = (*get_parameter_fn)( mcs );

	if ( trigger_mode != NULL ) {
		*trigger_mode = mcs->trigger_mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_set_trigger_mode( MX_RECORD *mcs_record, long trigger_mode )
{
	static const char fname[] = "mx_mcs_set_trigger_mode()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_TRIGGER_MODE;

	mcs->trigger_mode = trigger_mode;

	mx_status = (*set_parameter_fn)( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_autostart( MX_RECORD *mcs_record, long *autostart )
{
	static const char fname[] = "mx_mcs_get_autostart()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_AUTOSTART;

	mx_status = (*get_parameter_fn)( mcs );

	if ( autostart != NULL ) {
		*autostart = mcs->autostart;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_set_autostart( MX_RECORD *mcs_record, long autostart )
{
	static const char fname[] = "mx_mcs_set_autostart()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_AUTOSTART;

	mcs->autostart = autostart;

	mx_status = (*set_parameter_fn)( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_external_next_measurement( MX_RECORD *mcs_record,
					mx_bool_type *external_next_measurement )
{
	static const char fname[] = "mx_mcs_get_external_next_measurement()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT;

	mx_status = (*get_parameter_fn)( mcs );

	if ( external_next_measurement != NULL ) {
		*external_next_measurement = mcs->external_next_measurement;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_set_external_next_measurement( MX_RECORD *mcs_record,
					mx_bool_type external_next_measurement )
{
	static const char fname[] = "mx_mcs_set_external_next_measurement()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT;

	mcs->external_next_measurement = external_next_measurement;

	mx_status = (*set_parameter_fn)( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_external_prescale( MX_RECORD *mcs_record,
					unsigned long *external_prescale )
{
	static const char fname[] = "mx_mcs_get_external_prescale()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_EXTERNAL_PRESCALE;

	mx_status = (*get_parameter_fn)( mcs );

	if ( external_prescale != NULL ) {
		*external_prescale = mcs->external_prescale;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_set_external_prescale( MX_RECORD *mcs_record,
					unsigned long external_prescale )
{
	static const char fname[] = "mx_mcs_set_external_prescale()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_EXTERNAL_PRESCALE;

	mcs->external_prescale = external_prescale;

	mx_status = (*set_parameter_fn)( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_manual_next_measurement( MX_RECORD *mcs_record )
{
	static const char fname[] = "mx_mcs_manual_next_measurement()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MANUAL_NEXT_MEASUREMENT;

	mcs->manual_next_measurement = TRUE;

	mx_status = (*set_parameter_fn)( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_measurement_time( MX_RECORD *mcs_record, double *measurement_time )
{
	static const char fname[] = "mx_mcs_get_measurement_time()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MEASUREMENT_TIME;

	mx_status = (*get_parameter_fn)( mcs );

	if ( measurement_time != NULL ) {
		*measurement_time = mcs->measurement_time;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_set_measurement_time( MX_RECORD *mcs_record, double measurement_time )
{
	static const char fname[] = "mx_mcs_set_measurement_time()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MEASUREMENT_TIME;

	mcs->measurement_time = measurement_time;

	mx_status = (*set_parameter_fn)( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_measurement_counts( MX_RECORD *mcs_record,
					unsigned long *measurement_counts )
{
	static const char fname[] = "mx_mcs_get_measurement_counts()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MEASUREMENT_COUNTS;

	mx_status = (*get_parameter_fn)( mcs );

	if ( measurement_counts != NULL ) {
		*measurement_counts = mcs->measurement_counts;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_set_measurement_counts( MX_RECORD *mcs_record,
					unsigned long measurement_counts )
{
	static const char fname[] = "mx_mcs_set_measurement_counts()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MEASUREMENT_COUNTS;

	mcs->measurement_counts = measurement_counts;

	mx_status = (*set_parameter_fn)( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_num_measurements( MX_RECORD *mcs_record,
					unsigned long *num_measurements )
{
	static const char fname[] = "mx_mcs_get_num_measurements()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_CURRENT_NUM_MEASUREMENTS;

	mx_status = (*get_parameter_fn)( mcs );

	if ( num_measurements != NULL ) {
		*num_measurements = mcs->current_num_measurements;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_set_num_measurements( MX_RECORD *mcs_record,
					unsigned long num_measurements )
{
	static const char fname[] = "mx_mcs_set_num_measurements()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: num_measurements = %lu", fname, num_measurements));

	if ( num_measurements > mcs->maximum_num_measurements ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested number of measurements (%lu) for MCS '%s' exceeds "
	"the maximum number of measurements (%ld) allowed for this MCS.",
			num_measurements, mcs_record->name,
			mcs->maximum_num_measurements );
	}

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_CURRENT_NUM_MEASUREMENTS;

	mcs->current_num_measurements = num_measurements;

	mx_status = (*set_parameter_fn)( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_measurement_number( MX_RECORD *mcs_record,
				long *measurement_number )
{
	static const char fname[] = "mx_mcs_get_measurement_number()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MEASUREMENT_NUMBER;

	mx_status = (*get_parameter_fn)( mcs );

	if ( measurement_number != NULL ) {
		*measurement_number = mcs->measurement_number;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_dark_current_array( MX_RECORD *mcs_record,
				long num_scalers,
				double *dark_current_array )
{
	static const char fname[] = "mx_mcs_get_dark_current_array()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	unsigned long i;
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (num_scalers >= 0) && (num_scalers > mcs->current_num_scalers) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested number of scalers %lu for MCS '%s' exceeds the "
	"current number of scalers %lu.",
			num_scalers, mcs_record->name,
			mcs->current_num_scalers );
	}

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_DARK_CURRENT_ARRAY;

	mx_status = (*get_parameter_fn)( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( dark_current_array != NULL ) && ( num_scalers >= 0 ) ) {
		for ( i = 0; i < num_scalers; i++ ) {
			dark_current_array[i] = mcs->dark_current_array[i];
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_set_dark_current_array( MX_RECORD *mcs_record,
				unsigned long num_scalers,
				double *dark_current_array )
{
	static const char fname[] = "mx_mcs_set_dark_current_array()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	unsigned long i;
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( dark_current_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dark_current_array address passed is NULL." );
	}

	if ( num_scalers > mcs->current_num_scalers ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested number of scalers %lu for MCS '%s' exceeds the "
	"current number of scalers %lu.",
			num_scalers, mcs_record->name,
			mcs->current_num_scalers );
	}

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_DARK_CURRENT_ARRAY;

	for ( i = 0; i < num_scalers; i++ ) {
		mcs->dark_current_array[i] = dark_current_array[i];
	}

	mx_status = (*set_parameter_fn)( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_get_dark_current( MX_RECORD *mcs_record,
				unsigned long scaler_index,
				double *dark_current )
{
	static const char fname[] = "mx_mcs_get_dark_current()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scaler_index >= mcs->current_num_scalers ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested scaler index %lu for MCS '%s' is outside "
		"the allowed range of 0-%lu",
			scaler_index, mcs_record->name,
			mcs->current_num_scalers - 1 );
	}

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->scaler_index = (long) scaler_index;

	mcs->parameter_type = MXLV_MCS_DARK_CURRENT;

	mx_status = (*get_parameter_fn)( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: MCS '%s' index %lu, dark_current = %g",
		fname, mcs_record->name, scaler_index,
		mcs->dark_current_array[ scaler_index ]));

	if ( dark_current != NULL ) {
		*dark_current = mcs->dark_current_array[ scaler_index ];
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_set_dark_current( MX_RECORD *mcs_record,
				unsigned long scaler_index,
				double dark_current )
{
	static const char fname[] = "mx_mcs_set_dark_current()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: setting dark_current = %g", fname, dark_current));

	if ( scaler_index >= mcs->current_num_scalers ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested scaler index %lu for MCS '%s' is outside "
		"the allowed range of 0-%lu",
			scaler_index, mcs_record->name,
			mcs->current_num_scalers - 1 );
	}

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->scaler_index = (long) scaler_index;

	mcs->parameter_type = MXLV_MCS_DARK_CURRENT;

	mcs->dark_current_array[ scaler_index ] = dark_current;

	mx_status = (*set_parameter_fn)( mcs );

	MX_DEBUG( 2,("%s: MCS '%s' index %lu, dark_current = %g",
		fname, mcs_record->name, scaler_index,
		mcs->dark_current_array[ scaler_index ]));

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_mcs_get_parameter( MX_RECORD *mcs_record, long parameter_type )
{
	static const char fname[] = "mx_mcs_get_parameter()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The get_parameter function is not supported by the driver for record '%s'.",
			mcs_record->name );
	}

	mcs->parameter_type = parameter_type;

	mx_status = ( *fptr ) ( mcs );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mcs_set_parameter( MX_RECORD *mcs_record, long parameter_type )
{
	static const char fname[] = "mx_mcs_set_parameter()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MCS * );
	mx_status_type mx_status;

	mx_status = mx_mcs_get_pointers( mcs_record,
					&mcs, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The set_parameter function is not supported by the driver for record '%s'.",
			mcs_record->name );
	}

	mcs->parameter_type = parameter_type;

	mx_status = ( *fptr ) ( mcs );

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_mcs_default_get_parameter_handler( MX_MCS *mcs )
{
	static const char fname[] = "mx_mcs_default_get_parameter_handler()";

	switch( mcs->parameter_type ) {
	case MXLV_MCS_COUNTING_MODE:
	case MXLV_MCS_TRIGGER_MODE:
	case MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT:
	case MXLV_MCS_EXTERNAL_PRESCALE:
	case MXLV_MCS_MEASUREMENT_TIME:
	case MXLV_MCS_MEASUREMENT_COUNTS:
	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
	case MXLV_MCS_MEASUREMENT_NUMBER:
	case MXLV_MCS_DARK_CURRENT:
	case MXLV_MCS_DARK_CURRENT_ARRAY:
	case MXLV_MCS_CLEAR_DEADBAND:

		/* We just return the value that is already in the 
		 * data structure.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Parameter type '%s' (%ld) is not supported by the MX driver for MCS '%s'.",
			mx_get_field_label_string( mcs->record,
						mcs->parameter_type ),
			mcs->parameter_type,
			mcs->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mcs_default_set_parameter_handler( MX_MCS *mcs )
{
	static const char fname[] ="mx_mcs_default_set_parameter_handler()";

	switch( mcs->parameter_type ) {
	case MXLV_MCS_COUNTING_MODE:
	case MXLV_MCS_TRIGGER_MODE:
	case MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT:
	case MXLV_MCS_EXTERNAL_PRESCALE:
	case MXLV_MCS_MEASUREMENT_TIME:
	case MXLV_MCS_MEASUREMENT_COUNTS:
	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
	case MXLV_MCS_DARK_CURRENT:
	case MXLV_MCS_DARK_CURRENT_ARRAY:

		/* We do nothing but leave alone the value that is already
		 * stored in the data structure.
		 */

		break;

	case MXLV_MCS_MANUAL_NEXT_MEASUREMENT:
		/* Write a warning to the log, but otherwise do
		 * almost nothing.
		 */

		mx_warning( "'manual_next_measurement' is not supported "
		"by the MX '%s' driver used by MCS record '%s'.",
			mx_get_driver_name( mcs->record ), mcs->record->name );

		mcs->manual_next_measurement = FALSE;
		break;

	case MXLV_MCS_CLEAR_DEADBAND:

#if MX_MCS_DEBUG_CLEAR_DEADBAND
		MX_DEBUG(-2,("%s invoked for clear_deadband = %g seconds.",
			fname, mcs->clear_deadband));
#endif

		mcs->clear_deadband_ticks =
		      mx_convert_seconds_to_clock_ticks( mcs->clear_deadband );

		mcs->next_clear_tick = mx_current_clock_tick();

#if MX_MCS_DEBUG_CLEAR_DEADBAND
		MX_DEBUG(-2,
	("%s: clear_deadband_ticks = (%lu,%lu), next_clear_tick = (%lu,%lu)",
			fname, mcs->clear_deadband_ticks.high_order,
			mcs->clear_deadband_ticks.low_order,
			mcs->next_clear_tick.high_order,
			mcs->next_clear_tick.low_order));
#endif
		mcs->new_data_available = TRUE;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Parameter type '%s' (%ld) is not supported by the MX driver for MCS '%s'.",
			mx_get_field_label_string( mcs->record,
						mcs->parameter_type ),
			mcs->parameter_type,
			mcs->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}


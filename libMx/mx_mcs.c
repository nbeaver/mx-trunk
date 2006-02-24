/*
 * Name:    mx_mcs.c
 *
 * Purpose: MX function library for multichannel scalers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"
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
mx_mcs_initialize_type( long record_type,
			long *num_record_fields,
			MX_RECORD_FIELD_DEFAULTS **record_field_defaults,
			long *maximum_num_scalers_varargs_cookie,
			long *maximum_num_measurements_varargs_cookie )
{
	static const char fname[] = "mx_mcs_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type status;

	if ( num_record_fields == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"num_record_fields pointer passed was NULL." );
	}
	if ( record_field_defaults == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"record_field_defaults pointer passed was NULL." );
	}
	if ( maximum_num_scalers_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"maximum_num_scalers_varargs_cookie pointer passed was NULL." );
	}
	if ( maximum_num_measurements_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"maximum_num_measurements_varargs_cookie pointer passed was NULL." );
	}

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr
			= driver->record_field_defaults_ptr;

	if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	*record_field_defaults = *record_field_defaults_ptr;

	if ( *record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (long *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	*num_record_fields = *(driver->num_record_fields);

	/* Set varargs cookies in 'data_array' that depend on the values
	 * of 'maximum_num_scalers' and 'maximum_num_measurements'.
	 */

	status = mx_find_record_field_defaults( *record_field_defaults,
			*num_record_fields, "data_array", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_find_record_field_defaults_index(
			*record_field_defaults, *num_record_fields,
			"maximum_num_scalers", &referenced_field_index );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_construct_varargs_cookie( referenced_field_index, 0,
					maximum_num_scalers_varargs_cookie );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_find_record_field_defaults_index(
			*record_field_defaults, *num_record_fields,
			"maximum_num_measurements", &referenced_field_index );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_construct_varargs_cookie( referenced_field_index, 0,
				maximum_num_measurements_varargs_cookie );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = *maximum_num_scalers_varargs_cookie;
	field->dimension[1] = *maximum_num_measurements_varargs_cookie;

	/* 'timer_data' depends on 'maximum_num_measurements'. */

	status = mx_find_record_field_defaults( *record_field_defaults,
			*num_record_fields, "timer_data", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = *maximum_num_measurements_varargs_cookie;

	/* 'measurement_data' depends on 'maximum_num_scalers'. */

	status = mx_find_record_field_defaults( *record_field_defaults,
			*num_record_fields, "measurement_data", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = *maximum_num_scalers_varargs_cookie;

	/* 'dark_current_array' depends on 'maximum_num_scalers'. */

	status = mx_find_record_field_defaults( *record_field_defaults,
			*num_record_fields, "dark_current_array", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

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
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

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

	mcs->readout_preference = MXF_MCS_PREFER_READ_SCALER;

	mcs->external_channel_advance_record = NULL;
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

	status = mx_find_record_field( mcs_record, "scaler_data",
					&scaler_data_field );

	if ( status.code != MXE_SUCCESS )
		return status;

	scaler_data_field->dimension[0] = mcs->maximum_num_measurements;

	mcs->scaler_index = 0;

	mcs->scaler_data = mcs->data_array[0];

	status = mx_find_record_field( mcs_record, "measurement_data",
					&measurement_data_field );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_find_record_field( mcs_record, "timer_data",
					&timer_data_field );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* If the 'external_channel_advance_name' field has a non-zero
	 * length value then make mcs->external_channel_advance_record
	 * point to the record with that name.
	 */

	if ( strlen( mcs->external_channel_advance_name ) > 0 ) {

		mcs->external_channel_advance_record = mx_get_record(mcs_record,
				mcs->external_channel_advance_name );

		if ( mcs->external_channel_advance_record == (MX_RECORD *)NULL){
			return mx_error( MXE_NOT_FOUND, fname,
	"Record '%s' was specified as the external channel advance record "
	"for MCS '%s'.  However, record '%s' does not exist.",
				mcs_record->name,
				mcs->external_channel_advance_name,
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

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_mcs_start( MX_RECORD *mcs_record )
{
	static const char fname[] = "mx_mcs_start()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"start function ptr for record '%s' is NULL.",
			mcs_record->name );
	}

	status = (*start_fn)( mcs );

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_stop( MX_RECORD *mcs_record )
{
	static const char fname[] = "mx_mcs_stop()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *stop_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	stop_fn = function_list->stop;

	if ( stop_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"stop function ptr for record '%s' is NULL.",
			mcs_record->name );
	}

	status = (*stop_fn)( mcs );

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_clear( MX_RECORD *mcs_record )
{
	static const char fname[] = "mx_mcs_clear()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *clear_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	clear_fn = function_list->clear;

	if ( clear_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"clear function ptr for record '%s' is NULL.",
			mcs_record->name );
	}

	status = (*clear_fn)( mcs );

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_is_busy( MX_RECORD *mcs_record, int *busy )
{
	static const char fname[] = "mx_mcs_is_busy()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *busy_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	busy_fn = function_list->busy;

	if ( busy_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"busy function ptr for record '%s' is NULL.",
			mcs_record->name );
	}

	status = (*busy_fn)( mcs );

	if ( busy != NULL ) {
		*busy = mcs->busy;
	}

	return status;
}

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
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	read_all_fn = function_list->read_all;

	if ( read_all_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"Reading the MCS data as one big block is not supported for MCS '%s'.",
			mcs_record->name );
	}

	status = (*read_all_fn)( mcs );

	if ( status.code != MXE_SUCCESS )
		return status;

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
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	read_scaler_fn = function_list->read_scaler;

	if ( read_scaler_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Reading the MCS data one scaler channel at a time "
			"is not supported for MCS '%s'.",
			mcs_record->name );
	}

	if ( ((long) scaler_index) >= mcs->maximum_num_scalers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Scaler index %lu for MCS record '%s' is outside the allowed range of 0-%ld.",
			scaler_index, mcs_record->name,
			mcs->maximum_num_scalers - 1L );
	}

	mcs->scaler_index = (long) scaler_index;

	status = (*read_scaler_fn)( mcs );

	if ( status.code != MXE_SUCCESS )
		return status;

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
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	read_measurement_fn = function_list->read_measurement;

	if ( read_measurement_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Reading the MCS data one measurement at a time "
			"is not supported for MCS '%s'.",
			mcs_record->name );
	}

	if ( ((long) measurement_index) >= mcs->maximum_num_measurements ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Measurement index %lu for MCS record '%s' is outside "
		"the allowed range of 0-%ld.",
			measurement_index, mcs_record->name,
			mcs->maximum_num_measurements - 1L );
	}

	mcs->measurement_index = (long) measurement_index;

	status = (*read_measurement_fn)( mcs );

	if ( num_scalers != NULL ) {
		*num_scalers = mcs->current_num_scalers;
	}
	if ( measurement_data != NULL ) {
		*measurement_data = mcs->measurement_data;
	}

	return status;
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
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	read_timer_fn = function_list->read_timer;

	if ( read_timer_fn == NULL ) {
		for ( i = 0; i < mcs->current_num_measurements; i++ ) {
			(mcs->timer_data)[i] = 0.0;
		}
	} else {
		status = (*read_timer_fn)( mcs );
	}

	if ( num_measurements != NULL ) {
		*num_measurements = mcs->current_num_measurements;
	}
	if ( timer_data != NULL ) {
		*timer_data = mcs->timer_data;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_get_mode( MX_RECORD *mcs_record, long *mode )
{
	static const char fname[] = "mx_mcs_get_mode()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MODE;

	status = (*get_parameter_fn)( mcs );

	if ( mode != NULL ) {
		*mode = mcs->mode;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_set_mode( MX_RECORD *mcs_record, long mode )
{
	static const char fname[] = "mx_mcs_set_mode()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MODE;

	mcs->mode = mode;

	status = (*set_parameter_fn)( mcs );

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_get_external_channel_advance( MX_RECORD *mcs_record,
					int *external_channel_advance )
{
	static const char fname[] = "mx_mcs_get_external_channel_advance()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_EXTERNAL_CHANNEL_ADVANCE;

	status = (*get_parameter_fn)( mcs );

	if ( external_channel_advance != NULL ) {
		*external_channel_advance = mcs->external_channel_advance;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_set_external_channel_advance( MX_RECORD *mcs_record,
					int external_channel_advance )
{
	static const char fname[] = "mx_mcs_set_external_channel_advance()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_EXTERNAL_CHANNEL_ADVANCE;

	mcs->external_channel_advance = external_channel_advance;

	status = (*set_parameter_fn)( mcs );

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_get_external_prescale( MX_RECORD *mcs_record,
					unsigned long *external_prescale )
{
	static const char fname[] = "mx_mcs_get_external_prescale()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_EXTERNAL_PRESCALE;

	status = (*get_parameter_fn)( mcs );

	if ( external_prescale != NULL ) {
		*external_prescale = mcs->external_prescale;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_set_external_prescale( MX_RECORD *mcs_record,
					unsigned long external_prescale )
{
	static const char fname[] = "mx_mcs_set_external_prescale()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_EXTERNAL_PRESCALE;

	mcs->external_prescale = external_prescale;

	status = (*set_parameter_fn)( mcs );

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_get_measurement_time( MX_RECORD *mcs_record, double *measurement_time )
{
	static const char fname[] = "mx_mcs_get_measurement_time()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MEASUREMENT_TIME;

	status = (*get_parameter_fn)( mcs );

	if ( measurement_time != NULL ) {
		*measurement_time = mcs->measurement_time;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_set_measurement_time( MX_RECORD *mcs_record, double measurement_time )
{
	static const char fname[] = "mx_mcs_set_measurement_time()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MEASUREMENT_TIME;

	mcs->measurement_time = measurement_time;

	status = (*set_parameter_fn)( mcs );

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_get_measurement_counts( MX_RECORD *mcs_record,
					unsigned long *measurement_counts )
{
	static const char fname[] = "mx_mcs_get_measurement_counts()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MEASUREMENT_COUNTS;

	status = (*get_parameter_fn)( mcs );

	if ( measurement_counts != NULL ) {
		*measurement_counts = mcs->measurement_counts;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_set_measurement_counts( MX_RECORD *mcs_record,
					unsigned long measurement_counts )
{
	static const char fname[] = "mx_mcs_set_measurement_counts()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_mcs_default_set_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_MEASUREMENT_COUNTS;

	mcs->measurement_counts = measurement_counts;

	status = (*set_parameter_fn)( mcs );

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_get_num_measurements( MX_RECORD *mcs_record,
					unsigned long *num_measurements )
{
	static const char fname[] = "mx_mcs_get_num_measurements()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_mcs_default_get_parameter_handler;
	}

	mcs->parameter_type = MXLV_MCS_CURRENT_NUM_MEASUREMENTS;

	status = (*get_parameter_fn)( mcs );

	if ( num_measurements != NULL ) {
		*num_measurements = mcs->current_num_measurements;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_set_num_measurements( MX_RECORD *mcs_record,
					unsigned long num_measurements )
{
	static const char fname[] = "mx_mcs_set_num_measurements()";

	MX_MCS *mcs;
	MX_MCS_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_MCS * );
	mx_status_type status;

	status = mx_mcs_get_pointers( mcs_record,
					&mcs, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

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

	status = (*set_parameter_fn)( mcs );

	return status;
}

MX_EXPORT mx_status_type
mx_mcs_get_dark_current_array( MX_RECORD *mcs_record,
				unsigned long num_scalers,
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

	if ( num_scalers > mcs->current_num_scalers ) {
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

	if ( dark_current_array != NULL ) {
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

MX_EXPORT mx_status_type
mx_mcs_default_get_parameter_handler( MX_MCS *mcs )
{
	static const char fname[] = "mx_mcs_default_get_parameter_handler()";

	switch( mcs->parameter_type ) {
	case MXLV_MCS_MODE:
	case MXLV_MCS_EXTERNAL_CHANNEL_ADVANCE:
	case MXLV_MCS_EXTERNAL_PRESCALE:
	case MXLV_MCS_MEASUREMENT_TIME:
	case MXLV_MCS_MEASUREMENT_COUNTS:
	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
	case MXLV_MCS_DARK_CURRENT:

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
	case MXLV_MCS_MODE:
	case MXLV_MCS_EXTERNAL_CHANNEL_ADVANCE:
	case MXLV_MCS_EXTERNAL_PRESCALE:
	case MXLV_MCS_MEASUREMENT_TIME:
	case MXLV_MCS_MEASUREMENT_COUNTS:
	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
	case MXLV_MCS_DARK_CURRENT:

		/* We do nothing but leave alone the value that is already
		 * stored in the data structure.
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


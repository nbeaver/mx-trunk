/*
 * Name:    mx_autoscale.c
 *
 * Purpose: Implements the MX autoscale device class.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_autoscale.h"

static mx_status_type
mx_autoscale_get_pointers( MX_RECORD *autoscale_record,
			MX_AUTOSCALE **autoscale,
			MX_AUTOSCALE_FUNCTION_LIST **function_list,
			const char *calling_fname )
{
	const char fname[] = "mx_autoscale_get_pointers()";

	if ( autoscale_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The autoscale record pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( autoscale == (MX_AUTOSCALE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
"The autoscale pointer passed by '%s' for record '%s' is NULL.",
			calling_fname, autoscale_record->name );
	}
	if ( function_list == (MX_AUTOSCALE_FUNCTION_LIST **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"The function_list pointer passed by '%s' for record '%s' is NULL.",
			calling_fname, autoscale_record->name );
	}
	if ( autoscale_record->mx_class != MXC_AUTOSCALE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The record '%s' passed by '%s' is not an autoscale record.",
			autoscale_record->name, calling_fname );
	}

	*autoscale = (MX_AUTOSCALE *) autoscale_record->record_class_struct;

	if ( *autoscale == (MX_AUTOSCALE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_AUTOSCALE pointer for record '%s' passed by '%s' is NULL.",
			autoscale_record->name, calling_fname );
	}

	*function_list = (MX_AUTOSCALE_FUNCTION_LIST *)
				autoscale_record->class_specific_function_list;

	if ( *function_list == (MX_AUTOSCALE_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AUTOSCALE_FUNCTION_LIST pointer for "
		"record '%s' passed by '%s' is NULL.",
			autoscale_record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_autoscale_read_monitor( MX_RECORD *autoscale_record, long *monitor_value )
{
	const char fname[] = "mx_autoscale_read_monitor()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_FUNCTION_LIST *function_list;
	mx_status_type ( *read_monitor_fn ) ( MX_AUTOSCALE * );
	mx_status_type mx_status;

	mx_status = mx_autoscale_get_pointers( autoscale_record,
						&autoscale,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_monitor_fn = function_list->read_monitor;

	if ( read_monitor_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"read_monitor function pointer for autoscale record '%s' is NULL.",
			autoscale_record->name );
	}

	mx_status = (*read_monitor_fn)( autoscale );

	if ( monitor_value != (long *) NULL ) {
		*monitor_value = autoscale->monitor_value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_autoscale_get_change_request( MX_RECORD *autoscale_record,
				int *change_request )
{
	const char fname[] = "mx_autoscale_get_change_request()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_FUNCTION_LIST *function_list;
	mx_status_type ( *get_change_request_fn ) ( MX_AUTOSCALE * );
	mx_status_type mx_status;

	mx_status = mx_autoscale_get_pointers( autoscale_record,
						&autoscale,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_change_request_fn = function_list->get_change_request;

	if ( get_change_request_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"get_change_request function pointer for autoscale record '%s' is NULL.",
			autoscale_record->name );
	}

	mx_status = (*get_change_request_fn)( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( change_request != NULL ) {
		*change_request = autoscale->get_change_request;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_autoscale_change_control( MX_RECORD *autoscale_record, int change_control )
{
	const char fname[] = "mx_autoscale_change_control()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_FUNCTION_LIST *function_list;
	mx_status_type ( *change_control_fn ) ( MX_AUTOSCALE * );
	mx_status_type mx_status;

	mx_status = mx_autoscale_get_pointers( autoscale_record,
						&autoscale,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	change_control_fn = function_list->change_control;

	if ( change_control_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"change_control function pointer for autoscale record '%s' is NULL.",
			autoscale_record->name );
	}

	autoscale->change_control = change_control;

	mx_status = (*change_control_fn)( autoscale );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_autoscale_get_offset_index( MX_RECORD *autoscale_record,
				unsigned long *monitor_offset_index )
{
	const char fname[] = "mx_autoscale_get_offset_index()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_FUNCTION_LIST *function_list;
	mx_status_type ( *get_offset_index_fn ) ( MX_AUTOSCALE * );
	mx_status_type mx_status;

	mx_status = mx_autoscale_get_pointers( autoscale_record,
						&autoscale,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_offset_index_fn = function_list->get_offset_index;

	if ( get_offset_index_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_offset_index function pointer for autoscale record '%s' is NULL.",
			autoscale_record->name );
	}

	mx_status = (*get_offset_index_fn)( autoscale );

	if ( monitor_offset_index != (unsigned long *) NULL ) {
		*monitor_offset_index = autoscale->monitor_offset_index;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_autoscale_set_offset_index( MX_RECORD *autoscale_record,
				unsigned long monitor_offset_index )
{
	const char fname[] = "mx_autoscale_set_offset_index()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_FUNCTION_LIST *function_list;
	mx_status_type ( *set_offset_index_fn ) ( MX_AUTOSCALE * );
	mx_status_type mx_status;

	mx_status = mx_autoscale_get_pointers( autoscale_record,
						&autoscale,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( monitor_offset_index >= autoscale->num_monitor_offsets ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested monitor offset index %lu is larger than "
		"the largest allowed value of %lu.",
			monitor_offset_index,
			autoscale->num_monitor_offsets - 1L );
	}

	set_offset_index_fn = function_list->set_offset_index;

	if ( set_offset_index_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"set_offset_index function pointer for autoscale record '%s' is NULL.",
			autoscale_record->name );
	}

	autoscale->monitor_offset_index = monitor_offset_index;

	mx_status = (*set_offset_index_fn)( autoscale );

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_autoscale_default_get_parameter_handler( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mx_autoscale_default_get_parameter_handler()";

	MX_DEBUG( 2,("%s invoked for autoscale '%s', parameter type '%s' (%d).",
		fname, autoscale->record->name,
		mx_get_field_label_string(autoscale->record,
					autoscale->parameter_type),
		autoscale->parameter_type));

	switch( autoscale->parameter_type ) {
	case MXLV_AUT_LOW_LIMIT:
	case MXLV_AUT_HIGH_LIMIT:
	case MXLV_AUT_LOW_DEADBAND:
	case MXLV_AUT_HIGH_DEADBAND:
	case MXLV_AUT_MONITOR_OFFSET_ARRAY:
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Parameter type '%s' (%d) is not supported by the MX driver for autoscale '%s'.",
			mx_get_field_label_string( autoscale->record,
						autoscale->parameter_type ),
			autoscale->parameter_type,
			autoscale->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_autoscale_default_set_parameter_handler( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mx_autoscale_default_set_parameter_handler()";

	MX_DEBUG( 2,(
	"%s invoked for autoscale '%s', parameter type '%s' (%d).",
		fname, autoscale->record->name,
		mx_get_field_label_string(autoscale->record,
					autoscale->parameter_type),
		autoscale->parameter_type));


	switch( autoscale->parameter_type ) {
	case MXLV_AUT_LOW_LIMIT:
	case MXLV_AUT_HIGH_LIMIT:
	case MXLV_AUT_LOW_DEADBAND:
	case MXLV_AUT_HIGH_DEADBAND:
	case MXLV_AUT_MONITOR_OFFSET_ARRAY:
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Parameter type '%s' (%d) is not supported by the MX driver for autoscale '%s'.",
			mx_get_field_label_string( autoscale->record,
						autoscale->parameter_type ),
			autoscale->parameter_type,
			autoscale->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_autoscale_get_offset_array( MX_RECORD *autoscale_record,
				unsigned long *num_monitor_offsets,
				double **monitor_offset_array )
{
	const char fname[] = "mx_autoscale_get_offset_array()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_AUTOSCALE * );
	mx_status_type mx_status;

	mx_status = mx_autoscale_get_pointers( autoscale_record,
						&autoscale,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function pointer for autoscale record '%s' is NULL.",
			autoscale_record->name );
	}

	autoscale->parameter_type = MXLV_AUT_MONITOR_OFFSET_ARRAY;

	mx_status = (*get_parameter_fn)( autoscale );

	if ( num_monitor_offsets != (unsigned long *) NULL ) {
		*num_monitor_offsets = autoscale->num_monitor_offsets;
	}

	if ( monitor_offset_array != (double **) NULL ) {
		*monitor_offset_array = autoscale->monitor_offset_array;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_autoscale_set_offset_array( MX_RECORD *autoscale_record,
				unsigned long num_monitor_offsets,
				double *monitor_offset_array )
{
	const char fname[] = "mx_autoscale_set_offset_array()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_FUNCTION_LIST *function_list;
	long i;
	mx_status_type ( *set_parameter_fn ) ( MX_AUTOSCALE * );
	mx_status_type mx_status;

	mx_status = mx_autoscale_get_pointers( autoscale_record,
						&autoscale,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"set_parameter function pointer for autoscale record '%s' is NULL.",
			autoscale_record->name );
	}

	if ( num_monitor_offsets > autoscale->num_monitor_offsets ) {
		num_monitor_offsets = autoscale->num_monitor_offsets;
	}

	for ( i = 0; i < num_monitor_offsets; i++ ) {
		(autoscale->monitor_offset_array)[i] = monitor_offset_array[i];
	}

	autoscale->parameter_type = MXLV_AUT_MONITOR_OFFSET_ARRAY;

	mx_status = (*set_parameter_fn)( autoscale );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_autoscale_get_limits( MX_RECORD *autoscale_record,
			double *low_limit, double *high_limit,
			double *low_deadband, double *high_deadband )
{
	const char fname[] = "mx_autoscale_get_limits()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_AUTOSCALE * );
	mx_status_type mx_status;

	mx_status = mx_autoscale_get_pointers( autoscale_record,
						&autoscale,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function pointer for autoscale record '%s' is NULL.",
			autoscale_record->name );
	}

	/* Get all the limits and deadbands. */

	autoscale->parameter_type = MXLV_AUT_LOW_LIMIT;

	mx_status = (*get_parameter_fn)( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autoscale->parameter_type = MXLV_AUT_HIGH_LIMIT;

	mx_status = (*get_parameter_fn)( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autoscale->parameter_type = MXLV_AUT_LOW_DEADBAND;

	mx_status = (*get_parameter_fn)( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autoscale->parameter_type = MXLV_AUT_HIGH_DEADBAND;

	mx_status = (*get_parameter_fn)( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( low_limit != NULL ) {
		*low_limit = autoscale->low_limit;
	}
	if ( high_limit != NULL ) {
		*high_limit = autoscale->high_limit;
	}
	if ( low_deadband != NULL ) {
		*low_deadband = autoscale->low_deadband;
	}
	if ( high_deadband != NULL ) {
		*high_deadband = autoscale->high_deadband;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_autoscale_set_limits( MX_RECORD *autoscale_record,
			double low_limit, double high_limit,
			double low_deadband, double high_deadband )
{
	const char fname[] = "mx_autoscale_set_limits()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_AUTOSCALE * );
	mx_status_type mx_status;

	mx_status = mx_autoscale_get_pointers( autoscale_record,
						&autoscale,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"set_parameter function pointer for autoscale record '%s' is NULL.",
			autoscale_record->name );
	}

	autoscale->low_limit = low_limit;
	autoscale->high_limit = high_limit;
	autoscale->low_deadband = low_deadband;
	autoscale->high_deadband = high_deadband;

	/* Set all the limits and deadbands. */

	autoscale->parameter_type = MXLV_AUT_LOW_LIMIT;

	mx_status = (*set_parameter_fn)( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autoscale->parameter_type = MXLV_AUT_HIGH_LIMIT;

	mx_status = (*set_parameter_fn)( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autoscale->parameter_type = MXLV_AUT_LOW_DEADBAND;

	mx_status = (*set_parameter_fn)( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autoscale->parameter_type = MXLV_AUT_HIGH_DEADBAND;

	mx_status = (*set_parameter_fn)( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_autoscale_compute_dynamic_limits( MX_RECORD *autoscale_record,
				double *dynamic_low_limit,
				double *dynamic_high_limit )
{
	const char fname[] = "mx_autoscale_compute_dynamic_limits()";

	MX_AUTOSCALE *autoscale;
	MX_AUTOSCALE_FUNCTION_LIST *function_list;
	mx_status_type mx_status;

	mx_status = mx_autoscale_get_pointers( autoscale_record,
						&autoscale,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( dynamic_low_limit == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The dynamic_low_limit pointer passed is NULL." );
	}
	if ( dynamic_high_limit == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The dynamic_high_limit pointer passed is NULL." );
	}

	if ( autoscale->last_limit_tripped == MXF_AUTO_LOW_LIMIT_TRIPPED ) {

		*dynamic_low_limit = autoscale->low_limit;

		*dynamic_high_limit = autoscale->high_limit
						+ autoscale->high_deadband;
	} else
	if ( autoscale->last_limit_tripped == MXF_AUTO_HIGH_LIMIT_TRIPPED ) {

		*dynamic_low_limit = autoscale->low_limit
					- autoscale->low_deadband;

		*dynamic_high_limit = autoscale->high_limit;
	} else {
		*dynamic_low_limit = autoscale->low_limit;
		*dynamic_high_limit = autoscale->high_limit;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mx_autoscale_create_monitor_offset_array() gets its own function because
 * the record may not know how long the monitor offset array is until when
 * the mxd_..._open function is called.  This means that we must manually
 * patch the length of the array in the MX_RECORD_FIELD structure after we
 * have allocated the memory for the array.  Be sure to have initialized
 * the value of autoscale->num_monitor_offsets before calling this function.
 */

MX_EXPORT mx_status_type
mx_autoscale_create_monitor_offset_array( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mx_autoscale_create_monitor_offset_array()";

	MX_RECORD_FIELD *field;
	unsigned long i;
	mx_status_type mx_status;

	/* First, verify that we can find the "monitor_offset_array" field. */

	mx_status = mx_find_record_field( autoscale->record,
					"monitor_offset_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( autoscale->num_monitor_offsets == 0 ) {
		autoscale->monitor_offset_array = NULL;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Now allocate the memory for the array.  The value of
	 * autoscale->num_monitor_offsets must have been set before
	 * this function is called.
	 */

	autoscale->monitor_offset_array = (double *) malloc(
		autoscale->num_monitor_offsets * sizeof(double) );

	if ( autoscale->monitor_offset_array == (double *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate a %lu element array of doubles for "
"the monitor_offset_array field of record '%s'.",
			(unsigned long) autoscale->num_monitor_offsets,
			autoscale->record->name );
	}

	/* Initialize the contents of the array to zero. */

	for ( i = 0; i < autoscale->num_monitor_offsets; i++ ) {

		autoscale->monitor_offset_array[i] = 0.0;
	}

	/* Finally, set the length of the array in the MX_RECORD_FIELD
	 * structure.
	 */

	field->dimension[0] = (long) autoscale->num_monitor_offsets;

	return MX_SUCCESSFUL_RESULT;
}


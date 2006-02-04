/*
 * Name:    mx_analog_input.c
 *
 * Purpose: MX function library for analog input devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_timer.h"
#include "mx_measurement.h"
#include "mx_analog_input.h"

/*=======================================================================*/

static mx_status_type
mx_analog_input_get_pointers( MX_RECORD *record,
				MX_ANALOG_INPUT **analog_input,
				MX_ANALOG_INPUT_FUNCTION_LIST **flist_ptr,
				const char *calling_fname )
{
	static const char fname[] = "mx_analog_input_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( record->mx_class != MXC_ANALOG_INPUT ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not an analog input record.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			record->name, calling_fname,
			record->mx_superclass,
			record->mx_class,
			record->mx_type );
	}

	if ( analog_input != (MX_ANALOG_INPUT **) NULL ) {
		*analog_input = (MX_ANALOG_INPUT *)
						(record->record_class_struct);

		if ( *analog_input == (MX_ANALOG_INPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ANALOG_INPUT pointer for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	if ( flist_ptr != (MX_ANALOG_INPUT_FUNCTION_LIST **) NULL ) {
		*flist_ptr = (MX_ANALOG_INPUT_FUNCTION_LIST *)
				(record->class_specific_function_list);

		if ( *flist_ptr == (MX_ANALOG_INPUT_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_ANALOG_INPUT_FUNCTION_LIST ptr for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_analog_input_finish_record_initialization( MX_RECORD *analog_input_record )
{
	static const char fname[] =
			"mx_analog_input_finish_record_initialization()";

	MX_ANALOG_INPUT *analog_input;
	mx_status_type mx_status;

	mx_status = mx_analog_input_get_pointers( analog_input_record,
						&analog_input, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If no timer record name has been specified, this means that we
	 * do not want to associate a timer with this analog input record.
	 * Thus, we set analog_input->timer_record to NULL to indicate this.
	 */

	if ( strlen( analog_input->timer_record_name ) == 0 ) {
		analog_input->timer_record = NULL;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Save a pointer to the associated timer record. */

	analog_input->timer_record = mx_get_record( analog_input_record,
					analog_input->timer_record_name );

	if ( analog_input->timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Analog input '%s' says that it uses timer '%s', "
		"but that timer does not exist.",
			analog_input_record->name,
			analog_input->timer_record_name );
	}

	if ( ( analog_input->timer_record->mx_superclass != MXR_DEVICE )
	  || ( analog_input->timer_record->mx_class != MXC_TIMER ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Analog input '%s' says that it uses timer '%s', "
		"but record '%s' is not a timer record.",
			analog_input_record->name,
			analog_input->timer_record_name,
			analog_input->timer_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_analog_input_read( MX_RECORD *record, double *value_ptr )
{
	static const char fname[] = "mx_analog_input_read()";

	MX_ANALOG_INPUT *analog_input;
	MX_ANALOG_INPUT_FUNCTION_LIST *function_list;
	MX_RECORD *timer_record;
	double value, raw_value;
	mx_status_type ( *read_fn ) ( MX_ANALOG_INPUT * );
	int timer_mode, subtract_dark_current, normalize_to_value_per_second;
	double normalized_dark_current, last_measurement_time;
	mx_status_type mx_status;

	mx_status = mx_analog_input_get_pointers( record, &analog_input,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_fn = function_list->read;

	if ( read_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"read function ptr for MX_ANALOG_INPUT ptr 0x%p is NULL.",
			analog_input);
	}

	/* We only subtract a dark current here if the analog input has
	 * the MXF_AIN_SUBTRACT_DARK_CURRENT flag set.  However the
	 * details can vary depending on how other settings are set.
	 *
	 * If no timer record is specified, the unmodified dark current
	 * will be subtracted from the value read.  If a timer record,
	 * is specified, then the dark current will be stored as a normalized
	 * value per second.  The value reported to the caller will then
	 * depend on whether or not the MXF_AIN_PERFORM_TIME_NORMALIZATION
	 * flag is set.  In addition, if the timer is not in preset time
	 * mode, then no dark current subtraction will be performed.
	 *
	 * Please note that it is not necessary for this function to
	 * subtract a dark current if the MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT
	 * flag is set, since in that case the server will already have
	 * subtracted the dark current before it sends us a value.
	 */

	subtract_dark_current = FALSE;
	normalize_to_value_per_second = FALSE;

	if ( analog_input->analog_input_flags & MXF_AIN_SUBTRACT_DARK_CURRENT )
	{
		subtract_dark_current = TRUE;
	}

	if ( analog_input->analog_input_flags
					& MXF_AIN_PERFORM_TIME_NORMALIZATION )
	{
		normalize_to_value_per_second = TRUE;
	}

	timer_record = analog_input->timer_record;

	if ( timer_record == NULL ) {
		/* Cannot normalize if we have no way of finding
		 * the measurement time.
		 */

		normalize_to_value_per_second = FALSE;
	}

	mx_status = (*read_fn)( analog_input );

	if ( analog_input->subclass == MXT_AIN_INT32 ) {
		raw_value = (double) analog_input->raw_value.int32_value;
	} else {
		raw_value = analog_input->raw_value.double_value;
	}

	value = analog_input->offset + analog_input->scale * raw_value;

	if ( subtract_dark_current ) {

		if ( timer_record == NULL ) {
			/* No timer was specified. */

			value = value - analog_input->dark_current;

		} else {
			/* A timer was specified. */

			mx_status = mx_timer_get_mode( timer_record,
							&timer_mode );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( timer_mode != MXCM_PRESET_MODE ) {
				/* The timer was not in preset time mode
				 * so no dark current will be subtracted.
				 */

			} else {
				/* The timer _was_ in preset time mode. */

				normalized_dark_current =
						analog_input->dark_current;

				mx_status = mx_timer_get_last_measurement_time(
						timer_record,
						&last_measurement_time );

				if ( normalize_to_value_per_second ) {
					value = mx_divide_safely( value,
						    last_measurement_time )
						- normalized_dark_current;
				} else {
					value = value - (normalized_dark_current
						       * last_measurement_time);
				}
			}
		}
	} else {
		/* NO dark current subtraction. */

		if ( normalize_to_value_per_second ) {

			mx_status = mx_timer_get_last_measurement_time(
					timer_record, &last_measurement_time );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			value = mx_divide_safely(value, last_measurement_time);
		}
	}

	analog_input->value = value;

	if ( value_ptr != NULL ) {
		*value_ptr = analog_input->value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_analog_input_read_raw_int32( MX_RECORD *record, int32_t *raw_value )
{
	static const char fname[] = "mx_analog_input_read_raw_long()";

	MX_ANALOG_INPUT *analog_input;
	MX_ANALOG_INPUT_FUNCTION_LIST *function_list;
	int32_t local_raw_value;
	mx_status_type ( *read_fn ) ( MX_ANALOG_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_analog_input_get_pointers( record, &analog_input,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( analog_input->subclass != MXT_AIN_INT32 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"This function may only be used with analog inputs that return values "
	"as long integers.  The record '%s' returns values as doubles so "
	"you should use mx_analog_input_read_raw_double() with this record.",
			record->name );
	}

	read_fn = function_list->read;

	if ( read_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"read function ptr for MX_ANALOG_INPUT ptr 0x%p is NULL.",
			analog_input);
	}

	mx_status = (*read_fn)( analog_input );

	local_raw_value = analog_input->raw_value.int32_value;

	analog_input->value = analog_input->offset + analog_input->scale
					* (double) local_raw_value;

	if ( raw_value != NULL ) {
		*raw_value = local_raw_value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_analog_input_read_raw_double( MX_RECORD *record, double *raw_value )
{
	static const char fname[] = "mx_analog_input_read_raw_double()";

	MX_ANALOG_INPUT *analog_input;
	MX_ANALOG_INPUT_FUNCTION_LIST *function_list;
	double local_raw_value;
	mx_status_type ( *read_fn ) ( MX_ANALOG_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_analog_input_get_pointers( record, &analog_input,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( analog_input->subclass != MXT_AIN_DOUBLE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"This function may only be used with analog inputs that return values "
	"as doubles.  The record '%s' returns values as long integers so "
	"you should use mx_analog_input_read_raw_long() with this record.",
			record->name );
	}

	read_fn = function_list->read;

	if ( read_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"read function ptr for MX_ANALOG_INPUT ptr 0x%p is NULL.",
			analog_input);
	}

	mx_status = (*read_fn)( analog_input );

	local_raw_value = analog_input->raw_value.double_value;

	analog_input->value = analog_input->offset + analog_input->scale
					* local_raw_value;

	if ( raw_value != NULL ) {
		*raw_value = local_raw_value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_analog_input_get_dark_current( MX_RECORD *record, double *dark_current )
{
	static const char fname[] = "mx_analog_input_get_dark_current()";

	MX_ANALOG_INPUT *analog_input;
	MX_ANALOG_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *get_dark_current_fn ) ( MX_ANALOG_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_analog_input_get_pointers( record, &analog_input,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_dark_current_fn = function_list->get_dark_current;

	/* If get_dark_current_fn is not NULL, invoke it.  Otherwise, we
	 * just return the value already in analog_input->dark_current.
	 */

	if ( get_dark_current_fn != NULL ) {
		mx_status = (*get_dark_current_fn)( analog_input );
	}

	if ( dark_current != NULL ) {
		*dark_current = analog_input->dark_current;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_analog_input_set_dark_current( MX_RECORD *record, double dark_current )
{
	static const char fname[] = "mx_analog_input_set_dark_current()";

	MX_ANALOG_INPUT *analog_input;
	MX_ANALOG_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *set_dark_current_fn ) ( MX_ANALOG_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_analog_input_get_pointers( record, &analog_input,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_dark_current_fn = function_list->set_dark_current;

	/* If set_dark_current_fn is not NULL, invoke it.  Otherwise, we
	 * just store the value in analog_input->dark_current.
	 */

	analog_input->dark_current = dark_current;

	if ( set_dark_current_fn != NULL ) {
		mx_status = (*set_dark_current_fn)( analog_input );
	}

	return MX_SUCCESSFUL_RESULT;
}


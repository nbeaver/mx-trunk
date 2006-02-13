/*
 * Name:    mx_scaler.c
 *
 * Purpose: MX function library of generic scaler operations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2004, 2006 Illinois Institute of Technology
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
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "mx_scaler.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_scaler_...
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_scaler_get_pointers( MX_RECORD *scaler_record,
			MX_SCALER **scaler,
			MX_SCALER_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_scaler_get_pointers()";

	if ( scaler_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The scaler_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( scaler_record->mx_class != MXC_SCALER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' passed by '%s' is not a scaler.",
			scaler_record->name, calling_fname );
	}

	if ( scaler != (MX_SCALER **) NULL ) {
		*scaler = (MX_SCALER *) (scaler_record->record_class_struct);

		if ( *scaler == (MX_SCALER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCALER pointer for record '%s' passed by '%s' is NULL",
				scaler_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_SCALER_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_SCALER_FUNCTION_LIST *)
				(scaler_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_SCALER_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_SCALER_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				scaler_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_scaler_finish_record_initialization( MX_RECORD *scaler_record )
{
	static const char fname[] = "mx_scaler_finish_record_initialization()";

	MX_SCALER *scaler;
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
						&scaler, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If no timer record name has been specified, this means that we
	 * do not want to associate a timer with this scaler record.  Thus,
	 * we set scaler->timer_record to NULL to indicate this.
	 */

	if ( strlen( scaler->timer_record_name ) == 0 ) {
		scaler->timer_record = NULL;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Save a pointer to the associated timer record. */

	scaler->timer_record =
		mx_get_record( scaler_record, scaler->timer_record_name );

	if ( scaler->timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Scaler '%s' says that it uses timer '%s', "
		"but that timer does not exist.",
			scaler_record->name,
			scaler->timer_record_name );
	}

	if ( ( scaler->timer_record->mx_superclass != MXR_DEVICE )
	  || ( scaler->timer_record->mx_class != MXC_TIMER ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Scaler '%s' says that it uses timer '%s', "
		"but record '%s' is not a timer record.",
			scaler_record->name,
			scaler->timer_record_name,
			scaler->timer_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_scaler_clear( MX_RECORD *scaler_record )
{
	static const char fname[] = "mx_scaler_clear()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *clear_fn ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	clear_fn = function_list->clear;

	if ( clear_fn == NULL ) {
		scaler->raw_value = 0L;
	} else {
		mx_status = (*clear_fn)( scaler );
	}

	scaler->value = scaler->raw_value;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_scaler_overflow_set( MX_RECORD *scaler_record, mx_bool_type *overflow_set )
{
	static const char fname[] = "mx_scaler_overflow_set()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *overflow_set_fn ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	overflow_set_fn = function_list->overflow_set;

	if ( overflow_set_fn == NULL ) {
		scaler->overflow_set = FALSE;
	} else {
		mx_status = (*overflow_set_fn)( scaler );
	}

	if ( overflow_set != NULL ) {
		*overflow_set = scaler->overflow_set;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_scaler_read( MX_RECORD *scaler_record, int32_t *value )
{
	static const char fname[] = "mx_scaler_read()";

	MX_SCALER *scaler;
	MX_RECORD *timer_record;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *read_fn ) ( MX_SCALER * );
	int32_t offset;
	int subtract_dark_current, timer_mode;
	double last_measurement_time;
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_fn = function_list->read;

	if ( read_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'read' function pointer for scaler '%s' is NULL.",
			scaler_record->name );
	}

	subtract_dark_current = FALSE;
	last_measurement_time = 0.0;
	offset = 0;

	/* We only subtract a dark current here if:
	 *
	 * 1.  The scaler has the MXF_SCL_SUBTRACT_DARK_CURRENT flag set.
	 * 2.  There is a directly associated timer record.
	 * 3.  The timer record is in MXCM_PRESET_MODE, that is,
	 *     preset time mode.
	 *
	 * Please note that it is not necessary for this function to subtract
	 * a dark current if the MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT flag
	 * is set, since in that case the server will already have subtracted
	 * the dark current before it sends us a value.
	 */

	/* See if the MXF_SCL_SUBTRACT_DARK_CURRENT flag is set. */

	if ( scaler->scaler_flags & MXF_SCL_SUBTRACT_DARK_CURRENT ) {

		/* Check to see if there is a timer directly associated with
		 * this scaler.
		 */

		timer_record = scaler->timer_record;

		if ( timer_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Scaler record '%s' is configured to subtract a dark current, "
		"but no associated timer has been defined in the scaler's "
		"database entry.  You must either configure an associated "
		"timer record for this scaler, or turn the subtract dark "
		"current flag off.", scaler_record->name );
		}

		/* If the timer is in preset time mode, then we will subtract
		 * a dark current.  Otherwise, we assume that we are in 
		 * preset count mode where dark current subtraction is not
		 * supported.
		 */

		mx_status = mx_timer_get_mode( timer_record, &timer_mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( timer_mode == MXCM_PRESET_MODE ) {
			subtract_dark_current = TRUE;

			mx_status = mx_timer_get_last_measurement_time(
						timer_record,
						&last_measurement_time );

			if ( last_measurement_time <= 0.0 ) {
				last_measurement_time = 0.0;
			}
		}
	}

	/* Read the raw measurement. */

	mx_status = (*read_fn)( scaler );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute and subtract the dark current offset for this measurement
	 * if needed.
	 */

	if ( subtract_dark_current ) {
		offset = mx_round( scaler->dark_current
					* last_measurement_time );

		scaler->value = scaler->raw_value - offset;
	} else {
		scaler->value = scaler->raw_value;
	}

	if ( value != NULL ) {
		*value = scaler->value;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scaler_read_raw( MX_RECORD *scaler_record, int32_t *value )
{
	static const char fname[] = "mx_scaler_read_raw()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Use the read_raw function pointer unless it is NULL.
	 * In that case, use the read function instead.  The idea
	 * here is that the read_raw function is only needed if
	 * the software or hardware we are talking to has a raw 
	 * mode itself.
	 */

	if ( function_list->read_raw != NULL ) {
		fptr = function_list->read_raw;

	} else if ( function_list->read != NULL ) {
		fptr = function_list->read;
	} else {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Both the read and read_raw function pointers for "
			"scaler record '%s' are NULL.",
				scaler_record->name );
	}

	mx_status = (*fptr)( scaler );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler->value = scaler->raw_value;

	MX_DEBUG( 2,("%s: scaler '%s', value = %ld",
		fname, scaler->record->name, (long) scaler->value));

	if ( value != NULL ) {
		*value = scaler->value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_scaler_is_busy( MX_RECORD *scaler_record, mx_bool_type *busy )
{
	static const char fname[] = "mx_scaler_is_busy()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *busy_fn ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	busy_fn = function_list->scaler_is_busy;

	if ( busy_fn == NULL ) {
		scaler->busy = FALSE;
	} else {
		mx_status = (*busy_fn)( scaler );
	}

	if ( busy != NULL ) {
		*busy = scaler->busy;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_scaler_start( MX_RECORD *scaler_record, int32_t preset_count )
{
	static const char fname[] = "mx_scaler_start()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Starting '%s' scaler '%s' for a preset count is not "
			"currently supported.",
				mx_get_driver_name( scaler_record ),
				scaler_record->name );
	}

	scaler->value     = preset_count;
	scaler->raw_value = preset_count;

	mx_status = (*start_fn)( scaler );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_scaler_stop( MX_RECORD *scaler_record, int32_t *present_value )
{
	static const char fname[] = "mx_scaler_stop()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *stop_fn ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop_fn = function_list->stop;

	if ( stop_fn == NULL ) {
		scaler->raw_value = 0L;
	} else {
		mx_status = (*stop_fn)( scaler );
	}

	scaler->value = scaler->raw_value;

	if ( present_value != NULL ) {
		*present_value = scaler->value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_scaler_get_mode( MX_RECORD *scaler_record, int *mode )
{
	static const char fname[] = "mx_scaler_get_mode()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_scaler_default_get_parameter_handler;
	}

	scaler->parameter_type = MXLV_SCL_MODE;

	mx_status = (*get_parameter_fn)( scaler );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mode != NULL ) {
		*mode = scaler->mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_scaler_set_mode( MX_RECORD *scaler_record, int mode )
{
	static const char fname[] = "mx_scaler_set_mode()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_scaler_default_set_parameter_handler;
	}

	scaler->parameter_type = MXLV_SCL_MODE;

	scaler->mode = mode;

	mx_status = (*set_parameter_fn)( scaler );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_scaler_set_modes_of_associated_counters( MX_RECORD *scaler_record )
{
	static const char fname[] =
			"mx_scaler_set_modes_of_associated_counters()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *set_modes_of_associated_counters ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_modes_of_associated_counters =
			function_list->set_modes_of_associated_counters;

	/* Do nothing if the driver does not define this function. */

	if ( set_modes_of_associated_counters == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = (*set_modes_of_associated_counters)( scaler );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_scaler_get_dark_current( MX_RECORD *scaler_record, double *dark_current )
{
	static const char fname[] = "mx_scaler_get_dark_current()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_scaler_default_get_parameter_handler;
	}

	scaler->parameter_type = MXLV_SCL_DARK_CURRENT;

	mx_status = (*get_parameter_fn)( scaler );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( dark_current != NULL ) {
		*dark_current = scaler->dark_current;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scaler_set_dark_current( MX_RECORD *scaler_record, double dark_current )
{
	static const char fname[] = "mx_scaler_set_dark_current()";

	MX_SCALER *scaler;
	MX_SCALER_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_SCALER * );
	mx_status_type mx_status;

	mx_status = mx_scaler_get_pointers( scaler_record,
					&scaler, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_scaler_default_set_parameter_handler;
	}

	scaler->parameter_type = MXLV_SCL_DARK_CURRENT;

	scaler->dark_current = dark_current;

	mx_status = (*set_parameter_fn)( scaler );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_scaler_default_get_parameter_handler( MX_SCALER *scaler )
{
	static const char fname[] = "mx_scaler_default_get_parameter_handler()";

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
	case MXLV_SCL_DARK_CURRENT:

		/* We just return the value that is already in the 
		 * data structure.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Parameter type '%s' (%d) is not supported by the MX driver for scaler '%s'.",
			mx_get_field_label_string( scaler->record,
						scaler->parameter_type ),
			(int) scaler->parameter_type,
			scaler->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scaler_default_set_parameter_handler( MX_SCALER *scaler )
{
	static const char fname[] = "mx_scaler_default_set_parameter_handler()";

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
	case MXLV_SCL_DARK_CURRENT:

		/* We do nothing but leave alone the value that is already
		 * stored in the data structure.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Parameter type '%s' (%d) is not supported by the MX driver for scaler '%s'.",
			mx_get_field_label_string( scaler->record,
						scaler->parameter_type ),
			(int) scaler->parameter_type,
			scaler->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}


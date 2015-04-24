/*
 * Name:    mx_pulse_generator.c
 *
 * Purpose: MX function library of generic pulse generator and function
 *          generator operations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006, 2015 Illinois Institute of Technology
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
#include "mx_pulse_generator.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_pulse_generator_
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_pulse_generator_get_pointers( MX_RECORD *pulse_generator_record,
			MX_PULSE_GENERATOR **pulse_generator,
			MX_PULSE_GENERATOR_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_pulse_generator_get_pointers()";

	if ( pulse_generator_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pulse generator_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( pulse_generator_record->mx_class != MXC_PULSE_GENERATOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' passed by '%s' is not a pulse generator.",
			pulse_generator_record->name, calling_fname );
	}

	if ( pulse_generator != (MX_PULSE_GENERATOR **) NULL ) {
		*pulse_generator = (MX_PULSE_GENERATOR *)
				pulse_generator_record->record_class_struct;

		if ( *pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PULSE_GENERATOR pointer for record '%s' "
			"passed by '%s' is NULL",
				pulse_generator_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_PULSE_GENERATOR_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_PULSE_GENERATOR_FUNCTION_LIST *)
			pulse_generator_record->class_specific_function_list;

		if ( *function_list_ptr ==
			(MX_PULSE_GENERATOR_FUNCTION_LIST *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PULSE_GENERATOR_FUNCTION_LIST pointer for "
			"record '%s' passed by '%s' is NULL.",
				pulse_generator_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_pulse_generator_initialize( MX_RECORD *record )
{
	static const char fname[] = "mx_pulse_generator_initialize()";

	MX_PULSE_GENERATOR *pulse_generator;
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( record,
					&pulse_generator, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulse_generator->stop = 0;

	if ( pulse_generator->start != 0 ) {
		mx_status = mx_pulse_generator_set_pulse_period( record,
						pulse_generator->pulse_period );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_pulse_generator_set_pulse_width( record,
						pulse_generator->pulse_width );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_pulse_generator_set_num_pulses( record,
						pulse_generator->num_pulses );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_pulse_generator_set_pulse_delay( record,
						pulse_generator->pulse_delay );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_pulse_generator_set_mode( record,
						pulse_generator->mode );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_pulse_generator_start( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_pulse_generator_is_busy( MX_RECORD *pulse_generator_record,
				mx_bool_type *busy )
{
	static const char fname[] = "mx_pulse_generator_is_busy()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *busy_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	busy_fn = function_list->busy;

	if ( busy_fn == NULL ) {
		pulse_generator->busy = FALSE;
	} else {
		mx_status = (*busy_fn)( pulse_generator );
	}

	if ( busy != NULL ) {
		*busy = pulse_generator->busy;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_start( MX_RECORD *pulse_generator_record )
{
	static const char fname[] = "mx_pulse_generator_start()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Starting '%s' pulse generator '%s' is not yet implemented.",
			mx_get_driver_name( pulse_generator_record ),
			pulse_generator_record->name );
	}

	mx_status = (*start_fn)( pulse_generator );

	pulse_generator->start = 0;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_stop( MX_RECORD *pulse_generator_record )
{
	static const char fname[] = "mx_pulse_generator_stop()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *stop_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop_fn = function_list->stop;

	if ( stop_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Stopping '%s' pulse generator '%s' is not yet implemented.",
			mx_get_driver_name( pulse_generator_record ),
			pulse_generator_record->name );
	}

	mx_status = (*stop_fn)( pulse_generator );

	pulse_generator->stop = 0;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_get_mode( MX_RECORD *pulse_generator_record, long *mode )
{
	static const char fname[] = "mx_pulse_generator_get_mode()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_pulse_generator_default_get_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_MODE;

	mx_status = (*get_parameter_fn)( pulse_generator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mode != NULL ) {
		*mode = pulse_generator->mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_set_mode( MX_RECORD *pulse_generator_record, long mode )
{
	static const char fname[] = "mx_pulse_generator_set_mode()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_pulse_generator_default_set_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_MODE;

	pulse_generator->mode = mode;

	mx_status = (*set_parameter_fn)( pulse_generator );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_get_pulse_period( MX_RECORD *pulse_generator_record,
					double *pulse_period )
{
	static const char fname[] = "mx_pulse_generator_get_pulse_period()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_pulse_generator_default_get_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_PULSE_PERIOD;

	mx_status = (*get_parameter_fn)( pulse_generator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pulse_period != NULL ) {
		*pulse_period = pulse_generator->pulse_period;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pulse_generator_set_pulse_period( MX_RECORD *pulse_generator_record,
					double pulse_period )
{
	static const char fname[] = "mx_pulse_generator_set_pulse_period()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_pulse_generator_default_set_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_PULSE_PERIOD;

	pulse_generator->pulse_period = pulse_period;

	mx_status = (*set_parameter_fn)( pulse_generator );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_get_pulse_width( MX_RECORD *pulse_generator_record,
					double *pulse_width )
{
	static const char fname[] = "mx_pulse_generator_get_pulse_width()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_pulse_generator_default_get_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_PULSE_WIDTH;

	mx_status = (*get_parameter_fn)( pulse_generator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pulse_width != NULL ) {
		*pulse_width = pulse_generator->pulse_width;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pulse_generator_set_pulse_width( MX_RECORD *pulse_generator_record,
					double pulse_width )
{
	static const char fname[] = "mx_pulse_generator_set_pulse_width()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_pulse_generator_default_set_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_PULSE_WIDTH;

	pulse_generator->pulse_width = pulse_width;

	mx_status = (*set_parameter_fn)( pulse_generator );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_get_num_pulses( MX_RECORD *pulse_generator_record,
					unsigned long *num_pulses )
{
	static const char fname[] = "mx_pulse_generator_get_num_pulses()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_pulse_generator_default_get_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_NUM_PULSES;

	mx_status = (*get_parameter_fn)( pulse_generator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_pulses != NULL ) {
		*num_pulses = pulse_generator->num_pulses;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pulse_generator_set_num_pulses( MX_RECORD *pulse_generator_record,
					unsigned long num_pulses )
{
	static const char fname[] = "mx_pulse_generator_set_num_pulses()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_pulse_generator_default_set_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_NUM_PULSES;

	pulse_generator->num_pulses = num_pulses;

	mx_status = (*set_parameter_fn)( pulse_generator );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_get_pulse_delay( MX_RECORD *pulse_generator_record,
					double *pulse_delay )
{
	static const char fname[] = "mx_pulse_generator_get_pulse_delay()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_pulse_generator_default_get_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_PULSE_DELAY;

	mx_status = (*get_parameter_fn)( pulse_generator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pulse_delay != NULL ) {
		*pulse_delay = pulse_generator->pulse_delay;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pulse_generator_set_pulse_delay( MX_RECORD *pulse_generator_record,
					double pulse_delay )
{
	static const char fname[] = "mx_pulse_generator_set_pulse_delay()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_pulse_generator_default_set_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_PULSE_DELAY;

	pulse_generator->pulse_delay = pulse_delay;

	mx_status = (*set_parameter_fn)( pulse_generator );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_get_last_pulse_number( MX_RECORD *pulse_generator_record,
						long *last_pulse_number )
{
	static const char fname[] =
			"mx_pulse_generator_get_last_pulse_number()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_pulse_generator_default_get_parameter_handler;
	}

	pulse_generator->parameter_type = MXLV_PGN_LAST_PULSE_NUMBER;

	mx_status = (*get_parameter_fn)( pulse_generator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( last_pulse_number != NULL ) {
		*last_pulse_number = pulse_generator->last_pulse_number;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pulse_generator_default_get_parameter_handler(
			MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] =
		"mx_pulse_generator_default_get_parameter_handler()";

	MX_PULSE_GENERATOR_FUNCTION_LIST *flist;
	mx_status_type (*busy_fn)( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_MODE:
	case MXLV_PGN_NUM_PULSES:
	case MXLV_PGN_PULSE_DELAY:
	case MXLV_PGN_PULSE_PERIOD:
	case MXLV_PGN_PULSE_WIDTH:

		/* We just return the value that is already in the 
		 * data structure.
		 */

		break;

	case MXLV_PGN_LAST_PULSE_NUMBER:
		/* The default response is to return 0 if the pulser is busy
		 * or -1 if the pulser is _not_ busy.
		 */

		flist = pulse_generator->record->class_specific_function_list;

		if ( flist == (MX_PULSE_GENERATOR_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PULSE_GENERATOR_FUNCTION_LIST pointer "
			"for record '%s' is NULL.",
				pulse_generator->record->name );
		}

		busy_fn = flist->busy;

		/* If a busy function exists, call it.  In either case,
		 * we read the pulser status from pulser->busy.
		 */

		if ( busy_fn == NULL ) {
			mx_status = (*busy_fn)( pulse_generator );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( pulse_generator->busy ) {
			pulse_generator->last_pulse_number = 0;
		} else {
			pulse_generator->last_pulse_number = -1;
		}
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the "
		"MX driver for pulse generator '%s'.",
			mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
			pulse_generator->parameter_type,
			pulse_generator->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pulse_generator_default_set_parameter_handler(
			MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] =
		"mx_pulse_generator_default_set_parameter_handler()";

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_MODE:
	case MXLV_PGN_NUM_PULSES:
	case MXLV_PGN_PULSE_DELAY:
	case MXLV_PGN_PULSE_PERIOD:
	case MXLV_PGN_PULSE_WIDTH:

		/* We do nothing but leave alone the value that is already
		 * stored in the data structure.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the "
		"MX driver for pulse generator '%s'.",
			mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
			pulse_generator->parameter_type,
			pulse_generator->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}


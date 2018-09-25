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
 * Copyright 2002, 2006, 2015-2016, 2018 Illinois Institute of Technology
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

		mx_status = mx_pulse_generator_set_function_mode( record,
					pulse_generator->function_mode );

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
	mx_status_type ( *status_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type ( *busy_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status_fn = function_list->get_status;
	busy_fn = function_list->busy;

	if ( status_fn != NULL ) {
		mx_status = (*status_fn)( pulse_generator );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( pulse_generator->status & MXSF_PGN_IS_BUSY ) {
			pulse_generator->busy = TRUE;
		} else {
			pulse_generator->busy = FALSE;
		}
	} else
	if ( busy_fn != NULL ) {
		mx_status = (*busy_fn)( pulse_generator );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( pulse_generator->busy ) {
			pulse_generator->status = MXSF_PGN_IS_BUSY;
		} else {
			pulse_generator->status = 0;
		}
	} else {
		pulse_generator->busy = FALSE;
		pulse_generator->status = 0;
	}

	if ( busy != (mx_bool_type *) NULL ) {
		*busy = pulse_generator->busy;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_arm( MX_RECORD *pulse_generator_record )
{
	static const char fname[] = "mx_pulse_generator_arm()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *arm_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	arm_fn = function_list->arm;

	if ( arm_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Arming '%s' pulse generator '%s' is not yet implemented.",
			mx_get_driver_name( pulse_generator_record ),
			pulse_generator_record->name );
	}

	mx_status = (*arm_fn)( pulse_generator );

	pulse_generator->arm = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_trigger( MX_RECORD *pulse_generator_record )
{
	static const char fname[] = "mx_pulse_generator_trigger()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *trigger_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trigger_fn = function_list->trigger;

	if ( trigger_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Triggering '%s' pulse generator '%s' is not yet implemented.",
			mx_get_driver_name( pulse_generator_record ),
			pulse_generator_record->name );
	}

	mx_status = (*trigger_fn)( pulse_generator );

	pulse_generator->trigger = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_start( MX_RECORD *pulse_generator_record )
{
	static const char fname[] = "mx_pulse_generator_start()";

	MX_PULSE_GENERATOR *pulse_generator = NULL;
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_arm( pulse_generator_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_trigger( pulse_generator_record );

	pulse_generator->start = FALSE;

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
mx_pulse_generator_abort( MX_RECORD *pulse_generator_record )
{
	static const char fname[] = "mx_pulse_generator_abort()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *abort_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	abort_fn = function_list->abort;

	if ( abort_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Stopping '%s' pulse generator '%s' is not yet implemented.",
			mx_get_driver_name( pulse_generator_record ),
			pulse_generator_record->name );
	}

	mx_status = (*abort_fn)( pulse_generator );

	pulse_generator->abort = 0;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_setup( MX_RECORD *pulse_generator_record,
			double pulse_period,
			double pulse_width,
			long num_pulses,
			double pulse_delay,
			long function_mode,
			long trigger_mode )
{
	static const char fname[] = "mx_pulse_generator_setup()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *setup_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	setup_fn = function_list->setup;

	if ( setup_fn != NULL ) {

		/* If a 'setup' method exists, use that. */

		pulse_generator->setup[MXSUP_PGN_PULSE_PERIOD]  = pulse_period;
		pulse_generator->setup[MXSUP_PGN_PULSE_WIDTH]   = pulse_width;
		pulse_generator->setup[MXSUP_PGN_NUM_PULSES]    = num_pulses;
		pulse_generator->setup[MXSUP_PGN_PULSE_DELAY]   = pulse_delay;
		pulse_generator->setup[MXSUP_PGN_FUNCTION_MODE] = function_mode;
		pulse_generator->setup[MXSUP_PGN_TRIGGER_MODE]  = trigger_mode;

		mx_status = (*setup_fn)( pulse_generator );

		return mx_status;
	}

	/* If a 'setup' method does _not_ exist, then set the individual
	 * values directly.  If a value passed to this function is less
	 * than zero, then skip setting that value.
	 */

	if ( pulse_period >= 0.0 ) {
		mx_status = mx_pulse_generator_set_pulse_period(
				pulse_generator_record, pulse_period );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( pulse_width >= 0.0 ) {
		mx_status = mx_pulse_generator_set_pulse_width(
				pulse_generator_record, pulse_width );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( num_pulses >= 0 ) {
		mx_status = mx_pulse_generator_set_num_pulses(
				pulse_generator_record, num_pulses );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( pulse_delay >= 0.0 ) {
		mx_status = mx_pulse_generator_set_pulse_delay(
				pulse_generator_record, pulse_delay );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( function_mode >= 0 ) {
		mx_status = mx_pulse_generator_set_function_mode(
				pulse_generator_record, function_mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( trigger_mode >= 0 ) {
		mx_status = mx_pulse_generator_set_trigger_mode(
				pulse_generator_record, trigger_mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_pulse_generator_get_function_mode( MX_RECORD *pulse_generator_record,
					long *function_mode )
{
	static const char fname[] = "mx_pulse_generator_get_function_mode()";

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

	pulse_generator->parameter_type = MXLV_PGN_FUNCTION_MODE;

	mx_status = (*get_parameter_fn)( pulse_generator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( function_mode != NULL ) {
		*function_mode = pulse_generator->function_mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_set_function_mode( MX_RECORD *pulse_generator_record,
					long function_mode )
{
	static const char fname[] = "mx_pulse_generator_set_function_mode()";

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

	pulse_generator->parameter_type = MXLV_PGN_FUNCTION_MODE;

	pulse_generator->function_mode = function_mode;

	mx_status = (*set_parameter_fn)( pulse_generator );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_get_trigger_mode( MX_RECORD *pulse_generator_record,
					long *trigger_mode )
{
	static const char fname[] = "mx_pulse_generator_get_trigger_mode()";

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

	pulse_generator->parameter_type = MXLV_PGN_TRIGGER_MODE;

	mx_status = (*get_parameter_fn)( pulse_generator );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( trigger_mode != NULL ) {
		*trigger_mode = pulse_generator->trigger_mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_pulse_generator_set_trigger_mode( MX_RECORD *pulse_generator_record,
					long trigger_mode )
{
	static const char fname[] = "mx_pulse_generator_set_trigger_mode()";

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

	pulse_generator->parameter_type = MXLV_PGN_TRIGGER_MODE;

	pulse_generator->trigger_mode = trigger_mode;

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
mx_pulse_generator_get_status( MX_RECORD *pulse_generator_record,
					unsigned long *pulser_status )
{
	static const char fname[] = "mx_pulse_generator_get_status()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PULSE_GENERATOR_FUNCTION_LIST *function_list;
	mx_status_type ( *status_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type ( *busy_fn ) ( MX_PULSE_GENERATOR * );
	mx_status_type mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulse_generator_record,
				&pulse_generator, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status_fn = function_list->get_status;
	busy_fn = function_list->busy;

	if ( status_fn != NULL ) {
		mx_status = (*status_fn)( pulse_generator );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( pulse_generator->status & MXSF_PGN_IS_BUSY ) {
			pulse_generator->busy = TRUE;
		} else {
			pulse_generator->busy = FALSE;
		}
	} else
	if ( busy_fn != NULL ) {
		mx_status = (*busy_fn)( pulse_generator );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( pulse_generator->busy ) {
			pulse_generator->status = MXSF_PGN_IS_BUSY;
		} else {
			pulse_generator->status = 0;
		}
	} else {
		pulse_generator->busy = FALSE;
		pulse_generator->status = 0;
	}

	if ( pulser_status != (unsigned long *) NULL ) {
		*pulser_status = pulse_generator->status;
	}

	return mx_status;
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
	case MXLV_PGN_FUNCTION_MODE:
	case MXLV_PGN_NUM_PULSES:
	case MXLV_PGN_PULSE_DELAY:
	case MXLV_PGN_PULSE_PERIOD:
	case MXLV_PGN_PULSE_WIDTH:
	case MXLV_PGN_TRIGGER_MODE:

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
	case MXLV_PGN_FUNCTION_MODE:
	case MXLV_PGN_NUM_PULSES:
	case MXLV_PGN_PULSE_DELAY:
	case MXLV_PGN_PULSE_PERIOD:
	case MXLV_PGN_PULSE_WIDTH:
	case MXLV_PGN_TRIGGER_MODE:

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


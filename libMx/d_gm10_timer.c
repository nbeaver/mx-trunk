/*
 * Name:    d_gm10_timer.c
 *
 * Purpose: MX timer driver for Black Cat Systems GM-10, GM-45, GM-50, and
 *          GM-90 radiation detectors.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_GM10_TIMER_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "d_gm10_scaler.h"
#include "d_gm10_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_gm10_timer_record_function_list = {
	mxd_gm10_timer_initialize_type,
	mxd_gm10_timer_create_record_structures,
	mxd_gm10_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_gm10_timer_timer_function_list = {
	mxd_gm10_timer_is_busy,
	mxd_gm10_timer_start,
	mxd_gm10_timer_stop
};

/* MX gm10 timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_gm10_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_GM10_TIMER_STANDARD_FIELDS
};

long mxd_gm10_timer_num_record_fields
		= sizeof( mxd_gm10_timer_record_field_defaults )
		  / sizeof( mxd_gm10_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_gm10_timer_rfield_def_ptr
			= &mxd_gm10_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_gm10_timer_get_pointers( MX_TIMER *timer,
			MX_GM10_TIMER **gm10_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_gm10_timer_get_pointers()";

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( gm10_timer == (MX_GM10_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GM10_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*gm10_timer = (MX_GM10_TIMER *) timer->record->record_type_struct;

	if ( (*gm10_timer) == (MX_GM10_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_GM10_TIMER pointer for timer record '%s' passed by '%s' is NULL",
			timer->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_gm10_timer_initialize_type( long type )
{
        static const char fname[] = "mxs_gm10_timer_initialize_type()";

        MX_DRIVER *driver;
        MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
        MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
        MX_RECORD_FIELD_DEFAULTS *field;
        long num_record_fields;
	long referenced_field_index;
        long num_scalers_varargs_cookie;
        mx_status_type mx_status;

        driver = mx_get_driver_by_type( type );

        if ( driver == (MX_DRIVER *) NULL ) {
                return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
                        "Record type %ld not found.", type );
        }

        record_field_defaults_ptr = driver->record_field_defaults_ptr;

        if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'record_field_defaults_ptr' for record type '%s' is NULL.",
                        driver->name );
        }

        record_field_defaults = *record_field_defaults_ptr;

        if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'record_field_defaults_ptr' for record type '%s' is NULL.",
                        driver->name );
        }

        if ( driver->num_record_fields == (long *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'num_record_fields' pointer for record type '%s' is NULL.",
                        driver->name );
        }

	num_record_fields = *(driver->num_record_fields);

        mx_status = mx_find_record_field_defaults_index(
                        record_field_defaults, num_record_fields,
                        "num_scalers", &referenced_field_index );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        mx_status = mx_construct_varargs_cookie(
                        referenced_field_index, 0, &num_scalers_varargs_cookie);

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

	mx_status = mx_find_record_field_defaults(
		record_field_defaults, num_record_fields,
		"scaler_record_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_scalers_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gm10_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_gm10_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_GM10_TIMER *gm10_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	gm10_timer = (MX_GM10_TIMER *)
				malloc( sizeof(MX_GM10_TIMER) );

	if ( gm10_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GM10_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = gm10_timer;
	record->class_specific_function_list
			= &mxd_gm10_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gm10_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_gm10_timer_finish_record_initialization()";

	MX_TIMER *timer;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	timer->mode = MXCM_UNKNOWN_MODE;

	mx_status = mx_timer_finish_record_initialization( record );

	return mx_status;
}

#define MXD_GM10_TIMER_BLOCK_SIZE	1000

static mx_status_type
mxd_gm10_timer_get_new_counts( MX_RECORD *timer_record,
				MX_RECORD *scaler_record )
{
	static const char fname[] = "mxd_gm10_timer_get_new_counts()";

	MX_SCALER *scaler;
	MX_GM10_SCALER *gm10_scaler;
	char read_buffer[MXD_GM10_TIMER_BLOCK_SIZE];
	unsigned long num_bytes_available;
	unsigned long i, num_blocks, remainder;
	mx_status_type mx_status;

	if ( timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The timer record pointer passed was NULL." );
	}

	if ( scaler_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The scaler record pointer passed was NULL." );
	}

	if ( scaler_record->mx_type != MXT_SCL_GM10 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Scaler record '%s' used by 'gm10_timer' record '%s' "
		"is not a record of type 'gm10_scaler'.",
			scaler_record->name, timer_record->name );
	}

	scaler = (MX_SCALER *) scaler_record->record_class_struct;

	if ( scaler == (MX_SCALER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SCALER pointer for record '%s' is NULL.",
				scaler_record->name );
	}

	gm10_scaler = (MX_GM10_SCALER *) scaler_record->record_type_struct;

	if ( gm10_scaler == (MX_GM10_SCALER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_GM10_SCALER pointer for record '%s' is NULL.",
				scaler_record->name );
	}

	/* How many new counts have been seen for this detector? */

	mx_status = mx_rs232_num_input_bytes_available(
				gm10_scaler->rs232_record,
				&num_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler->value += num_bytes_available;

#if MXD_GM10_TIMER_DEBUG
	MX_DEBUG(-2,("%s: scaler '%s', new counts = %lu",
		fname, scaler_record->name, num_bytes_available));
#endif

	/* Discard the bytes available on the serial port. */

	num_blocks = num_bytes_available / MXD_GM10_TIMER_BLOCK_SIZE;

	remainder = num_bytes_available % MXD_GM10_TIMER_BLOCK_SIZE;

	for ( i = 0; i < num_blocks; i++ ) {
		mx_status = mx_rs232_read( gm10_scaler->rs232_record,
						read_buffer,
						MXD_GM10_TIMER_BLOCK_SIZE,
						NULL, MXD_GM10_TIMER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( remainder > 0 ) {
		mx_status = mx_rs232_read( gm10_scaler->rs232_record,
						read_buffer,
						remainder,
						NULL, MXD_GM10_TIMER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gm10_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_gm10_timer_is_busy()";

	MX_GM10_TIMER *gm10_timer;
	MX_CLOCK_TICK current_time_in_clock_ticks;
	int i, result;
	MX_RECORD *scaler_record;
	mx_status_type mx_status;

	gm10_timer = NULL;

	mx_status = mxd_gm10_timer_get_pointers( timer, &gm10_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get new counts for each of the scalers. */

	for ( i = 0; i < gm10_timer->num_scalers; i++ ) {
		scaler_record = gm10_timer->scaler_record_array[i];

		mx_status = mxd_gm10_timer_get_new_counts( timer->record,
							scaler_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Should we stop counting now? */

	current_time_in_clock_ticks = mx_current_clock_tick();

	result = mx_compare_clock_ticks( current_time_in_clock_ticks,
				gm10_timer->finish_time_in_clock_ticks );

	if ( result >= 0 ) {
		timer->busy = FALSE;
	} else {
		timer->busy = TRUE;
	}

#if MXD_GM10_TIMER_DEBUG

	MX_DEBUG(-2,("%s: current time = (%lu,%lu), busy = %d",
		fname, current_time_in_clock_ticks.high_order,
		current_time_in_clock_ticks.low_order,
		(int) timer->busy ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gm10_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_gm10_timer_start()";

	MX_GM10_TIMER *gm10_timer;
	MX_CLOCK_TICK start_time_in_clock_ticks;
	MX_CLOCK_TICK measurement_time_in_clock_ticks;
	mx_status_type mx_status;

	gm10_timer = NULL;

	mx_status = mxd_gm10_timer_get_pointers( timer, &gm10_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_time_in_clock_ticks = mx_current_clock_tick();

	measurement_time_in_clock_ticks
		= mx_convert_seconds_to_clock_ticks( timer->value );

	gm10_timer->finish_time_in_clock_ticks = mx_add_clock_ticks(
				start_time_in_clock_ticks,
				measurement_time_in_clock_ticks );

#if MXD_GM10_TIMER_DEBUG

	MX_DEBUG(-2,("%s: counting for %g seconds, (%lu,%ld) in clock ticks.",
		fname, timer->value,
		measurement_time_in_clock_ticks.high_order,
		(long) measurement_time_in_clock_ticks.low_order));

	MX_DEBUG(-2,("%s: starting time = (%lu,%ld), finish time = (%lu,%ld)",
		fname, start_time_in_clock_ticks.high_order,
		(long) start_time_in_clock_ticks.low_order,
		gm10_timer->finish_time_in_clock_ticks.high_order,
		(long) gm10_timer->finish_time_in_clock_ticks.low_order));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gm10_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_gm10_timer_stop()";

	MX_GM10_TIMER *gm10_timer;
	mx_status_type mx_status;

	gm10_timer = NULL;

	mx_status = mxd_gm10_timer_get_pointers( timer, &gm10_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	gm10_timer->finish_time_in_clock_ticks = mx_current_clock_tick();

	return MX_SUCCESSFUL_RESULT;
}


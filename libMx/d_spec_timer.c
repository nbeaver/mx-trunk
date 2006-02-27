/*
 * Name:    d_spec_timer.c
 *
 * Purpose: MX timer driver to control Spec timers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SPEC_TIMER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_spec.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "d_spec_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_spec_timer_record_function_list = {
	NULL,
	mxd_spec_timer_create_record_structures,
	mxd_spec_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_spec_timer_timer_function_list = {
	mxd_spec_timer_is_busy,
	mxd_spec_timer_start,
	mxd_spec_timer_stop
};

/* MX spec timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_spec_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_SPEC_TIMER_STANDARD_FIELDS
};

long mxd_spec_timer_num_record_fields
		= sizeof( mxd_spec_timer_record_field_defaults )
		  / sizeof( mxd_spec_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_spec_timer_rfield_def_ptr
			= &mxd_spec_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_spec_timer_get_pointers( MX_TIMER *timer,
			MX_SPEC_TIMER **spec_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_spec_timer_get_pointers()";

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The timer pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( timer->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for timer pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( timer->record->mx_type != MXT_TIM_SPEC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The timer '%s' passed by '%s' is not a spec timer.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			timer->record->name, calling_fname,
			timer->record->mx_superclass,
			timer->record->mx_class,
			timer->record->mx_type );
	}

	if ( spec_timer != (MX_SPEC_TIMER **) NULL ) {

		*spec_timer = (MX_SPEC_TIMER *)
				(timer->record->record_type_struct);

		if ( *spec_timer == (MX_SPEC_TIMER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SPEC_TIMER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
		}
	}

	if ( (*spec_timer)->spec_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The spec_server_record pointer for MX spec timer '%s' is NULL.",
			timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_spec_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_spec_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_SPEC_TIMER *spec_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	spec_timer = (MX_SPEC_TIMER *)
				malloc( sizeof(MX_SPEC_TIMER) );

	if ( spec_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SPEC_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = spec_timer;
	record->class_specific_function_list
			= &mxd_spec_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spec_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_spec_timer_finish_record_initialization()";

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

MX_EXPORT mx_status_type
mxd_spec_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_spec_timer_is_busy()";

	MX_SPEC_TIMER *spec_timer;
	long busy;
	mx_status_type mx_status;

	mx_status = mxd_spec_timer_get_pointers( timer, &spec_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_spec_get_number( spec_timer->spec_server_record,
					"scaler/.all./count",
					MXFT_LONG, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

#if MXD_SPEC_TIMER_DEBUG
	MX_DEBUG(-2,("%s: 'scaler/.all./count' = %d, timer->busy = %d",
		fname, busy, timer->busy ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spec_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_spec_timer_start()";

	MX_SPEC_TIMER *spec_timer;
	double seconds_to_count;
	mx_status_type mx_status;

	mx_status = mxd_spec_timer_get_pointers( timer, &spec_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds_to_count = timer->value;

#if MXD_SPEC_TIMER_DEBUG
	MX_DEBUG(-2,("%s: 'scaler/.all./count' = %g",
			fname, seconds_to_count ));
#endif

	mx_status = mx_spec_put_number( spec_timer->spec_server_record,
					"scaler/.all./count",
					MXFT_DOUBLE, &seconds_to_count );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_spec_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_spec_timer_stop()";

	MX_SPEC_TIMER *spec_timer;
	long stop;
	mx_status_type mx_status;

	mx_status = mxd_spec_timer_get_pointers( timer, &spec_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->value = 0;

	stop = 0;

#if MXD_SPEC_TIMER_DEBUG
	MX_DEBUG(-2,("%s: 'scaler/.all./count' = %d", fname, stop));
#endif

	mx_status = mx_spec_put_number( spec_timer->spec_server_record,
					"scaler/.all./count",
					MXFT_LONG, &stop );
	return mx_status;
}


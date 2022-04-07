/*
 * Name:    d_k8055_timer.c
 *
 * Purpose: MX timer driver used with a Velleman K8055 module.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_time.h"
#include "mx_hrt.h"
#include "mx_timer.h"
#include "i_k8055.h"
#include "d_k8055_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_k8055_timer_record_function_list = {
	NULL,
	mxd_k8055_timer_create_record_structures,
	mx_timer_finish_record_initialization,
	NULL,
	NULL,
	mxd_k8055_timer_open,
};

MX_TIMER_FUNCTION_LIST mxd_k8055_timer_timer_function_list = {
	mxd_k8055_timer_is_busy,
	mxd_k8055_timer_start,
	mxd_k8055_timer_stop
};

/* K8055 timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_k8055_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_K8055_TIMER_STANDARD_FIELDS
};

long mxd_k8055_timer_num_record_fields
		= sizeof( mxd_k8055_timer_record_field_defaults )
		  / sizeof( mxd_k8055_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_k8055_timer_rfield_def_ptr
			= &mxd_k8055_timer_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_k8055_timer_get_pointers( MX_TIMER *timer,
			MX_K8055_TIMER **k8055_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_k8055_timer_get_pointers()";

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( k8055_timer == (MX_K8055_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_K8055_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( timer->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_TIMER pointer "
		"passed by '%s' was NULL.", calling_fname );
	}

	*k8055_timer = (MX_K8055_TIMER *) timer->record->record_type_struct;

	if ( *k8055_timer == (MX_K8055_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_K8055_TIMER pointer for record '%s' is NULL.",
			timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*======*/

MX_EXPORT mx_status_type
mxd_k8055_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_k8055_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_K8055_TIMER *k8055_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	k8055_timer = (MX_K8055_TIMER *) malloc( sizeof(MX_K8055_TIMER) );

	if ( k8055_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_K8055_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = k8055_timer;
	record->class_specific_function_list
			= &mxd_k8055_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_k8055_timer_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_k8055_timer_open()";

	MX_TIMER *timer;
	MX_K8055_TIMER *k8055_timer = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	timer = (MX_TIMER *) (record->record_class_struct);

	mx_status = mxd_k8055_timer_get_pointers( timer, &k8055_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->mode = MXCM_COUNTER_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_k8055_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_k8055_timer_is_busy()";

	MX_K8055_TIMER *k8055_timer = NULL;
	struct timespec current_timespec;
	int comparison;
	mx_status_type mx_status;

	mx_status = mxd_k8055_timer_get_pointers( timer, &k8055_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	current_timespec = mx_high_resolution_time();

	comparison = mx_compare_timespec_times( current_timespec,
						k8055_timer->finish_timespec );

	if ( comparison < 0 ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_k8055_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_k8055_timer_start()";

	MX_K8055_TIMER *k8055_timer = NULL;
	double seconds;
	struct timespec counting_timespec, current_timespec;
	mx_status_type mx_status;

	mx_status = mxd_k8055_timer_get_pointers( timer, &k8055_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds = timer->value;

	counting_timespec = mx_convert_seconds_to_timespec_time( seconds );

	current_timespec = mx_high_resolution_time();

	k8055_timer->finish_timespec = mx_add_timespec_times( current_timespec,
							counting_timespec );

	ResetCounter(1);
	ResetCounter(2);

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_k8055_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_k8055_timer_stop()";

	MX_K8055_TIMER *k8055_timer = NULL;
	mx_status_type mx_status;

	mx_status = mxd_k8055_timer_get_pointers( timer, &k8055_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the timer from counting. */

	k8055_timer->finish_timespec = mx_high_resolution_time();

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}


/*
 * Name:    d_network_timer.c
 *
 * Purpose: MX timer driver to control MX network timers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_unistd.h"

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "d_network_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_network_timer_record_function_list = {
	NULL,
	mxd_network_timer_create_record_structures,
	mxd_network_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_network_timer_timer_function_list = {
	mxd_network_timer_is_busy,
	mxd_network_timer_start,
	mxd_network_timer_stop,
	mxd_network_timer_clear,
	mxd_network_timer_read,
	mxd_network_timer_get_mode,
	mxd_network_timer_set_mode,
	NULL,
	mxd_network_timer_get_last_measurement_time
};

/* MX network timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_network_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_NETWORK_TIMER_STANDARD_FIELDS
};

long mxd_network_timer_num_record_fields
		= sizeof( mxd_network_timer_record_field_defaults )
		  / sizeof( mxd_network_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_timer_rfield_def_ptr
			= &mxd_network_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_network_timer_get_pointers( MX_TIMER *timer,
			MX_NETWORK_TIMER **network_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_timer_get_pointers()";

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( network_timer == (MX_NETWORK_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_NETWORK_TIMER pointer passed by '%s' was NULL",
			calling_fname );
	}

	*network_timer = (MX_NETWORK_TIMER *) timer->record->record_type_struct;

	if ( *network_timer == (MX_NETWORK_TIMER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_TIMER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/


MX_EXPORT mx_status_type
mxd_network_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_network_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_NETWORK_TIMER *network_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	network_timer = (MX_NETWORK_TIMER *) malloc( sizeof(MX_NETWORK_TIMER) );

	if ( network_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = network_timer;
	record->class_specific_function_list
			= &mxd_network_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_timer_finish_record_initialization()";

	MX_TIMER *timer;
	MX_NETWORK_TIMER *network_timer;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	mx_status = mxd_network_timer_get_pointers(
				timer, &network_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->mode = MXCM_UNKNOWN_MODE;

	mx_status = mx_timer_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_timer->busy_nf),
		network_timer->server_record,
		"%s.busy", network_timer->remote_record_name );

	mx_network_field_init( &(network_timer->clear_nf),
		network_timer->server_record,
		"%s.clear", network_timer->remote_record_name );

	mx_network_field_init( &(network_timer->last_measurement_time_nf),
		network_timer->server_record,
		"%s.last_measurement_time", network_timer->remote_record_name );

	mx_network_field_init( &(network_timer->mode_nf),
		network_timer->server_record,
		"%s.mode", network_timer->remote_record_name );

	mx_network_field_init( &(network_timer->stop_nf),
		network_timer->server_record,
		"%s.stop", network_timer->remote_record_name );

	mx_network_field_init( &(network_timer->value_nf),
		network_timer->server_record,
		"%s.value", network_timer->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_network_timer_is_busy()";

	MX_NETWORK_TIMER *network_timer;
	int busy;
	mx_status_type mx_status;

	mx_status = mxd_network_timer_get_pointers(
				timer, &network_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_timer->busy_nf), MXFT_INT, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->busy = busy;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_network_timer_start()";

	MX_NETWORK_TIMER *network_timer;
	double seconds_to_count;
	mx_status_type mx_status;

	mx_status = mxd_network_timer_get_pointers(
				timer, &network_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds_to_count = timer->value;

	mx_status = mx_put( &(network_timer->value_nf),
				MXFT_DOUBLE, &seconds_to_count );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_network_timer_stop()";

	MX_NETWORK_TIMER *network_timer;
	int stop;
	double value;
	mx_status_type mx_status;

	mx_status = mxd_network_timer_get_pointers(
				timer, &network_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop = 1;

	mx_status = mx_put( &(network_timer->stop_nf), MXFT_INT, &stop );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_timer->value_nf), MXFT_DOUBLE, &value );

	timer->value = value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_timer_clear( MX_TIMER *timer )
{
	static const char fname[] = "mxd_network_timer_clear()";

	MX_NETWORK_TIMER *network_timer;
	int clear;
	mx_status_type mx_status;

	mx_status = mxd_network_timer_get_pointers(
				timer, &network_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	clear = 1;

	mx_status = mx_put( &(network_timer->clear_nf), MXFT_INT, &clear );

	timer->value = 0.0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_network_timer_read()";

	MX_NETWORK_TIMER *network_timer;
	double value;
	mx_status_type mx_status;

	mx_status = mxd_network_timer_get_pointers(
				timer, &network_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_timer->value_nf), MXFT_DOUBLE, &value );

	timer->value = value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_timer_get_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_network_timer_get_mode()";

	MX_NETWORK_TIMER *network_timer;
	int mode;
	mx_status_type mx_status;

	mx_status = mxd_network_timer_get_pointers(
				timer, &network_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_timer->mode_nf), MXFT_INT, &mode );

	timer->mode = mode;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_network_timer_set_mode()";

	MX_NETWORK_TIMER *network_timer;
	int mode;
	mx_status_type mx_status;

	mx_status = mxd_network_timer_get_pointers(
				timer, &network_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mode = timer->mode;

	mx_status = mx_put( &(network_timer->mode_nf), MXFT_INT, &mode );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_timer_get_last_measurement_time( MX_TIMER *timer )
{
	static const char fname[] =
			"mxd_network_timer_get_last_measurement_time()";

	MX_NETWORK_TIMER *network_timer;
	double last_measurement_time;
	mx_status_type mx_status;

	mx_status = mxd_network_timer_get_pointers(
				timer, &network_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_timer->last_measurement_time_nf),
				MXFT_DOUBLE, &last_measurement_time );

	timer->last_measurement_time = last_measurement_time;

	MX_DEBUG(-2,("%s: Timer '%s' last_measurement_time = %g",
		fname, timer->record->name, timer->last_measurement_time));

	return mx_status;
}


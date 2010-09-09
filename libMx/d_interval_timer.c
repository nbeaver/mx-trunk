/*
 * Name:    d_interval_timer.c
 *
 * Purpose: MX timer driver for using an MX interval timer as a timer device.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2007, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_INTERVAL_TIMER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_interval_timer.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "d_interval_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_interval_timer_record_function_list = {
	NULL,
	mxd_interval_timer_create_record_structures,
	mxd_interval_timer_finish_record_initialization,
	NULL,
	NULL,
	mxd_interval_timer_open,
	mxd_interval_timer_close
};

MX_TIMER_FUNCTION_LIST mxd_interval_timer_timer_function_list = {
	mxd_interval_timer_is_busy,
	mxd_interval_timer_start,
	mxd_interval_timer_stop,
	NULL,
	mxd_interval_timer_read
};

/* MX interval timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_interval_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_INTERVAL_TIMER_STANDARD_FIELDS
};

long mxd_interval_timer_num_record_fields
		= sizeof( mxd_interval_timer_record_field_defaults )
		  / sizeof( mxd_interval_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_interval_timer_rfield_def_ptr
			= &mxd_interval_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_interval_timer_get_pointers( MX_TIMER *timer,
			MX_INTERVAL_TIMER_DEVICE **itimer_device,
			const char *calling_fname )
{
	static const char fname[] = "mxd_interval_timer_get_pointers()";

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( itimer_device == (MX_INTERVAL_TIMER_DEVICE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER_DEVICE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*itimer_device = (MX_INTERVAL_TIMER_DEVICE *)
				timer->record->record_type_struct;

	if ( (*itimer_device) == (MX_INTERVAL_TIMER_DEVICE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERVAL_TIMER_DEVICE pointer for timer record '%s' "
		"passed by '%s' is NULL",
			timer->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_interval_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_interval_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_INTERVAL_TIMER_DEVICE *itimer_device;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	itimer_device = (MX_INTERVAL_TIMER_DEVICE *)
				malloc( sizeof(MX_INTERVAL_TIMER_DEVICE) );

	if ( itimer_device == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_INTERVAL_TIMER_DEVICE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = itimer_device;
	record->class_specific_function_list
			= &mxd_interval_timer_timer_function_list;

	timer->record = record;

	itimer_device->itimer = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_interval_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_interval_timer_finish_record_initialization()";

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
mxd_interval_timer_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_interval_timer_open()";

	MX_TIMER *timer;
	MX_INTERVAL_TIMER_DEVICE *itimer_device;
	MX_INTERVAL_TIMER *itimer;
	mx_status_type mx_status;

	itimer_device = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	mx_status = mxd_interval_timer_get_pointers( timer,
						&itimer_device, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_interval_timer_create( &itimer,
					MXIT_ONE_SHOT_TIMER, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	itimer_device->itimer = itimer;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_interval_timer_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_interval_timer_close()";

	MX_TIMER *timer;
	MX_INTERVAL_TIMER_DEVICE *itimer_device;
	mx_status_type mx_status;

	itimer_device = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	mx_status = mxd_interval_timer_get_pointers( timer,
						&itimer_device, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( itimer_device->itimer != (MX_INTERVAL_TIMER *) NULL ) {
		mx_status = mx_interval_timer_destroy( itimer_device->itimer );
	}

	itimer_device->itimer = NULL;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_interval_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_interval_timer_is_busy()";

	MX_INTERVAL_TIMER_DEVICE *itimer_device;
	mx_bool_type busy;
	mx_status_type mx_status;

	itimer_device = NULL;

	mx_status = mxd_interval_timer_get_pointers( timer,
						&itimer_device, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_interval_timer_is_busy( itimer_device->itimer, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_interval_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_interval_timer_start()";

	MX_INTERVAL_TIMER_DEVICE *itimer_device;
	mx_status_type mx_status;

	itimer_device = NULL;

	mx_status = mxd_interval_timer_get_pointers( timer,
						&itimer_device, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_interval_timer_start( itimer_device->itimer,
							timer->value );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_interval_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_interval_timer_stop()";

	MX_INTERVAL_TIMER_DEVICE *itimer_device;
	mx_status_type mx_status;

	itimer_device = NULL;

	mx_status = mxd_interval_timer_get_pointers( timer,
						&itimer_device, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_interval_timer_stop( itimer_device->itimer,
							&(timer->value) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_interval_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_interval_timer_read()";

	MX_INTERVAL_TIMER_DEVICE *itimer_device;
	mx_status_type mx_status;

	itimer_device = NULL;

	mx_status = mxd_interval_timer_get_pointers( timer,
						&itimer_device, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_interval_timer_read( itimer_device->itimer,
							&(timer->value) );

	return mx_status;
}


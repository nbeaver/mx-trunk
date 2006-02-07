/*
 * Name:    d_mcs_timer.c
 *
 * Purpose: MX timer driver to use an MCS as a timer.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2006 Illinois Institute of Technology
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
#include "mx_timer.h"
#include "mx_mcs.h"
#include "d_mcs_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mcs_timer_record_function_list = {
	NULL,
	mxd_mcs_timer_create_record_structures,
	mxd_mcs_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_mcs_timer_timer_function_list = {
	mxd_mcs_timer_is_busy,
	mxd_mcs_timer_start,
	mxd_mcs_timer_stop,
	mxd_mcs_timer_clear,
	mxd_mcs_timer_read,
	mxd_mcs_timer_get_mode,
	mxd_mcs_timer_set_mode,
	mxd_mcs_timer_set_modes_of_associated_counters,
	mxd_mcs_timer_get_last_measurement_time
};

/* MCS timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mcs_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_MCS_TIMER_STANDARD_FIELDS
};

mx_length_type mxd_mcs_timer_num_record_fields
		= sizeof( mxd_mcs_timer_record_field_defaults )
		  / sizeof( mxd_mcs_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mcs_timer_rfield_def_ptr
			= &mxd_mcs_timer_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_mcs_timer_get_pointers( MX_TIMER *timer,
			MX_MCS_TIMER **mcs_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mcs_timer_get_pointers()";

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs_timer == (MX_MCS_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MCS_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mcs_timer = (MX_MCS_TIMER *) (timer->record->record_type_struct);

	if ( *mcs_timer == (MX_MCS_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS_TIMER pointer for record '%s' is NULL.",
			timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_mcs_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_mcs_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_MCS_TIMER *mcs_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	mcs_timer = (MX_MCS_TIMER *) malloc( sizeof(MX_MCS_TIMER) );

	if ( mcs_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = mcs_timer;
	record->class_specific_function_list
			= &mxd_mcs_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_mcs_timer_finish_record_initialization()";

	MX_RECORD *mcs_record;
	MX_TIMER *timer;
	MX_MCS *mcs;
	MX_MCS_TIMER *mcs_timer;
	mx_status_type mx_status;

	mx_status = mx_timer_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer = (MX_TIMER *)( record->record_class_struct );

	mx_status = mxd_mcs_timer_get_pointers( timer, &mcs_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->mode = MXCM_UNKNOWN_MODE;

	mcs_record = mcs_timer->mcs_record;

	if ( mcs_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mcs_record pointer for MCS timer '%s' is NULL.",
			record->name );
	}

	mcs = (MX_MCS *) mcs_record->record_class_struct;

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS pointer for MCS record '%s' is NULL.",
			mcs_record->name );
	}

	/* If a timer record has not already been specified for the MCS,
	 * then list the current record as the timer record.
	 */

	if ( mcs->timer_record == (MX_RECORD *) NULL ) {
		mcs->timer_record = record;

		strlcpy( mcs->timer_name, record->name,
				MXU_RECORD_NAME_LENGTH );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mcs_timer_is_busy()";

	MX_MCS_TIMER *mcs_timer;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_mcs_timer_get_pointers( timer, &mcs_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for timer '%s'", fname, timer->record->name));

	mx_status = mx_mcs_is_busy( mcs_timer->mcs_record, &busy );

	timer->busy = busy;

	MX_DEBUG( 2,("%s: timer '%s' busy = %d",
			fname, timer->record->name, timer->busy));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mcs_timer_start()";

	MX_MCS_TIMER *mcs_timer;
	double seconds;
	mx_status_type mx_status;

	mx_status = mxd_mcs_timer_get_pointers( timer, &mcs_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds = timer->value;

	MX_DEBUG( 2,("%s invoked for timer '%s' for %g seconds.",
			fname, timer->record->name, seconds));

	mx_status = mx_mcs_set_measurement_time( mcs_timer->mcs_record,
							seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: measurement time has been set.", fname));

	mx_status = mx_mcs_set_num_measurements( mcs_timer->mcs_record, 1L );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: num measurements has been set.", fname));

	mx_status = mx_mcs_start( mcs_timer->mcs_record );

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mcs_timer_stop()";

	MX_MCS_TIMER *mcs_timer;
	mx_status_type mx_status;

	mx_status = mxd_mcs_timer_get_pointers( timer, &mcs_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for timer '%s'", fname, timer->record->name));

	mx_status = mx_mcs_stop( mcs_timer->mcs_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_set_measurement_time( mcs_timer->mcs_record, -1.0 );

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_timer_clear( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mcs_timer_clear()";

	MX_MCS_TIMER *mcs_timer;
	mx_status_type mx_status;

	mx_status = mxd_mcs_timer_get_pointers( timer, &mcs_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_clear( mcs_timer->mcs_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_set_measurement_time( mcs_timer->mcs_record, -1.0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mcs_timer_read()";

	MX_MCS_TIMER *mcs_timer;
	MX_MCS *mcs;
	mx_status_type mx_status;

	mx_status = mxd_mcs_timer_get_pointers( timer, &mcs_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for timer '%s'",
			fname, timer->record->name));

	mcs = (MX_MCS *) mcs_timer->mcs_record->record_class_struct;

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MCS pointer for MCS record '%s' is NULL.",
			mcs_timer->mcs_record->name );
	}
	if ( mcs->timer_data == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"timer_data pointer for MCS record '%s' is NULL.",
			mcs_timer->mcs_record->name );
	}

	mx_status = mx_mcs_set_num_measurements( mcs_timer->mcs_record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_read_timer( mcs_timer->mcs_record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->value = mcs->timer_data[0];

	MX_DEBUG( 2,("%s complete.  value = %g", fname, timer->value));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_timer_get_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mcs_timer_get_mode()";

	MX_MCS_TIMER *mcs_timer;
	int mode;
	mx_status_type mx_status;

	mx_status = mxd_mcs_timer_get_pointers( timer, &mcs_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_get_mode( mcs_timer->mcs_record, &mode );

	timer->mode = mode;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mcs_timer_set_mode()";

	MX_MCS_TIMER *mcs_timer;
	mx_status_type mx_status;

	mx_status = mxd_mcs_timer_get_pointers( timer, &mcs_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_set_mode( mcs_timer->mcs_record, timer->mode );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_timer_set_modes_of_associated_counters( MX_TIMER *timer )
{
	/* This function is unnecessary if all the counters for a measurement
	 * belong to the same multichannel scaler (which is true at present).
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_timer_get_last_measurement_time( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mcs_timer_get_last_measurement_time()";

	MX_MCS_TIMER *mcs_timer;
	double last_measurement_time;
	mx_status_type mx_status;

	mx_status = mxd_mcs_timer_get_pointers( timer, &mcs_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_get_measurement_time( mcs_timer->mcs_record,
						    &last_measurement_time );

	timer->last_measurement_time = last_measurement_time;

	return mx_status;
}


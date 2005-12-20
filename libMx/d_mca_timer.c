/*
 * Name:    d_mca_timer.c
 *
 * Purpose: MX timer driver to control MX mca timers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2005 Illinois Institute of Technology
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
#include "mx_measurement.h"
#include "mx_timer.h"
#include "mx_mca.h"
#include "d_mca_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mca_timer_record_function_list = {
	mxd_mca_timer_initialize_type,
	mxd_mca_timer_create_record_structures,
	mxd_mca_timer_finish_record_initialization,
	mxd_mca_timer_delete_record,
	NULL,
	mxd_mca_timer_read_parms_from_hardware,
	mxd_mca_timer_write_parms_to_hardware,
	mxd_mca_timer_open,
	mxd_mca_timer_close
};

MX_TIMER_FUNCTION_LIST mxd_mca_timer_timer_function_list = {
	mxd_mca_timer_is_busy,
	mxd_mca_timer_start,
	mxd_mca_timer_stop,
	mxd_mca_timer_clear,
	mxd_mca_timer_read,
	mxd_mca_timer_get_mode,
	mxd_mca_timer_set_mode,
	NULL,
	mxd_mca_timer_get_last_measurement_time
};

/* MX mca timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mca_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_MCA_TIMER_STANDARD_FIELDS
};

long mxd_mca_timer_num_record_fields
		= sizeof( mxd_mca_timer_record_field_defaults )
		  / sizeof( mxd_mca_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mca_timer_rfield_def_ptr
			= &mxd_mca_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_mca_timer_get_pointers( MX_TIMER *timer,
			MX_MCA_TIMER **mca_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mca_timer_get_pointers()";

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

	if ( timer->record->mx_type != MXT_TIM_MCA ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The timer '%s' passed by '%s' is not a mca timer.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			timer->record->name, calling_fname,
			timer->record->mx_superclass,
			timer->record->mx_class,
			timer->record->mx_type );
	}

	if ( mca_timer != (MX_MCA_TIMER **) NULL ) {

		*mca_timer = (MX_MCA_TIMER *)
				(timer->record->record_type_struct);

		if ( *mca_timer == (MX_MCA_TIMER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_MCA_TIMER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
		}
	}

	if ( (*mca_timer)->mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX MCA record pointer for MX mca timer '%s' is NULL.",
			timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_mca_timer_initialize_type( long type )
{
	/* Nothing needed here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_mca_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_MCA_TIMER *mca_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	mca_timer = (MX_MCA_TIMER *)
				malloc( sizeof(MX_MCA_TIMER) );

	if ( mca_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = mca_timer;
	record->class_specific_function_list
			= &mxd_mca_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_mca_timer_finish_record_initialization()";

	MX_TIMER *timer;
	MX_MCA_TIMER *mca_timer;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	timer->mode = MXCM_PRESET_MODE;

	mca_timer = (MX_MCA_TIMER *) record->record_type_struct;

	if ( mca_timer == (MX_MCA_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MCA_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	mca_timer->preset_time = 0.0;

	return mx_timer_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_mca_timer_delete_record( MX_RECORD *record )
{
	MX_MCA_TIMER *mca_timer;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mca_timer = (MX_MCA_TIMER *) record->record_type_struct;

	if ( mca_timer != NULL ) {

		free( mca_timer );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_timer_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_timer_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_timer_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_timer_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mca_timer_is_busy()";

	MX_MCA_TIMER *mca_timer;
	int busy;
	mx_status_type mx_status;

	mx_status = mxd_mca_timer_get_pointers( timer, &mca_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mca_is_busy( mca_timer->mca_record, &busy );

	timer->busy = busy;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mca_timer_start()";

	MX_MCA_TIMER *mca_timer;
	double seconds_to_count;
	mx_status_type mx_status;

	mx_status = mxd_mca_timer_get_pointers( timer, &mca_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds_to_count = timer->value;

	mca_timer->preset_time = seconds_to_count;

	if ( mca_timer->use_real_time ) {
		mx_status = mx_mca_start_for_preset_real_time(
			mca_timer->mca_record, seconds_to_count );
	} else {
		mx_status = mx_mca_start_for_preset_live_time(
			mca_timer->mca_record, seconds_to_count );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mca_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mca_timer_stop()";

	MX_MCA_TIMER *mca_timer;
	mx_status_type mx_status;

	mx_status = mxd_mca_timer_get_pointers( timer, &mca_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mca_stop( mca_timer->mca_record );

	timer->value = 0.0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mca_timer_clear( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mca_timer_clear()";

	MX_MCA_TIMER *mca_timer;
	mx_status_type mx_status;

	mx_status = mxd_mca_timer_get_pointers( timer, &mca_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mca_clear( mca_timer->mca_record );

	timer->value = 0.0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mca_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mca_timer_read()";

	MX_MCA_TIMER *mca_timer;
	double live_time, real_time;
	mx_status_type mx_status;

	mx_status = mxd_mca_timer_get_pointers( timer, &mca_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca_timer->use_real_time ) {
		mx_status = mx_mca_get_real_time( mca_timer->mca_record,
							&real_time );

		timer->value = real_time;
	} else {
		mx_status = mx_mca_get_live_time( mca_timer->mca_record,
							&live_time );

		timer->value = live_time;
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mca_timer_get_mode( MX_TIMER *timer )
{
	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mca_timer_set_mode()";

	if ( timer->mode != MXCM_PRESET_MODE ) {
		int mode;

		mode = timer->mode;
		timer->mode = MXCM_PRESET_MODE;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Timer mode %d is illegal for MCA timer '%s'",
			mode, timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_timer_get_last_measurement_time( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mca_timer_get_last_measurement_time()";

	MX_MCA *mca;
	MX_MCA_TIMER *mca_timer;
	mx_status_type mx_status;

	mx_status = mxd_mca_timer_get_pointers( timer, &mca_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca_timer->mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mca_record pointer for MCA timer '%s' is NULL.",
			timer->record->name );
	}

	mca = (MX_MCA *) mca_timer->mca_record->record_class_struct;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA pointer for MCA record '%s' is NULL.",
			mca_timer->mca_record->name );
	}

	timer->last_measurement_time = mca->last_measurement_interval;

	return MX_SUCCESSFUL_RESULT;
}


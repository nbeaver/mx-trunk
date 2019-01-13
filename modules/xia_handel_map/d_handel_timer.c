/*
 * Name:    d_handel_timer.c
 *
 * Purpose: MX timer driver to control XIA Handel controlled MCA timers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2006, 2009-2012, 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_HANDEL_TIMER_DEBUG		FALSE

#define MXD_HANDEL_TIMER_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_callback.h"
#include "mx_measurement.h"
#include "mx_hrt_debug.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_timer.h"
#include "mx_mca.h"

#include <handel.h>
#include <handel_errors.h>
#include <handel_generic.h>
#include <handel_constants.h>

#include "i_handel.h"
#include "d_handel_mca.h"
#include "d_handel_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_handel_timer_record_function_list = {
	NULL,
	mxd_handel_timer_create_record_structures,
	mxd_handel_timer_finish_record_initialization,
	NULL,
	NULL,
	mxd_handel_timer_open
};

MX_TIMER_FUNCTION_LIST mxd_handel_timer_timer_function_list = {
	mxd_handel_timer_is_busy,
	mxd_handel_timer_start,
	mxd_handel_timer_stop,
	mxd_handel_timer_clear,
	mxd_handel_timer_read,
	mxd_handel_timer_get_mode,
	mxd_handel_timer_set_mode
};

/* MX XIA Handel timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_handel_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_HANDEL_TIMER_STANDARD_FIELDS
};

long mxd_handel_timer_num_record_fields
		= sizeof( mxd_handel_timer_record_field_defaults )
		  / sizeof( mxd_handel_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_handel_timer_rfield_def_ptr
			= &mxd_handel_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_handel_timer_get_pointers( MX_TIMER *timer,
			MX_HANDEL_TIMER **handel_timer,
			MX_HANDEL **handel,
			const char *calling_fname )
{
	static const char fname[] = "mxd_handel_timer_get_pointers()";

	MX_HANDEL_TIMER *handel_timer_ptr;
	MX_RECORD *handel_record;

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

	handel_timer_ptr = (MX_HANDEL_TIMER *)
				timer->record->record_type_struct;

	if ( handel_timer_ptr == (MX_HANDEL_TIMER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_HANDEL_TIMER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
	}

	if ( handel_timer != (MX_HANDEL_TIMER **) NULL ) {
		*handel_timer = handel_timer_ptr;
	}

	if ( handel != (MX_HANDEL **) NULL ) {
		handel_record = handel_timer_ptr->handel_record;

		if ( handel_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The XIA Handel record pointer for MX XIA Handel timer '%s' is NULL.",
				timer->record->name );
		}

		*handel = (MX_HANDEL *) handel_record->record_type_struct;

		if ( (*handel) == (MX_HANDEL *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_HANDEL pointer for XIA Handel record '%s' used by "
	"timer record '%s' is NULL.", handel_record->name,
				timer->record->name );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_handel_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_handel_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_HANDEL_TIMER *handel_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	handel_timer = (MX_HANDEL_TIMER *) malloc( sizeof(MX_HANDEL_TIMER) );

	if ( handel_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_HANDEL_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = handel_timer;
	record->class_specific_function_list
			= &mxd_handel_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_handel_timer_finish_record_initialization()";

	MX_TIMER *timer;
	MX_HANDEL_TIMER *handel_timer;
	MX_RECORD *handel_record;
	const char *handel_driver_name;

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

	handel_timer = (MX_HANDEL_TIMER *) record->record_type_struct;

	if ( handel_timer == (MX_HANDEL_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_HANDEL_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	handel_record = handel_timer->handel_record;

	if ( handel_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The handel_record pointer for record '%s' is NULL.",
			record->name );
	}

	handel_driver_name = mx_get_driver_name( handel_record );

	if ( strcmp( handel_driver_name, "handel" ) != 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Handel timer record '%s' can only be used with a "
		"Handel record of type 'handel'.  Instead, Handel record '%s' "
		"is of type '%s'.",
			record->name, handel_record->name, handel_driver_name );
	}

	return mx_timer_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_handel_timer_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_timer_open()";

	MX_TIMER *timer;
	MX_HANDEL_TIMER *handel_timer;
	MX_HANDEL *handel;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	mx_status = mxd_handel_timer_get_pointers( timer,
					&handel_timer, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxp_handel_timer_send_value_changed_callbacks( MX_TIMER *timer,
						MX_HANDEL_TIMER *handel_timer,
						MX_HANDEL *handel )
{
	static const char fname[] =
		"mxp_handel_timer_send_value_changed_callbacks()";

	MX_RECORD *mca_record;
	MX_MCA *mca;
	MX_RECORD_FIELD *new_data_available_field;
	unsigned long i;
	mx_status_type mx_status;

#if 0
	MX_DEBUG(-2,("%s: Handel timer '%s' busy just went from TRUE to FALSE.",
		fname, timer->record->name ));
#endif

	if ( handel->mca_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The mca_record_array pointer for Handel interface '%s' is NULL.",
			handel->record->name );
	}

	for ( i = 0; i < handel->num_mcas; i++ ) {
		mca_record = handel->mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			continue;	/* Skip this MCA slot */
		}

		mca = (MX_MCA *) mca_record->record_class_struct;

		if ( mca == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		new_data_available_field = mca->new_data_available_field_ptr;

		if ( new_data_available_field == (MX_RECORD_FIELD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The 'new_data_available_field' pointer for "
			"MCA '%s' is NULL.", mca_record->name );
		}

		if ( new_data_available_field->callback_list != NULL ) {
		    mca->new_data_available = TRUE;

		    new_data_available_field->value_has_changed_manual_override
			= TRUE;

		    mx_status = mx_local_field_invoke_callback_list(
						new_data_available_field,
						MXCBT_VALUE_CHANGED );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_handel_timer_is_busy()";

	MX_HANDEL_TIMER *handel_timer;
	MX_HANDEL *handel;
	mx_bool_type old_busy;
	mx_status_type mx_status;

	mx_status = mxd_handel_timer_get_pointers( timer,
					&handel_timer, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	old_busy = timer->busy;

	mx_status = mxi_handel_is_busy( handel, &(timer->busy) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( timer->busy == FALSE ) {
		mx_status = mxd_handel_timer_stop( timer );
	}

	/* If the Handel timer has just transitioned from 'busy' to 'not busy',
	 * then we must check each of the associated MCA channels to see if
	 * value changed callbacks must be sent out.
	 */

	if ( (old_busy == TRUE) && (timer->busy == FALSE) ) {
		mx_status = mxp_handel_timer_send_value_changed_callbacks(
						timer, handel_timer, handel );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_handel_timer_start()";

	MX_HANDEL_TIMER *handel_timer;
	MX_HANDEL *handel;
	double seconds_to_count, preset_type;
	mx_status_type mx_status;

	mx_status = mxd_handel_timer_get_pointers( timer,
					&handel_timer, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds_to_count = timer->value;

#if MXD_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' starting for %g seconds.",
		fname, timer->record->name, seconds_to_count));
#endif

	/* Set preset. */

	if ( handel_timer->use_real_time ) {
		preset_type = XIA_PRESET_FIXED_REAL;
	} else {
		preset_type = XIA_PRESET_FIXED_LIVE;	/* energy livetime */
	}

	mx_status = mxi_handel_set_preset( handel,
					preset_type, seconds_to_count );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mxi_handel_set_data_available_flags( handel, TRUE );

	/* Start run */

	mx_status = mxi_handel_start_run( handel, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->last_measurement_time = seconds_to_count;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_handel_timer_stop()";

	MX_HANDEL_TIMER *handel_timer;
	MX_HANDEL *handel;
	mx_status_type mx_status;

	mx_status = mxd_handel_timer_get_pointers( timer,
					&handel_timer, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Stopping timer '%s'.", fname, timer->record->name ));
#endif

	/* Stop run */

	mx_status = mxi_handel_stop_run( handel );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read remaining time. */

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_timer_clear( MX_TIMER *timer )
{
#if 0
	static const char fname[] = "mxd_handel_timer_clear()";
#endif

	/* There does not seem to be a way of doing this without
	 * starting a new run.
	 */

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_handel_timer_read()";

	MX_HANDEL_TIMER *handel_timer;
	MX_HANDEL *handel;
	mx_status_type mx_status;

	mx_status = mxd_handel_timer_get_pointers( timer,
					&handel_timer, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read out the runtime (real time) or trigger_lifetime (live time)
	 * with xiaGetRunData().
	 */

	if ( handel_timer->use_real_time ) {
	} else {
	}

#if MXD_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' value = %g",
		fname, timer->record->name, timer->value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_timer_get_mode( MX_TIMER *timer )
{
	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_handel_timer_set_mode()";

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


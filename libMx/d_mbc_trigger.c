/*
 * Name:    d_mbc_trigger.c
 *
 * Purpose: MX timer driver to control the MBC (ALS 4.2.2) beamline
 *          trigger signal.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MBC_TRIGGER_DEBUG		TRUE

#define MXD_MBC_TRIGGER_DEBUG_TIMING	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_hrt_debug.h"
#include "mx_epics.h"
#include "mx_timer.h"
#include "d_mbc_trigger.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mbc_trigger_record_function_list = {
	NULL,
	mxd_mbc_trigger_create_record_structures,
	mxd_mbc_trigger_finish_record_initialization,
	NULL,
	NULL,
	mxd_mbc_trigger_open
};

MX_TIMER_FUNCTION_LIST mxd_mbc_trigger_timer_function_list = {
	mxd_mbc_trigger_is_busy,
	mxd_mbc_trigger_start,
	mxd_mbc_trigger_stop,
	NULL,
	mxd_mbc_trigger_read
};

/* MX MBC trigger timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mbc_trigger_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_MBC_TRIGGER_STANDARD_FIELDS
};

long mxd_mbc_trigger_num_record_fields
		= sizeof( mxd_mbc_trigger_record_field_defaults )
		  / sizeof( mxd_mbc_trigger_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mbc_trigger_rfield_def_ptr
			= &mxd_mbc_trigger_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_mbc_trigger_get_pointers( MX_TIMER *timer,
			MX_MBC_TRIGGER **mbc_trigger,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mbc_trigger_get_pointers()";

	MX_MBC_TRIGGER *mbc_trigger_ptr;
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

	mbc_trigger_ptr = (MX_MBC_TRIGGER *) timer->record->record_type_struct;

	if ( mbc_trigger_ptr == (MX_MBC_TRIGGER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_MBC_TRIGGER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
	}

	if ( mbc_trigger != (MX_MBC_TRIGGER **) NULL ) {
		*mbc_trigger = mbc_trigger_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_mbc_trigger_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_mbc_trigger_create_record_structures()";

	MX_TIMER *timer;
	MX_MBC_TRIGGER *mbc_trigger;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	mbc_trigger = (MX_MBC_TRIGGER *) malloc( sizeof(MX_MBC_TRIGGER) );

	if ( mbc_trigger == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MBC_TRIGGER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = mbc_trigger;
	record->class_specific_function_list
			= &mxd_mbc_trigger_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_mbc_trigger_finish_record_initialization()";

	MX_TIMER *timer;
	MX_MBC_TRIGGER *mbc_trigger;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	mx_status = mxd_mbc_trigger_get_pointers( timer, &mbc_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->mode = MXCM_PRESET_MODE;

	mx_status = mx_timer_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(mbc_trigger->command_pv),
			"%sNOIR:command", mbc_trigger->epics_prefix );

	mx_epics_pvname_init( &(mbc_trigger->command_trig_pv),
			"%sNOIR:commandTrig", mbc_trigger->epics_prefix );

	mx_epics_pvname_init( &(mbc_trigger->expose_pv),
			"%sCOLLECT:expose", mbc_trigger->epics_prefix );

	mx_epics_pvname_init( &(mbc_trigger->shutter_pv),
			"%sgsc:Shutter1", mbc_trigger->epics_prefix );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_mbc_trigger_open()";

	MX_TIMER *timer;
	MX_MBC_TRIGGER *mbc_trigger;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	mx_status = mxd_mbc_trigger_get_pointers( timer, &mbc_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mbc_trigger->exposure_in_progress = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mbc_trigger_is_busy()";

	MX_MBC_TRIGGER *mbc_trigger;
	long shutter_status;
	mx_status_type mx_status;

	mx_status = mxd_mbc_trigger_get_pointers( timer, &mbc_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mbc_trigger->exposure_in_progress == FALSE ) {
		timer->busy = FALSE;

		shutter_status = -1;
	} else {
		mx_status = mx_caget( &(mbc_trigger->shutter_pv),
					MX_CA_LONG, 1, &shutter_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( shutter_status ) {
			timer->busy = TRUE;
		} else {
			timer->busy = FALSE;
		}
	}

#if MXD_MBC_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s', shutter_status = %ld, busy = %d",
		fname, timer->record->name, shutter_status, timer->busy));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mbc_trigger_start()";

	MX_MBC_TRIGGER *mbc_trigger;
	double exposure_seconds;
	char epics_string[ MXU_EPICS_STRING_LENGTH+1 ];
	long command_trigger;
	mx_status_type mx_status;

	mx_status = mxd_mbc_trigger_get_pointers( timer, &mbc_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	exposure_seconds = timer->value;

#if MXD_MBC_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' starting for %g seconds.",
		fname, timer->record->name, exposure_seconds));
#endif
	/* Prepare the MBC beamline for an exposure. */

	strlcpy( epics_string, "init", sizeof(epics_string) );

	mx_status = mx_caput( &(mbc_trigger->command_pv), 
				MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	command_trigger = 0;

	mx_status = mx_caput( &(mbc_trigger->command_trig_pv),
				MX_CA_LONG, 1, &command_trigger );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caput( &(mbc_trigger->expose_pv),
				MX_CA_DOUBLE, 1, &exposure_seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the exposure. */

	command_trigger = 0;

	mx_status = mx_caput( &(mbc_trigger->command_trig_pv),
				MX_CA_LONG, 1, &command_trigger );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mbc_trigger->exposure_in_progress = TRUE;

	/* Save this measurement time. */

	timer->last_measurement_time = exposure_seconds;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mbc_trigger_stop()";

	MX_MBC_TRIGGER *mbc_trigger;
	mx_status_type mx_status;

	mx_status = mxd_mbc_trigger_get_pointers( timer, &mbc_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Stopping timer '%s'.", fname, timer->record->name ));
#endif

	mbc_trigger->exposure_in_progress = FALSE;

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mbc_trigger_read()";

	MX_MBC_TRIGGER *mbc_trigger;
	mx_status_type mx_status;

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_mbc_trigger_get_pointers( timer, &mbc_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mbc_trigger->exposure_in_progress == FALSE ) {
		timer->value = 0;
	}

#if MXD_MBC_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' value = %g",
		fname, timer->record->name, timer->value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

#endif  /* HAVE_EPICS */


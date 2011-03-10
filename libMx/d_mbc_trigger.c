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
	mxd_mbc_trigger_clear,
	mxd_mbc_trigger_read,
	mxd_mbc_trigger_get_mode,
	mxd_mbc_trigger_set_mode
};

/* MX XIA Handel timer data structures. */

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

#ifdef XYZZY

static mx_status_type
mxd_mbc_trigger_get_pointers( MX_TIMER *timer,
			MX_MBC_TRIGGER **mbc_trigger,
			MX_HANDEL **handel,
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

	if ( timer->record->mx_type != MXT_TIM_HANDEL ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The timer '%s' passed by '%s' is not an XIA Handel timer.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			timer->record->name, calling_fname,
			timer->record->mx_superclass,
			timer->record->mx_class,
			timer->record->mx_type );
	}

	mbc_trigger_ptr = (MX_MBC_TRIGGER *)
				timer->record->record_type_struct;

	if ( mbc_trigger_ptr == (MX_MBC_TRIGGER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_MBC_TRIGGER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
	}

	if ( mbc_trigger != (MX_MBC_TRIGGER **) NULL ) {
		*mbc_trigger = mbc_trigger_ptr;
	}

	if ( handel != (MX_HANDEL **) NULL ) {
		handel_record = mbc_trigger_ptr->handel_record;

		if ( handel_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The XIA Handel record pointer for MX XIA Handel timer '%s' is NULL.",
				timer->record->name );
		}

		if ( handel_record->mx_type != MXI_CTRL_HANDEL ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The handel_record '%s' passed by '%s' "
			"is not an XIA Handel record.  "
			"(superclass = %ld, class = %ld, type = %ld)",
				handel_record->name, calling_fname,
				handel_record->mx_superclass,
				handel_record->mx_class,
				handel_record->mx_type );
		}

		*handel = (MX_HANDEL *)
				handel_record->record_type_struct;

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

	mbc_trigger = (MX_MBC_TRIGGER *)
				malloc( sizeof(MX_MBC_TRIGGER) );

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

	mbc_trigger = (MX_MBC_TRIGGER *) record->record_type_struct;

	if ( mbc_trigger == (MX_MBC_TRIGGER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MBC_TRIGGER pointer for record '%s' is NULL.",
			record->name );
	}

	return mx_timer_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_mbc_trigger_open()";

	MX_TIMER *timer;
	MX_MBC_TRIGGER *mbc_trigger;
	MX_HANDEL *handel;
	MX_RECORD *mca_record;
	MX_HANDEL_MCA *handel_mca;
	unsigned long i;
	int xia_status, detector_channel;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	mx_status = mxd_mbc_trigger_get_pointers( timer,
					&mbc_trigger, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_warning( "The 'mbc_trigger' driver for timer record '%s' "
	"does not yet work correctly.", record->name );

	/* Choose a detector channel set number. */

	mbc_trigger->detector_channel_set = 1000;

	/* Add the detector channels for all the MCAs to
	 * this detector channel set.
	 */

	for ( i = 0; i < handel->num_mcas; i++ ) {
		mca_record = handel->mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MCA record pointer %lu for XIA Handel record '%s' used by timer '%s' is NULL.",
				i, handel->record->name,
				record->name );
		}

		handel_mca = (MX_HANDEL_MCA *) mca_record->record_type_struct;

		if ( handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "MX_HANDEL_MCA pointer for MCA '%s' used by timer record '%s' is NULL.",
				mca_record->name, record->name );
		}

		detector_channel = handel_mca->detector_channel;

		if ( i == 0 ) {
			mbc_trigger->first_detector_channel
				= detector_channel;
		}

#if MXD_MBC_TRIGGER_DEBUG
		MX_DEBUG(-2,("%s: adding MCA #%d = '%s', detector_channel = %d",
			fname, i, mca_record->name, detector_channel ));
#endif

		xia_status = xiaAddChannelSetElem(
				mbc_trigger->detector_channel_set,
				detector_channel );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unable to add detector channel %d for MCA '%s' "
			"to detector channel set %d for timer '%s'.",
				detector_channel, mca_record->name,
				mbc_trigger->detector_channel_set,
				record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mbc_trigger_is_busy()";

	MX_MBC_TRIGGER *mbc_trigger;
	MX_HANDEL *handel;
	unsigned long run_active;
	int xia_status;
	mx_status_type mx_status;

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_mbc_trigger_get_pointers( timer,
					&mbc_trigger, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaGetRunData( mbc_trigger->first_detector_channel,
					"run_active", &run_active );

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", timer->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get Handel running status for MCA timer '%s'.  "
		"Error code = %d, error message = '%s'",
			timer->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	if ( run_active & XIA_RUN_HARDWARE ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

#if MXD_MBC_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s', run_active = %#lx, busy = %d",
		fname, timer->record->name, run_active, timer->busy));
#endif

	if ( timer->busy == FALSE ) {
		mx_status = mxd_mbc_trigger_stop( timer );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mbc_trigger_start()";

	MX_MBC_TRIGGER *mbc_trigger;
	MX_HANDEL *handel;
	double seconds_to_count;
	int xia_status, ignored;
	mx_status_type mx_status;

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_mbc_trigger_get_pointers( timer,
					&mbc_trigger, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds_to_count = timer->value;

#if MXD_MBC_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' starting for %g seconds.",
		fname, timer->record->name, seconds_to_count));
#endif

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	if ( mbc_trigger->use_real_time ) {
		xia_status = xiaSetAcquisitionValues(
				mbc_trigger->detector_channel_set,
				"preset_value",
				(void *) &seconds_to_count );
	} else {
		xia_status = xiaSetAcquisitionValues(
				mbc_trigger->detector_channel_set,
				"preset_value",
				(void *) &seconds_to_count );
	}

	ignored = 0;

	xia_status = xiaBoardOperation( mbc_trigger->first_detector_channel,
					"apply", (void *) &ignored );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot apply acquisition values for Handel timer '%s' "
		"on behalf of MCA '%s'.  "
		"Error code = %d, '%s'",
			timer->record->name,
			xia_status,
			mxi_handel_strerror( xia_status ) );
	}

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"to set preset time for record '%s'", timer->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot set preset time for MCA timer '%s'.  "
		"Error code = %d, error message = '%s'",
			timer->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	mxi_handel_set_data_available_flags( handel, TRUE );

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaStartRun( mbc_trigger->detector_channel_set, 1 );

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"to start the run for record '%s'", timer->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot start a run for MCA timer '%s'.  "
		"Error code = %d, error message = '%s'",
			timer->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	timer->last_measurement_time = seconds_to_count;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mbc_trigger_stop()";

	MX_MBC_TRIGGER *mbc_trigger;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_mbc_trigger_get_pointers( timer,
					&mbc_trigger, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Stopping timer '%s'.", fname, timer->record->name ));
#endif

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaStopRun( mbc_trigger->detector_channel_set );

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"to stop the run for record '%s'", timer->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot stop a run for MCA timer '%s'.  "
		"Error code = %d, error message = '%s'",
			timer->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_clear( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mbc_trigger_clear()";

	/* There does not seem to be a way of doing this without
	 * starting a new run.
	 */

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mbc_trigger_read()";

	MX_MBC_TRIGGER *mbc_trigger;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_mbc_trigger_get_pointers( timer,
					&mbc_trigger, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	if ( mbc_trigger->use_real_time ) {
		xia_status = xiaGetRunData(
				mbc_trigger->detector_channel_set,
				"runtime", &(timer->value) );
	} else {
		xia_status = xiaGetRunData(
				mbc_trigger->detector_channel_set,
				"trigger_livetime", &(timer->value) );
	}

#if MXD_MBC_TRIGGER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", timer->record->name );
#endif

	if ( mbc_trigger->use_real_time ) {
		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Cannot read the real time for MCA timer '%s'.  "
			"Error code = %d, error message = '%s'",
				timer->record->name, xia_status,
				mxi_handel_strerror( xia_status ) );
		}
	} else {
		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Cannot read the live time for MCA timer '%s'.  "
			"Error code = %d, error message = '%s'",
				timer->record->name, xia_status,
				mxi_handel_strerror( xia_status ) );
		}
	}

#if MXD_MBC_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' value = %g",
		fname, timer->record->name, timer->value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_get_mode( MX_TIMER *timer )
{
	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_trigger_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_mbc_trigger_set_mode()";

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

#endif  /* XYZZY */

#endif  /* HAVE_EPICS */


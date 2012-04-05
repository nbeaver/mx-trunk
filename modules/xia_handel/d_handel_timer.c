/*
 * Name:    d_handel_timer.c
 *
 * Purpose: MX timer driver to control XIA Handel controlled MCA timers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2006, 2009-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_HANDEL_TIMER_DEBUG		FALSE

#define MXD_HANDEL_TIMER_DEBUG_TIMING	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_XIA_HANDEL

#include <stdlib.h>
#include <string.h>

#include <handel.h>
#include <handel_errors.h>
#include <handel_generic.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_hrt_debug.h"
#include "mx_timer.h"
#include "mx_mca.h"
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

	handel_timer = (MX_HANDEL_TIMER *)
				malloc( sizeof(MX_HANDEL_TIMER) );

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

	mx_status = mxd_handel_timer_get_pointers( timer,
					&handel_timer, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_warning( "The 'handel_timer' driver for timer record '%s' "
	"does not yet work correctly.", record->name );

	/* Choose a detector channel set number. */

	handel_timer->detector_channel_set = 1000;

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
			handel_timer->first_detector_channel
				= detector_channel;
		}

#if MXD_HANDEL_TIMER_DEBUG
		MX_DEBUG(-2,
		("%s: adding MCA #%lu = '%s', detector_channel = %d",
			fname, i, mca_record->name, detector_channel ));
#endif

		xia_status = xiaAddChannelSetElem(
				handel_timer->detector_channel_set,
				detector_channel );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unable to add detector channel %d for MCA '%s' "
			"to detector channel set %d for timer '%s'.",
				detector_channel, mca_record->name,
				handel_timer->detector_channel_set,
				record->name );
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
	unsigned long run_active;
	int xia_status;
	mx_status_type mx_status;

#if MXD_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_handel_timer_get_pointers( timer,
					&handel_timer, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaGetRunData( handel_timer->first_detector_channel,
					"run_active", &run_active );

#if MXD_HANDEL_TIMER_DEBUG_TIMING
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

#if MXD_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s', run_active = %#lx, busy = %d",
		fname, timer->record->name, run_active, timer->busy));
#endif

	if ( timer->busy == FALSE ) {
		mx_status = mxd_handel_timer_stop( timer );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_handel_timer_start()";

	MX_HANDEL_TIMER *handel_timer;
	MX_HANDEL *handel;
	double seconds_to_count;
	int xia_status, ignored;
	mx_status_type mx_status;

#if MXD_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_handel_timer_get_pointers( timer,
					&handel_timer, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds_to_count = timer->value;

#if MXD_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' starting for %g seconds.",
		fname, timer->record->name, seconds_to_count));
#endif

#if MXD_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	if ( handel_timer->use_real_time ) {
		xia_status = xiaSetAcquisitionValues(
				handel_timer->detector_channel_set,
				"preset_value",
				(void *) &seconds_to_count );
	} else {
		xia_status = xiaSetAcquisitionValues(
				handel_timer->detector_channel_set,
				"preset_value",
				(void *) &seconds_to_count );
	}

	ignored = 0;

	xia_status = xiaBoardOperation( handel_timer->first_detector_channel,
					"apply", (void *) &ignored );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot apply acquisition values for Handel timer '%s' "
		"on behalf of MCA '%s'.  "
		"Error code = %d, '%s'",
			timer->record->name,
			handel->record->name,
			xia_status,
			mxi_handel_strerror( xia_status ) );
	}

#if MXD_HANDEL_TIMER_DEBUG_TIMING
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

#if MXD_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaStartRun( handel_timer->detector_channel_set, 1 );

#if MXD_HANDEL_TIMER_DEBUG_TIMING
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
mxd_handel_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_handel_timer_stop()";

	MX_HANDEL_TIMER *handel_timer;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

#if MXD_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_handel_timer_get_pointers( timer,
					&handel_timer, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Stopping timer '%s'.", fname, timer->record->name ));
#endif

#if MXD_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaStopRun( handel_timer->detector_channel_set );

#if MXD_HANDEL_TIMER_DEBUG_TIMING
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
	int xia_status;
	mx_status_type mx_status;

#if MXD_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_handel_timer_get_pointers( timer,
					&handel_timer, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	if ( handel_timer->use_real_time ) {
		xia_status = xiaGetRunData(
				handel_timer->detector_channel_set,
				"runtime", &(timer->value) );
	} else {
		xia_status = xiaGetRunData(
				handel_timer->detector_channel_set,
				"trigger_livetime", &(timer->value) );
	}

#if MXD_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", timer->record->name );
#endif

	if ( handel_timer->use_real_time ) {
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

#endif  /* HAVE_XIA_HANDEL */


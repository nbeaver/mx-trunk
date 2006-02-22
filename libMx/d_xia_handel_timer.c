/*
 * Name:    d_xia_handel_timer.c
 *
 * Purpose: MX timer driver to control XIA Handel controlled MCA timers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_XIA_HANDEL_TIMER_DEBUG		FALSE

#define MXD_XIA_HANDEL_TIMER_DEBUG_TIMING	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_XIA_HANDEL

#include <stdlib.h>
#include <string.h>

#include <xia_version.h>
#include <handel.h>
#include <handel_errors.h>
#include <handel_generic.h>

#include "mx_unistd.h"

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_hrt_debug.h"
#include "mx_timer.h"
#include "mx_mca.h"
#include "i_xia_handel.h"
#include "d_xia_dxp_mca.h"
#include "d_xia_handel_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_xia_handel_timer_record_function_list = {
	NULL,
	mxd_xia_handel_timer_create_record_structures,
	mxd_xia_handel_timer_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_xia_handel_timer_open
};

MX_TIMER_FUNCTION_LIST mxd_xia_handel_timer_timer_function_list = {
	mxd_xia_handel_timer_is_busy,
	mxd_xia_handel_timer_start,
	mxd_xia_handel_timer_stop,
	mxd_xia_handel_timer_clear,
	mxd_xia_handel_timer_read,
	mxd_xia_handel_timer_get_mode,
	mxd_xia_handel_timer_set_mode,
	NULL,
#if 0
	mxd_xia_handel_timer_get_last_measurement_time
#endif
};

/* MX XIA Handel timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_xia_handel_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_XIA_HANDEL_TIMER_STANDARD_FIELDS
};

long mxd_xia_handel_timer_num_record_fields
		= sizeof( mxd_xia_handel_timer_record_field_defaults )
		  / sizeof( mxd_xia_handel_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_xia_handel_timer_rfield_def_ptr
			= &mxd_xia_handel_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_xia_handel_timer_get_pointers( MX_TIMER *timer,
			MX_XIA_HANDEL_TIMER **xia_handel_timer,
			MX_XIA_HANDEL **xia_handel,
			const char *calling_fname )
{
	static const char fname[] = "mxd_xia_handel_timer_get_pointers()";

	MX_XIA_HANDEL_TIMER *xia_handel_timer_ptr;
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

	xia_handel_timer_ptr = (MX_XIA_HANDEL_TIMER *)
				timer->record->record_type_struct;

	if ( xia_handel_timer_ptr == (MX_XIA_HANDEL_TIMER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_XIA_HANDEL_TIMER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
	}

	if ( xia_handel_timer != (MX_XIA_HANDEL_TIMER **) NULL ) {
		*xia_handel_timer = xia_handel_timer_ptr;
	}

	if ( xia_handel != (MX_XIA_HANDEL **) NULL ) {
		handel_record = xia_handel_timer_ptr->handel_record;

		if ( handel_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The XIA Handel record pointer for MX XIA Handel timer '%s' is NULL.",
				timer->record->name );
		}

		if ( handel_record->mx_type != MXI_GEN_XIA_HANDEL ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The handel_record '%s' passed by '%s' "
			"is not an XIA Handel record.  "
			"(superclass = %ld, class = %ld, type = %ld)",
				handel_record->name, calling_fname,
				handel_record->mx_superclass,
				handel_record->mx_class,
				handel_record->mx_type );
		}

		*xia_handel = (MX_XIA_HANDEL *)
				handel_record->record_type_struct;

		if ( (*xia_handel) == (MX_XIA_HANDEL *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_XIA_HANDEL pointer for XIA Handel record '%s' used by "
	"timer record '%s' is NULL.", handel_record->name,
				timer->record->name );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_xia_handel_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_xia_handel_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_XIA_HANDEL_TIMER *xia_handel_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	xia_handel_timer = (MX_XIA_HANDEL_TIMER *)
				malloc( sizeof(MX_XIA_HANDEL_TIMER) );

	if ( xia_handel_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_XIA_HANDEL_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = xia_handel_timer;
	record->class_specific_function_list
			= &mxd_xia_handel_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_handel_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_xia_handel_timer_finish_record_initialization()";

	MX_TIMER *timer;
	MX_XIA_HANDEL_TIMER *xia_handel_timer;

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

	xia_handel_timer = (MX_XIA_HANDEL_TIMER *) record->record_type_struct;

	if ( xia_handel_timer == (MX_XIA_HANDEL_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XIA_HANDEL_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	return mx_timer_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_xia_handel_timer_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_xia_handel_timer_open()";

	MX_TIMER *timer;
	MX_XIA_HANDEL_TIMER *xia_handel_timer;
	MX_XIA_HANDEL *xia_handel;
	MX_RECORD *mca_record;
	MX_XIA_DXP_MCA *xia_dxp_mca;
	unsigned long i;
	int xia_status, detector_channel;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	mx_status = mxd_xia_handel_timer_get_pointers( timer,
					&xia_handel_timer, &xia_handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Choose a detector channel set number. */

	xia_handel_timer->detector_channel_set = 1000;

	/* Add the detector channels for all the MCAs to
	 * this detector channel set.
	 */

	for ( i = 0; i < xia_handel->num_mcas; i++ ) {
		mca_record = xia_handel->mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MCA record pointer %lu for XIA Handel record '%s' used by timer '%s' is NULL.",
				i, xia_handel->record->name,
				record->name );
		}

		xia_dxp_mca = (MX_XIA_DXP_MCA *) mca_record->record_type_struct;

		if ( xia_dxp_mca == (MX_XIA_DXP_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "MX_XIA_DXP_MCA pointer for MCA '%s' used by timer record '%s' is NULL.",
				mca_record->name, record->name );
		}

		detector_channel = xia_dxp_mca->detector_channel;

		if ( i == 0 ) {
			xia_handel_timer->first_detector_channel
				= detector_channel;
		}

#if MXD_XIA_HANDEL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: adding MCA #%d = '%s', detector_channel = %d",
			fname, i, mca_record->name, detector_channel ));
#endif

		xia_status = xiaAddChannelSetElem(
				xia_handel_timer->detector_channel_set,
				detector_channel );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unable to add detector channel %d for MCA '%s' "
			"to detector channel set %d for timer '%s'.",
				detector_channel, mca_record->name,
				xia_handel_timer->detector_channel_set,
				record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_handel_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_handel_timer_is_busy()";

	MX_XIA_HANDEL_TIMER *xia_handel_timer;
	MX_XIA_HANDEL *xia_handel;
	unsigned long run_active;
	int xia_status;
	mx_status_type mx_status;

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_xia_handel_timer_get_pointers( timer,
					&xia_handel_timer, &xia_handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaGetRunData( xia_handel_timer->first_detector_channel,
					"run_active", &run_active );

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", timer->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get Handel running status for MCA timer '%s'.  "
		"Error code = %d, error message = '%s'",
			timer->record->name, xia_status,
			mxi_xia_handel_strerror( xia_status ) );
	}

	if ( run_active & XIA_RUN_HARDWARE ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

#if MXD_XIA_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s', run_active = %#lx, busy = %d",
		fname, timer->record->name, run_active, timer->busy));
#endif

	if ( timer->busy == FALSE ) {
		mx_status = mxd_xia_handel_timer_stop( timer );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_handel_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_handel_timer_start()";

	MX_XIA_HANDEL_TIMER *xia_handel_timer;
	MX_XIA_HANDEL *xia_handel;
	double seconds_to_count;
	int xia_status;
	mx_status_type mx_status;

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_xia_handel_timer_get_pointers( timer,
					&xia_handel_timer, &xia_handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds_to_count = timer->value;

#if MXD_XIA_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' starting for %g seconds.",
		fname, timer->record->name, seconds_to_count));
#endif

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	if ( xia_handel_timer->use_real_time ) {
		xia_status = xiaSetAcquisitionValues(
				xia_handel_timer->detector_channel_set,
				"preset_runtime",
				(void *) &seconds_to_count );
	} else {
		xia_status = xiaSetAcquisitionValues(
				xia_handel_timer->detector_channel_set,
				"preset_livetime",
				(void *) &seconds_to_count );
	}

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"to set preset time for record '%s'", timer->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot set preset time for MCA timer '%s'.  "
		"Error code = %d, error message = '%s'",
			timer->record->name, xia_status,
			mxi_xia_handel_strerror( xia_status ) );
	}

	mxi_xia_handel_set_data_available_flags( xia_handel, TRUE );

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaStartRun( xia_handel_timer->detector_channel_set, 0 );

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"to start the run for record '%s'", timer->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot start a run for MCA timer '%s'.  "
		"Error code = %d, error message = '%s'",
			timer->record->name, xia_status,
			mxi_xia_handel_strerror( xia_status ) );
	}

	timer->last_measurement_time = seconds_to_count;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_handel_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_handel_timer_stop()";

	MX_XIA_HANDEL_TIMER *xia_handel_timer;
	MX_XIA_HANDEL *xia_handel;
	int xia_status;
	mx_status_type mx_status;

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_xia_handel_timer_get_pointers( timer,
					&xia_handel_timer, &xia_handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XIA_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Stopping timer '%s'.", fname, timer->record->name ));
#endif

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaStopRun( xia_handel_timer->detector_channel_set );

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"to stop the run for record '%s'", timer->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot stop a run for MCA timer '%s'.  "
		"Error code = %d, error message = '%s'",
			timer->record->name, xia_status,
			mxi_xia_handel_strerror( xia_status ) );
	}

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_handel_timer_clear( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_handel_timer_clear()";

	/* There does not seem to be a way of doing this without
	 * starting a new run.
	 */

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_handel_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_handel_timer_read()";

	MX_XIA_HANDEL_TIMER *xia_handel_timer;
	MX_XIA_HANDEL *xia_handel;
	int xia_status;
	mx_status_type mx_status;

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_xia_handel_timer_get_pointers( timer,
					&xia_handel_timer, &xia_handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	if ( xia_handel_timer->use_real_time ) {
		xia_status = xiaGetRunData(
				xia_handel_timer->detector_channel_set,
				"runtime", &(timer->value) );
	} else {
		xia_status = xiaGetRunData(
				xia_handel_timer->detector_channel_set,
				"livetime", &(timer->value) );
	}

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", timer->record->name );
#endif

	if ( xia_handel_timer->use_real_time ) {
		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Cannot read the real time for MCA timer '%s'.  "
			"Error code = %d, error message = '%s'",
				timer->record->name, xia_status,
				mxi_xia_handel_strerror( xia_status ) );
		}
	} else {
		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Cannot read the live time for MCA timer '%s'.  "
			"Error code = %d, error message = '%s'",
				timer->record->name, xia_status,
				mxi_xia_handel_strerror( xia_status ) );
		}
	}

#if MXD_XIA_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' value = %g",
		fname, timer->record->name, timer->value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_handel_timer_get_mode( MX_TIMER *timer )
{
	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_handel_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_handel_timer_set_mode()";

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
mxd_xia_handel_timer_get_last_measurement_time( MX_TIMER *timer )
{
	static const char fname[] =
			"mxd_xia_handel_timer_get_last_measurement_time()";

	MX_XIA_HANDEL_TIMER *xia_handel_timer;
	MX_XIA_HANDEL *xia_handel;
	double seconds_to_count;
	int xia_status;
	mx_status_type mx_status;

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_xia_handel_timer_get_pointers( timer,
					&xia_handel_timer, &xia_handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	if ( xia_handel_timer->use_real_time ) {
		xia_status = xiaGetAcquisitionValues(
				xia_handel_timer->first_detector_channel,
				"preset_runtime",
				(void *) &seconds_to_count );
	} else {
		xia_status = xiaGetAcquisitionValues(
				xia_handel_timer->first_detector_channel,
				"preset_livetime",
				(void *) &seconds_to_count );
	}

#if MXD_XIA_HANDEL_TIMER_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", timer->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get preset time for MCA timer '%s'.  "
		"Error code = %d, error message = '%s'",
			timer->record->name, xia_status,
			mxi_xia_handel_strerror( xia_status ) );
	}

	timer->last_measurement_time = seconds_to_count;

#if MXD_XIA_HANDEL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer '%s' last measurement time = %g",
		fname, timer->record->name, timer->last_measurement_time));
#endif

#endif

	return MX_SUCCESSFUL_RESULT;
}

#endif  /* HAVE_XIA_HANDEL */


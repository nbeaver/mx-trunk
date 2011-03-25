/*
 * Name:    d_radicon_helios_trigger.c
 *
 * Purpose: MX pulse generator driver to generate a trigger pulse for the
 *          Radicon Helios detectors using the Pleora iPORT PLC.
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

#define MXD_RADICON_HELIOS_TRIGGER_DEBUG		TRUE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_pulse_generator.h"
#include "d_radicon_helios_trigger.h"

/* Initialize the pulse generator driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_rh_trigger_record_function_list = {
	NULL,
	mxd_rh_trigger_create_record_structures,
	mxd_rh_trigger_finish_record_initialization,
	NULL,
	NULL,
	mxd_rh_trigger_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_rh_trigger_pulser_function_list = {
	mxd_rh_trigger_is_busy,
	mxd_rh_trigger_start,
	mxd_rh_trigger_stop,
	mxd_rh_trigger_get_parameter,
	mxd_rh_trigger_set_parameter
};

/* MX MBC trigger timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_rh_trigger_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_RADICON_HELIOS_TRIGGER_STANDARD_FIELDS
};

long mxd_rh_trigger_num_record_fields
		= sizeof( mxd_rh_trigger_record_field_defaults )
		  / sizeof( mxd_rh_trigger_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_rh_trigger_rfield_def_ptr
			= &mxd_rh_trigger_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_rh_trigger_get_pointers( MX_PULSE_GENERATOR *pulser,
			MX_RADICON_HELIOS_TRIGGER **rh_trigger,
			const char *calling_fname )
{
	static const char fname[] = "mxd_rh_trigger_get_pointers()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger_ptr;

	if ( pulser == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( pulser->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for timer pointer passed by '%s' is NULL.",
			calling_fname );
	}

	rh_trigger_ptr = (MX_RADICON_HELIOS_TRIGGER *)
					pulser->record->record_type_struct;

	if ( rh_trigger_ptr == (MX_RADICON_HELIOS_TRIGGER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RADICON_HELIOS_TRIGGER pointer for pulse generator "
			"record '%s' passed by '%s' is NULL",
				pulser->record->name, calling_fname );
	}

	if ( rh_trigger != (MX_RADICON_HELIOS_TRIGGER **) NULL ) {
		*rh_trigger = rh_trigger_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_rh_trigger_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_rh_trigger_create_record_structures()";

	MX_PULSE_GENERATOR *pulser;
	MX_RADICON_HELIOS_TRIGGER *rh_trigger;

	/* Allocate memory for the necessary structures. */

	pulser = (MX_PULSE_GENERATOR *) malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PULSE_GENERATOR structure." );
	}

	rh_trigger = (MX_RADICON_HELIOS_TRIGGER *)
				malloc( sizeof(MX_RADICON_HELIOS_TRIGGER) );

	if ( rh_trigger == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RADICON_HELIOS_TRIGGER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = pulser;
	record->record_type_struct = rh_trigger;
	record->class_specific_function_list
			= &mxd_rh_trigger_pulser_function_list;

	pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_rh_trigger_finish_record_initialization()";

	MX_PULSE_GENERATOR *pulser;
	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_rh_trigger_get_pointers( pulser,
						&rh_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(rh_trigger->collect_count_pv),
			"%ssclC1.CNT", rh_trigger->epics_prefix );

	mx_epics_pvname_init( &(rh_trigger->exposure_gate_enable_pv),
			"%sgsc:ExpGateEna", rh_trigger->epics_prefix );

	mx_epics_pvname_init( &(rh_trigger->collect_preset_enable_pv),
			"%ssclC1.G1", rh_trigger->epics_prefix );

	mx_epics_pvname_init( &(rh_trigger->shutter_pv),
			"%sgsc:Shutter1", rh_trigger->epics_prefix );

	mx_epics_pvname_init( &(rh_trigger->shutter_control_pv),
			"%sgsc:CtrlSrcOut", rh_trigger->epics_prefix );

	mx_epics_pvname_init( &(rh_trigger->shutter_exposure_enable_pv),
			"%sgsc:Gate2GateEna", rh_trigger->epics_prefix );

	mx_epics_pvname_init( &(rh_trigger->shutter_gate_enable_pv),
			"%sgsc:Gate1GateEna", rh_trigger->epics_prefix );

	mx_epics_pvname_init( &(rh_trigger->shutter_shutter_enable_pv),
			"%sgsc:Sht1GateEna", rh_trigger->epics_prefix );

	mx_epics_pvname_init( &(rh_trigger->shutter_timeout_val_pv),
			"%sgsc:TimeoutVal", rh_trigger->epics_prefix );

	mx_epics_pvname_init( &(rh_trigger->timer_count_pv),
			"%ssclT1.CNT", rh_trigger->epics_prefix );

	mx_epics_pvname_init( &(rh_trigger->timer_preset_pv),
			"%ssclT1.TP", rh_trigger->epics_prefix );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_rh_trigger_open()";

	MX_PULSE_GENERATOR *pulser;
	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_rh_trigger_get_pointers( pulser,
						&rh_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rh_trigger->exposure_in_progress = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_is_busy( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_rh_trigger_is_busy()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	long shutter_status, timer_status;
	mx_status_type mx_status;

	mx_status = mxd_rh_trigger_get_pointers( pulser,
						&rh_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( rh_trigger->exposure_in_progress == FALSE ) {
		pulser->busy = FALSE;

		shutter_status = -1;
		timer_status   = -1;
	} else {
		mx_status = mx_caget( &(rh_trigger->shutter_pv),
					MX_CA_LONG, 1, &shutter_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(rh_trigger->timer_count_pv),
					MX_CA_LONG, 1, &timer_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( shutter_status | timer_status ) {
			pulser->busy = TRUE;
		} else {
			pulser->busy = FALSE;

			rh_trigger->exposure_in_progress = FALSE;

			/* Stop the "Collect" counter so that the time read
			 * out will reset to zero the next time the counter
			 * is started.
			 */

			mx_status = mx_caput(
					&(rh_trigger->collect_count_pv),
					MX_CA_STRING, 1, "Done" );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Pulse generator '%s', "
		"shutter_status = %ld, timer_status = %ld, busy = %d",
		fname, pulser->record->name,
		shutter_status, timer_status, pulser->busy));
#endif

	return mx_status;
}

/*----*/

static mx_status_type
mxd_rh_trigger_start_still_or_dark( MX_PULSE_GENERATOR *pulser,
					MX_RADICON_HELIOS_TRIGGER *rh_trigger )
{
#if 0
	static const char fname[] = "mxd_rh_trigger_start_still_or_dark()";
#endif

	double exposure_seconds, shutter_timeout;
	long exposure_mode;
	mx_status_type mx_status;

	exposure_seconds = pulser->pulse_width;

	exposure_mode = rh_trigger->area_detector_exposure_mode;

	/* Prepare the Joerger scaler called "Counter" to measure
	 * the actual exposure time.
	 */

	/* Disable the "Counter" internal timer gate so that it will use the
	 * external gate input instead to determine its measurement time.
	 */

	mx_status = mx_caput( &(rh_trigger->collect_preset_enable_pv),
				MX_CA_STRING, 1, "N" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the "Counter" scaler to 'start' counting.  Since
	 * the counter is configured for external gating, no actual counting
	 * will occur until the Gate1 output of the GonioSync Controller
	 * sends a gate pulse.
	 */

	mx_status = mx_caput_nowait( &(rh_trigger->collect_count_pv),
				MX_CA_STRING, 1, "Count" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the GSC to take its timer input from the "Time" scaler. */

	mx_status = mx_caput( &(rh_trigger->shutter_control_pv),
				MX_CA_STRING, 1, "Timer" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set a bunch of enable flags. */

	mx_status = mx_caput( &(rh_trigger->shutter_gate_enable_pv),
				MX_CA_STRING, 1, "Enabled" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caput( &(rh_trigger->exposure_gate_enable_pv),
				MX_CA_STRING, 1, "Enabled" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caput( &(rh_trigger->shutter_exposure_enable_pv),
				MX_CA_STRING, 1, "Enabled" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( exposure_mode == MXF_AD_DARK_MODE ) {
		mx_status = mx_caput(
				&(rh_trigger->shutter_shutter_enable_pv),
				MX_CA_STRING, 1, "Disabled" );
	} else {
		mx_status = mx_caput(
				&(rh_trigger->shutter_shutter_enable_pv),
				MX_CA_STRING, 1, "Enabled" );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the exposure time in the "Time" scaler. */

	mx_status = mx_caput( &(rh_trigger->timer_preset_pv),
				MX_CA_DOUBLE, 1, &exposure_seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute and set the timeout for the exposure shutter. */

	/* NOTE: This is the correct calculation for 'still' and 'dark'
	 * measurements, but not for 'expose' requests.
	 */

	shutter_timeout = ( 1.1 * exposure_seconds ) + 0.5;

	mx_status = mx_caput( &(rh_trigger->shutter_timeout_val_pv),
				MX_CA_DOUBLE, 1, &shutter_timeout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the exposure. */

	mx_status = mx_caput_nowait( &(rh_trigger->timer_count_pv),
					MX_CA_STRING, 1, "Count" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rh_trigger->exposure_in_progress = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_rh_trigger_start_expose( MX_PULSE_GENERATOR *pulser,
				MX_RADICON_HELIOS_TRIGGER *rh_trigger )
{
	static const char fname[] = "mxd_rh_trigger_start_expose()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Pulse generator start for record '%s' is not yet implemented "
	"for expose/oscillation mode.", pulser->record->name );
}

MX_EXPORT mx_status_type
mxd_rh_trigger_start( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_rh_trigger_start()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	mx_status_type mx_status;

	mx_status = mxd_rh_trigger_get_pointers( pulser,
						&rh_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG
	MX_DEBUG(-2,
	("%s: Pulse generator '%s' starting for %g seconds, exposure mode %ld.",
		fname, pulser->record->name,
		pulser->pulse_width,
		rh_trigger->area_detector_exposure_mode ));
#endif

	switch( rh_trigger->area_detector_exposure_mode ) {
	case MXF_AD_STILL_MODE:
	case MXF_AD_DARK_MODE:
		mx_status = mxd_rh_trigger_start_still_or_dark(
						pulser, rh_trigger );
		break;
	case MXF_AD_EXPOSE_MODE:
		mx_status = mxd_rh_trigger_start_expose( pulser,
							rh_trigger );
		break;
	case 0:
		mx_status = mx_error( MXE_INITIALIZATION_ERROR, fname,
			"The area detector exposure mode has not yet been set "
			"for pulse generator '%s'.",
				pulser->record->name );
		break;
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported area detector exposure mode %ld requested "
			"for pulse generator '%s'.",
				rh_trigger->area_detector_exposure_mode,
				pulser->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_stop( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_rh_trigger_stop()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	mx_status_type mx_status;

	mx_status = mxd_rh_trigger_get_pointers( pulser,
						&rh_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Stopping pulse generator '%s'.",
		fname, pulser->record->name ));
#endif
	mx_status = mx_caput_nowait( &(rh_trigger->timer_count_pv),
					MX_CA_STRING, 1, "Done" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rh_trigger->exposure_in_progress = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_get_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_rh_trigger_get_parameter()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_rh_trigger_get_pointers( pulser,
						&rh_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		break;
	case MXLV_PGN_PULSE_WIDTH:
		break;
	case MXLV_PGN_PULSE_DELAY:
		break;
	case MXLV_PGN_MODE:
		break;
	case MXLV_PGN_PULSE_PERIOD:
		break;
	default:
		return
		    mx_pulse_generator_default_get_parameter_handler( pulser );
	}

	MX_DEBUG(-2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rh_trigger_set_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_rh_trigger_set_parameter()";

	MX_RADICON_HELIOS_TRIGGER *rh_trigger;
	mx_status_type mx_status;

	rh_trigger = NULL;

	mx_status = mxd_rh_trigger_get_pointers( pulser,
						&rh_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_HELIOS_TRIGGER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		pulser->num_pulses = 1;
		break;

	case MXLV_PGN_PULSE_WIDTH:
		/* This value is used by the start() routine above. */
		break;

	case MXLV_PGN_PULSE_DELAY:
		pulser->pulse_delay = 0;
		break;

	case MXLV_PGN_MODE:
		pulser->mode = MXF_PGN_SQUARE_WAVE;
		break;

	case MXLV_PGN_PULSE_PERIOD:
		pulser->pulse_period = pulser->pulse_width;
		break;

	default:
		return
		    mx_pulse_generator_default_set_parameter_handler( pulser );
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

#endif  /* HAVE_EPICS */


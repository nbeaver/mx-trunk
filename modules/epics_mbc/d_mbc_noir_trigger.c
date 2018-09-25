/*
 * Name:    d_mbc_noir_trigger.c
 *
 * Purpose: MX pulse generator driver to control the MBC (ALS 4.2.2)
 *          beamline trigger signal using MBC:NOIR: EPICS PVs.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2011, 2015, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MBC_NOIR_TRIGGER_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_pulse_generator.h"
#include "d_mbc_noir_trigger.h"

/* Initialize the pulse generator driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mbc_noir_trigger_record_function_list = {
	NULL,
	mxd_mbc_noir_trigger_create_record_structures,
	mxd_mbc_noir_trigger_finish_record_initialization,
	NULL,
	NULL,
	mxd_mbc_noir_trigger_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_mbc_noir_trigger_pulser_function_list = {
	mxd_mbc_noir_trigger_is_busy,
	mxd_mbc_noir_trigger_arm,
	NULL,
	mxd_mbc_noir_trigger_stop,
	NULL,
	mxd_mbc_noir_trigger_get_parameter,
	mxd_mbc_noir_trigger_set_parameter
};

/* MX MBC trigger timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mbc_noir_trigger_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_MBC_NOIR_TRIGGER_STANDARD_FIELDS
};

long mxd_mbc_noir_trigger_num_record_fields
		= sizeof( mxd_mbc_noir_trigger_record_field_defaults )
		  / sizeof( mxd_mbc_noir_trigger_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mbc_noir_trigger_rfield_def_ptr
			= &mxd_mbc_noir_trigger_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_mbc_noir_trigger_get_pointers( MX_PULSE_GENERATOR *pulser,
			MX_MBC_NOIR_TRIGGER **mbc_noir_trigger,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mbc_noir_trigger_get_pointers()";

	MX_MBC_NOIR_TRIGGER *mbc_noir_trigger_ptr;

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

	mbc_noir_trigger_ptr = (MX_MBC_NOIR_TRIGGER *)
					pulser->record->record_type_struct;

	if ( mbc_noir_trigger_ptr == (MX_MBC_NOIR_TRIGGER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MBC_NOIR_TRIGGER pointer for pulse generator "
			"record '%s' passed by '%s' is NULL",
				pulser->record->name, calling_fname );
	}

	if ( mbc_noir_trigger != (MX_MBC_NOIR_TRIGGER **) NULL ) {
		*mbc_noir_trigger = mbc_noir_trigger_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_mbc_noir_trigger_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_mbc_noir_trigger_create_record_structures()";

	MX_PULSE_GENERATOR *pulser;
	MX_MBC_NOIR_TRIGGER *mbc_noir_trigger;

	/* Allocate memory for the necessary structures. */

	pulser = (MX_PULSE_GENERATOR *) malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PULSE_GENERATOR structure." );
	}

	mbc_noir_trigger = (MX_MBC_NOIR_TRIGGER *)
				malloc( sizeof(MX_MBC_NOIR_TRIGGER) );

	if ( mbc_noir_trigger == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MBC_NOIR_TRIGGER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = pulser;
	record->record_type_struct = mbc_noir_trigger;
	record->class_specific_function_list
			= &mxd_mbc_noir_trigger_pulser_function_list;

	pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_trigger_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_mbc_noir_trigger_finish_record_initialization()";

	MX_PULSE_GENERATOR *pulser;
	MX_MBC_NOIR_TRIGGER *mbc_noir_trigger;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_mbc_noir_trigger_get_pointers( pulser,
						&mbc_noir_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(mbc_noir_trigger->command_pv),
			"%sNOIR:command", mbc_noir_trigger->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir_trigger->command_trig_pv),
			"%sNOIR:commandTrig", mbc_noir_trigger->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir_trigger->expose_pv),
			"%sCOLLECT:expose", mbc_noir_trigger->epics_prefix );

	mx_epics_pvname_init( &(mbc_noir_trigger->shutter_pv),
			"%sgsc:Shutter1", mbc_noir_trigger->epics_prefix );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_trigger_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_mbc_noir_trigger_open()";

	MX_PULSE_GENERATOR *pulser;
	MX_MBC_NOIR_TRIGGER *mbc_noir_trigger;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_mbc_noir_trigger_get_pointers( pulser,
						&mbc_noir_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mbc_noir_trigger->exposure_in_progress = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_trigger_is_busy( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_mbc_noir_trigger_is_busy()";

	MX_MBC_NOIR_TRIGGER *mbc_noir_trigger;
	int32_t shutter_status;
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_trigger_get_pointers( pulser,
						&mbc_noir_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mbc_noir_trigger->exposure_in_progress == FALSE ) {
		pulser->busy = FALSE;

		shutter_status = -1;
	} else {
		mx_status = mx_caget( &(mbc_noir_trigger->shutter_pv),
					MX_CA_LONG, 1, &shutter_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( shutter_status ) {
			pulser->busy = TRUE;
		} else {
			pulser->busy = FALSE;
		}
	}

#if MXD_MBC_NOIR_TRIGGER_DEBUG
	MX_DEBUG(-2,
		("%s: Pulse generator '%s', shutter_status = %ld, busy = %d",
		fname, pulser->record->name, shutter_status, pulser->busy));
#endif

	return mx_status;
}

static mx_status_type
mxd_mbc_noir_trigger_send_command( MX_MBC_NOIR_TRIGGER *mbc_noir_trigger,
							char *command )
{
	char epics_string[ MXU_EPICS_STRING_LENGTH+1 ];
	int32_t command_trigger;
	mx_status_type mx_status;

	strlcpy( epics_string, command, sizeof(epics_string) );

	mx_status = mx_caput( &(mbc_noir_trigger->command_pv),
				MX_CA_STRING, 1, epics_string );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	command_trigger = 0;

	mx_status = mx_caput( &(mbc_noir_trigger->command_trig_pv),
				MX_CA_LONG, 1, &command_trigger );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_trigger_start( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_mbc_noir_trigger_start()";

	MX_MBC_NOIR_TRIGGER *mbc_noir_trigger;
	double exposure_seconds;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_trigger_get_pointers( pulser,
						&mbc_noir_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	exposure_seconds = pulser->pulse_width;

#if MXD_MBC_NOIR_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Pulse generator '%s' starting for %g seconds.",
		fname, pulser->record->name, exposure_seconds));
#endif
	/* Prepare the MBC beamline for an exposure. */

	mx_status = mxd_mbc_noir_trigger_send_command(
						mbc_noir_trigger, "init" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caput( &(mbc_noir_trigger->expose_pv),
				MX_CA_DOUBLE, 1, &exposure_seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the exposure. */

	mx_status = mxd_mbc_noir_trigger_send_command(
						mbc_noir_trigger, "snap" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	/* FIXME: This should not be necessary, but the procedure
	 *        does not work without it.  Note that 'caput' must
	 *        be in the PATH for this to work.
	 */

	snprintf( command, sizeof(command), "caput %sCOLLECT:commandTrig 0",
					mbc_noir_trigger->epics_prefix );

#if MXD_MBC_NOIR_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: executing command '%s'", fname, command));
#endif

	system( command );
#endif

	mbc_noir_trigger->exposure_in_progress = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_trigger_stop( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_mbc_noir_trigger_stop()";

	MX_MBC_NOIR_TRIGGER *mbc_noir_trigger;
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_trigger_get_pointers( pulser,
						&mbc_noir_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s: Stopping pulse generator '%s'.",
		fname, pulser->record->name ));
#endif

	mbc_noir_trigger->exposure_in_progress = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_trigger_get_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_mbc_noir_trigger_get_parameter()";

	MX_MBC_NOIR_TRIGGER *mbc_noir_trigger = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mbc_noir_trigger_get_pointers( pulser,
						&mbc_noir_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_TRIGGER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_FUNCTION_MODE:
		break;
	case MXLV_PGN_NUM_PULSES:
		break;
	case MXLV_PGN_PULSE_WIDTH:
		break;
	case MXLV_PGN_PULSE_DELAY:
		break;
	case MXLV_PGN_PULSE_PERIOD:
		break;
	case MXLV_PGN_TRIGGER_MODE:
		break;
	default:
		return
		    mx_pulse_generator_default_get_parameter_handler( pulser );
	}

#if MXD_MBC_NOIR_TRIGGER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mbc_noir_trigger_set_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_mbc_noir_trigger_set_parameter()";

	MX_MBC_NOIR_TRIGGER *mbc_noir_trigger;
	mx_status_type mx_status;

	mbc_noir_trigger = NULL;

	mx_status = mxd_mbc_noir_trigger_get_pointers( pulser,
						&mbc_noir_trigger, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MBC_NOIR_TRIGGER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_FUNCTION_MODE:
		pulser->function_mode = MXF_PGN_SQUARE_WAVE;
		break;

	case MXLV_PGN_NUM_PULSES:
		pulser->num_pulses = 1;
		break;

	case MXLV_PGN_PULSE_WIDTH:
		/* This value is used by the start() routine above. */
		break;

	case MXLV_PGN_PULSE_DELAY:
		pulser->pulse_delay = 0;
		break;

	case MXLV_PGN_PULSE_PERIOD:
		pulser->pulse_period = pulser->pulse_width;
		break;

	default:
		return
		    mx_pulse_generator_default_set_parameter_handler( pulser );
	}

#if MXD_MBC_NOIR_TRIGGER_DEBUG
	MX_DEBUG( 2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}


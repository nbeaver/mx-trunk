/*
 * Name:    d_epics_pmac_biocat.c 
 *
 * Purpose: MX motor driver for controlling a Delta Tau PMAC motor through
 *          the BioCAT version of Tom Coleman's EPICS PMAC database.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EPICS_PMAC_BIOCAT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_motor.h"
#include "d_epics_pmac_biocat.h"

MX_RECORD_FUNCTION_LIST mxd_epics_pmac_biocat_record_function_list = {
	NULL,
	mxd_epics_pmac_biocat_create_record_structures,
	mxd_epics_pmac_biocat_finish_record_initialization
};

MX_MOTOR_FUNCTION_LIST mxd_epics_pmac_biocat_motor_function_list = {
	mxd_epics_pmac_biocat_motor_is_busy,
	mxd_epics_pmac_biocat_move_absolute,
	mxd_epics_pmac_biocat_get_position,
	NULL,
	mxd_epics_pmac_biocat_soft_abort,
	NULL,
	mxd_epics_pmac_biocat_positive_limit_hit,
	mxd_epics_pmac_biocat_negative_limit_hit,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_epics_pmac_biocat_get_status,
};

MX_RECORD_FIELD_DEFAULTS mxd_epics_pmac_biocat_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_EPICS_PMAC_BIOCAT_STANDARD_FIELDS
};

long mxd_epics_pmac_biocat_num_record_fields
	= sizeof( mxd_epics_pmac_biocat_record_field_defaults )
		/ sizeof( mxd_epics_pmac_biocat_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_pmac_biocat_rfield_def_ptr
			= &mxd_epics_pmac_biocat_record_field_defaults[0];

/* === */

static mx_status_type
mxd_epics_pmac_biocat_get_pointers( MX_MOTOR *motor,
			MX_EPICS_PMAC_BIOCAT **epics_pmac_biocat,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_pmac_biocat_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( epics_pmac_biocat == (MX_EPICS_PMAC_BIOCAT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PMAC_BIOCAT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MOTOR pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*epics_pmac_biocat = 
		(MX_EPICS_PMAC_BIOCAT *) motor->record->record_type_struct;

	if ( *epics_pmac_biocat == (MX_EPICS_PMAC_BIOCAT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_PMAC_BIOCAT pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_epics_pmac_biocat_create_record_structures()";

	MX_MOTOR *motor;
	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	epics_pmac_biocat = (MX_EPICS_PMAC_BIOCAT *)
				malloc( sizeof(MX_EPICS_PMAC_BIOCAT) );

	if ( epics_pmac_biocat == (MX_EPICS_PMAC_BIOCAT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_PMAC_BIOCAT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = epics_pmac_biocat;
	record->class_specific_function_list
				= &mxd_epics_pmac_biocat_motor_function_list;

	motor->record = record;

	/* An EPICS-base PMAC BioCAT motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_epics_pmac_biocat_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat;
	MX_CLOCK_TICK current_tick;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	motor = record->record_class_struct;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

	epics_pmac_biocat->motion_state = MXF_EPB_NO_MOVE_IN_PROGRESS;

	current_tick = mx_current_clock_tick();

	epics_pmac_biocat->end_of_start_delay = current_tick;
	epics_pmac_biocat->end_of_end_delay = current_tick;

	epics_pmac_biocat->start_delay_ticks =
	    mx_convert_seconds_to_clock_ticks( epics_pmac_biocat->start_delay );

	epics_pmac_biocat->end_delay_ticks =
	    mx_convert_seconds_to_clock_ticks( epics_pmac_biocat->end_delay );

	/*---*/

	mx_epics_pvname_init( &(epics_pmac_biocat->abort_pv),
		"%s%s%sAbort", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->assembly_name );

	mx_epics_pvname_init( &(epics_pmac_biocat->actpos_pv),
		"%s%s%sActPos", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

	mx_epics_pvname_init( &(epics_pmac_biocat->ampena_pv),
		"%s%s%sAmpEna", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

	mx_epics_pvname_init( &(epics_pmac_biocat->nglimset_pv),
		"%s%s%sNgLimSet", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

	mx_epics_pvname_init( &(epics_pmac_biocat->pslimset_pv),
		"%s%s%sPsLimSet", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

	mx_epics_pvname_init( &(epics_pmac_biocat->rqspos_pv),
		"%s%s%sRqsPos", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

	mx_epics_pvname_init( &(epics_pmac_biocat->runprg_pv),
		"%s%s%sRunPrg", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->assembly_name );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_pmac_biocat_motor_is_busy()";

	MX_CLOCK_TICK current_tick;
	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat = NULL;
	long runprg_value;
	int comparison;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->busy = FALSE;

	current_tick = mx_current_clock_tick();

#if MXD_EPICS_PMAC_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s: motor '%s' initial motion_state = %ld",
		fname, motor->record->name, epics_pmac_biocat->motion_state));
#endif

	switch( epics_pmac_biocat->motion_state ) {
	case MXF_EPB_NO_MOVE_IN_PROGRESS:
		break;

	case MXF_EPB_START_DELAY_IN_PROGRESS:
		/* Has the start delay expired yet? */

		comparison = mx_compare_clock_ticks( current_tick,
					epics_pmac_biocat->end_of_start_delay );

		/* If not, unconditionally return with motor->busy == TRUE. */

		if ( comparison < 0 ) {
#if MXD_EPICS_PMAC_BIOCAT_DEBUG
			MX_DEBUG(-2,
			("%s: start delay still in progress.", fname));
#endif

			motor->busy = TRUE;

			return MX_SUCCESSFUL_RESULT;
		}

		/* Otherwise, change the motion state to move in progress. */

		epics_pmac_biocat->motion_state = MXF_EPB_MOVE_IN_PROGRESS;
		break;

	case MXF_EPB_MOVE_IN_PROGRESS:
		break;

	case MXF_EPB_END_DELAY_IN_PROGRESS:
		/* Has the end delay expired yet? */

		comparison = mx_compare_clock_ticks( current_tick,
					epics_pmac_biocat->end_of_end_delay );

		/* If not, unconditionally return with motor->busy == TRUE. */

		if ( comparison < 0 ) {
#if MXD_EPICS_PMAC_BIOCAT_DEBUG
			MX_DEBUG(-2,
			("%s: end delay still in progress.", fname));
#endif

			motor->busy = TRUE;

			return MX_SUCCESSFUL_RESULT;
		}

		/* Otherwise, change the motion state to no move in progress. */

		epics_pmac_biocat->motion_state = MXF_EPB_NO_MOVE_IN_PROGRESS;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"MX '%s' motor '%s' has illegal motion state %ld",
			mx_get_driver_name( motor->record ),
			motor->record->name,
			epics_pmac_biocat->motion_state );
		break;
	}

#if MXD_EPICS_PMAC_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s: motor '%s' modified motion_state = %ld",
		fname, motor->record->name, epics_pmac_biocat->motion_state));
#endif

	/* If we get here, ask EPICS for the 'run program' state. */

	mx_status = mx_caget( &(epics_pmac_biocat->runprg_pv),
				MX_CA_LONG, 1, &runprg_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_PMAC_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s: motor '%s' runprg = %ld",
		fname, motor->record->name, runprg_value));
#endif

	switch ( runprg_value ) {
	case 1:
		motor->busy = TRUE;
		break;
	case 0:
		motor->busy = FALSE;
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Run program record '%s' "
			"for EPICS PMAC BioCAT motor '%s' "
			"had an unexpected value of %ld",
			epics_pmac_biocat->runprg_pv.pvname,
			motor->record->name, runprg_value );

		break;
	}

	/* If 'in_position' says we are not moving, but we previously _were_
	 * moving, then if an end delay is specified, we must begin the
	 * end delay.
	 */

	if ( ( epics_pmac_biocat->motion_state == MXF_EPB_MOVE_IN_PROGRESS )
	  && ( motor->busy == FALSE )
	  && ( epics_pmac_biocat->end_delay > 0.0 ) )
	{
#if MXD_EPICS_PMAC_BIOCAT_DEBUG
		MX_DEBUG(-2,("%s: motor '%s', end delay is starting.",
			fname, motor->record->name ));
#endif

		current_tick = mx_current_clock_tick();

		epics_pmac_biocat->motion_state = MXF_EPB_END_DELAY_IN_PROGRESS;

		motor->busy = TRUE;

		epics_pmac_biocat->end_of_end_delay = mx_add_clock_ticks(
				current_tick, epics_pmac_biocat->end_delay_ticks );
	}

#if MXD_EPICS_PMAC_BIOCAT_DEBUG
	MX_DEBUG(-2,
		("%s: motor '%s' final motion_state = %ld, motor->busy = %d",
		fname, motor->record->name, epics_pmac_biocat->motion_state,
		(int) motor->busy ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_pmac_biocat_move_absolute()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat;
	double rqspos_value;
	MX_CLOCK_TICK current_tick;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rqspos_value = motor->raw_destination.analog;

	mx_status = mx_caput( &(epics_pmac_biocat->rqspos_pv),
				MX_CA_DOUBLE, 1, &rqspos_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_pmac_biocat->motion_state = MXF_EPB_START_DELAY_IN_PROGRESS;

	current_tick = mx_current_clock_tick();

	epics_pmac_biocat->end_of_start_delay = mx_add_clock_ticks(
			current_tick, epics_pmac_biocat->start_delay_ticks );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_pmac_biocat_get_position()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat;
	double actpos_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_pmac_biocat->actpos_pv),
				MX_CA_DOUBLE, 1, &actpos_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = actpos_value;
	
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_pmac_biocat_soft_abort()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat;
	long abort_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	abort_value = motor->soft_abort;

	mx_status = mx_caput( &(epics_pmac_biocat->abort_pv),
				MX_CA_LONG, 1, &abort_value );

	epics_pmac_biocat->motion_state = MXF_EPB_MOVE_IN_PROGRESS;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_epics_pmac_biocat_positive_limit_hit()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat;
	double pslimset_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_pmac_biocat->pslimset_pv),
				MX_CA_DOUBLE, 1, &pslimset_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fabs(pslimset_value) < 0.1 ) {
		motor->positive_limit_hit = FALSE;
	} else {
		motor->positive_limit_hit = TRUE;
	}
	
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_epics_pmac_biocat_negative_limit_hit()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat;
	double nglimset_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_pmac_biocat->nglimset_pv),
				MX_CA_DOUBLE, 1, &nglimset_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fabs(nglimset_value) < 0.1 ) {
		motor->negative_limit_hit = FALSE;
	} else {
		motor->negative_limit_hit = TRUE;
	}
	
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_pmac_biocat_get_status()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat;
	double ampena_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_PMAC_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s invoked for '%s'", fname, motor->record->name ));
#endif

	motor->status = 0;

	mx_status = mxd_epics_pmac_biocat_motor_is_busy( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->busy ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	mx_status = mxd_epics_pmac_biocat_positive_limit_hit( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->positive_limit_hit ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	mx_status = mxd_epics_pmac_biocat_negative_limit_hit( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->negative_limit_hit ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	mx_status = mx_caget( &(epics_pmac_biocat->ampena_pv),
				MX_CA_DOUBLE, 1, &ampena_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fabs(ampena_value) < 0.1 ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

#if MXD_EPICS_PMAC_BIOCAT_DEBUG
	MX_DEBUG(-2,("%s complete for '%s': status = %#0lx",
		fname, motor->record->name, motor->status ));
#endif

	return MX_SUCCESSFUL_RESULT;
}


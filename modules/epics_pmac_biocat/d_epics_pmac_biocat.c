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
 * Copyright 2011-2012 Illinois Institute of Technology
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
	mxd_epics_pmac_biocat_get_parameter,
	mxd_epics_pmac_biocat_set_parameter,
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
	epics_pmac_biocat->record = record;

	/* An EPICS-base PMAC BioCAT motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* We express accelerations in terms of the acceleration time. */

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

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

	mx_epics_pvname_init( &(epics_pmac_biocat->strcmd_pv),
		"%s%s%sStrCmd", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->assembly_name );

	mx_epics_pvname_init( &(epics_pmac_biocat->strrsp_pv),
		"%s%s%sStrRsp", epics_pmac_biocat->beamline_name,
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

	/* If 'runprg' says we are not moving, but we previously _were_
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

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_pmac_biocat_get_parameter()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat = NULL;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mxd_epics_pmac_biocat_get_motor_variable(
				epics_pmac_biocat, 22,
				MXFT_DOUBLE, &double_value,
				MXD_EPICS_PMAC_BIOCAT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Reported by the controller in count/msec. */

		motor->raw_speed = 1000.0 * double_value;
		break;
	case MXLV_MTR_BASE_SPEED:
		/* The PMAC doesn't seem to have the concept of a base speed,
		 * since it is oriented to servo motors.
		 */

		motor->raw_base_speed = 0.0;

		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mxd_epics_pmac_biocat_get_motor_variable(
						epics_pmac_biocat, 19,
						MXFT_DOUBLE, &double_value,
						MXD_EPICS_PMAC_BIOCAT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		double_value = 1000000.0 * double_value;

		motor->raw_acceleration_parameters[0] = double_value;

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;
	case MXLV_MTR_AXIS_ENABLE:
		mx_status = mxd_epics_pmac_biocat_get_motor_variable( epics_pmac_biocat, 0,
						MXFT_BOOL, &double_value,
						MXD_EPICS_PMAC_BIOCAT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mx_round(double_value) != 0 ) {
			motor->axis_enable = TRUE;
		} else {
			motor->axis_enable = FALSE;
		}
		break;
	case MXLV_MTR_PROPORTIONAL_GAIN:
		mx_status = mxd_epics_pmac_biocat_get_motor_variable(
					epics_pmac_biocat, 30, MXFT_LONG,
					&(motor->proportional_gain),
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		mx_status = mxd_epics_pmac_biocat_get_motor_variable(
					epics_pmac_biocat, 33, MXFT_LONG,
					&(motor->integral_gain),
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		mx_status = mxd_epics_pmac_biocat_get_motor_variable(
					epics_pmac_biocat, 31, MXFT_LONG,
					&(motor->derivative_gain),
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		mx_status = mxd_epics_pmac_biocat_get_motor_variable(
					epics_pmac_biocat, 32, MXFT_LONG,
					&(motor->velocity_feedforward_gain),
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		mx_status = mxd_epics_pmac_biocat_get_motor_variable(
					epics_pmac_biocat, 35, MXFT_LONG,
					&(motor->acceleration_feedforward_gain),
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_INTEGRAL_LIMIT:
		mx_status = mxd_epics_pmac_biocat_get_motor_variable(
					epics_pmac_biocat, 34, MXFT_LONG,
					&(motor->integral_limit),
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	default:
		mx_status = mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return mx_status;
}

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_pmac_biocat_set_parameter()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat = NULL;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		double_value = 0.001 * motor->raw_speed;

		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 22,
					MXFT_DOUBLE, double_value,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_BASE_SPEED:
		/* The PMAC doesn't seem to have the concept of a base speed,
		 * since it is oriented to servo motors.
		 */

		mx_status = MX_SUCCESSFUL_RESULT;
		break;

#if 0
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		/* These acceleration parameters are chosen such that the
		 * value of Ixx19 is used as counts/msec**2.
		 */

		double_value = motor->raw_acceleration_parameters[0];

		double_value = 1.0e-6 * double_value;

		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 19,
					MXFT_DOUBLE, double_value,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set Ixx20 (jog acceleration time) to 1.0 msec. */

		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 20,
					MXFT_DOUBLE, 1.0,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set Ixx21 (jog acceleration S-curve time) to 0. */

		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 21,
					MXFT_DOUBLE, 0.0,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
#endif

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = TRUE;
		}

		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 0,
					MXFT_BOOL, motor->axis_enable,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_CLOSED_LOOP:
		if ( motor->closed_loop ) {
			mx_status = mxd_epics_pmac_biocat_jog_command(
					epics_pmac_biocat, "J/", NULL, 0,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		} else {
			mx_status = mxd_epics_pmac_biocat_jog_command(
					epics_pmac_biocat, "K", NULL, 0,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		}
		break;
	case MXLV_MTR_FAULT_RESET:
		mx_status = mxd_epics_pmac_biocat_jog_command(
					epics_pmac_biocat, "J/", NULL, 0,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
#if 0
	case MXLV_MTR_PROPORTIONAL_GAIN:
		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 30, MXFT_LONG,
					motor->proportional_gain,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 33, MXFT_LONG,
					motor->integral_gain,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 31, MXFT_LONG,
					motor->derivative_gain,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 32, MXFT_LONG,
					motor->velocity_feedforward_gain,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 35, MXFT_LONG,
					motor->acceleration_feedforward_gain,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
	case MXLV_MTR_INTEGRAL_LIMIT:
		long_value = mx_round( motor->integral_limit );

		if ( long_value ) {
			long_value = 1;
		}

		mx_status = mxd_epics_pmac_biocat_set_motor_variable(
					epics_pmac_biocat, 34, MXFT_LONG,
					long_value,
					MXD_EPICS_PMAC_BIOCAT_DEBUG );
		break;
#endif
	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

/*------------------------------------------------------------------------*/

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

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_command( MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat,
				char *command,
				char *response,
				size_t max_response_length,
				int debug_flag )
{
	static const char fname[] = "mxd_epics_pmac_biocat_command()";

	mx_status_type mx_status;

	if ( epics_pmac_biocat == (MX_EPICS_PMAC_BIOCAT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PMAC_BIOCAT pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command string pointer passed for '%s' was NULL.",
			epics_pmac_biocat->record->name );
	}
	if ( (response != (char *) NULL )
	  && (max_response_length <= 40) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The specified length %lu of the response buffer for '%s' "
		"is shorter than the minimum length of 41.",
			(unsigned long) max_response_length,
			epics_pmac_biocat->record->name );
	}

	mx_status = mx_caput( &(epics_pmac_biocat->strcmd_pv),
				MX_CA_STRING, 1, command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response != (char *) NULL ) {
		mx_status = mx_caget( &(epics_pmac_biocat->strrsp_pv),
				MX_CA_STRING, 1, response );
	}

	return mx_status;
}

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_jog_command( MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat,
				char *command,
				char *response,
				size_t response_buffer_length,
				int debug_flag )
{
	static const char fname[] = "mxd_epics_pmac_biocat_jog_command()";

	char command_buffer[100];
	mx_status_type mx_status;

	if ( epics_pmac_biocat == (MX_EPICS_PMAC_BIOCAT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PMAC_BIOCAT pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command string pointer passed for motor '%s' was NULL.",
			epics_pmac_biocat->record->name );
	}

	snprintf( command_buffer, sizeof(command_buffer),
			"#%ld%s", epics_pmac_biocat->motor_number, command );

	mx_status = mxd_epics_pmac_biocat_command( epics_pmac_biocat,
						command_buffer,
						response,
						response_buffer_length,
						debug_flag );

	return mx_status;
}

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_get_motor_variable(
				MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat,
				long variable_number,
				long variable_type,
				double *double_ptr,
				int debug_flag )
{
	static const char fname[] =
			"mxd_epics_pmac_biocat_get_motor_variable()";

	char command_buffer[100];
	char response[100];
	int num_items;
	long long_value;
	mx_status_type mx_status;

	if ( epics_pmac_biocat == (MX_EPICS_PMAC_BIOCAT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PMAC_BIOCAT pointer passed was NULL." );
	}
	if ( double_ptr == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The value pointer passed for motor '%s' was NULL.",
			epics_pmac_biocat->record->name );
	}

	snprintf( command_buffer, sizeof(command_buffer),
		"I%ld%02ld", epics_pmac_biocat->motor_number, variable_number );

	mx_status = mxd_epics_pmac_biocat_command( epics_pmac_biocat,
						command_buffer,
						response,
						sizeof(response),
						debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the returned value. */

	switch( variable_type ) {
	case MXFT_LONG:
		num_items = sscanf( response, "%ld", &long_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot parse response to command '%s' as a long integer.  Response = '%s'",
				command_buffer, response );
		}

		*double_ptr = (double) long_value;
		break;
	case MXFT_BOOL:
		num_items = sscanf( response, "%ld", &long_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot parse response to command '%s' as a long integer.  Response = '%s'",
				command_buffer, response );
		}

		if ( long_value != 0 ) {
			*double_ptr = 1.0;
		} else {
			*double_ptr = 0.0;
		}
		break;
	case MXFT_DOUBLE:
		num_items = sscanf( response, "%lg", double_ptr );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot parse response to command '%s' as a double.  Response = '%s'",
				command_buffer, response );
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG, MXFT_BOOL, and MXFT_DOUBLE are supported." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_set_motor_variable(
				MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat,
				long variable_number,
				long variable_type,
				double double_value,
				int debug_flag )
{
	static const char fname[] =
				"mxd_epics_pmac_biocat_set_motor_variable()";

	char command_buffer[100];
	char response[100];
	long long_value;
	mx_status_type mx_status;

	if ( epics_pmac_biocat == (MX_EPICS_PMAC_BIOCAT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_PMAC_BIOCAT pointer passed was NULL." );
	}

	switch( variable_type ) {
	case MXFT_LONG:
	case MXFT_BOOL:
		long_value = mx_round( double_value );

		if ( (variable_type == MXFT_BOOL)
		  && (long_value != 0) )
		{
			long_value = 1;
		}

		snprintf( command_buffer, sizeof(command_buffer),
				"I%ld%02ld=%ld",
				epics_pmac_biocat->motor_number,
				variable_number,
				long_value );
		break;
	case MXFT_DOUBLE:
		snprintf( command_buffer, sizeof(command_buffer),
				"I%ld%02ld=%f",
				epics_pmac_biocat->motor_number,
				variable_number,
				double_value );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG, MXFT_BOOL, and MXFT_DOUBLE are supported." );
	}

	mx_status = mxd_epics_pmac_biocat_command( epics_pmac_biocat,
						command_buffer,
						response,
						sizeof(response),
						debug_flag );

	return mx_status;
}


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
 * Copyright 2011-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EPICS_PMAC_BIOCAT_DEBUG	FALSE

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
	mxd_epics_pmac_biocat_raw_home_command,
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

/*------------------------------------------------------------------------*/

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

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

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

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

	/*---*/

	/* FIXME: Surely there is a better way of determining whether
	 * or not a given motor is a raw axis than by looking at the
	 * first letter of its device name.
	 *
	 * Perhaps one could look at the DESC field for the motor,
	 * but that would require Channel Access I/O, which we want
	 * to avoid unless necessary.
	 */

	if ( epics_pmac_biocat->device_name[0] == 'm' ) {
		epics_pmac_biocat->is_raw_motor = TRUE;
	} else {
		epics_pmac_biocat->is_raw_motor = FALSE;
	}

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

	if ( epics_pmac_biocat->is_raw_motor ) {

		mx_epics_pvname_init( &(epics_pmac_biocat->home_pv),
		"%s%s%sHome", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->opnlpmd_pv),
		"%s%s%sOpnLpMd", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		/*----*/

		mx_epics_pvname_init( &(epics_pmac_biocat->ix20_fan_pv),
		"%s%s%sIx20:FAN.PROC", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix30_fan_pv),
		"%s%s%sIx30:FAN.PROC", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		/*----*/

		mx_epics_pvname_init( &(epics_pmac_biocat->ix20_li_pv),
		"%s%s%sIx20:LI", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix20_lo_pv),
		"%s%s%sIx20:LO", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix22_ai_pv),
		"%s%s%sIx22:AI", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix22_ao_pv),
		"%s%s%sIx22:AO", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix30_li_pv),
		"%s%s%sIx30:LI", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix30_lo_pv),
		"%s%s%sIx30:LO", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix31_li_pv),
		"%s%s%sIx31:LI", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix31_lo_pv),
		"%s%s%sIx31:LO", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix32_li_pv),
		"%s%s%sIx32:LI", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix32_lo_pv),
		"%s%s%sIx32:LO", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix33_li_pv),
		"%s%s%sIx33:LI", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix33_lo_pv),
		"%s%s%sIx33:LO", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix34_bi_pv),
		"%s%s%sIx34:BI", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix34_bo_pv),
		"%s%s%sIx34:BO", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix35_li_pv),
		"%s%s%sIx35:LI", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

		mx_epics_pvname_init( &(epics_pmac_biocat->ix35_lo_pv),
		"%s%s%sIx35:LO", epics_pmac_biocat->beamline_name,
				epics_pmac_biocat->component_name,
				epics_pmac_biocat->device_name );

	}

	/* The following is used by the 'epics_scaler_mce' driver. */

	epics_pmac_biocat->epics_position_pv_ptr =
			(char *) &(epics_pmac_biocat->actpos_pv);

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_pmac_biocat_motor_is_busy()";

	MX_CLOCK_TICK current_tick;
	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat = NULL;
	int32_t runprg_value;
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
			motor->record->name, (long) runprg_value );

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

/*------------------------------------------------------------------------*/

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

/*------------------------------------------------------------------------*/

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

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_pmac_biocat_soft_abort()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat;
	int32_t abort_value;
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

/*------------------------------------------------------------------------*/

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

/*------------------------------------------------------------------------*/

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
mxd_epics_pmac_biocat_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_epics_pmac_biocat_raw_home_command()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat;
	long home_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( epics_pmac_biocat->is_raw_motor == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	home_value = 0;

	mx_status = mx_caget( &(epics_pmac_biocat->home_pv),
				MX_CA_DOUBLE, 1, &home_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

static mx_status_type
mxp_epics_pmac_biocat_update_motor_readbacks(
			MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat )
{
	mx_status_type mx_status;

	int32_t ix20_fanout_value = 1;

	mx_status = mx_caput( &(epics_pmac_biocat->ix20_fan_pv),
				MX_CA_LONG, 1, &ix20_fanout_value );

	return mx_status;
}

/*----*/

static mx_status_type
mxp_epics_pmac_biocat_update_servo_readbacks(
			MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat )
{
	mx_status_type mx_status;

	int32_t ix30_fanout_value = 1;

	mx_status = mx_caput( &(epics_pmac_biocat->ix20_fan_pv),
				MX_CA_LONG, 1, &ix30_fanout_value );

	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mxd_epics_pmac_biocat_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_pmac_biocat_get_parameter()";

	MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat = NULL;
	int32_t int32_value;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( epics_pmac_biocat->is_raw_motor == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mxp_epics_pmac_biocat_update_motor_readbacks(
							epics_pmac_biocat );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_pmac_biocat->ix22_ai_pv),
					MX_CA_DOUBLE, 1, &double_value );

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
		mx_status = mxp_epics_pmac_biocat_update_motor_readbacks(
							epics_pmac_biocat );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_pmac_biocat->ix20_li_pv),
					MX_CA_LONG, 1, &int32_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_acceleration_parameters[0] = 
					1000000.0 * (double) int32_value;

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;
	case MXLV_MTR_AXIS_ENABLE:
		mx_status = mx_caget( &(epics_pmac_biocat->ampena_pv),
					MX_CA_LONG, 1, &int32_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( int32_value != 0 ) {
			motor->axis_enable = TRUE;
		} else {
			motor->axis_enable = FALSE;
		}
		break;
	case MXLV_MTR_CLOSED_LOOP:
		mx_status = mx_caget( &(epics_pmac_biocat->opnlpmd_pv),
					MX_CA_LONG, 1, &int32_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( int32_value == 0 ) {
			motor->closed_loop = TRUE;
		} else {
			motor->closed_loop = TRUE;
		}
		break;
	case MXLV_MTR_PROPORTIONAL_GAIN:
		mx_status = mxp_epics_pmac_biocat_update_servo_readbacks(
							epics_pmac_biocat );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_pmac_biocat->ix30_li_pv),
					MX_CA_LONG, 1, &int32_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->proportional_gain = (double) int32_value;
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		mx_status = mxp_epics_pmac_biocat_update_servo_readbacks(
							epics_pmac_biocat );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_pmac_biocat->ix33_li_pv),
					MX_CA_LONG, 1, &int32_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->integral_gain = (double) int32_value;
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		mx_status = mxp_epics_pmac_biocat_update_servo_readbacks(
							epics_pmac_biocat );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_pmac_biocat->ix31_li_pv),
					MX_CA_LONG, 1, &int32_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->derivative_gain = (double) int32_value;
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		mx_status = mxp_epics_pmac_biocat_update_servo_readbacks(
							epics_pmac_biocat );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_pmac_biocat->ix32_li_pv),
					MX_CA_LONG, 1, &int32_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->velocity_feedforward_gain = (double) int32_value;
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		mx_status = mxp_epics_pmac_biocat_update_servo_readbacks(
							epics_pmac_biocat );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_pmac_biocat->ix35_li_pv),
					MX_CA_LONG, 1, &int32_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->acceleration_feedforward_gain = (double) int32_value;
		break;
	case MXLV_MTR_EXTRA_GAIN:
		mx_status = mxp_epics_pmac_biocat_update_servo_readbacks(
							epics_pmac_biocat );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_pmac_biocat->ix34_bi_pv),
					MX_CA_LONG, 1, &int32_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( int32_value == 0 ) {
			motor->extra_gain = 0;
		} else {
			motor->extra_gain = 1;
		}
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
	int32_t int32_value;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_pmac_biocat_get_pointers( motor,
						&epics_pmac_biocat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( epics_pmac_biocat->is_raw_motor == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		double_value = 0.001 * motor->raw_speed;

		mx_status = mx_caput( &(epics_pmac_biocat->ix22_ao_pv),
				MXFT_DOUBLE, 1, &double_value );
		break;
	case MXLV_MTR_BASE_SPEED:
		/* The PMAC doesn't seem to have the concept of a base speed,
		 * since it is oriented to servo motors.
		 */

		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		int32_value = mx_round( motor->raw_acceleration_parameters[0] );

		int32_value = 1000000L * int32_value;

		mx_status = mx_caput( &(epics_pmac_biocat->ix20_lo_pv),
					MXFT_LONG, 1, &int32_value );
		break;
	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			int32_value = 1;
		} else {
			int32_value = 0;
		}

		mx_status = mx_caput( &(epics_pmac_biocat->ampena_pv),
					MXFT_LONG, 1, &int32_value );
		break;
	case MXLV_MTR_CLOSED_LOOP:
		if ( motor->closed_loop ) {
			int32_value = 0;
		} else {
			int32_value = 1;
		}

		mx_status = mx_caput( &(epics_pmac_biocat->opnlpmd_pv),
					MXFT_LONG, 1, &int32_value );
		break;
	case MXLV_MTR_FAULT_RESET:
		/* FIXME - The jog mode command would be J/  */

		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Fault reset is not yet implemented for motor '%s'.",
					motor->record->name );
		break;
	case MXLV_MTR_PROPORTIONAL_GAIN:
		int32_value = mx_round( motor->proportional_gain );

		mx_status = mx_caput( &(epics_pmac_biocat->ix30_lo_pv),
					MXFT_LONG, 1, &int32_value );
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		int32_value = mx_round( motor->integral_gain );

		mx_status = mx_caput( &(epics_pmac_biocat->ix33_lo_pv),
					MXFT_LONG, 1, &int32_value );
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		int32_value = mx_round( motor->derivative_gain );

		mx_status = mx_caput( &(epics_pmac_biocat->ix31_lo_pv),
					MXFT_LONG, 1, &int32_value );
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		int32_value = mx_round( motor->velocity_feedforward_gain );

		mx_status = mx_caput( &(epics_pmac_biocat->ix32_lo_pv),
					MXFT_LONG, 1, &int32_value );
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		int32_value = mx_round( motor->acceleration_feedforward_gain );

		mx_status = mx_caput( &(epics_pmac_biocat->ix35_lo_pv),
					MXFT_LONG, 1, &int32_value );
		break;
	case MXLV_MTR_EXTRA_GAIN:
		int32_value = mx_round( motor->extra_gain );

		if ( int32_value ) {
			int32_value = 1;
		}

		mx_status = mx_caput( &(epics_pmac_biocat->ix34_bo_pv),
					MXFT_LONG, 1, &int32_value );
		break;
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


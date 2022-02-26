/*
 * Name:    d_pmac_tc.c 
 *
 * Purpose: MX motor driver for controlling a Delta Tau PMAC motor
 *          through Tom Coleman's EPICS PMAC dual-ported RAM interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2004, 2006, 2008-2011, 2014, 2021
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PMACTC_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_motor.h"
#include "d_pmac_tc.h"

MX_RECORD_FUNCTION_LIST mxd_pmac_tc_motor_record_function_list = {
	NULL,
	mxd_pmac_tc_motor_create_record_structures,
	mxd_pmac_tc_motor_finish_record_initialization,
	NULL,
	mxd_pmac_tc_motor_print_structure
};

MX_MOTOR_FUNCTION_LIST mxd_pmac_tc_motor_motor_function_list = {
	mxd_pmac_tc_motor_motor_is_busy,
	mxd_pmac_tc_motor_move_absolute,
	mxd_pmac_tc_motor_get_position,
	NULL,
	mxd_pmac_tc_motor_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pmac_tc_motor_constant_velocity_move,
	mxd_pmac_tc_motor_get_parameter,
	mxd_pmac_tc_motor_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_pmac_tc_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PMAC_TC_MOTOR_STANDARD_FIELDS
};

long mxd_pmac_tc_motor_num_record_fields
		= sizeof( mxd_pmac_tc_motor_record_field_defaults )
			/ sizeof( mxd_pmac_tc_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmac_tc_motor_rfield_def_ptr
			= &mxd_pmac_tc_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_pmac_tc_motor_get_pointers( MX_MOTOR *motor,
			MX_PMAC_TC_MOTOR **pmac_tc_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmac_tc_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( pmac_tc_motor == (MX_PMAC_TC_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC_TC_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MOTOR pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*pmac_tc_motor = (MX_PMAC_TC_MOTOR *) motor->record->record_type_struct;

	if ( *pmac_tc_motor == (MX_PMAC_TC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PMAC_TC_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_pmac_tc_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pmac_tc_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_PMAC_TC_MOTOR *pmac_tc_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	pmac_tc_motor = (MX_PMAC_TC_MOTOR *) malloc( sizeof(MX_PMAC_TC_MOTOR) );

	if ( pmac_tc_motor == (MX_PMAC_TC_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_PMAC_TC_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = pmac_tc_motor;

	motor->record = record;

	/* A PMAC TC motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_tc_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pmac_tc_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_PMAC_TC_MOTOR *pmac_tc_motor = NULL;
	MX_DRIVER *driver;
	MX_CLOCK_TICK current_tick;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pmac_tc_motor_get_pointers( motor,
						&pmac_tc_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*--------*/

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

	driver = mx_get_driver_for_record( record );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Could not find the MX driver for record '%s'.", record->name );
	}

	if ( strcmp( driver->name, "pmac_tc_motor" ) == 0 ) {
		pmac_tc_motor->database_type = MXT_EPICS_PMAC_TC;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized EPICS PMAC type '%s' for record '%s'.  "
		"The allowed types are 'pmac_tc_motor'.",
			driver->name, record->name );
	}

	if ( pmac_tc_motor->database_type == MXT_EPICS_PMAC_TC ) {
		record->class_specific_function_list
				= &mxd_pmac_tc_motor_motor_function_list;
	}

	/*--------*/

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->acceleration_type = MXF_MTR_ACCEL_NONE;

	pmac_tc_motor->motion_state = MXF_PTC_NO_MOVE_IN_PROGRESS;

	current_tick = mx_current_clock_tick();

	pmac_tc_motor->end_of_start_delay = current_tick;
	pmac_tc_motor->end_of_end_delay = current_tick;

	pmac_tc_motor->start_delay_ticks =
		mx_convert_seconds_to_clock_ticks( pmac_tc_motor->start_delay );

	pmac_tc_motor->end_delay_ticks =
		mx_convert_seconds_to_clock_ticks( pmac_tc_motor->end_delay );

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(pmac_tc_motor->actual_position_pv),
				pmac_tc_motor->actual_position_record_name);

	mx_epics_pvname_init( &(pmac_tc_motor->requested_position_pv),
				pmac_tc_motor->requested_position_record_name);

	mx_epics_pvname_init( &(pmac_tc_motor->in_position_pv),
				pmac_tc_motor->in_position_record_name);

	mx_epics_pvname_init( &(pmac_tc_motor->abort_pv),
				pmac_tc_motor->abort_record_name);

	switch( pmac_tc_motor->database_type ) {
	case MXT_EPICS_PMAC_TC:
		mx_epics_pvname_init( &(pmac_tc_motor->strcmd_pv),
				"%s:StrCmd.VAL", pmac_tc_motor->pmac_name);

		mx_epics_pvname_init( &(pmac_tc_motor->strrsp_pv),
				"%s:StrRsp.VAL", pmac_tc_motor->pmac_name);
		break;
	}

	/* The following is used by the 'epics_scaler_mce' driver. */

	pmac_tc_motor->epics_position_pv_ptr =
			(char *) &(pmac_tc_motor->actual_position_pv);

#if MXD_PMACTC_DEBUG
	mx_epics_set_debug_flag( TRUE );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_tc_motor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_tc_motor_print_structure()";

	MX_MOTOR *motor;
	MX_PMAC_TC_MOTOR *pmac_tc_motor = NULL;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_pmac_tc_motor_get_pointers( motor,
						&pmac_tc_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	switch( pmac_tc_motor->database_type ) {
	case MXT_EPICS_PMAC_TC:
		fprintf(file,
		      "  Motor type                = PMAC_TC_MOTOR.\n\n");
		break;
	default:
		fprintf(file,
		      "  Motor type                = %ld (this is a bug).\n\n",
						record->mx_type );
		break;
	}

	fprintf(file, "  name                      = %s\n", record->name);
	fprintf(file, "  actual position record    = %s\n",
			pmac_tc_motor->actual_position_record_name);
	fprintf(file, "  requested position record = %s\n",
			pmac_tc_motor->requested_position_record_name);
	fprintf(file, "  in position record        = %s\n",
			pmac_tc_motor->in_position_record_name);
	fprintf(file, "  abort record              = %s\n",
			pmac_tc_motor->abort_record_name);

	fprintf(file, "  speed scale               = %g\n",
			pmac_tc_motor->speed_scale );
	fprintf(file, "  start delay               = %g sec\n",
			pmac_tc_motor->start_delay );
	fprintf(file, "  end delay                 = %g sec\n",
			pmac_tc_motor->end_delay );

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf( file, "  position                  = %g %s  (%g).\n",
		motor->position, motor->units,
		motor->raw_position.analog );
	fprintf( file, "  scale                     = %g %s per EPICS EGU.\n",
		motor->scale, motor->units );
	fprintf( file, "  offset                    = %g %s.\n",
		motor->offset, motor->units);
	fprintf( file, "  backlash                  = %g %s  (%g).\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	fprintf( file, "  negative limit            = %g %s  (%g).\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );
	fprintf( file, "  positive limit            = %g %s  (%g).\n",
	        motor->positive_limit, motor->units,
	        motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf( file, "  move deadband             = %g %s  (%g).\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_tc_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_tc_motor_motor_is_busy()";

	MX_CLOCK_TICK current_tick;
	MX_PMAC_TC_MOTOR *pmac_tc_motor = NULL;
	short in_position_value;
	int comparison;
	mx_status_type mx_status;

	mx_status = mxd_pmac_tc_motor_get_pointers( motor,
						&pmac_tc_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->busy = FALSE;

	current_tick = mx_current_clock_tick();

#if MXD_PMACTC_DEBUG
	MX_DEBUG(-2,("%s: motor '%s' initial motion_state = %ld",
		fname, motor->record->name, pmac_tc_motor->motion_state));
#endif

	switch( pmac_tc_motor->motion_state ) {
	case MXF_PTC_NO_MOVE_IN_PROGRESS:
		break;

	case MXF_PTC_START_DELAY_IN_PROGRESS:
		/* Has the start delay expired yet? */

		comparison = mx_compare_clock_ticks( current_tick,
					pmac_tc_motor->end_of_start_delay );

		/* If not, unconditionally return with motor->busy == TRUE. */

		if ( comparison < 0 ) {
#if MXD_PMACTC_DEBUG
			MX_DEBUG(-2,
			("%s: start delay still in progress.", fname));
#endif

			motor->busy = TRUE;

			return MX_SUCCESSFUL_RESULT;
		}

		/* Otherwise, change the motion state to move in progress. */

		pmac_tc_motor->motion_state = MXF_PTC_MOVE_IN_PROGRESS;
		break;

	case MXF_PTC_MOVE_IN_PROGRESS:
		break;

	case MXF_PTC_END_DELAY_IN_PROGRESS:
		/* Has the end delay expired yet? */

		comparison = mx_compare_clock_ticks( current_tick,
					pmac_tc_motor->end_of_end_delay );

		/* If not, unconditionally return with motor->busy == TRUE. */

		if ( comparison < 0 ) {
#if MXD_PMACTC_DEBUG
			MX_DEBUG(-2,
			("%s: end delay still in progress.", fname));
#endif

			motor->busy = TRUE;

			return MX_SUCCESSFUL_RESULT;
		}

		/* Otherwise, change the motion state to no move in progress. */

		pmac_tc_motor->motion_state = MXF_PTC_NO_MOVE_IN_PROGRESS;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"MX '%s' motor '%s' has illegal motion state %ld",
			mx_get_driver_name( motor->record ),
			motor->record->name,
			pmac_tc_motor->motion_state );
		break;
	}

#if MXD_PMACTC_DEBUG
	MX_DEBUG(-2,("%s: motor '%s' modified motion_state = %ld",
		fname, motor->record->name, pmac_tc_motor->motion_state));
#endif

	/* If we get here, ask EPICS for the 'in_position' state. */

	mx_status = mx_caget( &(pmac_tc_motor->in_position_pv),
				MX_CA_SHORT, 1, &in_position_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PMACTC_DEBUG
	MX_DEBUG(-2,("%s: motor '%s' in_position = %hd",
		fname, motor->record->name, in_position_value));
#endif

	switch ( in_position_value ) {
	case 0:
		motor->busy = TRUE;
		break;
	case 1:
		motor->busy = FALSE;
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"In position record '%s' for PMAC TC motor '%s' "
			"had an unexpected value of %d",
			pmac_tc_motor->in_position_pv.pvname,
			motor->record->name, in_position_value );

		break;
	}

	/* If 'in_position' says we are not moving, but we previously _were_
	 * moving, then if an end delay is specified, we must begin the
	 * end delay.
	 */

	if ( ( pmac_tc_motor->motion_state == MXF_PTC_MOVE_IN_PROGRESS )
	  && ( motor->busy == FALSE )
	  && ( pmac_tc_motor->end_delay > 0.0 ) )
	{
#if MXD_PMACTC_DEBUG
		MX_DEBUG(-2,("%s: motor '%s', end delay is starting.",
			fname, motor->record->name ));
#endif

		current_tick = mx_current_clock_tick();

		pmac_tc_motor->motion_state = MXF_PTC_END_DELAY_IN_PROGRESS;

		motor->busy = TRUE;

		pmac_tc_motor->end_of_end_delay = mx_add_clock_ticks(
				current_tick, pmac_tc_motor->end_delay_ticks );
	}

#if MXD_PMACTC_DEBUG
	MX_DEBUG(-2,
		("%s: motor '%s' final motion_state = %ld, motor->busy = %d",
		fname, motor->record->name, pmac_tc_motor->motion_state,
		(int) motor->busy ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_tc_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_tc_motor_move_absolute()";

	MX_CLOCK_TICK current_tick;
	MX_PMAC_TC_MOTOR *pmac_tc_motor = NULL;
	double new_destination;
	mx_status_type mx_status;

	mx_status = mxd_pmac_tc_motor_get_pointers( motor,
						&pmac_tc_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_destination = motor->raw_destination.analog;

	/* Send the move command. */

	mx_status = mx_caput( &(pmac_tc_motor->requested_position_pv),
				MX_CA_DOUBLE, 1, &new_destination );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pmac_tc_motor->motion_state = MXF_PTC_START_DELAY_IN_PROGRESS;

	current_tick = mx_current_clock_tick();

	pmac_tc_motor->end_of_start_delay = mx_add_clock_ticks( current_tick,
					pmac_tc_motor->start_delay_ticks );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_tc_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_tc_motor_get_position()";

	MX_PMAC_TC_MOTOR *pmac_tc_motor = NULL;
	double raw_position;
	mx_status_type mx_status;

	mx_status = mxd_pmac_tc_motor_get_pointers( motor,
						&pmac_tc_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(pmac_tc_motor->actual_position_pv),
				MX_CA_DOUBLE, 1, &raw_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = raw_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_tc_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_tc_motor_soft_abort()";

	MX_PMAC_TC_MOTOR *pmac_tc_motor = NULL;
	short abort_field;
	mx_status_type mx_status;

	mx_status = mxd_pmac_tc_motor_get_pointers( motor,
						&pmac_tc_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	abort_field = 1;

	mx_status = mx_caput( &(pmac_tc_motor->abort_pv),
				MX_CA_SHORT, 1, &abort_field );

	pmac_tc_motor->motion_state = MXF_PTC_MOVE_IN_PROGRESS;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_tc_motor_constant_velocity_move( MX_MOTOR *motor )
{
	mx_status_type mx_status;

	if ( motor->constant_velocity_move >= 0 ) {
		motor->raw_destination.analog = 10000;
	} else {
		motor->raw_destination.analog = -10000;
	}

	mx_status = mxd_pmac_tc_motor_move_absolute( motor );

	return mx_status;
}

/* Get and set parameter for SBC-CAT's version of the code. */

MX_EXPORT mx_status_type
mxd_pmac_tc_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_tc_motor_get_parameter()";

	MX_PMAC_TC_MOTOR *pmac_tc_motor = NULL;
	char command[41], response[41];
	int num_items;
	unsigned long pmac_speed;
	mx_status_type mx_status;

	mx_status = mxd_pmac_tc_motor_get_pointers( motor,
						&pmac_tc_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PMACTC_DEBUG
	MX_DEBUG(-2,
	("%s invoked for motor '%s' (type %ld) for parameter type '%s' (%ld).",
		fname, motor->record->name, motor->record->mx_type,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	/* Send the parameter request command to the StrCmd EPICS variable. */

	switch( motor->parameter_type ) {
	case MXLV_MTR_ACCELERATION_TYPE:
		motor->acceleration_type = MXF_MTR_ACCEL_NONE;
		break;
	case MXLV_MTR_SPEED:
		snprintf( command, sizeof(command),
			"I%ld16", pmac_tc_motor->motor_number );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Parameter type %ld is not supported by the driver for motor '%s'",
			motor->parameter_type, motor->record->name );
		break;
	}

	mx_status = mx_caput( &(pmac_tc_motor->strcmd_pv),
				MX_CA_STRING, 1, command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now read back the result from the StrRsp EPICS variable. */

	mx_status = mx_caget( &(pmac_tc_motor->strrsp_pv),
				MX_CA_STRING, 1, response );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		num_items = sscanf( response, "%lu", &pmac_speed );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse PMAC response '%s' for motor '%s'.",
				response, motor->record->name );
		}

		motor->raw_speed = (double) pmac_speed;
		break;
	}

#if MXD_PMACTC_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_tc_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_tc_motor_set_parameter()";

	MX_PMAC_TC_MOTOR *pmac_tc_motor = NULL;
	char command[41];
	mx_status_type mx_status;

	mx_status = mxd_pmac_tc_motor_get_pointers( motor,
						&pmac_tc_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PMACTC_DEBUG
	MX_DEBUG(-2,
	("%s invoked for motor '%s' (type %ld) for parameter type '%s' (%ld).",
		fname, motor->record->name, motor->record->mx_type,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	/* Send the parameter request set to the StrCmd EPICS variable. */

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		snprintf( command, sizeof(command),
			"I%ld16=%ld", pmac_tc_motor->motor_number,
			mx_round( motor->raw_speed ) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Parameter type %ld is not supported by the driver for motor '%s'",
			motor->parameter_type, motor->record->name );
		break;
	}

	mx_status = mx_caput( &(pmac_tc_motor->strcmd_pv),
				MX_CA_STRING, 1, command );

#if MXD_PMACTC_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}


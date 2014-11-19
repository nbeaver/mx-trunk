/*
 * Name:    d_powerpmac.c
 *
 * Purpose: MX driver for Delta Tau PowerPMAC motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010, 2012-2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define POWERPMAC_DEBUG			TRUE

#define POWERPMAC_USE_JOG_COMMANDS	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "gplib.h"	/* Delta Tau-provided include file. */

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "i_powerpmac.h"
#include "d_powerpmac.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_powerpmac_record_function_list = {
	NULL,
	mxd_powerpmac_create_record_structures,
	mxd_powerpmac_finish_record_initialization,
	NULL,
	NULL,
	mxd_powerpmac_open,
	NULL,
	NULL,
	NULL,
	mxd_powerpmac_special_processing_setup
};

MX_MOTOR_FUNCTION_LIST mxd_powerpmac_motor_function_list = {
	NULL,
	mxd_powerpmac_move_absolute,
	mxd_powerpmac_get_position,
	mxd_powerpmac_set_position,
	mxd_powerpmac_soft_abort,
	mxd_powerpmac_immediate_abort,
	NULL,
	NULL,
	mxd_powerpmac_raw_home_command,
	mxd_powerpmac_constant_velocity_move,
	mxd_powerpmac_get_parameter,
	mxd_powerpmac_set_parameter,
	mxd_powerpmac_simultaneous_start,
	mxd_powerpmac_get_status
};

MX_RECORD_FIELD_DEFAULTS mxd_powerpmac_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_POWERPMAC_STANDARD_FIELDS
};

long mxd_powerpmac_num_record_fields
		= sizeof( mxd_powerpmac_record_field_defaults )
			/ sizeof( mxd_powerpmac_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_rfield_def_ptr
			= &mxd_powerpmac_record_field_defaults[0];

/*--------*/

static mx_status_type mxd_powerpmac_process_function( void *record,
						void *record_field,
						int operation );

/*--------*/

static mx_status_type
mxd_powerpmac_get_pointers( MX_MOTOR *motor,
			MX_POWERPMAC_MOTOR **powerpmac_motor,
			MX_POWERPMAC **powerpmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_powerpmac_get_pointers()";

	MX_POWERPMAC_MOTOR *powerpmac_motor_ptr;
	MX_RECORD *powerpmac_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	powerpmac_motor_ptr = (MX_POWERPMAC_MOTOR *)
				motor->record->record_type_struct;

	if ( powerpmac_motor_ptr == (MX_POWERPMAC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POWERPMAC_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( powerpmac_motor != (MX_POWERPMAC_MOTOR **) NULL ) {
		*powerpmac_motor = powerpmac_motor_ptr;
	}

	if ( powerpmac == (MX_POWERPMAC **) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	powerpmac_record = powerpmac_motor_ptr->powerpmac_record;

	if ( powerpmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The powerpmac_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	*powerpmac = (MX_POWERPMAC *) powerpmac_record->record_type_struct;

	if ( *powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POWERPMAC pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_powerpmac_create_record_structures()";

	MX_MOTOR *motor;
	MX_POWERPMAC_MOTOR *powerpmac_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	powerpmac_motor = (MX_POWERPMAC_MOTOR *)
				malloc( sizeof(MX_POWERPMAC_MOTOR) );

	if ( powerpmac_motor == (MX_POWERPMAC_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_POWERPMAC_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = powerpmac_motor;
	record->class_specific_function_list
				= &mxd_powerpmac_motor_function_list;

	motor->record = record;
	powerpmac_motor->record = record;

	/* A PowerPMAC motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* We express accelerations in counts/sec**2. */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_powerpmac_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_POWERPMAC_MOTOR *powerpmac_motor;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_powerpmac_open()";

	MX_MOTOR *motor;
	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	long max_motors;
	char command[40];
	char response[100];
	char *ptr;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is the motor number valid? */

	max_motors = powerpmac->shared_mem->MaxMotors;

	if ( ( powerpmac_motor->motor_number < 1 )
	  || ( powerpmac_motor->motor_number > max_motors ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal PowerPMAC motor number %ld for record '%s'.  "
		"The allowed range is 1 to %lu.",
		    powerpmac_motor->motor_number, record->name, max_motors );
	}

	/* Get a pointer to the MotorData for this motor. */

	powerpmac_motor->motor_data =
	    &(powerpmac->shared_mem->Motor[ powerpmac_motor->motor_number ]);

	if ( powerpmac_motor->motor_data == (MotorData *) NULL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unable to get a pointer to the MotorData structure for "
		"PowerPMAC motor '%s'.", record->name );
	}

	powerpmac_motor->use_shm = TRUE;

	/* For PowerPMACs, the following call forces the
	 * MXSF_MTR_IS_BUSY bit on for 0.01 seconds after
	 * the start of a move.  This is done because
	 * sometimes the PowerPMAC controller will report
	 * the motor as not moving for a brief interval
	 * after the start of a move.
	 */

	mx_status = mx_motor_set_busy_start_interval( record, 0.01 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save a copy of the original value of the pLimits data structure. */

	snprintf( command, sizeof(command),
		"Motor[%lu].pLimits", powerpmac_motor->motor_number );

	mx_status = mxi_powerpmac_command( powerpmac, command,
					response, sizeof(response),
					POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = strchr( response, '=' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The command '%s' sent to Power PMAC '%s' received "
		"a response '%s' that does not contain a '=' character.",
			command, record->name, response );
	}

	ptr++;

	strlcpy( powerpmac_motor->original_plimits,
		ptr, MXU_POWERPMAC_PLIMITS_LENGTH );

	return MX_SUCCESSFUL_RESULT;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_powerpmac_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_move_absolute()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	char command[20];
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_destination.analog;

	snprintf( command, sizeof(command), "J=%ld", motor_steps );

	mx_status = mxd_powerpmac_jog_command( powerpmac_motor, powerpmac,
					command, NULL, 0, POWERPMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_get_position_steps()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	MotorData *motor_data = NULL;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_data = powerpmac_motor->motor_data;

	motor->raw_position.analog = motor_data->ActPos - motor_data->HomePos;

#if POWERPMAC_DEBUG
	MX_DEBUG(-2,("%s: motor->raw_position.analog = %g",
		fname, motor->raw_position.analog));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_set_position()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	MotorData *motor_data = NULL;
	double current_position, new_position, diff;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_data = powerpmac_motor->motor_data;

#if POWERPMAC_DEBUG
	MX_DEBUG(-2,("%s: ActPos = %g, HomePos = %g",
		fname, motor_data->ActPos, motor_data->HomePos));
#endif

	current_position = motor_data->ActPos - motor_data->HomePos;

	new_position = motor->raw_set_position.analog;

	diff = new_position - current_position;

#if POWERPMAC_DEBUG
	MX_DEBUG(-2,("%s: current_position = %g, new_position = %g, diff = %g",
		fname, current_position, new_position, diff ));
#endif

	motor_data->HomePos = motor_data->HomePos - diff;

#if POWERPMAC_DEBUG
	MX_DEBUG(-2,("%s: new HomePos = %g", fname, motor_data->HomePos));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_soft_abort()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_powerpmac_jog_command( powerpmac_motor, powerpmac, "J/",
						NULL, 0, POWERPMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_immediate_abort()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
							&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_powerpmac_jog_command( powerpmac_motor, powerpmac, "K",
						NULL, 0, POWERPMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_raw_home_command()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	double raw_home_speed;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_powerpmac_get_motor_variable( powerpmac_motor,
						powerpmac, "HomeVel",
						MXFT_DOUBLE, &raw_home_speed,
						POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_home_speed = fabs( raw_home_speed );

	if ( motor->raw_home_command < 0 ) {
		raw_home_speed = -raw_home_speed;
	}

	mx_status = mxd_powerpmac_set_motor_variable( powerpmac_motor,
						powerpmac, "HomeVel",
						MXFT_DOUBLE, raw_home_speed,
						POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_powerpmac_jog_command( powerpmac_motor, powerpmac, "HM",
						NULL, 0, POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_constant_velocity_move()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->constant_velocity_move > 0 ) {
		mx_status = mxd_powerpmac_jog_command( powerpmac_motor,
						powerpmac, "J+",
						NULL, 0, POWERPMAC_DEBUG );
	} else
	if ( motor->constant_velocity_move < 0 ) {
		mx_status = mxd_powerpmac_jog_command( powerpmac_motor,
						powerpmac, "J-",
						NULL, 0, POWERPMAC_DEBUG );
	} else {
		mx_status = MX_SUCCESSFUL_RESULT;
	}

	return mx_status;
}

static mx_status_type
mxd_powerpmac_get_capt_flag_sel_name( MX_MOTOR *motor,
					MX_POWERPMAC_MOTOR *powerpmac_motor,
					MX_POWERPMAC *powerpmac,
					char *capt_flag_sel_name,
					size_t max_name_length )
{
	static const char fname[] = "mxd_powerpmac_get_capt_flag_sel_name()";

	char command[100];
	char response[100];
	char *capt_flag_name, *ptr;
	mx_status_type mx_status;

	/* We need to figure out which channel is in use for
	 * capturing a triggered position.  The current setting
	 * pCaptFlag gives us a way of figuring that out.
	 */

	snprintf( command, sizeof(command),
		"Motor[%ld].pCaptFlag",
		powerpmac_motor->motor_number );

	mx_status = mxi_powerpmac_command( powerpmac, command,
				response, sizeof(response),
				POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Use the response to construct a command for reading
	 * the value of CaptFlagSel.
	 */

	capt_flag_name = strchr( response, '=' );

	if ( capt_flag_name == NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The response '%s' to command '%s' sent to Power PMAC '%s' "
		"did not contain an equals sign '='.",
			response, command, powerpmac->record->name );
	}

	capt_flag_name++;

#if 0
	MX_DEBUG(-2,("%s: capt_flag_name = '%s'",
		fname, capt_flag_name));
#endif

	/* FIXME: Currently we do not support Macro rings. */

	ptr = strstr( capt_flag_name, "Macro" );

	if ( ptr != NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Macro rings are not yet supported for Power PMAC '%s'.",
		powerpmac->record->name );
	}

	/* Look for the _second_ period character '.' in the name. */

	ptr = strchr( capt_flag_name, '.' );

	if ( ptr == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The Power PMAC variable '%s' has an illegal value of '%s' "
		"for Power PMAC '%s'.",
			command, capt_flag_name, powerpmac->record->name );
	}

	ptr++;

	ptr = strchr( ptr, '.' );

	if ( ptr == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The Power PMAC variable '%s' has an illegal value of '%s' "
		"for Power PMAC '%s'.",
			command, capt_flag_name, powerpmac->record->name );
	}

	/* Chop off the trailing part of the name. */

	*ptr = '\0';

	/* Append the string '.CaptFlagSel' to the name. */

	strlcat( response, ".CaptFlagSel", sizeof(response) );

	strlcpy( capt_flag_sel_name, capt_flag_name, max_name_length );

#if 0
	MX_DEBUG(-2,("%s: capt_flag_sel_name = '%s'",
		fname, capt_flag_sel_name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_get_parameter()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	MotorData *motor_data = NULL;
	double double_value;
	double jog_ta_in_seconds, jog_ts_in_seconds;
	char capt_flag_sel_name[100];
	long capt_flag_sel;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_data = powerpmac_motor->motor_data;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		/* Reported by the controller in count/msec. */

		motor->raw_speed = 1000.0 * motor_data->JogSpeed;
		break;
	case MXLV_MTR_BASE_SPEED:
		/* The POWERPMAC doesn't seem to have the concept of
		 * a base speed, since it is oriented to servo motors.
		 */

		motor->raw_base_speed = 0.0;

		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mxd_powerpmac_get_motor_variable( powerpmac_motor,
						powerpmac, "JogTa",
						MXFT_DOUBLE, &double_value,
						POWERPMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_acceleration_parameters[0] = 0.001 * double_value;

		mx_status = mxd_powerpmac_get_motor_variable( powerpmac_motor,
						powerpmac, "JogTs",
						MXFT_DOUBLE, &double_value,
						POWERPMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_acceleration_parameters[1] = 0.001 * double_value;

		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;
        case MXLV_MTR_ACCELERATION_TIME:
		mx_status =
		    mx_motor_update_speed_and_acceleration_parameters( motor );

		jog_ta_in_seconds = motor->raw_acceleration_parameters[0];
		jog_ts_in_seconds = motor->raw_acceleration_parameters[1];

		if ( jog_ta_in_seconds > jog_ts_in_seconds ) {
			motor->acceleration_time =
				jog_ta_in_seconds + jog_ts_in_seconds;
		} else {
			motor->acceleration_time =
				2.0 * jog_ts_in_seconds;
		}

		break;
	case MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
		mx_status = mxd_powerpmac_get_capt_flag_sel_name( motor,
					powerpmac_motor, powerpmac,
					capt_flag_sel_name,
					sizeof(capt_flag_sel_name) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		capt_flag_sel = mxi_powerpmac_get_long( powerpmac,
							capt_flag_sel_name,
							POWERPMAC_DEBUG );

		MX_DEBUG(-2,("%s: capt_flag_sel = %ld",
			fname, capt_flag_sel ));

		switch( capt_flag_sel ) {
		case 0:		/* HOMEn */
			motor->limit_switch_as_home_switch = 0;
			break;
		case 1:		/* PLIMn */
			motor->limit_switch_as_home_switch = 1;
			break;
		case 2:		/* MLIMn */
			motor->limit_switch_as_home_switch = -1;
			break;
		case 3:		/* USERn */
			return mx_error( MXE_UNSUPPORTED, fname,
			"The use of USERn (3) for CaptFlagSel is not supported "
			"by MX for Power PMAC '%s'.",
				powerpmac->record->name );
			break;
		default:
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Power PMAC '%s' returned an illegal value %ld "
			"for '%s'.",
				powerpmac->record->name,
				capt_flag_sel,
				capt_flag_sel_name );
			break;
		}

		MX_DEBUG(-2,("%s: '%s' limit_switch_as_home_switch = %ld",
			fname, motor->record->name,
			motor->limit_switch_as_home_switch));

		break;
	case MXLV_MTR_AXIS_ENABLE:
		if ( motor_data->ServoCtrl != 0 ) {
			motor->axis_enable = TRUE;
		} else {
			motor->axis_enable = FALSE;
		}
		break;
	case MXLV_MTR_PROPORTIONAL_GAIN:
		motor->proportional_gain = motor_data->Servo.Kp;
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		motor->integral_gain = motor_data->Servo.Ki;
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		motor->derivative_gain = motor_data->Servo.Kvfb;
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		motor->velocity_feedforward_gain = motor_data->Servo.Kvff;
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		motor->acceleration_feedforward_gain = motor_data->Servo.Kaff;
		break;
	case MXLV_MTR_INTEGRAL_LIMIT:
		motor->integral_limit = motor_data->Servo.MaxInt;
		break;
	default:
		mx_status = mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_set_parameter()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	MotorData *motor_data = NULL;
	char command[100];
	char capt_flag_sel_name[100];
	int capt_flag_sel;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_data = powerpmac_motor->motor_data;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		motor_data->JogSpeed = 0.001 * motor->raw_speed;
		break;
	case MXLV_MTR_BASE_SPEED:
		/* The POWERPMAC doesn't seem to have the concept of a
		 * base speed, since it is oriented to servo motors.
		 */

		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		/* If JogTa >= 0, then JogTa and JogTs specify acceleration
		 * times in milliseconds.  If JogTa < 0, then JogTa and JogTs
		 * are peak accelerations in motor units per msec squared.
		 */

		mx_status = mxd_powerpmac_set_motor_variable( powerpmac_motor,
						powerpmac, "JogTa",
						MXFT_DOUBLE,
					motor->raw_acceleration_parameters[0],
						POWERPMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Motor[x].JogTs (jog acceleration S-curve time) is in
		 * milliseconds.
		 */

		mx_status = mxd_powerpmac_set_motor_variable( powerpmac_motor,
						powerpmac, "JogTs",
						MXFT_DOUBLE,
					motor->raw_acceleration_parameters[1],
						POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
		MX_DEBUG(-2,("%s: '%s' limit_switch_as_home_switch = %ld",
			fname, motor->record->name,
			motor->limit_switch_as_home_switch));

		mx_status = mxd_powerpmac_get_capt_flag_sel_name( motor,
						powerpmac_motor, powerpmac,
						capt_flag_sel_name,
						sizeof(capt_flag_sel_name) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Construct a command to set a new value for CaptFlagSel. */

		if ( motor->limit_switch_as_home_switch > 0 ) {
			capt_flag_sel = 1;	/* PLIMn */
		} else
		if ( motor->limit_switch_as_home_switch == 0 ) {
			capt_flag_sel = 0;	/* HOMEn */
		} else {
			capt_flag_sel = 2;	/* MLIMn */
		}

		snprintf( command, sizeof(command), "%s=%d",
			capt_flag_sel_name, capt_flag_sel );

		mx_status = mxi_powerpmac_command( powerpmac, command,
						NULL, 0, POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = TRUE;
		}

		if ( motor->axis_enable == FALSE ) {
			snprintf( command, sizeof(command),
			"#%ld out 0", powerpmac_motor->motor_number );

			mx_status = mxi_powerpmac_command( powerpmac,
					command, NULL, 0, POWERPMAC_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		motor_data->ServoCtrl = motor->axis_enable;
		break;
	case MXLV_MTR_CLOSED_LOOP:
		if ( motor->closed_loop ) {
			mx_status = mxd_powerpmac_jog_command(
					powerpmac_motor, powerpmac,
					"J/", NULL, 0, POWERPMAC_DEBUG );
		} else {
			mx_status = mxd_powerpmac_jog_command(
					powerpmac_motor, powerpmac,
					"K", NULL, 0, POWERPMAC_DEBUG );
		}
		break;
	case MXLV_MTR_FAULT_RESET:
		mx_status = mxd_powerpmac_jog_command(
					powerpmac_motor, powerpmac, "J/",
					NULL, 0, POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_PROPORTIONAL_GAIN:
		motor_data->Servo.Kp = motor->proportional_gain;
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		motor_data->Servo.Ki = motor->integral_gain;
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		motor_data->Servo.Kvfb = motor->derivative_gain;
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		motor_data->Servo.Kvff = motor->velocity_feedforward_gain;
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		motor_data->Servo.Kaff = motor->acceleration_feedforward_gain;
		break;
	case MXLV_MTR_INTEGRAL_LIMIT:
		motor_data->Servo.MaxInt = motor->integral_limit;
		break;
	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_simultaneous_start( long num_motor_records,
				MX_RECORD **motor_record_array,
				double *position_array,
				unsigned long flags )
{
	static const char fname[] = "mxd_powerpmac_simultaneous_start()";

	MX_RECORD *powerpmac_interface_record = NULL;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	char command_buffer[500];
	char *ptr;
	const char *driver_name;
	double raw_position;
	long i, raw_steps;
	size_t length, buffer_left;
	mx_status_type mx_status;

	if ( num_motor_records <= 0 )
		return MX_SUCCESSFUL_RESULT;

	/* Construct the move command to send to the POWERPMAC. */

	ptr = command_buffer;

	*ptr = '\0';

	for ( i = 0; i < num_motor_records; i++ ) {
		motor_record = motor_record_array[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		driver_name = mx_get_driver_name( motor_record );

		if ( strcmp( driver_name, "powerpmac_motor" ) != 0 ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Cannot perform a simultaneous start since motors "
			"'%s' and '%s' are not the same type of motors.",
				motor_record_array[0]->name,
				motor_record->name );
		}

		powerpmac_motor = (MX_POWERPMAC_MOTOR *)
					motor_record->record_type_struct;

		if ( powerpmac_interface_record == (MX_RECORD *) NULL ) {
			powerpmac_interface_record
				= powerpmac_motor->powerpmac_record;

			powerpmac = (MX_POWERPMAC *)
				powerpmac_interface_record->record_type_struct;
		}

		/* Verify that the POWERPMAC motor records all belong
		 * to the same PowerPMAC controller.
		 */

		if ( powerpmac_interface_record
				!= powerpmac_motor->powerpmac_record )
		{
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are controlled by different POWERPMAC interfaces, "
		"namely '%s' and '%s'.",
				motor_record_array[0]->name,
				motor_record->name,
				powerpmac_interface_record->name,
				powerpmac_motor->powerpmac_record->name );
		}

		/* Compute the new position in raw units. */

		raw_position =
			mx_divide_safely( position_array[i] - motor->offset,
						motor->scale );

		raw_steps = mx_round( raw_position );

		/* Append the part of the command referring to this motor. */

		length = strlen( command_buffer );
		buffer_left = sizeof(command_buffer) - length;
		ptr = command_buffer + length;

		snprintf( ptr, buffer_left,
		    "#%ldJ=%ld ", powerpmac_motor->motor_number, raw_steps );
	}

	if ( powerpmac_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"No powerpmac interface record pointer was found for "
		"record '%s'.", motor_record_array[0]->name );
	}

	/* Send the command to the POWERPMAC. */

	MX_DEBUG( 2,("%s: command = '%s'", fname, command_buffer));

	mx_status = mxi_powerpmac_command( powerpmac, command_buffer,
						NULL, 0, POWERPMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_get_status()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	char response[50];
	unsigned long status[ MX_POWERPMAC_NUM_STATUS_CHARACTERS ];
	int i, length;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Request the motor status. */

	mx_status = mxd_powerpmac_jog_command( powerpmac_motor, powerpmac, "?",
				response, sizeof response, POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = (int) strlen( response );

	if ( length < MX_POWERPMAC_NUM_STATUS_CHARACTERS ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Too few characters seen in response to motor status command.  "
	"Only saw %d characters.", length );
	}

	/* Change the reported motor status from ASCII characters
	 * to unsigned long integers.
	 *
	 * Note that the first character returned is a $ character.
	 * We must skip over that.
	 */

	for ( i = 0; i < MX_POWERPMAC_NUM_STATUS_CHARACTERS; i++ ) {
		status[i] = mx_hex_char_to_unsigned_long( response[i+1] );

#if 0
		MX_DEBUG(-2,("%s: status[%d] = %#lx",
			fname, i, status[i]));
#endif
	}

	/* Check for all the status bits that we are interested in. */

	motor->status = 0;

	/* ============ First word returned. ============== */

	/* ---------- First character returned. ----------- */

	/* Bits 30 and 31: (ignored) */

	/* Bit 29: Hardware negative limit set */

	if ( status[0] & 0x2 ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* Bit 28: Hardware positive limit set */

	if ( status[0] & 0x1 ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* ---------- Second character returned. ----------- */

	/* Bit 27: (ignored) */

	/* Bit 26: Fatal following error */

	if ( status[1] & 0x4 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 25: (ignored) */

	/* Bit 24: Amplifier fault */

	if ( status[1] & 0x1 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}

	/* ---------- Third character returned. ----------- */

	/* Bit 23: Software negative limit set */

	if ( status[2] & 0x8 ) {
		motor->status |= MXSF_MTR_SOFT_NEGATIVE_LIMIT_HIT;
	}

	/* Bit 22: Software positive limit set */

	if ( status[2] & 0x4 ) {
		motor->status |= MXSF_MTR_SOFT_POSITIVE_LIMIT_HIT;
	}

	/* Bits 20 and 21: (ignored) */

	/* ---------- Fourth character returned. ----------- */

	/* Bits 16 to 19: (ignored) */

	/* ---------- Fifth character returned. ----------- */

	/* Bit 15: Home complete */

	if ( status[4] & 0x8 ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}

	/* Bit 14: (ignored) */

	/* Bit 13: Closed-loop mode */

	if ( (status[4] & 0x2) == 0 ) {
		motor->status |= MXSF_MTR_OPEN_LOOP;
	}

	/* Bit 12: Amplifier enabled */

	if ( (status[4] & 0x1) == 0 ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	/* ---------- Sixth character returned. ----------- */

	/* Bit 11: In position */

	if ( (status[5] & 0x8) == 0 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* Note: all remaining bits in the response are ignored. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_POWERPMAC_ORIGINAL_PLIMITS:
			record_field->process_function
					= mxd_powerpmac_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------*/

MX_EXPORT mx_status_type
mxd_powerpmac_jog_command( MX_POWERPMAC_MOTOR *powerpmac_motor,
			MX_POWERPMAC *powerpmac,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_powerpmac_jog_command()";

	char command_buffer[100];
	mx_status_type mx_status;

	if ( powerpmac_motor == (MX_POWERPMAC_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC_MOTOR pointer passed was NULL." );
	}
	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC pointer passed for motor '%s' was NULL.",
			powerpmac_motor->record->name );
	}

	snprintf( command_buffer, sizeof(command_buffer),
			"#%ld%s", powerpmac_motor->motor_number, command );

	mx_status = mxi_powerpmac_command( powerpmac, command_buffer,
				response, response_buffer_length, debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_get_motor_variable( MX_POWERPMAC_MOTOR *powerpmac_motor,
				MX_POWERPMAC *powerpmac,
				char *variable_name,
				long variable_type,
				double *double_ptr,
				int debug_flag )
{
	static const char fname[] = "mxd_powerpmac_get_motor_variable()";

	char command_buffer[100];
	char response[100];
	char *response_ptr;
	int num_items;
	long long_value;
	mx_status_type mx_status;

	if ( powerpmac_motor == (MX_POWERPMAC_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC_MOTOR pointer passed was NULL." );
	}
	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC pointer passed for motor '%s' was NULL.",
			powerpmac_motor->record->name );
	}

	snprintf( command_buffer, sizeof(command_buffer),
	    "Motor[%ld].%s", powerpmac_motor->motor_number, variable_name );

	mx_status = mxi_powerpmac_command( powerpmac, command_buffer,
				response, sizeof response, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* On PowerPMAC systems, the returned value is prefixed with the
	 * name of the variable followed by an equals sign (=).
	 */

	response_ptr = strchr( response, '=' );

	if ( response_ptr == NULL ) {
		response_ptr = response;
	} else {
		response_ptr++;
	}

	/* Parse the returned value. */

	switch( variable_type ) {
	case MXFT_LONG:
		num_items = sscanf( response_ptr, "%ld", &long_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot parse response to command '%s' as a long integer.  Response = '%s'",
				command_buffer, response );
		}

		*double_ptr = (double) long_value;
		break;
	case MXFT_BOOL:
		num_items = sscanf( response_ptr, "%ld", &long_value );

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
		num_items = sscanf( response_ptr, "%lg", double_ptr );

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
mxd_powerpmac_set_motor_variable( MX_POWERPMAC_MOTOR *powerpmac_motor,
				MX_POWERPMAC *powerpmac,
				char *variable_name,
				long variable_type,
				double double_value,
				int debug_flag )
{
	static const char fname[] = "mxd_powerpmac_set_motor_variable()";

	char command_buffer[100];
	char response[100];
	long long_value;
	mx_status_type mx_status;

	if ( powerpmac_motor == (MX_POWERPMAC_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC_MOTOR pointer passed was NULL." );
	}
	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC pointer passed for motor '%s' was NULL.",
			powerpmac_motor->record->name );
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
			"Motor[%ld].%s=%ld",
			powerpmac_motor->motor_number,
			variable_name,
			long_value );
		break;
	case MXFT_DOUBLE:
		snprintf( command_buffer, sizeof(command_buffer),
			"Motor[%ld].%s=%f",
			powerpmac_motor->motor_number,
			variable_name,
			double_value );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG, MXFT_BOOL, and MXFT_DOUBLE are supported." );
	}

	mx_status = mxi_powerpmac_command( powerpmac, command_buffer,
				response, sizeof response, debug_flag );

	return mx_status;
}

/*--------*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxd_powerpmac_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxd_powerpmac_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_POWERPMAC_MOTOR *powerpmac_motor;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	powerpmac_motor = (MX_POWERPMAC_MOTOR *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_POWERPMAC_ORIGINAL_PLIMITS:
			/* Nothing to do since the necessary string is
			 * already stored in the field.
			 */

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}


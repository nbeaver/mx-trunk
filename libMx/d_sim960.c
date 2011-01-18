/*
 * Name:    d_sim960.c
 *
 * Purpose: MX driver for the Stanford Research Systems SIM960
 *          Analog PID Controller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SIM960_DEBUG	TRUE

#define MXD_SIM960_ERROR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "d_sim960.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_sim960_record_function_list = {
	NULL,
	mxd_sim960_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_sim960_open
};

MX_MOTOR_FUNCTION_LIST mxd_sim960_motor_function_list = {
	NULL,
	mxd_sim960_move_absolute,
	mxd_sim960_get_position,
	NULL,
	mxd_sim960_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_sim960_get_parameter,
	mxd_sim960_set_parameter,
	NULL,
	mxd_sim960_get_status
};

/* SIM960 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_sim960_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_SIM960_STANDARD_FIELDS
};

long mxd_sim960_num_record_fields
		= sizeof( mxd_sim960_record_field_defaults )
			/ sizeof( mxd_sim960_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sim960_rfield_def_ptr
			= &mxd_sim960_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_sim960_get_pointers( MX_MOTOR *motor,
			MX_SIM960 **sim960,
			const char *calling_fname )
{
	static const char fname[] = "mxd_sim960_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( sim960 == (MX_SIM960 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SIM960 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*sim960 = (MX_SIM960 *) motor->record->record_type_struct;

	if ( *sim960 == (MX_SIM960 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SIM960 pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sim960_command( MX_SIM960 *sim960,
		char *command,
		char *response,
		size_t max_response_length,
		int debug_flag )
{
	static const char fname[] = "mxd_sim960_command()";

#if 0
	char esr_response[10];
	unsigned char esr_byte;
#endif
	mx_status_type mx_status;

	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command buffer pointer passed was NULL." );
	}

	/* Send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, sim960->record->name));
	}

	mx_status = mx_rs232_putline( sim960->port_record, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response != (char *) NULL ) {
		/* If a response is expected, read back the response. */

		mx_status = mx_rs232_getline( sim960->port_record,
					response, max_response_length,
					NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, sim960->record->name ));
		}
	}

	/* Check for errors in the previous command. */

#if 0

#if MXD_SIM960_ERROR_DEBUG
	MX_DEBUG(-2,("%s: sending '*ESR?' to '%s'",
		fname, sim960->record->name ));
#endif

	mx_status = mx_rs232_putline( sim960->port_record, "*ESR?", NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( sim960->port_record,
					esr_response, sizeof(esr_response),
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIM960_ERROR_DEBUG
	MX_DEBUG(-2,("%s: received '%s' from '%s'",
		fname, esr_response, sim960->record->name ));
#endif

	esr_byte = atol( esr_response );

	if ( esr_byte & 0x20 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"Command error (CME) seen for command '%s' sent to '%s'.",
			command, sim960->record->name );
	} else
	if ( esr_byte & 0x10 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"Execution error (EXE) seen for command '%s' sent to '%s'.",
			command, sim960->record->name );
	} else
	if ( esr_byte & 0x08 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "Device dependent error (DDE) seen for command '%s' sent to '%s'.",
			command, sim960->record->name );
	} else
	if ( esr_byte & 0x02 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Input data lost (INP) for command '%s' sent to '%s'.",
			command, sim960->record->name );
	} else
	if ( esr_byte & 0x04 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Output data lost (QYE) for command '%s' sent to '%s'.",
			command, sim960->record->name );
	}

#endif /* 0 */

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_sim960_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_sim960_create_record_structures()";

	MX_MOTOR *motor;
	MX_SIM960 *sim960;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	sim960 = (MX_SIM960 *) malloc( sizeof(MX_SIM960) );

	if ( sim960 == (MX_SIM960 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_SIM960 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = sim960;
	record->class_specific_function_list = &mxd_sim960_motor_function_list;

	motor->record = record;
	sim960->record = record;

	/* A SIM960 analog PID controller is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* The SIM960 does not implement acceleration. */

	motor->acceleration_type = MXF_MTR_ACCEL_NONE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sim960_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sim960_open()";

	MX_MOTOR *motor;
	MX_SIM960 *sim960;
	char command[100];
	char response[100];
	char copy_of_response[100];
	int argc, num_items;
	char **argv;
	unsigned long flags;
	mx_status_type mx_status;

	sim960 = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_sim960_get_pointers( motor, &sim960, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any outstanding characters. */

	mx_status = mx_rs232_discard_unwritten_output( sim960->port_record,
							MXD_SIM960_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( sim960->port_record,
							MXD_SIM960_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reset the communication interface by sending a break signal. */

#if MXD_SIM960_DEBUG
	MX_DEBUG(-2,("%s: sending a break signal to '%s'.",
				fname, record->name ));
#endif

	mx_status = mx_rs232_send_break( sim960->port_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that we are connected to a SIM960 analog PID controller. */

	mx_status = mxd_sim960_command( sim960, "*IDN?",
					response, sizeof(response),
					MXD_SIM960_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( copy_of_response, response, sizeof(copy_of_response) );

	mx_string_split( copy_of_response, ",", &argc, &argv );

	if ( argc != 4 ) {
		free( argv );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find 4 tokens in the response '%s' to "
		"the *IDN? command sent to '%s'.",
			response, record->name );
	}
	if ( strcmp( argv[0], "Stanford_Research_Systems" ) != 0 ) {
		free( argv );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Controller '%s' is not a Stanford Research Systems device.  "
		"The response to '*IDN?' was '%s'.",
			record->name, response );
	}
	if ( strcmp( argv[1], "SIM960" ) != 0 ) {
		free( argv );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Device '%s' is not a SIM960 analog PID controller.  "
		"The response to '*IDN?' was '%s'.",
			record->name, response );
	}

	/* Get the version number. */

	num_items = sscanf( argv[3], "ver%lf", &(sim960->version) );

	if ( num_items != 1 ) {
		mx_status = mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find the SIM960 version number in the token '%s' "
		"contained in the response '%s' to '*IDN?' by controller '%s'.",
			argv[3], response, record->name );

		free( argv );

		return mx_status;
	}

	/* Enable setpoint ramping.  This prevents the setpoint from
	 * suddenly jumping from one value to another.
	 */

	mx_status = mxd_sim960_command( sim960, "RAMP ON",
					NULL, 0, MXD_SIM960_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Specify whether or not we are using an internal or
	 * an external setpoint.
	 */

	flags = sim960->sim960_flags;

	if ( flags & MXF_SIM960_EXTERNAL_SETPOINT ) {
		strlcpy( command, "INPT EXT", sizeof(command) );
	} else {
		strlcpy( command, "INPT INT", sizeof(command) );
	}

	mx_status = mxd_sim960_command( sim960, command,
					NULL, 0, MXD_SIM960_DEBUG );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_sim960_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_sim960_move_absolute()";

	MX_SIM960 *sim960 = NULL;
	char command[80];
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_sim960_get_pointers( motor, &sim960, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = sim960->sim960_flags;

	if ( ( flags & MXF_SIM960_EXTERNAL_SETPOINT ) == 0 ) {

		snprintf( command, sizeof(command), "SETP %lf",
					motor->raw_destination.analog );

		mx_status = mxd_sim960_command( sim960, command,
					NULL, 0, MXD_SIM960_DEBUG );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sim960_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_sim960_get_position()";

	MX_SIM960 *sim960 = NULL;
	char response[100];
	mx_status_type mx_status;

	mx_status = mxd_sim960_get_pointers( motor, &sim960, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sim960_command( sim960, "MMON?",
					response, sizeof response,
					MXD_SIM960_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = atof( response );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sim960_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_sim960_soft_abort()";

	MX_SIM960 *sim960 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_sim960_get_pointers( motor, &sim960, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We implement soft abort by getting the current measured
	 * value and setting the setpoint to that.
	 */

	mx_status = mx_motor_get_position( motor->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_move_absolute( motor->record, motor->position, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sim960_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_sim960_get_parameter()";

	MX_SIM960 *sim960;
	char response[100];
	int proportional_action, integral_action, derivative_action;
	int output_control;
	mx_status_type mx_status;

	sim960 = NULL;

	mx_status = mxd_sim960_get_pointers( motor, &sim960, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIM960_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mxd_sim960_command( sim960, "RATE?",
						response, sizeof(response),
						MXD_SIM960_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_speed = atof( response );
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		/* If the proportional action is turned off, we return
		 * a value of 0.
		 */

		mx_status = mxd_sim960_command( sim960, "PCTL?",
						response, sizeof(response),
						MXD_SIM960_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		proportional_action = atol( response );

		if ( proportional_action == 0 ) {
			motor->proportional_gain = 0.0;
		} else {
			/* Proportional action is turned on, so get
			 * the proportional gain.
			 */

			mx_status = mxd_sim960_command( sim960, "GAIN?",
						response, sizeof(response),
						MXD_SIM960_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			motor->proportional_gain = atof( response );
		}
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		/* If the integral action is turned off, we return
		 * a value of 0.
		 */

		mx_status = mxd_sim960_command( sim960, "ICTL?",
						response, sizeof(response),
						MXD_SIM960_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		integral_action = atol( response );

		if ( integral_action == 0 ) {
			motor->integral_gain = 0.0;
		} else {
			/* Integral action is turned on, so get
			 * the integral gain.
			 */

			mx_status = mxd_sim960_command( sim960, "INTG?",
						response, sizeof(response),
						MXD_SIM960_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			motor->integral_gain = atof( response );
		}
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		/* If the derivative action is turned off, we return
		 * a value of 0.
		 */

		mx_status = mxd_sim960_command( sim960, "DCTL?",
						response, sizeof(response),
						MXD_SIM960_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		derivative_action = atol( response );

		if ( derivative_action == 0 ) {
			motor->derivative_gain = 0.0;
		} else {
			/* Derivative action is turned on, so get
			 * the derivative gain.
			 */

			mx_status = mxd_sim960_command( sim960, "DERV?",
						response, sizeof(response),
						MXD_SIM960_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			motor->derivative_gain = atof( response );
		}
		break;

	case MXLV_MTR_EXTRA_GAIN:
		/* If the output offset is turned off, we return
		 * a value of 0.
		 */

		mx_status = mxd_sim960_command( sim960, "OCTL?",
						response, sizeof(response),
						MXD_SIM960_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		output_control = atol( response );

		if ( output_control == 0 ) {
			motor->extra_gain = 0.0;
		} else {
			/* The output offset is turned on, so read it into
			 * the "extra" gain.
			 */

			mx_status = mxd_sim960_command( sim960, "OFST?",
						response, sizeof(response),
						MXD_SIM960_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			motor->extra_gain = atof( response );
		}
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sim960_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_sim960_set_parameter()";

	MX_SIM960 *sim960;
	char command[80];
	mx_status_type mx_status;

	sim960 = NULL;

	mx_status = mxd_sim960_get_pointers( motor, &sim960, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIM960_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:

		snprintf( command, sizeof(command),
			"RATE %lf", motor->raw_speed );

		mx_status = mxd_sim960_command( sim960, command,
						NULL, 0, MXD_SIM960_DEBUG );
		break;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = 1;
			strlcpy( command, "AMAN PID", sizeof(command) );
		} else {
			strlcpy( command, "AMAN MAN", sizeof(command) );
		}

		mx_status = mxd_sim960_command( sim960, command,
						NULL, 0, MXD_SIM960_DEBUG );
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:

		/* If the proportional gain is set to 0, then turn
		 * off proportional action.
		 */

		if ( fabs( motor->proportional_gain ) < 1.0e-10 ) {
			mx_status = mxd_sim960_command( sim960, "PCTL OFF",
						NULL, 0, MXD_SIM960_DEBUG );
		} else {
			/* Set the proportional gain and turn on
			 * proportional action.
			 */

			snprintf( command, sizeof(command),
				"GAIN %lf", motor->proportional_gain );

			mx_status = mxd_sim960_command( sim960, command,
						NULL, 0, MXD_SIM960_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_sim960_command( sim960, "PCTL ON",
						NULL, 0, MXD_SIM960_DEBUG );
		}
		break;

	case MXLV_MTR_INTEGRAL_GAIN:

		/* If the integral gain is set to 0, then turn
		 * off integral action.
		 */

		if ( fabs( motor->integral_gain ) < 1.0e-10 ) {
			mx_status = mxd_sim960_command( sim960, "ICTL OFF",
						NULL, 0, MXD_SIM960_DEBUG );
		} else {
			/* Set the integral gain and turn on
			 * integral action.
			 */

			snprintf( command, sizeof(command),
				"INTG %lf", motor->integral_gain );

			mx_status = mxd_sim960_command( sim960, command,
						NULL, 0, MXD_SIM960_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_sim960_command( sim960, "ICTL ON",
						NULL, 0, MXD_SIM960_DEBUG );
		}
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:

		/* If the derivative gain is set to 0, then turn
		 * off derivative action.
		 */

		if ( fabs( motor->derivative_gain ) < 1.0e-10 ) {
			mx_status = mxd_sim960_command( sim960, "DCTL OFF",
						NULL, 0, MXD_SIM960_DEBUG );
		} else {
			/* Set the derivative gain and turn on
			 * derivative action.
			 */

			snprintf( command, sizeof(command),
				"DERV %lf", motor->derivative_gain );

			mx_status = mxd_sim960_command( sim960, command,
						NULL, 0, MXD_SIM960_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_sim960_command( sim960, "ICTL ON",
						NULL, 0, MXD_SIM960_DEBUG );
		}
		break;

	case MXLV_MTR_EXTRA_GAIN:

		/* If the "extra" gain is set to 0, then turn
		 * off the output offset.
		 */

		if ( fabs( motor->extra_gain ) < 1.0e-10 ) {
			mx_status = mxd_sim960_command( sim960, "OCTL OFF",
						NULL, 0, MXD_SIM960_DEBUG );
		} else {
			/* Set and turn on the output offset. */

			snprintf( command, sizeof(command),
				"OFST %lf", motor->extra_gain );

			mx_status = mxd_sim960_command( sim960, command,
						NULL, 0, MXD_SIM960_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_sim960_command( sim960, "OCTL ON",
						NULL, 0, MXD_SIM960_DEBUG );
		}
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sim960_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_sim960_get_status()";

	MX_SIM960 *sim960;
	char response[80];
	long ramping_status;
	mx_status_type mx_status;

	sim960 = NULL;

	mx_status = mxd_sim960_get_pointers( motor, &sim960, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	mx_status = mxd_sim960_command( sim960, "RMPS?",
					response, sizeof response,
					MXD_SIM960_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ramping_status = atol( response );

	/* All states other than IDLE (0) are considered to be busy. */

	if ( ramping_status != 0 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}


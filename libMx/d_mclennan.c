/*
 * Name:    d_mclennan.c 
 *
 * Purpose: MX motor driver to control McLennan motor controllers.
 *
 * Author:  William Lavender
 *
 * Warning: So far, this driver has only been tested with the PM600 and PM304
 *          controllers.  The PM301, PM341, and PM381 support was written
 *          based on the PM3x1 manual and has not actually been tested yet.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2004, 2006-2007, 2010, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MCLENNAN_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "d_mclennan.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mclennan_record_function_list = {
	NULL,
	mxd_mclennan_create_record_structures,
	mxd_mclennan_finish_record_initialization,
	NULL,
	mxd_mclennan_print_motor_structure,
	mxd_mclennan_open,
	NULL,
	NULL,
	mxd_mclennan_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_mclennan_motor_function_list = {
	NULL,
	mxd_mclennan_move_absolute,
	mxd_mclennan_get_position,
	mxd_mclennan_set_position,
	mxd_mclennan_soft_abort,
	mxd_mclennan_immediate_abort,
	NULL,
	NULL,
	mxd_mclennan_raw_home_command,
	mxd_mclennan_constant_velocity_move,
	mxd_mclennan_get_parameter,
	mxd_mclennan_set_parameter,
	NULL,
	mxd_mclennan_get_status
};

/* McLennan MCLENNAN motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mclennan_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_MCLENNAN_STANDARD_FIELDS
};

long mxd_mclennan_num_record_fields
		= sizeof( mxd_mclennan_record_field_defaults )
			/ sizeof( mxd_mclennan_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mclennan_rfield_def_ptr
			= &mxd_mclennan_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_mclennan_get_pointers( MX_MOTOR *motor,
			MX_MCLENNAN **mclennan,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mclennan_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mclennan == (MX_MCLENNAN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MCLENNAN pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mclennan = (MX_MCLENNAN *) (motor->record->record_type_struct);

	if ( *mclennan == (MX_MCLENNAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCLENNAN pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type mxd_mclennan_handle_error( MX_MCLENNAN *mclennan,
						char *command,
						char *error_message );

static mx_status_type mxd_mclennan_send_reset( MX_MCLENNAN *mclennan,
						int do_reset );

/* === */

MX_EXPORT mx_status_type
mxd_mclennan_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_mclennan_create_record_structures()";

	MX_MOTOR *motor;
	MX_MCLENNAN *mclennan;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for an MX_MOTOR structure for record '%s'.",
			record->name );
	}

	mclennan = (MX_MCLENNAN *) malloc( sizeof(MX_MCLENNAN) );

	if ( mclennan == (MX_MCLENNAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for an MX_MCLENNAN structure for record '%s'.",
			record->name );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = mclennan;
	record->class_specific_function_list
				= &mxd_mclennan_motor_function_list;

	motor->record = record;
	mclennan->record = record;

	/* An McLennan MCLENNAN motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The MCLENNAN reports accelerations in steps/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mclennan_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_mclennan_print_motor_structure()";

	MX_MOTOR *motor;
	MX_MCLENNAN *mclennan;
	double position, move_deadband;
	mx_status_type mx_status;

	mclennan = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type          = MCLENNAN\n\n");
	fprintf(file, "  name                = %s\n", record->name);
	fprintf(file, "  rs232               = %s\n",
						mclennan->rs232_record->name);

	fprintf(file, "  axis number         = %ld\n", mclennan->axis_number);
	fprintf(file, "  axis encoder number = %ld\n",
						mclennan->axis_encoder_number);
	fprintf(file, "  controller type     = PM%ld\n",
						mclennan->controller_type);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read the position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position            = %g %s  (%ld steps)\n",
		motor->position, motor->units,
		motor->raw_position.stepper );
	fprintf(file, "  scale               = %g %s per step.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset              = %g %s.\n",
		motor->offset, motor->units);

	fprintf(file, "  backlash            = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper);

	fprintf(file, "  negative limit      = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit      = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband       = %g %s  (%ld steps)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_mclennan_open()";

	MX_MOTOR *motor;
	MX_MCLENNAN *mclennan;
	mx_status_type mx_status;

	mclennan = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify the line terminators. */

	mx_status = mx_rs232_verify_configuration( mclennan->rs232_record,
			(long) MXF_232_DONT_CARE, (int) MXF_232_DONT_CARE,
			(char) MXF_232_DONT_CARE, (int) MXF_232_DONT_CARE,
			(char) MXF_232_DONT_CARE,
			0x0d0a, 0x0d );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_mclennan_resynchronize( record );

	return mx_status;
}

#define PM301_BANNER "PM301"
#define PM304_BANNER "Mclennan Servo Supplies Ltd. PM304"
#define PM341_BANNER "PM341"
#define PM381_BANNER "PM381"
#define PM600_BANNER "Mclennan Digiloop Motor Controller"

MX_EXPORT mx_status_type
mxd_mclennan_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_mclennan_resynchronize()";

	MX_MOTOR *motor;
	MX_MCLENNAN *mclennan;
	char command[80];
	char response[80];
	char *ptr;
	mx_status_type mx_status;

	mclennan = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any unread RS-232 input or unwritten RS-232 output. */

	mx_status = mx_rs232_discard_unwritten_output( mclennan->rs232_record,
							MCLENNAN_DEBUG );

	if ( (mx_status.code != MXE_SUCCESS)
	  && (mx_status.code != MXE_UNSUPPORTED) )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( mclennan->rs232_record,
							MCLENNAN_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send an identify command to verify that the controller is there
	 * and figure out what kind of controller it is.
	 */

	sprintf( command, "%ldID", mclennan->axis_number );

	mx_status = mx_rs232_putline( mclennan->rs232_record, command,
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( mclennan->rs232_record,
					response, sizeof(response),
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Look for the prefix separator in the response.  It is safer
	 * to look for '#' first.
	 */

	ptr = strchr( response, '#' );

	if ( ptr == NULL ) {
		ptr = strchr( response, ':' );

		if ( ptr == NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Prefix character not seen in response to the ID command for motor '%s'.  "
"Response = '%s'.", record->name, response );
		}
	}

	ptr++;

	MX_DEBUG(-2,("%s: ptr = '%s'", fname, ptr));

	/* Figure out which kind of McLennan controller this is. */

	if ( strncmp( ptr, PM600_BANNER, strlen(PM600_BANNER) ) == 0 ) {
		mclennan->controller_type = MXT_MCLENNAN_PM600;
	} else
	if ( strncmp( ptr, PM304_BANNER, strlen(PM304_BANNER) ) == 0 ) {
		mclennan->controller_type = MXT_MCLENNAN_PM304;
	} else
	if ( strncmp( ptr, PM301_BANNER, strlen(PM301_BANNER) ) == 0 ) {
		mclennan->controller_type = MXT_MCLENNAN_PM301;
	} else
	if ( strncmp( ptr, PM341_BANNER, strlen(PM341_BANNER) ) == 0 ) {
		mclennan->controller_type = MXT_MCLENNAN_PM341;
	} else
	if ( strncmp( ptr, PM381_BANNER, strlen(PM341_BANNER) ) == 0 ) {
		mclennan->controller_type = MXT_MCLENNAN_PM381;
	} else {
		mclennan->controller_type = MXT_MCLENNAN_UNKNOWN;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Did not get Mclennan identification banner in response to an 'ID' command "
"for motor '%s'.  Instead got the response '%s'",
			record->name, response );
	}

	if ( mclennan->controller_type == MXT_MCLENNAN_PM600 ) {
		mclennan->num_dinput_ports  = 8;
		mclennan->num_doutput_ports = 8;
		mclennan->num_ainput_ports  = 5;
		mclennan->num_aoutput_ports = 2;
	} else {
		mclennan->num_dinput_ports  = 4;
		mclennan->num_doutput_ports = 4;
		mclennan->num_ainput_ports  = 0;
		mclennan->num_aoutput_ports = 0;
	}

	MX_DEBUG(-2,("%s: Mclennan controller type = PM%ld",
			fname, mclennan->controller_type));

	/* Reset any errors. */

	mx_status = mxd_mclennan_command( mclennan, "RS",
						NULL, 0, MCLENNAN_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mclennan->controller_type == MXT_MCLENNAN_PM304 ) {

		/* The PM304 has several different reset commands. */

		mx_status = mxd_mclennan_command( mclennan, "RSES",
						NULL, 0, MCLENNAN_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_mclennan_command( mclennan, "RSST",
						NULL, 0, MCLENNAN_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mclennan_move_absolute()";

	MX_MCLENNAN *mclennan;
	char command[20];
	long motor_steps;
	mx_status_type mx_status;

	mclennan = NULL;

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_destination.stepper;

	/* Format the move command and send it. */

	sprintf( command, "MA%ld", motor_steps );

	mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mclennan_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mclennan_get_position()";

	MX_MCLENNAN *mclennan;
	char response[30];
	int num_items;
	long motor_steps;
	mx_status_type mx_status;

	mclennan = NULL;

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	mx_status = mxd_mclennan_command( mclennan, "QP",
						NULL, 0, MCLENNAN_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_mclennan_command( mclennan, "CO",
						NULL, 0, MCLENNAN_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#endif

	mx_status = mxd_mclennan_command( mclennan, "OA",
			response, sizeof response, MCLENNAN_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mclennan->controller_type == MXT_MCLENNAN_PM304 ) {
		num_items = sscanf( response, "AP=%ld", &motor_steps );
	} else {
		num_items = sscanf( response, "%ld", &motor_steps );
	}

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Motor '%s' returned an unparseable position value '%s'.",
			motor->record->name, response );
	}

	motor->raw_position.stepper = motor_steps;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mclennan_set_position()";

	MX_MCLENNAN *mclennan;
	char command[40];
	char response[40];
	char expected_response[40];
	char c;
	long motor_steps;
	mx_status_type mx_status;

	mclennan = NULL;

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_set_position.stepper;

	sprintf( command, "AP%ld", motor_steps );

	mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If a PM368 encoder display is installed, set the
	 * encoder position to match.
	 */

	if ( mclennan->axis_encoder_number >= 0 ) {

		sprintf( command, "%ldAP%ld",
				mclennan->axis_encoder_number, motor_steps );

		mx_status = mx_rs232_putline( mclennan->rs232_record,
					command, NULL, MCLENNAN_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_rs232_getline( mclennan->rs232_record,
					response, sizeof response,
					NULL, MCLENNAN_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		sprintf( expected_response,
				"%ld:OK", mclennan->axis_encoder_number );

		if ( strcmp( response, expected_response ) != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Did not get 'OK' response from the encoder controller for motor '%s'.  "
"Instead got '%s'", motor->record->name, response );
		}

		mx_status = mx_rs232_getchar( mclennan->rs232_record,
						&c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mclennan_soft_abort()";

	MX_MCLENNAN *mclennan;
	mx_status_type mx_status;

	mclennan = NULL;

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the soft stop command (an ESC character). */

	mx_status = mx_rs232_putchar( mclennan->rs232_record,
					'\033', MXF_232_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( mclennan->rs232_record,
							MCLENNAN_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mclennan_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mclennan_immediate_abort()";

	MX_MCLENNAN *mclennan;
	mx_status_type mx_status;

	mclennan = NULL;

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the immediate abort command (a ctrl-C character). */

	mx_status = mx_rs232_putchar( mclennan->rs232_record,
					'\003', MXF_232_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( mclennan->rs232_record,
							MCLENNAN_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mclennan_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mclennan_raw_home_command()";

	MX_MCLENNAN *mclennan;
	char command[20];
	mx_status_type mx_status;

	mclennan = NULL;

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mclennan->controller_type == MXT_MCLENNAN_PM600 ) {
		if ( motor->raw_home_command >= 0 ) {
			strcpy( command, "HD" );
		} else {
			strcpy( command, "HD-1" );
		}
	} else {
		if ( motor->raw_home_command >= 0 ) {
			strcpy( command, "IX" );
		} else {
			strcpy( command, "IX-1" );
		}
	}
		
	mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mclennan_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mclennan_constant_velocity_move()";

	MX_MCLENNAN *mclennan;
	char command[20];
	double speed;
	mx_status_type mx_status;

	mclennan = NULL;

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mclennan->controller_type == MXT_MCLENNAN_PM600 ) {

		mx_status = mx_motor_get_speed( motor->record, &speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( motor->constant_velocity_move >= 0 ) {
			sprintf( command, "CV%ld",
					mx_round( motor->raw_speed ) );
		} else {
			sprintf( command, "CV-%ld",
					mx_round( motor->raw_speed ) );
		}
	} else {
		if ( motor->constant_velocity_move >= 0 ) {
			strcpy( command, "CV" );
		} else {
			strcpy( command, "CV-1" );
		}
	}

	mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );

	return mx_status;
}

static mx_status_type
mxd_mclennan_parse_qa_response( MX_MOTOR *motor, MX_MCLENNAN *mclennan )
{
	static const char fname[] = "mxd_mclennan_parse_qa_response()";

	char response[80];
	char *ptr;
	int num_items;
	mx_status_type mx_status;

	/* Annoyingly the PM301, PM341, and PM381 do not seem to provide
	 * an equivalent to the QS command, so one is forced to parse
	 * the output of the QA command.  Ick.
	 */

	mx_warning(
"The %s routine has never been tested with a real PM301, PM341, or PM381.",
		fname );

	mx_status = mxd_mclennan_command( mclennan, "QA",
						NULL, 0, MCLENNAN_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read response lines from the controller until we see the last line
	 * which should be 'Auto Execute'.  If the last line is _not_
	 * 'Auto Execute', then we are probably doomed.
	 */

	for (;;) {
		mx_status = mx_rs232_getline( mclennan->rs232_record,
			response, sizeof(response), NULL, MCLENNAN_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( strncmp( response, "Auto Execute", 12 ) == 0 ) {
			break;		/* Exit the for(;;) loop. */
		} else
		if ( strncmp( response, "Slew Speed", 10 ) == 0 ) {
			ptr = strchr( response, '=' );

			if ( ptr == NULL ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The slew speed for motor '%s' was unparseable.  Response = '%s'.",
					motor->record->name, response );
			}
			ptr++;

			num_items = sscanf( ptr, "%lg", &(motor->raw_speed) );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The slew speed for motor '%s' was unparseable.  Response = '%s'.",
					motor->record->name, response );
			}
		} else
		if ( strncmp( response, "Base Speed", 10 ) == 0 ) {
			ptr = strchr( response, '=' );

			if ( ptr == NULL ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The base speed for motor '%s' was unparseable.  Response = '%s'.",
					motor->record->name, response );
			}
			ptr++;

			num_items = sscanf( ptr, "%lg",
						&(motor->raw_base_speed) );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The base speed for motor '%s' was unparseable.  Response = '%s'.",
					motor->record->name, response );
			}
		} else
		if ( strncmp( response, "Acceleration", 12 ) == 0 ) {
			ptr = strchr( response, '=' );

			if ( ptr == NULL ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The acceleration for motor '%s' was unparseable.  Response = '%s'.",
					motor->record->name, response );
			}
			ptr++;

			num_items = sscanf( ptr, "%lg",
				&(motor->raw_acceleration_parameters[0]) );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The acceleration for motor '%s' was unparseable.  Response = '%s'.",
					motor->record->name, response );
			}
			motor->raw_acceleration_parameters[1]
				= motor->raw_acceleration_parameters[0];
		}
	}

	/* Just in case, we discard any further input from the controller. */

	mx_status = mx_rs232_discard_unread_input( mclennan->rs232_record,
							MCLENNAN_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mclennan_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mclennan_get_parameter()";

	MX_MCLENNAN *mclennan;
	char response[80];
	double dummy;
	int num_items;
	mx_status_type mx_status;

	mclennan = NULL;

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		switch( mclennan->controller_type ) {
		case MXT_MCLENNAN_PM301:
		case MXT_MCLENNAN_PM341:
		case MXT_MCLENNAN_PM381:
			mx_status = mxd_mclennan_parse_qa_response(
					motor, mclennan );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;
		case MXT_MCLENNAN_PM304:
		case MXT_MCLENNAN_PM600:
			mx_status = mxd_mclennan_command( mclennan, "QS",
				response, sizeof(response), MCLENNAN_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( mclennan->controller_type == MXT_MCLENNAN_PM600 ) {
				num_items = sscanf( response,
					"SC=%lg,SV=%lg,SA=%lg,SD=%lg",
					&dummy,
					&( motor->raw_speed ),
				    &( motor->raw_acceleration_parameters[0] ),
				    &( motor->raw_acceleration_parameters[1] ));

				if ( num_items != 4 ) {
					return mx_error(
						MXE_DEVICE_IO_ERROR, fname,
"Unable to parse response to a QS command for motor '%s'.  Response = '%s'",
						motor->record->name, response );
				}
				motor->raw_acceleration_parameters[2] = 0.0;
			} else {
				num_items = sscanf( response,
					"SV=%lg,SC=%lg,SA=%lg,SD=%lg",
					&( motor->raw_speed ),
					&dummy,
				    &( motor->raw_acceleration_parameters[0] ),
				    &( motor->raw_acceleration_parameters[1] ));

				if ( num_items != 4 ) {
					return mx_error(
						MXE_DEVICE_IO_ERROR, fname,
"Unable to parse response to a QS command for motor '%s'.  Response = '%s'",
						motor->record->name, response );
				}
			}

			/* This driver ignores the base speed. */

			motor->raw_base_speed = 0.0;
			motor->base_speed = 0.0;

			motor->raw_acceleration_parameters[2] = 0.0;
			motor->raw_acceleration_parameters[3] = 0.0;

			break;
		}
		motor->speed = fabs(motor->scale) * motor->raw_speed;
		motor->base_speed = fabs(motor->scale) * motor->raw_base_speed;
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		switch( mclennan->controller_type ) {
		case MXT_MCLENNAN_PM301:
		case MXT_MCLENNAN_PM341:
		case MXT_MCLENNAN_PM381:
			motor->raw_maximum_speed = 99999; /* steps per second */
			break;
		case MXT_MCLENNAN_PM304:
		case MXT_MCLENNAN_PM600:
			motor->raw_maximum_speed = 400000;/* steps per second */
			break;
		}
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
	case MXLV_MTR_INTEGRAL_GAIN:
	case MXLV_MTR_DERIVATIVE_GAIN:
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
	case MXLV_MTR_EXTRA_GAIN:
		switch( mclennan->controller_type ) {
		case MXT_MCLENNAN_PM304:
		case MXT_MCLENNAN_PM600:
			mx_status = mxd_mclennan_command( mclennan, "QK",
				response, sizeof(response), MCLENNAN_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response,
				"KP=%lg,KS=%lg,KV=%lg,KF=%lg,KX=%lg",
				&(motor->proportional_gain),
				&(motor->integral_gain),
				&(motor->derivative_gain),
				&(motor->velocity_feedforward_gain),
				&(motor->extra_gain) );

			if ( num_items != 5 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Unable to parse response to a QK command for motor '%s'.  Response = '%s'",
					motor->record->name, response );
			}
			motor->acceleration_feedforward_gain = 0.0;
			break;
		default:
			motor->proportional_gain = 0.0;
			motor->integral_gain = 0.0;
			motor->derivative_gain = 0.0;
			motor->velocity_feedforward_gain = 0.0;
			motor->acceleration_feedforward_gain = 0.0;
			break;
		}

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mclennan_set_parameter()";

	MX_MCLENNAN *mclennan;
	char command[20];
	mx_status_type mx_status;

	mclennan = NULL;

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		sprintf( command, "SV%ld", mx_round( motor->raw_speed ) );

		mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );
		break;

	case MXLV_MTR_BASE_SPEED:
		if ( (mclennan->controller_type == MXT_MCLENNAN_PM301)
		  || (mclennan->controller_type == MXT_MCLENNAN_PM341)
		  || (mclennan->controller_type == MXT_MCLENNAN_PM381) )
		{
			sprintf( command, "SB%ld",
					mx_round( motor->raw_base_speed ) );

			mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );
		}
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		sprintf( command, "SA%ld",
			mx_round( motor->raw_acceleration_parameters[0] ));

		mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (mclennan->controller_type == MXT_MCLENNAN_PM600)
		  || (mclennan->controller_type == MXT_MCLENNAN_PM304) )
		{
			sprintf( command, "SD%ld",
			    mx_round( motor->raw_acceleration_parameters[1] ));

			mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );
		}
		break;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			mx_status = mxd_mclennan_send_reset( mclennan, TRUE );
		} else {
			mx_status = mxd_mclennan_send_reset( mclennan, FALSE );
		}
		break;

	case MXLV_MTR_CLOSED_LOOP:
		if ( motor->closed_loop ) {
			mx_status = mxd_mclennan_send_reset( mclennan, TRUE );
		} else {
			mx_status = mxd_mclennan_send_reset( mclennan, FALSE );
		}
		break;

	case MXLV_MTR_FAULT_RESET:
		if ( motor->fault_reset ) {
			mx_status = mxd_mclennan_send_reset( mclennan, TRUE );
		} else {
			mx_status = mxd_mclennan_send_reset( mclennan, FALSE );
		}
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		switch( mclennan->controller_type ) {
		case MXT_MCLENNAN_PM600:
		case MXT_MCLENNAN_PM304:
			sprintf( command, "KP%ld",
				mx_round( motor->proportional_gain ) );

			mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );
			break;
		default:
			motor->proportional_gain = 0.0;
			break;
		}
		break;

	case MXLV_MTR_INTEGRAL_GAIN:

		/* This routine uses the "sum" gain as a standin for the
		 * integral gain.  According to the manual the sum gain
		 * is really the sum of the integral and proportional
		 * components of the servo loop.  Goodness only knows why
		 * they did it this way, but the manual seems to indicate
		 * the sum gain is to be used in a manner similar to the
		 * way integral gains are normally used.
		 */

		switch( mclennan->controller_type ) {
		case MXT_MCLENNAN_PM600:
		case MXT_MCLENNAN_PM304:
			sprintf( command, "KS%ld",
				mx_round( motor->integral_gain ) );

			mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );
			break;
		default:
			motor->proportional_gain = 0.0;
			break;
		}
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		switch( mclennan->controller_type ) {
		case MXT_MCLENNAN_PM600:
		case MXT_MCLENNAN_PM304:
			sprintf( command, "KV%ld",
				mx_round( motor->derivative_gain ) );

			mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );
			break;
		default:
			motor->derivative_gain = 0.0;
			break;
		}
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		switch( mclennan->controller_type ) {
		case MXT_MCLENNAN_PM600:
		case MXT_MCLENNAN_PM304:
			sprintf( command, "KF%ld",
				mx_round( motor->velocity_feedforward_gain ) );

			mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );
			break;
		default:
			motor->velocity_feedforward_gain = 0.0;
			break;
		}
		break;

	case MXLV_MTR_EXTRA_GAIN:
		switch( mclennan->controller_type ) {
		case MXT_MCLENNAN_PM600:
		case MXT_MCLENNAN_PM304:
			sprintf( command, "KX%ld",
				mx_round( motor->extra_gain ) );

			mx_status = mxd_mclennan_command( mclennan, command,
						NULL, 0, MCLENNAN_DEBUG );
			break;
		default:
			motor->extra_gain = 0.0;
			break;
		}
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}
	return mx_status;
}

static mx_status_type
mxd_mclennan_send_reset( MX_MCLENNAN *mclennan, int do_reset )
{
	mx_status_type mx_status;

	/* Send the reset command. */

	mx_status = mxd_mclennan_command( mclennan, "RS",
						NULL, 0, MCLENNAN_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If this is a Mclennan PM304, we must send more commands. */

	if ( mclennan->controller_type == MXT_MCLENNAN_PM304 ) {

		/* Reset from emergency stop. */

		mx_status = mxd_mclennan_command( mclennan, "RSES",
						NULL, 0, MCLENNAN_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Reset from motor stalled. */

		mx_status = mxd_mclennan_command( mclennan, "RSST",
						NULL, 0, MCLENNAN_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_mclennan_parse_pm600_status( MX_MOTOR *motor,
				MX_MCLENNAN *mclennan,
				char *response,
				long max_response_length )
{
	char *ptr;
	int i, c, length;
	mx_status_type mx_status;

	/* Please note that Mclennan's definition of the term 'busy'
	 * is not the same as MX's definition of the term 'busy'.
	 * For example, if a PM600 is stopped at a hard limit, Mclennan
	 * would say that the motor is 'busy', but MX would say that the
	 * motor is _not_ 'busy'.  For consistency, we use MX's definition
	 * of the term here.
	 */

	motor->status = 0;

	/* a: Controller is busy/idle */

	if ( response[0] == '0' ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* c: Upper hard limit status */

	if ( response[2] == '1' ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;

		/* Force off the busy bit. */

		motor->status &= ~(MXSF_MTR_IS_BUSY);
	}
		
	/* d: Lower hard limit status */

	if ( response[3] == '1' ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;

		/* Force off the busy bit. */

		motor->status &= ~(MXSF_MTR_IS_BUSY);
	}
		
	/* e: Jogging or joystick moving. */

	if ( response[4] == '1' ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* f: On datum sensor point. */

	if ( response[5] == '1' ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}

	/* Did some sort of error occur?  If so, find out what
	 * the nature of the error was.
	 */

	if ( response[1] == '1' ) {
		motor->status |= MXSF_MTR_ERROR;

		/* The only way I see to figure out what happened is to
		 * parse the output of the CO command.
		 */

		mx_status = mxd_mclennan_command( mclennan, "CO",
			response, max_response_length, MCLENNAN_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Skip over the leading 'Mode = ' text if present. */

		ptr = strchr( response, '=' );

		if ( ptr == NULL ) {
			ptr = response;
		} else {
			ptr++;
		}

		/* The word Abort seems to be inconsistently capitalized
		 * in the manual.  Just to be safe, we force the response
		 * string to lower case.
		 */

		length = (int) strlen(ptr);

		for ( i = 0; i < length; i++ ) {
			c = (int) ptr[i];

			if ( isupper(c) ) {
				ptr[i] = (char) tolower(c);
			}
		}

		if ( strncmp( ptr, "command abort", 13 ) == 0 ) {
			motor->status |= MXSF_MTR_AXIS_DISABLED;
		} else 
		if ( strncmp( ptr, "input abort", 11 ) == 0 ) {
			motor->status |= MXSF_MTR_AXIS_DISABLED;
		} else 
		if ( strncmp( ptr, "stall abort", 11 ) == 0 ) {
			motor->status |= MXSF_MTR_FOLLOWING_ERROR;
		} else 
		if ( strncmp( ptr, "tracking abort", 14 ) == 0 ) {
			motor->status |= MXSF_MTR_FOLLOWING_ERROR;
		} else 
		if ( strncmp( ptr, "not complete", 12 ) == 0 ) {
			/* Timeout Abort */

			motor->status |= MXSF_MTR_DRIVE_FAULT;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_mclennan_parse_pm304_status( MX_MOTOR *motor,
				MX_MCLENNAN *mclennan,
				char *response,
				size_t max_response_length )
{
	motor->status = 0;

	/* 0: Negative hard limit. */

	if ( response[0] == '1' ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* 1: Positive hard limit. */

	if ( response[1] == '1' ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* 2: Not error. */

	if ( response[2] == '0' ) {
		motor->status |= MXSF_MTR_ERROR;
	}

	/* 3: Controller idle. */

	if ( response[3] == '0' ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* 4: User abort. */

	if ( response[4] == '1' ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	/* 5: Tracking abort. */

	if ( response[5] == '1' ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* 6: Motor stalled. */

	if ( response[6] == '1' ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* 7: Emergency stop. */

	if ( response[7] == '1' ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_mclennan_parse_pm3x1_status( MX_MOTOR *motor,
				MX_MCLENNAN *mclennan,
				char *response,
				size_t max_response_length )
{
	motor->status = 0;

	/* Bit 9: Error occurred. */

	if ( response[0] == '1' ) {
		motor->status |= MXSF_MTR_ERROR;
	}

	/* Bit 8: Auto correct mode. (ignored) */

	/* Bit 7: Lower hard limit. */

	if ( response[2] == '1' ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* Bit 6: Upper hard limit. */

	if ( response[3] == '1' ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* Bit 5: Motor stalled. */

	if ( response[4] == '1' ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 4: Move not within window. */

	if ( response[5] == '1' ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 3: Busy (not idle). */

	if ( response[6] == '1' ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* Bit 2: Stop line active. */

	if ( response[7] == '1' ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	/* Bit 1: Loop active mode. */

	if ( response[8] == '0' ) {
		if ( mclennan->controller_type == MXT_MCLENNAN_PM341 ) {
			motor->status |= MXSF_MTR_AXIS_DISABLED;
		}
	}

	/* Bit 0: Jogs active. */

	if ( response[9] == '1' ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mclennan_get_status()";

	MX_MCLENNAN *mclennan;
	char response[80];
	mx_status_type mx_status;

	mclennan = NULL;

	mx_status = mxd_mclennan_get_pointers( motor, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_mclennan_command( mclennan, "OS",
			response, sizeof(response), MCLENNAN_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( mclennan->controller_type ) {
	case MXT_MCLENNAN_PM600:
		mx_status = mxd_mclennan_parse_pm600_status( motor, mclennan,
						response, sizeof(response) );
		break;

	case MXT_MCLENNAN_PM304:
		mx_status = mxd_mclennan_parse_pm304_status( motor, mclennan,
						response, sizeof(response) );
		break;

	case MXT_MCLENNAN_PM301:
	case MXT_MCLENNAN_PM341:
	case MXT_MCLENNAN_PM381:
		mx_status = mxd_mclennan_parse_pm3x1_status( motor, mclennan,
						response, sizeof(response) );
		break;
	}

	return mx_status;
}

/* === Extra functions for the use of this driver. === */

MX_EXPORT mx_status_type
mxd_mclennan_command( MX_MCLENNAN *mclennan, char *command, char *response,
		long response_buffer_length, int debug_flag )
{
	static const char fname[] = "mxd_mclennan_command()";

	char prefix[20], local_response_buffer[100];
	char *ptr, *response_ptr;
	int response_prefix, prefix_separator;
	long buffer_length, prefix_length, chars_to_copy;
	long command_length, response_length;
	mx_status_type mx_status;

	if ( mclennan == (MX_MCLENNAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MCLENNAN pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'command' pointer passed was NULL." );
	}

	/* Send the axis number prefix. */

	sprintf( prefix, "%ld", mclennan->axis_number );

	if ( debug_flag ) {
		MX_DEBUG(-2, ("mxd_mclennan_command: command = '%s%s'",
					prefix, command));
	}

	prefix_length = (int) strlen( prefix );

	command_length = prefix_length + (int) strlen( command );

	mx_status = mx_rs232_write( mclennan->rs232_record,
				prefix, prefix_length,
				NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the command string. */

	mx_status = mx_rs232_putline( mclennan->rs232_record,
					command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response. */

	if ( response == NULL ) {
		response_ptr = local_response_buffer;
		buffer_length = sizeof( local_response_buffer ) - 1;
	} else {
		response_ptr = response;
		buffer_length = response_buffer_length;
	}

	mx_status = mx_rs232_getline( mclennan->rs232_record,
					response_ptr, buffer_length, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The PM600 echoes back the transmitted characters.  We must discard
	 * 'command_length + 1' received characters, where the extra character
	 * is the carriage return appended to the end of the string.
	 */

	if ( mclennan->controller_type == MXT_MCLENNAN_PM600 ) {
		response_ptr += command_length + 1;
	}

	/* Delete any trailing line terminators. */

	response_length = (int) strlen( response_ptr );

	if ( response_ptr[response_length - 1] == MX_LF ) {
		response_ptr[ response_length - 1 ] = '\0';

		if ( response_ptr[ response_length - 2 ] == MX_CR ) {
			response_ptr[ response_length - 2 ] = '\0';
		}
	}

#if 0
	response_length = strlen( response_ptr );

	{
		int i;

		for ( i = 0; i < response_length; i++ ) {
			MX_DEBUG(-2,("%s: response_ptr[%d] = %#x '%c'",
				fname, i, response_ptr[i], response_ptr[i]));
		}
	}
#endif

	if ( debug_flag ) {
		MX_DEBUG(-2, ("mxd_mclennan_command: response_ptr = '%s'",
					response_ptr ));
	}

	/* Find the prefix separator in the response. */

	switch( mclennan->controller_type ) {
	case MXT_MCLENNAN_PM600:
	case MXT_MCLENNAN_PM304:
		prefix_separator = ':';
		break;
	case MXT_MCLENNAN_PM301:
	case MXT_MCLENNAN_PM341:
	case MXT_MCLENNAN_PM381:
		prefix_separator = '#';
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported Mclennan controller type %ld for motor '%s'",
			mclennan->controller_type, mclennan->record->name );
	}

	ptr = strchr( response_ptr, prefix_separator );

	if ( ptr == NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Prefix separator '%c' not seen in response from motor '%s'.  "
		"Response = '%s'",
		prefix_separator, mclennan->record->name, response_ptr );
	}

	/* Null terminate the prefix. */

	*ptr = '\0';

	/* See if the response prefix matches the original prefix. */

	response_prefix = atoi( response_ptr );

	if ( response_prefix != mclennan->axis_number ) {
		*ptr = prefix_separator;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The axis number %d in the response '%s' for motor '%s' does not "
	"match the expected axis number of %ld.",
			response_prefix, response_ptr,
			mclennan->record->name,
			mclennan->axis_number );
	}

	ptr++;

	/* Check to see if the Mclennan responded with an error message. */

	if ( *ptr == '!' ) {
		mx_status = mxd_mclennan_handle_error( mclennan, command, ptr );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* If the response pointer passed was not null, move the body
	 * of the response to the beginning of the response buffer.
	 */

	if ( response != NULL ) {
		chars_to_copy = (int) strlen( ptr );

		if ( chars_to_copy > response_buffer_length - 1 ) {
			chars_to_copy = response_buffer_length - 1;
		}

		memmove( response, ptr, chars_to_copy );

		response[chars_to_copy] = '\0';
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_mclennan_handle_error( MX_MCLENNAN *mclennan,
				char *command,
				char *error_message )
{
	static const char fname[] = "mxd_mclennan_handle_error()";

	/* First, ignore "error" conditions that are not real errors
	 * like sending a reset when the controller is not currently
	 * in an abort state.
	 */

	if ( strcmp( error_message, "! NOT ABORTED" ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( strcmp( error_message, "! NOT STALLED" ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( strcmp( error_message, "! NOT STOPPED" ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Return a generic response for everything else. */

	return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error occurred for command '%ld%s' sent to motor '%s'.  "
		"Error message = '%s'.",
			mclennan->axis_number, command,
			mclennan->record->name, error_message );
}


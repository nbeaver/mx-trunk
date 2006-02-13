/*
 * Name:    d_panther.c 
 *
 * Purpose: MX motor driver to control Panther HI & HE motor controllers 
 *          made by Intelligent Motion Systems, Inc. of Marlborough,
 *          Connecticut.
 *
 *          This driver supports both Single Mode where only one axis
 *          is available and the Party Line Mode where multiple Panther
 *          controllers are connected to a single RS-422 line.  The primary 
 *          difference is that in Party Line Mode all of the Panther 
 *          commands are preceded by the single character axis name.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PANTHER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "d_panther.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_panther_record_function_list = {
	NULL,
	mxd_panther_create_record_structures,
	mxd_panther_finish_record_initialization,
	NULL,
	mxd_panther_print_motor_structure,
	mxd_panther_read_parms_from_hardware,
	mxd_panther_write_parms_to_hardware,
	mxd_panther_open,
	mxd_panther_close,
	NULL,
	mxd_panther_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_panther_motor_function_list = {
	mxd_panther_motor_is_busy,
	mxd_panther_move_absolute,
	mxd_panther_get_position,
	mxd_panther_set_position,
	mxd_panther_soft_abort,
	mxd_panther_immediate_abort,
	mxd_panther_positive_limit_hit,
	mxd_panther_negative_limit_hit,
	mxd_panther_find_home_position,
	mxd_panther_constant_velocity_move,
	mxd_panther_get_parameter,
	mxd_panther_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_panther_hi_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PANTHER_HI_STANDARD_FIELDS
};

mx_length_type mxd_panther_hi_num_record_fields
		= sizeof( mxd_panther_hi_record_field_defaults )
			/ sizeof( mxd_panther_hi_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_panther_hi_rfield_def_ptr
			= &mxd_panther_hi_record_field_defaults[0];

MX_RECORD_FIELD_DEFAULTS mxd_panther_he_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PANTHER_HI_STANDARD_FIELDS,
	MXD_PANTHER_HE_STANDARD_FIELDS
};

mx_length_type mxd_panther_he_num_record_fields
		= sizeof( mxd_panther_he_record_field_defaults )
			/ sizeof( mxd_panther_he_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_panther_he_rfield_def_ptr
			= &mxd_panther_he_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_panther_get_pointers( MX_MOTOR *motor,
			MX_PANTHER **panther,
			const char *calling_fname )
{
	static const char fname[] = "mxd_panther_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( panther == (MX_PANTHER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PANTHER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*panther = (MX_PANTHER *) (motor->record->record_type_struct);

	if ( *panther == (MX_PANTHER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PANTHER pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_panther_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_panther_create_record_structures()";

	MX_MOTOR *motor;
	MX_PANTHER *panther;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	panther = (MX_PANTHER *) malloc( sizeof(MX_PANTHER) );

	if ( panther == (MX_PANTHER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PANTHER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = panther;
	record->class_specific_function_list
				= &mxd_panther_motor_function_list;

	motor->record = record;

	/* An Panther motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_panther_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_PANTHER *panther;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Don't need to check the validity of the motor pointer since
	 * mx_motor_finish_record_initialization() will have done it
	 * for us.
	 */

	motor = (MX_MOTOR *) record->record_class_struct;

	/* But we should check the panther pointer, just in case something
	 * has been overwritten.
	 */

	panther = (MX_PANTHER *) record->record_type_struct;

	if ( panther == (MX_PANTHER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PANTHER pointer for record '%s' is NULL.",
			record->name );
	}

	/* Set the initial speeds and accelerations. */

	motor->raw_speed = (double) panther->default_speed;
	motor->raw_base_speed = (double) panther->default_base_speed;
	motor->raw_maximum_speed = 20000.0;   /* See the Panther V command. */

	motor->speed = fabs(motor->scale) * motor->raw_speed;
	motor->base_speed = fabs(motor->scale) * motor->raw_base_speed;
	motor->maximum_speed = fabs(motor->scale) * motor->raw_maximum_speed;

	motor->raw_acceleration_parameters[0]
			= (double) panther->acceleration_slope;

	motor->raw_acceleration_parameters[1]
			= (double) panther->deceleration_slope;

	motor->raw_acceleration_parameters[2] = 0.0;
	motor->raw_acceleration_parameters[3] = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_panther_print_motor_structure()";

	MX_MOTOR *motor;
	MX_PANTHER *panther;
	char axis_name[MXU_RECORD_NAME_LENGTH + 1];
	int32_t motor_steps;
	double position, backlash, negative_limit, positive_limit;
	double scale, offset, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	switch( record->mx_type ) {
	case MXT_MTR_PANTHER_HI:
		fprintf(file,
		      "  Motor type         = PANTHER HI.\n\n");
		break;
	case MXT_MTR_PANTHER_HE:
		fprintf(file,
		      "  Motor type         = PANTHER HE.\n\n");
		break;
	default:
		fprintf(file,
		      "  Motor type         = PANTHER (unknown).\n\n");
		break;
	}

	fprintf(file, "  name               = %s\n", record->name);
	fprintf(file, "  rs232              = %s\n",
					panther->rs232_record->name);

	if ( panther->axis_name == '0' ) {
		strcpy( axis_name, "single mode" );
	} else {
		axis_name[0] = panther->axis_name;
		axis_name[1] = '\0';
	}

	fprintf(file, "  axis               = %s\n", axis_name );

	mx_status = mx_motor_get_position_steps( record, &motor_steps );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	scale = motor->scale;
	offset = motor->offset;

	if ( panther->encoder_resolution > 0 ) {
		scale *= 50.0 / (double) panther->encoder_resolution;
	}

	position = offset + scale * (double) motor_steps;

	fprintf(file, "  position           = %ld steps (%g %s)\n",
		(long) motor_steps, position, motor->units);
	fprintf(file, "  scale              = %g %s per step.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset             = %g %s.\n",
		motor->offset, motor->units);

	backlash = motor->scale
		* (double) (motor->raw_backlash_correction.stepper);

	fprintf(file, "  backlash           = %ld steps (%g %s)\n",
		(long) motor->raw_backlash_correction.stepper,
		backlash, motor->units);

	negative_limit = motor->offset + motor->scale
		* (double)(motor->raw_negative_limit.stepper);

	fprintf(file, "  negative limit     = %ld steps (%g %s)\n",
		(long) motor->raw_negative_limit.stepper,
		negative_limit, motor->units);

	positive_limit = motor->offset + motor->scale
		* (double)(motor->raw_positive_limit.stepper);

	fprintf(file, "  positive limit     = %ld steps (%g %s)\n",
		(long) motor->raw_positive_limit.stepper,
		positive_limit, motor->units);

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband      = %ld steps (%g %s)\n\n",
		(long) motor->raw_move_deadband.stepper,
		move_deadband, motor->units);

	fprintf(file, "  default speed      = %d steps per sec.\n",
					panther->default_speed );
	fprintf(file, "  default base speed = %d steps per sec.\n",
					panther->default_base_speed );
	fprintf(file, "  accel. slope       = %d\n",
					panther->acceleration_slope);
	fprintf(file, "  decel. slope       = %d\n",
					panther->deceleration_slope);
	fprintf(file, "  divide factor      = %d\n",
					panther->microstep_divide_factor );
	fprintf(file, "  resolution mode    = %c\n",
					panther->step_resolution_mode );
	fprintf(file, "  hold current       = %d %%\n",
					panther->hold_current );
	fprintf(file, "  run current        = %d %%\n",
					panther->run_current );
	fprintf(file, "  settling time      = %d\n",
					panther->settling_time_delay );
	fprintf(file, "  limit polarity     = %d\n\n",
					panther->limit_polarity );

	/* Encoder parameters. */

	if ( record->mx_type == MXT_MTR_PANTHER_HE ) {
		if ( panther->encoder_resolution > 0 ) {
			fprintf(file, "  encoder is ENABLED.\n");
		} else {
			fprintf(file, "  encoder is DISABLED.\n");
		}

		fprintf(file,
		    "  encoder resolution = %d encoder counts per 1/4 rev.\n",
					panther->encoder_resolution );
		fprintf(file, "  deadband size      = %d encoder counts\n",
					panther->deadband_size );
		fprintf(file,
		    "  hunt velocity      = %d motor steps per sec.\n",
					panther->hunt_velocity );
		fprintf(file, "  hunt resolution    = %d\n",
					panther->hunt_resolution );
		fprintf(file, "  stall factor       = %d\n",
					panther->stall_factor );
		fprintf(file, "  stall sample rate  = %d\n",
					panther->stall_sample_rate );
		fprintf(file, "  max stall retries  = %d\n",
					panther->max_stall_retries );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_read_parms_from_hardware( MX_RECORD *record )
{
	static const char fname[] = "mxd_panther_read_parms_from_hardware()";

	MX_MOTOR *motor;
	MX_PANTHER *panther;
	char response[5][80];
	int32_t motor_steps;
	int num_items;
	int hold_current, run_current, settling_time_delay;
	int acceleration_slope, deceleration_slope;
	int default_base_speed, default_speed;
	int i, length2, length3;
	int encoder_resolution, deadband_size;
	int hunt_velocity, hunt_resolution_factor;
	int stall_factor, stall_sample_rate, max_stall_retries;
	uint32_t num_input_bytes_available;
	char step_resolution_mode_string[20];
	char *stall_string_ptr;
	char *ptr;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current position.  This has the side effect of
	 * updating the position value in the MX_MOTOR structure.
	 */

	mx_status = mx_motor_get_position_steps( record, &motor_steps );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* In Single Mode, reading out the parameters is easy. */

	if ( panther->axis_name == '0' ) {

		/* This is a case where the Panther indexer does not
		 * echo the transmitted end of string character.  The
		 * third argument of mxd_panther_putline() is set to
		 * FALSE to indicate this.
		 */

		mx_status = mxd_panther_putline( panther, "X",
						FALSE, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* ======================================================== */
		/* The Panther puts out 2, 4 or 5 lines of text in response.*/
		/* ======================================================== */

		/* Get the first two lines. */

		for ( i = 0; i < 2; i++ ) {
			mx_status = mxd_panther_getline( panther,
					response[i], sizeof(response[i]),
					PANTHER_DEBUG );

			if ( mx_status.code != MXE_SUCCESS ) {

				if ( mx_status.code == MXE_LIMIT_WAS_EXCEEDED ) {
					/* Wait for all chars to arrive. */
					mx_msleep(500);

					(void) mx_rs232_discard_unread_input(
							panther->rs232_record,
							PANTHER_DEBUG );
				}
				return mx_status;
			}
		}
		
		/* Is there a third line?  If there is, there must be
		 * a fourth line.
		 */

		mx_msleep(100);

		mx_status = mx_rs232_num_input_bytes_available(
						panther->rs232_record,
						&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_input_bytes_available > 0 ) {
			for ( i = 2; i < 4; i++ ) {
				mx_status = mxd_panther_getline( panther,
					response[i], sizeof response[i],
					PANTHER_DEBUG );

				if ( mx_status.code != MXE_SUCCESS ) {

					if ( mx_status.code ==
						    MXE_LIMIT_WAS_EXCEEDED ) {
						/* Wait for all chars
						 * to arrive.
						 */
						mx_msleep(500);

						(void)
						mx_rs232_discard_unread_input(
							panther->rs232_record,
							PANTHER_DEBUG );
					}
					return mx_status;
				}
			}
		
		} else {
			if ( panther->encoder_resolution > 0 ) {
				(void) mx_error( MXE_DEVICE_IO_ERROR, fname,
"Did not see a third line of encoder status in response to the X command.  "
"Disabling encoder support." );
			}
			strcpy( response[2], "" );
			strcpy( response[3], "" );

			panther->encoder_resolution = 0;
		}

		/* A fifth line exists only if both the third line and
		 * the fourth line were not empty and the fourth line
		 * does not start with the letter 's'.
		 */

		length2 = strlen( response[2] );
		length3 = strlen( response[3] );

		if ( length2 > 0 && length3 > 0 && response[3][0] != 's' ) {

			mx_status = mx_rs232_num_input_bytes_available(
					panther->rs232_record,
					&num_input_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_panther_getline( panther,
					response[i], sizeof response[i],
					PANTHER_DEBUG );

			if ( mx_status.code != MXE_SUCCESS ) {

				if ( mx_status.code == MXE_LIMIT_WAS_EXCEEDED ) {
					/* Wait for all chars to arrive. */
					mx_msleep(500);

					(void) mx_rs232_discard_unread_input(
							panther->rs232_record,
							PANTHER_DEBUG );
				}
				return mx_status;
			}
		}

		/* There shouldn't be any leftover characters, but just
		 * in case there are, we discard them.
		 */

		mx_status = mx_rs232_discard_unread_input( panther->rs232_record,
							PANTHER_DEBUG);
		
		/* Now parse the response strings. */

		/* === First line === */

		num_items = sscanf( response[0],
			" Y= %d/%d E= %d K= %d/%d H=%s ",
			&hold_current,
			&run_current,
			&settling_time_delay,
			&acceleration_slope,
			&deceleration_slope,
			step_resolution_mode_string );

		if ( num_items != 6 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Error parsing X response line 1, num_items = %d, response = '%s'",
				num_items, response[0] );
		}

		panther->hold_current = hold_current;
		panther->run_current = run_current;
		panther->settling_time_delay = settling_time_delay;
		panther->acceleration_slope = acceleration_slope;
		panther->deceleration_slope = deceleration_slope;

		motor->raw_acceleration_parameters[0]
				= (double) acceleration_slope;

		motor->raw_acceleration_parameters[1]
				= (double) deceleration_slope;

		if ( strcmp( "fr", step_resolution_mode_string ) == 0 ) {
			panther->step_resolution_mode
			= MX_PANTHER_FIXED_RESOLUTION;
		} else
		if ( strcmp( "vr", step_resolution_mode_string ) == 0 ) {
			panther->step_resolution_mode
			= MX_PANTHER_VARIABLE_RESOLUTION;
		} else {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Error reading step resolution mode; read '%s'",
				step_resolution_mode_string );
		}

		/* === Second line === */

		ptr = strchr( response[1], 'I' );

		if ( ptr == NULL ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Error parsing X response line 2, 'I' was not found.  Response = '%s'",
				response[1] );
		}

		num_items = sscanf( ptr,
			"I= %d %*s %*s %*s V= %d ",
			&default_base_speed, &default_speed );

		if ( num_items != 2 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Error parsing X response line 2, num_items = %d, response = '%s'",
				num_items, response[1] );
		}

		panther->default_speed = default_speed;
		panther->default_base_speed = default_base_speed;

		motor->raw_speed = (double) default_speed;
		motor->raw_base_speed = (double) default_base_speed;

		motor->speed = fabs(motor->scale) * motor->raw_speed;
		motor->base_speed = fabs(motor->scale) * motor->raw_base_speed;

		/* === Third line (encoder resolution) === */

		if ( strlen(response[2]) > 0 ) {

			num_items = sscanf( response[2],
				"e = %d", &encoder_resolution );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Error parsing X response line 3, num_items = %d, response = '%s'",
					num_items, response[2] );
			}

			panther->encoder_resolution = encoder_resolution;
		} else {
			panther->encoder_resolution = 0;
		}

		/* === Fourth and fifth lines === */

		stall_string_ptr = NULL;

		if ( strlen(response[3]) > 0 ) {

			if ( response[3][0] == 'd' ) {

				num_items = sscanf( response[3],
					"d= %d v= %d /%d",
					&deadband_size,
					&hunt_velocity,
					&hunt_resolution_factor );

				if ( num_items != 3 ) {
					return mx_error(
						MXE_UNPARSEABLE_STRING, fname,
"Error parsing X response line 4 for 'd', num_items = %d, response = '%s'",
						num_items, response[3] );
				}

				panther->deadband_size = deadband_size;
				panther->hunt_velocity = hunt_velocity;

				panther->hunt_resolution = mx_round(
					log( hunt_resolution_factor )
					/ log( 2.0 ) );

				stall_string_ptr = response[4];

			} else if ( response[3][0] == 's' ) {

				panther->deadband_size = 0;

				stall_string_ptr = response[3];

			} else if ( response[3][0] == ' ' ) {

				panther->deadband_size = 0;
				panther->stall_sample_rate = 0;

				stall_string_ptr = NULL;

			} else {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Error parsing X response line 4, num_items = %d, response = '%s'",
					num_items, response[3] );
			}
		}

		if ( stall_string_ptr != NULL ) {

			if ( strlen( stall_string_ptr ) == 0 ) {

				panther->stall_sample_rate = 0;

			} else if ( stall_string_ptr[0] == ' ' ) {

				panther->stall_sample_rate = 0;

			} else {
				num_items =  sscanf( stall_string_ptr,
					"s= %d t= %d r= %d",
					&stall_factor,
					&stall_sample_rate,
					&max_stall_retries );

				if ( num_items != 3 ) {
					return mx_error(
						MXE_UNPARSEABLE_STRING, fname,
"Error parsing X response stall parameters, num_items = %d, response = '%s'",
						num_items, stall_string_ptr );
				}

				panther->stall_factor = stall_factor;
				panther->stall_sample_rate = stall_sample_rate;
				panther->max_stall_retries = max_stall_retries;
			}
		}

	} else {	/* Party Line mode. */

		/* In Party Line Mode, reading the current parameters
		 * from the hardware is difficult.  The manual suggests
		 * that one can read the values from non-volatile RAM,
		 * but then goes on to say that the values there are
		 * only valid after an 'S' (store parameters to the EEPROM)
		 * command or a power-on reset.  Using the 'S' command 
		 * is out, since the manual says that writing to the
		 * EEPROM slowly wears it out.  Doing a power on reset
		 * is out, since the ^C command affects _all_ axes,
		 * not just the one you are interested in.
		 *
		 * For now, we just live with only being able to
		 * read out the position, which is done at the top
		 * of this function.	(W. Lavender, 02/19/96)
		 */
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_write_parms_to_hardware( MX_RECORD *record )
{
	static const char fname[] = "mxd_panther_write_parms_to_hardware()";

	MX_MOTOR *motor;
	MX_PANTHER *panther;
	char command[50];
	char response[80];
	char c;
	int i;
	int num_items;
	int update_encoder_resolution;
	int old_encoder_resolution, limit_polarity;
	uint32_t num_input_bytes_available;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the setup commands.  The calls to mxd_panther_getc()
	 * are to eat a line feed character that is sent back by the
	 * indexer in Single mode.  In Party Line mode, no characters
	 * are echoed back at all.
	 */

	/* === Base speed === */

	sprintf( command, "I %d", panther->default_base_speed );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Slew speed === */

	sprintf( command, "V %d", panther->default_speed );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Acceleration and deceleration slopes === */

	sprintf( command, "K %d %d",
		panther->acceleration_slope, panther->deceleration_slope );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Microstep divide factor === */

	sprintf( command, "D %d", panther->microstep_divide_factor );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Step resolution mode === */

	c = panther->step_resolution_mode;

	switch( c ) {
	case MX_PANTHER_FIXED_RESOLUTION:
		sprintf( command, "H 0" );
		break;
	case MX_PANTHER_VARIABLE_RESOLUTION:
		sprintf( command, "H 1" );
		break;
	default:
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Unknown step resolution type = '%c'", c );
	}

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Hold and run currents === */

	sprintf( command, "Y %d %d",
		panther->hold_current, panther->run_current );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Settling time delay === */

	sprintf( command, "E %d", panther->settling_time_delay );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Limit polarity === */

	/* Limits are expected to be disabled if limit_polarity < 0. */

	if ( panther->limit_polarity >= 0 ) {
		limit_polarity = panther->limit_polarity;
	} else {
		limit_polarity = 0;
	}

	sprintf( command, "l %d", limit_polarity );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*======================================================*
	 *                  Encoder Parameters                  *
	 *======================================================*/

	/* If the indexer is a Panther HI, then there is no use
	 * in trying to send encoder commands to the indexer.
	 */

	if ( record->mx_type == MXT_MTR_PANTHER_HI ) {

		panther->encoder_resolution = 0;
		panther->deadband_size = 0;
		panther->hunt_velocity = 0;
		panther->hunt_resolution = 0;
		panther->stall_factor = 0;
		panther->stall_sample_rate = 0;
		panther->max_stall_retries = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	/* === Deadband size === */

	sprintf( command, "d %d", panther->deadband_size );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Hunt velocity === */

	sprintf( command, "v %d", panther->hunt_velocity );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Hunt resolution === */

	sprintf( command, "h %d", panther->hunt_resolution );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Stall factor === */

	sprintf( command, "s %d", panther->stall_factor );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Stall sample rate === */

	sprintf( command, "t %d", panther->stall_sample_rate );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* === Maximum stall retries === */

	sprintf( command, "r %d", panther->max_stall_retries );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* The encoder resolution is set last, since setting this
	 * turns on the encoder.  In addition, it is important to
	 * not send a command to turn on the encoder, if the encoder
	 * is already on.  This is because if you do, the position
	 * value for the encoder is reset to zero.  Since there is
	 * no easy way to tell if the encoder is on in Party Line
	 * Mode, we can only turn it on automatically in Single Mode.
	 * It is always safe to turn off the encoder.
	 *
	 * In Single Mode, it is simple to find out whether or not
	 * the encoder is on or not by sending an 'X' command to 
	 * the indexer.  If you only get 2 lines back, the encoder
	 * is off.  If you get 3 or more lines back, the encoder is on.
	 */

	update_encoder_resolution = FALSE;

	if ( panther->encoder_resolution == 0 ) {
		update_encoder_resolution = TRUE;
	} else {
		if ( panther->axis_name != '0' ) {  /* Party Line Mode. */

			update_encoder_resolution = FALSE;
		} else {
			/* Single Mode. */

			mx_status = mxd_panther_putline(panther, "X",
							FALSE, PANTHER_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Read back the first two lines. */

			for ( i = 0; i < 2; i++ ) {
				mx_status = mxd_panther_getline( panther,
				response, sizeof(response), PANTHER_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

			/* Is there any more? */

			mx_msleep(100);

			mx_status = mx_rs232_num_input_bytes_available(
						panther->rs232_record,
						&num_input_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_input_bytes_available == 0 ) {
				MX_DEBUG(-2,("*** The encoder was not on."));

				update_encoder_resolution = TRUE;
			} else {
				MX_DEBUG(-2,("*** The encoder was on."));

				/* Get the old encoder resolution. */

				mx_status = mxd_panther_getline( panther,
				response, sizeof(response), PANTHER_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				num_items = sscanf( response, "e = %d",
						&old_encoder_resolution );

				if ( num_items != 1 ) {
					return mx_error(
						MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse encoder resolution in response to an X command.  "
	"The offending line = '%s'",
						response );
				}

				if ( old_encoder_resolution
				  == panther->encoder_resolution ) {

					update_encoder_resolution = FALSE;
				} else {

					update_encoder_resolution = TRUE;
				}
			}
		}
	}

	/* Wait for all the characters in the response to the "X" command
	 * to arrive.
	 */

	mx_msleep(500);

	/* Discard them all. */

	mx_status = mx_rs232_discard_unread_input( panther->rs232_record,
						PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now update the resolution if possible. */

	if ( update_encoder_resolution == TRUE ) {
		sprintf( command, "e %d", panther->encoder_resolution );

		mx_status = mxd_panther_putline( panther, command,
						TRUE, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( panther->axis_name == '0' ) {
			mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	} else {
		MX_DEBUG(-2,("Encoder resolution was not changed."));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_open( MX_RECORD *record )
{
	return mxd_panther_resynchronize( record );
}

MX_EXPORT mx_status_type
mxd_panther_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_panther_resynchronize()";

	MX_MOTOR *motor;
	MX_PANTHER *panther;
	char response[80];
	char c;
	char *p;
	int new_version;
	int i, max_retries, signon_message_seen;
	int num_items, indexer_version, encoder_version;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Flush out any unwritten and unread characters before sending
	 * commands to the Panther indexer.
	 */

	mx_status = mx_rs232_discard_unwritten_output( panther->rs232_record,
							PANTHER_DEBUG );

	if (mx_status.code != MXE_SUCCESS && mx_status.code != MXE_UNSUPPORTED)
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( panther->rs232_record,
							PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Our first task is to find out what state the indexer is in.
	 * In Party Line Mode, the indexer is ready to receive commands
	 * as soon as it has been powered on.  However, in Single Mode,
	 * we are required to go through a sign on sequence just after
	 * power is applied.
	 */

	/* Only need to do this in Single Mode. */

	if ( panther->axis_name == '0' ) {

		/* Send a space character to see if the Panther indexer
		 * echoes it back.
		 */

		mx_status = mxd_panther_putc( panther, ' ', PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* We don't want to block here waiting for a timeout if 
		 * nothing is being echoed back, since we don't necessarily
		 * have any control over how long the timeout in the
		 * underlying RS-232 driver is.  Therefore, we implement
		 * a timeout here ourselves.
		 */

		max_retries = 5;

		for ( i = 0; i < max_retries; i++ ) {
			mx_status = mxd_panther_getc_nowait(
					panther, &c, PANTHER_DEBUG);

			if ( mx_status.code == MXE_SUCCESS ) {
				if ( c != ' ' ) {
					return mx_error( MXE_DEVICE_IO_ERROR,
						fname,
"A '%c' (0x%x) character was echoed in response to a space character.",
						c, c );
				}
				break;		/* Exit the for() loop. */
			}

			mx_msleep( 1000 );
		}

		if ( i >= max_retries ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"The Panther indexer isn't echoing characters back.  Is it on and connected?");
		}

		/* At this point, either a sign-on message has been displayed
		 * in response to the space character or the space character
		 * alone has been echoed and read back.  Sending an end of
		 * string character will cause the indexer to send back
		 * a '#' character followed by CR, LF.
		 */

		mx_status = mxd_panther_putc( panther,
				MX_PANTHER_SINGLE_MODE_EOS, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Read a line and see if the sign-on message was sent
		 * by the indexer.
		 */

		signon_message_seen = FALSE;

		mx_status = mxd_panther_getline( panther,
				response, sizeof(response), PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Look for the letter "A" in the line. */

		p = strchr( response, 'A' );

		if ( p != NULL ) {

			if ( strncmp( p, "ADVANCED MICRO SYSTEMS", 22 ) == 0 ) {
				new_version = TRUE;
			} else
			if ( strncmp( p, "AMS", 3 ) == 0 ) {
				new_version = FALSE;
			} else {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see correct sign-on message.  Instead saw '%s'",
					response );
			}

			signon_message_seen = TRUE;

			/* Check to see if we saw one version number or two.
			 * If we saw two, the second should be the encoder
			 * software version number.
			 */

			num_items = sscanf( response, "%d %d",
					&indexer_version,
					&encoder_version );

			if ( num_items <= 0 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Did not see indexer version number in sign-on message.  Instead saw '%s'",
					response );
			} else if ( num_items == 1 ) {
				if ( record->mx_type
				     == MXT_MTR_PANTHER_HE ) {

					MX_DEBUG(-2,("No encoder version "
		"number seen even though this is declared as a Panther HE."));
				}
			} else if ( num_items == 2 ) {
				if ( record->mx_type
				     == MXT_MTR_PANTHER_HI ) {

					MX_DEBUG(-2,("Encoder version "
	"number seen even though the database says this is a Panther HI."));
				}
			}

			/* If the indexer firmware is version 1.14
			 * or higher, there is another line to read.
			 */

			if ( new_version ) {
				mx_status = mxd_panther_getline( panther,
				response, sizeof(response), PANTHER_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
		}

		/* If the sign-on message was not seen, the previous call
		 * should have read in the #-CR-LF sequence at the end
		 * of the buffer.  If it _was_ seen, there is still a
		 * #-CR-LF sequence waiting in the buffer to be read out.
		 */

		if ( signon_message_seen == FALSE ) {
			MX_DEBUG(-2, ("Panther sign-on message not seen."));
		} else {
			MX_DEBUG(-2, ("Panther sign-on message seen."));

			mx_status = mxd_panther_getline( panther, response,
					sizeof(response), PANTHER_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Try to recover from any handshaking errors. */

		mx_status = mx_rs232_discard_unwritten_output(
				panther->rs232_record, PANTHER_DEBUG);

		if ( mx_status.code != MXE_SUCCESS
		  && mx_status.code != MXE_UNSUPPORTED )
			return mx_status;

		mx_status = mx_rs232_discard_unread_input(
				panther->rs232_record, PANTHER_DEBUG);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_motor_is_busy()";

	MX_PANTHER *panther;
	char response[20];
	int num_items, moving_status;
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This is a case where the Panther indexer does not
	 * echo the transmitted end of string character.  The
	 * third argument of mxd_panther_putline() is set to
	 * FALSE to indicate this.
	 */

	mx_status = mxd_panther_putline( panther, "^", FALSE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_panther_getline( panther,
			response, sizeof(response), PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d", &moving_status );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable moving status value '%s' was read.", response );
	}

	if ( moving_status & 0x01 ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_move_absolute()";

	MX_PANTHER *panther;
	char command[20];
	char c;
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Format the move command and send it. */

	motor_steps = motor->raw_destination.stepper;

	sprintf( command, "R%+ld", motor_steps );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	/* In Single mode, eat a line feed character sent back. */

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_panther_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_get_position()";

	MX_PANTHER *panther;
	char response[30];
	int num_items;
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This is a case where the Panther indexer does not
	 * echo the transmitted end of string character.  The
	 * third argument of mxd_panther_putline() is set to
	 * FALSE to indicate this.
	 */

	if ( panther->encoder_resolution > 0 ) {
		mx_status = mxd_panther_putline( panther, "z0",
						FALSE, PANTHER_DEBUG );
	} else {
		mx_status = mxd_panther_putline( panther, "Z0",
						FALSE, PANTHER_DEBUG );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_panther_getline( panther,
			response, sizeof(response), PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &motor_steps );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable position value '%s' was read.", response );
	}

	motor->raw_position.stepper = motor_steps;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_set_position()";

	MX_PANTHER *panther;
	char c, command[80];
	long set_position;
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_position = motor->raw_set_position.stepper;

	if ( set_position != 0L ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Panther controllers do not support setting the "
		"current position to any value other than zero." );
	}

	/* Send the set origin command. */

	mx_status = mxd_panther_putline( panther, "O", TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	/* Set the encoder resolution.  This is the only time
	 * that it is safe to set the encoder resolution to
	 * a non-zero value in Party Line mode.
	 */

	sprintf( command, "e %d", panther->encoder_resolution );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_soft_abort()";

	MX_PANTHER *panther;
	char response[10];
	int busy;
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is the motor currently moving? */

	mx_status = mx_motor_is_busy( motor->record, &busy );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	/* Don't need to do anything if the motor is not moving. */

	if ( busy == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, we have to send the soft stop command. */

	mx_status = mxd_panther_putline( panther, "@", TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* In Single mode, eat any characters sent back. */

	if ( panther->axis_name == '0' ) {
		(void) mxd_panther_getline( panther,
			response, sizeof(response), PANTHER_DEBUG );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_panther_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_immediate_abort()";

	MX_PANTHER *panther;
	char response[10];
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the immediate abort command (an ESC character). */

	mx_status = mxd_panther_putline( panther, "\033", TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* In Single mode, eat any characters sent back. */

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getline(panther,
				response, sizeof(response), PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( response[0] != '#' || response[1] != '\0' ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see correct response to immediate abort command." );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_positive_limit_hit()";

	MX_PANTHER *panther;
	char response[20];
	int num_items, limit_state;
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This is a case where the Panther indexer does not
	 * echo the transmitted end of string character.  The
	 * third argument of mxd_panther_putline() is set to
	 * FALSE to indicate this.
	 */

	mx_status = mxd_panther_putline( panther, "]0", FALSE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_panther_getline( panther,
			response, sizeof(response), PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d", &limit_state );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable limit state value '%s' was read.", response );
	}

	if ( limit_state & 0x02 ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_negative_limit_hit()";

	MX_PANTHER *panther;
	char response[20];
	int num_items, limit_state;
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This is a case where the Panther indexer does not
	 * echo the transmitted end of string character.  The
	 * third argument of mxd_panther_putline() is set to
	 * FALSE to indicate this.
	 */

	mx_status = mxd_panther_putline( panther, "]0", FALSE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_panther_getline( panther,
			response, sizeof(response), PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d", &limit_state );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable limit state value '%s' was read.", response );
	}

	if ( limit_state & 0x01 ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_find_home_position()";

	MX_PANTHER *panther;
	char c;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->home_search >= 0 ) {
		if ( panther->encoder_resolution > 0 ) {
			strcpy( command, "f 1" );
		} else {
			sprintf( command, "F %ld 1",
					mx_round( motor->raw_speed ) );
		}
	} else {
		if ( panther->encoder_resolution > 0 ) {
			strcpy( command, "f 0" );
		} else {
			sprintf( command, "F %ld 0",
					mx_round( motor->raw_speed ) );
		}
	}

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is there any response? */

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_constant_velocity_move()";

	MX_PANTHER *panther;
	long slew_speed;
	char c;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	slew_speed = mx_round( motor->raw_speed );

	if ( motor->constant_velocity_move < 0 ) {
		slew_speed = -slew_speed;
	}

	sprintf( command, "M%+ld", slew_speed );

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is there any response? */

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_constant_velocity_move()";

	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MOTOR pointer passed is NULL." );
	}

	/* There is no way to read any parameter without reading them all. */

	mx_status = mxd_panther_read_parms_from_hardware( motor->record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_panther_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_panther_set_parameter()";

	MX_PANTHER *panther;
	char c;
	char command[80];
	long raw_speed, raw_base_speed, raw_maximum_speed;
	long accel_slope, decel_slope;
	mx_status_type mx_status;

	mx_status = mxd_panther_get_pointers( motor, &panther, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Format the appropriate parameter setting command. */

	if ( motor->parameter_type == MXLV_MTR_SPEED ) {

		raw_speed = mx_round( motor->raw_speed );

		if ( ( raw_speed < MX_PANTHER_MOTOR_MINIMUM_SPEED )
		  || ( raw_speed > motor->raw_maximum_speed ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Requested raw speed of %ld steps/sec is outside "
	"the allowed range of %ld <= raw speed <= %ld.", raw_speed,
				MX_PANTHER_MOTOR_MINIMUM_SPEED,
				mx_round( motor->raw_maximum_speed ) );
		}

		sprintf( command, "V %ld", raw_speed );

	} else if ( motor->parameter_type == MXLV_MTR_BASE_SPEED ) {

		raw_base_speed = mx_round( motor->raw_base_speed );

		if ( ( raw_base_speed < MX_PANTHER_MOTOR_MINIMUM_SPEED )
		  || ( raw_base_speed > motor->raw_speed ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Requested raw base speed of %ld steps/sec is outside "
	"the allowed range of %ld <= raw base speed <= %ld.", raw_base_speed,
				MX_PANTHER_MOTOR_MINIMUM_SPEED,
				mx_round( motor->raw_speed ) );
		}

		sprintf( command, "I %ld", raw_base_speed );

	} else if ( motor->parameter_type == MXLV_MTR_MAXIMUM_SPEED ) {

		raw_maximum_speed = mx_round( motor->raw_maximum_speed );

		if ( ( raw_maximum_speed < MX_PANTHER_MOTOR_MINIMUM_SPEED )
		  || ( raw_maximum_speed > MX_PANTHER_MOTOR_MAXIMUM_SPEED ) )
		{
			if (raw_maximum_speed < MX_PANTHER_MOTOR_MINIMUM_SPEED)
			{
				motor->raw_maximum_speed
					= MX_PANTHER_MOTOR_MINIMUM_SPEED;
			} else {
				motor->raw_maximum_speed
					= MX_PANTHER_MOTOR_MAXIMUM_SPEED;
			}
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Requested raw maximum speed of %ld steps/sec is outside "
	"the allowed range of %ld < raw_maximum_speed <= %ld.",
				raw_maximum_speed,
				MX_PANTHER_MOTOR_MINIMUM_SPEED,
				MX_PANTHER_MOTOR_MAXIMUM_SPEED );
		}
		return MX_SUCCESSFUL_RESULT;

	} else if ( motor->parameter_type ==
			MXLV_MTR_RAW_ACCELERATION_PARAMETERS )
	{

		accel_slope = mx_round( motor->raw_acceleration_parameters[0] );
		decel_slope = mx_round( motor->raw_acceleration_parameters[1] );

		if ( ( accel_slope < MX_PANTHER_MOTOR_MINIMUM_ACCEL_SLOPE )
		  || ( accel_slope > MX_PANTHER_MOTOR_MAXIMUM_ACCEL_SLOPE ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Requested acceleration slope of %ld is outside "
	"the allowed range of %ld <= accel. slope <= %ld.",
				accel_slope,
				MX_PANTHER_MOTOR_MINIMUM_ACCEL_SLOPE,
				MX_PANTHER_MOTOR_MAXIMUM_ACCEL_SLOPE );
		}

		if ( ( decel_slope < MX_PANTHER_MOTOR_MINIMUM_ACCEL_SLOPE )
		  || ( decel_slope > MX_PANTHER_MOTOR_MAXIMUM_ACCEL_SLOPE ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Requested deceleration slope of %ld is outside "
	"the allowed range of %ld <= accel. slope <= %ld.",
				decel_slope,
				MX_PANTHER_MOTOR_MINIMUM_ACCEL_SLOPE,
				MX_PANTHER_MOTOR_MAXIMUM_ACCEL_SLOPE );
		}

		sprintf( command, "K %ld %ld", accel_slope, decel_slope );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			motor->parameter_type );
	}

	/* Send the command to the PANTHER controller. */

	mx_status = mxd_panther_putline( panther, command, TRUE, PANTHER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( panther->axis_name == '0' ) {
		mx_status = mxd_panther_getc( panther, &c, PANTHER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}


/* === Extra functions for the use of this driver. === */

MX_EXPORT mx_status_type
mxd_panther_getline( MX_PANTHER *panther,
		char *buffer, int buffer_length, int debug_flag )
{
	static const char fname[] = "mxd_panther_getline()";

	MX_RECORD *rs232_record;
	char c;
	int i;
	mx_status_type mx_status;

	if ( debug_flag ) {
		MX_DEBUG( 2,("%s invoked.", fname));
	}

	rs232_record = panther->rs232_record;

	for ( i = 0; i < (buffer_length - 1) ; i++ ) {
		mx_status = mx_rs232_getchar( rs232_record, &c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS ) {
			/* Make the buffer contents a valid C string
			 * before returning, so that we can at least
			 * see what appeared before the error.
			 */

			buffer[i] = '\0';

			if ( debug_flag ) {
				MX_DEBUG(-2,
				("%s failed with status = %ld, c = 0x%x '%c'",
				fname, mx_status.code, c, c));

				MX_DEBUG(-2, ("-> buffer = '%s'", buffer) );
			}

			return mx_status;
		}

		if ( debug_flag ) {
			MX_DEBUG( 2,
			("%s: received c = 0x%x '%c'", fname, c, c));
		}

		buffer[i] = c;

		/* Responses from the IMS Panther HI should be terminated
		 * by the end of string character.
		 */

		if ( panther->axis_name == '0' ) {  /* Single Mode */
			if ( c == MX_PANTHER_SINGLE_MODE_EOS ) {
				i++;

				MX_DEBUG( 2,("%s: found single mode EOS",
					fname));

				/* There is also a line feed character
				 * to eat.
				 */

				mx_status = mx_rs232_getchar( rs232_record,
						&c, MXF_232_WAIT );

				if ( mx_status.code != MXE_SUCCESS ) {
					if ( debug_flag ) {
						MX_DEBUG(-2,(
				"%s: failed to see any characters following "
				"the single mode end of string.", fname));
					}

					return mx_status;
				}

				if ( debug_flag ) {
					MX_DEBUG( 2,
				("%s: received c = 0x%x '%c'", fname, c, c));
				}

				/* Only exit the for() loop if the character
				 * we received is an LF.
				 */

				if ( c == MX_LF ) {
					MX_DEBUG( 2,
					("%s: found the LF.", fname));

					/* Throw the LF away and exit
					 * the for() loop.
					 */

					break;
				} else {
					buffer[i] = c;
				}
			}
		} else {			/* Party Line Mode */
			if ( c == MX_PANTHER_PARTY_LINE_MODE_EOS ) {
				i++;

				MX_DEBUG( 2,
				("%s: found party line mode eos", fname));

				break;		/* exit the for() loop */
			}
		}
	}

	/* Check to see if the last character was an end of string
	 * and if it is then overwrite it with a NULL.
	 */

	if ( panther->axis_name == '0' ) {	/* Single Mode */
		if ( buffer[i-1] != MX_PANTHER_SINGLE_MODE_EOS ) {

			mx_status = mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see correct end of string character.");

			buffer[i] = '\0';
		} else {
			mx_status = MX_SUCCESSFUL_RESULT;

			buffer[i-1] = '\0';
		}
	} else {				/* Party Line Mode */
		if ( buffer[i-1] != MX_PANTHER_PARTY_LINE_MODE_EOS ) {

			mx_status = mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see correct end of string character.");

			buffer[i] = '\0';
		} else {
			mx_status = MX_SUCCESSFUL_RESULT;

			buffer[i-1] = '\0';
		}
	}

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s: received '%s'", fname, buffer) );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_panther_putline( MX_PANTHER *panther,
		char *buffer, int echo_end_of_string_flag, int debug_flag )
{
	static const char fname[] = "mxd_panther_putline()";

	MX_RECORD *rs232_record;
	char *ptr;
	char c, echo;
	mx_status_type mx_status;

	if ( debug_flag ) {
		if ( panther->axis_name == '0' ) {
			MX_DEBUG(-2, ("%s: sending '%s'", fname, buffer));
		} else {
			MX_DEBUG(-2, ("%s: sending '%c%s'", fname,
					panther->axis_name, buffer));
		}
	}

	rs232_record = panther->rs232_record;

	ptr = &buffer[0];

	/* Don't send zero length strings. */

	if ( strlen(ptr) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we are in party line mode, send the axis name first. */

	if ( panther->axis_name != '0' ) {

		mx_status = mx_rs232_putchar( rs232_record,
					panther->axis_name, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG( 2,("%s: sent 0x%x '%c'", fname,
				panther->axis_name, panther->axis_name));
		}

		/* Read back the echoed character. */

		mx_status = mx_rs232_getchar( rs232_record,
					&echo, MXF_232_WAIT);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(  2,
				("%s: received 0x%x '%c'", fname,
				echo, echo));
		}

		if ( echo != panther->axis_name ) {
			mx_msleep(100);

			(void) mx_rs232_discard_unread_input(rs232_record,
								PANTHER_DEBUG);

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"The character '%c' (0x%x) was sent, but '%c' (0x%x) was echoed back.",
				panther->axis_name, panther->axis_name,
				echo, echo );
		}
	}

	while ( *ptr != '\0' ) {
		c = *ptr;

		mx_status = mx_rs232_putchar( rs232_record, c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG( 2,("%s: sent 0x%x '%c'", fname, c, c));
		}

		/* Normally, the Panther motor controller echoes back all
		 * characters sent to it including the end of string
		 * character.  However, if we are in Single Mode, the 
		 * Party Line Mode end of string character '\012' is 
		 * not echoed back.
		 */

		if ( panther->axis_name == '0'
				&& c == MX_PANTHER_PARTY_LINE_MODE_EOS ) {
			/* Do nothing. */

		} else {
			mx_msleep(1);

			mx_status = mx_rs232_getchar(rs232_record,
						&echo, MXF_232_WAIT);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( debug_flag ) {
				MX_DEBUG( 2,("%s: received 0x%x '%c'",
					fname, echo, echo));
			}

			if ( echo != c ) {
				mx_msleep(100);

				(void) mx_rs232_discard_unread_input(
						rs232_record, PANTHER_DEBUG );

				return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"The character '%c' (0x%x) was sent, but '%c' (0x%x) was echoed back.",
					c, c, echo, echo );
			}
		}
		ptr++;
	}

	(void) mx_rs232_discard_unread_input( rs232_record, PANTHER_DEBUG );

	/* Send an end of string character. */

	if ( panther->axis_name == '0' ) {		/* Single Mode */
		c = MX_PANTHER_SINGLE_MODE_EOS;
	} else {
		c = MX_PANTHER_PARTY_LINE_MODE_EOS;
	}

	if ( c != '\0' ) {
		/* Send the end of string. */

		mx_status = mx_rs232_putchar( rs232_record, c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG( 2,("%s: (EOS) sent 0x%x '%c'", fname, c, c));
		}

		/* Get the echoed character if asked. */

		if ( echo_end_of_string_flag ) {

			mx_status = mx_rs232_getchar(rs232_record,
						&echo, MXF_232_WAIT);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( debug_flag ) {
				MX_DEBUG( 2,("%s: (EOS) received 0x%x '%c'",
					fname, echo, echo));
			}

			if ( echo != c ) {
				mx_msleep(100);

				(void) mx_rs232_discard_unread_input(
						rs232_record, PANTHER_DEBUG);

				return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"The character '%c' (0x%x) was sent, but '%c' (0x%x) was echoed back.",
					c, c, echo, echo );
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_panther_getc( MX_PANTHER *panther, char *c, int debug_flag )
{
	static const char fname[] = "mxd_panther_getc()";

	mx_status_type mx_status;

	mx_status = mx_rs232_getchar( panther->rs232_record, c, MXF_232_WAIT );

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received 0x%x '%c'", fname, *c, *c));
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_panther_getc_nowait( MX_PANTHER *panther, char *c, int debug_flag )
{
	static const char fname[] = "mxd_panther_getc_nowait()";

	mx_status_type mx_status;

	mx_status = mx_rs232_getchar( panther->rs232_record, c, MXF_232_NOWAIT);

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received 0x%x '%c'", fname, *c, *c));
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_panther_putc( MX_PANTHER *panther, char c, int debug_flag )
{
	static const char fname[] = "mxd_panther_putc()";

	mx_status_type mx_status;

	mx_status = mx_rs232_putchar( panther->rs232_record, c, MXF_232_WAIT );

	/* The Panther controllers normally echo most characters sent
	 * to them, but since mxd_panther_putc() is intended as a low
	 * level function, we leave handling that up to the routine that
	 * invokes us.
	 */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sent 0x%x '%c'", fname, c, c));
	}

	return mx_status;
}


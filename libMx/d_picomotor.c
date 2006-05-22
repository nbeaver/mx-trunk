/*
 * Name:    d_picomotor.c
 *
 * Purpose: MX driver for New Focus Picomotor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PICOMOTOR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "i_picomotor.h"
#include "d_picomotor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_picomotor_record_function_list = {
	NULL,
	mxd_picomotor_create_record_structures,
	mxd_picomotor_finish_record_initialization,
	NULL,
	mxd_picomotor_print_structure,
	NULL,
	NULL,
	mxd_picomotor_open,
	NULL,
	NULL,
	mxd_picomotor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_picomotor_motor_function_list = {
	NULL,
	mxd_picomotor_move_absolute,
	mxd_picomotor_get_position,
	mxd_picomotor_set_position,
	mxd_picomotor_soft_abort,
	mxd_picomotor_immediate_abort,
	NULL,
	NULL,
	mxd_picomotor_find_home_position,
	mxd_picomotor_constant_velocity_move,
	mxd_picomotor_get_parameter,
	mxd_picomotor_set_parameter,
	NULL,
	mxd_picomotor_get_status
};

/* Picomotor motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_picomotor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PICOMOTOR_STANDARD_FIELDS
};

long mxd_picomotor_num_record_fields
		= sizeof( mxd_picomotor_record_field_defaults )
			/ sizeof( mxd_picomotor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_picomotor_rfield_def_ptr
			= &mxd_picomotor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_picomotor_get_pointers( MX_MOTOR *motor,
			MX_PICOMOTOR **picomotor,
			MX_PICOMOTOR_CONTROLLER **picomotor_controller,
			const char *calling_fname )
{
	static const char fname[] = "mxd_picomotor_get_pointers()";

	MX_PICOMOTOR *picomotor_pointer;
	MX_RECORD *picomotor_controller_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	picomotor_pointer = (MX_PICOMOTOR *)
				motor->record->record_type_struct;

	if ( picomotor_pointer == (MX_PICOMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PICOMOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( picomotor != (MX_PICOMOTOR **) NULL ) {
		*picomotor = picomotor_pointer;
	}

	if ( picomotor_controller != (MX_PICOMOTOR_CONTROLLER **) NULL ) {
		picomotor_controller_record =
			picomotor_pointer->picomotor_controller_record;

		if ( picomotor_controller_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The picomotor_controller_record pointer for record '%s' is NULL.",
				motor->record->name );
		}

		*picomotor_controller = (MX_PICOMOTOR_CONTROLLER *)
				picomotor_controller_record->record_type_struct;

		if ( *picomotor_controller == (MX_PICOMOTOR_CONTROLLER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PICOMOTOR_CONTROLLER pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_picomotor_motor_command( MX_PICOMOTOR_CONTROLLER *picomotor_controller,
			MX_PICOMOTOR *picomotor,
			char *command,
			char *response,
			size_t max_response_length,
			int debug_flag )
{
	static const char fname[] = "mxd_picomotor_motor_command()";

	MX_RECORD *current_motor_record;
	MX_PICOMOTOR *current_picomotor;
	char command_buffer[40], response_buffer[40];
	long driver_number, driver_type, motor_number;
	long current_motor_number, position_offset;
	int num_items;
	mx_status_type mx_status;

	/* If the target is a Model 8753 open-loop driver, set the
	 * motor channel first.
	 */

	driver_number = picomotor->driver_number;

	motor_number = picomotor->motor_number;

	driver_type = picomotor_controller->driver_type[ driver_number - 1 ];

	if ( driver_type == MXT_PICOMOTOR_8753_DRIVER ) {

		current_motor_number =
		  picomotor_controller->current_motor_number[driver_number - 1];

		if ( motor_number != current_motor_number ) {

			/* When the current motor number is changed,
			 * we must update the 'base_position' field
			 * for the current motor.
			 */

#if 0
		    {
		    	long j;

			for ( j = 0; j < MX_MAX_PICOMOTORS_PER_DRIVER; j++ ) {
			    current_motor_record =
		picomotor_controller->motor_record_array[driver_number - 1][j];

			    if ( current_motor_record == (MX_RECORD *) NULL ) {
				    MX_DEBUG(-2,("%s: motor[%ld][%ld] = NULL",
						fname, driver_number - 1, j ));
			    } else {
				    MX_DEBUG(-2,("%s: motor[%ld][%ld] = '%s'",
					fname, driver_number - 1, j,
					current_motor_record->name ));
			    }
			}
		    }
#endif
			current_motor_record =
			    picomotor_controller->motor_record_array
				[driver_number - 1][current_motor_number];

			if ( current_motor_record != (MX_RECORD *) NULL ) {

				current_picomotor = (MX_PICOMOTOR *)
				    current_motor_record->record_type_struct;

				if (current_picomotor == (MX_PICOMOTOR *)NULL) {
					return mx_error(
					    MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The MX_PICOMOTOR pointer for Picomotor "
				"record '%s' is NULL.",
						current_motor_record->name );
				}

				MX_DEBUG( 2,("%s: OLD base_position = %ld",
				    fname, current_picomotor->base_position));

				/* Get the position offset of the current
				 * motor record.
				 */

				snprintf(command_buffer, sizeof(command_buffer),
					"POS %s", picomotor->driver_name );

				mx_status = mxi_picomotor_command(
					picomotor_controller, command_buffer,
					response_buffer,sizeof(response_buffer),
					debug_flag );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				num_items = sscanf( response_buffer, "A%*d=%ld",
							&position_offset );

				if ( num_items != 1 ) {
					return mx_error(
						MXE_INTERFACE_IO_ERROR, fname,
				"Cannot find the motor position offset in "
				"the response '%s' to command '%s' for "
				"Picomotor controller '%s'.",
					response_buffer, command_buffer,
					picomotor_controller->record->name );
				}

				current_picomotor->base_position
					+= position_offset;

				MX_DEBUG( 2,
		    ("%s: position_offset = %ld  ==>  NEW base_position = %ld",
					fname, position_offset,
					current_picomotor->base_position));

				/* Add extra delay after getting
				 * the motor offset.
				 */

				mx_msleep(200);
			}

			/* Send a command to change the current motor number. */

			snprintf( command_buffer, sizeof(command_buffer),
				"CHL %s=%ld",
				picomotor->driver_name,
				picomotor->motor_number );

			mx_status = mxi_picomotor_command( picomotor_controller,
					command_buffer, NULL, 0, debug_flag );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

		   picomotor_controller->current_motor_number[driver_number - 1]
		   	= picomotor->motor_number;

			/* Add extra delay after setting the motor channel. */

			mx_msleep(300);
		}
	}

	/* Then send the real command to the controller. */

	mx_status = mxi_picomotor_command( picomotor_controller,
			command, response, max_response_length, debug_flag );

	mx_msleep(100);

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxd_picomotor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_picomotor_create_record_structures()";

	MX_MOTOR *motor;
	MX_PICOMOTOR *picomotor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	picomotor = (MX_PICOMOTOR *)
				malloc( sizeof(MX_PICOMOTOR) );

	if ( picomotor == (MX_PICOMOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_PICOMOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = picomotor;
	record->class_specific_function_list
				= &mxd_picomotor_motor_function_list;

	motor->record = record;
	picomotor->record = record;

	/* A Picomotor motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The Picomotor reports acceleration in steps/sec**2. */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_picomotor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	MX_PICOMOTOR *picomotor;
	int num_items;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( picomotor->driver_name, "A%ld",
				&(picomotor->driver_number) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Cannot find Picomotor driver number in the driver name '%s'.",
			picomotor->driver_name );
	}

	if ( (picomotor->driver_number < 1)
	  || (picomotor->driver_number > MX_MAX_PICOMOTOR_DRIVERS ) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested driver number %ld for driver '%s' "
		"used by motor '%s' is outside the allowed range of 1 to %d.",
			picomotor->driver_number,
			picomotor->driver_name,
			record->name,
			MX_MAX_PICOMOTOR_DRIVERS );
	}

	if ( (picomotor->motor_number < 0)
	  || (picomotor->motor_number > (MX_MAX_PICOMOTORS_PER_DRIVER - 1)) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested motor number %ld for motor '%s' is outside "
		"the allowed range of 0 to %d.",
			picomotor->motor_number,
			record->name,
			MX_MAX_PICOMOTORS_PER_DRIVER - 1 );
	}

	picomotor_controller->motor_record_array
	    [picomotor->driver_number - 1][picomotor->motor_number] = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_picomotor_print_structure()";

	MX_MOTOR *motor;
	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	double position, move_deadband, speed;
	long driver_number;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = PICOMOTOR motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  controller name   = %s\n",
					picomotor_controller->record->name);
	fprintf(file, "  driver name       = %s\n",
					picomotor->driver_name);
	fprintf(file, "  motor number      = %ld\n",
					picomotor->motor_number);
	fprintf(file, "  flags             = %#lx\n", picomotor->flags);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position          = %g %s  (%ld steps)\n",
			motor->position, motor->units,
			motor->raw_position.stepper );
	fprintf(file, "  scale             = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit    = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit    = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband     = %g %s  (%ld steps)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  speed             = %g %s/s  (%g steps/s)\n",
		speed, motor->units,
		motor->raw_speed );

	driver_number = picomotor->driver_number;

	fprintf(file, "  driver_type       = %ld\n",
		picomotor_controller->driver_type[driver_number - 1] );

	fprintf(file, "  base_position     = %ld\n",
		picomotor->base_position);

	fprintf(file, "\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_picomotor_open()";

	MX_MOTOR *motor;
	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	long driver_number, driver_type;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Model 8753 only supports relative positioning, so for that
	 * model we must store the current absolute position ourselves.
	 */

	driver_number = picomotor->driver_number;
	driver_type = picomotor_controller->driver_type[ driver_number - 1 ];

	if ( driver_type == MXT_PICOMOTOR_8753_DRIVER ) {
		picomotor->base_position = 0;
	} else {
		picomotor->base_position = 0;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_picomotor_resynchronize()";

	MX_PICOMOTOR *picomotor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	picomotor = (MX_PICOMOTOR *) record->record_type_struct;

	if ( picomotor == (MX_PICOMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PICOMOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxi_picomotor_resynchronize(
			picomotor->picomotor_controller_record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_picomotor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_move_absolute()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	long position_change;
	long driver_type;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("****** %s invoked for motor '%s' ******", 
		fname, motor->record->name));

	driver_type =
	    picomotor_controller->driver_type[ picomotor->driver_number - 1 ];

	switch( driver_type ) {
	case MXT_PICOMOTOR_8753_DRIVER:
		/* The Model 8753 _only_ supports relative moves
		 * so we must compute the position difference between
		 * the current destination and the current position.
		 */

		mx_status = mxd_picomotor_get_position( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		position_change = motor->raw_destination.stepper
				- motor->raw_position.stepper;

		MX_DEBUG( 2,("%s: motor->raw_destination.stepper = %ld",
			fname, motor->raw_destination.stepper));

		MX_DEBUG( 2,("%s: motor->raw_position.stepper = %ld",
		 	fname, motor->raw_position.stepper));

		MX_DEBUG( 2,("%s: position_change = %ld",
			fname, position_change));

		snprintf( command, sizeof(command),
			"REL %s=%ld G",
			picomotor->driver_name,
			position_change );

		break;
	default:
		/* For all others, use absolute positioning. */

		snprintf( command, sizeof(command),
			"ABS %s=%ld G",
			picomotor->driver_name,
			motor->raw_destination.stepper );
	}

	mx_status = mxd_picomotor_motor_command( picomotor_controller,
					picomotor, command,
					NULL, 0, MXD_PICOMOTOR_DEBUG );

	MX_DEBUG( 2,("****** %s complete for motor '%s' ******", 
		fname, motor->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_get_position()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char response[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	long motor_steps;
	long driver_type;
	int num_items;
	char *ptr;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("++++++ %s invoked for motor '%s' ++++++", 
		fname, motor->record->name));

	snprintf( command, sizeof(command), "POS %s", picomotor->driver_name );

	mx_status = mxd_picomotor_motor_command( picomotor_controller,
						picomotor, command,
						response, sizeof response,
						MXD_PICOMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = strchr( response, '=' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No equals sign seen in response to command '%s' by "
		"Picomotor '%s'.  Response = '%s'",
			command, motor->record->name, response );
	}

	ptr++;

	num_items = sscanf( ptr, "%ld", &motor_steps );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No position value seen in response to '%s' command.  "
		"Response seen = '%s'", command, response );
	}

	driver_type =
	    picomotor_controller->driver_type[ picomotor->driver_number - 1 ];

	switch( driver_type ) {
	case MXT_PICOMOTOR_8753_DRIVER:
		/* The Model 8753 _only_ reports relative positions,
		 * namely, the number of pulses sent since the last
		 * motion command.  Thus, we use the reported number
		 * of motor steps as an offset from the base position.
		 */

		MX_DEBUG( 2,("%s: picomotor->base_position = %ld",
		 	fname, picomotor->base_position));

		MX_DEBUG( 2,("%s: motor_steps = %ld", fname, motor_steps));

		motor->raw_position.stepper =
				picomotor->base_position + motor_steps;

		MX_DEBUG( 2,("%s: motor->raw_position.stepper = %ld",
			fname, motor->raw_position.stepper));
		break;
	default:
		/* For all others, use absolute positioning. */

		motor->raw_position.stepper = motor_steps;
		break;
	}

	MX_DEBUG( 2,("++++++ %s complete for motor '%s' ++++++", 
		fname, motor->record->name));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_set_position()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[100];
	long position_difference;
	long driver_type;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("!!!!!! %s invoked for motor '%s' !!!!!!", 
		fname, motor->record->name));

	driver_type =
	    picomotor_controller->driver_type[ picomotor->driver_number - 1 ];

	if ( driver_type == MXT_PICOMOTOR_8751_DRIVER ) {

		/* The Model 8751-C knows the absolute position
		 * of the motor.
		 */

		snprintf( command, sizeof(command),
				"POS %s=%ld", picomotor->driver_name,
				motor->raw_set_position.stepper );

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );

	} else {
		/* For the 8753, all we can do is redefine the value
		 * of picomotor->base_position.
		 */

		mx_status = mxd_picomotor_get_position( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		position_difference = motor->raw_set_position.stepper
					- motor->raw_position.stepper;

		MX_DEBUG( 2,("%s: motor->raw_set_position.stepper = %ld",
			fname, motor->raw_set_position.stepper));

		MX_DEBUG( 2,("%s: motor->raw_position.stepper = %ld",
			fname, motor->raw_position.stepper));

		MX_DEBUG( 2,("%s: position_difference = %ld",
			fname, position_difference));

		/* Increment the value of base_position
		 * by the position difference.
		 */

		MX_DEBUG( 2,("%s: OLD picomotor->base_position = %ld",
		 	fname, picomotor->base_position));

		picomotor->base_position += position_difference;

		MX_DEBUG( 2,("%s: NEW picomotor->base_position = %ld",
		 	fname, picomotor->base_position));
	}

	MX_DEBUG( 2,("!!!!!! %s complete for motor '%s' !!!!!!", 
		fname, motor->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_soft_abort()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "HAL %s", picomotor->driver_name );

	mx_status = mxd_picomotor_motor_command( picomotor_controller,
					picomotor, command,
					NULL, 0, MXD_PICOMOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_immediate_abort()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "STO %s", picomotor->driver_name );

	mx_status = mxd_picomotor_motor_command( picomotor_controller,
					picomotor, command,
					NULL, 0, MXD_PICOMOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_find_home_position()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( picomotor->flags & MXF_PICOMOTOR_HOME_TO_LIMIT_SWITCH ) {
		if ( motor->home_search >= 0 ) {
			snprintf( command, sizeof(command),
				"FLI %s", picomotor->driver_name );
		} else {
			snprintf( command, sizeof(command),
				"RLI %s", picomotor->driver_name );
		}
	} else {
		if ( motor->home_search >= 0 ) {
			snprintf( command, sizeof(command),
				"FIN %s", picomotor->driver_name );
		} else {
			snprintf( command, sizeof(command),
				"RIN %s", picomotor->driver_name );
		}
	}

	mx_status = mxd_picomotor_motor_command( picomotor_controller,
					picomotor, command,
					NULL, 0, MXD_PICOMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_constant_velocity_move()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the direction of the move. */

	if ( motor->constant_velocity_move >= 0 ) {
		snprintf( command, sizeof(command),
			"FOR %s G", picomotor->driver_name );
	} else {
		snprintf( command, sizeof(command),
			"REV %s G", picomotor->driver_name );
	}

	mx_status = mxi_picomotor_command( picomotor_controller, command,
					NULL, 0, MXD_PICOMOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_get_parameter()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char response[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char *ptr;
	int num_items;
	unsigned long ulong_value;
	long driver_type;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	driver_type =
	    picomotor_controller->driver_type[ picomotor->driver_number - 1 ];

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		snprintf( command, sizeof(command),
				"VEL %s %ld",
				picomotor->driver_name,
				picomotor->motor_number );

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, response, sizeof(response),
				MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr = strchr( response, '=' );

		if ( ptr == (char *) NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No equals sign seen in response to command '%s' by "
			"Picomotor '%s'.  Response = '%s'",
				command, motor->record->name, response );
		}

		ptr++;

		num_items = sscanf( ptr, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get the speed for motor '%s' "
			"in the response to the command '%s'.  "
			"Response = '%s'",
				motor->record->name, command, response );
		}

		motor->raw_speed = (double) ulong_value;
		break;
	case MXLV_MTR_BASE_SPEED:
		if ( driver_type != MXT_PICOMOTOR_8753_DRIVER ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Picomotor '%s' does not use a Model 8753 "
			"open-loop driver, so querying the base speed "
			"is not supported.", motor->record->name );
		}

		snprintf( command, sizeof(command),
				"MPV %s %ld",
				picomotor->driver_name,
				picomotor->motor_number );

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, response, sizeof(response),
				MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr = strchr( response, '=' );

		if ( ptr == (char *) NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No equals sign seen in response to command '%s' by "
			"Picomotor '%s'.  Response = '%s'",
				command, motor->record->name, response );
		}

		ptr++;

		num_items = sscanf( ptr, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get the base speed for motor '%s' "
			"in the response to the command '%s'.  "
			"Response = '%s'",
				motor->record->name, command, response );
		}

		motor->raw_base_speed = (double) ulong_value;
		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		snprintf( command, sizeof(command),
				"ACC %s %ld",
				picomotor->driver_name,
				picomotor->motor_number );

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, response, sizeof(response),
				MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr = strchr( response, '=' );

		if ( ptr == (char *) NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No equals sign seen in response to command '%s' by "
			"Picomotor '%s'.  Response = '%s'",
				command, motor->record->name, response );
		}

		ptr++;

		num_items = sscanf( ptr, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get the acceleration for motor '%s' "
			"in the response to the command '%s'.  "
			"Response = '%s'",
				motor->record->name, command, response );
		}

		motor->raw_acceleration_parameters[0] = (double) ulong_value;
		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;

	case MXLV_MTR_MAXIMUM_SPEED:

		/* As documented in the manual. */

		motor->raw_maximum_speed = 4095500;  /* steps per second */
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_set_parameter()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	unsigned long ulong_value;
	long driver_type;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	driver_type =
	    picomotor_controller->driver_type[ picomotor->driver_number - 1 ];

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		ulong_value = (unsigned long) mx_round( motor->raw_speed );

		snprintf( command, sizeof(command),
				"VEL %s %ld=%lu",
				picomotor->driver_name,
				picomotor->motor_number,
				ulong_value );
		
		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;
	case MXLV_MTR_BASE_SPEED:
		if ( driver_type != MXT_PICOMOTOR_8753_DRIVER ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Picomotor '%s' does not use a Model 8753 "
			"open-loop driver, so setting the base speed "
			"is not supported.", motor->record->name );
		}

		ulong_value = (unsigned long) mx_round( motor->raw_base_speed );

		snprintf( command, sizeof(command),
				"MPV %s %ld=%lu",
				picomotor->driver_name,
				picomotor->motor_number,
				ulong_value );
		
		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		ulong_value = (unsigned long)
			mx_round( motor->raw_acceleration_parameters[0] );

		snprintf( command, sizeof(command),
				"ACC %s %ld=%lu",
				picomotor->driver_name,
				picomotor->motor_number,
				ulong_value );
		
		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXLV_MTR_MAXIMUM_SPEED:

		motor->raw_maximum_speed = 4095500;  /* steps per second */

		return MX_SUCCESSFUL_RESULT;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = 1;
			snprintf( command, sizeof(command),
				"MON %s", picomotor->driver_name );
		} else {
			snprintf( command, sizeof(command),
				"MOF %s", picomotor->driver_name );
		}

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );
		break;

	case MXLV_MTR_CLOSED_LOOP:
		if ( driver_type != MXT_PICOMOTOR_8751_DRIVER ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Picomotor '%s' does not use a Model 8751 "
			"closed-loop driver, so enabling and disabling "
			"closed-loop mode is not supported.",
				motor->record->name );
		}

		if ( motor->closed_loop ) {
			motor->closed_loop = 1;
			snprintf( command, sizeof(command),
				"SER %s", picomotor->driver_name );
		} else {
			snprintf( command, sizeof(command),
				"NOS %s", picomotor->driver_name );
		}

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_get_status()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char response[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char *ptr;
	unsigned char status_byte;
	int in_diagnostic_mode;
	long driver_type;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "STA %s", picomotor->driver_name );

	mx_status = mxi_picomotor_command( picomotor_controller, command,
			response, sizeof response,
			MXD_PICOMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = strchr( response, '=' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No equals sign seen in response to command '%s' by "
		"Picomotor '%s'.  Response = '%s'",
			command, motor->record->name, response );
	}

	ptr++;

	status_byte = (unsigned char) mx_hex_string_to_unsigned_long( ptr );

	MX_DEBUG( 2,("%s: Picomotor status byte = %#x", fname, status_byte));

	/* Check all of the status bits that we are interested in. */

	motor->status = 0;

	driver_type =
	    picomotor_controller->driver_type[ picomotor->driver_number - 1 ];


	switch( driver_type ) {
	case MXT_PICOMOTOR_8751_DRIVER:
		in_diagnostic_mode = FALSE;

		/* Bit 0: Move done */

		if ( ( status_byte & 0x1 ) == 0 ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}

		/* Bit 1: Checksum error */

		if ( status_byte & 0x2 ) {
			motor->status |= MXSF_MTR_ERROR;
		}

		/* Bit 2: No motor */

		if ( status_byte & 0x4 ) {
			motor->status |= MXSF_MTR_DRIVE_FAULT;
		}

		if ( in_diagnostic_mode == FALSE ) {
			/* Bit 3: Power on */

			if ( ( status_byte & 0x8 ) == 0 ) {
				motor->status |= MXSF_MTR_DRIVE_FAULT;
			}
		}

		/* Bit 4: Position error */

		if ( status_byte & 0x10 ) {
			motor->status |= MXSF_MTR_FOLLOWING_ERROR;
		}

		if ( in_diagnostic_mode == FALSE ) {
			/* Bit 5: Reverse limit */

			if ( status_byte & 0x20 ) {
				motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
			}

			/* Bit 6: Forward limit */

			if ( status_byte & 0x40 ) {
				motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
			}
		}

		/* Bit 7: Home in progress */

		if ( ( status_byte & 0x80 ) == 0 ) {
			motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
		}
		break;
	case MXT_PICOMOTOR_8753_DRIVER:
		/* Bit 0: Motor is moving */

		if ( status_byte & 0x1 ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}

		/* Bit 1: Checksum error */

		if ( status_byte & 0x2 ) {
			motor->status |= MXSF_MTR_ERROR;
		}

		/* Bit 2: Motor is on */

		if ( ( status_byte & 0x4 ) == 0 ) {
			motor->status |= MXSF_MTR_AXIS_DISABLED;
		}

		/* Bit 3: Motor selector status */

		if ( ( status_byte & 0x8 ) == 0 ) {
			motor->status |= MXSF_MTR_DRIVE_FAULT;
		}

		/* Bit 4: At commanded velocity */

		if ( ( status_byte & 0x10 ) == 0 ) {
			motor->status |= MXSF_MTR_FOLLOWING_ERROR;
		}
		 
		/* Not sure what to do with the rest of these:
		 *
		 * Bit 5: Velocity profile mode
		 * Bit 6: Position (trapezoidal) profile mode
		 * Bit 7: Reserved
		 */

		break;
	default:
		break;
	}

	MX_DEBUG( 2,("%s: MX status word = %#lx", fname, motor->status));

	return MX_SUCCESSFUL_RESULT;
}


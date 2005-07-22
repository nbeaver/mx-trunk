/*
 * Name:    d_uglide.c
 *
 * Purpose: MX driver for BCW u-GLIDE motors from Oceaneering Space Systems.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "i_uglide.h"
#include "d_uglide.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_uglide_record_function_list = {
	NULL,
	mxd_uglide_create_record_structures,
	mxd_uglide_finish_record_initialization,
	NULL,
	mxd_uglide_print_structure,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_uglide_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_uglide_motor_function_list = {
	NULL,
	mxd_uglide_move_absolute,
	NULL,
	mxd_uglide_set_position,
	mxd_uglide_soft_abort,
	NULL,
	NULL,
	NULL,
	mxd_uglide_find_home_position,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler,
	NULL,
	NULL,
	mxd_uglide_get_extended_status
};

/* Compumotor motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_uglide_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_UGLIDE_STANDARD_FIELDS
};

long mxd_uglide_num_record_fields
		= sizeof( mxd_uglide_record_field_defaults )
			/ sizeof( mxd_uglide_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_uglide_rfield_def_ptr
			= &mxd_uglide_record_field_defaults[0];

#define UGLIDE_DEBUG	FALSE

/* A private function for the use of the driver. */

static mx_status_type
mxd_uglide_get_pointers( MX_MOTOR *motor,
			MX_UGLIDE_MOTOR **uglide_motor,
			MX_UGLIDE **uglide,
			const char *calling_fname )
{
	const char fname[] = "mxd_uglide_get_pointers()";

	MX_UGLIDE_MOTOR *uglide_motor_ptr;
	MX_RECORD *uglide_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_MOTOR structure passed by '%s' is NULL.",
			calling_fname );
	}

	uglide_motor_ptr = (MX_UGLIDE_MOTOR *)
				motor->record->record_type_struct;

	if ( uglide_motor_ptr == (MX_UGLIDE_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_UGLIDE_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( uglide_motor != (MX_UGLIDE_MOTOR **) NULL ) {
		*uglide_motor = uglide_motor_ptr;
	}

	if ( uglide != (MX_UGLIDE **) NULL ) {
		uglide_record = (*uglide_motor)->uglide_record;

		if ( uglide_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The uglide_record pointer for 'uglide_motor' '%s' is NULL.",
				motor->record->name );
		}

		*uglide = (MX_UGLIDE *) uglide_record->record_type_struct;

		if ( *uglide == (MX_UGLIDE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_UGLIDE pointer for 'uglide' record '%s' used by "
		"'uglide_motor' record '%s' is NULL.",
				uglide_record->name, motor->record->name );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_uglide_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_uglide_create_record_structures()";

	MX_MOTOR *motor;
	MX_UGLIDE_MOTOR *uglide_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	uglide_motor = (MX_UGLIDE_MOTOR *) malloc( sizeof(MX_UGLIDE_MOTOR) );

	if ( uglide_motor == (MX_UGLIDE_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_UGLIDE_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = uglide_motor;
	record->class_specific_function_list = &mxd_uglide_motor_function_list;

	motor->record = record;

	/* A u-GLIDE motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The u-GLIDE does not seem to have programmable acceleration
	 * so we set the acceleration type to MXF_MTR_ACCEL_NONE.
	 */

	motor->acceleration_type = MXF_MTR_ACCEL_NONE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_uglide_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_uglide_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_UGLIDE_MOTOR *uglide_motor;
	MX_UGLIDE *uglide;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_uglide_get_pointers( motor,
					&uglide_motor, &uglide, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_verify_driver_type( uglide_motor->uglide_record,
		MXR_INTERFACE, MXI_GENERIC, MXI_GEN_UGLIDE ) == FALSE )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The uglide record '%s' mentioned by 'uglide_motor' "
		"record '%s' is not actually a 'uglide' record.",
			uglide_motor->uglide_record->name, record->name );
	}

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Silently force the axis name to upper case. */

	if ( islower( (int) uglide_motor->axis_name ) )
		uglide_motor->axis_name =
			toupper( (int) uglide_motor->axis_name );

	/* Save a pointer to this record in the controller record. */

	switch( uglide_motor->axis_name ) {
	case 'X':
		uglide->x_motor_record = record;
		break;
	case 'Y':
		uglide->y_motor_record = record;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal motor axis name '%c' for motor '%s'.  "
		"The allowed axis names are 'X' and 'Y'.",
			uglide_motor->axis_name,
			record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_uglide_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_uglide_print_structure()";

	MX_MOTOR *motor;
	MX_UGLIDE_MOTOR *uglide_motor;
	MX_UGLIDE *uglide;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_uglide_get_pointers( motor,
					&uglide_motor, &uglide, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = UGLIDE motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  uglide record     = %s\n",
					uglide_motor->uglide_record->name );
	fprintf(file, "  axis name         = %d\n", uglide_motor->axis_name);

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

	fprintf(file, "\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_uglide_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxd_uglide_resynchronize()";

	MX_UGLIDE_MOTOR *uglide_motor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	uglide_motor = (MX_UGLIDE_MOTOR *) record->record_type_struct;

	if ( uglide_motor == (MX_UGLIDE_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_UGLIDE_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mx_resynchronize_record( uglide_motor->uglide_record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_uglide_move_absolute( MX_MOTOR *motor )
{
	const char fname[] = "mxd_uglide_move_absolute()";

	MX_UGLIDE_MOTOR *uglide_motor;
	MX_UGLIDE *uglide;
	char command[80];
	char command_name, other_axis_name;
	mx_status_type mx_status;

	mx_status = mxd_uglide_get_pointers( motor,
					&uglide_motor, &uglide, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( uglide->uglide_flags & MXF_UGLIDE_USE_RELATIVE_MODE ) {
		if ( uglide_motor->axis_name == 'X' ) {
			command_name = 'x';
		} else {
			command_name = 'y';
		}
	} else {
		if ( uglide_motor->axis_name == 'X' ) {
			command_name = 'X';
		} else {
			command_name = 'Y';
		}
	}

	sprintf( command, "%c %ld",
			command_name, motor->raw_destination.stepper );

	mx_status = mxi_uglide_command( uglide, command, UGLIDE_DEBUG );

	if ( mx_status.code == MXE_NOT_READY ) {
		if ( uglide_motor->axis_name == 'X' ) {
			other_axis_name = 'Y';
		} else {
			other_axis_name = 'X';
		}
		return mx_error( MXE_NOT_READY, fname,
	"Cannot start move for motor '%s' since the '%c' motor for this "
	"stage is already in motion and only one motor can move at a time.",
			motor->record->name, other_axis_name );
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		MX_DEBUG( 2,("%s: mx_status.code = %ld",
			fname, mx_status.code));

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_uglide_set_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_uglide_set_position()";

	MX_UGLIDE_MOTOR *uglide_motor;
	MX_UGLIDE *uglide;
	char command[10];
	mx_status_type mx_status;

	mx_status = mxd_uglide_get_pointers( motor,
					&uglide_motor, &uglide, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( (uglide->uglide_flags & MXF_UGLIDE_USE_RELATIVE_MODE) == 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"The position of motor '%s' can only be redefined when "
	"controller '%s' is in relative position mode, but it is currently "
	"in absolute position mode.",
			motor->record->name, uglide->record->name );
	}

	if ( motor->raw_set_position.stepper != 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"Motor '%s' can only have its current position redefined as zero.",
			motor->record->name );
	}

	if ( uglide_motor->axis_name == 'X' ) {
		strcpy( command, "a" );
	} else {
		strcpy( command, "b" );
	}

	mx_status = mxi_uglide_command( uglide, command, UGLIDE_DEBUG );

	if ( mx_status.code == MXE_NOT_READY ) {
		return mx_error( MXE_NOT_READY, fname,
		"Cannot redefine the position of motor '%s' since one of the "
		"motors belonging to controller '%s' is currently in motion.",
			motor->record->name,
			uglide->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_uglide_soft_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_uglide_soft_abort()";

	MX_UGLIDE_MOTOR *uglide_motor;
	MX_UGLIDE *uglide;
	mx_status_type mx_status;

	mx_status = mxd_uglide_get_pointers( motor,
					&uglide_motor, &uglide, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	/* Send the single character 's' to the controller. */

#if UGLIDE_DEBUG
	MX_DEBUG(-2,("%s: sending soft abort character 's' to '%s'",
			fname, uglide->record->name ));
#endif

	mx_status = mx_rs232_putline( uglide->rs232_record, "s", NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait a moment for the move to stop and then discard all
	 * existing responses from the controller.
	 */

	mx_msleep(500);

	mx_status = mx_rs232_discard_unread_input( uglide->rs232_record,
							UGLIDE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the controller record that the last command executed was
	 * a position report.
	 */

	uglide->last_response_code = MXF_UGLIDE_POSITION_REPORT;

	/* Refresh the position of the motors. */

	mx_status = mxi_uglide_command( uglide, "q", UGLIDE_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_uglide_find_home_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_uglide_find_home_position()";

	MX_UGLIDE_MOTOR *uglide_motor;
	MX_UGLIDE *uglide;
	char command[10];
	mx_status_type mx_status;

	mx_status = mxd_uglide_get_pointers( motor,
					&uglide_motor, &uglide, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	if ( uglide_motor->axis_name == 'X' ) {
		strcpy( command, "i" );
	} else {
		strcpy( command, "j" );
	}

	mx_status = mxi_uglide_command( uglide, command, UGLIDE_DEBUG );

	return mx_status;
}

#define MXD_UGLIDE_SET_HOME_SEARCH_SUCCEEDED_BIT \
		if ( motor->status & MXSF_MTR_IS_BUSY ) {                      \
			motor->status &= ( ~ MXSF_MTR_HOME_SEARCH_SUCCEEDED ); \
		} else {                                                       \
			motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;       \
		}

MX_EXPORT mx_status_type
mxd_uglide_get_extended_status( MX_MOTOR *motor )
{
	const char fname[] = "mxd_uglide_get_extended_status()";

	MX_UGLIDE_MOTOR *uglide_motor;
	MX_UGLIDE *uglide;
	mx_status_type mx_status;

	mx_status = mxd_uglide_get_pointers( motor,
					&uglide_motor, &uglide, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor->record->name));

	/* Ask for the current position. */

	mx_status = mxi_uglide_command( uglide, "q", UGLIDE_DEBUG );

	if ( mx_status.code == MXE_NOT_READY ) {

		motor->status |= MXSF_MTR_IS_BUSY;

		MXD_UGLIDE_SET_HOME_SEARCH_SUCCEEDED_BIT;

		return MX_SUCCESSFUL_RESULT;

	} else if ( mx_status.code != MXE_SUCCESS ) {
		MXD_UGLIDE_SET_HOME_SEARCH_SUCCEEDED_BIT;

		return mx_status;
	}

	if ( uglide->last_response_code == MXF_UGLIDE_MOVE_IN_PROGRESS ) {

		motor->status |= MXSF_MTR_IS_BUSY;

		MXD_UGLIDE_SET_HOME_SEARCH_SUCCEEDED_BIT;

		return MX_SUCCESSFUL_RESULT;
	}

	MXD_UGLIDE_SET_HOME_SEARCH_SUCCEEDED_BIT;

	/* The motor is not busy. */

	if ( uglide_motor->axis_name == 'X' ) {
		motor->raw_position.stepper = uglide->x_position;
	} else
	if ( uglide_motor->axis_name == 'Y' ) {
		motor->raw_position.stepper = uglide->y_position;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized axis name '%c' for motor '%s'.  "
			"The only legal axis names are 'X' and 'Y'.",
				uglide_motor->axis_name,
				motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}


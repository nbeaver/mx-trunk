/*
 * Name:    d_ggcs.c
 *
 * Purpose: MX driver for Bruker GGCS controlled motors.
 *
 * Warning: This driver is incomplete and would need further development
 *          to be useable.
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define GGCS_MOTOR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "d_ggcs.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_ggcs_motor_record_function_list = {
	mxd_ggcs_motor_initialize_type,
	mxd_ggcs_motor_create_record_structures,
	mxd_ggcs_motor_finish_record_initialization,
	mxd_ggcs_motor_delete_record,
	mxd_ggcs_motor_print_structure,
	mxd_ggcs_motor_read_parms_from_hardware,
	mxd_ggcs_motor_write_parms_to_hardware,
	mxd_ggcs_motor_open,
	mxd_ggcs_motor_close
};

MX_MOTOR_FUNCTION_LIST mxd_ggcs_motor_motor_function_list = {
	mxd_ggcs_motor_motor_is_busy,
	mxd_ggcs_motor_move_absolute,
	mxd_ggcs_motor_get_position,
	mxd_ggcs_motor_set_position,
	mxd_ggcs_motor_soft_abort,
	mxd_ggcs_motor_immediate_abort,
	mxd_ggcs_motor_positive_limit_hit,
	mxd_ggcs_motor_negative_limit_hit,
	mxd_ggcs_motor_find_home_position
};

/* GGCS motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_ggcs_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_GGCS_MOTOR_STANDARD_FIELDS
};

mx_length_type mxd_ggcs_motor_num_record_fields
		= sizeof( mxd_ggcs_motor_record_field_defaults )
			/ sizeof( mxd_ggcs_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ggcs_motor_rfield_def_ptr
			= &mxd_ggcs_motor_record_field_defaults[0];

/* === */

static mx_status_type
mxd_ggcs_motor_get_pointers( MX_MOTOR *motor,
				MX_GGCS_MOTOR **ggcs_motor,
				MX_GGCS **ggcs,
				const char *calling_fname )
{
	const char fname[] = "mxd_ggcs_motor_get_pointers()";

	MX_RECORD *ggcs_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MOTOR pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( ggcs_motor == (MX_GGCS_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_GGCS_MOTOR pointer passed by '%s' was NULL.",
		calling_fname );
	}

	if ( ggcs == (MX_GGCS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_GGCS pointer passed by '%s' was NULL.",
		calling_fname );
	}

	*ggcs_motor = (MX_GGCS_MOTOR *) (motor->record->record_type_struct);

	if ( *ggcs_motor == (MX_GGCS_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GGCS_MOTOR pointer for motor '%s' is NULL.",
				motor->record->name );
	}

	ggcs_record = (MX_RECORD *) ((*ggcs_motor)->ggcs_record);

	if ( ggcs_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Interface record pointer for GGCS motor '%s' is NULL.",
			motor->record->name );
	}

	*ggcs = (MX_GGCS *)(ggcs_record->record_type_struct);

	if ( *ggcs == (MX_GGCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GGCS pointer for GGCS motor record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_ggcs_motor_initialize_type( long type )
{
	/* Nothing needed here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_ggcs_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_GGCS_MOTOR *ggcs_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	ggcs_motor = (MX_GGCS_MOTOR *) malloc( sizeof(MX_GGCS_MOTOR) );

	if ( ggcs_motor == (MX_GGCS_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GGCS_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = ggcs_motor;
	record->class_specific_function_list
				= &mxd_ggcs_motor_motor_function_list;

	motor->record = record;

	/* A GGCS motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type status;

	status = mx_motor_finish_record_initialization( record );

	return status;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_delete_record( MX_RECORD *record )
{
	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_ggcs_motor_print_structure()";

	MX_MOTOR *motor;
	MX_RECORD *ggcs_record;
	MX_GGCS *ggcs;
	MX_GGCS_MOTOR *ggcs_motor;
	double position, move_deadband;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	ggcs_motor = (MX_GGCS_MOTOR *) (record->record_type_struct);

	if ( ggcs_motor == (MX_GGCS_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_GGCS_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	ggcs_record = (MX_RECORD *) (ggcs_motor->ggcs_record);

	if ( ggcs_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Interface record pointer for GGCS motor '%s' is NULL.",
			record->name );
	}

	ggcs = (MX_GGCS *) (ggcs_record->record_type_struct);

	if ( ggcs == (MX_GGCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_GGCS pointer for GGCS motor record '%s' is NULL.",
			record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = GGCS_MOTOR motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  port name         = %s\n", ggcs_record->name);
	fprintf(file, "  axis number       = %d\n", ggcs_motor->axis_number);

	status = mx_motor_get_position( record, &position );

	if ( status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position          = %g %s  (%g)\n",
			motor->position, motor->units,
			motor->raw_position.analog );
	fprintf(file, "  scale             = %g %s per GGCS unit.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%g)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	
	fprintf(file, "  negative limit    = %g %s  (%g)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit    = %g %s  (%g)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband     = %g %s  (%g)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_read_parms_from_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_ggcs_motor_read_parms_from_hardware()";

	MX_MOTOR *motor;
	MX_GGCS_MOTOR *ggcs_motor;
	MX_RECORD *ggcs_record;
	MX_GGCS *ggcs;
	double position;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	ggcs_motor = (MX_GGCS_MOTOR *) (record->record_type_struct);

	if ( ggcs_motor == (MX_GGCS_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GGCS_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	ggcs_record = (MX_RECORD *)
			(ggcs_motor->ggcs_record);

	if ( ggcs_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Interface record pointer for GGCS motor '%s' is NULL.",
			record->name );
	}

	ggcs = (MX_GGCS *)
			(ggcs_record->record_type_struct);

	if ( ggcs == (MX_GGCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_GGCS pointer for GGCS motor record '%s' is NULL.",
			record->name );
	}


	/* Get the current position.  This has the side effect of
	 * updating the position value in the MX_MOTOR structure.
	 */

	status = mx_motor_get_position( record, &position );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* At present, the position is the only parameter we read out.
	 * We rely on the non-volatile memory of the controller
	 * for the rest of the controller parameters.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_write_parms_to_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_ggcs_motor_write_parms_to_hardware()";

	MX_MOTOR *motor;
	MX_GGCS_MOTOR *ggcs_motor;
	MX_RECORD *ggcs_record;
	MX_GGCS *ggcs;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	ggcs_motor = (MX_GGCS_MOTOR *) (record->record_type_struct);

	if ( ggcs_motor == (MX_GGCS_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			"MX_GGCS_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	ggcs_record = (MX_RECORD *)
			(ggcs_motor->ggcs_record);

	if ( ggcs_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Interface record pointer for GGCS motor '%s' is NULL.",
			record->name );
	}

	ggcs = (MX_GGCS *)(ggcs_record->record_type_struct);

	if ( ggcs == (MX_GGCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_GGCS pointer for GGCS motor record '%s' is NULL.",
			record->name );
	}

	/* === Now we can set the motor position. === */

#if 0	/* For now, this is disabled. */

	status = mxd_ggcs_motor_set_position_steps(
			motor, motor->raw_motor_steps );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* At present, the position is the only parameter we set here.
	 * We rely on the non-volatile memory of the controller
	 * for the rest of the controller parameters.
	 */
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_open( MX_RECORD *record )
{
	/* Nothing to do. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_close( MX_RECORD *record )
{
	/* Nothing to do. */

	return MX_SUCCESSFUL_RESULT;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_ggcs_motor_motor_is_busy( MX_MOTOR *motor )
{
	const char fname[] = "mxd_ggcs_motor_motor_is_busy()";

	MX_GGCS *ggcs;
	MX_GGCS_MOTOR *ggcs_motor;
	char command[20];
	char response[80];
	mx_status_type status;

	status = mxd_ggcs_motor_get_pointers( motor,
			&ggcs_motor, &ggcs, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	sprintf( command, "%%%%%d", ggcs_motor->axis_number );

#if 0
	status = mxi_ggcs_command( ggcs, command,
			response, sizeof response, GGCS_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( response[0] == '1' ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}
#else
	strcpy( response, "" );

	motor->busy = TRUE;
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_move_absolute( MX_MOTOR *motor )
{
	const char fname[] = "mxd_ggcs_motor_move_absolute()";

	MX_GGCS *ggcs;
	MX_GGCS_MOTOR *ggcs_motor;
	char command[20];
	mx_status_type status;

	status = mxd_ggcs_motor_get_pointers( motor,
			&ggcs_motor, &ggcs, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Set the position to move to. */

	sprintf( command, "F%d,%g", ggcs_motor->axis_number,
					motor->raw_position.analog );

	status = mxi_ggcs_command( ggcs, command,
					NULL, 0, GGCS_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Command the move to start. */

	status = mxi_ggcs_command( ggcs, "D",
					NULL, 0, GGCS_MOTOR_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_get_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_ggcs_motor_get_position()";

	MX_GGCS *ggcs;
	MX_GGCS_MOTOR *ggcs_motor;
	char command[20];
	char response[80];
	double raw_position;
	int num_tokens;
	mx_status_type status;

	status = mxd_ggcs_motor_get_pointers( motor,
			&ggcs_motor, &ggcs, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	sprintf( command, "P%d", ggcs_motor->axis_number );

	status = mxi_ggcs_command( ggcs, command,
			response, sizeof response, GGCS_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_tokens = sscanf( response, "%lg", &raw_position );

	if ( num_tokens != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No position value seen in response to '%s' command.  "
		"Response seen = '%s'", command, response );
	}

	motor->raw_position.analog = raw_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_set_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_ggcs_motor_set_position()";

	MX_GGCS *ggcs;
	MX_GGCS_MOTOR *ggcs_motor;
	char command[100];
	mx_status_type status;

	status = mxd_ggcs_motor_get_pointers( motor,
			&ggcs_motor, &ggcs, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	sprintf( command, "%%%%%d", ggcs_motor->axis_number );

#if 0
	status = mxi_ggcs_command( ggcs, command, NULL, 0, GGCS_MOTOR_DEBUG );

	return status;
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_soft_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_ggcs_motor_soft_abort()";

	MX_GGCS *ggcs;
	MX_GGCS_MOTOR *ggcs_motor;
	mx_status_type status;

	status = mxd_ggcs_motor_get_pointers( motor,
			&ggcs_motor, &ggcs, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Send a CTRL-F */

	status = mxi_ggcs_command( ggcs, "\006",
					NULL, 0, GGCS_MOTOR_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_immediate_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_ggcs_motor_immediate_abort()";

	MX_GGCS *ggcs;
	MX_GGCS_MOTOR *ggcs_motor;
	mx_status_type status;

	status = mxd_ggcs_motor_get_pointers( motor,
			&ggcs_motor, &ggcs, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Send a CTRL-G */

	status = mxi_ggcs_command( ggcs, "\007",
					NULL, 0, GGCS_MOTOR_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_positive_limit_hit( MX_MOTOR *motor )
{
	const char fname[] = "mxd_ggcs_motor_positive_limit_hit()";

	MX_GGCS *ggcs;
	MX_GGCS_MOTOR *ggcs_motor;
	char response[80];
	int num_items, hardware_status, error_status;
	mx_status_type status;

	status = mxd_ggcs_motor_get_pointers( motor,
			&ggcs_motor, &ggcs, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxi_ggcs_command( ggcs, "U0",
				response, sizeof response, GGCS_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_items = sscanf( response, "%*s %*s %*s %*s %d %d",
					&hardware_status, &error_status );

	if ( num_items != 2 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Response from GGCS U0 command is unparseable.  Response = '%s'",
			response );
	}

	if ( hardware_status & 1 ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_negative_limit_hit( MX_MOTOR *motor )
{
	const char fname[] = "mxd_ggcs_motor_negative_limit_hit()";

	MX_GGCS *ggcs;
	MX_GGCS_MOTOR *ggcs_motor;
	char response[80];
	int num_items, hardware_status, error_status;
	mx_status_type status;

	status = mxd_ggcs_motor_get_pointers( motor,
			&ggcs_motor, &ggcs, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxi_ggcs_command( ggcs, "U0",
				response, sizeof response, GGCS_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_items = sscanf( response, "%*s %*s %*s %*s %d %d",
					&hardware_status, &error_status );

	if ( num_items != 2 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Response from GGCS U0 command is unparseable.  Response = '%s'",
			response );
	}

	if ( hardware_status & 1 ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ggcs_motor_find_home_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_ggcs_motor_find_home_position()";

	MX_GGCS *ggcs;
	MX_GGCS_MOTOR *ggcs_motor;
	char command[20];
	mx_status_type status;

	status = mxd_ggcs_motor_get_pointers( motor,
			&ggcs_motor, &ggcs, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	sprintf( command, "%%%%%d", ggcs_motor->axis_number );

#if 0
	status = mxi_ggcs_command( ggcs, command,
					NULL, 0, GGCS_MOTOR_DEBUG );

	return status;
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}


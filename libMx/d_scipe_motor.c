/*
 * Name:    d_scipe_motor.c 
 *
 * Purpose: MX motor driver for motors controlled by the SCIPE protocol
 *          developed by John Quintana of Northwestern University.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2003, 2010, 2012, 2015-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "i_scipe.h"
#include "d_scipe_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_scipe_motor_record_function_list = {
	NULL,
	mxd_scipe_motor_create_record_structures,
	mxd_scipe_motor_finish_record_initialization,
	NULL,
	mxd_scipe_motor_print_structure,
	mxd_scipe_motor_open,
	NULL,
	NULL,
	mxd_scipe_motor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_scipe_motor_motor_function_list = {
	NULL,
	mxd_scipe_motor_move_absolute,
	mxd_scipe_motor_get_position,
	NULL,
	mxd_scipe_motor_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler,
	NULL,
	mxd_scipe_motor_get_status
};

/* SCIPE motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_scipe_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_SCIPE_MOTOR_STANDARD_FIELDS
};

long mxd_scipe_motor_num_record_fields
		= sizeof( mxd_scipe_motor_record_field_defaults )
			/ sizeof( mxd_scipe_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_scipe_motor_rfield_def_ptr
			= &mxd_scipe_motor_record_field_defaults[0];

#define SCIPE_MOTOR_DEBUG FALSE

/* A private function for the use of the driver. */

static mx_status_type
mxd_scipe_motor_get_pointers( MX_MOTOR *motor,
			MX_SCIPE_MOTOR **scipe_motor,
			MX_SCIPE_SERVER **scipe_server,
			const char *calling_fname )
{
	static const char fname[] = "mxd_scipe_motor_get_pointers()";

	MX_RECORD *scipe_server_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scipe_motor == (MX_SCIPE_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCIPE_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scipe_server == (MX_SCIPE_SERVER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCIPE_SERVER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MOTOR pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*scipe_motor = (MX_SCIPE_MOTOR *) motor->record->record_type_struct;

	if ( *scipe_motor == (MX_SCIPE_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCIPE_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	scipe_server_record = (*scipe_motor)->scipe_server_record;

	if ( scipe_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'scipe_server_record' pointer for record '%s' is NULL.",
			motor->record->name );
	}

	*scipe_server = (MX_SCIPE_SERVER *)
				scipe_server_record->record_type_struct;

	if ( *scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SCIPE_SERVER pointer for the SCIPE server '%s' used by '%s' is NULL.",
			scipe_server_record->name, motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_scipe_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_scipe_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_SCIPE_MOTOR *scipe_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	scipe_motor = (MX_SCIPE_MOTOR *) malloc( sizeof(MX_SCIPE_MOTOR) );

	if ( scipe_motor == (MX_SCIPE_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCIPE_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = scipe_motor;
	record->class_specific_function_list
				= &mxd_scipe_motor_motor_function_list;

	motor->record = record;

	/* An SCIPE motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_motor_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type status;

	status = mx_motor_finish_record_initialization( record );

	return status;
}

MX_EXPORT mx_status_type
mxd_scipe_motor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_scipe_motor_print_structure()";

	MX_MOTOR *motor;
	MX_SCIPE_MOTOR *scipe_motor;
	MX_SCIPE_SERVER *scipe_server;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_scipe_motor_get_pointers( motor,
				&scipe_motor, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type       = SCIPE_MOTOR.\n\n");

	fprintf(file, "  name             = %s\n", record->name);
	fprintf(file, "  SCIPE server     = %s\n",
					scipe_motor->scipe_server_record->name);
	fprintf(file, "  SCIPE motor name = %s\n\n",
					scipe_motor->scipe_motor_name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position         = %g %s  (%g).\n",
		motor->position, motor->units, 
		motor->raw_position.analog );
	fprintf(file, "  scale            = %g %s per SCIPE EGU.\n",
		motor->scale, motor->units );
	fprintf(file, "  offset           = %g %s.\n",
		motor->offset, motor->units );
	fprintf(file, "  backlash         = %g %s  (%g).\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	fprintf(file, "  negative limit   = %g %s  (%g).\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );
	fprintf(file, "  positive limit   = %g %s  (%g).\n",
	    motor->positive_limit, motor->units,
	    motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband    = %g %s  (%g).\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_scipe_motor_open()";

	MX_MOTOR *motor;
	MX_SCIPE_MOTOR *scipe_motor;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_scipe_motor_get_pointers( motor,
				&scipe_motor, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out whether or not the SCIPE server knows about this motor. */

	snprintf( command, sizeof(command),
			"%s desc", scipe_motor->scipe_motor_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for motor '%s'.  Response = '%s'",
				command, record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_motor_resynchronize( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_scipe_motor_move_absolute()";

	MX_SCIPE_MOTOR *scipe_motor;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_motor_get_pointers( motor,
				&scipe_motor, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
			"%s movenow %g",
			scipe_motor->scipe_motor_name, 
			motor->raw_destination.analog );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for motor '%s'.  Response = '%s'",
				command, motor->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_scipe_motor_get_position()";

	MX_SCIPE_MOTOR *scipe_motor;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	double raw_position;
	int num_items, scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_motor_get_pointers( motor,
				&scipe_motor, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
			"%s position", scipe_motor->scipe_motor_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for motor '%s'.  Response = '%s'",
				command, motor->record->name, response );
	}

	num_items = sscanf( result_ptr, "%lg", &raw_position );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Cannot find the motor position value in the response from SCIPE server '%s' "
"for SCIPE motor '%s'.  Server response = '%s'",
			scipe_server->record->name,
			motor->record->name,
			response );
	}

	motor->raw_position.analog = raw_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_scipe_motor_soft_abort()";

	MX_SCIPE_MOTOR *scipe_motor;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_motor_get_pointers( motor,
				&scipe_motor, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
			"%s halt", scipe_motor->scipe_motor_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_scipe_motor_get_status()";

	MX_SCIPE_MOTOR *scipe_motor;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_motor_get_pointers( motor,
				&scipe_motor, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current status of this motor. */

	snprintf( command, sizeof(command),
			"%s status", scipe_motor->scipe_motor_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	switch( scipe_response_code ) {
	case MXF_SCIPE_NOT_MOVING:
		break;
	case MXF_SCIPE_MOVING:
		motor->status |= MXSF_MTR_IS_BUSY;
		break;
	case MXF_SCIPE_UPPER_BOUND_FAULT:
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		break;
	case MXF_SCIPE_LOWER_BOUND_FAULT:
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		break;
	case MXF_SCIPE_ACTUATOR_GENERAL_FAULT:
		motor->status |= MXSF_MTR_ERROR;
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Receive the unexpected response code %d for the command '%s' "
"from the SCIPE server '%s' for SCIPE motor '%s'.  Server response = '%s'",
			scipe_response_code,
			command,
			scipe_server->record->name,
			motor->record->name,
			response );
	}
	return MX_SUCCESSFUL_RESULT;
}


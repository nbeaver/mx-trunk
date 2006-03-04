/*
 * Name:    d_bluice_motor.c
 *
 * Purpose: MX driver for Blu-Ice controlled motors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_bluice.h"
#include "d_bluice_motor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_bluice_motor_record_function_list = {
	NULL,
	mxd_bluice_motor_create_record_structures,
	mxd_bluice_motor_finish_record_initialization,
	NULL,
	mxd_bluice_motor_print_structure,
	NULL,
	NULL,
	mxd_bluice_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_bluice_motor_motor_function_list = {
	NULL,
	mxd_bluice_motor_move_absolute,
	mxd_bluice_motor_get_position,
	mxd_bluice_motor_set_position,
	mxd_bluice_motor_soft_abort,
	mxd_bluice_motor_immediate_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bluice_motor_get_parameter,
	mxd_bluice_motor_set_parameter,
	NULL,
	mxd_bluice_motor_get_status
};

/* Blu-Ice motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_bluice_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_BLUICE_MOTOR_STANDARD_FIELDS
};

long mxd_bluice_motor_num_record_fields
		= sizeof( mxd_bluice_motor_record_field_defaults )
			/ sizeof( mxd_bluice_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_motor_rfield_def_ptr
			= &mxd_bluice_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_bluice_motor_get_pointers( MX_MOTOR *motor,
			MX_BLUICE_MOTOR **bluice_motor,
			MX_BLUICE_SERVER **bluice_server,
			MX_BLUICE_FOREIGN_DEVICE **foreign_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bluice_motor_get_pointers()";

	MX_BLUICE_MOTOR *bluice_motor_ptr;
	MX_RECORD *bluice_server_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( bluice_motor == (MX_BLUICE_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bluice_motor_ptr = (MX_BLUICE_MOTOR *)
				motor->record->record_type_struct;

	if ( bluice_motor_ptr == (MX_BLUICE_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( bluice_motor != (MX_BLUICE_MOTOR **) NULL ) {
		*bluice_motor = bluice_motor_ptr;
	}

	bluice_server_record = bluice_motor_ptr->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' "
		"is NULL.", motor->record->name );
	}

	switch( bluice_server_record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
	case MXN_BLUICE_DHS_SERVER:
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Blu-Ice server record '%s' should be either of type "
		"'bluice_dcss_server' or 'bluice_dhs_server'.  Instead, it is "
		"of type '%s'.",
			bluice_server_record->name,
			mx_get_driver_name( bluice_server_record ) );
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = (MX_BLUICE_SERVER *)
				bluice_server_record->record_class_struct;

		if ( (*bluice_server) == (MX_BLUICE_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BLUICE_SERVER pointer for Blu-Ice server "
			"record '%s' used by record '%s' is NULL.",
				bluice_server_record->name,
				motor->record->name );
		}
	}

	if ( foreign_motor != (MX_BLUICE_FOREIGN_DEVICE **) NULL ) {
		*foreign_motor = bluice_motor_ptr->foreign_device;

		if ( (*foreign_motor) == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"The foreign_device pointer for motor '%s' "
			"has not been initialized.", motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_bluice_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_bluice_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_BLUICE_MOTOR *bluice_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	bluice_motor = (MX_BLUICE_MOTOR *) malloc( sizeof(MX_BLUICE_MOTOR) );

	if ( bluice_motor == (MX_BLUICE_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_BLUICE_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = bluice_motor;
	record->class_specific_function_list =
			&mxd_bluice_motor_motor_function_list;

	motor->record = record;
	bluice_motor->record = record;

	/* A Blu-Ice controlled motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* Blu-Ice reports acceleration time in milliseconds. */

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

	bluice_motor->foreign_device = NULL;

	motor->status = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_bluice_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	unsigned long saved_motor_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	saved_motor_status = motor->status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( saved_motor_status & MXSF_MTR_SOFTWARE_ERROR ) {
		motor->status |= MXSF_MTR_SOFTWARE_ERROR;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_bluice_motor_print_structure()";

	MX_MOTOR *motor;
	MX_BLUICE_MOTOR *bluice_motor;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = Blu-Ice motor.\n\n");

	fprintf(file, "  name               = %s\n", record->name);
	fprintf(file, "  server             = %s\n",
				    bluice_motor->bluice_server_record->name);
	fprintf(file, "  Blu-Ice name       = %s\n",
					bluice_motor->bluice_name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position           = %g %s  (%g)\n",
			motor->position, motor->units,
			motor->raw_position.analog );
	fprintf(file, "  scale              = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset             = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash           = %g %s  (%g)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	
	fprintf(file, "  negative limit     = %g %s  (%g)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit     = %g %s  (%g)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband      = %g %s  (%g)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  speed              = %g %s/s  (%g steps/s)\n",
		speed, motor->units,
		motor->raw_speed );

	fprintf(file, "\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_bluice_motor_open()";

	MX_MOTOR *motor;
	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_bluice_motor_get_pointers( motor,
				&bluice_motor, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait 5 seconds for the Blu-Ice server thread to assign a value to
	 * the foreign_motor pointer.
	 */

#if BLUICE_MOTOR_DEBUG
	MX_DEBUG(-2,
	    ("%s: About to wait for device pointer initialization.", fname));
#endif

	mx_status = mx_bluice_wait_for_device_pointer_initialization(
						bluice_server,
						bluice_motor->bluice_name,
						MXT_BLUICE_FOREIGN_MOTOR,
		 				&(bluice_server->motor_array),
						&(bluice_server->num_motors),
		  				&(bluice_motor->foreign_device),
						5.0 );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if BLUICE_MOTOR_DEBUG
	MX_DEBUG(-2,
	("%s: Successfully waited for device pointer initialization.", fname));
#endif

	bluice_motor->foreign_device->u.motor.mx_motor = motor;

	return MX_SUCCESSFUL_RESULT;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_bluice_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_move_absolute()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_motor;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor, &bluice_motor,
					&bluice_server, &foreign_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		snprintf( command, sizeof(command),
				"gtos_start_motor_move %s %g",
				bluice_motor->bluice_name,
				motor->raw_destination.analog );
		break;
	case MXN_BLUICE_DHS_SERVER:
		snprintf( command, sizeof(command),
				"stoh_start_motor_move %s %g",
				bluice_motor->bluice_name,
				motor->raw_destination.analog );
		break;
	}

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	foreign_motor->u.motor.move_in_progress = TRUE;

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0, -1 );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_get_position()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_motor;
	mx_status_type mx_status;
	long mx_status_code;

	mx_status = mxd_bluice_motor_get_pointers( motor, &bluice_motor,
					&bluice_server, &foreign_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	motor->raw_position.analog = foreign_motor->u.motor.position;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_set_position()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
				&bluice_motor, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		snprintf( command, sizeof(command),
				"gtos_set_motor_position %s %g",
				bluice_motor->bluice_name,
				motor->raw_set_position.analog );
		break;
	case MXN_BLUICE_DHS_SERVER:
		snprintf( command, sizeof(command),
				"stoh_set_motor_position %s %g",
				bluice_motor->bluice_name,
				motor->raw_set_position.analog );
		break;
	}

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0, -1 );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_soft_abort()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
				&bluice_motor, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		strlcpy( command, "gtos_abort_all soft", sizeof(command) );
		break;
	case MXN_BLUICE_DHS_SERVER:
		strlcpy( command, "stoh_abort_all soft", sizeof(command) );
		break;
	}

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0, -1 );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_immediate_abort()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
				&bluice_motor, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		strlcpy( command, "gtos_abort_all hard", sizeof(command) );
		break;
	case MXN_BLUICE_DHS_SERVER:
		strlcpy( command, "stoh_abort_all hard", sizeof(command) );
		break;
	}

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0, -1 );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_get_parameter()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_motor;
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor, &bluice_motor,
					&bluice_server, &foreign_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));


	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		motor->raw_speed = foreign_motor->u.motor.scale_factor
					* foreign_motor->u.motor.speed;
		break;
	case MXLV_MTR_ACCELERATION_TIME:
		/* Blu-Ice uses milliseconds for acceleration time
		 * while MX uses seconds.
		 */

		motor->acceleration_time = 0.001 *
				foreign_motor->u.motor.acceleration_time;
		break;
	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_set_parameter()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_motor;
	char command[200], command_name[40];
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor, &bluice_motor,
					&bluice_server, &foreign_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	if ( foreign_motor->u.motor.is_pseudo ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Setting motor parameters for Blu-Ice pseudomotor '%s' "
		"used by record '%s' is not supported.",
			bluice_motor->bluice_name,
			motor->record->name );
	}

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		foreign_motor->u.motor.speed =
			mx_divide_safely( motor->raw_speed,
				foreign_motor->u.motor.scale_factor );
		break;
	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		strlcpy( command_name, "gtos_configure_device",
						sizeof(command_name) );
		break;
	case MXN_BLUICE_DHS_SERVER:
		strlcpy( command_name, "stoh_configure_device",
						sizeof(command_name) );
		break;
	}

	snprintf( command, sizeof(command),
		"%s %s %g %g %g %g %g %g %g %d %d %d %d %d",
		command_name,
		foreign_motor->name,
		foreign_motor->u.motor.position,
		foreign_motor->u.motor.upper_limit,
		foreign_motor->u.motor.lower_limit,
		foreign_motor->u.motor.scale_factor,
		foreign_motor->u.motor.speed,
		foreign_motor->u.motor.acceleration_time,
		foreign_motor->u.motor.backlash,
		(int) foreign_motor->u.motor.lower_limit_on,
		(int) foreign_motor->u.motor.upper_limit_on,
		(int) foreign_motor->u.motor.motor_lock_on,
		(int) foreign_motor->u.motor.backlash_on,
		(int) foreign_motor->u.motor.reverse_on );

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0, -1 );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_get_status()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_motor;
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor, &bluice_motor,
					&bluice_server, &foreign_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( foreign_motor->u.motor.move_in_progress ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	} else {
		motor->status &= ( ~ MXSF_MTR_IS_BUSY );
	}

	MX_DEBUG( 2,("%s: MX status word = %#lx", fname, motor->status));

	return MX_SUCCESSFUL_RESULT;
}


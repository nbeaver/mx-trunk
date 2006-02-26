/*
 * Name:    d_record_field_motor.c
 *
 * Purpose: MX driver for a pseudomotor that implements its functionality
 *          by reading from and writing to MX record fields belonging to
 *          other MX records.
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

#define MXD_RECORD_FIELD_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_array.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_hrt_debug.h"
#include "mx_motor.h"
#include "d_record_field_motor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_record_field_motor_record_function_list = {
	NULL,
	mxd_record_field_motor_create_record_structures,
	mxd_record_field_motor_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_record_field_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_record_field_motor_motor_function_list = {
	mxd_record_field_motor_busy,
	mxd_record_field_motor_move_absolute,
	mxd_record_field_motor_get_position,
	mxd_record_field_motor_set_position,
	mxd_record_field_motor_soft_abort,
	mxd_record_field_motor_immediate_abort,
	mxd_record_field_motor_positive_limit_hit,
	mxd_record_field_motor_negative_limit_hit
};

/* Picomotor motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_record_field_motor_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_RECORD_FIELD_MOTOR_STANDARD_FIELDS
};

long mxd_record_field_motor_num_record_fields
		= sizeof( mxd_record_field_motor_rf_defaults )
		/ sizeof( mxd_record_field_motor_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_record_field_motor_rfield_def_ptr
			= &mxd_record_field_motor_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_record_field_motor_get_pointers( MX_MOTOR *motor,
			MX_RECORD_FIELD_MOTOR **record_field_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_record_field_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( record_field_motor == (MX_RECORD_FIELD_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*record_field_motor = (MX_RECORD_FIELD_MOTOR *)
				motor->record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_record_field_motor_get_long( MX_RECORD_FIELD *field, long *long_value )
{
	void *field_value_ptr;
	mx_status_type mx_status;

	field_value_ptr = mx_get_field_value_pointer( field );

	mx_status = mx_convert_and_copy_array( field_value_ptr,
						field->datatype,
						field->num_dimensions,
						field->dimension,
						field->data_element_size,
						long_value,
						MXFT_LONG, 0, NULL, NULL );
	return mx_status;
}

static mx_status_type
mxd_record_field_motor_set_long( MX_RECORD_FIELD *field, long long_value )
{
	void *field_value_ptr;
	mx_status_type mx_status;

	field_value_ptr = mx_get_field_value_pointer( field );

	mx_status = mx_convert_and_copy_array( &long_value,
						MXFT_LONG, 0, NULL, NULL,
						field_value_ptr,
						field->datatype,
						field->num_dimensions,
						field->dimension,
						field->data_element_size );
	return mx_status;
}

static mx_status_type
mxd_record_field_motor_get_double( MX_RECORD_FIELD *field, double *double_value)
{
	void *field_value_ptr;
	mx_status_type mx_status;

	field_value_ptr = mx_get_field_value_pointer( field );

	mx_status = mx_convert_and_copy_array( field_value_ptr,
						field->datatype,
						field->num_dimensions,
						field->dimension,
						field->data_element_size,
						double_value,
						MXFT_DOUBLE, 0, NULL, NULL );
	return mx_status;
}

static mx_status_type
mxd_record_field_motor_set_double( MX_RECORD_FIELD *field, double double_value )
{
	void *field_value_ptr;
	mx_status_type mx_status;

	field_value_ptr = mx_get_field_value_pointer( field );

	mx_status = mx_convert_and_copy_array( &double_value,
						MXFT_DOUBLE, 0, NULL, NULL,
						field_value_ptr,
						field->datatype,
						field->num_dimensions,
						field->dimension,
						field->data_element_size );
	return mx_status;
}

static mx_status_type
mxd_record_field_motor_get_disable_flag(
				MX_RECORD_FIELD_MOTOR *record_field_motor,
				long *disable_flag )
{
	static const char fname[] =
		"mxd_record_field_motor_get_disable_flag()";

	MX_RECORD_FIELD_HANDLER *handler;
	mx_status_type mx_status;

	if ( record_field_motor == (MX_RECORD_FIELD_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD_MOTOR pointer passed was NULL." );
	}
	if ( disable_flag == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'disable_flag' pointer passed was NULL." );
	}

	handler = record_field_motor->disable_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL ) {
		*disable_flag = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_GET );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_get_long( handler->record_field,
						disable_flag );
	return mx_status;
}

#if 0
static mx_status_type
mxd_record_field_motor_set_disable_flag(
				MX_RECORD_FIELD_MOTOR *record_field_motor,
				long disable_flag )
{
	static const char fname[] =
		"mxd_record_field_motor_set_disable_flag()";

	MX_RECORD_FIELD_HANDLER *handler;
	mx_status_type mx_status;

	if ( record_field_motor == (MX_RECORD_FIELD_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD_MOTOR pointer passed was NULL." );
	}

	handler = record_field_motor->disable_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mxd_record_field_motor_set_long( handler->record_field,
						disable_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_PUT );

	return mx_status;
}
#endif

static mx_status_type
mxd_record_field_motor_get_command_in_progress_flag(
				MX_RECORD_FIELD_MOTOR *record_field_motor,
				long *command_in_progress_flag )
{
	static const char fname[] =
		"mxd_record_field_motor_get_command_in_progress_flag()";

	MX_RECORD_FIELD_HANDLER *handler;
	unsigned long record_field_motor_flags;
	long disable_flag;
	mx_status_type mx_status;

	if ( record_field_motor == (MX_RECORD_FIELD_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD_MOTOR pointer passed was NULL." );
	}
	if ( command_in_progress_flag == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'command_in_progress_flag' pointer passed was NULL." );
	}

	handler = record_field_motor->command_in_progress_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL ) {
		*command_in_progress_flag = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If the disable_flag is set, then command_in_progress is always 0. */

	mx_status = mxd_record_field_motor_get_disable_flag( record_field_motor,
								&disable_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( disable_flag != 0 ) {
		*command_in_progress_flag = FALSE;
	} else {
		/* Get the command_in_progress flag. */

		mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_GET );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_record_field_motor_get_long(
						handler->record_field,
						command_in_progress_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* If a command was previously in progress, we need to store the
	 * ending time of the command.
	 */

	if ( ( *command_in_progress_flag == FALSE )
	  && ( record_field_motor->command_in_progress != FALSE ) )
	{
		MX_HRT_END( record_field_motor->in_progress_interval );

		record_field_motor_flags =
			record_field_motor->record_field_motor_flags;

		if ( record_field_motor_flags & MXF_RFM_SHOW_IN_PROGRESS_DELAY )
		{
			MX_HRT_RESULTS(record_field_motor->in_progress_interval,
			    fname, "command_in_progress delay for motor '%s'",
				record_field_motor->record->name);
		}
	}

	record_field_motor->command_in_progress = *command_in_progress_flag;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_record_field_motor_set_command_in_progress_flag(
				MX_RECORD_FIELD_MOTOR *record_field_motor,
				long command_in_progress_flag )
{
	static const char fname[] =
		"mxd_record_field_motor_set_command_in_progress_flag()";

	MX_RECORD_FIELD_HANDLER *handler;
	long disable_flag;
	mx_status_type mx_status;

	if ( record_field_motor == (MX_RECORD_FIELD_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD_MOTOR pointer passed was NULL." );
	}

	handler = record_field_motor->command_in_progress_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL )
		return MX_SUCCESSFUL_RESULT;

	/* If the disable_flag is set, then set command_in_progress to 0. */

	mx_status = mxd_record_field_motor_get_disable_flag( record_field_motor,
								&disable_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( disable_flag != 0 ) {
		command_in_progress_flag = FALSE;
	}

	/* Store a local copy of the command_in_progress flag. */

	record_field_motor->command_in_progress = command_in_progress_flag;

	/* Record the start time of the command_in_progress interval. */

	if ( command_in_progress_flag != FALSE ) {
		MX_HRT_START(record_field_motor->in_progress_interval);
	}

	/* Send the command_in_progress flag. */

	mx_status = mxd_record_field_motor_set_long( handler->record_field,
						command_in_progress_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_PUT );

	return mx_status;
}

static mx_status_type
mxd_record_field_motor_set_handler( MX_RECORD_FIELD_MOTOR *record_field_motor,
					MX_RECORD_FIELD_HANDLER **handler_ptr,
					char *handler_name_ptr )
{
	static const char fname[] = "mxd_record_field_motor_set_handler()";

	MX_RECORD *record_list, *record;
	MX_RECORD_FIELD *field;
	char *record_name_ptr, *field_name_ptr, *last_period_ptr;
	char handler_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	mx_status_type mx_status;

	if ( record_field_motor == (MX_RECORD_FIELD_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD_MOTOR pointer passed was NULL." );
	}
	if ( handler_ptr == (MX_RECORD_FIELD_HANDLER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD_HANDLER pointer passed was NULL." );
	}
	if ( record_field_motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The 'record' pointer for the MX_RECORD_FIELD_POINTER passed is NULL.");
	}

	record_list = record_field_motor->record->list_head;

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'list_head' pointer for record '%s' is NULL.",
			record_field_motor->record->name );
	}

	*handler_ptr = NULL;

	if ( handler_name_ptr == NULL )
		return MX_SUCCESSFUL_RESULT;

	/* Save a temporary copy of the handler name. */

	strlcpy( handler_name, handler_name_ptr,
			MXU_RECORD_FIELD_NAME_LENGTH );

	/* If the handler name is of length zero, set the handler pointer
	 * to NULL and return.
	 */

	if ( strlen( handler_name ) == 0 )
		return MX_SUCCESSFUL_RESULT;

	/* Find and null terminate the record name and field name in
	 * the temporary copy of the handler name.
	 */

	record_name_ptr = handler_name;

	last_period_ptr = strrchr( handler_name, '.' );

	if ( last_period_ptr == (char *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested handler name '%s' does not have a period '.' "
		"character in it, so it cannot be used as the full name of "
		"an MX record field.", handler_name );
	}

	field_name_ptr = last_period_ptr + 1;

	if ( *field_name_ptr == '\0' ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The last occurence of a period '.' character in the "
		"requested handler name '%s' does not have any characters "
		"after it, so it cannot be used as the full name of "
		"an MX record field.", handler_name );
	}

	*last_period_ptr = '\0';

	/* Find the requested record in the MX database. */

	record = mx_get_record( record_list, record_name_ptr );

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The record '%s' specified in handler name '%s' cannot "
		"be found in the MX database.",
			record_name_ptr, handler_name_ptr );
	}

	field = mx_get_record_field( record, field_name_ptr );

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The record field '%s' cannot be found in record '%s' "
		"specified for handler '%s'.",
			field_name_ptr, record->name, handler_name_ptr );
	}

	*handler_ptr = (MX_RECORD_FIELD_HANDLER *)
				malloc( sizeof(MX_RECORD_FIELD_HANDLER) );

	if (*handler_ptr == (MX_RECORD_FIELD_HANDLER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_RECORD_FIELD_HANDLER structure for handler '%s'.",
			handler_name_ptr );
	}

	(*handler_ptr)->record = record;
	(*handler_ptr)->record_field = field;

	/* Set up record processing for the record. */

	mx_status = mx_initialize_record_processing( record );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxd_record_field_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_record_field_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_RECORD_FIELD_MOTOR *record_field_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	record_field_motor = (MX_RECORD_FIELD_MOTOR *)
				malloc( sizeof(MX_RECORD_FIELD_MOTOR) );

	if ( record_field_motor == (MX_RECORD_FIELD_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_RECORD_FIELD_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = record_field_motor;
	record->class_specific_function_list
				= &mxd_record_field_motor_motor_function_list;

	motor->record = record;
	record_field_motor->record = record;

	/* A record field motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_record_field_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_record_field_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_RECORD_FIELD_MOTOR *record_field_motor;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_record_field_motor_get_pointers( motor,
						&record_field_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_record_field_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_record_field_motor_open()";

	MX_MOTOR *motor;
	MX_RECORD_FIELD_MOTOR *record_field_motor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_record_field_motor_get_pointers( motor,
						&record_field_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_handler( record_field_motor,
			&(record_field_motor->disable_handler),
			record_field_motor->disable_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_handler( record_field_motor,
			&(record_field_motor->command_in_progress_handler),
			record_field_motor->command_in_progress_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_handler( record_field_motor,
			&(record_field_motor->busy_handler),
			record_field_motor->busy_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_handler( record_field_motor,
			&(record_field_motor->move_absolute_handler),
			record_field_motor->move_absolute_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_handler( record_field_motor,
			&(record_field_motor->get_position_handler),
			record_field_motor->get_position_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_handler( record_field_motor,
			&(record_field_motor->set_position_handler),
			record_field_motor->set_position_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_handler( record_field_motor,
			&(record_field_motor->soft_abort_handler),
			record_field_motor->soft_abort_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_handler( record_field_motor,
			&(record_field_motor->immediate_abort_handler),
			record_field_motor->immediate_abort_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_handler( record_field_motor,
			&(record_field_motor->positive_limit_hit_handler),
			record_field_motor->positive_limit_hit_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_handler( record_field_motor,
			&(record_field_motor->negative_limit_hit_handler),
			record_field_motor->negative_limit_hit_name );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_record_field_motor_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_record_field_motor_busy()";

	MX_RECORD_FIELD_MOTOR *record_field_motor;
	MX_RECORD_FIELD_HANDLER *handler;
	long command_in_progress, busy_field_value;
	mx_status_type mx_status;

	mx_status = mxd_record_field_motor_get_pointers( motor,
						&record_field_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'", fname, motor->record->name));

	motor->busy = FALSE;

	/* If a previous command is still in progress, we declare the motor
	 * to be 'busy'.
	 */

	mx_status = mxd_record_field_motor_get_command_in_progress_flag(
				record_field_motor, &command_in_progress );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( command_in_progress ) {
		motor->busy = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If there is no busy handler, we declare the motor to _not_ be busy */

	handler = record_field_motor->busy_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL ) {
		motor->busy = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If there is a busy handler, process the specified record field
	 * to get the busy status.
	 */

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_GET );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_get_long( handler->record_field,
							&busy_field_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy_field_value ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_record_field_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_record_field_motor_move_absolute()";

	MX_RECORD_FIELD_MOTOR *record_field_motor;
	MX_RECORD_FIELD_HANDLER *handler;
	mx_status_type mx_status;

	mx_status = mxd_record_field_motor_get_pointers( motor,
						&record_field_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'", 
		fname, motor->record->name));

	handler = record_field_motor->move_absolute_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"No value was specified for the 'move_absolute_name' field "
		"in the record description for motor '%s'.",
			motor->record->name );
	}

	mx_status = mxd_record_field_motor_set_double( handler->record_field,
						motor->raw_destination.analog );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_PUT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_set_command_in_progress_flag(
						record_field_motor, 1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_record_field_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_record_field_motor_get_position()";

	MX_RECORD_FIELD_MOTOR *record_field_motor;
	MX_RECORD_FIELD_HANDLER *handler;
	mx_status_type mx_status;

	mx_status = mxd_record_field_motor_get_pointers( motor,
						&record_field_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'", fname, motor->record->name));

	/* If there is no get_position handler, copy the last commanded
	 * destination to the position variable.
	 */

	handler = record_field_motor->get_position_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL ) {
		motor->raw_position.analog = motor->raw_destination.analog;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If there is a get_position handler, process the specified
	 * record field to get the position.
	 */

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_GET );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_get_double( handler->record_field,
						&(motor->raw_position.analog) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_record_field_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_record_field_motor_set_position()";

	MX_RECORD_FIELD_MOTOR *record_field_motor;
	MX_RECORD_FIELD_HANDLER *handler;
	mx_status_type mx_status;

	mx_status = mxd_record_field_motor_get_pointers( motor,
						&record_field_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'", fname, motor->record->name));

	/* If there is no set_position handler, copy the specified new
	 * position to the position variable.
	 */

	handler = record_field_motor->set_position_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL ) {
		motor->raw_position.analog = motor->raw_set_position.analog;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If there is a set_position handler, process the specified
	 * record field to set the position.
	 */

	mx_status = mxd_record_field_motor_set_double( handler->record_field,
					motor->raw_set_position.analog );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_PUT );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_record_field_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_record_field_motor_soft_abort()";

	MX_RECORD_FIELD_MOTOR *record_field_motor;
	MX_RECORD_FIELD_HANDLER *handler;
	mx_status_type mx_status;

	mx_status = mxd_record_field_motor_get_pointers( motor,
						&record_field_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'", fname, motor->record->name));

	/* If there is no soft_abort handler, do nothing. */

	handler = record_field_motor->soft_abort_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL )
		return MX_SUCCESSFUL_RESULT;

	/* If there is a soft_abort handler, process the specified
	 * record field to send the abort command.
	 */

	mx_status = mxd_record_field_motor_set_long( handler->record_field,
						motor->soft_abort );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_PUT );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_record_field_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_record_field_motor_immediate_abort()";

	MX_RECORD_FIELD_MOTOR *record_field_motor;
	MX_RECORD_FIELD_HANDLER *handler;
	mx_status_type mx_status;

	mx_status = mxd_record_field_motor_get_pointers( motor,
						&record_field_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'", fname, motor->record->name));

	/* If there is no immediate_abort handler, invoke the
	 * soft_abort handler.
	 */

	handler = record_field_motor->immediate_abort_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL ) {
		mx_status = mxd_record_field_motor_soft_abort( motor );

		return mx_status;
	}

	/* If there is a immediate_abort handler, process the specified
	 * record field to send the abort command.
	 */

	mx_status = mxd_record_field_motor_set_long( handler->record_field,
						motor->immediate_abort );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_PUT );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_record_field_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_record_field_motor_positive_limit_hit()";

	MX_RECORD_FIELD_MOTOR *record_field_motor;
	MX_RECORD_FIELD_HANDLER *handler;
	long limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_record_field_motor_get_pointers( motor,
						&record_field_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'", fname, motor->record->name));

	/* If there is no positive_limit_hit handler, we declare that
	 * the limit is not tripped.
	 */

	handler = record_field_motor->positive_limit_hit_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL ) {
		motor->positive_limit_hit = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If there is a positive_limit_hit handler, process the specified
	 * record field to get the limit status.
	 */

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_GET );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_get_long( handler->record_field,
								&limit_hit );

	if ( limit_hit ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_record_field_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_record_field_motor_negative_limit_hit()";

	MX_RECORD_FIELD_MOTOR *record_field_motor;
	MX_RECORD_FIELD_HANDLER *handler;
	long limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_record_field_motor_get_pointers( motor,
						&record_field_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'", fname, motor->record->name));

	/* If there is no negative_limit_hit handler, we declare that
	 * the limit is not tripped.
	 */

	handler = record_field_motor->negative_limit_hit_handler;

	if ( handler == (MX_RECORD_FIELD_HANDLER *) NULL ) {
		motor->negative_limit_hit = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If there is a negative_limit_hit handler, process the specified
	 * record field to get the limit status.
	 */

	mx_status = mx_process_record_field( handler->record,
					handler->record_field, MX_PROCESS_GET );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_record_field_motor_get_long( handler->record_field,
								&limit_hit );

	if ( limit_hit ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return mx_status;
}


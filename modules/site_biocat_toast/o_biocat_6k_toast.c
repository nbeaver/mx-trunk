/*
 * Name:    o_biocat_6k_toast.c
 *
 * Purpose: MX operation driver that oscillates a Compumotor 6K-controlled
 *          motor back and forth until stopped.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXO_BIOCAT_6K_TOAST_DEBUG		FALSE

#define MXO_BIOCAT_6K_TOAST_DEBUG_PROGRAM	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_callback.h"
#include "mx_motor.h"
#include "mx_operation.h"
#include "d_compumotor.h"

#include "o_biocat_6k_toast.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxo_biocat_6k_toast_record_function_list = {
	NULL,
	mxo_biocat_6k_toast_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxo_biocat_6k_toast_open
};

MX_OPERATION_FUNCTION_LIST mxo_biocat_6k_toast_operation_function_list = {
	mxo_biocat_6k_toast_get_status,
	mxo_biocat_6k_toast_start,
	mxo_biocat_6k_toast_stop
};

MX_RECORD_FIELD_DEFAULTS mxo_biocat_6k_toast_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_OPERATION_STANDARD_FIELDS,
	MXO_BIOCAT_6K_TOAST_STANDARD_FIELDS
};

long mxo_biocat_6k_toast_num_record_fields
	= sizeof( mxo_biocat_6k_toast_record_field_defaults )
	/ sizeof( mxo_biocat_6k_toast_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxo_biocat_6k_toast_rfield_def_ptr
		= &mxo_biocat_6k_toast_record_field_defaults[0];

/*---*/

static mx_status_type
mxo_biocat_6k_toast_get_pointers( MX_OPERATION *operation,
			MX_BIOCAT_6K_TOAST **toast,
			MX_MOTOR **motor,
			MX_COMPUMOTOR **compumotor,
			MX_COMPUMOTOR_INTERFACE **compumotor_interface,
			const char *calling_fname )
{
	static const char fname[] =
			"mxo_biocat_6k_toast_get_pointers()";

	MX_BIOCAT_6K_TOAST *toast_ptr = NULL;
	MX_RECORD *motor_record = NULL;
	MX_COMPUMOTOR *compumotor_ptr = NULL;
	MX_RECORD *compumotor_interface_record = NULL;

	if ( operation == (MX_OPERATION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_OPERATION pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( operation->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_OPERATION %p is NULL.",
			operation );
	}

	toast_ptr = (MX_BIOCAT_6K_TOAST *)operation->record->record_type_struct;

	if ( toast_ptr == (MX_BIOCAT_6K_TOAST *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_BIOCAT_6K_TOAST pointer for record '%s' is NULL.",
			operation->record->name );
	}

	if ( toast != (MX_BIOCAT_6K_TOAST **) NULL ) {
		*toast = toast_ptr;
	}

	motor_record = toast_ptr->motor_record;

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The motor_record pointer for operation record '%s' is NULL.",
			operation->record->name );
	}

	if ( motor != (MX_MOTOR **) NULL ) {
		*motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( (*motor) == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MOTOR pointer for record '%s' "
			"used by operation '%s' is NULL.",
				motor_record->name,
				operation->record->name );
		}
	}


	compumotor_ptr = (MX_COMPUMOTOR *)motor_record->record_type_struct;

	if ( compumotor_ptr == (MX_COMPUMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_COMPUMOTOR pointer for record '%s' "
			"used by operation '%s' is NULL.",
				motor_record->name,
				operation->record->name );
	}

	if ( compumotor != (MX_COMPUMOTOR **) NULL ) {
		*compumotor = compumotor_ptr;
	}

	if ( compumotor_interface != (MX_COMPUMOTOR_INTERFACE **) NULL ) {

		compumotor_interface_record =
			compumotor_ptr->compumotor_interface_record;

		if ( compumotor_interface_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The compumotor_interface_record pointer for "
			"compumotor record '%s' is NULL.",
				motor_record->name );
		}

		*compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
			compumotor_interface_record->record_type_struct;

		if ((*compumotor_interface) == (MX_COMPUMOTOR_INTERFACE *) NULL)
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_COMPUMOTOR_INTERFACE pointer for record '%s' "
			"used by operation '%s' is NULL.",
				compumotor_interface_record->name,
				operation->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxo_biocat_6k_toast_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxo_biocat_6k_toast_create_record_structures()";

	MX_OPERATION *operation;
	MX_BIOCAT_6K_TOAST *toast = NULL;

	operation = (MX_OPERATION *) malloc( sizeof(MX_OPERATION) );

	if ( operation == (MX_OPERATION *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_OPERATION structure." );
	}

	toast = (MX_BIOCAT_6K_TOAST *) malloc( sizeof(MX_BIOCAT_6K_TOAST));

	if ( toast == (MX_BIOCAT_6K_TOAST *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_BIOCAT_6K_TOAST structure.");
	}

	record->record_superclass_struct = operation;
	record->record_class_struct = NULL;
	record->record_type_struct = toast;
	record->superclass_specific_function_list = 
			&mxo_biocat_6k_toast_operation_function_list;
	record->class_specific_function_list = NULL;

	operation->record = record;
	toast->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxo_biocat_6k_toast_open( MX_RECORD *record )
{
	static const char fname[] = "mxo_biocat_6k_toast_open()";

	MX_OPERATION *operation = NULL;
	MX_BIOCAT_6K_TOAST *toast = NULL;
	MX_MOTOR *motor = NULL;
	MX_COMPUMOTOR *compumotor = NULL;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	operation = (MX_OPERATION *) record->record_superclass_struct;

	mx_status = mxo_biocat_6k_toast_get_pointers( operation,
					&toast, &motor, &compumotor,
					&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( toast->task_number == 0 ) {
		toast->task_prefix[0] = '\0';
	} else
	if ( toast->task_number <= 10 ) {
		snprintf( toast->task_prefix, sizeof(toast->task_prefix),
			"%lu%%", toast->task_number );
	} else {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested task number %lu for record '%s' is outside the "
	"allowed range of 0 to 10.", toast->task_number, record->name );
	}

	toast->move_to_finish_in_progress = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_biocat_6k_toast_get_status( MX_OPERATION *operation )
{
	static const char fname[] = "mxo_biocat_6k_toast_get_status()";

	MX_BIOCAT_6K_TOAST *toast = NULL;
	MX_MOTOR *motor = NULL;
	MX_COMPUMOTOR *compumotor = NULL;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface = NULL;
	unsigned long motor_status;
	int program_status = 0;
	char command[20];
	char response[20];
	mx_status_type mx_status;

	mx_status = mxo_biocat_6k_toast_get_pointers( operation,
					&toast, &motor, &compumotor,
					&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	operation->status = 0;

	/* See if the motor has a fault. */

	mx_status = mx_motor_get_status( motor->record, &motor_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor_status & MXSF_MTR_ERROR ) {
		operation->status = MXSF_OP_FAULT;
	}

	program_status = 0;

	if ( operation->status != MXSF_OP_FAULT ) {

		if ( toast->move_to_finish_in_progress ) {

			/* If a move to the finish position is in progress,
			 * we check to see if the motor is still moving.
			 */

			if ( motor_status & MXSF_MTR_IS_BUSY ) {
				operation->status = MXSF_OP_BUSY;
			} else {
				operation->status = 0;

				toast->move_to_finish_in_progress = FALSE;
			}

		} else {
			/* Otherwise, we check to see if a program is running
			 * for this task.
			 */

			snprintf( command, sizeof(command),
				"!%sTSS.3", toast->task_prefix );

			mx_status = mxi_compumotor_command(
						compumotor_interface,
						command,
						response, sizeof(response),
						MXO_BIOCAT_6K_TOAST_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			program_status = atoi( response );

			if ( program_status == 0 ) {
				operation->status = 0;
			} else {
				operation->status = MXSF_OP_BUSY;
			}
		}
	}

#if MXO_BIOCAT_6K_TOAST_DEBUG
	MX_DEBUG(-2,("%s: motor '%s' status = %#lx, move to finish = %d, "
		"program status = %d, operation status = %#lx",
			fname, motor->record->name, motor->status,
			toast->move_to_finish_in_progress,
			program_status, operation->status));
#endif

	return mx_status;
}

static double
mxo_biocat_6k_toast_compute_6k_destination( MX_MOTOR *motor,
						MX_COMPUMOTOR *compumotor,
						double mx_destination )
{
#if MXO_BIOCAT_6K_TOAST_DEBUG
	static const char fname[] =
		"mxo_biocat_6k_toast_compute_6k_destination()";
#endif

	double c6k_destination;
	unsigned long flags;

	flags = compumotor->compumotor_flags;

	if ( ( flags & MXF_COMPUMOTOR_USE_ENCODER_POSITION )
	  && ( compumotor->is_servo == FALSE ) )
	{
		mx_destination = mx_destination * mx_divide_safely(
					compumotor->motor_resolution,
					compumotor->encoder_resolution );
	}

	if ( flags & MXF_COMPUMOTOR_ROUND_POSITION_OUTPUT_TO_NEAREST_INTEGER ) {
		mx_destination = (double) mx_round( mx_destination );
	}

	c6k_destination = mx_divide_safely( mx_destination - motor->offset,
						motor->scale );

#if MXO_BIOCAT_6K_TOAST_DEBUG
	MX_DEBUG(-2,("%s: mx_destination = %f, c6k_destination = %f",
		fname, mx_destination, c6k_destination));
#endif

	return c6k_destination;
}

MX_EXPORT mx_status_type
mxo_biocat_6k_toast_start( MX_OPERATION *operation )
{
	static const char fname[] = "mxo_biocat_6k_toast_start()";

	MX_BIOCAT_6K_TOAST *toast = NULL;
	MX_MOTOR *motor = NULL;
	MX_COMPUMOTOR *compumotor = NULL;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface = NULL;
	double low_6k_destination, high_6k_destination;
	char command[100];
	mx_status_type mx_status;

#if MXO_BIOCAT_6K_TOAST_DEBUG_PROGRAM
	char response[2000];
#endif

	mx_status = mxo_biocat_6k_toast_get_pointers( operation,
					&toast, &motor, &compumotor,
					&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	low_6k_destination = mxo_biocat_6k_toast_compute_6k_destination(
							motor,
							compumotor, 
							toast->low_position );

	high_6k_destination = mxo_biocat_6k_toast_compute_6k_destination(
							motor,
							compumotor, 
							toast->high_position );

	/* Turn off continuous command execution. */

	snprintf( command, sizeof(command), "%sCOMEXC0", toast->task_prefix );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Delete any preexisting program. */

	snprintf( command, sizeof(command), "DEL %s", toast->program_name );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/***** START OF PROGRAM *****/

	/* Start defining a motion program that implements the toast op. */

	snprintf( command, sizeof(command), "DEF %s", toast->program_name );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the infinite while loop. */

	snprintf( command, sizeof(command), "WHILE(%luAS=bX)",
					compumotor->axis_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the destination for the move to the high position. */

	snprintf( command, sizeof(command), "%ldD%f",
					compumotor->axis_number,
					high_6k_destination );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the move. */

	snprintf( command, sizeof(command), "%ldGO1", compumotor->axis_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the destination for the move to the low position. */

	snprintf( command, sizeof(command), "%ldD%f",
					compumotor->axis_number,
					low_6k_destination );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the move. */

	snprintf( command, sizeof(command), "%ldGO1", compumotor->axis_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Specify the end of the while loop. */

	mx_status = mxi_compumotor_command( compumotor_interface, "NWHILE",
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Specify the end of the program. */

	mx_status = mxi_compumotor_command( compumotor_interface, "END",
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/***** END OF PROGRAM *****/

	mx_msleep(100);

	mx_status = mx_rs232_discard_unread_input(
					compumotor_interface->rs232_record,
					MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXO_BIOCAT_6K_TOAST_DEBUG_PROGRAM

	/* Display the program that we just defined. */

	snprintf( command, sizeof(command), "TPROG %s", toast->program_name );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
						response, sizeof(response),
						MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	while (1) {
		unsigned long num_bytes_available;

		mx_msleep(200);

		mx_status = mx_rs232_num_input_bytes_available(
					compumotor_interface->rs232_record,
					&num_bytes_available );

		MX_DEBUG(-2,("%s: num_bytes_available = %lu",
			fname, num_bytes_available));

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_bytes_available == 0 ) {
			MX_DEBUG(-2,("%s: MARKER 0.0", fname));
			break;
		}

		mx_status = mx_rs232_getline(
				compumotor_interface->rs232_record,
				response, sizeof(response),
				NULL, MXO_BIOCAT_6K_TOAST_DEBUG );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;
		case MXE_TIMED_OUT:
			MX_DEBUG(-2,("%s: MARKER 0.1", fname));
			num_bytes_available = 0;
			break;
		default:
			return mx_status;
			break;
		}
	}

	MX_DEBUG(-2,("%s: MARKER 1", fname));

#endif /* MXO_BIOCAT_6K_TOAST_DEBUG_PROGRAM */

	/* Start the program. */

	snprintf( command, sizeof(command), "%s%s",
		toast->task_prefix, toast->program_name );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

#if 0
	MX_DEBUG(-2,("%s: MARKER 2", fname));
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	operation->status = MXSF_OP_BUSY;

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_biocat_6k_toast_stop( MX_OPERATION *operation )
{
	static const char fname[] = "mxo_biocat_6k_toast_stop()";

	MX_BIOCAT_6K_TOAST *toast = NULL;
	MX_MOTOR *motor = NULL;
	MX_COMPUMOTOR *compumotor = NULL;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface = NULL;
	char command[100];
	mx_status_type mx_status;

	mx_status = mxo_biocat_6k_toast_get_pointers( operation,
					&toast, &motor, &compumotor,
					&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXO_BIOCAT_6K_TOAST_DEBUG
	MX_DEBUG(-2,("%s: stopping motor '%s'",
		fname, toast->motor_record->name));
#endif

	/* Send a kill command. */

	snprintf( command, sizeof(command), "!%sK", toast->task_prefix );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn on continuous command execution. */

	snprintf( command, sizeof(command), "%sCOMEXC1", toast->task_prefix );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, command the motor to move to the finish position. */

	if ( toast->toast_flags & MXSF_TOAST_USE_FINISH_POSITION ) {

		toast->move_to_finish_in_progress = TRUE;

		mx_status = mx_motor_move_absolute( motor->record,
						toast->finish_position,
						MXF_MTR_NOWAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Wait a little while for the motor to start moving. */

		mx_msleep(100);
	} else {
		toast->move_to_finish_in_progress = FALSE;
	}

	return mx_status;
}


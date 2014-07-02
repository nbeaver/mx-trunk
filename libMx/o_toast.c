/*
 * Name:    o_toast.c
 *
 * Purpose: MX operation driver that oscillates a motor back and forth until
 *          stopped.  For some kinds of experiments, this can be thought of
 *          as "toasting" the sample in the X-ray beam.
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

#define MXO_TOAST_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_callback.h"
#include "mx_motor.h"
#include "mx_operation.h"

#include "o_toast.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxo_toast_record_function_list = {
	NULL,
	mxo_toast_create_record_structures
};

MX_OPERATION_FUNCTION_LIST mxo_toast_operation_function_list = {
	mxo_toast_get_status,
	mxo_toast_start,
	mxo_toast_stop
};

MX_RECORD_FIELD_DEFAULTS mxo_toast_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_OPERATION_STANDARD_FIELDS,
	MXO_TOAST_STANDARD_FIELDS
};

long mxo_toast_num_record_fields
	= sizeof( mxo_toast_record_field_defaults )
	/ sizeof( mxo_toast_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxo_toast_rfield_def_ptr
		= &mxo_toast_record_field_defaults[0];

/*---*/

static mx_status_type
mxo_toast_get_pointers( MX_OPERATION *operation,
			MX_TOAST **toast,
			MX_MOTOR **motor,
			const char *calling_fname )
{
	static const char fname[] =
			"mxo_toast_get_pointers()";

	MX_TOAST *toast_ptr;
	MX_RECORD *motor_record;

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

	toast_ptr = (MX_TOAST *) operation->record->record_type_struct;

	if ( toast_ptr == (MX_TOAST *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_TOAST pointer for record '%s' is NULL.",
			operation->record->name );
	}

	if ( toast != (MX_TOAST **) NULL ) {
		*toast = toast_ptr;
	}

	if ( motor != (MX_MOTOR **) NULL ) {
		motor_record = toast_ptr->motor_record;

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The motor_record pointer for "
			"operation record '%s' is NULL.",
				operation->record->name );
		}

		*motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( (*motor) == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MOTOR pointer for record '%s' "
			"used by operation '%s' is NULL.",
				motor_record->name,
				operation->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxo_toast_callback( MX_CALLBACK_MESSAGE *message )
{
	static const char fname[] = "mxo_toast_callback()";

	MX_RECORD *operation_record;
	MX_OPERATION *operation;
	MX_TOAST *toast;
	MX_CLOCK_TICK turnaround_delay_ticks, current_tick;
	int comparison;
	unsigned long motor_status;
	mx_status_type mx_status;

	if ( message == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"This callback was invoked with a NULL callback message!" );
	}

#if 0
	MX_DEBUG(-2,("%s: callback_message = %p, callback_type = %lu",
		fname, message, message->callback_type));
#endif

	if ( message->callback_type != MXCBT_FUNCTION ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Callback message %p sent to this callback function is not "
		"of type MXCBT_FUNCTION.  Instead, it is of type %lu",
			message, message->callback_type );
	}

	operation_record = (MX_RECORD *) message->u.function.callback_args;

	if ( operation_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for callback message %p is NULL.",
			message );
	}

	operation = (MX_OPERATION *) operation_record->record_superclass_struct;

	if ( operation == (MX_OPERATION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_OPERATION pointer for toast operation record '%s' "
		"is NULL.", operation_record->name );
	}

	toast = (MX_TOAST *) operation_record->record_type_struct;

	if ( toast == (MX_TOAST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TOAST pointer for toast operation record '%s' is NULL.",
			operation_record->name );
	}

	/* Restart the virtual timer to arrange for the next callback. */

	mx_status = mx_virtual_timer_start(
			message->u.function.oneshot_timer,
			message->u.function.callback_interval );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update toast operation state. */

#if MXO_TOAST_DEBUG
	MX_DEBUG(-2,("%s: BEFORE: state = %lu, next state = %lu",
		fname, toast->toast_state, toast->next_toast_state));
#endif

	switch( toast->toast_state ) {
	case MXST_TOAST_FAULT:
	case MXST_TOAST_TIMED_OUT:
		/* These are error states, so just return. */

		operation->status = MXSF_OP_FAULT;
		break;

	case MXST_TOAST_IDLE:
		switch( toast->next_toast_state ) {
		case MXST_TOAST_IDLE:
			/* Stay idle */

			operation->status = MXSF_OP_BUSY;
			break;

		case MXST_TOAST_MOVING_TO_HIGH:
			mx_status = mx_motor_move_absolute( toast->motor_record,
					toast->high_position, MXF_MTR_NOWAIT );

			if ( mx_status.code != MXE_SUCCESS ) {
				toast->toast_state = MXST_TOAST_FAULT;
				operation->status = MXSF_OP_FAULT;
				return mx_status;
			}

			toast->toast_state = toast->next_toast_state;
			toast->next_toast_state = MXST_TOAST_IN_TURNAROUND;
			operation->status = MXSF_OP_BUSY;
			break;

		case MXST_TOAST_MOVING_TO_LOW:
			mx_status = mx_motor_move_absolute( toast->motor_record,
					toast->low_position, MXF_MTR_NOWAIT );

			if ( mx_status.code != MXE_SUCCESS ) {
				toast->toast_state = MXST_TOAST_FAULT;
				operation->status = MXSF_OP_FAULT;
				return mx_status;
			}

			toast->toast_state = toast->next_toast_state;
			toast->next_toast_state = MXST_TOAST_IN_TURNAROUND;
			operation->status = MXSF_OP_BUSY;
			break;

		default:
			toast->toast_state = MXST_TOAST_FAULT;
			operation->status = MXSF_OP_FAULT;

			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected next_toast_state %lu for operation '%s' "
			"currently in the 'idle' state.",
				toast->next_toast_state,
				operation_record->name );
			break;
		}
		break;

	case MXST_TOAST_MOVING_TO_HIGH:
	case MXST_TOAST_MOVING_TO_LOW:
		/* Is the motor still moving? */

		mx_status = mx_motor_get_status( toast->motor_record,
							&motor_status );

		if ( mx_status.code != MXE_SUCCESS ) {
			toast->toast_state = MXST_TOAST_FAULT;
			operation->status = MXSF_OP_FAULT;
			return mx_status;
		}

		if ( motor_status & MXSF_MTR_IS_BUSY ) {
			/* The motor is still moving, so just return. */

			operation->status = MXSF_OP_BUSY;

			return MX_SUCCESSFUL_RESULT;
		}

		/* The motor has stopped moving, so begin the
		 * turnaround delay interval.
		 */

		turnaround_delay_ticks = mx_convert_seconds_to_clock_ticks(
						toast->turnaround_delay );

		toast->interval_finish_tick = mx_add_clock_ticks(
			turnaround_delay_ticks, mx_current_clock_tick() );

		if ( toast->toast_state == MXST_TOAST_MOVING_TO_HIGH ) {
			toast->next_toast_state = MXST_TOAST_MOVING_TO_LOW;
		} else {
			toast->next_toast_state = MXST_TOAST_MOVING_TO_HIGH;
		}

		toast->toast_state = MXST_TOAST_IN_TURNAROUND;
		operation->status = MXSF_OP_BUSY;
		break;

	case MXST_TOAST_IN_TURNAROUND:
		/* Has the turnaround delay completed? */

		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks( current_tick,
					toast->interval_finish_tick );

#if MXO_TOAST_DEBUG
		MX_DEBUG(-2,("%s: current tick = (%lu,%lu), "
			"finish tick = (%lu,%lu), comparison = %d",
			fname,
			current_tick.high_order,
			current_tick.low_order,
			toast->interval_finish_tick.high_order,
			toast->interval_finish_tick.low_order,
			comparison));
#endif

		if ( comparison < 0 ) {
			/* The turnaround delay is not finished yet so return.*/

			operation->status = MXSF_OP_BUSY;

			return MX_SUCCESSFUL_RESULT;
		}

		/* Send the motor to its new destination. */

		switch( toast->next_toast_state ) {
		case MXST_TOAST_MOVING_TO_HIGH:
			mx_status = mx_motor_move_absolute( toast->motor_record,
					toast->high_position, MXF_MTR_NOWAIT );

			if ( mx_status.code != MXE_SUCCESS ) {
				toast->toast_state = MXST_TOAST_FAULT;
				operation->status = MXSF_OP_FAULT;
				return mx_status;
			}

			toast->toast_state = toast->next_toast_state;
			toast->next_toast_state = MXST_TOAST_IN_TURNAROUND;
			operation->status = MXSF_OP_BUSY;
			break;

		case MXST_TOAST_MOVING_TO_LOW:
			mx_status = mx_motor_move_absolute( toast->motor_record,
					toast->low_position, MXF_MTR_NOWAIT );

			if ( mx_status.code != MXE_SUCCESS ) {
				toast->toast_state = MXST_TOAST_FAULT;
				operation->status = MXSF_OP_FAULT;
				return mx_status;
			}

			toast->toast_state = toast->next_toast_state;
			toast->next_toast_state = MXST_TOAST_IN_TURNAROUND;
			operation->status = MXSF_OP_BUSY;
			break;

		default:
			toast->toast_state = MXST_TOAST_FAULT;
			operation->status = MXSF_OP_FAULT;

			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected next_toast_state %lu for operation '%s' "
			"currently in the 'in turnaround' state.",
				toast->next_toast_state,
				operation_record->name );
		}
		break;

	case MXST_TOAST_STOPPING:
		/* Is the motor still moving? */

		mx_status = mx_motor_get_status( toast->motor_record,
							&motor_status );

		if ( mx_status.code != MXE_SUCCESS ) {
			toast->toast_state = MXST_TOAST_FAULT;
			operation->status = MXSF_OP_FAULT;
			return mx_status;
		}

		if ( (motor_status & MXSF_MTR_IS_BUSY) == 0 ) {
			/* The motor has stopped moving. */

			if ( toast->toast_flags
				& MXSF_TOAST_USE_FINISH_POSITION )
			{
				/* If requested, command a move
				 * to the finish position.
				 */

				toast->toast_state =
					MXST_TOAST_MOVING_TO_FINISH;
				toast->next_toast_state = MXST_TOAST_IDLE;
				operation->status = MXSF_OP_BUSY;

				mx_status = mx_motor_move_absolute(
						toast->motor_record,
						toast->finish_position,
						MXF_MTR_NOWAIT );

				if ( mx_status.code != MXE_SUCCESS ) {
					toast->toast_state = MXST_TOAST_FAULT;
					operation->status = MXSF_OP_FAULT;
					return mx_status;
				}
			} else {
				/* Otherwise, delete the toast callback. */

				toast->toast_state = MXST_TOAST_IDLE;
				toast->next_toast_state = MXST_TOAST_IDLE;
				operation->status = 0;

				if ( toast->callback_message
					!= (MX_CALLBACK_MESSAGE *) NULL )
				{
					mx_status = mx_function_delete_callback(
						toast->callback_message );

					if ( mx_status.code != MXE_SUCCESS ) {
					    operation->status = MXSF_OP_FAULT;
					    return mx_status;
					}
	
					toast->callback_message = NULL;
				}
			}
		}
		break;

	case MXST_TOAST_MOVING_TO_FINISH:
		/* Is the motor still moving? */

		mx_status = mx_motor_get_status( toast->motor_record,
							&motor_status );

		if ( mx_status.code != MXE_SUCCESS ) {
			toast->toast_state = MXST_TOAST_FAULT;
			operation->status = MXSF_OP_FAULT;
			return mx_status;
		}

		if ( (motor_status & MXSF_MTR_IS_BUSY) == 0 ) {
			/* The motor has stopped moving. */

			/* Delete the toast callback. */

			toast->toast_state = MXST_TOAST_IDLE;
			toast->next_toast_state = MXST_TOAST_IDLE;
			operation->status = 0;

			if ( toast->callback_message
					!= (MX_CALLBACK_MESSAGE *) NULL )
			{
				mx_status = mx_function_delete_callback(
					toast->callback_message );

				if ( mx_status.code != MXE_SUCCESS ) {
					operation->status = MXSF_OP_FAULT;
					return mx_status;
				}
	
				toast->callback_message = NULL;
			}
		}
		break;

	default:
		toast->toast_state = MXST_TOAST_FAULT;
		operation->status = MXSF_OP_FAULT;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized toast state %lu for operation '%s'.",
			toast->toast_state, operation_record->name );
		break;
	}

#if MXO_TOAST_DEBUG
	MX_DEBUG(-2,("%s: AFTER: state = %lu, next state = %lu",
		fname, toast->toast_state, toast->next_toast_state));
#endif

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxo_toast_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxo_toast_create_record_structures()";

	MX_OPERATION *operation;
	MX_TOAST *toast = NULL;

	operation = (MX_OPERATION *) malloc( sizeof(MX_OPERATION) );

	if ( operation == (MX_OPERATION *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_OPERATION structure." );
	}

	toast = (MX_TOAST *) malloc( sizeof(MX_TOAST));

	if ( toast == (MX_TOAST *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_TOAST structure.");
	}

	record->record_superclass_struct = operation;
	record->record_class_struct = NULL;
	record->record_type_struct = toast;
	record->superclass_specific_function_list = 
			&mxo_toast_operation_function_list;
	record->class_specific_function_list = NULL;

	operation->record = record;
	toast->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxo_toast_get_status( MX_OPERATION *operation )
{
	static const char fname[] = "mxo_toast_get_status()";

	MX_TOAST *toast = NULL;
	MX_MOTOR *motor = NULL;
	mx_status_type mx_status;

	mx_status = mxo_toast_get_pointers( operation, &toast, &motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	/* It is better to set this in the toast callback above. */

	switch( toast->toast_state ) {
	case MXST_TOAST_IDLE:
		operation->status = 0;
		break;

	case MXST_TOAST_FAULT:
	case MXST_TOAST_TIMED_OUT:
		operation->status = MXSF_OP_FAULT;
		break;

	case MXST_TOAST_MOVING_TO_HIGH:
	case MXST_TOAST_MOVING_TO_LOW:
	case MXST_TOAST_IN_TURNAROUND:
	case MXST_TOAST_STOPPING:
	default:
		operation->status = MXSF_OP_BUSY;
		break;

	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_toast_start( MX_OPERATION *operation )
{
	static const char fname[] = "mxo_toast_start()";

	MX_TOAST *toast = NULL;
	MX_MOTOR *motor = NULL;
	mx_status_type mx_status;

	mx_status = mxo_toast_get_pointers( operation, &toast, &motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( toast->callback_message != (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"Operation '%s' is already running.",
			operation->record->name );
	}

	/* Start the toast callback with a callback interval of 0.1 seconds. */

	toast->toast_state = MXST_TOAST_IDLE;

	toast->next_toast_state = MXST_TOAST_MOVING_TO_HIGH;

	operation->status = MXSF_OP_BUSY;

	mx_status = mx_function_add_callback( operation->record,
						mxo_toast_callback,
						NULL,
						operation->record,
						0.1,
						&(toast->callback_message) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_toast_stop( MX_OPERATION *operation )
{
	static const char fname[] = "mxo_toast_stop()";

	MX_TOAST *toast = NULL;
	MX_MOTOR *motor = NULL;
	mx_status_type mx_status;

	mx_status = mxo_toast_get_pointers( operation, &toast, &motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Unconditionally stop the motor and set the current state to IDLE. */

	toast->toast_state = MXST_TOAST_STOPPING;

	toast->next_toast_state = MXST_TOAST_IDLE;

	mx_status = mx_motor_soft_abort( toast->motor_record );

	if ( mx_status.code != MXE_SUCCESS ) {
		toast->toast_state = MXST_TOAST_FAULT;

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}


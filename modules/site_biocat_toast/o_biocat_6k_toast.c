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
 * Copyright 2014-2015 Illinois Institute of Technology
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
#include "d_linear_function.h"

#include "o_biocat_6k_toast.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxo_biocat_6k_toast_record_function_list = {
	NULL,
	mxo_biocat_6k_toast_create_record_structures,
	mxo_biocat_6k_toast_finish_record_initialization,
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
			const char *calling_fname )
{
	static const char fname[] =
			"mxo_biocat_6k_toast_get_pointers()";

	MX_BIOCAT_6K_TOAST *toast_ptr = NULL;

	if ( operation == (MX_OPERATION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_OPERATION pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( toast == (MX_BIOCAT_6K_TOAST **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BIOCAT_6K_TOAST pointer passed was NULL." );
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

	*toast = toast_ptr;

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
mxo_biocat_6k_toast_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxo_biocat_6k_toast_finish_record_initialization()";

	MX_BIOCAT_6K_TOAST *toast = NULL;
	MX_RECORD *compumotor_record = NULL;
	MX_RECORD *compumotor_interface_record = NULL;
	const char *motor_driver_name = NULL;
	const char *compumotor_driver_name = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	toast = (MX_BIOCAT_6K_TOAST *) record->record_type_struct;

	if ( toast == (MX_BIOCAT_6K_TOAST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BIOCAT_6K_TOAST pointer for record '%s' is NULL.",
			record->name );
	}

	if ( toast->motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The motor_record pointer for toast record '%s' is NULL.",
			record->name );
	}

	/* We need to set up some pointers in the MX_BIOCAT_6K_TOAST structure
	 * that we will use later over and over.
	 */

	motor_driver_name = mx_get_driver_name( toast->motor_record );

	/* If the motor record uses the 'compumotor' driver, then things
	 * are relatively simple.
	 */

	if ( strcmp( motor_driver_name, "compumotor" ) == 0 ) {
		toast->linear_function_motor = NULL;
		toast->motor_of_linear_function = NULL;

		toast->compumotor = (MX_COMPUMOTOR *)
				toast->motor_record->record_type_struct;

		if ( toast->compumotor == (MX_COMPUMOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_COMPUMOTOR pointer of record '%s' "
			"used by toast record '%s' is NULL.",
				toast->motor_record->name,
				record->name );
		}

		toast->motor_of_compumotor = (MX_MOTOR *)
				toast->motor_record->record_class_struct;

		if ( toast->motor_of_compumotor == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MOTOR pointer of record '%s' "
			"used by toast record '%s' is NULL.",
				toast->motor_record->name,
				record->name );
		}

		/* We also need a pointer to MX_COMPUMOTOR_INTERFACE object,
		 * since we will use that for mxi_compumotor_command().
		 */

		compumotor_interface_record =
			toast->compumotor->compumotor_interface_record;

		if ( compumotor_interface_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The 'compumotor_interface_record' pointer for "
			"motor '%s' used by toast record '%s' is NULL.",
				toast->compumotor->record->name,
				record->name );
		}

		toast->compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
			compumotor_interface_record->record_type_struct;

		if ( toast->compumotor_interface ==
				(MX_COMPUMOTOR_INTERFACE *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_COMPUMOTOR_INTERFACE pointer for interface "
			"record '%s' of motor '%s' used by toast record '%s' "
			"is NULL.",
				compumotor_interface_record->name,
				toast->compumotor->record->name,
				record->name );
		}

		MX_DEBUG(-2,
		("%s: compumotor = '%s', compumotor_interface = '%s'",
			fname, toast->compumotor->record->name,
			toast->compumotor_interface->record->name));

		return MX_SUCCESSFUL_RESULT;
	}

	/* The only other allowed driver is 'linear_function'. */

	if ( strcmp( motor_driver_name, "linear_function" ) != 0 ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The motor record '%s' used by toast record '%s' "
		"has an MX driver of type '%s' which is not allowed.  "
		"The only supported motor drivers for this toast record "
		"are of type 'compumotor' and 'linear_function'.",
			toast->motor_record->name,
			record->name,
			motor_driver_name );
	}

	/* If we get here, the motor record uses a 'linear_function' driver. */

	toast->linear_function_motor = (MX_LINEAR_FUNCTION_MOTOR *)
		toast->motor_record->record_type_struct;

	if ( toast->linear_function_motor == (MX_LINEAR_FUNCTION_MOTOR *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LINEAR_FUNCTION_MOTOR pointer of record '%s' "
		"used by toast record '%s' is NULL.",
			toast->motor_record->name,
			record->name );
	}

	toast->motor_of_linear_function = (MX_MOTOR *)
		toast->motor_record->record_class_struct;

	if ( toast->motor_of_linear_function == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer of record '%s' used by record '%s' "
		"is NULL.", toast->motor_record->name, record->name );
	}

	/* The linear_function motor should depend on only one motor record
	 * and no variable records.
	 */

	if ( (toast->linear_function_motor->num_motors != 1 )
	  || (toast->linear_function_motor->num_variables != 0) )
	{
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The 'linear_function' pseudomotor '%s' used by "
		"toast record '%s' should only depend on one real motor record "
		"and zero variable records.  Instead, it depends on "
		"%ld motor records and %ld variable records.",
			toast->motor_record->name,
			record->name,
			toast->linear_function_motor->num_motors,
			toast->linear_function_motor->num_variables );
	}

	if ( toast->linear_function_motor->motor_record_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The motor_record_array pointer for 'linear_function' "
		"pseudomotor '%s' used by toast record '%s' is NULL.",
			toast->motor_record->name, record->name );
	}

	/* We now extract information about the underlying Compumotor record
	 * from the linear_function record.
	 */

	compumotor_record = toast->linear_function_motor->motor_record_array[0];

	if ( compumotor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Compumotor record pointer used by 'linear_function' "
		"record '%s' of toast record '%s' is NULL.",
			toast->motor_record->name, record->name );
	}

	compumotor_driver_name = mx_get_driver_name( compumotor_record );

	if ( strcmp( compumotor_driver_name, "compumotor" ) != 0 ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The motor record '%s' of 'linear_function' record '%s' "
		"used by toast record '%s' does not use a 'compumotor' "
		"driver.  Instead, it uses a driver of type '%s'",
			compumotor_record->name,
			toast->motor_record->name,
			record->name,
			compumotor_driver_name );
	}

	toast->compumotor = (MX_COMPUMOTOR *)
			compumotor_record->record_type_struct;

	if ( toast->compumotor == (MX_COMPUMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_COMPUMOTOR pointer of motor record '%s' of "
		"'linear_function' record '%s' used by toast record '%s' "
		"is NULL.", compumotor_record->name,
			toast->motor_record->name,
			record->name );
	}

	toast->motor_of_compumotor = (MX_MOTOR *)
			compumotor_record->record_class_struct;

	if ( toast->motor_of_compumotor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer of motor record '%s' of "
		"'linear_function' record '%s' used by toast record '%s' "
		"is NULL.", compumotor_record->name,
			toast->motor_record->name,
			record->name );
	}

	/* We also need a pointer to MX_COMPUMOTOR_INTERFACE object, since
	 * we will use that for mxi_compumotor_command().
	 */

	compumotor_interface_record =
		toast->compumotor->compumotor_interface_record;

	if ( compumotor_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'compumotor_interface_record' pointer for motor '%s' "
		"used by toast record '%s' is NULL.",
			toast->compumotor->record->name,
			record->name );
	}

	toast->compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
		compumotor_interface_record->record_type_struct;

	if ( toast->compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_COMPUMOTOR_INTERFACE pointer for interface record '%s' "
		"of motor '%s' used by toast record '%s' is NULL.",
			compumotor_interface_record->name,
			toast->compumotor->record->name,
			record->name );
	}

	/*---*/

	MX_DEBUG(-2,("%s: linear_function = '%s', compumotor = '%s', "
	    "compumotor_interface = '%s'",
		fname, toast->linear_function_motor->record->name,
		toast->compumotor->record->name,
		toast->compumotor_interface->record->name ));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxo_biocat_6k_toast_open( MX_RECORD *record )
{
	static const char fname[] = "mxo_biocat_6k_toast_open()";

	MX_OPERATION *operation = NULL;
	MX_BIOCAT_6K_TOAST *toast = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	operation = (MX_OPERATION *) record->record_superclass_struct;

	mx_status = mxo_biocat_6k_toast_get_pointers(operation, &toast, fname);

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
	unsigned long motor_status;
	int program_status = 0;
	char command[20];
	char response[20];
	mx_status_type mx_status;

	mx_status = mxo_biocat_6k_toast_get_pointers(operation, &toast, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	operation->status = 0;

	/* See if the motor has a fault. */

	mx_status = mx_motor_get_status( toast->compumotor->record,
						&motor_status );

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
						toast->compumotor_interface,
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

/* mxo_biocat_6k_toast_compute_6k_destination() converts database values
 * like 'high_position' and 'low_position' into raw Compumotor 6K positions
 * in controller units.
 */

static double
mxo_biocat_6k_toast_compute_6k_destination( MX_BIOCAT_6K_TOAST *toast,
						double mx_destination )
{
#if MXO_BIOCAT_6K_TOAST_DEBUG
	static const char fname[] =
		"mxo_biocat_6k_toast_compute_6k_destination()";
#endif

	MX_LINEAR_FUNCTION_MOTOR *lf;
	MX_COMPUMOTOR *cr;
	MX_MOTOR *motor_lf;
	MX_MOTOR *motor_cr;
	double lf_destination;
	double lf_raw_destination;
	double cr_destination;
	double c6k_destination;
	unsigned long flags;

	lf = toast->linear_function_motor;
	cr = toast->compumotor;

	motor_lf = toast->motor_of_linear_function;
	motor_cr = toast->motor_of_compumotor;

	/* If toast->motor_record is a 'linear_function' record, we must
	 * convert the mx_destination from linear function record units
	 * into Compumotor motor record units.
	 */

	if ( lf == (MX_LINEAR_FUNCTION_MOTOR *) NULL ) {
		cr_destination = mx_destination;
	} else {
		lf_destination = mx_destination;

		lf_raw_destination =
			mx_divide_safely( lf_destination - (motor_lf->offset),
						(motor_lf->scale) );

		cr_destination = mx_divide_safely(
			lf_raw_destination - (lf->real_motor_offset[0]),
				lf->real_motor_scale[0] );
	}

	/* Now we work with the real Compumotor motor record units. */

	flags = toast->compumotor->compumotor_flags;

	if ( ( flags & MXF_COMPUMOTOR_USE_ENCODER_POSITION )
	  && ( toast->compumotor->is_servo == FALSE ) )
	{
		cr_destination = cr_destination * mx_divide_safely(
					cr->motor_resolution,
					cr->encoder_resolution );
	}

	if ( flags & MXF_COMPUMOTOR_ROUND_POSITION_OUTPUT_TO_NEAREST_INTEGER ) {
		cr_destination = (double) mx_round( cr_destination );
	}

	c6k_destination = mx_divide_safely( cr_destination - (motor_cr->offset),
						(motor_cr->scale) );

#if MXO_BIOCAT_6K_TOAST_DEBUG
	MX_DEBUG(-2,("%s: mx_destination = %f, c6k_destination = %f",
		fname, mx_destination, c6k_destination));
#endif

	return c6k_destination;
}

#define IDLE (-1)
#define END   (0)
#define START (1)

static mx_status_type
mxo_biocat_6k_toast_signal( MX_BIOCAT_6K_TOAST *toast, int new_state )
{
	char command[40];
	unsigned long flags;
	mx_bool_type use_gate, use_trigger;
	mx_status_type mx_status;

	flags = toast->toast_flags;

	use_gate = FALSE;
	use_trigger = FALSE;

	if ( flags & MXSF_TOAST_GENERATE_GATE_SIGNAL ) {
		use_gate = TRUE;

		use_gate = use_gate;	/* Suppress set but not used error. */
	} else
	if ( flags & MXSF_TOAST_GENERATE_TRIGGER_SIGNAL ) {
		use_trigger = TRUE;
	} else {
		return MX_SUCCESSFUL_RESULT;
	}

	if ( new_state == IDLE ) {
		snprintf(command, sizeof(command), "%s-0", toast->output_name);

		mx_status = mxi_compumotor_command( toast->compumotor_interface,
						command, NULL, 0,
						MXO_BIOCAT_6K_TOAST_DEBUG );
	} else
	if ( use_trigger ) {
		/* Trigger start and trigger end do the same thing. */

		/* Raise the trigger high. */

		snprintf(command, sizeof(command), "%s-1", toast->output_name);

		mx_status = mxi_compumotor_command( toast->compumotor_interface,
						command, NULL, 0,
						MXO_BIOCAT_6K_TOAST_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Wait for the trigger delay. */

		snprintf(command, sizeof(command), "T%f", toast->trigger_width);

		mx_status = mxi_compumotor_command( toast->compumotor_interface,
						command, NULL, 0,
						MXO_BIOCAT_6K_TOAST_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Lower the trigger low. */

		snprintf(command, sizeof(command), "%s-1", toast->output_name);

		mx_status = mxi_compumotor_command( toast->compumotor_interface,
						command, NULL, 0,
						MXO_BIOCAT_6K_TOAST_DEBUG );
	} else
	if ( new_state == END ) {
		/* End of the gate signal */

		snprintf(command, sizeof(command), "%s-0", toast->output_name);

		mx_status = mxi_compumotor_command( toast->compumotor_interface,
						command, NULL, 0,
						MXO_BIOCAT_6K_TOAST_DEBUG );
	} else
	if ( new_state == START ) {
		/* Start of the gate signal */

		snprintf(command, sizeof(command), "%s-1", toast->output_name);

		mx_status = mxi_compumotor_command( toast->compumotor_interface,
						command, NULL, 0,
						MXO_BIOCAT_6K_TOAST_DEBUG );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_biocat_6k_toast_start( MX_OPERATION *operation )
{
	static const char fname[] = "mxo_biocat_6k_toast_start()";

	MX_BIOCAT_6K_TOAST *toast = NULL;
	double low_6k_destination, high_6k_destination;
	char command[100];
	mx_bool_type use_toast_signal;
	mx_status_type mx_status;

#if MXO_BIOCAT_6K_TOAST_DEBUG_PROGRAM
	char response[2000];
#endif

	mx_status = mxo_biocat_6k_toast_get_pointers(operation, &toast, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strlen(toast->output_name) > 0 ) {
		use_toast_signal = TRUE;
	} else {
		use_toast_signal = FALSE;
	}

	/* Set the trigger signal to idle. */

	if ( use_toast_signal ) {
		mx_status = mxo_biocat_6k_toast_signal( toast, IDLE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Compute the destinations. */

	low_6k_destination = mxo_biocat_6k_toast_compute_6k_destination(
							toast,
							toast->low_position );

	high_6k_destination = mxo_biocat_6k_toast_compute_6k_destination(
							toast,
							toast->high_position );

	/* Turn off continuous command execution. */

	snprintf( command, sizeof(command), "%sCOMEXC0", toast->task_prefix );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Delete any preexisting program. */

	snprintf( command, sizeof(command), "DEL %s", toast->program_name );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/***** START OF PROGRAM *****/

	/* Start defining a motion program that implements the toast op. */

	snprintf( command, sizeof(command), "DEF %s", toast->program_name );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Define a label for the start of the loop. */

	snprintf( command, sizeof(command), "$%s", toast->variable_name );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/**** Set the destination for the move to the high position. ****/

	snprintf( command, sizeof(command), "%ldD%f",
					toast->compumotor->axis_number,
					high_6k_destination );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Signal the start of the move. */

	if ( use_toast_signal ) {
		mx_status = mxo_biocat_6k_toast_signal( toast, START );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Start the move. */

	snprintf( command, sizeof(command),
		"%ldGO1", toast->compumotor->axis_number );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Signal the end of the move. */

	if ( use_toast_signal ) {
		mx_status = mxo_biocat_6k_toast_signal( toast, END );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/**** Set the destination for the move to the low position. ****/

	snprintf( command, sizeof(command), "%ldD%f",
					toast->compumotor->axis_number,
					low_6k_destination );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Signal the start of the move. */

	if ( use_toast_signal ) {
		mx_status = mxo_biocat_6k_toast_signal( toast, START );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Start the move. */

	snprintf( command, sizeof(command),
		"%ldGO1", toast->compumotor->axis_number );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Signal the end of the move. */

	if ( use_toast_signal ) {
		mx_status = mxo_biocat_6k_toast_signal( toast, END );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Jump back to the start of the loop. */

	snprintf( command, sizeof(command), "GOTO %s", toast->variable_name );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Specify the end of the program. */

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				"END", NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/***** END OF PROGRAM *****/

	mx_msleep(100);

	mx_status = mx_rs232_discard_unread_input(
				toast->compumotor_interface->rs232_record,
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

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

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
	char command[100];
	mx_status_type mx_status;

	mx_status = mxo_biocat_6k_toast_get_pointers(operation, &toast, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXO_BIOCAT_6K_TOAST_DEBUG
	MX_DEBUG(-2,("%s: stopping motor '%s'",
		fname, toast->motor_record->name));
#endif

	/* Send a kill command. */

	snprintf( command, sizeof(command), "!%sK", toast->task_prefix );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn on continuous command execution. */

	snprintf( command, sizeof(command), "%sCOMEXC1", toast->task_prefix );

	mx_status = mxi_compumotor_command( toast->compumotor_interface,
				command, NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, command the motor to move to the finish position. */

	if ( toast->toast_flags & MXSF_TOAST_USE_FINISH_POSITION ) {

		toast->move_to_finish_in_progress = TRUE;

		mx_status = mx_motor_move_absolute( toast->compumotor->record,
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


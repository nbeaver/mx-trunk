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
	mxo_biocat_6k_toast_open,
	mxo_biocat_6k_toast_close
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

	compumotor_interface_record =
			compumotor_ptr->compumotor_interface_record;

	if ( compumotor_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The compumotor_interface_record pointer for compumotor "
		"record '%s' is NULL.", motor_record->name );
	}

	if ( compumotor_interface == (MX_COMPUMOTOR_INTERFACE **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_COMPUMOTOR_INTERFACE pointer for interface record '%s' "
		"used by record '%s' is NULL.",
			compumotor_interface_record->name,
			motor_record->name );
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

	/* Send a stop command, just in case the operation was started
	 * by a previous instance of MX.
	 */

	mx_status = mxo_biocat_6k_toast_stop( operation );

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_biocat_6k_toast_close( MX_RECORD *record )
{
	static const char fname[] = "mxo_biocat_6k_toast_close()";

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
	mx_status_type mx_status;

	mx_status = mxo_biocat_6k_toast_get_pointers( operation,
					&toast, &motor, &compumotor,
					&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_status( motor->record, &motor_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor_status & MXSF_MTR_ERROR ) {
		operation->status = MXSF_OP_FAULT;
	} else
	if ( motor_status & MXSF_MTR_IS_BUSY ) {
		operation->status = MXSF_OP_BUSY;
	} else {
		operation->status = 0;
	}

	return mx_status;
}

static double
mxo_biocat_6k_toast_compute_6k_destination( MX_COMPUMOTOR *compumotor,
						double mx_destination )
{
	double raw_destination;
	unsigned long flags;

	flags = compumotor->compumotor_flags;

	raw_destination = mx_destination;

	if ( ( flags & MXF_COMPUMOTOR_USE_ENCODER_POSITION )
	  && ( compumotor->is_servo == FALSE ) )
	{
		raw_destination = raw_destination * mx_divide_safely(
					compumotor->motor_resolution,
					compumotor->encoder_resolution );
	}

	if ( flags & MXF_COMPUMOTOR_ROUND_POSITION_OUTPUT_TO_NEAREST_INTEGER ) {
		raw_destination = (double) mx_round( raw_destination );
	}

	return raw_destination;
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
	char command[1000];
	char go_cmd[100];
	size_t i;
	mx_status_type mx_status;

	mx_status = mxo_biocat_6k_toast_get_pointers( operation,
					&toast, &motor, &compumotor,
					&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	low_6k_destination = mxo_biocat_6k_toast_compute_6k_destination(
							compumotor, 
							toast->low_position );

	high_6k_destination = mxo_biocat_6k_toast_compute_6k_destination(
							compumotor, 
							toast->high_position );

	/* Construct a GO command string that will start just this axis. */

	strlcpy( go_cmd, "GO", sizeof(go_cmd) );

	for ( i = 0; i < MX_MAX_COMPUMOTOR_AXES; i++ ) {

		if ( (i+1) == compumotor->axis_number ) {
			strlcat( go_cmd, "1", sizeof(go_cmd) );
		} else {
			strlcat( go_cmd, "X", sizeof(go_cmd) );
		}
	}

	/* Construct the command to send to the controller. */

	snprintf( command, sizeof(command),
		"%ld_!WHILE(%ldAS=bX):%ldD%f:%s:%ldD%f:%s:NWHILE",
			compumotor->controller_number,
			compumotor->axis_number,
			compumotor->axis_number,
			high_6k_destination, go_cmd,
			compumotor->axis_number,
			low_6k_destination, go_cmd );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, MXO_BIOCAT_6K_TOAST_DEBUG );

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
	mx_status_type mx_status;

	mx_status = mxo_biocat_6k_toast_get_pointers( operation,
					&toast, &motor, &compumotor,
					&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( toast->motor_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}


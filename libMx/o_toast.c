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

#define MXO_TOAST_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_motor.h"
#include "mx_operation.h"

#include "o_toast.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxo_toast_record_function_list = {
	NULL,
	mxo_toast_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxo_toast_open,
	mxo_toast_close
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
mxo_toast_open( MX_RECORD *record )
{
	static const char fname[] = "mxo_toast_open()";

	MX_OPERATION *operation = NULL;
	MX_TOAST *toast = NULL;
	MX_MOTOR *motor = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	operation = (MX_OPERATION *) record->record_superclass_struct;

	mx_status = mxo_toast_get_pointers( operation, &toast, &motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_toast_close( MX_RECORD *record )
{
	static const char fname[] = "mxo_toast_close()";

	MX_OPERATION *operation = NULL;
	MX_TOAST *toast = NULL;
	MX_MOTOR *motor = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	operation = (MX_OPERATION *) record->record_superclass_struct;

	mx_status = mxo_toast_get_pointers( operation, &toast, &motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
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

	return mx_status;
}


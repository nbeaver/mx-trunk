/*
 * Name:    o_network.c
 *
 * Purpose: MX operation driver for operations controlled by a remote MX server.
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

#define MXO_NETWORK_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_operation.h"

#include "o_network.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxo_network_record_function_list = {
	NULL,
	mxo_network_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxo_network_open,
	mxo_network_close
};

MX_OPERATION_FUNCTION_LIST mxo_network_operation_function_list = {
	mxo_network_get_status,
	mxo_network_start,
	mxo_network_stop
};

MX_RECORD_FIELD_DEFAULTS mxo_network_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_OPERATION_STANDARD_FIELDS,
	MXO_NETWORK_STANDARD_FIELDS
};

long mxo_network_num_record_fields
	= sizeof( mxo_network_record_field_defaults )
	/ sizeof( mxo_network_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxo_network_rfield_def_ptr
		= &mxo_network_record_field_defaults[0];

/*---*/

static mx_status_type
mxo_network_get_pointers( MX_OPERATION *operation,
			MX_NETWORK_OPERATION **network_op,
			const char *calling_fname )
{
	static const char fname[] =
			"mxo_network_get_pointers()";

	MX_NETWORK_OPERATION *network_op_ptr;

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

	network_op_ptr = (MX_NETWORK_OPERATION *)
			operation->record->record_type_struct;

	if ( network_op_ptr == (MX_NETWORK_OPERATION *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_NETWORK_OPERATION pointer for record '%s' is NULL.",
			operation->record->name );
	}

	if ( network_op != (MX_NETWORK_OPERATION **) NULL ) {
		*network_op = network_op_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxo_network_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxo_network_create_record_structures()";

	MX_OPERATION *operation;
	MX_NETWORK_OPERATION *network_op = NULL;

	operation = (MX_OPERATION *) malloc( sizeof(MX_OPERATION) );

	if ( operation == (MX_OPERATION *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_OPERATION structure." );
	}

	network_op = (MX_NETWORK_OPERATION *)
				malloc( sizeof(MX_NETWORK_OPERATION));

	if ( network_op == (MX_NETWORK_OPERATION *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for an MX_NETWORK_OPERATION structure.");
	}

	record->record_superclass_struct = operation;
	record->record_class_struct = NULL;
	record->record_type_struct = network_op;
	record->superclass_specific_function_list = 
			&mxo_network_operation_function_list;
	record->class_specific_function_list = NULL;

	operation->record = record;
	network_op->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxo_network_open( MX_RECORD *record )
{
	static const char fname[] = "mxo_network_open()";

	MX_OPERATION *operation = NULL;
	MX_NETWORK_OPERATION *network_op = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	operation = (MX_OPERATION *) record->record_superclass_struct;

	mx_status = mxo_network_get_pointers( operation, &network_op, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_network_close( MX_RECORD *record )
{
	static const char fname[] = "mxo_network_close()";

	MX_OPERATION *operation = NULL;
	MX_NETWORK_OPERATION *network_op = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	operation = (MX_OPERATION *) record->record_superclass_struct;

	mx_status = mxo_network_get_pointers( operation, &network_op, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_network_get_status( MX_OPERATION *operation )
{
	static const char fname[] = "mxo_network_get_status()";

	MX_NETWORK_OPERATION *network_op = NULL;
	mx_status_type mx_status;

	mx_status = mxo_network_get_pointers( operation, &network_op, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_network_start( MX_OPERATION *operation )
{
	static const char fname[] = "mxo_network_start()";

	MX_NETWORK_OPERATION *network_op = NULL;
	mx_status_type mx_status;

	mx_status = mxo_network_get_pointers( operation, &network_op, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxo_network_stop( MX_OPERATION *operation )
{
	static const char fname[] = "mxo_network_stop()";

	MX_NETWORK_OPERATION *network_op = NULL;
	mx_status_type mx_status;

	mx_status = mxo_network_get_pointers( operation, &network_op, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}


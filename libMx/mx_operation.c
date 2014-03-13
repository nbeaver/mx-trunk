/*
 * Name:    mx_operation.c
 *
 * Purpose: Operation support for the MX library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"

#include "mx_operation.h"

static mx_status_type
mx_operation_get_pointers( MX_RECORD *op_record,
			MX_OPERATION **operation,
			MX_OPERATION_FUNCTION_LIST **flist,
			const char *calling_fname )
{
	static const char fname[] = "mx_operation_get_pointers()";

	MX_OPERATION *operation_ptr;

	if ( op_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	operation_ptr = (MX_OPERATION *) op_record->record_superclass_struct;

	if ( operation_ptr == (MX_OPERATION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_OPERATION pointer for record '%s' is NULL.",
			op_record->name );
	}

	if ( operation != (MX_OPERATION **) NULL ) {
		*operation = operation_ptr;
	}

	if ( flist != (MX_OPERATION_FUNCTION_LIST **) NULL ) {
		*flist = (MX_OPERATION_FUNCTION_LIST *)
				op_record->superclass_specific_function_list;

		if ( (*flist) == (MX_OPERATION_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_OPERATION_FUNCTION_LIST for record '%s' is NULL.",
				op_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_operation_get_status( MX_RECORD *op_record,
			unsigned long *op_status )
{
	static const char fname[] = "mx_operation_get_status()";

	MX_OPERATION *operation;
	MX_OPERATION_FUNCTION_LIST *flist;
	mx_status_type ( *get_status_fn )( MX_OPERATION * );
	mx_status_type mx_status;

	mx_status = mx_operation_get_pointers( op_record,
					&operation, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_status_fn = flist->get_status;

	if ( get_status_fn == NULL ) {
		operation->status = 0;

		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = (*get_status_fn)( operation );
	}

	if ( op_status != (unsigned long *) NULL ) {
		*op_status = operation->status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_operation_start( MX_RECORD *op_record )
{
	static const char fname[] = "mx_operation_start()";

	MX_OPERATION *operation;
	MX_OPERATION_FUNCTION_LIST *flist;
	mx_status_type ( *start_fn )( MX_OPERATION * );
	mx_status_type mx_status;

	mx_status = mx_operation_get_pointers( op_record,
					&operation, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = flist->start;

	if ( start_fn == NULL ) {
		operation->start = FALSE;

		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = (*start_fn)( operation );
	}

	operation->start = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_operation_stop( MX_RECORD *op_record )
{
	static const char fname[] = "mx_operation_stop()";

	MX_OPERATION *operation;
	MX_OPERATION_FUNCTION_LIST *flist;
	mx_status_type ( *stop_fn )( MX_OPERATION * );
	mx_status_type mx_status;

	mx_status = mx_operation_get_pointers( op_record,
					&operation, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop_fn = flist->stop;

	if ( stop_fn == NULL ) {
		operation->stop = FALSE;

		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = (*stop_fn)( operation );
	}

	operation->stop = FALSE;

	return mx_status;
}


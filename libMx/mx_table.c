/*
 * Name:    mx_table.c
 *
 * Purpose: MX function library of generic table operations.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_table.h"
#include "mx_motor.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_table_...
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_table_get_pointers( MX_RECORD *table_record,
			MX_TABLE **table,
			MX_TABLE_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_table_get_pointers()";

	if ( table_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The table_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( table_record->mx_class != MXC_TABLE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not a table record.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			table_record->name, calling_fname,
			table_record->mx_superclass,
			table_record->mx_class,
			table_record->mx_type );
	}

	if ( table != (MX_TABLE **) NULL ) {
		*table = (MX_TABLE *) (table_record->record_class_struct);

		if ( *table == (MX_TABLE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TABLE pointer for record '%s' passed by '%s' is NULL",
				table_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_TABLE_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_TABLE_FUNCTION_LIST *)
				(table_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_TABLE_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_TABLE_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				table_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_table_is_busy( MX_RECORD *table_record, long axis_id, mx_bool_type *busy )
{
	static const char fname[] = "mx_table_is_busy()";

	MX_TABLE *table;
	MX_TABLE_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_TABLE * );
	mx_status_type mx_status;

	mx_status = mx_table_get_pointers( table_record,
					&table, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->table_is_busy;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"table_is_busy function pointer is NULL.");
	}

	table->axis_id = axis_id;

	mx_status = ( *fptr ) ( table );

	if ( busy != (mx_bool_type *) NULL ) {
		*busy = table->busy;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_table_soft_abort( MX_RECORD *table_record, long axis_id )
{
	static const char fname[] = "mx_table_soft_abort()";

	MX_TABLE *table;
	MX_TABLE_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_TABLE * );
	mx_status_type mx_status;

	mx_status = mx_table_get_pointers( table_record,
					&table, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->soft_abort;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"soft_abort function pointer is NULL.");
	}

	table->axis_id = axis_id;

	mx_status = ( *fptr ) ( table );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_table_immediate_abort( MX_RECORD *table_record, long axis_id )
{
	static const char fname[] = "mx_table_immediate_abort()";

	MX_TABLE *table;
	MX_TABLE_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_TABLE *);
	mx_status_type mx_status;

	mx_status = mx_table_get_pointers( table_record,
					&table, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->immediate_abort;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"immediate_abort function pointer is NULL.");
	}

	table->axis_id = axis_id;

	mx_status = ( *fptr ) ( table );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_table_move_absolute( MX_RECORD *table_record,
				long axis_id, double destination )
{
	static const char fname[] = "mx_table_move_absolute()";

	MX_TABLE *table;
	MX_TABLE_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_TABLE * );
	mx_status_type mx_status;

	mx_status = mx_table_get_pointers( table_record,
					&table, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->move_absolute;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"move_absolute function pointer is NULL.");
	}

	table->axis_id = axis_id;

	table->destination = destination;

	mx_status = ( *fptr ) ( table );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_table_get_position( MX_RECORD *table_record,
				long axis_id, double *position )
{
	static const char fname[] = "mx_table_get_position()";

	MX_TABLE *table;
	MX_TABLE_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_TABLE * );
	mx_status_type mx_status;

	mx_status = mx_table_get_pointers( table_record,
					&table, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->get_position;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"get_position function pointer is NULL.");
	}

	table->axis_id = axis_id;

	mx_status = ( *fptr ) ( table );

	if ( position != (double *) NULL ) {
		*position = table->position;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_table_set_position( MX_RECORD *table_record,
				long axis_id, double set_position )
{
	static const char fname[] = "mx_table_set_position()";

	MX_TABLE *table;
	MX_TABLE_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_TABLE * );
	mx_status_type mx_status;

	mx_status = mx_table_get_pointers( table_record,
					&table, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->set_position;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"set_position function pointer is NULL.");
	}

	table->axis_id = axis_id;

	table->set_position = set_position;

	mx_status = ( *fptr ) ( table );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_table_positive_limit_hit( MX_RECORD *table_record,
				long axis_id, mx_bool_type *limit_hit )
{
	static const char fname[] = "mx_table_positive_limit_hit()";

	MX_TABLE *table;
	MX_TABLE_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_TABLE * );
	mx_status_type mx_status;

	mx_status = mx_table_get_pointers( table_record,
					&table, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->positive_limit_hit;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"positive_limit_hit function pointer is NULL.");
	}

	table->axis_id = axis_id;

	mx_status = ( *fptr ) ( table );

	if ( limit_hit != (mx_bool_type *) NULL ) {
		*limit_hit = table->positive_limit_hit;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_table_negative_limit_hit( MX_RECORD *table_record,
				long axis_id, mx_bool_type *limit_hit )
{
	static const char fname[] = "mx_table_negative_limit_hit()";

	MX_TABLE *table;
	MX_TABLE_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_TABLE * );
	mx_status_type mx_status;

	mx_status = mx_table_get_pointers( table_record,
					&table, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->negative_limit_hit;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"negative_limit_hit function pointer is NULL.");
	}

	table->axis_id = axis_id;

	mx_status = ( *fptr ) ( table );

	if ( limit_hit != (mx_bool_type *) NULL ) {
		*limit_hit = table->negative_limit_hit;
	}

	return mx_status;
}


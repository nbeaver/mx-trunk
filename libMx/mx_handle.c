/*
 * Name:    mx_handle.c
 *
 * Purpose: Support for MX handle tables.  MX handles are integer numbers
 *          that are used as indices into a table of C pointers.  This
 *          feature exists so that programming languages that cannot directly
 *          manipulate MX objects using C pointers can refer to the objects
 *          via handle numbers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004-2006, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_handle.h"

MX_EXPORT mx_status_type
mx_create_handle_table( MX_HANDLE_TABLE **handle_table,
			unsigned long handle_table_block_size,
			unsigned long num_blocks )
{
	static const char fname[] = "mx_create_handle_table()";

	MX_HANDLE_TABLE *new_handle_table;
	MX_HANDLE_STRUCT *handle_struct_array;
	unsigned long i, handle_table_size;

	if ( handle_table == (MX_HANDLE_TABLE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDLE_TABLE pointer passed was NULL." );
	}

	*handle_table = NULL;

	/* Create an initial block of handles. */

	new_handle_table = (MX_HANDLE_TABLE *)
				malloc( sizeof(MX_HANDLE_TABLE) );

	if ( new_handle_table == (MX_HANDLE_TABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Could not allocate an MX_HANDLE_TABLE structure." );
	}

	handle_table_size = handle_table_block_size * num_blocks;

	handle_struct_array = (MX_HANDLE_STRUCT *) malloc(
			handle_table_size * sizeof(MX_HANDLE_STRUCT) );

	if ( handle_struct_array == (MX_HANDLE_STRUCT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Could not allocate an array of %lu MX_HANDLE_STRUCTs.",
				handle_table_size );
	}

	new_handle_table->handles_in_use = 0L;
	new_handle_table->block_size = handle_table_block_size;
	new_handle_table->num_blocks = num_blocks;
	new_handle_table->handle_struct_array = handle_struct_array;

	/* Initialize the initial group of handles. */

	for ( i = 0; i < handle_table_size; i++ ) {
		handle_struct_array[i].handle = MX_ILLEGAL_HANDLE;
		handle_struct_array[i].pointer = NULL;
	}

	*handle_table = new_handle_table;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_resize_handle_table( MX_HANDLE_TABLE *handle_table,
			unsigned long new_num_blocks )
{
	static const char fname[] = "mx_resize_handle_table()";

	MX_HANDLE_STRUCT *old_handle_struct_array, *new_handle_struct_array;
	unsigned long i, old_array_size, new_array_size;

	if ( handle_table == (MX_HANDLE_TABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDLE_TABLE pointer passed was NULL." );
	}

	old_handle_struct_array = handle_table->handle_struct_array;
	old_array_size = handle_table->block_size * handle_table->num_blocks;

	if ( old_handle_struct_array == (MX_HANDLE_STRUCT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"handle_struct_array pointer for handle table %p is NULL.",
			handle_table );
	}

	new_array_size = new_num_blocks * handle_table->block_size;

	new_handle_struct_array = (MX_HANDLE_STRUCT *) realloc(
				old_handle_struct_array,
				new_array_size * sizeof( MX_HANDLE_STRUCT ) );

	if ( new_handle_struct_array == (MX_HANDLE_STRUCT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Failed at attempt to change the length of handle_struct_array "
"to %lu elements.", new_array_size );
	}

	for ( i = old_array_size; i < new_array_size; i++ ){
		new_handle_struct_array[i].handle = MX_ILLEGAL_HANDLE;
		new_handle_struct_array[i].pointer = NULL;
	}

	handle_table->handle_struct_array = new_handle_struct_array;
	handle_table->num_blocks = new_num_blocks;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_delete_handle_table( MX_HANDLE_TABLE *handle_table )
{
	if ( handle_table == (MX_HANDLE_TABLE *) NULL )
		return;

	if ( handle_table->handle_struct_array != NULL ) {
		free( handle_table->handle_struct_array );

		handle_table->handle_struct_array = NULL;
	}
	handle_table->handles_in_use = 0L;
	handle_table->block_size = 0L;
	handle_table->num_blocks = 0L;

	free( handle_table );

	return;
}

MX_EXPORT mx_status_type
mx_create_handle( signed long *handle, 
		MX_HANDLE_TABLE *handle_table,
		void *pointer )
{
	static const char fname[] = "mx_create_handle()";

	MX_HANDLE_STRUCT *handle_struct_array;
	unsigned long i, array_size;
	mx_status_type status;

	if ( handle == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"handle pointer passed is NULL." );
	}

	*handle = MX_ILLEGAL_HANDLE;

	if ( handle_table == (MX_HANDLE_TABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_HANDLE_TABLE pointer is NULL." );
	}
	if ( pointer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL pointer argument passed." );
	}

	/* Do we need to expand the size of the handle structure array? */

	array_size = handle_table->block_size * handle_table->num_blocks;

	MX_DEBUG( 8,("%s: handles_in_use = %lu, array_size = %lu",
		fname, handle_table->handles_in_use, array_size));

	if ( handle_table->handles_in_use >= array_size ) {

		status = mx_resize_handle_table( handle_table,
						1 + handle_table->num_blocks );

		if ( status.code != MXE_SUCCESS )
			return status;
	}

	handle_struct_array = handle_table->handle_struct_array;

	if ( handle_struct_array == (MX_HANDLE_STRUCT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"handle_struct_array for handle table %p is NULL.",
			handle_table );
	}

	array_size = handle_table->block_size * handle_table->num_blocks;

	/* Search for a slot in the array that is not in use. */

	for ( i = 0; i < array_size; i++ ) {
		if ( handle_struct_array[i].handle < 0 ) {
			break;			/* Exit the for() loop. */
		}
	}

	if ( i >= array_size ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Can't find an empty slot in the MX handle table.  "
		"This shouldn't be able to happen." );
	}

	handle_struct_array[i].handle = (signed long) i;
	handle_struct_array[i].pointer = pointer;

	*handle = (signed long) i;

	( handle_table->handles_in_use )++;

	MX_DEBUG( 2,("%s: pointer = %p, handle = %ld",
			fname, pointer, *handle ));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_handle( signed long handle, MX_HANDLE_TABLE *handle_table )
{
	static const char fname[] = "mx_delete_handle()";

	MX_HANDLE_STRUCT *handle_struct_array;

	if ( handle < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal MX handle value %ld passed.", handle );
	}
	if ( handle_table == (MX_HANDLE_TABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL handle_table pointer passed." );
	}

	handle_struct_array = handle_table->handle_struct_array;

	if ( handle_struct_array == (MX_HANDLE_STRUCT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"handle_struct_array for handle table %p is NULL.",
			handle_table );
	}

	handle_struct_array[handle].handle = MX_ILLEGAL_HANDLE;
	handle_struct_array[handle].pointer = NULL;

	(handle_table->handles_in_use)--;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_handle_from_pointer( signed long *handle,
			MX_HANDLE_TABLE *handle_table,
			void *pointer )
{
	static const char fname[] = "mx_get_handle_from_pointer()";

	MX_HANDLE_STRUCT *handle_struct_array;
	unsigned long i, array_size;
	mx_status_type status;

	if ( handle == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL handle pointer passed." );
	}

	*handle = MX_ILLEGAL_HANDLE;

	if ( pointer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL pointer argument passed." );
	}

	MX_DEBUG( 8,("%s invoked.  handle_table = %p, pointer = %p",
			fname, handle_table, pointer));

	handle_struct_array = handle_table->handle_struct_array;

	if ( handle_struct_array == (MX_HANDLE_STRUCT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"handle_struct_array for handle table %p is NULL.",
			handle_table );
	}

	/* Does the handle already exist? */

	array_size = handle_table->block_size * handle_table->num_blocks;

	for ( i = 0; i < array_size; i++ ) {
		if ( handle_struct_array[i].pointer == pointer ) {
			break;			/* Exit the for() loop. */
		}
	}

	if ( i < array_size ) {
		*handle = handle_struct_array[i].handle;
	} else {
		/* Need to create a new handle. */

		status = mx_create_handle( handle, handle_table, pointer );

		if ( status.code != MXE_SUCCESS )
			return status;
	}

	MX_DEBUG( 8,("%s: *handle = %ld", fname, *handle));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_pointer_from_handle( void **pointer,
			MX_HANDLE_TABLE *handle_table,
			signed long handle )
{
	static const char fname[] = "mx_get_pointer_from_handle()";

	MX_HANDLE_STRUCT *handle_struct_array;
	signed long maximum_handle_number;

	if ( pointer == (void **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL pointer argument passed." );
	}

	*pointer = NULL;

	if ( handle_table == (MX_HANDLE_TABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL handle_table pointer passed." );
	}
	if ( handle < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Negative handle values like %ld are illegal.", handle );
	}

	handle_struct_array = handle_table->handle_struct_array;

	if ( handle_struct_array == (MX_HANDLE_STRUCT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"handle_struct_array for handle table %p is NULL.",
			handle_table );
	}

	/* Is this a valid handle number? */

	if ( handle < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested handle %ld is a negative number "
		"which is not allowed.", handle );
	}

	maximum_handle_number = (signed long)
			( handle_table->block_size * handle_table->num_blocks );

	if ( handle >= maximum_handle_number ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested handle %ld is larger than the current "
		"maximum handle value %ld.", handle, maximum_handle_number );
	}

	/* Is this handle active? */

	if ( handle != handle_struct_array[handle].handle ) {
		return mx_error( MXE_BAD_HANDLE, fname,
		"The handle %ld is not currently an active handle.", handle );
	}

	/* Send back the pointer. */

	*pointer = handle_struct_array[handle].pointer;

	if ( *pointer == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The pointer for handle %ld in handle table %p is NULL.",
			handle, handle_table );
	}
	return MX_SUCCESSFUL_RESULT;
}


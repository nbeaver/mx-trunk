/*
 * Name:     mx_record.c
 *
 * Purpose:  Generic record support.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_variable.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_handle.h"
#include "mx_signal.h"
#include "mx_list_head.h"
#include "mx_net.h"

/* === Private function definitions === */

static mx_status_type mx_delete_placeholders( MX_RECORD *record,
						MX_LIST_HEAD *list_head );

static mx_status_type mx_delete_placeholder_handler( MX_RECORD *record,
					MX_RECORD_FIELD *record_field,
					void *handler_data_ptr,
					long *array_indices,
					void *array_ptr,
					long dimension_level );

/* === */

MX_EXPORT int
mx_verify_driver_type( MX_RECORD *record, long mx_superclass,
				long mx_class, long mx_type )
{
	static const char fname[] = "mx_verify_driver_type()";

	int superclass_matches, class_matches, type_matches;
	int record_matches;

	if ( record == (MX_RECORD *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );

		return FALSE;
	}

	superclass_matches = FALSE;
	class_matches = FALSE;
	type_matches = FALSE;

	if ( ( mx_superclass == MXR_ANY )
	  || ( mx_superclass == record->mx_superclass ) )
	{
		superclass_matches = TRUE;
	}

	if ( ( mx_class == MXC_ANY )
	  || ( mx_class == record->mx_class ) )
	{
		class_matches = TRUE;
	}

	if ( ( mx_type == MXT_ANY )
	  || ( mx_type == record->mx_type ) )
	{
		type_matches = TRUE;
	}

	if ( superclass_matches && class_matches && type_matches ) {
		record_matches = TRUE;
	} else {
		record_matches = FALSE;
	}

#if 0
	MX_DEBUG(-2,("%s: '%s', (%ld, %ld, %ld) (%ld, %ld, %ld) (%d, %d, %d)",
		fname, record->name,
		mx_superclass, mx_class, mx_type,
		record->mx_superclass, record->mx_class, record->mx_type,
		superclass_matches, class_matches, type_matches));
#endif

	return record_matches;
}

MX_EXPORT MX_DRIVER *
mx_get_driver_by_name( char *name )
{
	static const char fname[] = "mx_get_driver_by_name()";

	MX_DRIVER **driver_list_array;
	MX_DRIVER *mx_type_list, *result;
	char *list_name;
	int i;

	if ( name == NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"Record type name passed was NULL." );

		return NULL;
	}

	driver_list_array = mx_get_driver_lists();

	mx_type_list = driver_list_array[2];

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( mx_type_list[i].mx_superclass == 0 ) {
			return (MX_DRIVER *) NULL;
		}

		list_name = mx_type_list[i].name;

		if ( list_name == NULL ) {
			return (MX_DRIVER *) NULL;
		}

		if ( strcmp( name, list_name ) == 0 ){
			result = &( mx_type_list[i] );

			MX_DEBUG( 8,
	( "%s: ptr = 0x%p, type = %ld", fname, result, result->mx_type));

			return result;
		}
	}
}

MX_EXPORT MX_DRIVER *
mx_get_driver_by_type( long mx_type )
{
	static const char fname[] = "mx_get_driver_by_type()";

	MX_DRIVER **driver_list_array;
	MX_DRIVER *mx_type_list, *result;
	int i;

	driver_list_array = mx_get_driver_lists();

	mx_type_list = driver_list_array[2];

	for ( i = 0; ; i++ ) {
		/* Check for the end of the list. */

		if ( mx_type_list[i].mx_superclass == 0 ) {
			return (MX_DRIVER *) NULL;
		}

		if ( mx_type_list[i].mx_type == mx_type ) {

			result = &( mx_type_list[i] );

			MX_DEBUG( 8,
	( "%s: ptr = 0x%p, type = %ld", fname, result, result->mx_type));

			return result;
		}
	}
}

MX_EXPORT MX_DRIVER *
mx_get_driver_for_record( MX_RECORD *record )
{
	static const char fname[] = "mx_get_driver_for_record()";

	MX_RECORD_FIELD *field;
	MX_DRIVER *driver;

	if ( record == (MX_RECORD *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );

		return NULL;
	}

	field = mx_get_record_field( record, "mx_type" );

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Record '%s' does not have a 'mx_type' field.  "
		"This should be impossible!", record->name );

		return NULL;
	}

	driver = (MX_DRIVER *) field->typeinfo;

	if ( driver == (MX_DRIVER *) NULL ) {
		mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Record '%s' does not appear to have a device driver.  "
		"This should be impossible for a running database!", 
			record->name );

		return NULL;
	}

	return driver;
}

MX_EXPORT const char *
mx_get_driver_name( MX_RECORD *record )
{
	MX_DRIVER *driver;

	driver = mx_get_driver_for_record( record );

	if ( driver == (MX_DRIVER *) NULL ) 
		return NULL;

	return driver->name;
}

MX_EXPORT MX_RECORD *
mx_create_record( void )
{
	static const char fname[] = "mx_create_record()";

	static MX_RECORD *new_record;

	new_record = (MX_RECORD *) malloc( sizeof(MX_RECORD) );

	MX_DEBUG(8,("%s: new_record = %p", fname, new_record));

	if ( new_record == (MX_RECORD *) NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
		"Memory allocation failed trying to allocate a new record.");
	} else {
		/* Initialize all the non-list related fields. */

		new_record->mx_superclass = 0;
		new_record->mx_class = 0;
		new_record->mx_type = 0;
		strcpy( (new_record->name), "" );
		strcpy( (new_record->label), "" );
		new_record->handle = MX_ILLEGAL_HANDLE;
		new_record->precision = 3;
		new_record->resynchronize = 0;
		new_record->record_flags = MXF_REC_ENABLED;
		new_record->record_processing_flags = 0;
		new_record->record_superclass_struct = NULL;
		new_record->record_class_struct = NULL;
		new_record->record_type_struct = NULL;
		new_record->record_function_list = NULL;
		new_record->superclass_specific_function_list = NULL;
		new_record->class_specific_function_list = NULL;
		new_record->num_record_fields = 0;
		new_record->record_field_array = NULL;
		new_record->allocated_by = NULL;
		new_record->num_groups = 0;
		new_record->group_array = NULL;
		new_record->num_parent_records = 0;
		new_record->parent_record_array = NULL;
		new_record->num_child_records = 0;
		new_record->child_record_array = NULL;
		new_record->event_time_manager = NULL;
		new_record->event_queue = NULL;
		new_record->application_ptr = NULL;

		new_record->previous_record = NULL;
		new_record->next_record = NULL;
		new_record->list_head = NULL;
	}
	return new_record;
}

MX_EXPORT mx_status_type
mx_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mx_delete_record()";

	MX_LIST_HEAD *list_head_struct;
	MX_RECORD *previous_record, *next_record, *parent_record;
	MX_RECORD_FUNCTION_LIST *flist;
	mx_status_type (*fptr)( MX_RECORD * );
	long i, num_parent_records;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL record passed to this routine." );
	}

	MX_DEBUG( 8,("%s: Deleting '%s' (%p)", fname, record->name, record));

	/* Does this record have any child records that depend
	 * on it?  If so, do not permit the record to be deleted.
	 */

	if ( record->num_child_records > 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"The record '%s' has %ld child records that depend on it.  The child "
	"records must be deleted first before this record may be deleted.",
				record->name, record->num_child_records );
	}

	/* Get a pointer to the record list head for later use. */

#if 0
	list_head_struct = mx_get_record_list_head_struct( record );
#else
	{
		MX_RECORD *list_head_record;

		list_head_record = record->list_head;

		if ( list_head_record == (MX_RECORD *) NULL ) {
			/* If this is a placeholder record, then just
			 * delete the MX_RECORD structure, since the record
			 * will not have been completely set up.
			 */

			if ( record->mx_superclass == MXR_PLACEHOLDER ) {
				mx_free( record );

				return MX_SUCCESSFUL_RESULT;
			} else {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
			"list_head record pointer for record %p is NULL.",
				record );
			}
		}
		list_head_struct = (MX_LIST_HEAD *)
			list_head_record->record_superclass_struct;
	}
#endif

	if ( list_head_struct == NULL ) {

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LIST_HEAD structure for record %p is NULL.",
				record );
	}

	/* If this record has any parent record dependencies,
	 * delete them.  Note that we do not index the parent
	 * record array using the index 'i'.  This is because
	 * mx_delete_parent_dependency() squeezes out any NULLs
	 * in the parent record array before returning.  Thus,
	 * if we keep reading from element[0] of the array,
	 * each of the parent records will be shifted down
	 * to the beginning of the array in turn.
	 */

	if ( record->num_parent_records > 0 ) {

		num_parent_records = record->num_parent_records;

		for ( i = 0; i < num_parent_records; i++ ) {

			parent_record = record->parent_record_array[0];

			mx_status = mx_delete_parent_dependency(
					record, TRUE, parent_record );

			if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
		}
	}

	/* Eliminate the MX handle, if any. */

	if ( record->handle >= 0 ) {
		(void) mx_delete_handle( record->handle,
					list_head_struct->handle_table );
	}

	/* If the record list is not marked as active, then
	 * mx_create_record_from_description() installs placeholder records 
	 * rather than pointers to the actual record.  If the record list
	 * is still marked as inactive when we get here, then
	 * mx_fixup_placeholder_records() will not have been called and
	 * we have to find those placeholder records and get rid of them.
	 */

	if ( list_head_struct->fixup_records_in_use ) {
		mx_status = mx_delete_placeholders( record, list_head_struct );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Decrement the number of records in the record list. */

	if ( list_head_struct->num_records == 0 ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Attempted to subtract from the number of records "
		"when the recorded number of records is already zero "
		"while deleting record '%s'", record->name );
	} else {
		list_head_struct->num_records--;

		MX_DEBUG( 8,("%s: deleted record '%s', num_records = %lu",
			fname, record->name, list_head_struct->num_records));
	}

	/* Find the type specific 'delete record' function to 
	 * delete the type specific parts of the record.
	 */

	flist = (MX_RECORD_FUNCTION_LIST *) (record->record_function_list);

	if ( flist == (MX_RECORD_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"record_function_list pointer for record '%s' is NULL.",
				record->name );
	}

	fptr = flist->delete_record;

	if ( fptr == NULL ) {
		mx_status = mx_default_delete_record_handler( record );
	} else {
		mx_status = (*fptr)( record );

		/* Question to ponder: Should a failure of the
		 * 'delete_record' function stop us from removing
		 * the MX_RECORD structure from the record list?
		 * For now, it does not.
		 */
	}

	/* Eliminate the MX_RECORD part of the record from the list. */

	previous_record = record->previous_record;
	next_record = record->next_record;

	next_record->previous_record = previous_record;
	previous_record->next_record = next_record;

	MX_DEBUG( 8,("%s: About to free record '%s'", fname, record->name));

	/* Now get rid of the MX_RECORD structure itself. */

	*(record->name) = '\0';   /* Erase the name */

	free( record );

	MX_DEBUG( 8,("%s is complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_default_delete_record_handler( MX_RECORD *record )
{
	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_delete_placeholders( MX_RECORD *record, MX_LIST_HEAD *list_head )
{
	static const char fname[] = "mx_delete_placeholders()";

	long i, num_record_fields;
	MX_RECORD_FIELD *record_field_array, *record_field;
	mx_status_type mx_status;

	MX_DEBUG( 8,("*** %s invoked for record '%s' ***",
			fname, record->name));

	num_record_fields = record->num_record_fields;
	record_field_array = record->record_field_array;

	for ( i = 0; i < num_record_fields; i++ ) {
		record_field = &record_field_array[i];

		MX_DEBUG( 8,("%s: record_field = '%s'",
				fname, record_field->name));

		if ( ( record_field->datatype == MXFT_RECORD )
		  || ( record_field->datatype == MXFT_INTERFACE ) )
		{
			switch( record_field->label_value ) {
			case MXLV_REC_ALLOCATED_BY:
			case MXLV_REC_GROUP_ARRAY:
			case MXLV_REC_PARENT_RECORD_ARRAY:
			case MXLV_REC_CHILD_RECORD_ARRAY:
				/* Skip over several fields. */

				break;
			default:
				mx_status = mx_traverse_field( record,
						record_field,
						mx_delete_placeholder_handler,
						list_head,
						NULL );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				break;
			}
		}
	}

	MX_DEBUG( 8,("%s is complete for record '%s'.", fname, record->name));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_delete_placeholder_handler( MX_RECORD *record,
				MX_RECORD_FIELD *record_field,
				void *handler_data_ptr,
				long *array_indices,
				void *array_ptr,
				long dimension_level )
{
	static const char fname[] = "mx_delete_placeholder_handler()";

	MX_RECORD *placeholder_record;
	MX_INTERFACE *interface;
	MX_LIST_HEAD *list_head;
	long fixup_record_index;
	void **fixup_record_array;
	void *value_ptr;

	if ( dimension_level > 0 ) {

		/* We haven't reached the bottom array level of the
		 * field data yet, so just return.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG( 8,("%s invoked for record '%s', field '%s'.",
			fname, record->name, record_field->name ));

	switch( record_field->datatype ) {
	case MXFT_INTERFACE:
		interface = (MX_INTERFACE *) array_ptr;
		placeholder_record = interface->record;
		break;
	case MXFT_RECORD:
		value_ptr = mx_read_void_pointer_from_memory_location(
								array_ptr );
		placeholder_record = (MX_RECORD *) value_ptr;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal field datatype %ld for record '%s', field '%s'",
			record_field->datatype, record->name,
			record_field->name );
		break;
	}

	if ( placeholder_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The placeholder_record pointer for record field '%s' of "
		"record '%s' is NULL.  "
		"This probably means that the MX database file entry for "
		"record '%s' was incorrectly formatted and that parsing "
		"of the record description aborted early.",
			record_field->name, record->name, record->name );
	}

	MX_DEBUG( 8,("%s: placeholder_record = %p", fname, placeholder_record));
	MX_DEBUG( 8,("%s: placeholder_record = '%s'",
				fname, placeholder_record->name ));

	if ( placeholder_record->mx_superclass != MXR_PLACEHOLDER ) {

		MX_DEBUG(-2,("%s: Record '%s' is not a placeholder.",
			fname, record->name));

		return MX_SUCCESSFUL_RESULT;
	}

	/* Remove the placeholder from the fixup record array.  The index
	 * for this placeholder in the fixup record array is stored in the
	 * record_flags item of the placeholder_record structure.
	 */

	fixup_record_index = (long) placeholder_record->record_flags;

	list_head = (MX_LIST_HEAD *) handler_data_ptr;

	if ( ( fixup_record_index < 0 )
	  || ( fixup_record_index >= list_head->num_fixup_records ) )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The fixup_record_index (%ld) for record '%s', field '%s' is outside "
	"the allowed range of 0 to %ld.",
			fixup_record_index, record->name, record_field->name,
			(list_head->num_fixup_records) - 1 );
	}

	fixup_record_array = list_head->fixup_record_array;

	fixup_record_array[ fixup_record_index ] = NULL;

	/* Delete the placeholder record. */

	MX_DEBUG( 8,("%s: About to free the placeholder record.", fname));

	free( placeholder_record );

	MX_DEBUG( 8,("%s: After freeing the placeholder record.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/* mx_insert_before_record() adds a record just before the current record. */

MX_EXPORT mx_status_type
mx_insert_before_record( MX_RECORD *current_record, MX_RECORD *new_record )
{
	static const char fname[] = "mx_insert_before_record()";

	MX_RECORD *previous_record;
	mx_status_type mx_status;

	if ( current_record == (MX_RECORD *) NULL) {
		previous_record = (MX_RECORD *) NULL;
	} else {
		previous_record = current_record->previous_record;

		if ( previous_record == ( MX_RECORD * ) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Pointer to the record prior to the record %p is NULL.",
				current_record );
		}
	}
	mx_status = mx_insert_after_record( previous_record, new_record );

	return mx_status;
}

/* mx_insert_after_record() adds a record to the list just after the one
 * specified in the calling arguments.
 */

MX_EXPORT mx_status_type
mx_insert_after_record( MX_RECORD *old_record, MX_RECORD *new_record )
{
	static const char fname[] = "mx_insert_after_record()";

	MX_RECORD *old_next_record;
	MX_LIST_HEAD *list_head;

	if ( old_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"old_record pointer passed was NULL." );
	}
	if ( new_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"new_record pointer passed was NULL." );
	}

	list_head = mx_get_record_list_head_struct(old_record);

	new_record->precision = list_head->default_precision;

	old_next_record = old_record->next_record;

	if ( old_next_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"old_record->next_record == NULL.  This shouldn't happen!" );
	}

	new_record->next_record = old_next_record;
	new_record->previous_record = old_record;
	new_record->list_head = old_record->list_head;

	/* Insert the record into the list. */

	old_next_record->previous_record = new_record;
	old_record->next_record = new_record;

	list_head->num_records++;

	MX_DEBUG( 8,("%s: inserted record '%s', num_records = %lu",
		fname, new_record->name, list_head->num_records));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT MX_RECORD *
mx_get_record( MX_RECORD *specified_record, char *record_name )
{
	static const char fname[] = "mx_get_record()";

	MX_RECORD *current_record;
	MX_RECORD *matching_record;

	if ( specified_record == (MX_RECORD *) NULL ) {
		mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record pointer passed was NULL." );

		return NULL;
	}
	if ( record_name == NULL ) {
		mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record name passed was NULL." );

		return NULL;
	}

	/* Walk through the linked list looking for the record.
	 * Since the list is a circular list, we will find the
	 * record regardless of where in the list we start.
	 */

	current_record = specified_record;
	matching_record = (MX_RECORD *) NULL;

	while ( current_record != NULL ) {

		if ( strcmp( current_record->name, record_name ) == 0 ) {
			matching_record = current_record;

			break;		/* Exit the while() loop. */
		}

		current_record = current_record->next_record;

		if ( current_record == specified_record ) {
			return NULL;
		}
	}

	return matching_record;
}

MX_EXPORT mx_status_type
mx_delete_record_list( MX_RECORD *record_list )
{
	mx_status_type mx_status;

	mx_status = mx_delete_record_class( record_list, MXR_ANY );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_delete_record_class( MX_RECORD *record_list, long record_class )
{
	static const char fname[] = "mx_delete_record_class()";

	MX_RECORD *current_record;
	MX_RECORD *next_record;
	MX_RECORD *last_record;
	long current_record_class;
	mx_status_type mx_status;

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"record list pointer passed was NULL.");
	}

	current_record = record_list;
	next_record = current_record->next_record;
	last_record = current_record->previous_record;

	if ( next_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"List error:  current_record->next_record is NULL.\n"
			"This shouldn't happen." );
	}

	if ( last_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "List error:  current_record->previous_record is NULL.\n"
		    "This shouldn't happen." );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	/* Traverse the list until we reach the end. */

	while ( next_record != last_record ) {

		current_record_class = current_record->mx_class;

		/* Delete the record if the record class matches. */

		if ( current_record_class & record_class ) {
			mx_status = mx_delete_record( current_record );

			if ( mx_status.code != MXE_SUCCESS ) {
				return mx_status;
			}
		}

		current_record = next_record;
		next_record = current_record->next_record;

		if ( next_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"List error:  current_record->next_record is NULL.\n"
			"This shouldn't happen." );
		}
	}

	/* Delete the last record if the record class matches. */

	current_record_class = current_record->mx_class;

	if ( current_record_class & record_class ) {
		mx_status = mx_delete_record( current_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return mx_status;
		}
	}

	return mx_status;
}

MX_EXPORT MX_RECORD *
mx_initialize_record_list( void )
{
	static const char fname[] = "mx_initialize_record_list()";

	MX_RECORD *record_list_head;
	mx_status_type mx_status;

	/* Construct the head of list record. */

	record_list_head = mx_create_record();

	MX_DEBUG( 2, ("record_list_head = 0x%p", record_list_head) );

	if ( record_list_head == NULL ) {
		mx_error( MXE_OUT_OF_MEMORY, fname,
			"Out of memory allocating record list head.");

		return (MX_RECORD *) NULL;
	}

	/* Make it into the first record of a record list. */

	record_list_head->previous_record = record_list_head;
	record_list_head->next_record = record_list_head;
	record_list_head->list_head = record_list_head;

	/* Fill in the fields of the record list head. */

	mx_status = mxr_create_list_head( record_list_head );

	if ( mx_status.code != MXE_SUCCESS )
		return NULL;

	MX_DEBUG( 2,("%s complete.", fname));

	return record_list_head;
}

MX_EXPORT mx_status_type
mx_initialize_drivers( void )
{
	static const char fname[] = "mx_initialize_drivers()";

	MX_DRIVER **mx_list_of_types;
	MX_DRIVER *list_ptr;
	MX_RECORD_FIELD_DEFAULTS *defaults_array;
	MX_RECORD_FIELD_DEFAULTS *array_element;
	MX_RECORD_FUNCTION_LIST *flist_ptr;
	mx_status_type (*fptr) ( long );
	mx_status_type mx_status;
	long *num_record_fields_ptr;
	long i, j, num_record_fields;

	MX_DEBUG( 6,("%s invoked.", fname));

	mx_status = MX_SUCCESSFUL_RESULT;

	/* Go through the list of compiled in drivers. */

	mx_list_of_types = mx_get_driver_lists();

	for ( i = 0; mx_list_of_types[i] != NULL; i++ ) {

#if 0
		list_ptr = (MX_DRIVER *) (mx_list_of_types[i]);
#else
		list_ptr = mx_list_of_types[i];
#endif

		MX_DEBUG( 8,("i = %ld, list_ptr = %p", i, list_ptr));

		while ( list_ptr->mx_type != 0 ) {
			MX_DEBUG( 8,
		("superclass = %ld, class = %ld, type = %ld, name = '%s'",
				list_ptr->mx_superclass, list_ptr->mx_class,
				list_ptr->mx_type, list_ptr->name));

			/* Initialize the typeinfo fields if possible. */

			if ( (list_ptr->num_record_fields != NULL)
			  && (list_ptr->record_field_defaults_ptr != NULL) )
			{
				MX_DEBUG( 8,
		("num_record_fields = %ld, *record_field_defaults_ptr = %p",
				    *(list_ptr->num_record_fields),
				    *(list_ptr->record_field_defaults_ptr)));

				mx_status
				    = mx_setup_typeinfo_for_record_type_fields(
					*(list_ptr->num_record_fields),
					*(list_ptr->record_field_defaults_ptr),
					list_ptr->mx_type,
					list_ptr->mx_class,
					list_ptr->mx_superclass );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

			/* Next, do type specific initialization. */

			flist_ptr = (MX_RECORD_FUNCTION_LIST *)
				(list_ptr->record_function_list);

			MX_DEBUG( 8,("  record_function_list pointer = %p",
				flist_ptr));

			if ( flist_ptr != NULL ) {
				fptr = flist_ptr->initialize_type;

				MX_DEBUG( 8,("  initialize_type pointer = %p",
					fptr));

				/* If all is well, invoke the function. */

				if ( fptr != NULL ) {
					mx_status = (*fptr)( list_ptr->mx_type );

					if ( mx_status.code != MXE_SUCCESS )
						return mx_status;
				}
			}

			/* Assign values to the field numbers. */

			num_record_fields_ptr = list_ptr->num_record_fields;

			if ( num_record_fields_ptr != NULL ) {
				defaults_array
				    = *(list_ptr->record_field_defaults_ptr);

				num_record_fields = *num_record_fields_ptr;

				for ( j = 0; j < num_record_fields; j++ ) {
					array_element = &defaults_array[j];

					array_element->field_number = j;
				}
			}

			list_ptr++;
		}
	}

	MX_DEBUG( 6,("%s done.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mx_read_database_file( MX_RECORD *record_list_head,
			char *filename,
			unsigned long flags )
{
	static const char fname[] = "mx_read_database_file()";

	MX_RECORD *created_record;
	FILE *file;
	char buffer[MXU_RECORD_DESCRIPTION_LENGTH+1];
	int line_number, saved_errno;
	size_t length;
	MX_RECORD_FIELD_PARSE_STATUS parse_status;
	char token[ MXU_FILENAME_LENGTH + 1 ];
	mx_status_type mx_status;

	char separators[] = MX_RECORD_FIELD_SEPARATORS;

	/* See if we can open the input file. */

	MX_DEBUG( 2, ( "mx_read_database_file() entered.  "
		"Filename = '%s', record_list = 0x%p",
		filename, record_list_head ) );

	file = fopen( filename, "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot open MX database file '%s'.  "
			"errno = %d, error message = '%s'.",
			filename, saved_errno, strerror( saved_errno ) );
	}

	/* Try to read the first record in the file. */

	fgets( buffer, sizeof buffer, file );

	if ( feof(file) ) {
		mx_info("WARNING: Save file '%s' is empty.", filename);
		fclose( file );
		return MX_SUCCESSFUL_RESULT;
	}

	/* Loop until all the database file lines have been read. */

	line_number = 1;

	while ( 1 ) {
		MX_DEBUG( 2, ("line %d: '%s'", line_number, buffer));

		/* Zap any trailing newline. */

		length = strlen( buffer );

		if ( buffer[ length - 1 ] == '\n' ) {
			buffer[ length - 1 ] = '\0';
		}

		/* Figure out what to do with this line. */

		if ( buffer[0] == '#' ) {

			/* Lines that begin with the comment character '#'
			 * are skipped.
			 */

		} else if ( strncmp( buffer, "!include ", 9 ) == 0 ) {

			/* If the first 9 characters of a line consists
			 * of the string '!include' followed by a space,
			 * then the rest of the line is assumed to contain
			 * the name of another database file to include
			 * here.
			 */

			/* Get ready to parse the first two tokens on
			 * this line.
			 */

			mx_initialize_parse_status( &parse_status,
							buffer, separators );

			/* Skip the first token since we already know that
			 * it is the !include statement.
			 */

			mx_status = mx_get_next_record_token( &parse_status,
						token, sizeof( token ) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* The next token should be the include file name
			 * that we are looking for.
			 */

			mx_status = mx_get_next_record_token( &parse_status,
						token, sizeof( token ) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			MX_DEBUG( 2,("%s: Trying to read include file '%s'",
				fname, token));

			/* Try to read the include file. */

			mx_status = mx_read_database_file( record_list_head,
						token, flags );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			MX_DEBUG( 2,("%s: Successfully read include file '%s'",
				fname, token));
		} else {
			/* Otherwise, we assume this line is just a
			 * record description and try to parse it.
			 */

			mx_status = mx_create_record_from_description(
					record_list_head, buffer,
					&created_record, flags );

			if ( mx_status.code != MXE_SUCCESS ) {
			    if ( (flags & MXFC_DELETE_BROKEN_RECORDS) == 0 ) {

				return mx_status;

			    } else {
				if ( created_record == (MX_RECORD *) NULL ) {
					mx_warning(
				"Skipping unrecognizable line in file '%s'.",
						filename );
				} else {
				    mx_warning(
				"Deleting broken record '%s' from file '%s'.",
					created_record->name, filename );

				    (void) mx_delete_record( created_record );
				}
			    }
			}
		}

		/* Get the next line from the save file. */

		fgets( buffer, sizeof buffer, file );

		/* If we have reached the end of the file, we are done. */

		if ( feof(file) ) {
			MX_DEBUG( 2, ("End of save file reached!"));
			break;		/* Exit the while() loop. */
		}

		line_number++;
	}

	fclose( file );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mx_finish_record_initialization()";

	MX_RECORD_FIELD *field;
	MX_RECORD_FUNCTION_LIST *flist;
	MX_RECORD_ARRAY_DEPENDENCY_STRUCT dependency_struct;
	mx_status_type ( *fptr )( MX_RECORD * );
	long i;
	mx_status_type mx_status;

	MX_DEBUG( 6,("%s: *** Record '%s' ***", fname, record->name));

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	flist = (MX_RECORD_FUNCTION_LIST *) (record->record_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The record_function_list element of record '%s' is NULL.  "
		"This should not be able to happen.", record->name );
	}

	fptr = flist->finish_record_initialization;

	if ( fptr != NULL ) {
		mx_status = (*fptr)( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Now that the device specific initialization has finished,
	 * go through the record field list to see if any of them
	 * are of type MXFT_RECORD or MXFT_INTERFACE.
	 */

	for ( i = 0; i < record->num_record_fields; i++ ) {

		field = &(record->record_field_array[i]);

		if ( ( field->datatype == MXFT_RECORD )
		  || ( field->datatype == MXFT_INTERFACE ) ) {

			switch( field->label_value ) {
			case MXLV_REC_ALLOCATED_BY:
			case MXLV_REC_GROUP_ARRAY:
			case MXLV_REC_PARENT_RECORD_ARRAY:
			case MXLV_REC_CHILD_RECORD_ARRAY:
				/* Skip over several fields. */

				break;
			default:
				dependency_struct.add_dependency = TRUE;
				dependency_struct.dependency_is_to_parent
								= TRUE;

				mx_status = mx_traverse_field( record, field,
					    mx_record_array_dependency_handler,
					    &dependency_struct, NULL );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				break;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_finish_database_initialization( MX_RECORD *record_list_head )
{
	static const char fname[] = "mx_finish_database_initialization()";

	MX_RECORD *current_record, *next_record;
	MX_LIST_HEAD *list_head_struct;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* The following comment is only true for record types handled
	 * by the the new record field support.  Hopefully, soon this
	 * will be all of them.
	 */

	/* When mx_create_record_from_description() finds a reference
	 * in a record description to a different record that the
	 * current record depends on, it does not immediately try
	 * find the record using mx_get_record().  Rather, it installs
	 * a placeholder for the other record in the current record.
	 * This is to allow for forward references in the description
	 * file (among other things).
	 *
	 * Now that we have read all of the database file description
	 * lines, we must go back and replace all those placeholders
	 * with pointers to the real records.  That is what the
	 * function mx_fixup_placeholder_records() does.
	 */

	mx_status = mx_fixup_placeholder_records( record_list_head );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Finally, we invoke the "finish_record_initialization"
	 * function for all of the records to allow them to perform
	 * any record initialization that cannot be done until
	 * all of the MX_RECORD pointers have been set up correctly
	 * by mx_fixup_placeholder_records().
	 * 
	 * If a given record has been marked as broken, probably by
	 * mx_fixup_placeholder_records(), then the record should
	 * be deleted.  In addition, scan records that fail their
	 * finish_record_initialization step should be deleted rather
	 * than causing database initialization to be aborted.
	 */

	current_record = record_list_head;

	do {
		next_record = current_record->next_record;

		if ( next_record == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"next_record pointer for record '%s' is NULL!  "
			"On a circular list, this shouldn't happen.",
				current_record->name );
		}

		if ( current_record->record_flags & MXF_REC_BROKEN ) {
			mx_warning( "Deleting broken record '%s'.",
				current_record->name );

			mx_status = mx_delete_record( current_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} else {
			mx_status =
			    mx_finish_record_initialization( current_record );

			if ( mx_status.code != MXE_SUCCESS ) {
			    if ( current_record->mx_superclass != MXR_SCAN ) {

				return mx_status;

			    } else {
				mx_warning( "Deleting broken scan record '%s'.",
					current_record->name );

				mx_status = mx_delete_record( current_record );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			    }
			}
		}

		current_record = next_record;

	} while ( current_record != record_list_head );

	/* Initialization is complete, so mark the list as active. */

	list_head_struct
	    = (MX_LIST_HEAD *)( record_list_head->record_superclass_struct );

	list_head_struct->list_is_active = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_write_database_file( MX_RECORD *record_list, char *filename,
	long num_record_superclasses, long *record_superclass_list )
{
	static const char fname[] = "mx_write_database_file()";

	MX_RECORD *current_record;
	FILE *file;
	char buffer[MXU_RECORD_DESCRIPTION_LENGTH+1];
	int superclass_is_a_match, saved_errno;
	long i;
	mx_status_type mx_status;

	MX_DEBUG( 7, ("%s invoked.  Filename = '%s'", fname, filename));

	if ( record_superclass_list == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"record_superclass_list pointer passed is NULL." );
	}

	/* See if we can open the output file. */

	file = fopen( filename, "w" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot open MX output database file '%s'.  "
			"errno = %d, error message = '%s'.",
			filename, saved_errno, strerror( saved_errno ) );
	}


	/* Skip over the first record since it is the list head. */

	current_record = record_list->next_record;

	/* Walk through the record list until we reach the beginning again. */

	while ( current_record != record_list ) {

		/* Is this the right kind of record? */

		superclass_is_a_match = FALSE;

		for ( i = 0; i < num_record_superclasses; i++ ) {
			if ( current_record->mx_superclass
					== record_superclass_list[i] ) {

				superclass_is_a_match = TRUE;

				break;      /* Exit the for() loop. */
			}
		}

		if ( superclass_is_a_match == TRUE ) {

			mx_status = mx_create_description_from_record(
				current_record, buffer, sizeof buffer );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_error( MXE_FUNCTION_FAILED, fname,
			    "Couldn't format output string for record '%s'",
					current_record->name );

				/* Don't abort because of this!!! */

			} else {
				/* Write out the record description. */

				fprintf(file, "%s\n", buffer);
			}
		}

		current_record = current_record->next_record;
	}

	fclose(file);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_initialize_hardware( MX_RECORD *record_list_head, int inithw_flags )
{
	static const char fname[] = "mx_initialize_hardware()";

	MX_RECORD *current_record;
	mx_status_type mx_status;

	MX_DEBUG( 7,("%s invoked.", fname));

	if ( record_list_head == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"Record list head pointer is NULL.");
	}

	/* Is the first record the record list head? */

	if ( record_list_head->mx_superclass != MXR_LIST_HEAD ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Pointer passed to this function is not a record list head.");
	}

	/* Initialize the hardware. */

	current_record = record_list_head->next_record;

	while( current_record != record_list_head ) {
		if ( current_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			    "NULL pointer to record found in record list.");
		}

		if ( inithw_flags & MXF_INITHW_TRACE_OPENS ) {

			mx_info( "Opening record '%s'.", current_record->name );
		}

		mx_status = mx_open_hardware( current_record );

		if ( mx_status.code != MXE_SUCCESS ) {

			if ( inithw_flags & MXF_INITHW_ABORT_ON_FAULT ) {
				return mx_status;
			} else {
				current_record->record_flags |= MXF_REC_FAULTED;
			}
		}

		current_record = current_record->next_record;
	}

	/* A few drivers require extra initialization steps after
	 * almost everything else has been initialized.
	 */

	current_record = record_list_head->next_record;

	while( current_record != record_list_head ) {
		if ( current_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			    "NULL pointer to record found in record list.");
		}

		if ( (current_record->record_flags & MXF_REC_FAULTED) == 0 ) {

			mx_status = mx_finish_delayed_initialization(
							current_record );

			if ( mx_status.code != MXE_SUCCESS ) {

				if ( inithw_flags & MXF_INITHW_ABORT_ON_FAULT )
				{
					return mx_status;
				} else {
					current_record->record_flags
						|= MXF_REC_FAULTED;
				}
			}
		}
		current_record = current_record->next_record;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_shutdown_hardware( MX_RECORD *record_list_head )
{
	static const char fname[] = "mx_shutdown_hardware()";

	MX_RECORD *current_record;
	mx_status_type mx_status;

	MX_DEBUG( 7,
		("mx_shutdown_hardware() invoked.  record list head = 0x%p",
			record_list_head) );

	if ( record_list_head == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"Record list head pointer is NULL.");
	}

	/* Is the first record the record list head? */

	if ( record_list_head->mx_superclass != MXR_LIST_HEAD ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Pointer passed to this function is not a record list head.");
	}

	/* Loop through all the records.
	 *
	 * We go through the list in reverse order in the hope that this
	 * will cause things to be shut down in an appropriate order.
	 * Once device records have been changed so that forward references
	 * are allowed, we will have to revisit this decision.
	 */

	current_record = record_list_head->previous_record;

	while( current_record != record_list_head ) {

		if ( current_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			    "NULL pointer to record found in record list.");
		}

		MX_DEBUG(7, ("record name = '%s'", current_record->name) );

		mx_status = mx_close_hardware( current_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return mx_status;
		}

		current_record = current_record->previous_record;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ========= */

/*
 * mx_setup_database() is a simplified startup routine that performs
 * many of the standard startup actions for you.  For many programs,
 * calling mx_setup_database() should be all you need to do.
 */

MX_EXPORT mx_status_type
mx_setup_database( MX_RECORD **record_list, char *filename )
{
	static const char fname[] = "mx_setup_database()";

	mx_status_type mx_status;

	if ( record_list == (MX_RECORD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The record_list argument to this function was NULL." );
	}
	if ( filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename argument passed to this function is NULL." );
	}

	/* If we have a child process, like gnuplot, and the process
	 * dies before we expected, then any attempt to write to the
	 * pipe connecting us to the child will result in us being
	 * sent a SIGPIPE signal.  The default response under Unix
	 * to a SIGPIPE is to terminate the process.  This is
	 * undesirable for many MX programs, since it does not give 
	 * MX a chance to recover from the error.  Thus, we arrange to
	 * ignore any SIGPIPE signals sent our way and depend on
	 * 'errno' to let us know what really happened during a
	 * failed write.
	 */

#if defined( SIGPIPE )
	signal( SIGPIPE, SIG_IGN );
#endif

#if defined( _POSIX_REALTIME_SIGNALS ) && ( _POSIX_REALTIME_SIGNALS >= 0 )
	mx_status = mx_signal_initialize();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Set the MX debugging level to zero. */

	mx_set_debug_level(0);

	/* Initialize the subsecond sleep functions (if they need it). */

	mx_msleep(1);

	/* Initialize the MX time keeping functions. */

	mx_initialize_clock_ticks();

	/* Initialize the MX device drivers. */

	mx_status = mx_initialize_drivers();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create a new record list. */

	*record_list = mx_initialize_record_list();

	if ( *record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to create an MX record list." );
	}

	/* Read in the database and initialize the corresponding hardware. */

	mx_status = mx_read_database_file( *record_list, filename, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_finish_database_initialization( *record_list );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_initialize_hardware( *record_list, 0 );

	return mx_status;
}

/* ========= */

MX_EXPORT MX_LIST_HEAD *
mx_get_record_list_head_struct( MX_RECORD *record )
{
	static const char fname[] = "mx_get_record_list_head_struct()";

	MX_RECORD *list_head_record;
	MX_LIST_HEAD *list_head;

	if ( record == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );

		return NULL;
	}

	list_head_record = record->list_head;

	if ( list_head_record == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"record->list_head pointer is NULL for record '%s'",
			record->name );

		return NULL;
	}

	list_head = (MX_LIST_HEAD *)list_head_record->record_superclass_struct;

	if ( list_head == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LIST_HEAD pointer for record list head is NULL." );
	}
	return list_head;
}

MX_EXPORT mx_status_type
mx_set_event_interval( MX_RECORD *record, double event_interval_in_seconds )
{
	static const char fname[] = "mx_set_event_interval()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	if ( record->event_time_manager == (MX_EVENT_TIME_MANAGER *) NULL ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"The record '%s' is not configured for event time management.",
			record->name );
	}

	record->event_time_manager->event_interval =
		mx_convert_seconds_to_clock_ticks( event_interval_in_seconds );

	/* Set the next event time to the current time, so that the first
	 * event sent to the record can process the record immediately.
	 */

	record->event_time_manager->next_allowed_event_time
			= mx_current_clock_tick();

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mx_print_structure()";

	MX_RECORD_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( FILE *, MX_RECORD * );
	mx_status_type mx_status;

	MX_DEBUG(2, ("mx_print_structure() invoked for record '%s'",
		record->name) );

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD structure pointer is NULL.");
	}

	if ( file == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"FILE pointer is NULL.");
	}

	fl_ptr = (MX_RECORD_FUNCTION_LIST *)(record->record_function_list);

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RECORD_FUNCTION_LIST pointer is NULL." );
	}

	fptr = ( fl_ptr->print_structure );

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"print_structure function pointer is NULL.");
	}

	/* Invoke the function. */

	mx_status = (*fptr)( file, record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_read_parms_from_hardware( MX_RECORD *record )
{
	static const char fname[] = "mx_read_parms_from_hardware()";

	MX_RECORD_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RECORD * );
	mx_status_type mx_status;

	MX_DEBUG(7, ("mx_read_parms_from_hardware() invoked for record '%s'",
		record->name) );

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD structure pointer is NULL.");
	}

	fl_ptr = (MX_RECORD_FUNCTION_LIST *)(record->record_function_list);

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RECORD_FUNCTION_LIST pointer is NULL." );
	}

	fptr = ( fl_ptr->read_parms_from_hardware );

	if ( fptr == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = (*fptr)( record );

		return mx_status;
	}
}
	
MX_EXPORT mx_status_type
mx_open_hardware( MX_RECORD *record )
{
	static const char fname[] = "mx_open_hardware()";

	MX_RECORD_FUNCTION_LIST *fl_ptr;
	mx_status_type (*open_ptr)( MX_RECORD * );
	mx_status_type (*write_parms_ptr)( MX_RECORD * );
	mx_status_type mx_status;

	MX_DEBUG( 7, ("%s invoked for record '%s'", fname, record->name) );

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD structure pointer is NULL.");
	}

	fl_ptr = (MX_RECORD_FUNCTION_LIST *)(record->record_function_list);

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RECORD_FUNCTION_LIST pointer is NULL." );
	}

	open_ptr = ( fl_ptr->open );

	if ( open_ptr != NULL ) {

		mx_status = (*open_ptr)( record );

		if ( mx_status.code != MXE_SUCCESS ) {

			record->record_flags |= MXF_REC_FAULTED;

			return mx_status;
		}
	}

	write_parms_ptr = ( fl_ptr->write_parms_to_hardware );

	if ( write_parms_ptr != NULL ) {

		mx_status = (*write_parms_ptr)( record );

		if ( mx_status.code != MXE_SUCCESS ) {

			record->record_flags |= MXF_REC_FAULTED;

			return mx_status;
		}
	}
	MX_DEBUG( 7, ("Leaving %s for record '%s'", fname, record->name) );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_close_hardware( MX_RECORD *record )
{
	static const char fname[] = "mx_close_hardware()";

	MX_RECORD_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RECORD * );
	mx_status_type mx_status;

	MX_DEBUG(7, ("%s invoked for record '%s'", fname, record->name) );

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD structure pointer is NULL.");
	}

	fl_ptr = (MX_RECORD_FUNCTION_LIST *)(record->record_function_list);

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RECORD_FUNCTION_LIST pointer is NULL." );
	}

	fptr = ( fl_ptr->close );

	if ( fptr == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = (*fptr)( record );

		return mx_status;
	}
}

MX_EXPORT mx_status_type
mx_finish_delayed_initialization( MX_RECORD *record )
{
	static const char fname[] = "mx_finish_delayed_initialization()";

	MX_RECORD_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RECORD * );
	mx_status_type mx_status;

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD structure pointer is NULL.");
	}

	fl_ptr = (MX_RECORD_FUNCTION_LIST *)(record->record_function_list);

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RECORD_FUNCTION_LIST pointer is NULL." );
	}

	fptr = ( fl_ptr->finish_delayed_initialization );

	if ( fptr == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = (*fptr)( record );

		return mx_status;
	}
}

MX_EXPORT mx_status_type
mx_resynchronize_record( MX_RECORD *record )
{
	static const char fname[] = "mx_resynchronize_record()";

	MX_RECORD_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RECORD * );
	mx_status_type mx_status;

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD structure pointer is NULL.");
	}

	fl_ptr = (MX_RECORD_FUNCTION_LIST *)(record->record_function_list);

	if ( fl_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RECORD_FUNCTION_LIST pointer is NULL." );
	}

	fptr = ( fl_ptr->resynchronize );

	if ( fptr == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = (*fptr)( record );

		return mx_status;
	}
}

MX_EXPORT mx_status_type
mx_record_array_dependency_handler( MX_RECORD *record,
			MX_RECORD_FIELD *field,
			void *dependency_struct_ptr,
			long *array_indices,
			void *array_ptr,
			long dimension_level )
{
	static const char fname[] = "mx_record_array_dependency_handler()";

	MX_RECORD_ARRAY_DEPENDENCY_STRUCT *dependency_struct;
	MX_RECORD *dependent_record;
	MX_INTERFACE *interface;
	void *value_ptr;
	mx_status_type mx_status;

	if ( dimension_level > 0 ) {

		/* If we are not at the bottom level of the field array,
		 * just return.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG( 8,("%s invoked for record '%s', field '%s'",
		fname, record->name, field->name));

	switch( field->datatype ) {
	case MXFT_INTERFACE:
		interface = (MX_INTERFACE *) array_ptr;
		dependent_record = interface->record;
		break;
	case MXFT_RECORD:
		value_ptr = mx_read_void_pointer_from_memory_location(
								array_ptr );
		dependent_record = (MX_RECORD *) value_ptr;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"This function only supports MXFT_RECORD and MXFT_INTERFACE record_fields.  "
"The field type we were passed = %ld", field->datatype );
		break;
	}

	MX_DEBUG( 8,("%s: dependent_record = '%s'",
		fname, dependent_record->name));

	dependency_struct
		= (MX_RECORD_ARRAY_DEPENDENCY_STRUCT *) dependency_struct_ptr;

	MX_DEBUG( 8,("%s: add_dependency = %d, dependency_is_to_parent = %d",
		fname, dependency_struct->add_dependency,
		dependency_struct->dependency_is_to_parent));

	if ( dependency_struct->add_dependency ) {
		if ( dependency_struct->dependency_is_to_parent ) {

			mx_status = mx_add_parent_dependency( record, TRUE,
							dependent_record );
		} else {
			mx_status = mx_add_child_dependency( record, TRUE,
							dependent_record );
		}
	} else {
		if ( dependency_struct->dependency_is_to_parent ) {

			mx_status = mx_delete_parent_dependency( record, TRUE,
							dependent_record );
		} else {
			mx_status = mx_delete_child_dependency( record, TRUE,
							dependent_record );
		}
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mx_add_parent_dependency(MX_RECORD *current_record,
		int add_child_pointer_in_parent,
		MX_RECORD *parent_record )
{
	static const char fname[] = "mx_add_parent_dependency()";

	MX_RECORD **parent_record_array;
	MX_RECORD_FIELD *parent_record_array_field;
	long old_num_parent_records, new_num_parent_records;
	long old_num_parent_array_blocks, new_num_parent_array_blocks;
	long i, new_num_parent_array_elements;
	mx_status_type mx_status;

	if ( current_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"current_record pointer passed is NULL." );
	}

	if ( parent_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"parent_record pointer passed is NULL." );
	}

	/* If the current record doesn't support record fields, then there
	 * is no useful work we can do here.
	 */

	if ( current_record->num_record_fields <= 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_find_record_field( current_record, "parent_record_array",
					&parent_record_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* How many parent records are there currently? */

	old_num_parent_records = current_record->num_parent_records;

	if ( old_num_parent_records < 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Old number of parent records (%ld) was less than zero.  "
		"This should not happen.", old_num_parent_records );
	}

	/* We attempt to avoid calling realloc() too often. */

	old_num_parent_array_blocks = old_num_parent_records
					/ MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	if ( old_num_parent_records % MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH != 0 ){
		old_num_parent_array_blocks++;
	}

	new_num_parent_records = old_num_parent_records + 1;

	new_num_parent_array_blocks = new_num_parent_records
					/ MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	if ( new_num_parent_records % MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH != 0 ){
		new_num_parent_array_blocks++;
	}

	new_num_parent_array_elements = new_num_parent_array_blocks
					* MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	parent_record_array = current_record->parent_record_array;

	MX_DEBUG( 8,("%s: Adding parent record '%s' to record '%s'",
			fname, parent_record->name, current_record->name ));
	MX_DEBUG( 8,
	("%s: old_num_parent_records = %ld, new_num_parent_records = %ld",
		fname, old_num_parent_records, new_num_parent_records));
	MX_DEBUG( 8,
("%s: old_num_parent_array_blocks = %ld, new_num_parent_array_blocks = %ld",
	fname, old_num_parent_array_blocks, new_num_parent_array_blocks));

	if ( old_num_parent_array_blocks != new_num_parent_array_blocks ) {

		/* Attempt to make the parent record array bigger. */

		if ( parent_record_array == NULL ) {
			parent_record_array = ( MX_RECORD ** ) malloc(
					new_num_parent_array_elements
					* sizeof( MX_RECORD * ) );
		} else {
			parent_record_array = ( MX_RECORD ** ) realloc(
					parent_record_array,
					new_num_parent_array_elements
					* sizeof( MX_RECORD * ) );
		}

		if ( parent_record_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to increase the size of parent_record_array to %ld",
				new_num_parent_array_elements );
		}

		MX_DEBUG( 8,("%s: New size of parent record array = %ld",fname,
			new_num_parent_array_elements * sizeof(MX_RECORD *)));

		current_record->parent_record_array = parent_record_array;
	}

	parent_record_array[ old_num_parent_records ] = parent_record;

	MX_DEBUG( 8,("%s: current_record->parent_record_array = %p",
		fname, current_record->parent_record_array));
	MX_DEBUG( 8,("%s: parent_record_array[%ld] = %p",
		fname, old_num_parent_records,
		parent_record_array[old_num_parent_records]));

	if ( add_child_pointer_in_parent ) {
		mx_status = mx_add_child_dependency( parent_record, FALSE,
						current_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return mx_status;
		}
	}

	/* Fill the rest of the array with NULLs. */

	for (i = new_num_parent_records; i<new_num_parent_array_elements; i++){

		parent_record_array[i] = NULL;
	}

	current_record->num_parent_records = new_num_parent_records;

	parent_record_array_field->dimension[0] = new_num_parent_records;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_parent_dependency(MX_RECORD *current_record,
		int delete_child_pointer_in_parent,
		MX_RECORD *parent_record )
{
	static const char fname[] = "mx_delete_parent_dependency()";

	MX_RECORD **parent_record_array;
	MX_RECORD_FIELD *parent_record_array_field;
	long old_num_parent_records, new_num_parent_records;
	long old_num_parent_array_blocks, new_num_parent_array_blocks;
	long i, j, new_num_parent_array_elements;
	mx_status_type mx_status;

	if ( current_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"current_record pointer passed is NULL." );
	}

	if ( parent_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"parent_record pointer passed is NULL." );
	}

	/* If the current record doesn't support record fields, then there
	 * is no useful work we can do here.
	 */

	if ( current_record->num_record_fields <= 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_find_record_field( current_record, "parent_record_array",
					&parent_record_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* How many parent records are there currently? */

	old_num_parent_records = current_record->num_parent_records;

	if ( old_num_parent_records < 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Old number of parent records (%ld) was less than zero.  "
		"This should not happen.", old_num_parent_records );

	} else if ( old_num_parent_records == 0 ) {

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of parents for this record is already zero." );
	}

	/* We attempt to avoid calling realloc() too often. */

	old_num_parent_array_blocks = old_num_parent_records
					/ MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	if ( old_num_parent_records % MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH != 0 ){
		old_num_parent_array_blocks++;
	}

	new_num_parent_records = old_num_parent_records - 1;

	new_num_parent_array_blocks = new_num_parent_records
					/ MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	if ( new_num_parent_records % MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH != 0 ){
		new_num_parent_array_blocks++;
	}

	new_num_parent_array_elements = new_num_parent_array_blocks
					* MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	parent_record_array = current_record->parent_record_array;

	MX_DEBUG( 2,("%s: Deleting parent record '%s' from record '%s'",
			fname, parent_record->name, current_record->name ));

	/* Search for the record and then delete it. */

	for ( i = 0; i < old_num_parent_records; i++ ) {

		if ( parent_record_array[i] == parent_record ) {

			if ( delete_child_pointer_in_parent ) {
				mx_status = mx_delete_child_dependency(
					parent_record, FALSE, current_record );

				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
			}

			parent_record_array[i] = NULL;

			break;  /* Exit the for() loop over j */
		}
	}

	if ( i >= old_num_parent_records ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Record '%s' was not found in the list of parents of record '%s'.",
				parent_record->name, current_record->name );
	}

	/* Squeeze the NULLs out of the beginning of the array. */

	for ( i = 0, j = 0; j < old_num_parent_records; ) {

		if ( parent_record_array[j] != NULL ) {
			if ( i != j ) {
			    parent_record_array[i] = parent_record_array[j];
			}
			i++;
		}
		j++;
	}

	/* Fill the rest of the array with NULLs. */

	for ( ; i<new_num_parent_array_elements; i++){

		parent_record_array[i] = NULL;
	}

	current_record->num_parent_records = new_num_parent_records;

	if ( new_num_parent_records <= 0 ) {
		if ( current_record->parent_record_array != NULL ) {

			free( current_record->parent_record_array );

			current_record->parent_record_array = NULL;
		}
		return MX_SUCCESSFUL_RESULT;
	}

	if ( old_num_parent_array_blocks != new_num_parent_array_blocks ) {

		/* Attempt to make the parent record array smaller. */

		if ( parent_record_array == NULL ) {
			parent_record_array = ( MX_RECORD ** ) malloc(
					new_num_parent_array_elements
					* sizeof( MX_RECORD * ) );
		} else {
			parent_record_array = ( MX_RECORD ** ) realloc(
					parent_record_array,
					new_num_parent_array_elements
					* sizeof( MX_RECORD * ) );
		}

		if ( parent_record_array == NULL ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
	"Attempt to decrease the size of parent_record_array to %ld failed.",
				new_num_parent_array_elements );
		}

		current_record->parent_record_array = parent_record_array;
	}

	parent_record_array_field->dimension[0] = new_num_parent_records;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_add_child_dependency(MX_RECORD *current_record,
		int add_parent_pointer_in_child,
		MX_RECORD *child_record )
{
	static const char fname[] = "mx_add_child_dependency()";

	MX_RECORD **child_record_array;
	MX_RECORD_FIELD *child_record_array_field;
	long old_num_child_records, new_num_child_records;
	long old_num_child_array_blocks, new_num_child_array_blocks;
	long i, new_num_child_array_elements;
	mx_status_type mx_status;

	if ( current_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"current_record pointer passed is NULL." );
	}

	if ( child_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"child_record pointer passed is NULL." );
	}

	/* If the current record doesn't support record fields, then there
	 * is no useful work we can do here.
	 */

	if ( current_record->num_record_fields <= 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_find_record_field( current_record, "child_record_array",
					&child_record_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* How many child records are there currently? */

	old_num_child_records = current_record->num_child_records;

	if ( old_num_child_records < 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Old number of child records (%ld) was less than zero.  "
		"This should not happen.", old_num_child_records );
	}

	/* We attempt to avoid calling realloc() too often. */

	old_num_child_array_blocks = old_num_child_records
					/ MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	if ( old_num_child_records % MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH != 0 ){
		old_num_child_array_blocks++;
	}

	new_num_child_records = old_num_child_records + 1;

	new_num_child_array_blocks = new_num_child_records
					/ MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	if ( new_num_child_records % MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH != 0 ){
		new_num_child_array_blocks++;
	}

	new_num_child_array_elements = new_num_child_array_blocks
					* MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	child_record_array = current_record->child_record_array;

	MX_DEBUG( 8,("%s: Adding child record '%s' to record '%s'",
			fname, child_record->name, current_record->name ));

	MX_DEBUG( 8,
	("%s: old_num_child_records = %ld, new_num_child_records = %ld",
		fname, old_num_child_records, new_num_child_records));
	MX_DEBUG( 8,
("%s: old_num_child_array_blocks = %ld, new_num_child_array_blocks = %ld",
	fname, old_num_child_array_blocks, new_num_child_array_blocks));

	if ( old_num_child_array_blocks != new_num_child_array_blocks ) {

		/* Attempt to make the child record array bigger. */

		if ( child_record_array == NULL ) {
			child_record_array = ( MX_RECORD ** ) malloc(
					new_num_child_array_elements
					* sizeof( MX_RECORD * ) );
		} else {
			child_record_array = ( MX_RECORD ** ) realloc(
					child_record_array,
					new_num_child_array_elements
					* sizeof( MX_RECORD * ) );
		}

		if ( child_record_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to increase the size of child_record_array to %ld",
				new_num_child_array_elements );
		}

		current_record->child_record_array = child_record_array;
	}

	child_record_array[ old_num_child_records ] = child_record;

	if ( add_parent_pointer_in_child ) {
		mx_status = mx_add_parent_dependency( child_record, FALSE,
						current_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return mx_status;
		}
	}

	/* Fill the rest of the array with NULLs. */

	for (i = new_num_child_records; i<new_num_child_array_elements; i++){

		child_record_array[i] = NULL;
	}

	current_record->num_child_records = new_num_child_records;

	child_record_array_field->dimension[0] = new_num_child_records;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_child_dependency(MX_RECORD *current_record,
		int delete_parent_pointer_in_child,
		MX_RECORD *child_record )
{
	static const char fname[] = "mx_delete_child_dependency()";

	MX_RECORD **child_record_array;
	MX_RECORD_FIELD *child_record_array_field;
	long old_num_child_records, new_num_child_records;
	long old_num_child_array_blocks, new_num_child_array_blocks;
	long i, j, new_num_child_array_elements;
	mx_status_type mx_status;

	if ( current_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"current_record pointer passed is NULL." );
	}

	if ( child_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"child_record pointer passed is NULL." );
	}

	/* If the current record doesn't support record fields, then there
	 * is no useful work we can do here.
	 */

	if ( current_record->num_record_fields <= 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_find_record_field( current_record, "child_record_array",
					&child_record_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* How many child records are there currently? */

	old_num_child_records = current_record->num_child_records;

	if ( old_num_child_records < 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Old number of child records (%ld) was less than zero.  "
		"This should not happen.", old_num_child_records );

	} else if ( old_num_child_records == 0 ) {

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of children for this record is already zero." );
	}

	/* We attempt to avoid calling realloc() too often. */

	old_num_child_array_blocks = old_num_child_records
					/ MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	if ( old_num_child_records % MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH != 0 ){
		old_num_child_array_blocks++;
	}

	new_num_child_records = old_num_child_records - 1;

	new_num_child_array_blocks = new_num_child_records
					/ MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	if ( new_num_child_records % MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH != 0 ){
		new_num_child_array_blocks++;
	}

	new_num_child_array_elements = new_num_child_array_blocks
					* MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH;

	child_record_array = current_record->child_record_array;

	MX_DEBUG( 2,("%s: Deleting child record '%s' from record '%s'",
			fname, child_record->name, current_record->name ));

	/* Search for the record and then delete it. */

	for ( i = 0; i < old_num_child_records; i++ ) {

		if ( child_record_array[i] == child_record ) {

			if ( delete_parent_pointer_in_child ) {
				mx_status = mx_delete_parent_dependency(
						child_record, FALSE,
						current_record );

				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
			}

			child_record_array[i] = NULL;

			break;  /* Exit the for() loop over j */
		}
	}

	if ( i >= old_num_child_records ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Record '%s' was not found in the list of childs of record '%s'.",
				child_record->name, current_record->name );
	}

	/* Squeeze the NULLs out of the beginning of the array. */

	for ( i = 0, j = 0; j < old_num_child_records; ) {

		if ( child_record_array[j] != NULL ) {
			if ( i != j ) {
			    child_record_array[i] = child_record_array[j];
			}
			i++;
		}
		j++;
	}

	/* Fill the rest of the array with NULLs. */

	for ( ; i<new_num_child_array_elements; i++){

		child_record_array[i] = NULL;
	}

	current_record->num_child_records = new_num_child_records;

	if ( new_num_child_records <= 0 ) {
		if ( current_record->child_record_array != NULL ) {

			free( current_record->child_record_array );

			current_record->child_record_array = NULL;
		}
		return MX_SUCCESSFUL_RESULT;
	}

	if ( old_num_child_array_blocks != new_num_child_array_blocks ) {

		/* Attempt to make the child record array smaller. */

		if ( child_record_array == NULL ) {
			child_record_array = ( MX_RECORD ** ) malloc(
					new_num_child_array_elements
					* sizeof( MX_RECORD * ) );
		} else {
			child_record_array = ( MX_RECORD ** ) realloc(
					child_record_array,
					new_num_child_array_elements
					* sizeof( MX_RECORD * ) );
		}

		if ( child_record_array == NULL ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
	"Attempt to decrease the size of child_record_array to %ld failed.",
				new_num_child_array_elements );
		}

		current_record->child_record_array = child_record_array;
	}

	child_record_array_field->dimension[0] = new_num_child_records;

	return MX_SUCCESSFUL_RESULT;
}

/* The application_ptr fields in records, record fields and record list heads
 * are intended to be places where individual application programs can attach
 * structures that are specific to that particular application.  The main
 * libmx code leaves these fields strictly alone except for initializing them
 * to NULL when the record, record field or record list head is originally
 * created.  Note that since the data structure pointed to by these pointers
 * can be literally anything, it is not practical to define record fields
 * that point at application pointers.
 *
 * The following functions merely check to see if the application pointer
 * is in use before setting a new value.
 */

MX_EXPORT mx_status_type
mx_set_list_application_ptr( MX_RECORD *record_list, void *application_ptr )
{
	static const char fname[] = "mx_set_list_application_ptr()";

	MX_LIST_HEAD *list_head;

	/* Make sure we are really using the record list head. */

	record_list = record_list->list_head;

	/* Now check the pointer. */

	list_head = (MX_LIST_HEAD *) record_list->record_superclass_struct;

	if ( list_head->application_ptr != NULL ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"The record list head application pointer is already in use.  "
		"It currently points to %p", list_head->application_ptr );
	}

	list_head->application_ptr = application_ptr;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_set_record_application_ptr( MX_RECORD *record, void *application_ptr )
{
	static const char fname[] = "mx_set_record_application_ptr()";

	if ( record->application_ptr != NULL ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"The application pointer for record '%s' is already in use.  "
		"It currently points to %p", record->name,
			record->application_ptr );
	}

	record->application_ptr = application_ptr;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_set_field_application_ptr( MX_RECORD_FIELD *record_field,
				void *application_ptr )
{
	static const char fname[] = "mx_set_field_application_ptr()";

	if ( record_field->application_ptr != NULL ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"The application pointer for record field '%s' is already in use.  "
	"It currently points to %p", record_field->name,
			record_field->application_ptr );
	}

	record_field->application_ptr = application_ptr;

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/* Fast mode in an MX database is a hint to MX device drivers that they
 * may make certain optimizations in the name of speed that they would
 * not normally make.  This would most commonly be used by scans.
 *
 * The canonical example is the device driver for the Keithley 428
 * amplifier where fast mode is used to skip certain time consuming 
 * commands such as 'get gain' while a scan is in progress under the
 * assumption that no other process will change the gain while the
 * scan is in progress.
 */

MX_EXPORT mx_status_type
mx_get_fast_mode( MX_RECORD *record, int *mode_flag )
{
	static const char fname[] = "mx_get_fast_mode()";

	MX_LIST_HEAD *list_head;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}
	if ( mode_flag == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The mode_flag pointer passed was NULL." );
	}

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"mx_get_record_list_head_struct() returned a NULL pointer.  "
		"This should not be able to happen." );
	}

	if ( list_head->allow_fast_mode ) {
		*mode_flag = list_head->fast_mode;
	} else {
		*mode_flag = FALSE;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_set_fast_mode( MX_RECORD *record, int mode_flag )
{
	static const char fname[] = "mx_set_fast_mode()";

	MX_RECORD *list_head_record, *current_record;
	MX_LIST_HEAD *list_head;
	int fast_mode_flag;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	if ( mode_flag ) {
		mode_flag = TRUE;
	} else {
		mode_flag = FALSE;
	}

	/* Set the flag in the local database. */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"mx_get_record_list_head_struct() returned a NULL pointer.  "
		"This should not be able to happen." );
	}

	list_head->fast_mode = mode_flag;

	/* We also want to set the fast mode flag in all the servers this
	 * client is connected to.
	 */

	list_head_record = record->list_head;

	current_record = list_head_record->next_record;

	while ( current_record != list_head_record ) {

		if ( ( current_record->mx_superclass == MXR_SERVER )
		  && ( current_record->mx_class == MXN_NETWORK_SERVER ) )
		{
			fast_mode_flag = mode_flag;

			(void) mx_put_by_name( current_record,
					"mx_database.fast_mode",
					MXFT_INT, &fast_mode_flag );
		}

		current_record = current_record->next_record;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_set_program_name( MX_RECORD *record, char *program_name )
{
	static const char fname[] = "mx_set_program_name()";

	MX_RECORD *list_head_record, *current_record;
	MX_LIST_HEAD *list_head;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	/* Set the program name in the local database. */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"mx_get_record_list_head_struct() returned a NULL pointer.  "
		"This should not be able to happen." );
	}

	strlcpy( list_head->program_name, program_name,
				MXU_PROGRAM_NAME_LENGTH );

	/* We also want to send the program name to all the servers this
	 * client is connected to.
	 */

	list_head_record = record->list_head;

	current_record = list_head_record->next_record;

	while ( current_record != list_head_record ) {

		if ( ( current_record->mx_superclass == MXR_SERVER )
		  && ( current_record->mx_class == MXN_NETWORK_SERVER ) )
		{
			(void) mx_set_client_info( current_record,
						list_head->username,
						program_name );
		}

		current_record = current_record->next_record;
	}

	return MX_SUCCESSFUL_RESULT;
}


/*
 * Name:    mx_list.c
 *
 * Purpose: MX list handling functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_list.h"

MX_EXPORT mx_status_type
mx_list_create( MX_LIST **list )
{
	static const char fname[] = "mx_list_create()";

	if ( list == (MX_LIST **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LIST pointer passed was NULL." );
	}

	*list = malloc( sizeof(MX_LIST) );

	if ((*list) == (MX_LIST *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a new MX_LIST structure." );
	}

	(*list)->num_list_entries = 0;
	(*list)->list_start = NULL;
	(*list)->list_data = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_list_destroy( MX_LIST *list )
{
	MX_LIST_ENTRY *current_list_entry;
	MX_LIST_ENTRY *next_list_entry;
	void (*destructor)( MX_LIST_ENTRY * );

	if ( list == (MX_LIST *) NULL )
		return;

	next_list_entry = NULL;

	current_list_entry = list->list_start;

	while ( current_list_entry != (MX_LIST_ENTRY *) NULL ) {

		next_list_entry = current_list_entry->next_list_entry;

		destructor = current_list_entry->destructor;

		if ( destructor != NULL ) {
			(*destructor)( current_list_entry );
		}

		free( current_list_entry );

		current_list_entry = next_list_entry;
	}

	return;
}

MX_EXPORT mx_status_type
mx_list_add_entry( MX_LIST *list, MX_LIST_ENTRY *list_entry )
{
	static const char fname[] = "mx_list_add_entry()";

	MX_LIST_ENTRY *list_start, *end_of_list;

	if ( list == (MX_LIST *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LIST pointer passed was NULL." );
	}
	if ( list_entry == (MX_LIST_ENTRY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LIST_ENTRY pointer passed was NULL." );
	}

	list_entry->list = list;

	/* Find the right place to insert the list entry. */

	list_start = list->list_start;

	if ( list_start == (MX_LIST_ENTRY *) NULL ) {

		/* Currently, there are no list entries in the list, so
		 * we make this list entry the first entry in the list.
		 */

		/* Make the list entry point to itself. */

		list_entry->next_list_entry = list_entry;
		list_entry->previous_list_entry = list_entry;

		/* Insert the list entry as the start of the list. */

		list->list_start = list_entry;

		list->num_list_entries = 1;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, the list is not empty, so we are going to insert
	 * the new list entry at the end of the list.
	 */

	end_of_list = list_start->previous_list_entry;

	/* Update the list entry itself. */

	list_entry->next_list_entry = list_start;
	list_entry->previous_list_entry = end_of_list;

	/* Insert the list entry into the list. */

	end_of_list->next_list_entry = list_entry;
	list_start->previous_list_entry = list_entry;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_list_delete_entry( MX_LIST *list, MX_LIST_ENTRY *list_entry )
{
	static const char fname[] = "mx_list_delete_entry()";

	MX_LIST_ENTRY *previous_list_entry, *next_list_entry;

	if ( list == (MX_LIST *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LIST pointer passed was NULL." );
	}
	if ( list_entry == (MX_LIST_ENTRY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LIST_ENTRY pointer passed was NULL." );
	}

	previous_list_entry = list_entry->previous_list_entry;
	next_list_entry     = list_entry->next_list_entry;

	/* If the list entry is the list_start entry, then we must
	 * take special action.
	 */

	if ( list_entry == list->list_start ) {
		if ( list_entry == next_list_entry ) {
			/* If the list_start entry points to itself, then
			 * deleting this entry leaves us with an empty list.
			 */

			list->list_start = NULL;
		} else {
			/* Otherwise, redirect the list_start pointer to
			 * the next entry.
			 */

			list->list_start = next_list_entry;
		}
	}

	/* Remove the list entry from the list. */

	next_list_entry->previous_list_entry = previous_list_entry;
	previous_list_entry->next_list_entry = next_list_entry;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_list_entry_create( MX_LIST_ENTRY **list_entry,
			void *list_entry_data,
			void (*destructor)( MX_LIST_ENTRY * ) )
{
	static const char fname[] = "mx_list_entry_create()";

	if ( list_entry == (MX_LIST_ENTRY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LIST_ENTRY pointer passed was NULL." );
	}

	*list_entry = malloc( sizeof(MX_LIST_ENTRY) );

	if ( (*list_entry) == (MX_LIST_ENTRY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_LIST_ENTRY structure." );
	}

	(*list_entry)->list_entry_data = list_entry_data;
	(*list_entry)->destructor      = destructor;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_list_entry_destroy( MX_LIST_ENTRY *list_entry )
{
	void (*destructor)( MX_LIST_ENTRY * );

	if ( list_entry == (MX_LIST_ENTRY *) NULL )
		return;

	/* If there is a destructor, invoke it. */

	destructor = list_entry->destructor;

	if ( destructor != NULL ) {
		(*destructor)( list_entry );
	}

	/* Free the list entry structure itself. */

	free( list_entry );

	return;
}

MX_EXPORT mx_status_type
mx_list_traverse( MX_LIST *list,
		mx_status_type (*function)( MX_LIST_ENTRY *, void * ),
		void *argument )
{
	static const char fname[] = "mx_list_traverse()";

	MX_LIST_ENTRY *list_start, *current_list_entry, *next_list_entry;
	mx_status_type mx_status;

	if ( list == (MX_LIST *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LIST pointer passed was NULL." );
	}
	if ( function == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The function pointer passed was NULL." );
	}

	list_start = list->list_start;

	/* If the list is empty, do nothing. */

	if ( list_start == (MX_LIST_ENTRY *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Walk through the list. */

	current_list_entry = list_start;

	do {
		mx_status = (*function)( current_list_entry, argument );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		next_list_entry = current_list_entry->next_list_entry;

		if ( next_list_entry == (MX_LIST_ENTRY *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The next_list_entry pointer for list entry %p is NULL.",
		    		current_list_entry );
		}

		current_list_entry = next_list_entry;

	} while ( current_list_entry != list_start );

	return MX_SUCCESSFUL_RESULT;
}


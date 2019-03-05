/*
 * Name:    mx_dictionary.c
 *
 * Purpose: MX dictionaries are a key-value database where the keys are
 *          found in doubly-linked list.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_DICTIONARY_DEBUG			FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_cfn.h"
#include "mx_array.h"
#include "mx_dictionary.h"

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_dictionary_create( MX_DICTIONARY **new_dictionary,
			const char *dictionary_name,
			MX_RECORD *record )
{
	static const char fname[] = "mx_dictionary_create()";

	if ( new_dictionary == (MX_DICTIONARY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY pointer passed was NULL." );
	}

	*new_dictionary = (MX_DICTIONARY *) malloc( sizeof(MX_DICTIONARY) );

	if ( *new_dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt to create a new MX_DICTIONARY structure failed." );
	}

	strlcpy( (*new_dictionary)->name,
		dictionary_name,
		sizeof( (*new_dictionary)->name ) );

	(*new_dictionary)->num_dictionary_entries = 0;

	(*new_dictionary)->dictionary_start = NULL;

	(*new_dictionary)->record = record;
	(*new_dictionary)->application_ptr = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT void
mx_dictionary_destroy( MX_DICTIONARY *dictionary )
{
#if MX_DICTIONARY_DEBUG
	static const char fname[] = "mx_dictionary_destroy()";
#endif
	MX_DICTIONARY_ENTRY *current_dictionary_entry;
	MX_DICTIONARY_ENTRY *next_dictionary_entry;
	void (*destructor)( void * );

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: destroying dictionary %p.", fname, dictionary));
#endif

	if ( dictionary == (MX_DICTIONARY *) NULL )
		return;

	next_dictionary_entry = NULL;

	current_dictionary_entry = dictionary->dictionary_start;

	while ( current_dictionary_entry != (MX_DICTIONARY_ENTRY *) NULL ) {

		next_dictionary_entry =
			current_dictionary_entry->next_dictionary_entry;

		destructor = current_dictionary_entry->destructor;

		if ( destructor != NULL ) {
			(*destructor)( current_dictionary_entry->value );
		}

		free( current_dictionary_entry );

		current_dictionary_entry = next_dictionary_entry;
	}

	return;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_dictionary_add_entry( MX_DICTIONARY *dictionary,
			MX_DICTIONARY_ENTRY *dictionary_entry )
{
	static const char fname[] = "mx_dictionary_add_entry()";

	MX_DICTIONARY_ENTRY *dictionary_start, *end_of_dictionary;

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: Adding %p to dictionary %p",
		fname, dictionary_entry, dictionary ));
#endif

	if ( dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY pointer passed was NULL." );
	}
	if ( dictionary_entry == (MX_DICTIONARY_ENTRY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY_ENTRY pointer passed was NULL." );
	}

	dictionary_entry->dictionary = dictionary;

	/* Find the right place to insert the dictionary entry. */

	dictionary_start = dictionary->dictionary_start;

	if ( dictionary_start == (MX_DICTIONARY_ENTRY *) NULL ) {

		/* Currently, there are no entries in the dictionary, so we
		 * make this dictionary entry the first entry in the dictionary.
		 */

		/* Make the dictionary entry point to itself. */

		dictionary_entry->next_dictionary_entry = dictionary_entry;
		dictionary_entry->previous_dictionary_entry = dictionary_entry;

		/* Insert the dictionary entry as the start of the dictionary.*/

		dictionary->dictionary_start = dictionary_entry;

		dictionary->num_dictionary_entries = 1;

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,
		("%s: #1 dictionary %p, num_dictionary_entries = %lu",
			fname, dictionary, dictionary->num_dictionary_entries));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, the dictionary is not empty, so we are going to
	 * insert the new dictionary entry at the end of the dictionary.
	 */

	end_of_dictionary = dictionary_start->previous_dictionary_entry;

	/* Update the dictionary entry itself. */

	dictionary_entry->next_dictionary_entry = dictionary_start;
	dictionary_entry->previous_dictionary_entry = end_of_dictionary;

	/* Insert the dictionary entry into the dictionary. */

	end_of_dictionary->next_dictionary_entry = dictionary_entry;
	dictionary_start->previous_dictionary_entry = dictionary_entry;

	dictionary->num_dictionary_entries++;

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: #2 dictionary %p, num_dictionary_entries = %lu",
		fname, dictionary, dictionary->num_dictionary_entries));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_dictionary_delete_entry( MX_DICTIONARY *dictionary,
			MX_DICTIONARY_ENTRY *dictionary_entry )
{
	static const char fname[] = "mx_dictionary_delete_entry()";

	MX_DICTIONARY_ENTRY *previous_dictionary_entry, *next_dictionary_entry;

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: Deleting %p from dictionary %p",
		fname, dictionary_entry, dictionary ));
#endif

	if ( dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY pointer passed was NULL." );
	}
	if ( dictionary_entry == (MX_DICTIONARY_ENTRY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY_ENTRY pointer passed was NULL." );
	}

	if ( dictionary->dictionary_start == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Cannot delete dictionary entry %p from empty dictionary %p.",
			dictionary_entry, dictionary );
	}

	previous_dictionary_entry = dictionary_entry->previous_dictionary_entry;
	next_dictionary_entry     = dictionary_entry->next_dictionary_entry;

	/* If the dictionary entry is the dictionary_start entry, then we must
	 * take special action.
	 */

	if ( dictionary_entry == dictionary->dictionary_start ) {
		if ( dictionary_entry == next_dictionary_entry ) {
			/* If the dictionary_start entry points to itself,
			 * then deleting this entry leaves us with an empty
			 * dictionary.
			 */

			dictionary->dictionary_start = NULL;
		} else {
			/* Otherwise, redirect the dictionary_start pointer to
			 * the next entry.
			 */

			dictionary->dictionary_start = next_dictionary_entry;
		}
	}

	/* Remove the dictionary entry from the dictionary. */

	next_dictionary_entry->previous_dictionary_entry
					= previous_dictionary_entry;

	previous_dictionary_entry->next_dictionary_entry
					= next_dictionary_entry;

	dictionary->num_dictionary_entries--;

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: dictionary %p, num_dictionary_entries = %lu",
		fname, dictionary, dictionary->num_dictionary_entries));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_dictionary_add_entry_from_description( MX_DICTIONARY *dictionary,
					char *entry_key,
					long mx_datatype,
					long num_dimensions,
					long *dimension_array,
					char *entry_description )
{
	static const char fname[] =
		"mx_dictionary_add_entry_from_description()";

	MX_DICTIONARY_ENTRY *dictionary_entry = NULL;
	mx_status_type mx_status;

	if ( dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY pointer passed was NULL." );
	}

	mx_status = mx_dictionary_entry_create_from_description(
						&dictionary_entry,
						entry_key,
						mx_datatype,
						num_dimensions,
						dimension_array,
						entry_description );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dictionary_entry->dictionary = dictionary;

	mx_status = mx_dictionary_add_entry( dictionary, dictionary_entry );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_dictionary_entry_create_from_description(
				MX_DICTIONARY_ENTRY **dictionary_entry,
				char *entry_key,
				long mx_datatype,
				long num_dimensions,
				long *dimension_array,
				char *entry_description )
{
	static const char fname[] =
		"mx_dictionary_entry_create_from_description()";

	MX_DICTIONARY_ENTRY *dictionary_entry_ptr = NULL;
	MX_RECORD_FIELD local_temp_record_field;
	size_t data_element_size[MXU_FIELD_MAX_DIMENSIONS];
	mx_status_type mx_status;

	if ( dictionary_entry == (MX_DICTIONARY_ENTRY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY_ENTRY pointer passed was NULL." );
	}
	if ( entry_key == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The entry_key pointer passed was NULL." );
	}
	if ( dimension_array == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dimension_array pointer passed was NULL." );
	}
	if ( entry_description == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The entry_description pointer passed was NULL." );
	}

	/* Create the new MX_DICTIONARY_ENTRY structure. */

	dictionary_entry_ptr = (MX_DICTIONARY_ENTRY *)
				calloc( 1, sizeof(MX_DICTIONARY_ENTRY) );

	if ( dictionary_entry_ptr == (MX_DICTIONARY_ENTRY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_DICTIONARY_ENTRY structure." );
	}

	strlcpy( dictionary_entry_ptr->key, entry_key,
			sizeof(dictionary_entry_ptr->key) );

	/* Setting the first element of data_element_size to be zero causes
	 * mx_construct_temp_record_field() to use a default data element
	 * size array.
	 */

	data_element_size[0] = 0L;

	/* Setup a temporary record field to manage the parsing of
	 * data from the entry_description string.
	 */

	mx_status = mx_initialize_temp_record_field( &local_temp_record_field,
			mx_datatype, num_dimensions, dimension_array,
			data_element_size, &(dictionary_entry_ptr->value) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*dictionary_entry = dictionary_entry_ptr;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/


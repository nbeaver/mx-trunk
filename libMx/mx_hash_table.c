/*
 * Name:    mx_hash_table.c
 *
 * Purpose: Support for MX hash tables.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_hash_table.h"

static long
mx_default_hash_table_function( MX_HASH_TABLE *hash_table, const char *key )
{
	static const char fname[] = "mx_default_hash_table_function()";

	long i, max_key_length, hash;
	unsigned long sum;
	char c;

	if ( hash_table == (MX_HASH_TABLE *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HASH_TABLE pointer passed was NULL." );

		return -1;
	}

	if ( key == (const char *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The key pointer passed was NULL." );

		return -1;
	}

	/* Compute the sum of all of the characters in the key. */

	max_key_length = hash_table->key_length;

	sum = 0;

	for ( i = 0; ; i++ ) {
		if ( i >= max_key_length ) {
			break;
		}

		c = key[i];

		if ( c == '\0' ) {
			break;
		}

		sum = sum + c;
	}

	hash = (long) ( sum % (hash_table->table_size) );

	return hash;
}

MX_EXPORT mx_status_type
mx_hash_table_create( MX_HASH_TABLE **hash_table,
			long key_length,
			long table_size,
			long (*hash_function)(MX_HASH_TABLE *, const char *) )
{
	static const char fname[] = "mx_hash_table_create()";

	if ( hash_table == (MX_HASH_TABLE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HASH_TABLE pointer passed was NULL." );
	}

	*hash_table = malloc( sizeof(MX_HASH_TABLE) );

	if ( *hash_table == (MX_HASH_TABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	   "Ran out of memory trying to allocate an MX_HASH_TABLE structure." );
	}

	(*hash_table)->key_length = key_length;
	(*hash_table)->table_size = table_size;

	(*hash_table)->array = calloc(table_size, sizeof(MX_KEY_VALUE_PAIR *));

	if ( (*hash_table)->array == (MX_KEY_VALUE_PAIR **) NULL ) {
		mx_free( *hash_table );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element "
		"table of MX_KEY_VALUE_PAIR structures.", table_size );
	}

	if ( hash_function == NULL ) {
		(*hash_table)->hash_function = mx_default_hash_table_function;
	} else {
		(*hash_table)->hash_function = hash_function;
	}

	return MX_SUCCESSFUL_RESULT;
}

static void
mx_hash_table_destroy_hash_list( MX_KEY_VALUE_PAIR *list_start )
{
	MX_KEY_VALUE_PAIR *current_list_entry;
	MX_KEY_VALUE_PAIR *next_list_entry;

	current_list_entry = list_start;

	while ( current_list_entry != NULL ) {

		next_list_entry = current_list_entry->next_list_entry;

		mx_free( current_list_entry->key );
		mx_free( current_list_entry );

		current_list_entry = next_list_entry;
	}

	return;
}

MX_EXPORT void
mx_hash_table_destroy( MX_HASH_TABLE *hash_table )
{
	static const char fname[] = "mx_hash_table_destroy()";

	MX_KEY_VALUE_PAIR *key_value_pair;
	long i;
	
	if ( hash_table == (MX_HASH_TABLE *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HASH_TABLE pointer passed was NULL." );
		return;
	}

	for ( i = 0; i < hash_table->table_size; i++ ) {
		key_value_pair = hash_table->array[i];

		if ( key_value_pair != (MX_KEY_VALUE_PAIR *) NULL ) {
			mx_hash_table_destroy_hash_list( key_value_pair );
		}
	}

	mx_free( hash_table );

	return;
}

MX_EXPORT mx_status_type
mx_hash_table_insert_key( MX_HASH_TABLE *hash_table,
			const char *key,
			void *value )
{
	static const char fname[] = "mx_hash_table_insert_key()";

	MX_KEY_VALUE_PAIR *list_entry, *new_list_entry;
	MX_KEY_VALUE_PAIR **new_list_entry_ptr = NULL;
	long hash;

	if ( hash_table == (MX_HASH_TABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HASH_TABLE pointer passed was NULL." );
	}
	if ( key == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The key pointer passed was NULL." );
	}

	/* Compute the hash for this key. */

	hash = hash_table->hash_function( hash_table, key );

	if ( hash < 0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The hash %ld returned for key '%s' by function %p "
		"for hash table %p is less than zero.",
			hash, key, hash_table->hash_function, hash_table );
	} else
	if ( hash >= hash_table->table_size ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The hash %ld returned for key '%s' by function %p "
		"for hash table %p is greater than the maximum value of %ld.",
			hash, key, hash_table->hash_function, hash_table,
			hash_table->table_size );
	}

	/* Find the correct place to insert the new key. */

	if ( hash_table->array[hash] == NULL ) {
		new_list_entry_ptr = &(hash_table->array[hash]);
	} else {
		list_entry = hash_table->array[hash];

		while( list_entry != NULL ) {

			if ( strcmp( key, list_entry->key ) == 0 ) {
				/* This key is already present, so we just
				 * replace its value with the new value.
				 */

				list_entry->value = value;

				return MX_SUCCESSFUL_RESULT;
			}

			if ( list_entry->next_list_entry == NULL ) {
				new_list_entry_ptr =
					&(list_entry->next_list_entry);

				break;	/* Exit the while() loop. */
			}

			list_entry = list_entry->next_list_entry;
		}
	}

	/* Create a new MX_KEY_VALUE_PAIR struct. */

	new_list_entry = malloc( sizeof(MX_KEY_VALUE_PAIR) );

	if ( new_list_entry == (MX_KEY_VALUE_PAIR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to create a new MX_KEY_VALUE_PAIR "
		"for key '%s'.", key );
	}

	new_list_entry->key = malloc( hash_table->key_length );

	if ( new_list_entry->key == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to copy key '%s'.", key );
	}

	strlcpy( new_list_entry->key, key, hash_table->key_length );

	new_list_entry->value = value;
	new_list_entry->next_list_entry = NULL;

	*new_list_entry_ptr = new_list_entry;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_hash_table_delete_key( MX_HASH_TABLE *hash_table,
			const char *key )
{
	static const char fname[] = "mx_hash_table_delete_key()";

	MX_KEY_VALUE_PAIR *first_list_entry;
	MX_KEY_VALUE_PAIR *previous_list_entry, *current_list_entry;
	long hash;

	if ( hash_table == (MX_HASH_TABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HASH_TABLE pointer passed was NULL." );
	}
	if ( key == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The key pointer passed was NULL." );
	}

	/* Compute the hash for this key. */

	hash = hash_table->hash_function( hash_table, key );

	if ( hash < 0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The hash %ld returned for key '%s' by function %p "
		"for hash table %p is less than zero.",
			hash, key, hash_table->hash_function, hash_table );
	} else
	if ( hash >= hash_table->table_size ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The hash %ld returned for key '%s' by function %p "
		"for hash table %p is greater than the maximum value of %ld.",
			hash, key, hash_table->hash_function, hash_table,
			hash_table->table_size );
	}

	/* If there is no list for this hash, then return MXE_NOT_FOUND. */

	if ( hash_table->array[hash] == (MX_KEY_VALUE_PAIR *) NULL ) {
		return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
		"Key '%s' was not found in the hash table.", key );
	}

	/* Does this key match the first entry in the list? */

	first_list_entry = hash_table->array[hash];

	if ( strcmp( key, first_list_entry->key ) == 0 ) {

		/* If so, then delete it. */

		hash_table->array[hash] = first_list_entry->next_list_entry;

		mx_free( first_list_entry->key );
		mx_free( first_list_entry );

		return MX_SUCCESSFUL_RESULT;
	}

	/* Look through the rest of the list. */

	previous_list_entry = first_list_entry;

	current_list_entry = previous_list_entry->next_list_entry;

	while(1) {
		if ( strcmp( key, current_list_entry->key ) == 0 ) {

			/* This is the matching list entry, so delete it. */

			previous_list_entry->next_list_entry
				= current_list_entry->next_list_entry;

			mx_free( current_list_entry->key );
			mx_free( current_list_entry );

			return MX_SUCCESSFUL_RESULT;
		}

		previous_list_entry = current_list_entry;

		current_list_entry = current_list_entry->next_list_entry;

		if ( current_list_entry == (MX_KEY_VALUE_PAIR *) NULL ) {
			return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
			"Key '%s' was not found in the hash table.", key );
		}
	}

#if defined(OS_SOLARIS)
	/* Do nothing */
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mx_hash_table_lookup_key( MX_HASH_TABLE *hash_table,
			const char *key,
			void **value )
{
	static const char fname[] = "mx_hash_table_lookup_key()";

	MX_KEY_VALUE_PAIR *list_entry;
	long hash;

	if ( hash_table == (MX_HASH_TABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HASH_TABLE pointer passed was NULL." );
	}
	if ( key == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The key pointer passed was NULL." );
	}

	/* Compute the hash for this key. */

	hash = hash_table->hash_function( hash_table, key );

	if ( hash < 0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The hash %ld returned for key '%s' by function %p "
		"for hash table %p is less than zero.",
			hash, key, hash_table->hash_function, hash_table );
	} else
	if ( hash >= hash_table->table_size ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The hash %ld returned for key '%s' by function %p "
		"for hash table %p is greater than the maximum value of %ld.",
			hash, key, hash_table->hash_function, hash_table,
			hash_table->table_size );
	}

	/* If there is no list for this hash, then return MXE_NOT_FOUND. */

	if ( hash_table->array[hash] == (MX_KEY_VALUE_PAIR *) NULL ) {
		return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
		"Key '%s' was not found in the hash table.", key );
	}

	/* Look for the key in the list. */

	list_entry = hash_table->array[hash];

	while(1) {
		if ( strcmp( key, list_entry->key ) == 0 ) {

			/* This is the matching list entry,
			 * so return the value for it.
			 */

			*value = list_entry->value;

			return MX_SUCCESSFUL_RESULT;
		}

		list_entry = list_entry->next_list_entry;

		if ( list_entry == (MX_KEY_VALUE_PAIR *) NULL ) {
			return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
			"Key '%s' was not found in the hash table.", key );
		}
	}

#if defined(OS_SOLARIS)
	/* Do nothing */
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}


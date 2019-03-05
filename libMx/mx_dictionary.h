/*
 * Name:    mx_dictionary.h
 *
 * Purpose: MX dictionaries are a key-value database where the keys are
 *          found in a doubly-linked list.
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

#ifndef _MX_DICTIONARY_H_
#define _MX_DICTIONARY_H_

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_DICTIONARY_NAME_LENGTH	40
#define MXU_DICTIONARY_KEY_LENGTH	200

struct mx_dictionary_entry_type {
	struct mx_dictionary_type *dictionary;

	struct mx_dictionary_entry_type *next_dictionary_entry;
	struct mx_dictionary_entry_type *previous_dictionary_entry;

	char key[MXU_DICTIONARY_KEY_LENGTH+1];
	void *value;

	void (*destructor)( void * );
};

struct mx_dictionary_type {
	char name[MXU_DICTIONARY_NAME_LENGTH+1];

	unsigned long num_dictionary_entries;

	struct mx_dictionary_entry_type *dictionary_start;

	MX_RECORD *record;

	void *application_ptr;
};

typedef struct mx_dictionary_entry_type MX_DICTIONARY_ENTRY;
typedef struct mx_dictionary_type	MX_DICTIONARY;

/*---*/

MX_API mx_status_type mx_dictionary_create( MX_DICTIONARY **new_dictionary,
						const char *dictionary_name,
						MX_RECORD *record );

MX_API void mx_dictionary_destroy( MX_DICTIONARY *dictionary );

MX_API mx_status_type mx_dictionary_add_entry( MX_DICTIONARY *dictionary,
					MX_DICTIONARY_ENTRY *dictionary_entry );

MX_API mx_status_type mx_dictionary_delete_entry( MX_DICTIONARY *dictionary,
					MX_DICTIONARY_ENTRY *dictionary_entry );

MX_API mx_status_type mx_dictionary_add_entry_from_description(
					MX_DICTIONARY *dictionary,
					char *entry_key,
					long mx_datatype,
					long num_dimensions,
					long *dimension_array,
					char *entry_description );
					
MX_API mx_status_type mx_dictionary_entry_create_from_description(
					MX_DICTIONARY_ENTRY **dictionary_entry,
					char *entry_key,
					long mx_datatype,
					long num_dimensions,
					long *dimension_array,
					char *entry_description );
					
#ifdef __cplusplus
}
#endif

#endif /* _MX_DICTIONARY_H_ */


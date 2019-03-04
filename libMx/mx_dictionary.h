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

typedef struct {
	char name[MXU_DICTIONARY_NAME_LENGTH+1];

	struct mx_dictionary_element *head_element;

	MX_RECORD *record;

	void *application_ptr;
} MX_DICTIONARY;

typedef struct mx_dictionary_element {
	MX_DICTIONARY *dictionary;
	struct mx_dictionary_element *next;
	struct mx_dictionary_element *previous;

	char key[MXU_DICTIONARY_KEY_LENGTH+1];
	void *value;
} MX_DICTIONARY_ELEMENT;


/*---*/

MX_API mx_status_type mx_dictionary_create( MX_DICTIONARY **new_dictionary,
						const char *dictionary_name,
						MX_RECORD *record );

MX_API mx_status_type mx_dictionary_show_dictionary(
					MX_DICTIONARY *dictionary );

MX_API mx_status_type mx_dictionary_read_file( MX_DICTIONARY *dictionary,
					const char *dictionary_filename );

#ifdef __cplusplus
}
#endif

#endif /* _MX_DICTIONARY_H_ */


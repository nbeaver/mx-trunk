/*
 * Name:    mx_dictionary.h
 *
 * Purpose: Support for an MX record superclass that provides support
 *          for key/value pairs as used in dictionary, associative array,
 *          map, symbol table, etc. data structures.  The dictionary is
 *          referred to by its MX record name, while the keys are MX
 *          record fields in the record.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
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

typedef struct {
	MX_RECORD *record;

	void *application_ptr;
} MX_DICTIONARY;

/*---*/

typedef struct {
	mx_status_type ( *create_key ) ( MX_DICTIONARY *dictionary );
	mx_status_type ( *delete_key ) ( MX_DICTIONARY *dictionary );
} MX_DICTIONARY_FUNCTION_LIST;

#ifdef __cplusplus
}
#endif

#endif /* _MX_DICTIONARY_H_ */


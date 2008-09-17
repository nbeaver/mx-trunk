/*
 * Name:    mx_list.h
 *
 * Purpose: Header for MX list handling functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2007-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_LIST_H__
#define __MX_LIST_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

struct mx_list_entry_type {
	struct mx_list_type *list;

	struct mx_list_entry_type *next_list_entry;
	struct mx_list_entry_type *previous_list_entry;

	void *list_entry_data;
	void (*destructor)( struct mx_list_entry_type * );
};

struct mx_list_type {
	unsigned long num_list_entries;

	struct mx_list_entry_type *list_start;

	void *list_data;
};

typedef struct mx_list_entry_type MX_LIST_ENTRY;
typedef struct mx_list_type       MX_LIST;

MX_API mx_status_type mx_list_create( MX_LIST **list );

MX_API void           mx_list_destroy( MX_LIST *list );

MX_API mx_status_type mx_list_add_entry( MX_LIST *list,
					MX_LIST_ENTRY *list_entry );

MX_API mx_status_type mx_list_delete_entry( MX_LIST *list,
					MX_LIST_ENTRY *list_entry );

MX_API mx_status_type mx_list_entry_create( MX_LIST_ENTRY **,
						void *list_entry_data,
						void (*)( MX_LIST_ENTRY * ) );

MX_API void           mx_list_entry_destroy( MX_LIST_ENTRY * );

MX_API mx_status_type mx_list_traverse(
		MX_LIST *list,
		mx_status_type (*function)(MX_LIST_ENTRY *, void *, void **),
		void *input_argument,
		void **output_argument );

MX_API mx_status_type mx_list_find_list_entry( MX_LIST *list,
					void *list_entry_data,
					MX_LIST_ENTRY **list_entry );
			
#ifdef __cplusplus
}
#endif

#endif /* __MX_LIST_H__ */


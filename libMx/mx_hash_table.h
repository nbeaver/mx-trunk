/*
 * Name:    mx_hash_table.h
 *
 * Purpose: Support for MX hash tables.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_HASH_TABLE_H__
#define __MX_HASH_TABLE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mx_key_value_pair_t {
	char *key;
	void *value;
	struct mx_key_value_pair_t *next_list_entry;
} MX_KEY_VALUE_PAIR;

typedef struct mx_hash_table_t {
	long key_length;
	long table_size;
	MX_KEY_VALUE_PAIR **array;
	long (*hash_function)( struct mx_hash_table_t *, const char * );
} MX_HASH_TABLE;

MX_API mx_status_type mx_hash_table_create( MX_HASH_TABLE **hash_table,
				long key_length,
				long table_size,
			long (*hash_function)(MX_HASH_TABLE *, const char *)
				);

MX_API void mx_hash_table_destroy( MX_HASH_TABLE *hash_table );

MX_API mx_status_type mx_hash_table_insert_key( MX_HASH_TABLE *hash_table,
						const char *key, void *value );

MX_API mx_status_type mx_hash_table_delete_key( MX_HASH_TABLE *hash_table,
						const char *key );

MX_API mx_status_type mx_hash_table_lookup_key( MX_HASH_TABLE *hash_table,
						const char *key, void **value );


#ifdef __cplusplus
}
#endif

#endif /* __MX_HASH_TABLE_H__ */


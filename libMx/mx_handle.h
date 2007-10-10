/*
 * Name:    mx_handle.h
 *
 * Purpose: Support for MX handle tables.  MX handles are integer numbers
 *          that are used as indices into a table of C pointers.  This
 *          feature exists so that programming languages that cannot directly
 *          manipulate MX objects using C pointers can refer to the objects
 *          via handle numbers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_HANDLE_H__
#define __MX_HANDLE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	signed long handle;
	void *pointer;
} MX_HANDLE_STRUCT;

typedef struct {
	unsigned long handles_in_use;
	unsigned long block_size;
	unsigned long num_blocks;
	MX_HANDLE_STRUCT *handle_struct_array;
} MX_HANDLE_TABLE;

#define MX_ILLEGAL_HANDLE	(-1L)

MX_API mx_status_type mx_create_handle_table(
					MX_HANDLE_TABLE **handle_table,
					unsigned long block_size,
					unsigned long num_blocks );

MX_API mx_status_type mx_resize_handle_table(
					MX_HANDLE_TABLE *handle_table,
					unsigned long new_num_blocks );

MX_API void mx_delete_handle_table( MX_HANDLE_TABLE *handle_table );

MX_API mx_status_type mx_create_handle( signed long *handle,
					MX_HANDLE_TABLE *handle_table,
					void *pointer );

MX_API mx_status_type mx_delete_handle( signed long handle,
					MX_HANDLE_TABLE *handle_table );

MX_API mx_status_type mx_get_handle_from_pointer(
					signed long *handle,
					MX_HANDLE_TABLE *handle_table,
					void *pointer );

MX_API mx_status_type mx_get_pointer_from_handle(
					void **pointer,
					MX_HANDLE_TABLE *handle_table,
					signed long handle );

#ifdef __cplusplus
}
#endif

#endif /* __MX_HANDLE_H__ */


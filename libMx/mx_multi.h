/*
 * Name:    mx_version.h
 *
 * Purpose: Support for cross-protocol network functionality.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MULTI_H__
#define __MX_MULTI_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned long network_type;
	void *private_ptr;
} MX_MULTI_NETWORK_VARIABLE;

typedef struct {
	unsigned long network_type;
	void *private_ptr;
} MX_MULTI_NETWORK_CALLBACK;

/* 'network_type' definitions. */

#define MXT_MNV_MX	1
#define MXT_MNV_EPICS	2

/*----*/

MX_API void mx_multi_set_debug_flag( MX_RECORD *record_list,
					mx_bool_type value );

MX_API mx_status_type mx_multi_create( MX_MULTI_NETWORK_VARIABLE **mnv,
					char *network_variable_description,
					void **network_object );

MX_API mx_status_type mx_multi_close( MX_MULTI_NETWORK_VARIABLE *mnv );

MX_API mx_status_type mx_multi_get( MX_MULTI_NETWORK_VARIABLE *mnv,
					long mx_datatype,
					long num_dimensions,
					long *dimension,
					void *value );

MX_API mx_status_type mx_multi_put( MX_MULTI_NETWORK_VARIABLE *mnv,
					long mx_datatype,
					long num_dimensions,
					long *dimension,
					void *value );

MX_API mx_status_type mx_multi_add_callback( MX_MULTI_NETWORK_VARIABLE *mnv,
					unsigned long mx_callback_type,
					mx_status_type ( *callback_function ),
					void *callback_argument,
					MX_MULTI_NETWORK_CALLBACK **cb );

MX_API mx_status_type mx_multi_delete_callback( MX_MULTI_NETWORK_CALLBACK *cb );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MULTI_H__ */


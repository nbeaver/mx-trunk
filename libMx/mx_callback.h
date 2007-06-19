/*
 * Name:    mx_callback.h
 *
 * Purpose: Header for MX callback handling functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_CALLBACK_H__
#define __MX_CALLBACK_H__

/*--- Callback classes ---*/

#define MXCBC_NETWORK		1
#define MXCBC_FIELD		2

/*--- Callback types ---*/

#define MXCBT_VALUE_CHANGED	1
#define MXCBT_POLL		2

/*---*/

typedef struct mx_callback_type {
	unsigned long callback_class;
	unsigned long callback_type;
	uint32_t callback_id;
	mx_bool_type active;
	mx_status_type ( *callback_function )
				( struct mx_callback_type *, void * );
	void *callback_argument;
	union {
		MX_NETWORK_FIELD *network_field;
		MX_RECORD_FIELD *record_field;
	} u;
} MX_CALLBACK;

typedef struct {
	MX_LIST_HEAD *list_head;
} MX_CALLBACK_POLL_MESSAGE;

typedef struct {
	unsigned long callback_type;
	union {
		MX_CALLBACK_POLL_MESSAGE poll;
	} u;
} MX_CALLBACK_MESSAGE;

/*--- Network client callbacks ---*/

MX_API mx_status_type mx_network_add_callback( MX_NETWORK_FIELD *nf,
					unsigned long callback_type,
					mx_status_type ( *callback_function )
						( MX_CALLBACK *, void * ),
					void *callback_argument,
					MX_CALLBACK **callback_object );
					
MX_API mx_status_type mx_field_add_callback( MX_RECORD_FIELD *rf,
					unsigned long callback_type,
					mx_status_type ( *callback_function )
						( MX_CALLBACK *, void * ),
					void *callback_argument,
					MX_CALLBACK **callback_object );

MX_API mx_status_type mx_field_initialize_callbacks( MX_RECORD *record_list );

MX_API mx_status_type mx_field_invoke_callback_list( MX_RECORD_FIELD *field,
						unsigned long callback_type );

MX_API mx_status_type mx_invoke_callback( MX_CALLBACK *cb );

#endif /* __MX_CALLBACK_H__ */


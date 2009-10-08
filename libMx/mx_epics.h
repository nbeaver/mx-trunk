/*
 * Name:    mx_epics.h
 *
 * Purpose: Header file for some utilities for using EPICS with MX.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 1999-2005, 2007, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_EPICS_H__
#define __MX_EPICS_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_hrt.h"

/* We do not want to require including the EPICS include files tsDefs.h
 * and cadef.h here, so we just define the data types values here.  
 * However, the values below _must_ match the EPICS db_access.h file.
 */

#define MX_CA_CHAR	4		/* DBR_CHAR   */
#define MX_CA_STRING	0		/* DBR_STRING */
#define MX_CA_INT	1		/* DBR_INT    */
#define MX_CA_SHORT	MX_CA_INT
#define MX_CA_LONG	5		/* DBR_LONG   */
#define MX_CA_FLOAT	2		/* DBR_FLOAT  */
#define MX_CA_DOUBLE	6		/* DBR_DOUBLE */
#define MX_CA_ENUM	3		/* DBR_ENUM   */

#define MXU_EPICS_STRING_LENGTH		40

/* EPICS event trigger masks. */

#define MX_CA_EVENT_VALUE	0x1	/* DBE_VALUE */
#define MX_CA_EVENT_LOG		0x2	/* DBE_LOG */
#define MX_CA_EVENT_ALARM	0x4	/* DBE_ALARM */

#define MXU_EPICS_PVNAME_LENGTH		100

/* Define values for EPICS callback handlers in the MX_EPICS_PV structure. */

#define MXF_EPVH_IDLE				0
#define MXF_EPVH_CALLBACK_IN_PROGRESS		1
#define MXF_EPVH_UNEXPECTED_HANDLER_CALL	2

/* Declaring channel_id below as a 'chid' would require every file that
 * includes 'mx_epics.h' to include 'cadef.h' as well.  Since we want
 * 'mx_epics.c' to be the only file that includes 'cadef.h', we declare
 * channel_id below as a void pointer.
 */

typedef struct {
	char pvname[ MXU_EPICS_PVNAME_LENGTH + 1 ];
	void *channel_id;	/* In EPICS this is a 'chid'. */

	struct timespec connection_state_last_change_time;
	long connection_state;	/* CA_OP_CONN_UP or CA_OP_CONN_DOWN */

	struct timespec connect_timeout_interval;
	struct timespec reconnect_timeout_interval;
	struct timespec io_timeout_interval;

	int put_callback_status;

	void *application_ptr;
} MX_EPICS_PV;

typedef struct {
	unsigned int group_id;	/* In EPICS this is a 'CA_SYNC_GID'. */
} MX_EPICS_GROUP;

typedef struct mx_epics_callback_type {
	MX_EPICS_PV *pv;
	void *event_id;		/* In EPICS this is an 'evid' */

	mx_status_type ( *callback_function )
			( struct mx_epics_callback_type *, void * );
	void *callback_argument;

	long epics_type;
	long epics_count;
	const void *value_ptr;
	int epics_status;
} MX_EPICS_CALLBACK;

/* MX EPICS functions for persistent connections. */

MX_API void mx_epics_pvname_init( MX_EPICS_PV *pv, char *name_format, ... );

MX_API mx_status_type mx_epics_pv_connect( MX_EPICS_PV *pv,
				mx_bool_type wait_for_connection );

MX_API mx_status_type mx_epics_pv_disconnect( MX_EPICS_PV *pv );

MX_API mx_status_type mx_epics_pend_io( double timeout );

MX_API mx_status_type mx_epics_pend_event( double timeout );

#define mx_epics_poll()  mx_epics_pend_event( 1.0e-12 )

MX_API mx_status_type mx_epics_flush_io( void );

/*---*/

MX_API mx_status_type mx_caget( MX_EPICS_PV *pv,
				long epics_type,
				unsigned long num_elements,
				void *data_buffer );

MX_API mx_status_type mx_caget_with_timeout( MX_EPICS_PV *pv,
				long epics_type,
				unsigned long num_elements,
				void *data_buffer,
				double timeout );

/*---*/

MX_API mx_status_type mx_caput( MX_EPICS_PV *pv,
				long epics_type,
				unsigned long num_elements,
				void *data_buffer );

MX_API mx_status_type mx_caput_nowait( MX_EPICS_PV *pv,
				long epics_type,
				unsigned long num_elements,
				void *data_buffer );

MX_API mx_status_type mx_caput_with_callback( MX_EPICS_PV *pv,
				long epics_type,
				unsigned long num_elements,
				void *data_buffer,
				mx_status_type (*callback_function)
					( MX_EPICS_CALLBACK *, void * ),
				void *callback_argument );

MX_API mx_status_type mx_caput_with_timeout( MX_EPICS_PV *pv,
				long epics_type,
				unsigned long num_elements,
				void *data_buffer,
				double timeout );

/*---*/

MX_API long mx_epics_pv_get_field_type( MX_EPICS_PV *pv );

MX_API unsigned long mx_epics_pv_get_element_count( MX_EPICS_PV *pv );

MX_API size_t mx_epics_get_element_size( long field_type );

/* MX EPICS asynchronous callback functions. */

MX_API mx_status_type mx_epics_add_callback( MX_EPICS_PV *pv,
				unsigned long requested_callback_mask,
				mx_status_type ( *callback_function )
					( MX_EPICS_CALLBACK *, void * ),
				void *callback_argument,
				MX_EPICS_CALLBACK **callback_object );

MX_API mx_status_type mx_epics_delete_callback( MX_EPICS_CALLBACK *callback );

/* MX EPICS functions for one time only connections. */

MX_API mx_status_type mx_caget_by_name( char *pvname,
					long epics_type,
					unsigned long num_elements,
					void *data_buffer );

MX_API mx_status_type mx_caput_by_name( char *pvname,
					long epics_type,
					unsigned long num_elements,
					void *data_buffer );

MX_API mx_status_type mx_caput_nowait_by_name( char *pvname,
					long epics_type,
					unsigned long num_elements,
					void *data_buffer );

/* MX EPICS functions for Channel Access synchronous groups. */

MX_API mx_status_type mx_epics_start_group( MX_EPICS_GROUP *epics_group );

MX_API mx_status_type mx_epics_end_group( MX_EPICS_GROUP *epics_group );

MX_API mx_status_type mx_epics_delete_group( MX_EPICS_GROUP *epics_group );

MX_API mx_status_type mx_group_caget( MX_EPICS_GROUP *epics_group,
					MX_EPICS_PV *pv,
					long epics_type,
					unsigned long num_elements,
					void *data_buffer );

MX_API mx_status_type mx_group_caput( MX_EPICS_GROUP *epics_group,
					MX_EPICS_PV *pv,
					long epics_type,
					unsigned long num_elements,
					void *data_buffer );

/* Miscellaneous functions. */

MX_API mx_status_type mx_epics_initialize( void );

MX_API int  mx_epics_get_debug_flag( void );

MX_API void mx_epics_set_debug_flag( int value );

MX_API mx_status_type mx_epics_get_pv_type( char *pvname,
						long *epics_type,
						long *array_length );

MX_API mx_status_type mx_epics_convert_mx_type_to_epics_type(
				long mx_type, long *epics_type );

MX_API mx_status_type mx_epics_convert_epics_type_to_mx_type(
				long epics_type, long *mx_type );

#ifdef __cplusplus
}
#endif

#endif /* __MX_EPICS_H__ */


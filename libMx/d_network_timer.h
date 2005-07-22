/*
 * Name:    d_network_timer.h
 *
 * Purpose: Header file for MX timer driver to control MX network timers.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_TIMER_H__
#define __D_NETWORK_TIMER_H__

#include "mx_timer.h"

/* ==== MX network timer data structure ==== */

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD busy_nf;
	MX_NETWORK_FIELD clear_nf;
	MX_NETWORK_FIELD last_measurement_time_nf;
	MX_NETWORK_FIELD mode_nf;
	MX_NETWORK_FIELD stop_nf;
	MX_NETWORK_FIELD value_nf;
} MX_NETWORK_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_network_timer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_timer_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_network_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_network_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_network_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_network_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_network_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_network_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_network_timer_set_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_network_timer_get_last_measurement_time(
						MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_network_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_network_timer_timer_function_list;

extern long mxd_network_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_timer_rfield_def_ptr;

#define MXD_NETWORK_TIMER_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_TIMER, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_TIMER, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_NETWORK_TIMER_H__ */


/*
 * Name:    d_am9513_timer.h
 *
 * Purpose: Header file for MX timer driver to control Am9513 counters
 *          as timers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */


#ifndef __D_AM9513_TIMER_H__
#define __D_AM9513_TIMER_H__

#include "mx_timer.h"

/* ==== Am9513 timer data structure ==== */

typedef struct {
	mx_length_type num_counters;
	MX_INTERFACE *am9513_interface_array;
	double clock_frequency;
} MX_AM9513_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_am9513_timer_initialize_type( long type );
MX_API mx_status_type mxd_am9513_timer_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_am9513_timer_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_am9513_timer_open( MX_RECORD *record );
MX_API mx_status_type mxd_am9513_timer_close( MX_RECORD *record );

MX_API mx_status_type mxd_am9513_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_am9513_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_am9513_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_am9513_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_am9513_timer_set_mode( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_am9513_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_am9513_timer_timer_function_list;

extern mx_length_type mxd_am9513_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_am9513_timer_rfield_def_ptr;

#define MXD_AM9513_TIMER_STANDARD_FIELDS \
  {-1, -1, "num_counters", MXFT_LENGTH, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_TIMER, num_counters), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "am9513_interface_array", MXFT_INTERFACE, NULL, \
	1, {MXU_VARARGS_LENGTH}, MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AM9513_TIMER, am9513_interface_array), \
	{sizeof(MX_INTERFACE)}, \
		NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)},\
  \
  {-1, -1, "clock_frequency", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_TIMER, clock_frequency), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_AM9513_TIMER_H__ */


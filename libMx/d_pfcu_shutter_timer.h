/*
 * Name:    d_pfcu_shutter_timer.h
 *
 * Purpose: Header file for MX timer driver to control the exposure time
 *          of a PF2S2 shutter controlled by an XIA PFCU controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PFCU_SHUTTER_TIMER_H__
#define __D_PFCU_SHUTTER_TIMER_H__

#define MX_PFCU_SHUTTER_TIMER_MAXIMUM_COUNTING_TIME \
				( 0.01 * 65535.0 * 65535.0 )

#define MX_PFCU_SHUTTER_TIMER_MINIMUM_COUNTING_TIME	0.01

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pfcu_record;
	int module_number;
} MX_PFCU_SHUTTER_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_pfcu_shutter_timer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pfcu_shutter_timer_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_pfcu_shutter_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_pfcu_shutter_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_pfcu_shutter_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_pfcu_shutter_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_pfcu_shutter_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_pfcu_shutter_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_pfcu_shutter_timer_set_mode( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_pfcu_shutter_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_pfcu_shutter_timer_timer_function_list;

extern mx_length_type mxd_pfcu_shutter_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pfcu_shutter_timer_rfield_def_ptr;

#define MXD_PFCU_SHUTTER_TIMER_STANDARD_FIELDS \
  {-1, -1, "pfcu_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU_SHUTTER_TIMER, pfcu_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "module_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU_SHUTTER_TIMER, module_number),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}


#endif /* __D_PFCU_SHUTTER_TIMER_H__ */


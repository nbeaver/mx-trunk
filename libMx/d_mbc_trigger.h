/*
 * Name:    d_mbc_trigger.h
 *
 * Purpose: Header file for MX timer driver to control the MBC (ALS 4.2.2)
 *          beamline trigger signal.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MBC_TRIGGER_H__
#define __D_MBC_TRIGGER_H__

#include "mx_timer.h"

/* ==== MX MCA timer data structure ==== */

typedef struct {
	char epics_prefix[ MXU_EPICS_PVNAME_LENGTH+1 ];

	mx_bool_type exposure_in_progress;

	MX_EPICS_PV command_pv;
	MX_EPICS_PV command_trig_pv;
	MX_EPICS_PV expose_pv;
	MX_EPICS_PV shutter_pv;
} MX_MBC_TRIGGER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mbc_trigger_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mbc_trigger_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_mbc_trigger_open( MX_RECORD *record );

MX_API mx_status_type mxd_mbc_trigger_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_mbc_trigger_start( MX_TIMER *timer );
MX_API mx_status_type mxd_mbc_trigger_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_mbc_trigger_read( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_mbc_trigger_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_mbc_trigger_timer_function_list;

extern long mxd_mbc_trigger_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mbc_trigger_rfield_def_ptr;

#define MXD_MBC_TRIGGER_STANDARD_FIELDS \
  {-1, -1, "epics_prefix", MXFT_STRING, NULL, 0, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MBC_TRIGGER, epics_prefix), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_MBC_TRIGGER_H__ */


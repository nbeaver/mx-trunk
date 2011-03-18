/*
 * Name:    d_mbc_gsc_trigger.h
 *
 * Purpose: Header file for MX pulse generator driver to control
 *          the MBC (ALS 4.2.2) beamline trigger signal using
 *          MBC:gsc: EPICS PVs.
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

#ifndef __D_MBC_GSC_TRIGGER_H__
#define __D_MBC_GSC_TRIGGER_H__

#include "mx_pulse_generator.h"

/* ==== MBC trigger data structure ==== */

typedef struct {
	char epics_prefix[ MXU_EPICS_PVNAME_LENGTH+1 ];

	mx_bool_type exposure_in_progress;

	MX_EPICS_PV collect_count_pv;
	MX_EPICS_PV collect_preset_enable_pv;
	MX_EPICS_PV shutter_pv;
	MX_EPICS_PV shutter_timeout_val_pv;
	MX_EPICS_PV timer_count_pv;
	MX_EPICS_PV timer_preset_pv;

	double timer_frequency;
} MX_MBC_GSC_TRIGGER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mbc_gsc_trigger_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mbc_gsc_trigger_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_mbc_gsc_trigger_open( MX_RECORD *record );

MX_API mx_status_type mxd_mbc_gsc_trigger_is_busy( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_mbc_gsc_trigger_start( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_mbc_gsc_trigger_stop( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_mbc_gsc_trigger_get_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_mbc_gsc_trigger_set_parameter(
					MX_PULSE_GENERATOR *pulser );

extern MX_RECORD_FUNCTION_LIST mxd_mbc_gsc_trigger_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST mxd_mbc_gsc_trigger_pulser_function_list;

extern long mxd_mbc_gsc_trigger_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mbc_gsc_trigger_rfield_def_ptr;

#define MXD_MBC_GSC_TRIGGER_STANDARD_FIELDS \
  {-1, -1, "epics_prefix", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MBC_GSC_TRIGGER, epics_prefix), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_MBC_GSC_TRIGGER_H__ */


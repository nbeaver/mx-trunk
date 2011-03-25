/*
 * Name:    d_radicon_helios_trigger.h
 *
 * Purpose: Header file for an MX pulse generator driver to generate
 *          a trigger pulse for the Radicon Helios detectors using
 *          the Pleora iPORT PLC.
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

#ifndef __D_RADICON_HELIOS_TRIGGER_H__
#define __D_RADICON_HELIOS_TRIGGER_H__

#include "mx_pulse_generator.h"

/* ==== Radicon Helios trigger data structure ==== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *radicon_helios_record;
} MX_RADICON_HELIOS_TRIGGER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_rh_trigger_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_rh_trigger_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_rh_trigger_open( MX_RECORD *record );

MX_API mx_status_type mxd_rh_trigger_is_busy( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_rh_trigger_start( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_rh_trigger_stop( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_rh_trigger_get_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_rh_trigger_set_parameter(
					MX_PULSE_GENERATOR *pulser );

extern MX_RECORD_FUNCTION_LIST mxd_rh_trigger_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST mxd_rh_trigger_pulser_function_list;

extern long mxd_rh_trigger_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_rh_trigger_rfield_def_ptr;

#define MXD_RADICON_HELIOS_TRIGGER_STANDARD_FIELDS \
  {-1, -1, "radicon_helios_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_HELIOS_TRIGGER, radicon_helios_record), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_RADICON_HELIOS_TRIGGER_H__ */


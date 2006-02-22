/*
 * Name:    d_keithley2700_amp.h
 *
 * Purpose: Header file for per-channel amplifiers for Keithley 2700
 *          switching multimeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KEITHLEY2700_AMP_H__
#define __D_KEITHLEY2700_AMP_H__

#include "mx_amplifier.h"

typedef struct {
	MX_RECORD *controller_record;
	int system_channel;

	int slot;
	int channel;
} MX_KEITHLEY2700_AMP;

MX_API mx_status_type mxd_keithley2700_amp_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_keithley2700_amp_open( MX_RECORD *record );
MX_API mx_status_type mxd_keithley2700_amp_close( MX_RECORD *record );

MX_API mx_status_type mxd_keithley2700_amp_get_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_keithley2700_amp_set_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_keithley2700_amp_get_offset(MX_AMPLIFIER *amplifier);
MX_API mx_status_type mxd_keithley2700_amp_set_offset(MX_AMPLIFIER *amplifier);
MX_API mx_status_type mxd_keithley2700_amp_get_time_constant(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_keithley2700_amp_set_time_constant(
						MX_AMPLIFIER *amplifier );

extern MX_RECORD_FUNCTION_LIST mxd_keithley2700_amp_record_function_list;
extern MX_AMPLIFIER_FUNCTION_LIST mxd_keithley2700_amp_amplifier_function_list;

extern long mxd_keithley2700_amp_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_keithley2700_amp_rfield_def_ptr;

#define MXD_KEITHLEY2700_AMP_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2700_AMP, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "system_channel", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2700_AMP, system_channel), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_KEITHLEY2700_AMP_H__ */

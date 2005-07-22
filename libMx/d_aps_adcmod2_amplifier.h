/*
 * Name:    d_aps_adcmod2_amplifier.h
 *
 * Purpose: Header file for MX driver for the ADCMOD2 electrometer system
 *          designed by Steve Ross of the Advanced Photon Source.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_APS_ADCMOD2_AMPLIFIER_H__
#define __D_APS_ADCMOD2_AMPLIFIER_H__

#include "mx_amplifier.h"

typedef struct {
	MX_RECORD *aps_adcmod2_record;

	int amplifier_number;
} MX_APS_ADCMOD2_AMPLIFIER;

MX_API mx_status_type mxd_aps_adcmod2_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_aps_adcmod2_open( MX_RECORD *record );

MX_API mx_status_type mxd_aps_adcmod2_set_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_aps_adcmod2_set_time_constant(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_aps_adcmod2_get_parameter( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_aps_adcmod2_set_parameter( MX_AMPLIFIER *amplifier );

extern MX_RECORD_FUNCTION_LIST mxd_aps_adcmod2_record_function_list;
extern MX_AMPLIFIER_FUNCTION_LIST mxd_aps_adcmod2_amplifier_function_list;

extern long mxd_aps_adcmod2_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aps_adcmod2_rfield_def_ptr;

#define MXD_APS_ADCMOD2_STANDARD_FIELDS \
  {-1, -1, "aps_adcmod2_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_ADCMOD2_AMPLIFIER, aps_adcmod2_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "amplifier_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_ADCMOD2_AMPLIFIER, amplifier_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_APS_ADCMOD2_AMPLIFIER_H__ */

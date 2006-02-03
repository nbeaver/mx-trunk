/*
 * Name:    d_aps_quadem_amplifier.h
 *
 * Purpose: Header file for MX driver for the QUADEM electrometer system
 *          designed by Steve Ross of the Advanced Photon Source using the
 *          quadEM EPICS support written by Mark Rivers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_APS_QUADEM_AMPLIFIER_H__
#define __D_APS_QUADEM_AMPLIFIER_H__

#include "mx_amplifier.h"

typedef struct {
	char epics_record_namestem[ MXU_EPICS_PVNAME_LENGTH+1 ];

	MX_EPICS_PV gain_pv;
	MX_EPICS_PV conversion_time_pv;
} MX_APS_QUADEM_AMPLIFIER;

MX_API mx_status_type mxd_aps_quadem_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_aps_quadem_open( MX_RECORD *record );

MX_API mx_status_type mxd_aps_quadem_get_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_aps_quadem_set_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_aps_quadem_get_time_constant(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_aps_quadem_set_time_constant(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_aps_quadem_get_parameter( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_aps_quadem_set_parameter( MX_AMPLIFIER *amplifier );

extern MX_RECORD_FUNCTION_LIST mxd_aps_quadem_record_function_list;
extern MX_AMPLIFIER_FUNCTION_LIST mxd_aps_quadem_amplifier_function_list;

extern mx_length_type mxd_aps_quadem_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aps_quadem_rfield_def_ptr;

#define MXD_APS_QUADEM_STANDARD_FIELDS \
  {-1, -1, "epics_record_namestem", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_QUADEM_AMPLIFIER, epics_record_namestem), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_APS_QUADEM_AMPLIFIER_H__ */

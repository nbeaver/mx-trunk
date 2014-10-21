/*
 * Name:     d_epics_scaler_mce.h
 *
 * Purpose: Header file for MX multichannel encoder driver to record an
 *          EPICS-controlled motor's position as part of a synchronous
 *          group that also reads out channels from an EPICS Scaler record.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_SCALER_MCE_H__
#define __D_EPICS_SCALER_MCE_H__

#include "mx_mce.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mcs_record;

	MX_EPICS_PV **epics_pv_array;
} MX_EPICS_SCALER_MCE;

#define MXD_EPICS_SCALER_MCE_STANDARD_FIELDS \
  {-1, -1, "mcs_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_SCALER_MCE, mcs_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_epics_scaler_mce_initialize_driver(MX_DRIVER *driver);
MX_API mx_status_type mxd_epics_scaler_mce_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_scaler_mce_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_scaler_mce_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_scaler_mce_read( MX_MCE *mce );
MX_API mx_status_type mxd_epics_scaler_mce_get_current_num_values(MX_MCE *mce);
MX_API mx_status_type mxd_epics_scaler_mce_connect_mce_to_motor( MX_MCE *mce,
						MX_RECORD *motor_record );

extern MX_RECORD_FUNCTION_LIST mxd_epics_scaler_mce_record_function_list;
extern MX_MCE_FUNCTION_LIST mxd_epics_scaler_mce_mce_function_list;

extern long mxd_epics_scaler_mce_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_scaler_mce_rfield_def_ptr;

#endif /* __D_EPICS_SCALER_MCE_H__ */

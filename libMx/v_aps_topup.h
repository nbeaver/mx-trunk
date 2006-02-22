/*
 * Name:    v_aps_topup.h
 *
 * Purpose: Header file for the Advanced Photon Source top-up interlock signals.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _V_APS_TOPUP_H_
#define _V_APS_TOPUP_H_

typedef struct {
	double extra_delay_time;

	MX_EPICS_PV topup_time_to_inject_pv;
} MX_APS_TOPUP;

#define MX_APS_TOPUP_STANDARD_FIELDS \
  {-1, -1, "extra_delay_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_TOPUP, extra_delay_time), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxv_aps_topup_initialize_type( long );
MX_API_PRIVATE mx_status_type mxv_aps_topup_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_aps_topup_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_aps_topup_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_aps_topup_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_aps_topup_variable_function_list;

extern long mxv_aps_topup_interlock_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_aps_topup_interlock_field_def_ptr;

extern long mxv_aps_topup_time_to_inject_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_aps_topup_time_to_inject_field_def_ptr;

#endif /* _V_APS_TOPUP_H_ */


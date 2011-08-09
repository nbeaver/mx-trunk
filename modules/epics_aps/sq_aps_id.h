/*
 * Name:    sq_aps_id.h
 *
 * Purpose: Header file for quick scans that use an MX MCS to collect the data
 *          and simultaneously synchronously scan an Advanced Photon Source
 *          insertion device.  This scan type is a variant of the MCS quick
 *          scan and uses most of the same code.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SQ_APS_ID_H__
#define __SQ_APS_ID_H__

#define MX_APSID_PARAMS_RECORD_NAME		"mx_apsid_params"

#define MX_APSID_NUM_PARAMS			5

#define MX_APSID_ID_EV_RECORD_NAME		0
#define MX_APSID_ID_EV_ENABLED_NAME		1
#define MX_APSID_ID_EV_PARAMS_NAME		2
#define MX_APSID_UNDULATOR_HARMONIC_NAME	3
#define MX_APSID_D_SPACING_NAME			4

typedef struct {
	MX_RECORD *id_ev_record;
	int id_ev_enabled;
	int id_ev_harmonic;
	double id_ev_offset;
	double d_spacing;
	int undulator_harmonic;
	int sector_number;

	MX_EPICS_PV busy_pv;
	MX_EPICS_PV energy_set_pv;
	MX_EPICS_PV gap_set_pv;
	MX_EPICS_PV harmonic_value_pv;
	MX_EPICS_PV message1_pv;
	MX_EPICS_PV ss_end_gap_pv;
	MX_EPICS_PV ss_start_pv;
	MX_EPICS_PV ss_start_gap_pv;
	MX_EPICS_PV ss_state_pv;
	MX_EPICS_PV ss_time_pv;
	MX_EPICS_PV sync_scan_mode_pv;
} MX_APSID_QUICK_SCAN_EXTENSION;

MX_API mx_status_type mxs_apsid_quick_scan_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxs_apsid_quick_scan_prepare_for_scan_start(
							MX_SCAN *scan );
MX_API mx_status_type mxs_apsid_quick_scan_execute_scan_body(
							MX_SCAN *scan );
MX_API mx_status_type mxs_apsid_quick_scan_cleanup_after_scan_end(
							MX_SCAN *scan );

extern MX_RECORD_FUNCTION_LIST mxs_apsid_quick_scan_record_function_list;
extern MX_SCAN_FUNCTION_LIST mxs_apsid_quick_scan_scan_function_list;

extern long mxs_apsid_quick_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_apsid_quick_scan_def_ptr;

#endif /* __SQ_APS_ID_H__ */


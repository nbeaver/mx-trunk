/*
 * Name:    sq_energy_mcs.h
 *
 * Purpose: Header file for quick scans of the energy pseudomotor that use
 * an MX MCS to collect the data.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SQ_ENERGY_MCS_H__
#define __SQ_ENERGY_MCS_H__

typedef struct {
	double d_spacing;
} MX_ENERGY_MCS_QUICK_SCAN_EXTENSION;

MX_API mx_status_type mxs_energy_mcs_quick_scan_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxs_energy_mcs_quick_scan_prepare_for_scan_start(
							MX_SCAN *scan );
MX_API mx_status_type mxs_energy_mcs_quick_scan_execute_scan_body(
							MX_SCAN *scan );
MX_API mx_status_type mxs_energy_mcs_quick_scan_cleanup_after_scan_end(
							MX_SCAN *scan );

extern MX_RECORD_FUNCTION_LIST mxs_energy_mcs_quick_scan_record_function_list;
extern MX_SCAN_FUNCTION_LIST mxs_energy_mcs_quick_scan_scan_function_list;

extern long mxs_energy_mcs_quick_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_energy_mcs_quick_scan_def_ptr;

#endif /* __SQ_ENERGY_MCS_H__ */


/*
 * Name:    sa_wedge.h
 *
 * Purpose: Header file for area detector wedge scan.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SA_WEDGE_H__
#define __SA_WEDGE_H__

MX_API mx_status_type mxs_wedge_scan_create_record_structures(
					MX_RECORD *record,
					MX_SCAN *scan,
					MX_AREA_DETECTOR_SCAN *ad_scan );
MX_API mx_status_type mxs_wedge_scan_finish_record_initialization(
					MX_RECORD *record );

extern MX_AREA_DETECTOR_SCAN_FUNCTION_LIST
			mxs_wedge_area_detector_scan_function_list;

extern long mxs_wedge_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_wedge_scan_def_ptr;

#endif /* __SA_WEDGE_H__ */

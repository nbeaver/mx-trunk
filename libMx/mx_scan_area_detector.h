/*
 * Name:    mx_scan_area_detector.h
 *
 * Purpose: Header file for MX_AREA_DETECTOR_SCAN records.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_SCAN_AREA_DETECTOR_H_
#define _MX_SCAN_AREA_DETECTOR_H_

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_SCAN *scan; /* Pointer to the parent scan superclass structure. */

	void *record_type_struct;
} MX_AREA_DETECTOR_SCAN;

typedef struct {
	mx_status_type ( *create_record_structures ) (
				MX_RECORD *record,
				MX_SCAN *scan,
				MX_AREA_DETECTOR_SCAN *ad_scan );
	mx_status_type ( *finish_record_initialization ) (
				MX_RECORD *record );
	mx_status_type ( *prepare_for_scan_start ) (
				MX_SCAN *scan );
} MX_AREA_DETECTOR_SCAN_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mxs_area_detector_scan_initialize_type( long );
MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_create_record_structures( MX_RECORD *record );
MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_finish_record_initialization( MX_RECORD *record);
MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_delete_record( MX_RECORD *record);

MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_print_structure( FILE *file, MX_RECORD *record);

MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_prepare_for_scan_start( MX_SCAN *scan);
MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_execute_scan_body( MX_SCAN *scan);
MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_cleanup_after_scan_end( MX_SCAN *scan);

extern MX_RECORD_FUNCTION_LIST mxs_area_detector_scan_record_function_list;
extern MX_SCAN_FUNCTION_LIST mxs_area_detector_scan_scan_function_list;

#define MX_AREA_DETECTOR_SCAN_STANDARD_FIELDS

#ifdef __cplusplus
}
#endif

#endif /* _MX_SCAN_AREA_DETECTOR_H_ */


/*
 * Name:    mx_scan_list.h
 *
 * Purpose: Header file for MX_LIST_SCAN scan records.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2004, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_SCAN_LIST_H_
#define _MX_SCAN_LIST_H_

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_SCAN *scan;	/* Pointer to the parent scan superclass structure. */

	void *record_type_struct;
} MX_LIST_SCAN;

typedef struct {
	mx_status_type ( *create_record_structures )
			( MX_RECORD *, MX_SCAN *, MX_LIST_SCAN * );
	mx_status_type ( *finish_record_initialization ) ( MX_RECORD * );
	mx_status_type ( *delete_record ) ( MX_RECORD * );
	mx_status_type ( *open_list ) ( MX_LIST_SCAN * );
	mx_status_type ( *close_list ) ( MX_LIST_SCAN * );
	mx_status_type ( *get_next_measurement_parameters )
					( MX_SCAN *, MX_LIST_SCAN * );
} MX_LIST_SCAN_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mxs_list_scan_initialize_type( long type );
MX_API_PRIVATE mx_status_type mxs_list_scan_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxs_list_scan_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxs_list_scan_delete_record( MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxs_list_scan_print_scan_structure(
						FILE *file, MX_RECORD *record);

MX_API_PRIVATE mx_status_type mxs_list_scan_prepare_for_scan_start(
							MX_SCAN *scan);
MX_API_PRIVATE mx_status_type mxs_list_scan_execute_scan_body(
							MX_SCAN *scan );
MX_API_PRIVATE mx_status_type mxs_list_scan_cleanup_after_scan_end(
							MX_SCAN *scan );

extern MX_RECORD_FUNCTION_LIST mxs_list_scan_record_function_list;
extern MX_SCAN_FUNCTION_LIST mxs_list_scan_scan_function_list;

/* Currently, there are no standard fields for a list scan. */

#define MX_LIST_SCAN_STANDARD_FIELDS 

#ifdef __cplusplus
}
#endif

#endif /* _MX_SCAN_LIST_H_ */


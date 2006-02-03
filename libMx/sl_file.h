/*
 * Name:    sl_file.h
 *
 * Purpose: Header file for list scans that read positions from a file.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SL_FILE_H__
#define __SL_FILE_H__

typedef struct {
	FILE *position_file;
	char position_filename[MXU_FILENAME_LENGTH+1];
} MX_FILE_LIST_SCAN;

MX_API mx_status_type mxs_file_list_scan_create_record_structures(
	MX_RECORD *record, MX_SCAN *scan, MX_LIST_SCAN *list_scan );
MX_API mx_status_type mxs_file_list_scan_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxs_file_list_scan_delete_record(
						MX_RECORD *record );
MX_API mx_status_type mxs_file_list_scan_open_list( MX_LIST_SCAN *list_scan );
MX_API mx_status_type mxs_file_list_scan_close_list( MX_LIST_SCAN *list_scan );
MX_API mx_status_type mxs_file_list_scan_get_next_measurement_parameters(
					MX_SCAN *scan, MX_LIST_SCAN * );

extern MX_LIST_SCAN_FUNCTION_LIST mxs_file_list_scan_function_list;

extern mx_length_type mxs_file_list_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_file_list_scan_def_ptr;

#define MX_FILE_LIST_SCAN_STANDARD_FIELDS \
  {-1, -1, "position_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FILE_LIST_SCAN, position_filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __SL_FILE_H__ */


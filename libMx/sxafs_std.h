/*
 * Name:    sxafs_std.h
 *
 * Purpose: Header file for standard XAFS scan.
 * 
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SXAFS_STD_H__
#define __SXAFS_STD_H__

typedef struct {
	int dummy;
} MX_XAFS_STANDARD_SCAN;

MX_API mx_status_type mxs_xafs_std_scan_create_record_structures(
	MX_RECORD *record, MX_SCAN *scan, MX_XAFS_SCAN *list_scan );
MX_API mx_status_type mxs_xafs_std_scan_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxs_xafs_std_scan_delete_record(
						MX_RECORD *record );

extern long mxs_xafs_std_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_xafs_std_scan_def_ptr;

#endif /* __SXAFS_STD_H__ */


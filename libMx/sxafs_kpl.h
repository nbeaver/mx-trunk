/*
 * Name:    sxafs_kpl.h
 *
 * Purpose: Header file for standard XAFS scan.
 * 
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SXAFS_KPL_H__
#define __SXAFS_KPL_H__

typedef struct {
	int dummy;
} MX_XAFS_K_POWER_LAW_SCAN;

extern long mxs_xafs_kpl_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_xafs_kpl_scan_def_ptr;

#endif /* __SXAFS_KPL_H__ */


/*
 * Name:    sxafs_kpl.c
 *
 * Purpose: Scan description file for standard XAFS scans.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_scan.h"
#include "mx_scan_xafs.h"
#include "sxafs_kpl.h"


MX_RECORD_FIELD_DEFAULTS mxs_xafs_kpl_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_XAFS_SCAN_STANDARD_FIELDS
};

long mxs_xafs_kpl_scan_num_record_fields
			= sizeof( mxs_xafs_kpl_scan_defaults )
			/ sizeof( mxs_xafs_kpl_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_xafs_kpl_scan_def_ptr
			= &mxs_xafs_kpl_scan_defaults[0];


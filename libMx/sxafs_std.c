/*
 * Name:    sxafs_std.c
 *
 * Purpose: Scan description file for standard XAFS scans.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
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
#include "sxafs_std.h"


MX_RECORD_FIELD_DEFAULTS mxs_xafs_std_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_XAFS_SCAN_STANDARD_FIELDS
};

mx_length_type mxs_xafs_std_scan_num_record_fields
			= sizeof( mxs_xafs_std_scan_defaults )
			/ sizeof( mxs_xafs_std_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_xafs_std_scan_def_ptr
			= &mxs_xafs_std_scan_defaults[0];


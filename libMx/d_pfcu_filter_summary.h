/*
 * Name:    d_pfcu_filter_summary.h
 *
 * Purpose: Header file for controlling all of the filters of an XIA PFCU
 *          as a unit.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PFCU_FILTER_SUMMARY_H__
#define __D_PFCU_FILTER_SUMMARY_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pfcu_record;
	long module_number;
} MX_PFCU_FILTER_SUMMARY;

MX_API mx_status_type mxd_pfcu_filter_summary_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_pfcu_filter_summary_read(MX_DIGITAL_OUTPUT *doutput);
MX_API mx_status_type mxd_pfcu_filter_summary_write(MX_DIGITAL_OUTPUT *doutput);

extern MX_RECORD_FUNCTION_LIST mxd_pfcu_filter_summary_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_pfcu_filter_summary_digital_output_function_list;

extern long mxd_pfcu_filter_summary_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pfcu_filter_summary_rfield_def_ptr;

#define MXD_PFCU_FILTER_SUMMARY_STANDARD_FIELDS \
  {-1, -1, "pfcu_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU_FILTER_SUMMARY, pfcu_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "module_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU_FILTER_SUMMARY, module_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_PFCU_FILTER_SUMMARY_H__ */

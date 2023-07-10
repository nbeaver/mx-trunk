/*
 * Name:    d_handel_sum.h
 *
 * Purpose: Header file for a weighted sum of XIA Handel readings.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2004, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _D_HANDEL_SUM_H_
#define _D_HANDEL_SUM_H_

typedef struct {
	MX_RECORD *handel_record;
	unsigned long roi_number;
	MX_RECORD *mca_enable_record;
} MX_HANDEL_SUM;

#define MX_HANDEL_SUM_STANDARD_FIELDS \
  {-1, -1, "handel_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_SUM, handel_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "roi_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_SUM, roi_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "mca_enable_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_SUM, mca_enable_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxd_handel_sum_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_handel_sum_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_handel_sum_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST mxd_handel_sum_analog_input_function_list;

extern long mxd_handel_sum_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_handel_sum_rfield_def_ptr;

#endif /* _D_HANDEL_SUM_H_ */

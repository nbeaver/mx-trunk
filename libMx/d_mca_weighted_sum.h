/*
 * Name:    d_mca_weighted_sum.h
 *
 * Purpose: Header file for weighted sums of MCA regions of interest.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _D_MCA_WEIGHTED_SUM_H_
#define _D_MCA_WEIGHTED_SUM_H_

typedef struct {
	unsigned long roi_number;
	MX_RECORD *mca_enable_record;
	long num_mcas;
	MX_RECORD **mca_record_array;
} MX_MCA_WEIGHTED_SUM;

#define MX_MCA_WEIGHTED_SUM_STANDARD_FIELDS \
  {-1, -1, "roi_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_WEIGHTED_SUM, roi_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "mca_enable_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_WEIGHTED_SUM, mca_enable_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_mcas", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_WEIGHTED_SUM, num_mcas), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "mca_record_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_WEIGHTED_SUM, mca_record_array), \
	{sizeof(MX_RECORD *)}, NULL, \
			(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

MX_API_PRIVATE mx_status_type mxd_mca_weighted_sum_initialize_type( long type );

MX_API_PRIVATE mx_status_type mxd_mca_weighted_sum_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_mca_weighted_sum_read(
						MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_mca_weighted_sum_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_mca_weighted_sum_analog_input_function_list;

extern long mxd_mca_weighted_sum_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mca_weighted_sum_rfield_def_ptr;

#endif /* _D_MCA_WEIGHTED_SUM_H_ */


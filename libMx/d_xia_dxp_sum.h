/*
 * Name:    d_xia_dxp_sum.h
 *
 * Purpose: Header file for a weighted sum of XIA DXP readings.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _D_XIA_DXP_SUM_H_
#define _D_XIA_DXP_SUM_H_

typedef struct {
	MX_RECORD *xia_dxp_record;
	unsigned long roi_number;
	MX_RECORD *mca_enable_record;
} MX_XIA_DXP_SUM;

#define MX_XIA_DXP_SUM_STANDARD_FIELDS \
  {-1, -1, "xia_dxp_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_SUM, xia_dxp_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "roi_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_SUM, roi_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "mca_enable_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_SUM, mca_enable_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxd_xia_dxp_sum_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_xia_dxp_sum_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_xia_dxp_sum_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST mxd_xia_dxp_sum_analog_input_function_list;

extern mx_length_type mxd_xia_dxp_sum_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_xia_dxp_sum_rfield_def_ptr;

#endif /* _D_XIA_DXP_SUM_H_ */

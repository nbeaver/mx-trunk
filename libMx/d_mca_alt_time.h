/*
 * Name:    d_mca_alt_time.h
 *
 * Purpose: Header file for MX scaler driver to read out alternate MCA time
 *          values.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCA_ALT_TIME_H__
#define __D_MCA_ALT_TIME_H__

typedef struct {
	int time_type;
	double timer_scale;
} MX_MCA_ALT_TIME;

#define MXF_MCA_ALT_TIME_LIVE_TIME		0
#define MXF_MCA_ALT_TIME_REAL_TIME		1
#define MXF_MCA_ALT_TIME_COMPLEMENTARY_TIME	2

MX_API mx_status_type mxd_mca_alt_time_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mca_alt_time_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_mca_alt_time_read( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_mca_alt_time_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_mca_alt_time_scaler_function_list;

extern mx_length_type mxd_mca_alt_time_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mca_alt_time_rfield_def_ptr;

#define MXD_MCA_ALT_TIME_STANDARD_FIELDS \
  {-1, -1, "time_type", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_ALT_TIME, time_type),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "timer_scale", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_ALT_TIME, timer_scale),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_MCA_ALT_TIME_H__ */

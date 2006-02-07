/*
 * Name:    d_mca_roi_integral.h
 *
 * Purpose: Header file for MX scaler driver for reading out the integral
 *          of a multichannel analyzer's region of interest (ROI).
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCA_ROI_INTEGRAL_H__
#define __D_MCA_ROI_INTEGRAL_H__

typedef struct {
	MX_RECORD *mca_record;
	uint32_t roi_number;
} MX_MCA_ROI_INTEGRAL;

MX_API mx_status_type mxd_mca_roi_integral_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mca_roi_integral_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_mca_roi_integral_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_roi_integral_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_roi_integral_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_roi_integral_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_roi_integral_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_roi_integral_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_roi_integral_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_mca_roi_integral_set_parameter( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_mca_roi_integral_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_mca_roi_integral_scaler_function_list;

extern mx_length_type mxd_mca_roi_integral_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mca_roi_integral_rfield_def_ptr;

#define MXD_MCA_ROI_INTEGRAL_STANDARD_FIELDS \
  {-1, -1, "mca_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_ROI_INTEGRAL, mca_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "roi_number", MXFT_UINT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_ROI_INTEGRAL, roi_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_MCA_ROI_INTEGRAL_H__ */

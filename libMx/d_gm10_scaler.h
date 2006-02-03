/*
 * Name:    d_gm10_scaler.h
 *
 * Purpose: Header file for Black Cat Systems GM-10, GM-45, GM-50, and
 *          GM-90 radiation detectors.
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GM10_SCALER_H__
#define __D_GM10_SCALER_H__

typedef struct {
	MX_RECORD *rs232_record;
} MX_GM10_SCALER;

MX_API mx_status_type mxd_gm10_scaler_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_gm10_scaler_read( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_gm10_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_gm10_scaler_scaler_function_list;

extern mx_length_type mxd_gm10_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_gm10_scaler_rfield_def_ptr;

#define MXD_GM10_SCALER_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GM10_SCALER, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \

#endif /* __D_GM10_SCALER_H__ */

/*
 * Name:    d_qs450.h
 *
 * Purpose: Header file for MX scaler driver to control DSP QS450 
 *          scaler/counters and Kinetic Systems KS3610 scaler/counters.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_QS450_H__
#define __D_QS450_H__

#include "mx_scaler.h"

/* ===== QS450 scaler data structure ===== */

typedef struct {
	MX_RECORD *camac_record;
	int32_t slot;
	int32_t subaddress;
} MX_QS450;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_qs450_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_qs450_finish_record_initialization(MX_RECORD *record);

MX_API mx_status_type mxd_qs450_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_qs450_scaler_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_qs450_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_qs450_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_qs450_scaler_set_parameter( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_qs450_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_qs450_scaler_function_list;

extern mx_length_type mxd_qs450_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_qs450_record_field_def_ptr;

extern mx_length_type mxd_ks3610_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ks3610_record_field_def_ptr;

#define MXD_QS450_STANDARD_FIELDS \
  {-1, -1, "camac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_QS450, camac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  {-1, -1, "slot", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_QS450, slot), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  {-1, -1, "subaddress", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_QS450, subaddress), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_QS450_H__ */


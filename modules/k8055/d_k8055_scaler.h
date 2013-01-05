/*
 * Name:    d_k8055_scaler.h
 *
 * Purpose: Header file for Velleman K8055 counter channels as scalers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_K8055_SCALER_H__
#define __D_K8055_SCALER_H__

#include "mx_scaler.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *k8055_record;
	unsigned long channel;

} MX_K8055_SCALER;

#define MXD_K8055_SCALER_STANDARD_FIELDS \
  {-1, -1, "k8055_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_K8055_SCALER, k8055_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_K8055_SCALER, channel), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_k8055_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_k8055_scaler_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_k8055_scaler_read( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_k8055_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_k8055_scaler_scaler_function_list;

extern long mxd_k8055_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_k8055_scaler_rfield_def_ptr;

#endif /* __D_K8055_SCALER_H__ */


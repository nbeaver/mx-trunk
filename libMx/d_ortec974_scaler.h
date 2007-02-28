/*
 * Name:    d_ortec974_scaler.h
 *
 * Purpose: Header file for MX scaler driver to control 
 *          EG&G Ortec 974 scaler/counters.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ORTEC974_SCALER_H__
#define __D_ORTEC974_SCALER_H__

#include "mx_scaler.h"

/* ===== ORTEC 974 scaler data structure ===== */

typedef struct {
	MX_RECORD *ortec974_record;
	long channel_number;
} MX_ORTEC974_SCALER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_ortec974_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_ortec974_scaler_finish_record_initialization(
							MX_RECORD *record);

MX_API mx_status_type mxd_ortec974_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_ortec974_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_ortec974_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_ortec974_scaler_set_parameter( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_ortec974_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_ortec974_scaler_scaler_function_list;

extern long mxd_ortec974_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ortec974_scaler_rfield_def_ptr;

#define MXD_ORTEC974_SCALER_STANDARD_FIELDS \
  {-1, -1, "ortec974_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ORTEC974_SCALER, ortec974_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ORTEC974_SCALER, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_ORTEC974_SCALER_H__ */


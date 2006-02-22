/*
 * Name:    d_databox_mce.h
 *
 * Purpose: Header file for MX mce driver to read Databox motor positions
 *          via a multichannel encoder record.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DATABOX_MCE_H__
#define __D_DATABOX_MCE_H__

#include "mx_mce.h"

/* ===== DATABOX mce data structure ===== */

typedef struct {
	MX_RECORD *associated_motor_record;

	MX_RECORD *databox_mcs_record;
} MX_DATABOX_ENCODER;

#define MXD_DATABOX_ENCODER_STANDARD_FIELDS \
  {-1, -1, "associated_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DATABOX_ENCODER, associated_motor_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "databox_mcs_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DATABOX_ENCODER, databox_mcs_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_databox_encoder_initialize_type( long type );
MX_API mx_status_type mxd_databox_encoder_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_databox_encoder_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_databox_encoder_read( MX_MCE *mce );
MX_API mx_status_type mxd_databox_encoder_get_current_num_values( MX_MCE *mce );

extern MX_RECORD_FUNCTION_LIST mxd_databox_encoder_record_function_list;
extern MX_MCE_FUNCTION_LIST mxd_databox_encoder_mce_function_list;

extern long mxd_databox_encoder_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_databox_encoder_rfield_def_ptr;

#endif /* __D_DATABOX_MCE_H__ */


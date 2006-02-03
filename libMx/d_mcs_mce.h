/*
 * Name:    d_mcs_mce.h
 *
 * Purpose: Header file for MX mce driver to interpret MCS channels as
 *          a multichannel encoder.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCS_MCE_H__
#define __D_MCS_MCE_H__

#include "mx_mce.h"

/* ===== MCS mce data structure ===== */

typedef struct {
	MX_RECORD *associated_motor_record;

	MX_RECORD *mcs_record;
	unsigned long down_channel;
	unsigned long up_channel;
} MX_MCS_ENCODER;

#define MXD_MCS_ENCODER_STANDARD_FIELDS \
  {-1, -1, "associated_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MCS_ENCODER, associated_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "mcs_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCS_ENCODER, mcs_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "down_channel", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCS_ENCODER, down_channel), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "up_channel", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCS_ENCODER, up_channel), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mcs_encoder_initialize_type( long type );
MX_API mx_status_type mxd_mcs_encoder_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mcs_encoder_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_mcs_encoder_read( MX_MCE *mce );
MX_API mx_status_type mxd_mcs_encoder_get_current_num_values( MX_MCE *mce );

extern MX_RECORD_FUNCTION_LIST mxd_mcs_encoder_record_function_list;
extern MX_MCE_FUNCTION_LIST mxd_mcs_encoder_mce_function_list;

extern mx_length_type mxd_mcs_encoder_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mcs_encoder_rfield_def_ptr;

#endif /* __D_MCS_MCE_H__ */


/*
 * Name:    d_pmac_mce.h
 *
 * Purpose: Header file for MX multichannel encoder driver to record
 *          a PMAC motor's position in two channels of a multichannel encoder.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PMAC_MCE_H__
#define __D_PMAC_MCE_H__

#include "mx_mce.h"

/* ===== MCS mce data structure ===== */

typedef struct {
	MX_RECORD *pmac_record;
	int card_number;

	MX_RECORD *mcs_record;
	unsigned long down_channel;
	unsigned long up_channel;
	int plc_program_number;

	MX_RECORD *selected_motor_record;
} MX_PMAC_MCE;

#define MXD_PMAC_MCE_STANDARD_FIELDS \
  {-1, -1, "pmac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_MCE, pmac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "card_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_MCE, card_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "mcs_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_MCE, mcs_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "down_channel", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_MCE, down_channel), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "up_channel", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_MCE, up_channel), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "plc_program_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_MCE, plc_program_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \

/* Define all of the interface functions. */

MX_API mx_status_type mxd_pmac_mce_initialize_type( long type );
MX_API mx_status_type mxd_pmac_mce_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pmac_mce_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_pmac_mce_delete_record( MX_RECORD *record );

MX_API mx_status_type mxd_pmac_mce_read( MX_MCE *mce );
MX_API mx_status_type mxd_pmac_mce_get_current_num_values( MX_MCE *mce );
MX_API mx_status_type mxd_pmac_mce_connect_mce_to_motor( MX_MCE *mce,
						MX_RECORD *motor_record );

extern MX_RECORD_FUNCTION_LIST mxd_pmac_mce_record_function_list;
extern MX_MCE_FUNCTION_LIST mxd_pmac_mce_mce_function_list;

extern mx_length_type mxd_pmac_mce_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmac_mce_rfield_def_ptr;

#endif /* __D_PMAC_MCE_H__ */


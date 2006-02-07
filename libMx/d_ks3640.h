/*
 * Name:    d_ks3640.h
 *
 * Purpose: Header file for MX encoder driver to control a Kinetic Systems
 *          model 3640 up/down counter.  The up/down counter is treated
 *          as an incremental encoder.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KS3640_H__
#define __D_KS3640_H__

#include "mx_encoder.h"

/* ===== KS3640 up/down counter data structure ===== */

typedef struct {
	MX_RECORD *camac_record;
	int32_t slot;
	int32_t subaddress;
	bool use_32bit_mod;
} MX_KS3640;

/* The 32 bit modification flagged by 'use_32bit_mod' is a field modification
 * of the circuit board that combines the 4 16-bit channels into 2 32-bit
 * channels.
 */

/* Device flags. */

#define MXF_KS3640_32BIT_MODIFICATION	1

/* Define all of the interface functions. */

MX_API mx_status_type mxd_ks3640_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_ks3640_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_ks3640_read_parms_from_hardware( MX_RECORD *record );
MX_API mx_status_type mxd_ks3640_write_parms_to_hardware( MX_RECORD *record );
MX_API mx_status_type mxd_ks3640_open( MX_RECORD *record );

MX_API mx_status_type mxd_ks3640_get_overflow_status( MX_ENCODER *encoder );
MX_API mx_status_type mxd_ks3640_reset_overflow_status( MX_ENCODER *encoder );
MX_API mx_status_type mxd_ks3640_read( MX_ENCODER *encoder );
MX_API mx_status_type mxd_ks3640_write( MX_ENCODER *encoder );

extern MX_RECORD_FUNCTION_LIST mxd_ks3640_record_function_list;
extern MX_ENCODER_FUNCTION_LIST mxd_ks3640_encoder_function_list;

extern mx_length_type mxd_ks3640_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ks3640_rfield_def_ptr;

#define MXD_KS3640_STANDARD_FIELDS \
  {-1, -1, "camac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3640, camac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "slot", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3640, slot), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "subaddress", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3640, subaddress), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "use_32bit_mod", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3640, use_32bit_mod), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_KS3640_H__ */

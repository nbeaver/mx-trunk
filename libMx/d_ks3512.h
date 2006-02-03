/*
 * Name:    d_ks3512.h
 *
 * Purpose: Header file for MX input drivers to control KS3512 analog input.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KS3512_H__
#define __D_KS3512_H__

#include "mx_analog_input.h"

/* ===== KS3512 analog input register data structure ===== */

typedef struct {
	MX_RECORD *camac_record;
	int slot;
	int subaddress;
} MX_KS3512;

#define MXD_KS3512_STANDARD_FIELDS \
  {-1, -1, "camac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3512, camac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "slot", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3512, slot), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "subaddress", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3512, subaddress), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_ks3512_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxd_ks3512_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_ks3512_read( MX_ANALOG_INPUT *adc );

extern MX_RECORD_FUNCTION_LIST mxd_ks3512_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST mxd_ks3512_analog_input_function_list;

extern mx_length_type mxd_ks3512_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ks3512_rfield_def_ptr;

#endif /* __D_KS3512_H__ */


/*
 * Name:    d_udt_tramp.h
 *
 * Purpose: Header file for MX driver for the UDT Instruments TRAMP
 *          current amplifier.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_UDT_TRAMP_H__
#define __D_UDT_TRAMP_H__

#include "mx_amplifier.h"

typedef struct {
	MX_RECORD *doutput_record;
} MX_UDT_TRAMP;

MX_API mx_status_type mxd_udt_tramp_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_udt_tramp_open( MX_RECORD *record );

MX_API mx_status_type mxd_udt_tramp_get_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_udt_tramp_set_gain( MX_AMPLIFIER *amplifier );

extern MX_RECORD_FUNCTION_LIST mxd_udt_tramp_record_function_list;
extern MX_AMPLIFIER_FUNCTION_LIST mxd_udt_tramp_amplifier_function_list;

extern mx_length_type mxd_udt_tramp_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_udt_tramp_rfield_def_ptr;

#define MXD_UDT_TRAMP_STANDARD_FIELDS \
  {-1, -1, "doutput_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UDT_TRAMP, doutput_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_UDT_TRAMP_H__ */

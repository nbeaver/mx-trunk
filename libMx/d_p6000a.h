/*
 * Name:    d_p6000a.h
 *
 * Purpose: Header file for reading measurements from a Newport Electronics
 *          P6000A analog input.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_P6000A_H__
#define __D_P6000A_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *rs232_record;
} MX_P6000A;

MX_API mx_status_type mxd_p6000a_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_p6000a_open( MX_RECORD *record );
MX_API mx_status_type mxd_p6000a_close( MX_RECORD *record );

MX_API mx_status_type mxd_p6000a_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_p6000a_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_p6000a_analog_input_function_list;

extern mx_length_type mxd_p6000a_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_p6000a_rfield_def_ptr;

#define MXD_P6000A_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_P6000A, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_P6000A_H__ */

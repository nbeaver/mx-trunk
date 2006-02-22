/*
 * Name:    d_lpt.h
 *
 * Purpose: Header file for MX input and output drivers to use 
 *          PC compatible LPT printer ports for digital I/O
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_LPT_H__
#define __D_LPT_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== LPT digital I/O data structures ===== */

typedef struct {
	MX_RECORD *interface_record;
	char port_name[MX_LPT_MAX_PORT_NAME_LENGTH + 1];
	int port_number;
} MX_LPT_IN;

typedef struct {
	MX_RECORD *interface_record;
	char port_name[MX_LPT_MAX_PORT_NAME_LENGTH + 1];
	int port_number;
} MX_LPT_OUT;

#define MXD_LPT_IN_STANDARD_FIELDS \
  {-1, -1, "interface_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_LPT_IN, interface_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_name", MXFT_STRING, NULL, 1, {MX_LPT_MAX_PORT_NAME_LENGTH}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_LPT_IN, port_name), \
        {sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_LPT_OUT_STANDARD_FIELDS \
  {-1, -1, "interface_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_LPT_OUT, interface_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_name", MXFT_STRING, NULL, 1, {MX_LPT_MAX_PORT_NAME_LENGTH}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_LPT_OUT, port_name), \
        {sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_lpt_in_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_lpt_in_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_lpt_in_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_lpt_in_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST mxd_lpt_in_digital_input_function_list;

extern long mxd_lpt_in_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_lpt_in_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_lpt_out_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_lpt_out_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_lpt_out_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_lpt_out_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_lpt_out_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_lpt_out_digital_output_function_list;

extern long mxd_lpt_out_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_lpt_out_rfield_def_ptr;

#endif /* __D_LPT_H__ */


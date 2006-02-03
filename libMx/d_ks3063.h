/*
 * Name:    d_ks3063.h
 *
 * Purpose: Header file for MX input and output drivers to control KS3063
 *          16-bit input gate/output registers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KS3063_H__
#define __D_KS3063_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== KS3063 input gate/output register data structure ===== */

typedef struct {
	MX_RECORD *camac_record;
	int slot;
} MX_KS3063_IN;

typedef struct {
	MX_RECORD *camac_record;
	int slot;
} MX_KS3063_OUT;

#define MXD_KS3063_IN_STANDARD_FIELDS \
  {-1, -1, "camac_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_KS3063_IN, camac_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "slot", MXFT_INT, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_KS3063_IN, slot), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_KS3063_OUT_STANDARD_FIELDS \
  {-1, -1, "camac_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_KS3063_OUT, camac_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "slot", MXFT_INT, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_KS3063_OUT, slot), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_ks3063_in_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_ks3063_in_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_ks3063_in_print_structure( FILE *file,
							MX_RECORD *record );

MX_API mx_status_type mxd_ks3063_in_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_ks3063_in_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_ks3063_in_digital_input_function_list;

extern mx_length_type mxd_ks3063_in_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ks3063_in_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_ks3063_out_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_ks3063_out_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_ks3063_out_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_ks3063_out_open( MX_RECORD *record );

MX_API mx_status_type mxd_ks3063_out_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_ks3063_out_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_ks3063_out_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_ks3063_out_digital_output_function_list;

extern mx_length_type mxd_ks3063_out_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ks3063_out_rfield_def_ptr;

#endif /* __D_KS3063_H__ */


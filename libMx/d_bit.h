/*
 * Name:    d_bit.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          bit ranges in digital input and output records.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BIT_H__
#define __D_BIT_H__

typedef struct {
	mx_bool_type invert;

	MX_RECORD *input_record;
	long input_bit_offset;
	long num_input_bits;

	unsigned long input_mask;
} MX_BIT_IN;

typedef struct {
	mx_bool_type invert;

	MX_RECORD *input_record;
	long input_bit_offset;
	long num_input_bits;

	MX_RECORD *output_record;
	long output_bit_offset;
	long num_output_bits;

	unsigned long input_mask;
	unsigned long output_mask;
} MX_BIT_OUT;

#define MXD_BIT_IN_STANDARD_FIELDS \
  {-1, -1, "invert", MXFT_BOOL, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_IN, invert), \
        {0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "input_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_IN, input_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "input_bit_offset", MXFT_LONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_IN, input_bit_offset), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_input_bits", MXFT_LONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_IN, num_input_bits), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_BIT_OUT_STANDARD_FIELDS \
  {-1, -1, "invert", MXFT_BOOL, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_OUT, invert), \
        {0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "input_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_OUT, input_record), \
        {0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "input_bit_offset", MXFT_LONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_OUT, input_bit_offset), \
        {0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_input_bits", MXFT_LONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_OUT, num_input_bits), \
        {0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "output_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_OUT, output_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "output_bit_offset", MXFT_LONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_OUT, output_bit_offset), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_output_bits", MXFT_LONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_BIT_OUT, num_output_bits), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First, the input functions. */

MX_API mx_status_type mxd_bit_in_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bit_in_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_bit_in_print_structure( FILE *file,
							MX_RECORD *record );

MX_API mx_status_type mxd_bit_in_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_bit_in_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_bit_in_digital_input_function_list;

extern long mxd_bit_in_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bit_in_rfield_def_ptr;

/* Second, the output functions. */

MX_API mx_status_type mxd_bit_out_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bit_out_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_bit_out_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_bit_out_open( MX_RECORD *record );

MX_API mx_status_type mxd_bit_out_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_bit_out_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_bit_out_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_bit_out_digital_output_function_list;

extern long mxd_bit_out_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bit_out_rfield_def_ptr;

#endif /* __D_BIT_H__ */


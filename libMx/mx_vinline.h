/*
 * Name:    mx_vinline.h
 *
 * Purpose: Header file for inline variables in MX database description files.
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

#ifndef __MX_VINLINE_H__
#define __MX_VINLINE_H__

MX_API_PRIVATE mx_status_type mxv_inline_variable_initialize_type( long );
MX_API_PRIVATE mx_status_type mxv_inline_variable_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_inline_variable_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_inline_variable_delete_record(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_inline_variable_dummy_function(
							MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxv_inline_variable_record_function_list;

/* The various different kinds of inline variables all have the same
 * number of record fields, so I only use one variable for the number
 * of fields.
 */

extern mx_length_type mxv_inline_string_variable_num_record_fields;
extern mx_length_type mxv_inline_char_variable_num_record_fields;
extern mx_length_type mxv_inline_uchar_variable_num_record_fields;
extern mx_length_type mxv_inline_int8_variable_num_record_fields;
extern mx_length_type mxv_inline_uint8_variable_num_record_fields;
extern mx_length_type mxv_inline_int16_variable_num_record_fields;
extern mx_length_type mxv_inline_uint16_variable_num_record_fields;
extern mx_length_type mxv_inline_int32_variable_num_record_fields;
extern mx_length_type mxv_inline_uint32_variable_num_record_fields;
extern mx_length_type mxv_inline_int64_variable_num_record_fields;
extern mx_length_type mxv_inline_uint64_variable_num_record_fields;
extern mx_length_type mxv_inline_float_variable_num_record_fields;
extern mx_length_type mxv_inline_double_variable_num_record_fields;
extern mx_length_type mxv_inline_record_variable_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_string_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_char_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_uchar_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_int8_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_uint8_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_int16_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_uint16_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_int32_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_uint32_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_int64_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_uint64_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_float_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_double_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_inline_record_variable_def_ptr;

#endif /* __MX_VINLINE_H__ */


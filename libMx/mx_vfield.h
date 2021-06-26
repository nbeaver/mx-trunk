/*
 * Name:    mx_vfield.h
 *
 * Purpose: Header file for local field variables in MX database files.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright 2012, 2015, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_VFIELD_H__
#define __MX_VFIELD_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Available flag bits for the 'external_field_flags' field. */

#define MXF_FIELD_VARIABLE_READ_ONLY		0x1
#define MXF_FIELD_VARIABLE_WRITE_ON_OPEN	0x2

typedef struct {
	MX_RECORD *record;
	MX_RECORD_FIELD *internal_field;

	char external_field_name[ MXU_RECORD_FIELD_NAME_LENGTH+1 ];
	unsigned long external_field_flags;

	MX_RECORD *external_record;
	MX_RECORD_FIELD *external_field;

	long original_datatype;
	long original_num_dimensions;
	long *original_dimension;
	size_t *original_data_element_size;
	void *original_data_pointer;
	void *original_value_pointer;
} MX_FIELD_VARIABLE;

#define MX_FIELD_VARIABLE_STANDARD_FIELDS \
  {-1, -1, "external_field_name", MXFT_STRING, \
		NULL, 1, {MXU_RECORD_FIELD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_FIELD_VARIABLE,external_field_name),\
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "external_field_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_FIELD_VARIABLE,external_field_flags),\
	{0}, NULL, MXFF_IN_DESCRIPTION }

MX_API_PRIVATE mx_status_type mxv_field_variable_initialize_driver(
							MX_DRIVER *driver );
MX_API_PRIVATE mx_status_type mxv_field_variable_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_field_variable_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_field_variable_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_field_variable_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_field_variable_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_field_variable_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_field_variable_variable_function_list;

/* The various different kinds of local field variables all have the same
 * number of record fields, so I only use one variable for the number
 * of fields.
 */

extern long mxv_field_string_variable_num_record_fields;
extern long mxv_field_char_variable_num_record_fields;
extern long mxv_field_uchar_variable_num_record_fields;
extern long mxv_field_int8_variable_num_record_fields;
extern long mxv_field_uint8_variable_num_record_fields;
extern long mxv_field_short_variable_num_record_fields;
extern long mxv_field_ushort_variable_num_record_fields;
extern long mxv_field_bool_variable_num_record_fields;
extern long mxv_field_long_variable_num_record_fields;
extern long mxv_field_ulong_variable_num_record_fields;
extern long mxv_field_int64_variable_num_record_fields;
extern long mxv_field_uint64_variable_num_record_fields;
extern long mxv_field_float_variable_num_record_fields;
extern long mxv_field_double_variable_num_record_fields;
extern long mxv_field_hex_variable_num_record_fields;
extern long mxv_field_record_variable_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_field_string_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_char_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_uchar_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_int8_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_uint8_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_short_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_ushort_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_bool_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_long_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_ulong_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_int64_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_uint64_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_float_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_double_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_hex_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_field_record_variable_dptr;

#ifdef __cplusplus
}
#endif

#endif /* __MX_VFIELD_H__ */


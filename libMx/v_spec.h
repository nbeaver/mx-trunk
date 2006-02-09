/*
 * Name:    v_spec.h
 *
 * Purpose: Header file for spec property variables.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_SPEC_H__
#define __V_SPEC_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *spec_server_record;
	char property_name[ SV_NAME_LEN + 1 ];
} MX_SPEC_PROPERTY;

#define MXV_SPEC_PROPERTY_STANDARD_FIELDS \
  {-1, -1, "spec_server_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_PROPERTY, spec_server_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "property_name", MXFT_STRING, NULL, 1, {SV_NAME_LEN}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_PROPERTY, property_name), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API_PRIVATE mx_status_type mxv_spec_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_spec_send_variable( MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_spec_receive_variable( MX_VARIABLE *variable);

extern MX_RECORD_FUNCTION_LIST mxv_spec_property_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_spec_property_variable_function_list;

extern mx_length_type mxv_spec_int32_num_record_fields;
extern mx_length_type mxv_spec_uint32_num_record_fields;
extern mx_length_type mxv_spec_int16_num_record_fields;
extern mx_length_type mxv_spec_uint16_num_record_fields;
extern mx_length_type mxv_spec_int8_num_record_fields;
extern mx_length_type mxv_spec_uint8_num_record_fields;
extern mx_length_type mxv_spec_float_num_record_fields;
extern mx_length_type mxv_spec_double_num_record_fields;
extern mx_length_type mxv_spec_char_num_record_fields;
extern mx_length_type mxv_spec_string_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_spec_int32_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_spec_uint32_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_spec_int16_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_spec_uint16_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_spec_int8_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_spec_uint8_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_spec_float_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_spec_double_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_spec_char_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_spec_string_rfield_def_ptr;

#endif /* __V_SPEC_H__ */


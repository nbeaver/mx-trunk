/*
 * Name:    v_indirect_string.h
 *
 * Purpose: Header file for a string constructed from record field values.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_INDIRECT_STRING_H__
#define __V_INDIRECT_STRING_H__

#define MXU_INDIRECT_STRING_FORMAT_LENGTH	250

typedef struct {
	char format[ MXU_INDIRECT_STRING_FORMAT_LENGTH+1 ];
	long num_fields;
	MX_RECORD_FIELD **record_field_array;
} MX_INDIRECT_STRING;

#define MX_INDIRECT_STRING_STANDARD_FIELDS \
  {-1, -1, "format", MXFT_STRING, NULL, 1, {MXU_INDIRECT_STRING_FORMAT_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_INDIRECT_STRING, format), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_fields", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_INDIRECT_STRING, num_fields), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "record_field_array", MXFT_RECORD_FIELD, NULL, \
						1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_INDIRECT_STRING, record_field_array), \
	{sizeof(MX_RECORD_FIELD *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

MX_API_PRIVATE mx_status_type mxv_indirect_string_initialize_driver(
							MX_DRIVER *driver );
MX_API_PRIVATE mx_status_type mxv_indirect_string_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_indirect_string_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_indirect_string_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_indirect_string_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_indirect_string_variable_function_list;

extern long mxv_indirect_string_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_indirect_string_rfield_def_ptr;

#endif /* __V_INDIRECT_STRING_H__ */


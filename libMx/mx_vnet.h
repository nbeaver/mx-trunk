/*
 * Name:    mx_vnet.h
 *
 * Purpose: Header file for network variables in MX database description files.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_VNET_H__
#define __MX_VNET_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_RECORD *server_record;
	char remote_field_name[ MXU_RECORD_FIELD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD value_nf;
} MX_NETWORK_VARIABLE;

#define MX_NETWORK_VARIABLE_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_NETWORK_VARIABLE, server_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "remote_field_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_FIELD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
	offsetof(MX_NETWORK_VARIABLE, remote_field_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }

MX_API_PRIVATE mx_status_type mxv_network_variable_initialize_type( long );
MX_API_PRIVATE mx_status_type mxv_network_variable_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_network_variable_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_network_variable_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_network_variable_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_network_variable_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_network_variable_variable_function_list;

/* The various different kinds of network variables all have the same
 * number of record fields, so I only use one variable for the number
 * of fields.
 */

extern long mxv_network_string_variable_num_record_fields;
extern long mxv_network_char_variable_num_record_fields;
extern long mxv_network_uchar_variable_num_record_fields;
extern long mxv_network_short_variable_num_record_fields;
extern long mxv_network_ushort_variable_num_record_fields;
extern long mxv_network_bool_variable_num_record_fields;
extern long mxv_network_long_variable_num_record_fields;
extern long mxv_network_ulong_variable_num_record_fields;
extern long mxv_network_int64_variable_num_record_fields;
extern long mxv_network_uint64_variable_num_record_fields;
extern long mxv_network_float_variable_num_record_fields;
extern long mxv_network_double_variable_num_record_fields;
extern long mxv_network_record_variable_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_network_string_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_char_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_uchar_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_short_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_ushort_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_bool_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_long_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_ulong_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_int64_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_uint64_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_float_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_double_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_network_record_variable_dptr;

#ifdef __cplusplus
}
#endif

#endif /* __MX_VNET_H__ */


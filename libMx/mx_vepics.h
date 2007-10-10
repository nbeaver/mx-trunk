/*
 * Name:    mx_vepics.h
 *
 * Purpose: Header file for EPICS variables in MX database description files.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_VEPICS_H__
#define __MX_VEPICS_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char pvname[MXU_EPICS_PVNAME_LENGTH+1];
	long epics_type;
	long num_elements;

	MX_EPICS_PV pv;
} MX_EPICS_VARIABLE;

#define MX_EPICS_VARIABLE_STANDARD_FIELDS \
  {-1, -1, "pvname", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_EPICS_VARIABLE, pvname), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "epics_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_EPICS_VARIABLE, epics_type), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "num_elements", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_EPICS_VARIABLE, num_elements), \
	{0}, NULL, 0}

MX_API_PRIVATE mx_status_type mxv_epics_variable_initialize_type( long );
MX_API_PRIVATE mx_status_type mxv_epics_variable_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_epics_variable_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_epics_variable_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_epics_variable_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_epics_variable_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_epics_variable_variable_function_list;

/* The various different kinds of EPICS variables all have the same
 * number of record fields, so I only use one variable for the number
 * of fields.
 */

extern long mxv_epics_string_variable_num_record_fields;
extern long mxv_epics_char_variable_num_record_fields;
extern long mxv_epics_uchar_variable_num_record_fields;
extern long mxv_epics_short_variable_num_record_fields;
extern long mxv_epics_ushort_variable_num_record_fields;
extern long mxv_epics_int_variable_num_record_fields;
extern long mxv_epics_uint_variable_num_record_fields;
extern long mxv_epics_long_variable_num_record_fields;
extern long mxv_epics_ulong_variable_num_record_fields;
extern long mxv_epics_float_variable_num_record_fields;
extern long mxv_epics_double_variable_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_string_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_char_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_uchar_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_short_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_ushort_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_int_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_uint_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_long_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_ulong_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_float_variable_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_double_variable_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __MX_VEPICS_H__ */


/*
 * Name:    v_flowbus.h
 *
 * Purpose: Header file for Bronkhorst FLOW-BUS parameters.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_FLOWBUS_H__
#define __V_FLOWBUS_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *flowbus_record;
	unsigned long node_address;
	unsigned long process_number;
	unsigned long parameter_number;
	unsigned long parameter_type;
} MX_FLOWBUS_PARAMETER;

#define MXV_FLOWBUS_PARAMETER_STANDARD_FIELDS \
  {-1, -1, "flowbus_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_PARAMETER, flowbus_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "node_address", MXFT_ULONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_PARAMETER, node_address), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "process_number", MXFT_ULONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_PARAMETER, process_number), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "parameter_number", MXFT_ULONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_PARAMETER, parameter_number), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API_PRIVATE mx_status_type mxv_flowbus_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_flowbus_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_flowbus_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_flowbus_receive_variable(
						MX_VARIABLE *variable);

extern MX_RECORD_FUNCTION_LIST mxv_flowbus_parameter_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_flowbus_parameter_variable_function_list;

extern long mxv_flowbus_string_num_record_fields;
extern long mxv_flowbus_uchar_num_record_fields;
extern long mxv_flowbus_ushort_num_record_fields;
extern long mxv_flowbus_ulong_num_record_fields;
extern long mxv_flowbus_float_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_string_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_uchar_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_ushort_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_ulong_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_float_rfield_def_ptr;

#endif /* __V_FLOWBUS_H__ */


/*
 * Name:    v_flowbus_capacity.h
 *
 * Purpose: Header file for Bronkhorst FLOW-BUS fluid capacity values.
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

#ifndef __V_FLOWBUS_CAPACITY_H__
#define __V_FLOWBUS_CAPACITY_H__ 

#define MXF_FLOWBUS_CAPACITY_READ	0x1
#define MXF_FLOWBUS_CAPACITY_WRITE	0x2

#define MXU_FLOWBUS_CAPACITY_LENGTH	7

typedef struct {
	MX_RECORD *record;

	MX_RECORD *flowbus_record;
	unsigned long node_address;
	char units[ MXU_FLOWBUS_CAPACITY_LENGTH+1 ];
	unsigned long access_mode;
} MX_FLOWBUS_CAPACITY;

#define MXV_FLOWBUS_CAPACITY_STANDARD_FIELDS \
  {-1, -1, "flowbus_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_CAPACITY, flowbus_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "node_address", MXFT_ULONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_CAPACITY, node_address), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "units", MXFT_STRING, NULL, 1, {MXU_FLOWBUS_CAPACITY_LENGTH}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_CAPACITY, units), \
        {sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "access_mode", MXFT_HEX, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_CAPACITY, access_mode), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API_PRIVATE mx_status_type mxv_flowbus_capacity_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_flowbus_capacity_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_flowbus_capacity_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_flowbus_capacity_receive_variable(
						MX_VARIABLE *variable);

extern MX_RECORD_FUNCTION_LIST mxv_flowbus_capacity_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_flowbus_capacity_variable_function_list;

extern long mxv_flowbus_capacity_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_capacity_rfield_def_ptr;

#endif /* __V_FLOWBUS_H__ */


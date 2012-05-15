/*
 * Name:    v_u500.h
 *
 * Purpose: Header file for Aerotech U500 V variables.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_U500_H__
#define __V_U500_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *u500_record;
	long board_number;
	unsigned long variable_number;

	double *mx_vptr;
} MX_U500_VARIABLE;

#define MXV_U500_VARIABLE_STANDARD_FIELDS \
  {-1, -1, "u500_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_U500_VARIABLE, u500_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "board_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_VARIABLE, board_number), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "variable_number", MXFT_ULONG, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_U500_VARIABLE, variable_number), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API_PRIVATE mx_status_type mxv_u500_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_u500_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_u500_send_variable( MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_u500_receive_variable( MX_VARIABLE *variable);

extern MX_RECORD_FUNCTION_LIST mxv_u500_variable_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_u500_variable_variable_function_list;

extern long mxv_u500_variable_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_u500_variable_rfield_def_ptr;

#endif /* __V_U500_H__ */


/*
 * Name:    v_powerpmac.h
 *
 * Purpose: Header file for POWERPMAC motor controller variables.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_POWERPMAC_H__
#define __V_POWERPMAC_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *powerpmac_record;
	char powerpmac_variable_name[ MXU_POWERPMAC_VARIABLE_NAME_LENGTH + 1 ];
} MX_POWERPMAC_VARIABLE;

#define MXV_POWERPMAC_VARIABLE_STANDARD_FIELDS \
  {-1, -1, "powerpmac_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_VARIABLE, powerpmac_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "powerpmac_variable_name", MXFT_STRING, NULL, \
                                1, {MXU_POWERPMAC_VARIABLE_NAME_LENGTH}, \
        MXF_REC_TYPE_STRUCT, \
		offsetof(MX_POWERPMAC_VARIABLE, powerpmac_variable_name), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API_PRIVATE mx_status_type mxv_powerpmac_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_powerpmac_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_powerpmac_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_powerpmac_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_powerpmac_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_powerpmac_variable_function_list;

extern long mxv_powerpmac_long_num_record_fields;
extern long mxv_powerpmac_ulong_num_record_fields;
extern long mxv_powerpmac_double_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_powerpmac_long_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_powerpmac_ulong_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_powerpmac_double_rfield_def_ptr;

#endif /* __V_POWERPMAC_H__ */


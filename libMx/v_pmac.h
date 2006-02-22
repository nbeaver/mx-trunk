/*
 * Name:    v_pmac.h
 *
 * Purpose: Header file for PMAC motor controller variables.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_PMAC_H__
#define __V_PMAC_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pmac_record;
	int card_number;
	char pmac_variable_name[ MXU_PMAC_VARIABLE_NAME_LENGTH + 1 ];
} MX_PMAC_VARIABLE;

#define MXV_PMAC_VARIABLE_STANDARD_FIELDS \
  {-1, -1, "pmac_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_VARIABLE, pmac_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "card_number", MXFT_INT, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_VARIABLE, card_number), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pmac_variable_name", MXFT_STRING, NULL, \
                                1, {MXU_PMAC_VARIABLE_NAME_LENGTH}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_VARIABLE, pmac_variable_name), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API_PRIVATE mx_status_type mxv_pmac_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_pmac_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_pmac_send_variable( MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_pmac_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_pmac_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_pmac_variable_function_list;

extern long mxv_pmac_long_num_record_fields;
extern long mxv_pmac_ulong_num_record_fields;
extern long mxv_pmac_double_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_pmac_long_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_pmac_ulong_rfield_def_ptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_pmac_double_rfield_def_ptr;

#endif /* __V_PMAC_H__ */


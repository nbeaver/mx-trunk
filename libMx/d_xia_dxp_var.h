/*
 * Name:    d_xia_dxp_var.h
 *
 * Purpose: Header file for XIA DXP derived variables.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _D_XIA_DXP_VAR_H_
#define _D_XIA_DXP_VAR_H_

#define MXU_XIA_DXP_VARIABLE_PARAMETER_LENGTH	80

#define MXT_XIA_DXP_VAR_PARAMETER			1
#define MXT_XIA_DXP_VAR_ROI_INTEGRAL			2
#define MXT_XIA_DXP_VAR_RATE_CORRECTED_ROI_INTEGRAL	3
#define MXT_XIA_DXP_VAR_REALTIME			4
#define MXT_XIA_DXP_VAR_LIVETIME			5
#define MXT_XIA_DXP_VAR_INPUT_COUNT_RATE		6
#define MXT_XIA_DXP_VAR_OUTPUT_COUNT_RATE		7
#define MXT_XIA_DXP_VAR_NUM_FAST_PEAKS			8
#define MXT_XIA_DXP_VAR_NUM_EVENTS			9
#define MXT_XIA_DXP_VAR_NUM_UNDERFLOWS			10
#define MXT_XIA_DXP_VAR_NUM_OVERFLOWS			11
#define MXT_XIA_DXP_VAR_LIVETIME_CORRECTED_ROI_INTEGRAL	12

typedef struct {
	MX_RECORD *mca_record;
	int value_type;
	char value_parameters[ MXU_XIA_DXP_VARIABLE_PARAMETER_LENGTH + 1 ];
} MX_XIA_DXP_INPUT;

#define MX_XIA_DXP_INPUT_STANDARD_FIELDS \
  {-1, -1, "mca_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_INPUT, mca_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "value_type", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_INPUT, value_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "value_parameters", MXFT_STRING, NULL, \
				1, {MXU_XIA_DXP_VARIABLE_PARAMETER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_INPUT, value_parameters), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxd_xia_dxp_input_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_xia_dxp_input_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_xia_dxp_input_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_xia_dxp_input_analog_input_function_list;

extern long mxd_xia_dxp_input_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_xia_dxp_input_rfield_def_ptr;

#endif /* _D_XIA_DXP_VAR_H_ */

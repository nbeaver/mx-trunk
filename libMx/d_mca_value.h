/*
 * Name:    d_mca_value.h
 *
 * Purpose: Header file for MCA derived values.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _D_MCA_VALUE_H_
#define _D_MCA_VALUE_H_

#define MXU_MCA_VALUE_NAME_LENGTH	40
#define MXU_MCA_VALUE_PARAMETER_LENGTH	80

#define MXT_MCA_VALUE_ROI_INTEGRAL			1
#define MXT_MCA_VALUE_SOFT_ROI_INTEGRAL			2
#define MXT_MCA_VALUE_REAL_TIME				3
#define MXT_MCA_VALUE_LIVE_TIME				4
#define MXT_MCA_VALUE_CORRECTED_ROI_INTEGRAL		5
#define MXT_MCA_VALUE_CORRECTED_SOFT_ROI_INTEGRAL	6
#define MXT_MCA_VALUE_INPUT_COUNT_RATE			7
#define MXT_MCA_VALUE_OUTPUT_COUNT_RATE			8

typedef struct {
	MX_RECORD *mca_record;
	int32_t value_type;
	char value_name[ MXU_MCA_VALUE_NAME_LENGTH + 1 ];
	char value_parameters[ MXU_MCA_VALUE_PARAMETER_LENGTH + 1 ];
} MX_MCA_VALUE;

#define MX_MCA_VALUE_STANDARD_FIELDS \
  {-1, -1, "mca_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_VALUE, mca_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "value_type", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_VALUE, value_type), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "value_name", MXFT_STRING, NULL, 1, {MXU_MCA_VALUE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_VALUE, value_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "value_parameters", MXFT_STRING, NULL, \
				1, {MXU_MCA_VALUE_PARAMETER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_VALUE, value_parameters), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxd_mca_value_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_mca_value_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_mca_value_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_mca_value_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST mxd_mca_value_analog_input_function_list;

extern mx_length_type mxd_mca_value_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mca_value_rfield_def_ptr;

#endif /* _D_MCA_VALUE_H_ */


/*
 * Name:    d_handel_input.h
 *
 * Purpose: Header file for XIA Handel-derived variables.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2004, 2006, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _D_HANDEL_INPUT_H_
#define _D_HANDEL_INPUT_H_

#define MXU_HANDEL_INPUT_PARAMETER_LENGTH		80

#define MXT_HANDEL_INPUT_PARAMETER			1
#define MXT_HANDEL_INPUT_ROI_INTEGRAL			2
#define MXT_HANDEL_INPUT_RATE_CORRECTED_ROI_INTEGRAL	3
#define MXT_HANDEL_INPUT_REALTIME			4
#define MXT_HANDEL_INPUT_LIVETIME			5
#define MXT_HANDEL_INPUT_INPUT_COUNT_RATE		6
#define MXT_HANDEL_INPUT_OUTPUT_COUNT_RATE		7
#define MXT_HANDEL_INPUT_NUM_TRIGGERS			8
#define MXT_HANDEL_INPUT_NUM_EVENTS			9
#define MXT_HANDEL_INPUT_NUM_UNDERFLOWS			10
#define MXT_HANDEL_INPUT_NUM_OVERFLOWS			11
#define MXT_HANDEL_INPUT_LIVETIME_CORRECTED_ROI_INTEGRAL 12
#define MXT_HANDEL_INPUT_ACQUISITION_VALUE		13

typedef struct {
	MX_RECORD *mca_record;
	long value_type;
	char value_parameters[ MXU_HANDEL_INPUT_PARAMETER_LENGTH + 1 ];
} MX_HANDEL_INPUT;

#define MX_HANDEL_INPUT_STANDARD_FIELDS \
  {-1, -1, "mca_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_INPUT, mca_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "value_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_INPUT, value_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "value_parameters", MXFT_STRING, NULL, \
				1, {MXU_HANDEL_INPUT_PARAMETER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_INPUT, value_parameters), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxd_handel_input_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_handel_input_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_handel_input_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_handel_input_analog_input_function_list;

extern long mxd_handel_input_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_handel_input_rfield_def_ptr;

#endif /* _D_HANDEL_INPUT_H_ */

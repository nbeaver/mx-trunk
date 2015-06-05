/*
 * Name:    d_nuvant_ezware2_ainput.h
 *
 * Purpose: Header file for NuVant EZWare2 analog input channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NUVANT_EZWARE2_AINPUT_H__
#define __D_NUVANT_EZWARE2_AINPUT_H__

#include "mx_analog_input.h"

#define MXU_NUVANT_EZWARE2_AINPUT_TYPE_LENGTH	80

#define MXT_NE_AINPUT_AI0			0
#define MXT_NE_AINPUT_AI1			1
#define MXT_NE_AINPUT_AI2			2
#define MXT_NE_AINPUT_AI3			3

#define MXT_NE_AINPUT_POTENTIOSTAT		100
#define MXT_NE_AINPUT_GALVANOSTAT		101

typedef struct {
	MX_RECORD *record;

	MX_RECORD *nuvant_ezware2_record;
	char input_type_name[MXU_NUVANT_EZWARE2_AINPUT_TYPE_LENGTH+1];

	unsigned long input_type;

} MX_NUVANT_EZWARE2_AINPUT;

MX_API mx_status_type mxd_nuvant_ezware2_ainput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_nuvant_ezware2_ainput_open( MX_RECORD *record );

MX_API mx_status_type mxd_nuvant_ezware2_ainput_read(MX_ANALOG_INPUT *ainput);

extern MX_RECORD_FUNCTION_LIST mxd_nuvant_ezware2_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_nuvant_ezware2_ainput_analog_input_function_list;

extern long mxd_nuvant_ezware2_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_nuvant_ezware2_ainput_rfield_def_ptr;

#define MXD_NUVANT_EZWARE2_AINPUT_STANDARD_FIELDS \
  {-1, -1, "nuvant_ezware2_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NUVANT_EZWARE2_AINPUT, nuvant_ezware2_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "input_type_name", MXFT_STRING, \
			NULL, 1, {MXU_NUVANT_EZWARE2_AINPUT_TYPE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_NUVANT_EZWARE2_AINPUT, input_type_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_NUVANT_EZWARE2_AINPUT_H__ */


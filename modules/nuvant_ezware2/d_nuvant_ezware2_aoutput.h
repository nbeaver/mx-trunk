/*
 * Name:    d_nuvant_ezware2_aoutput.h
 *
 * Purpose: Header file for NuVant EZWare2 analog output channels.
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

#ifndef __D_NUVANT_EZWARE2_AOUTPUT_H__
#define __D_NUVANT_EZWARE2_AOUTPUT_H__

#include "mx_analog_output.h"

#define MXU_NUVANT_EZWARE2_AOUTPUT_TYPE_LENGTH	40

#define MXT_NUVANT_EZWARE2_AOUTPUT_POTENTIOSTAT_VOLTAGE		1
#define MXT_NUVANT_EZWARE2_AOUTPUT_GALVANOSTAT_CURRENT		2
#define MXT_NUVANT_EZWARE2_AOUTPUT_POTENTIOSTAT_CURRENT_RANGE	3
#define MXT_NUVANT_EZWARE2_AOUTPUT_GALVANOSTAT_CURRENT_RANGE	4

typedef struct {
	MX_RECORD *record;

	MX_RECORD *nuvant_ezware2_record;
	char output_type_name[MXU_NUVANT_EZWARE2_AOUTPUT_TYPE_LENGTH+1];

	unsigned long output_type;

} MX_NUVANT_EZWARE2_AOUTPUT;

MX_API mx_status_type mxd_nuvant_ezware2_aoutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_nuvant_ezware2_aoutput_open( MX_RECORD *record );

MX_API mx_status_type mxd_nuvant_ezware2_aoutput_write(MX_ANALOG_OUTPUT *aoutput);

extern MX_RECORD_FUNCTION_LIST mxd_nuvant_ezware2_aoutput_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_nuvant_ezware2_aoutput_analog_output_function_list;

extern long mxd_nuvant_ezware2_aoutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_nuvant_ezware2_aoutput_rfield_def_ptr;

#define MXD_NUVANT_EZWARE2_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "nuvant_ezware2_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NUVANT_EZWARE2_AOUTPUT, nuvant_ezware2_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "output_type_name", MXFT_STRING, \
			NULL, 1, {MXU_NUVANT_EZWARE2_AOUTPUT_TYPE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_NUVANT_EZWARE2_AOUTPUT, output_type_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_NUVANT_EZWARE2_AOUTPUT_H__ */


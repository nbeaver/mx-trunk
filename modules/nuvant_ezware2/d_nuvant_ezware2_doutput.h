/*
 * Name:    d_nuvant_ezware2_doutput.h
 *
 * Purpose: Header file for NuVant EZstat digital output channels.
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

#ifndef __D_NUVANT_EZWARE2_DOUTPUT_H__
#define __D_NUVANT_EZWARE2_DOUTPUT_H__

#include "mx_digital_output.h"

#define MXU_NUVANT_EZWARE2_DOUTPUT_TYPE_LENGTH	40

#define MXT_NUVANT_EZWARE2_DOUTPUT_CELL_ENABLE		1
#define MXT_NUVANT_EZWARE2_DOUTPUT_EXTERNAL_SWITCH	2
#define MXT_NUVANT_EZWARE2_DOUTPUT_MODE_SELECT		3

typedef struct {
	MX_RECORD *record;

	MX_RECORD *nuvant_ezware2_record;
	char output_type_name[MXU_NUVANT_EZWARE2_DOUTPUT_TYPE_LENGTH+1];

	unsigned long output_type;

} MX_NUVANT_EZWARE2_DOUTPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_nuvant_ezware2_doutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_nuvant_ezware2_doutput_open( MX_RECORD *record );

MX_API mx_status_type mxd_nuvant_ezware2_doutput_read(
						MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_nuvant_ezware2_doutput_write(
						MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_nuvant_ezware2_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_nuvant_ezware2_doutput_digital_output_function_list;

extern long mxd_nuvant_ezware2_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_nuvant_ezware2_doutput_rfield_def_ptr;

#define MXD_NUVANT_EZWARE2_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "nuvant_ezware2_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NUVANT_EZWARE2_DOUTPUT, nuvant_ezware2_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "output_type_name", MXFT_STRING, NULL, \
			1, {MXU_NUVANT_EZWARE2_DOUTPUT_TYPE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_NUVANT_EZWARE2_DOUTPUT, output_type_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }

#endif /* __D_NUVANT_EZWARE2_DOUTPUT_H__ */


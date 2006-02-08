/*
 * Name:    d_sr630_aoutput.h
 *
 * Purpose: Header file for setting voltage outputs for the Stanford Research
 *          Systems model SR630 16 channel thermocouple reader.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SR630_AOUTPUT_H__
#define __D_SR630_AOUTPUT_H__

#include "mx_analog_output.h"

typedef struct {
	MX_RECORD *controller_record;
	int32_t channel_number;
} MX_SR630_AOUTPUT;

MX_API mx_status_type mxd_sr630_aoutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sr630_aoutput_open( MX_RECORD *record );

MX_API mx_status_type mxd_sr630_aoutput_write(MX_ANALOG_OUTPUT *aoutput);

extern MX_RECORD_FUNCTION_LIST mxd_sr630_aoutput_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_sr630_aoutput_analog_output_function_list;

extern mx_length_type mxd_sr630_aoutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sr630_aoutput_rfield_def_ptr;

#define MXD_SR630_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SR630_AOUTPUT, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  {-1, -1, "channel_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR630_AOUTPUT, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_SR630_AOUTPUT_H__ */

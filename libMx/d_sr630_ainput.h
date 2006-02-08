/*
 * Name:    d_sr630_ainput.h
 *
 * Purpose: Header file for reading out Stanford Research Systems model SR630
 *          thermocouple analog input channels.
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

#ifndef __D_SR630_AINPUT_H__
#define __D_SR630_AINPUT_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *controller_record;
	int32_t channel_number;
} MX_SR630_AINPUT;

MX_API mx_status_type mxd_sr630_ainput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sr630_ainput_open( MX_RECORD *record );
MX_API mx_status_type mxd_sr630_ainput_close( MX_RECORD *record );

MX_API mx_status_type mxd_sr630_ainput_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_sr630_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_sr630_ainput_analog_input_function_list;

extern mx_length_type mxd_sr630_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sr630_ainput_rfield_def_ptr;

#define MXD_SR630_AINPUT_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SR630_AINPUT, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR630_AINPUT,channel_number),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_SR630_AINPUT_H__ */

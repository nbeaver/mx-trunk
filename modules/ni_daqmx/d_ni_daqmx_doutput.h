/*
 * Name:    d_daqmx_base_doutput.h
 *
 * Purpose: Header file for NI-DAQmx Base digital output channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DAQMX_BASE_DOUTPUT_H__
#define __D_DAQMX_BASE_DOUTPUT_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD * record;

	MX_RECORD *daqmx_base_record;
	char channel_name[ MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH+1 ];

	TaskHandle handle;

} MX_DAQMX_BASE_DOUTPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_daqmx_base_doutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_daqmx_base_doutput_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_daqmx_base_doutput_open( MX_RECORD *record );
MX_API mx_status_type mxd_daqmx_base_doutput_close( MX_RECORD *record );

MX_API mx_status_type mxd_daqmx_base_doutput_write(MX_DIGITAL_OUTPUT *doutput);

extern MX_RECORD_FUNCTION_LIST mxd_daqmx_base_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_daqmx_base_doutput_digital_output_function_list;

extern long mxd_daqmx_base_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_daqmx_base_doutput_rfield_def_ptr;

#define MXD_DAQMX_BASE_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "daqmx_base_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_DAQMX_BASE_DOUTPUT, daqmx_base_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "channel_name", MXFT_STRING, \
			NULL, 1, {MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DAQMX_BASE_DOUTPUT, channel_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_DAQMX_BASE_DOUTPUT_H__ */


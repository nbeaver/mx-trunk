/*
 * Name:    d_ni_daqmx_doutput.h
 *
 * Purpose: Header file for NI-DAQmx digital output channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NI_DAQMX_DOUTPUT_H__
#define __D_NI_DAQMX_DOUTPUT_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD * record;

	MX_RECORD *ni_daqmx_record;
	char task_name[ MXU_NI_DAQMX_TASK_NAME_LENGTH+1 ];
	char channel_name[ MXU_NI_DAQMX_CHANNEL_NAME_LENGTH+1 ];

	MX_NI_DAQMX_TASK *task;

} MX_NI_DAQMX_DOUTPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_ni_daqmx_doutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_ni_daqmx_doutput_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_ni_daqmx_doutput_open( MX_RECORD *record );
MX_API mx_status_type mxd_ni_daqmx_doutput_close( MX_RECORD *record );

MX_API mx_status_type mxd_ni_daqmx_doutput_write(MX_DIGITAL_OUTPUT *doutput);

extern MX_RECORD_FUNCTION_LIST mxd_ni_daqmx_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_ni_daqmx_doutput_digital_output_function_list;

extern long mxd_ni_daqmx_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ni_daqmx_doutput_rfield_def_ptr;

#define MXD_NI_DAQMX_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "ni_daqmx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_NI_DAQMX_DOUTPUT, ni_daqmx_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "task_name", MXFT_STRING, \
			NULL, 1, {MXU_NI_DAQMX_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI_DAQMX_DOUTPUT, task_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "channel_name", MXFT_STRING, \
			NULL, 1, {MXU_NI_DAQMX_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI_DAQMX_DOUTPUT, channel_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_NI_DAQMX_DOUTPUT_H__ */


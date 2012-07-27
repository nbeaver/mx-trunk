/*
 * Name:    d_ni_daqmx_dinput.h
 *
 * Purpose: Header file for NI-DAQmx digital input channels.
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

#ifndef __D_NI_DAQMX_DINPUT_H__
#define __D_NI_DAQMX_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	MX_RECORD * record;

	MX_RECORD *ni_daqmx_record;
	char task_name[ MXU_NI_DAQMX_TASK_NAME_LENGTH+1 ];
	char channel_name[ MXU_NI_DAQMX_CHANNEL_NAME_LENGTH+1 ];

	MX_NI_DAQMX_TASK *task;
	unsigned long channel_offset;

} MX_NI_DAQMX_DINPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_ni_daqmx_dinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_ni_daqmx_dinput_open( MX_RECORD *record );

MX_API mx_status_type mxd_ni_daqmx_dinput_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_ni_daqmx_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_ni_daqmx_dinput_digital_input_function_list;

extern long mxd_ni_daqmx_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ni_daqmx_dinput_rfield_def_ptr;

#define MXD_NI_DAQMX_DINPUT_STANDARD_FIELDS \
  {-1, -1, "ni_daqmx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI_DAQMX_DINPUT, ni_daqmx_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "task_name", MXFT_STRING, \
			NULL, 1, {MXU_NI_DAQMX_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI_DAQMX_DINPUT, task_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "channel_name", MXFT_STRING, \
			NULL, 1, {MXU_NI_DAQMX_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI_DAQMX_DINPUT, channel_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_NI_DAQMX_DINPUT_H__ */


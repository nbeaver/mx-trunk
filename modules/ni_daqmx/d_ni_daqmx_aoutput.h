/*
 * Name:    d_ni_daqmx_aoutput.h
 *
 * Purpose: Header file for NI-DAQmx analog output channels.
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

#ifndef __D_NI_DAQMX_AOUTPUT_H__
#define __D_NI_DAQMX_AOUTPUT_H__

#include "mx_analog_output.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *ni_daqmx_record;
	char task_name[ MXU_NI_DAQMX_TASK_NAME_LENGTH+1 ];
	char channel_name[ MXU_NI_DAQMX_CHANNEL_NAME_LENGTH+1 ];
	double minimum_value;
	double maximum_value;

	MX_NI_DAQMX_TASK *task;
	unsigned long channel_offset;

} MX_NI_DAQMX_AOUTPUT;

MX_API mx_status_type mxd_ni_daqmx_aoutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_ni_daqmx_aoutput_open( MX_RECORD *record );

MX_API mx_status_type mxd_ni_daqmx_aoutput_write(MX_ANALOG_OUTPUT *aoutput);

extern MX_RECORD_FUNCTION_LIST mxd_ni_daqmx_aoutput_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_ni_daqmx_aoutput_analog_output_function_list;

extern long mxd_ni_daqmx_aoutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ni_daqmx_aoutput_rfield_def_ptr;

#define MXD_NI_DAQMX_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "ni_daqmx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NI_DAQMX_AOUTPUT, ni_daqmx_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "task_name", MXFT_STRING, \
			NULL, 1, {MXU_NI_DAQMX_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI_DAQMX_AOUTPUT, task_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_name", MXFT_STRING, \
			NULL, 1, {MXU_NI_DAQMX_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI_DAQMX_AOUTPUT, channel_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "minimum_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI_DAQMX_AOUTPUT, minimum_value), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "maximum_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI_DAQMX_AOUTPUT, maximum_value), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_NI_DAQMX_AOUTPUT_H__ */

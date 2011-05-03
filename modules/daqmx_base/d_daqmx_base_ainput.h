/*
 * Name:    d_daqmx_base_ainput.h
 *
 * Purpose: Header file for NI-DAQmx Base analog input channels.
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

#ifndef __D_DAQMX_BASE_AINPUT_H__
#define __D_DAQMX_BASE_AINPUT_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *daqmx_base_record;
	char channel_name[ MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH+1 ];
	double minimum_value;
	double maximum_value;
	char terminal_config[40];

	TaskHandle handle;

} MX_DAQMX_BASE_AINPUT;

MX_API mx_status_type mxd_daqmx_base_ainput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_daqmx_base_ainput_open( MX_RECORD *record );
MX_API mx_status_type mxd_daqmx_base_ainput_close( MX_RECORD *record );

MX_API mx_status_type mxd_daqmx_base_ainput_read(MX_ANALOG_INPUT *ainput);

extern MX_RECORD_FUNCTION_LIST mxd_daqmx_base_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_daqmx_base_ainput_analog_input_function_list;

extern long mxd_daqmx_base_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_daqmx_base_ainput_rfield_def_ptr;

#define MXD_DAQMX_BASE_AINPUT_STANDARD_FIELDS \
  {-1, -1, "daqmx_base_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DAQMX_BASE_AINPUT, daqmx_base_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_name", MXFT_STRING, \
			NULL, 1, {MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DAQMX_BASE_AINPUT, channel_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "minimum_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DAQMX_BASE_AINPUT, minimum_value), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "maximum_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DAQMX_BASE_AINPUT, maximum_value), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "terminal_config", MXFT_STRING, \
			NULL, 1, {MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DAQMX_BASE_AINPUT, terminal_config), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \

#endif /* __D_DAQMX_BASE_AINPUT_H__ */

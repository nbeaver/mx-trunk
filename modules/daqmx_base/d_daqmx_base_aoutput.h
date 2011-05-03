/*
 * Name:    d_daqmx_base_aoutput.h
 *
 * Purpose: Header file for NI-DAQmx Base analog output channels.
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

#ifndef __D_DAQMX_BASE_AOUTPUT_H__
#define __D_DAQMX_BASE_AOUTPUT_H__

#include "mx_analog_output.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *daqmx_base_record;
	char channel_name[ MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH+1 ];
	double minimum_value;
	double maximum_value;

	TaskHandle handle;

} MX_DAQMX_BASE_AOUTPUT;

MX_API mx_status_type mxd_daqmx_base_aoutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_daqmx_base_aoutput_open( MX_RECORD *record );
MX_API mx_status_type mxd_daqmx_base_aoutput_close( MX_RECORD *record );

MX_API mx_status_type mxd_daqmx_base_aoutput_write(MX_ANALOG_OUTPUT *aoutput);

extern MX_RECORD_FUNCTION_LIST mxd_daqmx_base_aoutput_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_daqmx_base_aoutput_analog_output_function_list;

extern long mxd_daqmx_base_aoutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_daqmx_base_aoutput_rfield_def_ptr;

#define MXD_DAQMX_BASE_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "daqmx_base_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DAQMX_BASE_AOUTPUT, daqmx_base_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_name", MXFT_STRING, \
			NULL, 1, {MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DAQMX_BASE_AOUTPUT, channel_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "minimum_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DAQMX_BASE_AOUTPUT, minimum_value), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "maximum_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DAQMX_BASE_AOUTPUT, maximum_value), \
	{0}, NULL, 0 }

#endif /* __D_DAQMX_BASE_AOUTPUT_H__ */

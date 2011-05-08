/*
 * Name:    d_daqmx_base_thermocouple.h
 *
 * Purpose: Header file for NI-DAQmx Base thermocouple channels.
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

#ifndef __D_DAQMX_BASE_THERMOCOUPLE_H__
#define __D_DAQMX_BASE_THERMOCOUPLE_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *daqmx_base_record;
	char channel_name[ MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH+1 ];
	double minimum_value;
	double maximum_value;
	char thermocouple_units[40];
	char thermocouple_type[40];
	char cold_junction[40];
	double cold_junction_temperature;
	char cold_junction_channel_name[80];

	TaskHandle handle;

} MX_DAQMX_BASE_THERMOCOUPLE;

MX_API mx_status_type mxd_daqmx_base_thermocouple_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_daqmx_base_thermocouple_open( MX_RECORD *record );
MX_API mx_status_type mxd_daqmx_base_thermocouple_close( MX_RECORD *record );

MX_API mx_status_type mxd_daqmx_base_thermocouple_read(MX_ANALOG_INPUT *ainput);

extern MX_RECORD_FUNCTION_LIST mxd_daqmx_base_thermocouple_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_daqmx_base_thermocouple_analog_input_function_list;

extern long mxd_daqmx_base_thermocouple_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_daqmx_base_thermocouple_rfield_def_ptr;

#define MXD_DAQMX_BASE_THERMOCOUPLE_STANDARD_FIELDS \
  {-1, -1, "daqmx_base_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DAQMX_BASE_THERMOCOUPLE, daqmx_base_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_name", MXFT_STRING, \
			NULL, 1, {MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_DAQMX_BASE_THERMOCOUPLE, channel_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "minimum_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_DAQMX_BASE_THERMOCOUPLE, minimum_value), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "maximum_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_DAQMX_BASE_THERMOCOUPLE, maximum_value), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "thermocouple_units", MXFT_STRING, \
			NULL, 1, {MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DAQMX_BASE_THERMOCOUPLE, thermocouple_units), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "thermocouple_type", MXFT_STRING, \
			NULL, 1, {MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DAQMX_BASE_THERMOCOUPLE, thermocouple_type), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "cold_junction", MXFT_STRING, \
			NULL, 1, {MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DAQMX_BASE_THERMOCOUPLE, cold_junction), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "cold_junction_temperature", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_DAQMX_BASE_THERMOCOUPLE, cold_junction_temperature), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "cold_junction_channel_name", MXFT_STRING, \
					NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_DAQMX_BASE_THERMOCOUPLE, cold_junction_channel_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_DAQMX_BASE_THERMOCOUPLE_H__ */


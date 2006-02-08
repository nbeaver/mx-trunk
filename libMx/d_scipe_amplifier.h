/*
 * Name:    d_scipe_amplifier.h
 *
 * Purpose: Header file for MX driver for SCIPE amplifiers.
 *
 * Author:  William Lavender and Steven Weigand
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SCIPE_AMPLIFIER_H__
#define __D_SCIPE_AMPLIFIER_H__

#include "mx_amplifier.h"

typedef struct {
	MX_RECORD *scipe_server_record;
	char scipe_amplifier_name[ MX_SCIPE_OBJECT_NAME_LENGTH + 1 ];

	double  bias_voltage;
	int32_t filter_type;
	double  lowpass_filter_3db_point;
	double  highpass_filter_3db_point;
	int32_t reset_filter;
	int32_t gain_mode;
	int32_t invert_signal;
	int32_t blank_output;
} MX_SCIPE_AMPLIFIER;

#define MXLV_SCIPE_AMPLIFIER_BIAS_VOLTAGE		0
#define MXLV_SCIPE_AMPLIFIER_FILTER_TYPE		1
#define MXLV_SCIPE_AMPLIFIER_LOWPASS_FILTER_3DB_POINT	2
#define MXLV_SCIPE_AMPLIFIER_HIGHPASS_FILTER_3DB_POINT	3
#define MXLV_SCIPE_AMPLIFIER_RESET_FILTER		4
#define MXLV_SCIPE_AMPLIFIER_GAIN_MODE			5
#define MXLV_SCIPE_AMPLIFIER_INVERT_SIGNAL		6
#define MXLV_SCIPE_AMPLIFIER_BLANK_OUTPUT		7

MX_API mx_status_type mxd_scipe_amplifier_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_scipe_amplifier_open( MX_RECORD *record );
MX_API mx_status_type mxd_scipe_amplifier_special_processing_setup(
						MX_RECORD *record );

MX_API mx_status_type mxd_scipe_amplifier_get_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_scipe_amplifier_set_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_scipe_amplifier_get_offset( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_scipe_amplifier_set_offset( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_scipe_amplifier_set_time_constant(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_scipe_amplifier_get_parameter(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_scipe_amplifier_set_parameter(
						MX_AMPLIFIER *amplifier );

extern MX_RECORD_FUNCTION_LIST mxd_scipe_amplifier_record_function_list;
extern MX_AMPLIFIER_FUNCTION_LIST mxd_scipe_amplifier_amplifier_function_list;

extern mx_length_type mxd_scipe_amplifier_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_scipe_amplifier_rfield_def_ptr;

#define MXD_SCIPE_AMPLIFIER_STANDARD_FIELDS \
  {-1, -1, "scipe_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AMPLIFIER, scipe_server_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scipe_amplifier_name", MXFT_STRING, NULL, \
	  				1, {MX_SCIPE_OBJECT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AMPLIFIER, \
					scipe_amplifier_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_SCIPE_AMPLIFIER_BIAS_VOLTAGE, -1, \
  		"bias_voltage", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AMPLIFIER, bias_voltage), \
	{0}, NULL, 0}, \
  \
  {MXLV_SCIPE_AMPLIFIER_FILTER_TYPE, -1, \
  		"filter_type", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AMPLIFIER, filter_type), \
	{0}, NULL, 0}, \
  \
  {MXLV_SCIPE_AMPLIFIER_LOWPASS_FILTER_3DB_POINT, -1, \
		  "lowpass_filter_3db_point", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AMPLIFIER, \
						lowpass_filter_3db_point), \
	{0}, NULL, 0}, \
  \
  {MXLV_SCIPE_AMPLIFIER_HIGHPASS_FILTER_3DB_POINT, -1, \
	  	"highpass_filter_3db_point", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AMPLIFIER, \
						highpass_filter_3db_point), \
	{0}, NULL, 0}, \
  \
  {MXLV_SCIPE_AMPLIFIER_RESET_FILTER, -1, \
	  	"reset_filter", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AMPLIFIER, reset_filter), \
	{0}, NULL, 0}, \
  \
  {MXLV_SCIPE_AMPLIFIER_GAIN_MODE, -1, "gain_mode", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AMPLIFIER, gain_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_SCIPE_AMPLIFIER_INVERT_SIGNAL, -1, \
	  	"invert_signal", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AMPLIFIER, invert_signal), \
	{0}, NULL, 0}, \
  \
  {MXLV_SCIPE_AMPLIFIER_BLANK_OUTPUT, -1, \
	  	"blank_output", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AMPLIFIER, blank_output), \
	{0}, NULL, 0}

#endif /* __D_SCIPE_AMPLIFIER_H__ */

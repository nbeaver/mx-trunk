/*
 * Name:    d_sr570.h
 *
 * Purpose: Header file for MX driver for the Stanford Research Systems
 *          Model SR570 current amplifier
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SR570_H__
#define __D_SR570_H__

#include "mx_amplifier.h"

typedef struct {
	MX_RECORD *rs232_record;

	double bias_voltage;
	long   filter_type;
	double lowpass_filter_hz;
	double highpass_filter_hz;
	long   reset_filter;
	long   gain_mode;
	long   invert_signal;
	long   blank_output;
	double lowpass_filter_time;
	double highpass_filter_time;

	/* The following fields are used to store the old settings of SR570
	 * parameters, so that if the user requests an invalid value, the
	 * value can be restored to the old value.  This is necessary since
	 * there is no way to ask the SR570 for its current settings.
	 */

	double old_gain;
	double old_offset;
	double old_bias_voltage;
	long   old_filter_type;
	double old_lowpass_filter_hz;
	double old_highpass_filter_hz;
	long   old_gain_mode;
	long   old_invert_signal;
	long   old_blank_output;
} MX_SR570;

#define MXLV_SR570_BIAS_VOLTAGE		0
#define MXLV_SR570_FILTER_TYPE		1
#define MXLV_SR570_LOWPASS_FILTER_HZ	2
#define MXLV_SR570_HIGHPASS_FILTER_HZ	3
#define MXLV_SR570_RESET_FILTER		4
#define MXLV_SR570_GAIN_MODE		5
#define MXLV_SR570_INVERT_SIGNAL	6
#define MXLV_SR570_BLANK_OUTPUT		7
#define MXLV_SR570_LOWPASS_FILTER_TIME	8
#define MXLV_SR570_HIGHPASS_FILTER_TIME	9

MX_API mx_status_type mxd_sr570_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_sr570_open( MX_RECORD *record );
MX_API mx_status_type mxd_sr570_special_processing_setup( MX_RECORD *record );

MX_API mx_status_type mxd_sr570_set_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_sr570_set_offset( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_sr570_get_time_constant( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_sr570_set_time_constant( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_sr570_get_parameter( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_sr570_set_parameter( MX_AMPLIFIER *amplifier );

extern MX_RECORD_FUNCTION_LIST mxd_sr570_record_function_list;
extern MX_AMPLIFIER_FUNCTION_LIST mxd_sr570_amplifier_function_list;

extern long mxd_sr570_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sr570_rfield_def_ptr;

#define MXD_SR570_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_SR570_BIAS_VOLTAGE, -1, "bias_voltage", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, bias_voltage), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_SR570_FILTER_TYPE, -1, "filter_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, filter_type), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_SR570_LOWPASS_FILTER_HZ, -1, "lowpass_filter_hz", \
	  				MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, lowpass_filter_hz), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_SR570_HIGHPASS_FILTER_HZ, -1, "highpass_filter_hz", \
	  				MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, highpass_filter_hz), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_SR570_RESET_FILTER, -1, "reset_filter", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, reset_filter), \
	{0}, NULL, 0}, \
  \
  {MXLV_SR570_GAIN_MODE, -1, "gain_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, gain_mode), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_SR570_INVERT_SIGNAL, -1, "invert_signal", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, invert_signal), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_SR570_BLANK_OUTPUT, -1, "blank_output", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, blank_output), \
	{0}, NULL, 0}, \
  \
  {MXLV_SR570_LOWPASS_FILTER_TIME, -1, "lowpass_filter_time", \
	    MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, lowpass_filter_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_SR570_HIGHPASS_FILTER_TIME, -1, "highpass_filter_time", \
	    MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR570, highpass_filter_time), \
	{0}, NULL, 0}

#endif /* __D_SR570_H__ */

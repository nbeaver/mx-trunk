/*
 * Name:    mx_analog_input.h
 *
 * Purpose: MX header file for analog input devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004, 2006-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_ANALOG_INPUT_H__
#define __MX_ANALOG_INPUT_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXF_AIN_SUBTRACT_DARK_CURRENT		0x1
#define MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT	0x2

#define MXF_AIN_PERFORM_TIME_NORMALIZATION	0x1000

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
                            * to this device.
                            */

	union { long long_value;
		double double_value;
	} raw_value;

	long subclass;

	double value;
	double dark_current;

	double scale;
	double offset;
	char units[MXU_UNITS_NAME_LENGTH+1];
	unsigned long analog_input_flags;

	char timer_record_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	MX_RECORD *timer_record;
} MX_ANALOG_INPUT;

#define MXLV_AIN_RAW_VALUE	2001
#define MXLV_AIN_VALUE		2002
#define MXLV_AIN_DARK_CURRENT	2003

#define MX_LONG_ANALOG_INPUT_STANDARD_FIELDS \
  {MXLV_AIN_RAW_VALUE, -1, "raw_value", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_ANALOG_INPUT, raw_value.long_value), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_POLL)}

#define MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS \
  {MXLV_AIN_RAW_VALUE, -1, "raw_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_ANALOG_INPUT, raw_value.double_value), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_POLL)}

#define MX_ANALOG_INPUT_STANDARD_FIELDS \
  {-1, -1, "subclass", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_INPUT, subclass), \
	{0}, NULL, 0}, \
  \
  {MXLV_AIN_VALUE, -1, "value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_INPUT, value), \
	{0}, NULL, (MXFF_IN_SUMMARY | MXFF_POLL)}, \
  \
  {-1, -1, "scale", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_INPUT, scale), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_INPUT, offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "units", MXFT_STRING, NULL, 1, {MXU_UNITS_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_INPUT, units), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "analog_input_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_INPUT, analog_input_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_AIN_DARK_CURRENT, -1, "dark_current", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_INPUT, dark_current), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "timer_record_name", MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_INPUT, timer_record_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}

/*
 * The structure type MX_ANALOG_INPUT_FUNCTION_LIST contains a list of
 * pointers to functions that vary from analog input type to analog
 * input type.
 */

typedef struct {
	mx_status_type ( *read ) ( MX_ANALOG_INPUT *adc );
	mx_status_type ( *get_dark_current ) ( MX_ANALOG_INPUT *adc );
	mx_status_type ( *set_dark_current ) ( MX_ANALOG_INPUT *adc );
	mx_status_type ( *clear ) ( MX_ANALOG_INPUT *adc );
} MX_ANALOG_INPUT_FUNCTION_LIST;

MX_API mx_status_type mx_analog_input_finish_record_initialization(
							MX_RECORD *adc_record );

MX_API mx_status_type mx_analog_input_read( MX_RECORD *adc_record,
							double *value );

MX_API mx_status_type mx_analog_input_read_raw_long( MX_RECORD *adc_record,
							long *raw_value );

MX_API mx_status_type mx_analog_input_read_raw_double( MX_RECORD *adc_record,
							double *raw_value );

MX_API mx_status_type mx_analog_input_get_dark_current( MX_RECORD *adc_record,
							double *dark_current );

MX_API mx_status_type mx_analog_input_set_dark_current( MX_RECORD *adc_record,
							double dark_current );

MX_API mx_status_type mx_analog_input_clear( MX_RECORD *adc_record );

extern MX_RECORD_FUNCTION_LIST mx_analog_input_record_function_list;

#ifdef __cplusplus
}
#endif

#endif /* __MX_ANALOG_INPUT_H__ */

/*
 * Name:    mx_analog_output.h
 *
 * Purpose: MX header file for analog output devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_ANALOG_OUTPUT_H__
#define __MX_ANALOG_OUTPUT_H__

#include "mx_record.h"

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
                            * to this device.
                            */

	union { int32_t int32_value;
		double double_value;
	} raw_value;

	int32_t subclass;

	double value;
	double scale;
	double offset;
	char units[MXU_UNITS_NAME_LENGTH+1];
	mx_hex_type analog_output_flags;
} MX_ANALOG_OUTPUT;

#define MXLV_AOU_RAW_VALUE	3001
#define MXLV_AOU_VALUE		3002

#define MX_INT32_ANALOG_OUTPUT_STANDARD_FIELDS \
  {MXLV_AOU_RAW_VALUE, -1, "raw_value", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_ANALOG_OUTPUT, raw_value.int32_value), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#define MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS \
  {MXLV_AOU_RAW_VALUE, -1, "raw_value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_ANALOG_OUTPUT, raw_value.double_value), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#define MX_ANALOG_OUTPUT_STANDARD_FIELDS \
  {-1, -1, "subclass", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_OUTPUT, subclass), \
	{0}, NULL, 0}, \
  \
  {MXLV_AOU_VALUE, -1, "value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_OUTPUT, value), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {-1, -1, "scale", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_OUTPUT, scale), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_OUTPUT, offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "units", MXFT_STRING, NULL, 1, {MXU_UNITS_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_OUTPUT, units), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "analog_output_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ANALOG_OUTPUT, analog_output_flags),\
	{0}, NULL, MXFF_IN_DESCRIPTION}

/*
 * The structure type MX_ANALOG_OUTPUT_FUNCTION_LIST contains a list of
 * pointers to functions that vary from analog output type to analog
 * output type.
 */

typedef struct {
	mx_status_type ( *read ) ( MX_ANALOG_OUTPUT *dac );
	mx_status_type ( *write ) ( MX_ANALOG_OUTPUT *dac );
} MX_ANALOG_OUTPUT_FUNCTION_LIST;

MX_API mx_status_type mx_analog_output_read(
				MX_RECORD *dac_record, double *value );

MX_API mx_status_type mx_analog_output_write(
				MX_RECORD *dac_record, double value );

MX_API mx_status_type mx_analog_output_read_raw_int32(
				MX_RECORD *dac_record, int32_t *value );

MX_API mx_status_type mx_analog_output_write_raw_int32(
				MX_RECORD *dac_record, int32_t value );

MX_API mx_status_type mx_analog_output_read_raw_double(
				MX_RECORD *dac_record, double *value );

MX_API mx_status_type mx_analog_output_write_raw_double(
				MX_RECORD *dac_record, double value );

extern MX_RECORD_FUNCTION_LIST mx_analog_output_record_function_list;

#endif /* __MX_ANALOG_OUTPUT_H__ */

/*
 * Name:    mx_digital_output.h
 *
 * Purpose: MX header file for digital output devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2007-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_DIGITAL_OUTPUT_H__
#define __MX_DIGITAL_OUTPUT_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
                            * to this device.
                            */
	unsigned long value;

	long pulse_on_value;
	long pulse_off_value;
	double pulse_duration;		/* in seconds */
} MX_DIGITAL_OUTPUT;

#define MXLV_DOU_VALUE			5001
#define MXLV_DOU_PULSE_ON		5002
#define MXLV_DOU_PULSE_OFF		5003
#define MXLV_DOU_PULSE_DURATION		5004

#define MX_DIGITAL_OUTPUT_STANDARD_FIELDS \
  {MXLV_DOU_VALUE, -1, "value", MXFT_HEX, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, offsetof(MX_DIGITAL_OUTPUT, value), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_DOU_PULSE_ON, -1, "pulse_on_value", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_DIGITAL_OUTPUT, pulse_on_value), \
	{0}, NULL, 0}, \
  \
  {MXLV_DOU_PULSE_OFF, -1, "pulse_off_value", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_DIGITAL_OUTPUT, pulse_off_value), \
	{0}, NULL, 0}, \
  \
  {MXLV_DOU_PULSE_DURATION, -1, "pulse_duration", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_DIGITAL_OUTPUT, pulse_duration), \
	{0}, NULL, 0}

/*
 * The structure type MX_DIGITAL_OUTPUT_FUNCTION_LIST contains a list of
 * pointers to functions that vary from digital output type to digital
 * output type.
 */

typedef struct {
	mx_status_type ( *read ) ( MX_DIGITAL_OUTPUT *doutput );
	mx_status_type ( *write ) ( MX_DIGITAL_OUTPUT *doutput );
	mx_status_type ( *pulse ) ( MX_DIGITAL_OUTPUT *doutput );
} MX_DIGITAL_OUTPUT_FUNCTION_LIST;

MX_API mx_status_type mx_digital_output_read(MX_RECORD *doutput_record,
							unsigned long *value);

MX_API mx_status_type mx_digital_output_write(MX_RECORD *doutput_record,
							unsigned long value);

MX_API mx_status_type mx_digital_output_pulse( MX_RECORD *doutput_record,
						long pulse_on_value,
						long pulse_off_value,
						double pulse_duration );

MX_API mx_status_type mx_digital_output_pulse_wait( MX_RECORD *doutput_record,
						long pulse_on_value,
						long pulse_off_value,
						double pulse_duration,
						mx_bool_type busy_wait );

extern MX_RECORD_FUNCTION_LIST mx_digital_output_record_function_list;

#ifdef __cplusplus
}
#endif

#endif /* __MX_DIGITAL_OUTPUT_H__ */

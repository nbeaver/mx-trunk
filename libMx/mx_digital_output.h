/*
 * Name:    mx_digital_output.h
 *
 * Purpose: MX header file for digital output devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_DIGITAL_OUTPUT_H__
#define __MX_DIGITAL_OUTPUT_H__

#include "mx_record.h"

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
                            * to this device.
                            */
	unsigned long value;
} MX_DIGITAL_OUTPUT;

#define MXLV_DOU_VALUE 5001

#define MX_DIGITAL_OUTPUT_STANDARD_FIELDS \
  {MXLV_DOU_VALUE, -1, "value", MXFT_HEX, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, offsetof(MX_DIGITAL_OUTPUT, value), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/*
 * The structure type MX_DIGITAL_OUTPUT_FUNCTION_LIST contains a list of
 * pointers to functions that vary from digital output type to digital
 * output type.
 */

typedef struct {
	mx_status_type ( *read ) ( MX_DIGITAL_OUTPUT *doutput );
	mx_status_type ( *write ) ( MX_DIGITAL_OUTPUT *doutput );
} MX_DIGITAL_OUTPUT_FUNCTION_LIST;

MX_API mx_status_type mx_digital_output_read(MX_RECORD *doutput,
							unsigned long *value);

MX_API mx_status_type mx_digital_output_write(MX_RECORD *doutput,
							unsigned long value);

extern MX_RECORD_FUNCTION_LIST mx_digital_output_record_function_list;

#endif /* __MX_DIGITAL_OUTPUT_H__ */

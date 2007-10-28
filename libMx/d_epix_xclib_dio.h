/*
 * Name:    d_epix_xclib_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          the digital I/O ports on EPIX imaging boards.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPIX_XCLIB_DIO_H__
#define __D_EPIX_XCLIB_DIO_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== EPIX XCLIB digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *epix_xclib_vinput_record;
	long trigger_number;
} MX_EPIX_XCLIB_DIGITAL_INPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *epix_xclib_vinput_record;
} MX_EPIX_XCLIB_DIGITAL_OUTPUT;

#define MXD_EPIX_XCLIB_DIGITAL_INPUT_STANDARD_FIELDS \
  {-1, -1, "epix_xclib_vinput_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_EPIX_XCLIB_DIGITAL_INPUT, epix_xclib_vinput_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "trigger_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_EPIX_XCLIB_DIGITAL_INPUT, trigger_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_EPIX_XCLIB_DIGITAL_OUTPUT_STANDARD_FIELDS \
  {-1, -1, "epix_xclib_vinput_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_EPIX_XCLIB_DIGITAL_OUTPUT, epix_xclib_vinput_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_epix_xclib_dinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_dinput_read( MX_DIGITAL_INPUT *dinput );
MX_API mx_status_type mxd_epix_xclib_dinput_clear( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_epix_xclib_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_epix_xclib_dinput_digital_input_function_list;

extern long mxd_epix_xclib_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epix_xclib_dinput_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_epix_xclib_doutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_doutput_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_epix_xclib_doutput_write( MX_DIGITAL_OUTPUT *doutput);

extern MX_RECORD_FUNCTION_LIST mxd_epix_xclib_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_epix_xclib_doutput_digital_output_function_list;

extern long mxd_epix_xclib_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epix_xclib_doutput_rfield_def_ptr;

#endif /* __D_EPIX_XCLIB_DIO_H__ */


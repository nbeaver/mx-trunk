/*
 * Name:    d_cm17a_doutput.h
 *
 * Purpose: Header file for X10 Firecracker (CM17A) digital outputs.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_CM17A_DOUTPUT_H__
#define __D_CM17A_DOUTPUT_H__

/* ===== CM17A_DOUTPUT digital output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *cm17a_record;
	char house_code;
	int device_code;

	mx_uint16_type on_command;
	mx_uint16_type off_command;
	mx_uint16_type bright_command;
	mx_uint16_type dim_command;
} MX_CM17A_DOUTPUT;

#define MXD_CM17A_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "cm17a_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CM17A_DOUTPUT, cm17a_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "house_code", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CM17A_DOUTPUT, house_code), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "device_code", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CM17A_DOUTPUT, device_code), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_cm17a_doutput_create_record_structures(
							MX_RECORD *record);
MX_API mx_status_type mxd_cm17a_doutput_open( MX_RECORD *record);

MX_API mx_status_type mxd_cm17a_doutput_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_cm17a_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_cm17a_doutput_digital_output_function_list;

extern long mxd_cm17a_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_cm17a_doutput_rfield_def_ptr;

#endif /* __D_CM17A_DOUTPUT_H__ */


/*
 * Name:    d_pdi45_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          Prairie Digital Model 45 digital I/O lines.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PDI45_DIO_H__
#define __D_PDI45_DIO_H__

/* Values for the 'pdi45_dinput_flags' and 'pdi45_doutput_flags' below. */

#define MXF_PDI45_DIO_INVERT	0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pdi45_record;
	long line_number;
	unsigned long pdi45_dinput_flags;
} MX_PDI45_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pdi45_record;
	long line_number;
	unsigned long pdi45_doutput_flags;
} MX_PDI45_DOUTPUT;

#define MXD_PDI45_DINPUT_STANDARD_FIELDS \
  {-1, -1, "pdi45_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_DINPUT, pdi45_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "line_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_DINPUT, line_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pdi45_dinput_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_DINPUT, pdi45_dinput_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_PDI45_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "pdi45_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_DOUTPUT, pdi45_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "line_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_DOUTPUT, line_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pdi45_doutput_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_DOUTPUT, pdi45_doutput_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_pdi45_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pdi45_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_pdi45_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_pdi45_din_digital_input_function_list;

extern long mxd_pdi45_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_pdi45_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pdi45_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_pdi45_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_pdi45_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_pdi45_dout_digital_output_function_list;

extern long mxd_pdi45_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_dout_rfield_def_ptr;

#endif /* __D_PDI45_DIO_H__ */


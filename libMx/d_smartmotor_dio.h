/*
 * Name:    d_smartmotor_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          the digital I/O ports available on Animatics SmartMotor
 *          motor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SMARTMOTOR_DIO_H__
#define __D_SMARTMOTOR_DIO_H__

/* ===== SmartMotor digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *smartmotor_record;
	char port_name[ MXU_SMARTMOTOR_IO_NAME_LENGTH+1 ];
} MX_SMARTMOTOR_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *smartmotor_record;
	char port_name[ MXU_SMARTMOTOR_IO_NAME_LENGTH+1 ];
} MX_SMARTMOTOR_DOUTPUT;

#define MXD_SMARTMOTOR_DINPUT_STANDARD_FIELDS \
  {-1, -1, "smartmotor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_SMARTMOTOR_DINPUT, smartmotor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_name", MXFT_STRING, NULL, 1, {MXU_SMARTMOTOR_IO_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SMARTMOTOR_DINPUT, port_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_SMARTMOTOR_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "smartmotor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_SMARTMOTOR_DOUTPUT, smartmotor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_name", MXFT_STRING, NULL, 1, {MXU_SMARTMOTOR_IO_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SMARTMOTOR_DOUTPUT, port_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_smartmotor_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_smartmotor_din_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_smartmotor_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_smartmotor_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_smartmotor_din_digital_input_function_list;

extern long mxd_smartmotor_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_smartmotor_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_smartmotor_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_smartmotor_dout_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_smartmotor_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_smartmotor_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_smartmotor_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_smartmotor_dout_digital_output_function_list;

extern long mxd_smartmotor_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_smartmotor_dout_rfield_def_ptr;

#endif /* __D_SMARTMOTOR_DIO_H__ */


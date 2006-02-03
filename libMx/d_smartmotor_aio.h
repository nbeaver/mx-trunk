/*
 * Name:    d_smartmotor_aio.h
 *
 * Purpose: Header file for MX input and output drivers to read 
 *          the analog I/O ports on an Animatics SmartMotor
 *          motor controller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SMARTMOTOR_AIO_H__
#define __D_SMARTMOTOR_AIO_H__

/* ===== SmartMotor analog input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *smartmotor_record;
	char port_name[ MXU_SMARTMOTOR_IO_NAME_LENGTH+1 ];
} MX_SMARTMOTOR_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *smartmotor_record;
	char port_name[ MXU_SMARTMOTOR_IO_NAME_LENGTH+1 ];
} MX_SMARTMOTOR_AOUTPUT;

#define MXD_SMARTMOTOR_AINPUT_STANDARD_FIELDS \
  {-1, -1, "smartmotor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_SMARTMOTOR_AINPUT, smartmotor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_name", MXFT_STRING, NULL, 1, {MXU_SMARTMOTOR_IO_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SMARTMOTOR_AINPUT, port_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_SMARTMOTOR_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "smartmotor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_SMARTMOTOR_AOUTPUT, smartmotor_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_name", MXFT_STRING, NULL, 1, {MXU_SMARTMOTOR_IO_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SMARTMOTOR_AOUTPUT, port_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_smartmotor_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_smartmotor_ain_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_smartmotor_ain_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_smartmotor_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_smartmotor_ain_analog_input_function_list;

extern mx_length_type mxd_smartmotor_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_smartmotor_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_smartmotor_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_smartmotor_aout_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_smartmotor_aout_read( MX_ANALOG_OUTPUT *dinput );
MX_API mx_status_type mxd_smartmotor_aout_write( MX_ANALOG_OUTPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_smartmotor_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_smartmotor_aout_analog_output_function_list;

extern mx_length_type mxd_smartmotor_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_smartmotor_aout_rfield_def_ptr;

#endif /* __D_SMARTMOTOR_AIO_H__ */


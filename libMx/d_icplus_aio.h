/*
 * Name:    d_icplus_aio.h
 *
 * Purpose: Header file for MX analog input and output drivers to read and
 *          write various parameters for the Oxford Danfysik IC PLUS 
 *          intensity monitor and QBPM position monitor.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ICPLUS_AIO_H__
#define __D_ICPLUS_AIO_H__

/* ===== IC PLUS analog input/output data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *icplus_record;
	char value_name[ MXU_ICPLUS_VALUE_NAME_LENGTH+1 ];

	char command[ MXU_ICPLUS_COMMAND_LENGTH+1 ];
} MX_ICPLUS_AINPUT;

#define MXAT_ICPLUS_AOUTPUT_NONE	0
#define MXAT_ICPLUS_AOUTPUT_INTEGER	1
#define MXAT_ICPLUS_AOUTPUT_DOUBLE	2
#define MXAT_ICPLUS_AOUTPUT_AVERAGE	3

typedef struct {
	MX_RECORD *record;

	MX_RECORD *icplus_record;
	char value_name[ MXU_ICPLUS_VALUE_NAME_LENGTH+1 ];

	int argument_type;
	char input_command[ MXU_ICPLUS_COMMAND_LENGTH+1 ];
	char output_command[ MXU_ICPLUS_COMMAND_LENGTH+1 ];
} MX_ICPLUS_AOUTPUT;

#define MXD_ICPLUS_AINPUT_STANDARD_FIELDS \
  {-1, -1, "icplus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS_AINPUT, icplus_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "value_name", MXFT_STRING, NULL, 1, {MXU_ICPLUS_VALUE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS_AINPUT, value_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_ICPLUS_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "icplus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS_AOUTPUT, icplus_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "value_name", MXFT_STRING, NULL, 1, {MXU_ICPLUS_VALUE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS_AOUTPUT, value_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the functions to read the ion chamber current. */

MX_API mx_status_type mxd_icplus_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_icplus_ain_open( MX_RECORD *record );

MX_API mx_status_type mxd_icplus_ain_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_icplus_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_icplus_ain_analog_input_function_list;

extern long mxd_icplus_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_icplus_ain_rfield_def_ptr;

/* Second the functions to set the high voltage. */

MX_API mx_status_type mxd_icplus_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_icplus_aout_open( MX_RECORD *record );

MX_API mx_status_type mxd_icplus_aout_read( MX_ANALOG_OUTPUT *doutput );
MX_API mx_status_type mxd_icplus_aout_write( MX_ANALOG_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_icplus_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_icplus_aout_analog_output_function_list;

extern long mxd_icplus_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_icplus_aout_rfield_def_ptr;

#endif /* __D_ICPLUS_AIO_H__ */


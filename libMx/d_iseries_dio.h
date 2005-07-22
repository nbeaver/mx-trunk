/*
 * Name:    d_iseries_dio.h
 *
 * Purpose: Header file for drivers to control iSeries functions
 *          as if they were digital I/O ports.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ISERIES_DIO_H__
#define __D_ISERIES_DIO_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *iseries_record;
	char command[ MXU_ISERIES_COMMAND_LENGTH+1 ];
	int num_command_bytes;

	char command_prefix;
	unsigned long command_index;
} MX_ISERIES_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *iseries_record;
	char command[ MXU_ISERIES_COMMAND_LENGTH+1 ];
	int num_command_bytes;

	char command_prefix;
	unsigned long command_index;
} MX_ISERIES_DOUTPUT;

#define MXD_ISERIES_DINPUT_STANDARD_FIELDS \
  {-1, -1, "iseries_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_DINPUT, iseries_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "command", MXFT_STRING, NULL, 1, {MXU_ISERIES_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_DINPUT, command), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_command_bytes", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_DINPUT, num_command_bytes), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#define MXD_ISERIES_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "iseries_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_DOUTPUT, iseries_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "command", MXFT_STRING, NULL, 1, {MXU_ISERIES_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_DOUTPUT, command), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_command_bytes", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_DOUTPUT, num_command_bytes), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_iseries_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_iseries_din_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_iseries_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_iseries_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_iseries_din_digital_input_function_list;

extern long mxd_iseries_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_iseries_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_iseries_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_iseries_dout_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_iseries_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_iseries_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_iseries_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_iseries_dout_digital_output_function_list;

extern long mxd_iseries_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_iseries_dout_rfield_def_ptr;

#endif /* __D_ISERIES_DIO_H__ */


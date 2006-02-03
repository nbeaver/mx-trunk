/*
 * Name:    d_iseries_aio.h
 *
 * Purpose: Header file for drivers to control iSeries functions
 *          as if they were analog I/O ports.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ISERIES_AIO_H__
#define __D_ISERIES_AIO_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *iseries_record;
	char command[ MXU_ISERIES_COMMAND_LENGTH+1 ];
	int num_command_bytes;
	int default_precision;

	char command_prefix;
	unsigned long command_index;
} MX_ISERIES_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *iseries_record;
	char command[ MXU_ISERIES_COMMAND_LENGTH+1 ];
	int num_command_bytes;
	int default_precision;

	char command_prefix;
	unsigned long command_index;
} MX_ISERIES_AOUTPUT;

#define MXD_ISERIES_AINPUT_STANDARD_FIELDS \
  {-1, -1, "iseries_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_AINPUT, iseries_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "command", MXFT_STRING, NULL, 1, {MXU_ISERIES_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_AINPUT, command), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_command_bytes", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_AINPUT, num_command_bytes), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_precision", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_AINPUT, default_precision), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#define MXD_ISERIES_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "iseries_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_AOUTPUT, iseries_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "command", MXFT_STRING, NULL, 1, {MXU_ISERIES_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_AOUTPUT, command), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_command_bytes", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_AOUTPUT, num_command_bytes), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_precision", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES_AOUTPUT, default_precision), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_iseries_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_iseries_ain_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_iseries_ain_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_iseries_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_iseries_ain_analog_input_function_list;

extern mx_length_type mxd_iseries_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_iseries_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_iseries_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_iseries_aout_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_iseries_aout_read( MX_ANALOG_OUTPUT *doutput );
MX_API mx_status_type mxd_iseries_aout_write( MX_ANALOG_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_iseries_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_iseries_aout_analog_output_function_list;

extern mx_length_type mxd_iseries_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_iseries_aout_rfield_def_ptr;

#endif /* __D_ISERIES_AIO_H__ */


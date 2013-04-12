/*
 * Name:    z_external_command.h
 *
 * Purpose: Header file for running external commands using mx_spawn().
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __Z_EXTERNAL_COMMAND_H__
#define __Z_EXTERNAL_COMMAND_H__

#define MXU_MAX_EXTERNAL_COMMAND_LENGTH    255

#define MXF_EXTERNAL_COMMAND_READ_ONLY					0x1
#define MXF_EXTERNAL_COMMAND_RUN_DURING_FINISH_RECORD_INITIALIZATION	0x2
#define MXF_EXTERNAL_COMMAND_RUN_DURING_OPEN				0x4

typedef struct {
	MX_RECORD *record;

	unsigned long external_command_flags;

	char command_name[MXU_FILENAME_LENGTH+1];
	char command_arguments[MXU_MAX_EXTERNAL_COMMAND_LENGTH+1];

	mx_bool_type run;
} MX_EXTERNAL_COMMAND;

#define MXZ_EXTERNAL_COMMAND_STANDARD_FIELDS \
  {-1, -1, "external_command_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EXTERNAL_COMMAND, external_command_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "command_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EXTERNAL_COMMAND, command_name), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "command_arguments", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EXTERNAL_COMMAND, command_arguments), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "run", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EXTERNAL_COMMAND, run), \
	{0}, NULL, 0}

MX_API_PRIVATE mx_status_type mxz_external_command_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxz_external_command_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxz_external_command_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxz_external_command_run(
					MX_EXTERNAL_COMMAND *exernal_command );

extern long mxz_external_command_num_record_fields;
extern MX_RECORD_FUNCTION_LIST mxz_external_command_record_function_list;
extern MX_RECORD_FIELD_DEFAULTS *mxz_external_command_rfield_def_ptr;

#endif /* __Z_EXTERNAL_COMMAND_H__ */


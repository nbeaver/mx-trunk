/*
 * Name:    mx_vfile.h
 *
 * Purpose: Header file for file variables in MX database description files.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_VFILE_H__
#define __MX_VFILE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Bit definitions for 'file_flags'. */

#define MXF_VFILE_READ		0x1
#define MXF_VFILE_WRITE		0x2

#define MXF_VFILE_TEMP_OPEN	0x10
#define MXF_VFILE_SHARED_OPEN	0x20

#define MXF_VFILE_CONCATENATE	0x100

typedef struct {
	MX_RECORD *record;

	long row_offset;
	long column_offset;
	unsigned long file_flags;
	char filename[ MXU_FILENAME_LENGTH+1 ];

	FILE *file;
} MX_FILE_VARIABLE;

#define MX_FILE_VARIABLE_STANDARD_FIELDS \
  {-1, -1, "row_offset", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_FILE_VARIABLE, row_offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "column_offset", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_FILE_VARIABLE, column_offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "file_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_FILE_VARIABLE, file_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
	offsetof(MX_FILE_VARIABLE, filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }

MX_API_PRIVATE mx_status_type mxv_file_variable_initialize_type( long );
MX_API_PRIVATE mx_status_type mxv_file_variable_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_file_variable_open( MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_file_variable_close( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_file_variable_send_variable(
						MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_file_variable_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_file_variable_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_file_variable_variable_function_list;

/* The various different kinds of file variables all have the same
 * number of record fields, so I only use one variable for the number
 * of fields.
 */

extern long mxv_file_string_variable_num_record_fields;
extern long mxv_file_long_variable_num_record_fields;
extern long mxv_file_ulong_variable_num_record_fields;
extern long mxv_file_double_variable_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxv_file_string_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_file_long_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_file_ulong_variable_dptr;
extern MX_RECORD_FIELD_DEFAULTS *mxv_file_double_variable_dptr;

#ifdef __cplusplus
}
#endif

#endif /* __MX_VFILE_H__ */


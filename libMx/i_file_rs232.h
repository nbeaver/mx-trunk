/*
 * Name:    i_file_rs232.h
 *
 * Purpose: Header for MX driver for simulating RS-232 ports with
 *          either 1 or 2 files.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_FILE_RS232_H__
#define __I_FILE_RS232_H__

/* Define the data structures used by this type of interface. */

typedef struct {
	MX_RECORD *record;

	FILE *input_file;
	FILE *output_file;

	char input_filename[MXU_FILENAME_LENGTH+1];
	char output_filename[MXU_FILENAME_LENGTH+1];
} MX_FILE_RS232;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_file_rs232_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_file_rs232_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_file_rs232_open( MX_RECORD *record );
MX_API mx_status_type mxi_file_rs232_close( MX_RECORD *record );

MX_API mx_status_type mxi_file_rs232_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_file_rs232_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_file_rs232_read( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_file_rs232_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );
MX_API mx_status_type mxi_file_rs232_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_file_rs232_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
MX_API mx_status_type mxi_file_rs232_num_input_bytes_available(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_file_rs232_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_file_rs232_get_configuration( MX_RS232 *rs232 );
MX_API mx_status_type mxi_file_rs232_set_configuration( MX_RS232 *rs232 );
MX_API mx_status_type mxi_file_rs232_send_break( MX_RS232 *rs232 );

extern MX_RECORD_FUNCTION_LIST mxi_file_rs232_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_file_rs232_rs232_function_list;

extern long mxi_file_rs232_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_file_rs232_rfield_def_ptr;

#define MXI_FILE_RS232_STANDARD_FIELDS \
  {-1, -1, "input_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FILE_RS232, input_filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "output_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FILE_RS232, output_filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_FILE_RS232_H__ */


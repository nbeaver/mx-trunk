/*
 * Name:    i_u500_rs232.h
 *
 * Purpose: Header file for sending program lines to Aerotech Unidex 500
 *          series motor controllers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2008-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_U500_RS232_H__
#define __I_U500_RS232_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *u500_record;
	long board_number;

	char com_port_name[MXU_FILENAME_LENGTH+1];
	char loopback_port_name[MXU_FILENAME_LENGTH+1];
	MX_RECORD *loopback_port_record;

	char putchar_buffer[500];
	unsigned long num_putchars_received;
	unsigned long num_write_terminators_seen;
} MX_U500_RS232;

MX_API mx_status_type mxi_u500_rs232_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxi_u500_rs232_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxi_u500_rs232_open( MX_RECORD *record );
MX_API mx_status_type mxi_u500_rs232_close( MX_RECORD *record );

MX_API mx_status_type mxi_u500_rs232_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_u500_rs232_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_u500_rs232_putline( MX_RS232 *rs232,
						char *buffer,
						size_t *bytes_written );

MX_API mx_status_type mxi_u500_rs232_num_input_bytes_available(MX_RS232 *rs232);

extern MX_RECORD_FUNCTION_LIST mxi_u500_rs232_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_u500_rs232_rs232_function_list;

extern long mxi_u500_rs232_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_u500_rs232_rfield_def_ptr;

#define MXI_U500_RS232_STANDARD_FIELDS \
  {-1, -1, "u500_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_RS232, u500_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "board_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_RS232, board_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "com_port_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_RS232, com_port_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "loopback_port_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_RS232, loopback_port_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_U500_RS232_H__ */


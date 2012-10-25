/*
 * Name:    i_edt_rs232.h
 *
 * Purpose: Header for MX driver to communicate with the serial port of
 *          an EDT video input board.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_EDT_RS232_H__
#define __I_EDT_RS232_H__

typedef struct {
	MX_RECORD *edt_vinput_record;
} MX_EDT_RS232;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_edt_rs232_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxi_edt_rs232_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxi_edt_rs232_open( MX_RECORD *record );

MX_API mx_status_type mxi_edt_rs232_getchar( MX_RS232 *rs232, char *c);
MX_API mx_status_type mxi_edt_rs232_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_edt_rs232_read( MX_RS232 *rs232,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read );
MX_API mx_status_type mxi_edt_rs232_write( MX_RS232 *rs232,
						char *buffer,
						size_t max_bytes_to_write,
						size_t *bytes_written );
MX_API mx_status_type mxi_edt_rs232_putline( MX_RS232 *rs232,
						char *buffer,
						size_t *bytes_written );
MX_API mx_status_type mxi_edt_rs232_num_input_bytes_available(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_edt_rs232_discard_unread_input(
							MX_RS232 *rs232 );

extern MX_RECORD_FUNCTION_LIST mxi_edt_rs232_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_edt_rs232_rs232_function_list;

extern long mxi_edt_rs232_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_edt_rs232_rfield_def_ptr;

#define MXI_EDT_RS232_STANDARD_FIELDS \
  {-1, -1, "edt_vinput_record", MXFT_RECORD, NULL, 0, {0}, \
      MXF_REC_TYPE_STRUCT, offsetof(MX_EDT_RS232, edt_vinput_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __I_EDT_RS232_H__ */


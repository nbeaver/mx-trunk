/*
 * Name:    i_epix_rs232.h
 *
 * Purpose: Header for MX driver to communicate with the serial port of
 *          an EPIX, Inc. video input board.
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

#ifndef __I_EPIX_RS232_H__
#define __I_EPIX_RS232_H__

#define MX_EPIX_RS232_RECEIVE_BUFFER_SIZE   1024

typedef struct {
	MX_RECORD *xclib_record;
	long unit_number;

	long unitmap;
} MX_EPIX_RS232;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_epix_rs232_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxi_epix_rs232_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxi_epix_rs232_open( MX_RECORD *record );

MX_API mx_status_type mxi_epix_rs232_getchar( MX_RS232 *rs232, char *c);
MX_API mx_status_type mxi_epix_rs232_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_epix_rs232_read( MX_RS232 *rs232,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read );
MX_API mx_status_type mxi_epix_rs232_write( MX_RS232 *rs232,
						char *buffer,
						size_t max_bytes_to_write,
						size_t *bytes_written );
MX_API mx_status_type mxi_epix_rs232_num_input_bytes_available(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_epix_rs232_discard_unread_input(
							MX_RS232 *rs232 );

extern MX_RECORD_FUNCTION_LIST mxi_epix_rs232_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_epix_rs232_rs232_function_list;

extern long mxi_epix_rs232_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_epix_rs232_rfield_def_ptr;

#define MXI_EPIX_RS232_STANDARD_FIELDS \
  {-1, -1, "xclib_record", MXFT_RECORD, NULL, 0, {0}, \
      MXF_REC_TYPE_STRUCT, offsetof(MX_EPIX_RS232, xclib_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "unit_number", MXFT_LONG, NULL, 0, {0}, \
      MXF_REC_TYPE_STRUCT, offsetof(MX_EPIX_RS232, unit_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __I_EPIX_RS232_H__ */


/*
 * Name:    i_soft_rs232.h
 *
 * Purpose: Header for MX driver for software emulated RS-232 devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SOFT_RS232_H__
#define __I_SOFT_RS232_H__

/* Define the data structures used by a SOFT_RS232 interface. */

#define MXI_SOFT_RS232_BUFFER_LENGTH	500

typedef struct {
	char *read_ptr;
	char *write_ptr;
	char *end_ptr;
	char buffer[ MXI_SOFT_RS232_BUFFER_LENGTH + 1 ];
} MX_SOFT_RS232;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_soft_rs232_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxi_soft_rs232_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxi_soft_rs232_open( MX_RECORD *record );

MX_API mx_status_type mxi_soft_rs232_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_soft_rs232_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_soft_rs232_num_input_bytes_available(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_soft_rs232_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_soft_rs232_discard_unwritten_output( MX_RS232 *rs232);

extern MX_RECORD_FUNCTION_LIST mxi_soft_rs232_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_soft_rs232_rs232_function_list;

extern mx_length_type mxi_soft_rs232_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_soft_rs232_rfield_def_ptr;

#endif /* __I_SOFT_RS232_H__ */


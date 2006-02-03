/*
 * Name:    i_vms_terminal.h
 *
 * Purpose: Header for MX driver for VMS terminal ports.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_VMS_TERMINAL_H__
#define __I_VMS_TERMINAL_H__

#include "mx_record.h"

/* Define all of the interface functions. */

MX_API mx_status_type mxi_vms_terminal_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_vms_terminal_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxi_vms_terminal_open( MX_RECORD *record );

MX_API mx_status_type mxi_vms_terminal_close( MX_RECORD *record );

MX_API mx_status_type mxi_vms_terminal_getchar( MX_RS232 *rs232, char *c );

MX_API mx_status_type mxi_vms_terminal_putchar( MX_RS232 *rs232, char c );

MX_API mx_status_type mxi_vms_terminal_read( MX_RS232 *rs232,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read );

MX_API mx_status_type mxi_vms_terminal_write( MX_RS232 *rs232,
						char *buffer,
						size_t max_bytes_to_write,
						size_t *bytes_written );

MX_API mx_status_type mxi_vms_terminal_getline( MX_RS232 *rs232,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read );

MX_API mx_status_type mxi_vms_terminal_putline( MX_RS232 *rs232,
						char *buffer,
						size_t *bytes_written );

MX_API mx_status_type mxi_vms_terminal_num_input_bytes_available(
							MX_RS232 *rs232 );

MX_API mx_status_type mxi_vms_terminal_discard_unread_input( MX_RS232 *rs232 );

MX_API mx_status_type mxi_vms_terminal_discard_unwritten_output(
							MX_RS232 *rs232 );

/* Define the data structures used by a VMS terminal device. */

typedef struct {
	char device_name[MXU_FILENAME_LENGTH + 1];
	unsigned short vms_channel;

	unsigned short read_terminator_quadword[4];
	unsigned char read_terminator_mask[32];
} MX_VMS_TERMINAL;

extern MX_RECORD_FUNCTION_LIST mxi_vms_terminal_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_vms_terminal_rs232_function_list;

extern mx_length_type mxi_vms_terminal_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_vms_terminal_rfield_def_ptr;

#define MXI_VMS_TERMINAL_STANDARD_FIELDS \
  {-1, -1, "device_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VMS_TERMINAL, device_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_VMS_TERMINAL_H__ */


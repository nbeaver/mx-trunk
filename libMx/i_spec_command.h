/*
 * Name:    i_spec_command.h
 *
 * Purpose: Header for MX driver for sending commands to and receiving
 *          responses from the Spec command line as if it were an
 *          RS-232 device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SPEC_COMMAND_H__
#define __I_SPEC_COMMAND_H__

#include "mx_record.h"

/* Define all of the interface functions. */

MX_API mx_status_type mxi_spec_command_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_spec_command_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxi_spec_command_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_spec_command_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
MX_API mx_status_type mxi_spec_command_num_input_bytes_available(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_spec_command_discard_unread_input( MX_RS232 *rs232 );

/* Values for the 'spec_command_flags' field. */

/* Define the data structures used by this type of interface. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *spec_server_record;
	unsigned long spec_command_flags;
} MX_SPEC_COMMAND;

extern MX_RECORD_FUNCTION_LIST mxi_spec_command_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_spec_command_rs232_function_list;

extern mx_length_type mxi_spec_command_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_spec_command_rfield_def_ptr;

#define MXI_SPEC_COMMAND_STANDARD_FIELDS \
  {-1, -1, "spec_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_COMMAND, spec_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "spec_command_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_COMMAND, spec_command_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_SPEC_COMMAND_H__ */


/*
 * Name:    i_flowbus.h
 *
 * Purpose: Header file for Bronkhorst FLOW-BUS protocol interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_FLOWBUS_H__
#define __I_FLOWBUS_H__

/* Values for protocol_type */

#define MXT_FLOWBUS_ASCII	1
#define MXT_FLOWBUS_BINARY	2

/* Values for flowbus_flags */

#define MXF_FLOWBUS_DEBUG	0x1

/*---*/

#define MXU_FLOWBUS_PROTOCOL_TYPE_STRING_LENGTH		20

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	char protocol_type_string[MXU_FLOWBUS_PROTOCOL_TYPE_STRING_LENGTH+1];
	unsigned long flowbus_flags;

	unsigned long protocol_type;
	unsigned long server_node;
	unsigned long sequence_number;

	uint8_t command_buffer[80];
	uint8_t response_buffer[80];
} MX_FLOWBUS;

#define MXI_FLOWBUS_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "protocol_type_string", MXFT_STRING, NULL, \
				1, {MXU_FLOWBUS_PROTOCOL_TYPE_STRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, protocol_type_string), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "flowbus_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, flowbus_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "protocol_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, protocol_type), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "server_node", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, server_node), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "sequence_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, sequence_number), \
	{0}, NULL, 0 }

MX_API mx_status_type mxi_flowbus_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_flowbus_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_flowbus_record_function_list;

extern long mxi_flowbus_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxi_flowbus_rfield_def_ptr;

MX_EXPORT mx_status_type
mxi_flowbus_command( MX_FLOWBUS *flowbus, char *command,
			char *response, size_t max_response_length );

/*---*/

MX_EXPORT mx_status_type
mxi_flowbus_send_parameter( MX_FLOWBUS *flowbus,
				unsigned long node,
				unsigned long process,
				unsigned long parameter,
				char *parameter_value_to_send,
				char *status_response,
				size_t max_response_length );

MX_EXPORT mx_status_type
mxi_flowbus_request_parameter( MX_FLOWBUS *flowbus,
				unsigned long node,
				unsigned long process,
				unsigned long parameter,
				char *requested_parameter_value,
				size_t max_response_length );

			

#endif /* __I_FLOWBUS_H__ */


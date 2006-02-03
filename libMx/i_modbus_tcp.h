/*
 * Name:    i_modbus_tcp.h
 *
 * Purpose: Header for MX interface driver for MODBUS/TCP fieldbus
 *          communication.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_MODBUS_TCP_H__
#define __I_MODBUS_TCP_H__

#define MXU_MODBUS_TCP_HEADER_LENGTH	7

#define MX_MODBUS_TCP_TRANSACTION_ID	0
#define MX_MODBUS_TCP_PROTOCOL_ID	2
#define MX_MODBUS_TCP_LENGTH		4
#define MX_MODBUS_TCP_UNIT_ID		6

/* Define the data structures used by the MODBUS/TCP interface code. */

typedef struct {
	MX_RECORD *record;

	MX_SOCKET *socket;
	char hostname[MXU_HOSTNAME_LENGTH + 1];
	int port_number;
	unsigned long unit_id;

	uint16_t transaction_id;

	uint8_t send_header[MXU_MODBUS_TCP_HEADER_LENGTH];
	uint8_t receive_header[MXU_MODBUS_TCP_HEADER_LENGTH];
} MX_MODBUS_TCP;

extern MX_RECORD_FUNCTION_LIST mxi_modbus_tcp_record_function_list;
extern MX_MODBUS_FUNCTION_LIST mxi_modbus_tcp_modbus_function_list;

extern mx_length_type mxi_modbus_tcp_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_modbus_tcp_rfield_def_ptr;

#define MXI_MODBUS_TCP_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_TCP, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_TCP, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "unit_id", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_TCP, unit_id), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxi_modbus_tcp_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_modbus_tcp_open( MX_RECORD *record );
MX_API mx_status_type mxi_modbus_tcp_close( MX_RECORD *record );

MX_API mx_status_type mxi_modbus_tcp_command( MX_MODBUS *modbus );

#endif /* __I_MODBUS_TCP_H__ */


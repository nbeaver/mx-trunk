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

#define MXF_FLOWBUS_DEBUG		0x1
#define	MXF_FLOWBUS_CACHE_VALUES	0x2

/* Values for flowbus_command_flags */

#define MXFCF_FLOWBUS_DEBUG		MXF_FLOWBUS_DEBUG
#define MXFCF_FLOWBUS_QUIET		0x1000

/* Flowbus parameter types */

#define MXDT_FLOWBUS_ILLEGAL		((unsigned long)(-1))

#define MXDT_FLOWBUS_UINT8		0x0
#define MXDT_FLOWBUS_USHORT		0x2
#define MXDT_FLOWBUS_ULONG_FLOAT	0x4
#define MXDT_FLOWBUS_STRING		0x6

/* Flowbus node address range. */

#define MXA_FLOWBUS_MIN_NODE_ADDRESS	3
#define MXA_FLOWBUS_MAX_NODE_ADDRESS	120

/* String length parameters
 *
 * Note that commands 01 and 02 can be longer than any other command.
 */

#define MXU_FLOWBUS_MAX_STRING_LENGTH	255	/* Max unsigned char */

#define MXU_FLOWBUS_HEADER_LENGTH	6	/* Command 01 or 02 */

#define MXU_FLOWBUS_TERMINATOR_LENGTH	2	/* CR, LF */

#define MXU_FLOWBUS_MAX_LENGTH		( MXU_FLOWBUS_MAX_STRING_LENGTH \
		+ MXU_FLOWBUS_HEADER_LENGTH + MXU_FLOWBUS_TERMINATOR_LENGTH )

/*---*/

#define MXU_FLOWBUS_PROTOCOL_TYPE_STRING_LENGTH		20

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	char protocol_type_string[MXU_FLOWBUS_PROTOCOL_TYPE_STRING_LENGTH+1];
	unsigned long flowbus_flags;

	unsigned long protocol_type;
	unsigned long master_address;
	unsigned long sequence_number;

	mx_bool_type show_nodes;

	/* Command can be sent across the Flow-Bus either by using
	 * Flow-Bus variable records or by manually constructing
	 * and sending the commands using the variables below.
	 */

	unsigned long node_address;
	unsigned long process_number;
	unsigned long parameter_number;

	uint8_t uint8_value;
	unsigned short ushort_value;
	unsigned long ulong_value;
	float float_value;
	char string_value[ MXU_FLOWBUS_MAX_STRING_LENGTH + 1];
} MX_FLOWBUS;

#define MXLV_FB_SHOW_NODES		87340
#define MXLV_FB_UINT8_VALUE		87341
#define MXLV_FB_USHORT_VALUE		87342
#define MXLV_FB_ULONG_VALUE		87343
#define MXLV_FB_FLOAT_VALUE		87344
#define MXLV_FB_STRING_VALUE		87345

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
  {-1, -1, "master_address", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, master_address), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "sequence_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, sequence_number), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "node_address", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, node_address), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "process_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, process_number), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "parameter_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, parameter_number), \
	{0}, NULL, 0 }, \
  \
  {MXLV_FB_SHOW_NODES, -1, "show_nodes", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, show_nodes), \
	{0}, NULL, 0 }, \
  \
  {MXLV_FB_UINT8_VALUE, -1, "uint8_value", MXFT_UINT8, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, uint8_value), \
	{0}, NULL, 0 }, \
  \
  {MXLV_FB_USHORT_VALUE, -1, "ushort_value", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, ushort_value), \
	{0}, NULL, 0 }, \
  \
  {MXLV_FB_ULONG_VALUE, -1, "ulong_value", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, ulong_value), \
	{0}, NULL, 0 }, \
  \
  {MXLV_FB_FLOAT_VALUE, -1, "float_value", MXFT_FLOAT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, float_value), \
	{0}, NULL, 0 }, \
  \
  {MXLV_FB_STRING_VALUE, -1, "string_value", \
			MXFT_STRING, NULL, 1, {MXU_FLOWBUS_MAX_STRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS, string_value), \
	{sizeof(char)}, NULL, 0 }

MX_API mx_status_type mxi_flowbus_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_flowbus_open( MX_RECORD *record );

MX_API mx_status_type mxi_flowbus_special_processing_setup( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_flowbus_record_function_list;

extern long mxi_flowbus_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxi_flowbus_rfield_def_ptr;

MX_API mx_status_type
mxi_flowbus_command( MX_FLOWBUS *flowbus, char *command,
			char *response, size_t max_response_length,
			unsigned long flowbus_command_flags );

/*---*/

MX_API mx_status_type
mxi_flowbus_send_parameter( MX_FLOWBUS *flowbus,
				unsigned long node_address,
				unsigned long process_number,
				unsigned long parameter_number,
				unsigned long flowbus_parameter_type,
				void *parameter_value_to_send,
				char *status_response,
				size_t max_response_length,
				unsigned long flowbus_command_flags );

MX_API mx_status_type
mxi_flowbus_request_parameter( MX_FLOWBUS *flowbus,
				unsigned long node_address,
				unsigned long process_number,
				unsigned long parameter_number,
				unsigned long flowbus_parameter_type,
				void *requested_parameter_value,
				size_t max_parameter_length,
				unsigned long flowbus_command_flags );

/*---*/

MX_API mx_status_type
mxi_flowbus_format_string( char *external_buffer,
			size_t external_buffer_bytes,
			unsigned long field_number,
			unsigned long mx_datatype,
			void *value_ptr );
			

#endif /* __I_FLOWBUS_H__ */


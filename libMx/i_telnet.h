/*
 * Name:    i_telnet.h
 *
 * Purpose: Header for MX driver for treating a connection to a Telnet server
 *          as if it were an RS-232 device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_TELNET_H__
#define __I_TELNET_H__

/* Telnet command values. */

#define MXF_TELNET_SE			(0xf0)
#define MXF_TELNET_NOP			(0xf1)
#define MXF_TELNET_DATA_MARK		(0xf2)
#define MXF_TELNET_BREAK		(0xf3)
#define MXF_TELNET_INTERRUPT_PROCESS	(0xf4)
#define MXF_TELNET_ABORT_OUTPUT		(0xf5)
#define MXF_TELNET_ARE_YOU_THERE	(0xf6)
#define MXF_TELNET_ERASE_CHARACTER	(0xf7)
#define MXF_TELNET_ERASE_LINE		(0xf8)
#define MXF_TELNET_GO_AHEAD		(0xf9)
#define MXF_TELNET_SB			(0xfa)
#define MXF_TELNET_WILL			(0xfb)
#define MXF_TELNET_WONT			(0xfc)
#define MXF_TELNET_DO			(0xfd)
#define MXF_TELNET_DONT			(0xfe)
#define MXF_TELNET_IAC			(0xff)

/* Values for the 'telnet_flags' field. */

#define MXF_TELNET_QUIET			0x2

/* Define the data structures used by this type of interface. */

typedef struct {
	MX_RECORD *record;

	MX_SOCKET *socket;
	char hostname[MXU_HOSTNAME_LENGTH + 1];
	long port_number;
	unsigned long telnet_flags;

	unsigned char saved_byte;
	mx_bool_type saved_byte_is_available;

	mx_bool_type client_sends_binary;
	mx_bool_type server_sends_binary;

	mx_bool_type server_echo_on;
} MX_TELNET;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_telnet_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_telnet_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_telnet_open( MX_RECORD *record );
MX_API mx_status_type mxi_telnet_close( MX_RECORD *record );

MX_API mx_status_type mxi_telnet_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_telnet_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_telnet_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );
MX_API mx_status_type mxi_telnet_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
MX_API mx_status_type mxi_telnet_num_input_bytes_available( MX_RS232 *rs232 );
MX_API mx_status_type mxi_telnet_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_telnet_discard_unwritten_output( MX_RS232 *rs232 );
MX_API mx_status_type mxi_telnet_send_break( MX_RS232 *rs232 );

MX_API mx_status_type mxi_telnet_handle_commands( MX_RS232 *rs232,
					unsigned long delay_milliseconds,
					unsigned long max_retries );

extern MX_RECORD_FUNCTION_LIST mxi_telnet_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_telnet_rs232_function_list;

extern long mxi_telnet_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_telnet_rfield_def_ptr;

#define MXI_TELNET_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TELNET, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TELNET, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "telnet_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TELNET, telnet_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_TELNET_H__ */


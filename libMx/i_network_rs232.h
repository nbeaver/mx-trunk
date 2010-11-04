/*
 * Name:    i_network_rs232.h
 *
 * Purpose: Header for MX driver for network RS-232 devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2003-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NETWORK_RS232_H__
#define __I_NETWORK_RS232_H__

#define MX_VERSION_GETCHAR_PUTCHAR_HAS_CHAR_TYPE	1002000UL

/* Define the data structures used by a NETWORK_RS232 interface. */

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	mx_bool_type getchar_putchar_is_char;

	long remote_read_buffer_length;
	long remote_write_buffer_length;
	long remote_getline_buffer_length;
	long remote_putline_buffer_length;

	MX_NETWORK_FIELD discard_unread_input_nf;
	MX_NETWORK_FIELD discard_unwritten_output_nf;
	MX_NETWORK_FIELD flow_control_nf;
	MX_NETWORK_FIELD getchar_nf;
	MX_NETWORK_FIELD get_configuration_nf;
	MX_NETWORK_FIELD num_input_bytes_available_nf;
	MX_NETWORK_FIELD parity_nf;
	MX_NETWORK_FIELD putchar_nf;
	MX_NETWORK_FIELD read_terminators_nf;
	MX_NETWORK_FIELD resynchronize_nf;
	MX_NETWORK_FIELD send_break_nf;
	MX_NETWORK_FIELD set_configuration_nf;
	MX_NETWORK_FIELD signal_state_nf;
	MX_NETWORK_FIELD speed_nf;
	MX_NETWORK_FIELD stop_bits_nf;
	MX_NETWORK_FIELD word_size_nf;
	MX_NETWORK_FIELD write_terminators_nf;
} MX_NETWORK_RS232;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_network_rs232_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxi_network_rs232_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxi_network_rs232_open( MX_RECORD *record );
MX_API mx_status_type mxi_network_rs232_close( MX_RECORD *record );
MX_API mx_status_type mxi_network_rs232_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_network_rs232_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_network_rs232_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_network_rs232_read( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_network_rs232_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );
MX_API mx_status_type mxi_network_rs232_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_network_rs232_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
MX_API mx_status_type mxi_network_rs232_num_input_bytes_available(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_network_rs232_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_network_rs232_discard_unwritten_output(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_network_rs232_get_signal_state( MX_RS232 *rs232 );
MX_API mx_status_type mxi_network_rs232_set_signal_state( MX_RS232 *rs232 );
MX_API mx_status_type mxi_network_rs232_get_configuration( MX_RS232 *rs232 );
MX_API mx_status_type mxi_network_rs232_set_configuration( MX_RS232 *rs232 );
MX_API mx_status_type mxi_network_rs232_send_break( MX_RS232 *rs232 );

extern MX_RECORD_FUNCTION_LIST mxi_network_rs232_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_network_rs232_rs232_function_list;

extern long mxi_network_rs232_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_network_rs232_rfield_def_ptr;

#define MXI_NETWORK_RS232_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_RS232, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_RS232, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __I_NETWORK_RS232_H__ */


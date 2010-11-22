/*
 * Name:    i_sim900_port.h
 *
 * Purpose: Header for MX driver for connecting to SIM900 ports.
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

#ifndef __I_SIM900_PORT_H__
#define __I_SIM900_PORT_H__

/* Define the data structures used by this type of interface. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *sim900_record;
	char port_name;
} MX_SIM900_PORT;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_sim900_port_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_sim900_port_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_sim900_port_open( MX_RECORD *record );
MX_API mx_status_type mxi_sim900_port_close( MX_RECORD *record );

MX_API mx_status_type mxi_sim900_port_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_sim900_port_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_sim900_port_read( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_sim900_port_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );
MX_API mx_status_type mxi_sim900_port_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_sim900_port_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
MX_API mx_status_type mxi_sim900_port_num_input_bytes_available(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_sim900_port_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_sim900_port_get_configuration( MX_RS232 *rs232 );
MX_API mx_status_type mxi_sim900_port_set_configuration( MX_RS232 *rs232 );
MX_API mx_status_type mxi_sim900_port_send_break( MX_RS232 *rs232 );

extern MX_RECORD_FUNCTION_LIST mxi_sim900_port_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_sim900_port_rs232_function_list;

extern long mxi_sim900_port_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_sim900_port_rfield_def_ptr;

#define MXI_SIM900_PORT_STANDARD_FIELDS \
  {-1, -1, "sim900_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM900_PORT, sim900_record), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM900_PORT, port_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_SIM900_PORT_H__ */


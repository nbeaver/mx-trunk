/*
 * Name:    i_wago750_serial.h
 *
 * Purpose: Header for MX driver for Wago 750-65x serial interface modules.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_WAGO750_SERIAL_H__
#define __I_WAGO750_SERIAL_H__

/* Bit definitions in the control byte. */

#define MXF_WAGO750_SERIAL_CTRL_TRANSMIT_REQUEST		0x01
#define MXF_WAGO750_SERIAL_CTRL_RECEIVE_ACKNOWLEDGE		0x02
#define MXF_WAGO750_SERIAL_CTRL_INITIALIZATION_REQUEST		0x04

#define MXF_WAGO750_SERIAL_CTRL_FRAMES_AVAILABLE		0x70

/* Bit definitions in the status byte. */

#define MXF_WAGO750_SERIAL_STAT_TRANSMIT_ACKNOWLEDGE		0x01
#define MXF_WAGO750_SERIAL_STAT_RECEIVE_REQUEST			0x02
#define MXF_WAGO750_SERIAL_STAT_INITIALIZATION_ACKNOWLEDGE	0x04
#define MXF_WAGO750_SERIAL_STAT_INPUT_BUFFER_IS_FULL		0x08
#define MXF_WAGO750_SERIAL_STAT_FRAMES_AVAILABLE		0x70

/* Define the data structures used by a Wago 750-65x serial interface. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bus_record;
	unsigned long input_bus_address;
	unsigned long output_bus_address;
	int num_data_bytes;	/* Either 3 or 5. */

	unsigned int num_input_bytes_in_buffer;
	unsigned int next_input_byte_index;
	uint8_t input_buffer[5];

	int interleaved_data_addresses;
	int control_address;
	int status_address;

	int num_data_bytes_warning;

	unsigned long wait_ms;
	unsigned long max_attempts;
	double actual_timeout;
} MX_WAGO750_SERIAL;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_wago750_serial_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxi_wago750_serial_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxi_wago750_serial_open( MX_RECORD *record );
MX_API mx_status_type mxi_wago750_serial_close( MX_RECORD *record );
MX_API mx_status_type mxi_wago750_serial_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_wago750_serial_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_wago750_serial_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_wago750_serial_read( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_wago750_serial_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );
MX_API mx_status_type mxi_wago750_serial_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_wago750_serial_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
MX_API mx_status_type mxi_wago750_serial_num_input_bytes_available(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_wago750_serial_discard_unread_input(
							MX_RS232 *rs232 );
MX_API mx_status_type mxi_wago750_serial_discard_unwritten_output(
							MX_RS232 *rs232 );

extern MX_RECORD_FUNCTION_LIST mxi_wago750_serial_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_wago750_serial_rs232_function_list;

extern mx_length_type mxi_wago750_serial_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_wago750_serial_rfield_def_ptr;

#define MXI_WAGO750_SERIAL_STANDARD_FIELDS \
  {-1, -1, "bus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_WAGO750_SERIAL, bus_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "input_bus_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_WAGO750_SERIAL, input_bus_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "output_bus_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_WAGO750_SERIAL, output_bus_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_data_bytes", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_WAGO750_SERIAL, num_data_bytes), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __I_WAGO750_SERIAL_H__ */


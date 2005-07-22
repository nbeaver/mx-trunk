/*
 * Name:    i_epics_rs232.h
 *
 * Purpose: Header for MX driver for RS-232 devices controlled via
 *          the EPICS RS-232 record written by Mark Rivers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_EPICS_RS232_H__
#define __I_EPICS_RS232_H__

#include "mx_record.h"

/* Flags for DBF_RECCHOICE process variables. */

#define MXF_EPICS_RS232_WRITE_READ	0
#define MXF_EPICS_RS232_WRITE		1
#define MXF_EPICS_RS232_READ		2

#define MXF_EPICS_RS232_ASCII_FORMAT	0
#define MXF_EPICS_RS232_BINARY_FORMAT	1

#define MXF_EPICS_RS232_BPS_300		0
#define MXF_EPICS_RS232_BPS_600		1
#define MXF_EPICS_RS232_BPS_1200	2
#define MXF_EPICS_RS232_BPS_2400	3
#define MXF_EPICS_RS232_BPS_4800	4
#define MXF_EPICS_RS232_BPS_9600	5
#define MXF_EPICS_RS232_BPS_19200	6
#define MXF_EPICS_RS232_BPS_38400	7

#define MXF_EPICS_RS232_NO_PARITY	0
#define MXF_EPICS_RS232_EVEN_PARITY	1
#define MXF_EPICS_RS232_ODD_PARITY	2

#define MXF_EPICS_RS232_WORDSIZE_5	0
#define MXF_EPICS_RS232_WORDSIZE_6	1
#define MXF_EPICS_RS232_WORDSIZE_7	2
#define MXF_EPICS_RS232_WORDSIZE_8	3

#define MXF_EPICS_RS232_STOPBITS_1	0
#define MXF_EPICS_RS232_STOPBITS_2	1

#define MXF_EPICS_RS232_NO_FLOW_CONTROL	0
#define MXF_EPICS_RS232_HW_FLOW_CONTROL	1

/* EPICS RS-232 interface data structure. */

typedef struct {
	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	int max_input_length;
	char *input_buffer;
	char *next_unread_char_ptr;
	int last_read_length;

	int max_output_length;
	char *output_buffer;

	int input_delimiter_on;
	long transaction_mode;
	long timeout;
	long default_timeout;
	long num_chars_to_read;
	long num_chars_to_write;

	MX_EPICS_PV binp_pv;
	MX_EPICS_PV bout_pv;
	MX_EPICS_PV idel_pv;
	MX_EPICS_PV nord_pv;
	MX_EPICS_PV nowt_pv;
	MX_EPICS_PV nrrd_pv;
	MX_EPICS_PV tmod_pv;
	MX_EPICS_PV tmot_pv;
} MX_EPICS_RS232;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_epics_rs232_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxi_epics_rs232_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxi_epics_rs232_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_epics_rs232_open( MX_RECORD *record );

MX_API mx_status_type mxi_epics_rs232_getchar( MX_RS232 *rs232, char *c );

MX_API mx_status_type mxi_epics_rs232_putchar( MX_RS232 *rs232, char c );

MX_API mx_status_type mxi_epics_rs232_read( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );

MX_API mx_status_type mxi_epics_rs232_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );

MX_API mx_status_type mxi_epics_rs232_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );

MX_API mx_status_type mxi_epics_rs232_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );

MX_API mx_status_type mxi_epics_rs232_num_input_bytes_available(
					MX_RS232 *rs232 );

MX_API mx_status_type mxi_epics_rs232_discard_unread_input( MX_RS232 *rs232 );

MX_API mx_status_type mxi_epics_rs232_discard_unwritten_output(MX_RS232 *rs232);

extern MX_RECORD_FUNCTION_LIST mxi_epics_rs232_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_epics_rs232_rs232_function_list;

extern long mxi_epics_rs232_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_epics_rs232_rfield_def_ptr;

#define MXI_EPICS_RS232_STANDARD_FIELDS \
  {-1, -1, "epics_record_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_RS232, epics_record_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_EPICS_RS232_H__ */


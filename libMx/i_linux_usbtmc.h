/*
 * Name:    i_linux_usbtmc.h
 *
 * Purpose: Header for MX driver for treating a TCP socket connection
 *          as if it were an RS-232 device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2006, 2010, 2013, 2015-2016, 2018
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_LINUX_USBTMC_H__
#define __I_LINUX_USBTMC_H__

#include "mx_record.h"

/* Define all of the interface functions. */

MX_API mx_status_type mxi_linux_usbtmc_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_linux_usbtmc_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_linux_usbtmc_open( MX_RECORD *record );
MX_API mx_status_type mxi_linux_usbtmc_close( MX_RECORD *record );
MX_API mx_status_type mxi_linux_usbtmc_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_linux_usbtmc_getchar( MX_RS232 *rs232, char *c );
MX_API mx_status_type mxi_linux_usbtmc_putchar( MX_RS232 *rs232, char c );
MX_API mx_status_type mxi_linux_usbtmc_read( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_linux_usbtmc_write( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_write,
					size_t *bytes_written );
MX_API mx_status_type mxi_linux_usbtmc_getline( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
MX_API mx_status_type mxi_linux_usbtmc_putline( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
MX_API mx_status_type mxi_linux_usbtmc_num_input_bytes_available( MX_RS232 *rs232 );
MX_API mx_status_type mxi_linux_usbtmc_discard_unread_input( MX_RS232 *rs232 );
MX_API mx_status_type mxi_linux_usbtmc_discard_unwritten_output( MX_RS232 *rs232 );
MX_API mx_status_type mxi_linux_usbtmc_wait_for_input_available( MX_RS232 *rs232,
						double wait_timeout_in_seconds);
MX_API mx_status_type mxi_linux_usbtmc_flush( MX_RS232 *rs232 );

/* Values for the 'usbtmc_flags' field. */

#define MXF_LINUX_USBTMC_OPEN_DURING_TRANSACTION	0x1
#define MXF_LINUX_USBTMC_QUIET			0x2
#define MXF_LINUX_USBTMC_USE_MX_RECEIVE_BUFFER	0x4
#define MXF_LINUX_USBTMC_AUTOMATIC_REOPEN		0x8
#define MXF_LINUX_USBTMC_USE_MX_SOCKET_RESYNCHRONIZE	0x10

/* Define the data structures used by this type of interface. */

typedef struct {
	MX_RECORD *record;

	MX_SOCKET *socket;
	char hostname[MXU_HOSTNAME_LENGTH + 1];
	long port_number;
	unsigned long usbtmc_flags;

	long receive_buffer_size;

	unsigned long resync_delay_milliseconds;
} MX_LINUX_USBTMC;

extern MX_RECORD_FUNCTION_LIST mxi_linux_usbtmc_record_function_list;
extern MX_RS232_FUNCTION_LIST mxi_linux_usbtmc_rs232_function_list;

extern long mxi_linux_usbtmc_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_linux_usbtmc_rfield_def_ptr;

#define MXI_LINUX_USBTMC_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_USBTMC, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_USBTMC, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "usbtmc_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_USBTMC, usbtmc_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "receive_buffer_size", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_USBTMC, receive_buffer_size), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "resync_delay_milliseconds", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_USBTMC, resync_delay_milliseconds), \
	{0}, NULL, 0 }

#endif /* __I_LINUX_USBTMC_H__ */


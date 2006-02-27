/*
 * Name:    i_network_gpib.h
 *
 * Purpose: Header for MX driver for network GPIB devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NETWORK_GPIB_H__
#define __I_NETWORK_GPIB_H__

/* Define the data structures used by a NETWORK_GPIB interface. */

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	long remote_read_buffer_length;
	long remote_write_buffer_length;

	MX_NETWORK_FIELD address_nf;
	MX_NETWORK_FIELD bytes_read_nf;
	MX_NETWORK_FIELD bytes_to_read_nf;
	MX_NETWORK_FIELD bytes_to_write_nf;
	MX_NETWORK_FIELD bytes_written_nf;
	MX_NETWORK_FIELD close_device_nf;
	MX_NETWORK_FIELD device_clear_nf;
	MX_NETWORK_FIELD go_to_local_nf;
	MX_NETWORK_FIELD interface_clear_nf;
	MX_NETWORK_FIELD local_lockout_nf;
	MX_NETWORK_FIELD open_device_nf;
	MX_NETWORK_FIELD read_nf;
	MX_NETWORK_FIELD remote_enable_nf;
	MX_NETWORK_FIELD resynchronize_nf;
	MX_NETWORK_FIELD selective_device_clear_nf;
	MX_NETWORK_FIELD serial_poll_nf;
	MX_NETWORK_FIELD serial_poll_disable_nf;
	MX_NETWORK_FIELD trigger_nf;
	MX_NETWORK_FIELD wait_for_service_request_nf;
	MX_NETWORK_FIELD write_nf;
} MX_NETWORK_GPIB;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_network_gpib_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxi_network_gpib_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxi_network_gpib_open( MX_RECORD *record );
MX_API mx_status_type mxi_network_gpib_close( MX_RECORD *record );
MX_API mx_status_type mxi_network_gpib_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_network_gpib_open_device( MX_GPIB *gpib,
							long address );
MX_API mx_status_type mxi_network_gpib_close_device( MX_GPIB *gpib,
							long address );
MX_API mx_status_type mxi_network_gpib_read( MX_GPIB *gpib,
						long address,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read,
						unsigned long flags );
MX_API mx_status_type mxi_network_gpib_write( MX_GPIB *gpib,
						long address,
						char *buffer,
						size_t bytes_to_write,
						size_t *bytes_written,
						unsigned long flags );
MX_API mx_status_type mxi_network_gpib_interface_clear( MX_GPIB *gpib );
MX_API mx_status_type mxi_network_gpib_device_clear( MX_GPIB *gpib );
MX_API mx_status_type mxi_network_gpib_selective_device_clear( MX_GPIB *gpib,
								long address );
MX_API mx_status_type mxi_network_gpib_local_lockout( MX_GPIB *gpib );
MX_API mx_status_type mxi_network_gpib_remote_enable( MX_GPIB *gpib,
								long address );
MX_API mx_status_type mxi_network_gpib_go_to_local( MX_GPIB *gpib,
								long address );
MX_API mx_status_type mxi_network_gpib_trigger( MX_GPIB *gpib, long address );
MX_API mx_status_type mxi_network_gpib_wait_for_service_request( MX_GPIB *gpib,
							double timeout );
MX_API mx_status_type mxi_network_gpib_serial_poll( MX_GPIB *gpib,
					long address,
					unsigned char *serial_poll_byte );
MX_API mx_status_type mxi_network_gpib_serial_poll_disable( MX_GPIB *gpib );

extern MX_RECORD_FUNCTION_LIST mxi_network_gpib_record_function_list;
extern MX_GPIB_FUNCTION_LIST mxi_network_gpib_gpib_function_list;

extern long mxi_network_gpib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_network_gpib_rfield_def_ptr;

#define MXI_NETWORK_GPIB_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_GPIB, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_GPIB, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __I_NETWORK_GPIB_H__ */


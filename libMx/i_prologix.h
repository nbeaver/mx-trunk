/*
 * Name:    i_prologix.h
 *
 * Purpose: Header for MX driver for the Prologix GPIB-USB and GPIB-Ethernet
 *          interfaces in System Controller mode.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2009, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PROLOGIX_H__
#define __I_PROLOGIX_H__

/* Values for the 'prologix_flags' field. */

#define MXF_PROLOGIX_FOOBAR		0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;

	int current_address;

	size_t write_buffer_length;
	char *write_buffer;
} MX_PROLOGIX;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_prologix_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_prologix_open( MX_RECORD *record );

MX_API mx_status_type mxi_prologix_open_device(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_prologix_close_device(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_prologix_read( MX_GPIB *gpib,
						long address,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read,
						unsigned long flags);
MX_API mx_status_type mxi_prologix_write( MX_GPIB *gpib,
						long address,
						char *buffer,
						size_t bytes_to_write,
						size_t *bytes_written,
						unsigned long flags);
MX_API mx_status_type mxi_prologix_interface_clear(MX_GPIB *gpib);
MX_API mx_status_type mxi_prologix_device_clear(MX_GPIB *gpib);
MX_API mx_status_type mxi_prologix_selective_device_clear(MX_GPIB *gpib,
						long address);
MX_API mx_status_type mxi_prologix_local_lockout(MX_GPIB *gpib);
MX_API mx_status_type mxi_prologix_remote_enable(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_prologix_go_to_local(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_prologix_trigger(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_prologix_wait_for_service_request( MX_GPIB *gpib,
						double timeout );
MX_API mx_status_type mxi_prologix_serial_poll( MX_GPIB *gpib, long address,
					unsigned char *serial_poll_byte );
MX_API mx_status_type mxi_prologix_serial_poll_disable(MX_GPIB *gpib);

extern MX_RECORD_FUNCTION_LIST mxi_prologix_record_function_list;
extern MX_GPIB_FUNCTION_LIST mxi_prologix_gpib_function_list;

extern long mxi_prologix_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_prologix_rfield_def_ptr;

#define MXI_PROLOGIX_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PROLOGIX, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_PROLOGIX_H__ */


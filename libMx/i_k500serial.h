/*
 * Name:    i_k500serial.h
 *
 * Purpose: Header for MX driver for the Keithley 500-SERIAL
 *          RS232 to GPIB interfaces.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_K500SERIAL_H__
#define __I_K500SERIAL_H__

#include "mx_record.h"

/* Define the data structure used by the Keithley 500-SERIAL GPIB interface. */

typedef struct {
	MX_RECORD *rs232_record;
} MX_K500SERIAL;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_k500serial_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_k500serial_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_k500serial_print_interface_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxi_k500serial_open( MX_RECORD *record );

MX_API mx_status_type mxi_k500serial_open_device(MX_GPIB *gpib, int address);
MX_API mx_status_type mxi_k500serial_close_device(MX_GPIB *gpib, int address);
MX_API mx_status_type mxi_k500serial_read( MX_GPIB *gpib,
						int address,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read,
						int flags);
MX_API mx_status_type mxi_k500serial_write( MX_GPIB *gpib,
						int address,
						char *buffer,
						size_t bytes_to_write,
						size_t *bytes_written,
						int flags);
MX_API mx_status_type mxi_k500serial_interface_clear(MX_GPIB *gpib);
MX_API mx_status_type mxi_k500serial_device_clear(MX_GPIB *gpib);
MX_API mx_status_type mxi_k500serial_selective_device_clear(MX_GPIB *gpib,
						int address);
MX_API mx_status_type mxi_k500serial_local_lockout(MX_GPIB *gpib);
MX_API mx_status_type mxi_k500serial_remote_enable(MX_GPIB *gpib, int address);
MX_API mx_status_type mxi_k500serial_go_to_local(MX_GPIB *gpib, int address);
MX_API mx_status_type mxi_k500serial_trigger(MX_GPIB *gpib, int address);
MX_API mx_status_type mxi_k500serial_wait_for_service_request( MX_GPIB *gpib,
						double timeout );
MX_API mx_status_type mxi_k500serial_serial_poll( MX_GPIB *gpib, int address,
					unsigned char *serial_poll_byte );
MX_API mx_status_type mxi_k500serial_serial_poll_disable(MX_GPIB *gpib);

extern MX_RECORD_FUNCTION_LIST mxi_k500serial_record_function_list;
extern MX_GPIB_FUNCTION_LIST mxi_k500serial_gpib_function_list;

extern mx_length_type mxi_k500serial_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_k500serial_rfield_def_ptr;

#define MXI_K500SERIAL_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_K500SERIAL, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_K500SERIAL_H__ */


/*
 * Name:    i_micro488ex.h
 *
 * Purpose: Header for MX driver for the IOtech Micro488EX RS232 to GPIB
 *          interface in System Controller mode.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_MICRO488EX_H__
#define __I_MICRO488EX_H__

/* Values for the 'micro488ex_flags' field. */

#define MXF_MICRO488EX_DISABLE_ERROR_CHECKING		0x1

#define MXF_MICRO488EX_RESTORE_TO_FACTORY_DEFAULTS	0x1000

/* Define the data structure used by the Micro488EX interface. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long micro488ex_flags;
} MX_MICRO488EX;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_micro488ex_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_micro488ex_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_micro488ex_print_interface_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxi_micro488ex_open( MX_RECORD *record );

MX_API mx_status_type mxi_micro488ex_open_device( MX_GPIB *gpib,
						int32_t address );
MX_API mx_status_type mxi_micro488ex_close_device( MX_GPIB *gpib,
						int32_t address );
MX_API mx_status_type mxi_micro488ex_read( MX_GPIB *gpib,
						int32_t address,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read,
						mx_hex_type flags );
MX_API mx_status_type mxi_micro488ex_write( MX_GPIB *gpib,
						int32_t address,
						char *buffer,
						size_t bytes_to_write,
						size_t *bytes_written,
						mx_hex_type flags );
MX_API mx_status_type mxi_micro488ex_interface_clear( MX_GPIB *gpib );
MX_API mx_status_type mxi_micro488ex_device_clear( MX_GPIB *gpib );
MX_API mx_status_type mxi_micro488ex_selective_device_clear( MX_GPIB *gpib,
						int32_t address );
MX_API mx_status_type mxi_micro488ex_local_lockout( MX_GPIB *gpib );
MX_API mx_status_type mxi_micro488ex_remote_enable( MX_GPIB *gpib,
						int32_t address );
MX_API mx_status_type mxi_micro488ex_go_to_local( MX_GPIB *gpib,
						int32_t address );
MX_API mx_status_type mxi_micro488ex_trigger( MX_GPIB *gpib, int32_t address );
MX_API mx_status_type mxi_micro488ex_wait_for_service_request( MX_GPIB *gpib,
						double timeout );
MX_API mx_status_type mxi_micro488ex_serial_poll( MX_GPIB *gpib,
						int32_t address,
						uint8_t *serial_poll_byte );
MX_API mx_status_type mxi_micro488ex_serial_poll_disable(MX_GPIB *gpib);

extern MX_RECORD_FUNCTION_LIST mxi_micro488ex_record_function_list;
extern MX_GPIB_FUNCTION_LIST mxi_micro488ex_gpib_function_list;

extern mx_length_type mxi_micro488ex_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_micro488ex_rfield_def_ptr;

#define MXI_MICRO488EX_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MICRO488EX, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "micro488ex_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MICRO488EX, micro488ex_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_MICRO488EX_H__ */


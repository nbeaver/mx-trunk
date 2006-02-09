/*
 * Name:    i_ni488.h
 *
 * Purpose: Header for MX driver for National Instruments GPIB interfaces
 *          and for the Linux Lab Project GPIB interface.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NI488_H__
#define __I_NI488_H__

#include "mx_record.h"

/* Define the data structure used by National Instruments GPIB interfaces. */

typedef struct {
	MX_RECORD *record;

	int32_t board_number;
	int32_t board_descriptor;
	int32_t device_descriptor[MX_NUM_GPIB_ADDRESSES];
} MX_NI488;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_ni488_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_ni488_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_ni488_print_interface_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxi_ni488_open( MX_RECORD *record );

MX_API mx_status_type mxi_ni488_open_device( MX_GPIB *gpib, int32_t address );
MX_API mx_status_type mxi_ni488_close_device( MX_GPIB *gpib, int32_t address );
MX_API mx_status_type mxi_ni488_read( MX_GPIB *gpib,
						int32_t address,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read,
						mx_hex_type flags );
MX_API mx_status_type mxi_ni488_write( MX_GPIB *gpib,
						int32_t address,
						char *buffer,
						size_t bytes_to_write,
						size_t *bytes_written,
						mx_hex_type flags );
MX_API mx_status_type mxi_ni488_interface_clear( MX_GPIB *gpib );
MX_API mx_status_type mxi_ni488_device_clear( MX_GPIB *gpib );
MX_API mx_status_type mxi_ni488_selective_device_clear( MX_GPIB *gpib,
						int32_t address );
MX_API mx_status_type mxi_ni488_local_lockout( MX_GPIB *gpib );
MX_API mx_status_type mxi_ni488_remote_enable( MX_GPIB *gpib, int32_t address );
MX_API mx_status_type mxi_ni488_go_to_local( MX_GPIB *gpib, int32_t address );
MX_API mx_status_type mxi_ni488_trigger( MX_GPIB *gpib, int32_t address );
MX_API mx_status_type mxi_ni488_wait_for_service_request( MX_GPIB *gpib,
						double timeout );
MX_API mx_status_type mxi_ni488_serial_poll( MX_GPIB *gpib,
						int32_t address,
						uint8_t *serial_poll_byte );
MX_API mx_status_type mxi_ni488_serial_poll_disable( MX_GPIB *gpib );

extern MX_RECORD_FUNCTION_LIST mxi_ni488_record_function_list;
extern MX_GPIB_FUNCTION_LIST mxi_ni488_gpib_function_list;

extern mx_length_type mxi_ni488_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_ni488_rfield_def_ptr;

#define MXI_NI488_STANDARD_FIELDS \
  {-1, -1, "board_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI488, board_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "board_descriptor", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI488, board_descriptor), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "device_descriptor", MXFT_INT32, NULL, 1, {MX_NUM_GPIB_ADDRESSES}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NI488, device_descriptor), \
	{sizeof(int)}, NULL, 0}

#endif /* __I_NI488_H__ */


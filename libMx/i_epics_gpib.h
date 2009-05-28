/*
 * Name:    i_epics_gpib.h
 *
 * Purpose: Header for MX driver for GPIB devices controlled via
 *          the EPICS GPIB recrod written by Mark Rivers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_EPICS_GPIB_H__
#define __I_EPICS_GPIB_H__

#include "mx_record.h"

#define MXF_EPICS_GPIB_MAX_BUFFER_LENGTH	512

/* Flags for DBF_RECCHOICE process variables. */

#define MXF_EPICS_GPIB_WRITE_READ		0
#define MXF_EPICS_GPIB_WRITE			1
#define MXF_EPICS_GPIB_READ			2

#define MXF_EPICS_GPIB_ASCII_FORMAT		0
#define MXF_EPICS_GPIB_BINARY_FORMAT		1

#define MXF_EPICS_GPIB_UNIV_NONE		0
#define MXF_EPICS_GPIB_DEVICE_CLEAR		1
#define MXF_EPICS_GPIB_LOCAL_LOCKOUT		2
#define MXF_EPICS_GPIB_SERIAL_POLL_DISABLE	3
#define MXF_EPICS_GPIB_SERIAL_POLL_ENABLE	4
#define MXF_EPICS_GPIB_UNLISTEN			5
#define MXF_EPICS_GPIB_UNTALK			6

#define MXF_EPICS_GPIB_ADDR_NONE		0
#define MXF_EPICS_GPIB_GROUP_EXECUTE_TRIGGER	1
#define MXF_EPICS_GPIB_GO_TO_LOCAL		2
#define MXF_EPICS_GPIB_SELECTED_DEVICE_CLEAR	3
#define MXF_EPICS_GPIB_TAKE_CONTROL		4
#define MXF_EPICS_GPIB_SERIAL_POLL		5

/* EPICS GPIB data structure */

typedef struct {
	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	int32_t max_input_length;
	int32_t max_output_length;
	int32_t current_eos_char;

	long transaction_mode;
	long timeout;
	long default_timeout;

	long current_address;
	long num_chars_to_read;
	long num_chars_to_write;

	MX_EPICS_PV addr_pv;
	MX_EPICS_PV binp_pv;
	MX_EPICS_PV bout_pv;
	MX_EPICS_PV eos_pv;
	MX_EPICS_PV nord_pv;
	MX_EPICS_PV nowt_pv;
	MX_EPICS_PV nrrd_pv;
	MX_EPICS_PV tmod_pv;
	MX_EPICS_PV tmot_pv;
} MX_EPICS_GPIB;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_epics_gpib_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_epics_gpib_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_epics_gpib_print_interface_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxi_epics_gpib_open( MX_RECORD *record );

MX_API mx_status_type mxi_epics_gpib_open_device(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_epics_gpib_close_device(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_epics_gpib_read( MX_GPIB *gpib,
						long address,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read,
						unsigned long flags);
MX_API mx_status_type mxi_epics_gpib_write( MX_GPIB *gpib,
						long address,
						char *buffer,
						size_t bytes_to_write,
						size_t *bytes_written,
						unsigned long flags);
MX_API mx_status_type mxi_epics_gpib_interface_clear(MX_GPIB *gpib);
MX_API mx_status_type mxi_epics_gpib_device_clear(MX_GPIB *gpib);
MX_API mx_status_type mxi_epics_gpib_selective_device_clear(MX_GPIB *gpib,
						long address);
MX_API mx_status_type mxi_epics_gpib_local_lockout(MX_GPIB *gpib);
MX_API mx_status_type mxi_epics_gpib_remote_enable(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_epics_gpib_go_to_local(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_epics_gpib_trigger(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_epics_gpib_wait_for_service_request( MX_GPIB *gpib,
						double timeout );
MX_API mx_status_type mxi_epics_gpib_serial_poll( MX_GPIB *gpib, long address,
					unsigned char *serial_poll_byte );
MX_API mx_status_type mxi_epics_gpib_serial_poll_disable(MX_GPIB *gpib);

extern MX_RECORD_FUNCTION_LIST mxi_epics_gpib_record_function_list;
extern MX_GPIB_FUNCTION_LIST mxi_epics_gpib_gpib_function_list;

extern long mxi_epics_gpib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_epics_gpib_rfield_def_ptr;

#define MXI_EPICS_GPIB_STANDARD_FIELDS \
  {-1, -1, "epics_record_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_GPIB, epics_record_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_EPICS_GPIB_H__ */


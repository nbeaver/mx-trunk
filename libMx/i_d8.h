/*
 * Name:   i_d8.h
 *
 * Purpose: Header for MX driver for a Bruker D8 goniostat controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_D8_H__
#define __I_D8_H__

#include "mx_record.h"
#include "mx_generic.h"

#include "mx_motor.h"
#include "mx_rs232.h"

/* Define the data structures used by a Bruker D8 interface. */

#define MX_MAX_D8_MOTOR_AXES	16

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	MX_RECORD *motor_array[MX_MAX_D8_MOTOR_AXES];
} MX_D8;

#define MXI_D8_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_D8, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_d8_initialize_type( long type );
MX_API mx_status_type mxi_d8_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_d8_finish_record_initialization( MX_RECORD *record );
MX_API mx_status_type mxi_d8_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_d8_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_d8_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_d8_open( MX_RECORD *record );
MX_API mx_status_type mxi_d8_close( MX_RECORD *record );

MX_API mx_status_type mxi_d8_getchar( MX_GENERIC *generic,
					char *c, int flags );
MX_API mx_status_type mxi_d8_putchar( MX_GENERIC *generic,
					char c, int flags );
MX_API mx_status_type mxi_d8_read( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_d8_write( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_d8_num_input_bytes_available( MX_GENERIC *generic,
				uint32_t *num_input_bytes_available );
MX_API mx_status_type mxi_d8_discard_unread_input(
				MX_GENERIC *generic, int debug_flag );
MX_API mx_status_type mxi_d8_discard_unwritten_output(
				MX_GENERIC *generic, int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_d8_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_d8_generic_function_list;

extern mx_length_type mxi_d8_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_d8_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_d8_command( MX_D8 *d8, char *command,
		char *response, int response_buffer_length, int debug_flag );

#endif /* __I_D8_H__ */

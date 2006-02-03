/*
 * Name:    i_ggcs.h
 *
 * Purpose: Header for MX driver for a Bruker GGCS goniostat controller.
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

#ifndef __I_GGCS_H__
#define __I_GGCS_H__

#include "mx_record.h"
#include "mx_generic.h"

#include "mx_motor.h"
#include "mx_rs232.h"

/* Define the data structures used by a Bruker GGCS interface. */

#define MX_MAX_GGCS_MOTOR_AXES	8

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	MX_RECORD *motor_array[MX_MAX_GGCS_MOTOR_AXES];
} MX_GGCS;

#define MXI_GGCS_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GGCS, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_ggcs_initialize_type( long type );
MX_API mx_status_type mxi_ggcs_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_ggcs_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_ggcs_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_ggcs_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_ggcs_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_ggcs_open( MX_RECORD *record );
MX_API mx_status_type mxi_ggcs_close( MX_RECORD *record );

MX_API mx_status_type mxi_ggcs_getchar( MX_GENERIC *generic,
					char *c, int flags );
MX_API mx_status_type mxi_ggcs_putchar( MX_GENERIC *generic,
					char c, int flags );
MX_API mx_status_type mxi_ggcs_read( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_ggcs_write( MX_GENERIC *generic,
					void *buffer, size_t count );

MX_API mx_status_type mxi_ggcs_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available );

MX_API mx_status_type mxi_ggcs_discard_unread_input(
				MX_GENERIC *generic, int debug_flag );
MX_API mx_status_type mxi_ggcs_discard_unwritten_output(
				MX_GENERIC *generic, int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_ggcs_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_ggcs_generic_function_list;

extern mx_length_type mxi_ggcs_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_ggcs_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_ggcs_command( MX_GGCS *ggcs, char *command,
		char *response, int response_buffer_length, int debug_flag );

#endif /* __I_GGCS_H__ */

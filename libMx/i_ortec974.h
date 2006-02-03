/*
 * Name:   i_ortec974.h
 *
 * Purpose: Header for MX driver for generic interface to
 *          EG&G Ortec 974 counter/timer units.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_ORTEC974_H__
#define __I_ORTEC974_H__

#include "mx_record.h"
#include "mx_generic.h"

#include "mx_rs232.h"
#include "mx_gpib.h"

/* Define the data structures used by an Ortec 974 counter/timer. */

#define MX_NUM_ORTEC974_CHANNELS	4

typedef struct {
	MX_RECORD *record;

	/* The module_interface field is a reference to the RS-232
	 * or GPIB port that the Ortec 974 is plugged into.
	 */

	MX_INTERFACE module_interface;
	MX_RECORD *channel_record[MX_NUM_ORTEC974_CHANNELS];
} MX_ORTEC974;

#define MXI_ORTEC974_STANDARD_FIELDS \
  {-1, -1, "module_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ORTEC974, module_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_ortec974_initialize_type( long type );
MX_API mx_status_type mxi_ortec974_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_ortec974_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_ortec974_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_ortec974_read_parms_from_hardware(MX_RECORD *record);
MX_API mx_status_type mxi_ortec974_write_parms_to_hardware(MX_RECORD *record);
MX_API mx_status_type mxi_ortec974_open( MX_RECORD *record );
MX_API mx_status_type mxi_ortec974_close( MX_RECORD *record );

MX_API mx_status_type mxi_ortec974_getchar( MX_GENERIC *generic,
					char *c, int flags );
MX_API mx_status_type mxi_ortec974_putchar( MX_GENERIC *generic,
					char c, int flags );
MX_API mx_status_type mxi_ortec974_read( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_ortec974_write( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_ortec974_num_input_bytes_available(
				MX_GENERIC *generic,
				unsigned long *num_input_bytes_available );
MX_API mx_status_type mxi_ortec974_discard_unread_input(
					MX_GENERIC *generic, int debug_flag);
MX_API mx_status_type mxi_ortec974_discard_unwritten_output(
					MX_GENERIC *generic, int debug_flag);

extern MX_RECORD_FUNCTION_LIST mxi_ortec974_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_ortec974_generic_function_list;

extern mx_length_type mxi_ortec974_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_ortec974_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_ortec974_command( MX_ORTEC974 *ortec974,
	char *command, char *response, int response_buffer_length,
	char *error_message, int error_message_buffer_length,
	int debug_flag );
MX_API mx_status_type mxi_ortec974_getline( MX_ORTEC974 *ortec974,
			char *buffer, long buffer_size, int debug_flag );
MX_API mx_status_type mxi_ortec974_putline( MX_ORTEC974 *ortec974,
			char *buffer, int debug_flag );

#endif /* __I_ORTEC974_H__ */

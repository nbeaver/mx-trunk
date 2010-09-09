/*
 * Name:    i_scipe.h
 *
 * Purpose: Header for MX driver for connections to SCIPE servers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2005, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SCIPE_H__
#define __I_SCIPE_H__

#include "mx_record.h"
#include "mx_generic.h"

#include "mx_motor.h"
#include "mx_rs232.h"

#define MX_SCIPE_OBJECT_NAME_LENGTH		40
#define MX_SCIPE_ERROR_MESSAGE_LENGTH		80

/* SCIPE response codes */

#define MXF_SCIPE_OK				100
#define MXF_SCIPE_OK_WITH_RESULT		101

#define MXF_SCIPE_NOT_MOVING			200
#define MXF_SCIPE_MOVING			201
#define MXF_SCIPE_ACTUATOR_GENERAL_FAULT	202
#define MXF_SCIPE_UPPER_BOUND_FAULT		203
#define MXF_SCIPE_LOWER_BOUND_FAULT		204

#define MXF_SCIPE_NOT_COLLECTING		300
#define MXF_SCIPE_COLLECTING			301
#define MXF_SCIPE_DETECTOR_GENERAL_FAULT	302

#define MXF_SCIPE_COMMAND_NOT_FOUND		500
#define MXF_SCIPE_OBJECT_NOT_FOUND		501
#define MXF_SCIPE_ERROR_IN_COMMAND		502
#define MXF_SCIPE_ERROR_IN_OBJECT_PROCESSING	503

/* Define the data structures used by a SCIPE server interface. */

typedef struct {
	MX_RECORD *record;
	MX_RECORD *rs232_record;
} MX_SCIPE_SERVER;

#define MXI_SCIPE_SERVER_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_SERVER, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_scipe_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_scipe_open( MX_RECORD *record );
MX_API mx_status_type mxi_scipe_close( MX_RECORD *record );
MX_API mx_status_type mxi_scipe_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_scipe_getchar( MX_GENERIC *generic,
					char *c, int flags );
MX_API mx_status_type mxi_scipe_putchar( MX_GENERIC *generic,
					char c, int flags );
MX_API mx_status_type mxi_scipe_read( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_scipe_write( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_scipe_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available );
MX_API mx_status_type mxi_scipe_discard_unread_input(
				MX_GENERIC *generic, int debug_flag );
MX_API mx_status_type mxi_scipe_discard_unwritten_output(
				MX_GENERIC *generic, int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_scipe_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_scipe_generic_function_list;

extern long mxi_scipe_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_scipe_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_scipe_command(
	MX_SCIPE_SERVER *scipe_server, char *command,
	char *response, int response_buffer_length,
	int *response_code, char **result_ptr,
	int debug_flag );

MX_API char *mxi_scipe_strerror( int response_code );

MX_API long mxi_scipe_get_mx_status( int response_code );

#endif /* __I_SCIPE_H__ */

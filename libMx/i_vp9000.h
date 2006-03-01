/*
 * Name:   i_vp9000.h
 *
 * Purpose: Header for MX driver for Velmex VP9000 series motor controllers.
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

#ifndef __I_VP9000_H__
#define __I_VP9000_H__

#include "mx_record.h"
#include "mx_generic.h"

#include "mx_motor.h"
#include "mx_rs232.h"

/* Bit masks used to select parts of the controller status. */

#define MXF_VP9000_PROGRAM_RUNNING	1

/* Define the data structures used by a Velmex VP9000 interface. */

#define MX_MAX_VP9000_AXES	4

typedef struct {
	MX_RECORD *record;

	long interface_subtype;
	MX_RECORD *rs232_record;
	long num_controllers;
	long *num_motors;
	long active_controller;
	long active_motor;

	MX_RECORD *(*motor_array)[MX_MAX_VP9000_AXES];
} MX_VP9000;

#define MXI_VP9000_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VP9000, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_controllers", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_VP9000, num_controllers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_motors", MXFT_LONG, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VP9000, num_motors), \
	{sizeof(long)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}

#ifndef MX_CR
#define MX_CR	'\015'
#define MX_LF	'\012'
#endif

MX_API mx_status_type mxi_vp9000_initialize_type( long type );
MX_API mx_status_type mxi_vp9000_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_vp9000_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_vp9000_open( MX_RECORD *record );
MX_API mx_status_type mxi_vp9000_close( MX_RECORD *record );
MX_API mx_status_type mxi_vp9000_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_vp9000_getchar( MX_GENERIC *generic,
					char *c, int flags );
MX_API mx_status_type mxi_vp9000_putchar( MX_GENERIC *generic,
					char c, int flags );
MX_API mx_status_type mxi_vp9000_read( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_vp9000_write( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_vp9000_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available );
MX_API mx_status_type mxi_vp9000_discard_unread_input(
				MX_GENERIC *generic, int debug_flag );
MX_API mx_status_type mxi_vp9000_discard_unwritten_output(
				MX_GENERIC *generic, int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_vp9000_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_vp9000_generic_function_list;

extern long mxi_vp9000_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_vp9000_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_vp9000_command(
	MX_VP9000 *vp9000_interface, long controller_number,
	char *command, char *response, int response_buffer_length,
	int debug_flag );
MX_API mx_status_type mxi_vp9000_getline(
			MX_VP9000 *vp9000_interface, long controller_number,
			char *buffer, long buffer_size, int debug_flag );
MX_API mx_status_type mxi_vp9000_putline(
			MX_VP9000 *vp9000_interface, long controller_number,
			char *buffer, int debug_flag );
MX_API mx_status_type mxi_vp9000_getc(
			MX_VP9000 *vp9000_interface, long controller_number,
			char *c, int debug_flag );
MX_API mx_status_type mxi_vp9000_putc(
			MX_VP9000 *vp9000_interface, long controller_number,
			char c, int debug_flag );

#endif /* __I_VP9000_H__ */

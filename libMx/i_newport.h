/*
 * Name:   i_newport.h
 *
 * Purpose: Header for MX driver for Newport MM3000, MM4000/4005, and ESP
 *          motor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NEWPORT_H__
#define __I_NEWPORT_H__

#include "mx_record.h"
#include "mx_generic.h"

#include "mx_motor.h"
#include "mx_rs232.h"

/* Define the data structures used by a Newport motor controller interfaces. */

#define MX_MAX_NEWPORT_MOTORS	4

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE controller_interface;
	MX_RECORD *motor_record[MX_MAX_NEWPORT_MOTORS];
} MX_NEWPORT;

#define MXI_NEWPORT_STANDARD_FIELDS \
  {-1, -1, "controller_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT, controller_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#ifndef MX_CR
#define MX_CR	'\015'
#define MX_LF	'\012'
#endif

MX_API mx_status_type mxi_newport_initialize_type( long type );
MX_API mx_status_type mxi_newport_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_newport_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_newport_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_newport_read_parms_from_hardware( MX_RECORD *record );
MX_API mx_status_type mxi_newport_write_parms_to_hardware( MX_RECORD *record );
MX_API mx_status_type mxi_newport_open( MX_RECORD *record );
MX_API mx_status_type mxi_newport_close( MX_RECORD *record );
MX_API mx_status_type mxi_newport_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_newport_discard_unread_input( MX_GENERIC *generic,
							int debug_flag );
MX_API mx_status_type mxi_newport_discard_unwritten_output( MX_GENERIC *generic,
							int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_newport_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_newport_generic_function_list;

extern long mxi_mm3000_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_mm3000_rfield_def_ptr;

extern long mxi_mm4000_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_mm4000_rfield_def_ptr;

extern long mxi_esp_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_esp_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_newport_turn_motor_drivers_on(
			MX_NEWPORT *newport, int debug_flag );

MX_API mx_status_type mxi_newport_command( MX_NEWPORT *newport, char *command,
	char *response, int response_buffer_length, int debug_flag );

MX_API mx_status_type mxi_newport_getline( MX_NEWPORT *newport,
			char *buffer, long buffer_size, int debug_flag );

MX_API mx_status_type mxi_newport_putline( MX_NEWPORT *newport,
			char *buffer, int debug_flag );

#endif /* __I_NEWPORT_H__ */

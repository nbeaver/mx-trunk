/*
 * Name:    d_e662.h 
 *
 * Purpose: Header file for MX motor driver to control Physik Instrumente
 *          E-662 LVPZT position servo controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_E662_H__
#define __D_E662_H__

/* ===== E-662 motor data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
} MX_E662;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_e662_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_e662_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_e662_open( MX_RECORD *record );
MX_API mx_status_type mxd_e662_close( MX_RECORD *record );
MX_API mx_status_type mxd_e662_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_e662_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_e662_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_e662_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_e662_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_e662_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_e662_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_e662_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_e662_negative_limit_hit( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_e662_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_e662_motor_function_list;

/* Define an extra function for the use of this driver. */

MX_API mx_status_type mxd_e662_command( MX_E662 *e662, char *command,
			char *response, int response_buffer_length,
			int debug_flag );

extern mx_length_type mxd_e662_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_e662_rfield_def_ptr;

#define MXD_E662_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_E662, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_E662_H__ */

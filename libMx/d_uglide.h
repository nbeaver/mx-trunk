/*
 * Name:    d_uglide.h
 *
 * Purpose: Header file for MX drivers for BCW u-GLIDE motors from
 *          Oceaneering Space Systems.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2002, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_UGLIDE_H__
#define __D_UGLIDE_H__

typedef struct {
	MX_RECORD *uglide_record;
	char axis_name;
} MX_UGLIDE_MOTOR;

MX_API mx_status_type mxd_uglide_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_uglide_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_uglide_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_uglide_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_uglide_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_uglide_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_uglide_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_uglide_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_uglide_get_extended_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_uglide_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_uglide_motor_function_list;

extern long mxd_uglide_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_uglide_rfield_def_ptr;

#define MXD_UGLIDE_STANDARD_FIELDS \
  {-1, -1, "uglide_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UGLIDE_MOTOR, uglide_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UGLIDE_MOTOR, axis_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_UGLIDE_H__ */

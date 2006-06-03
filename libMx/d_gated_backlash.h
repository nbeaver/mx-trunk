/*
 * Name:    d_gated_backlash.h 
 *
 * Purpose: Header file for MX pseudomotor driver to arrange for a gate
 *          signal to be sent to a relay or digital output when the real
 *          motor is executing a backlash correction.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GATED_BACKLASH_H__
#define __D_GATED_BACKLASH_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *real_motor_record;
	MX_RECORD *gate_record;
	unsigned long gate_on;
	unsigned long gate_off;

	struct timespec gate_delay;
	struct timespec gate_delay_finish;
} MX_GATED_BACKLASH;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_gated_backlash_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_gated_backlash_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_gated_backlash_open( MX_RECORD *record );
MX_API mx_status_type mxd_gated_backlash_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_gated_backlash_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_gated_backlash_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_gated_backlash_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_gated_backlash_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_gated_backlash_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_gated_backlash_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_gated_backlash_constant_velocity_move(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_gated_backlash_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_gated_backlash_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_gated_backlash_get_status( MX_MOTOR *motor );

MX_API mx_status_type mxd_gated_backlash_get_pointers( MX_MOTOR *motor,
				MX_GATED_BACKLASH **gated_backlash,
				const char *calling_fname );

extern MX_RECORD_FUNCTION_LIST mxd_gated_backlash_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_gated_backlash_motor_function_list;

extern long mxd_gated_backlash_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_gated_backlash_rfield_def_ptr;

#define MXD_GATED_BACKLASH_STANDARD_FIELDS \
  {-1, -1, "real_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GATED_BACKLASH, real_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "gate_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GATED_BACKLASH, gate_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "gate_on", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GATED_BACKLASH, gate_on), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "gate_off", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GATED_BACKLASH, gate_off), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_GATED_BACKLASH_H__ */


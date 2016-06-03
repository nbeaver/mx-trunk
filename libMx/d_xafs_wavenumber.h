/*
 * Name:    d_xafs_wavenumber.h 
 *
 * Purpose: Header file for MX motor driver to control an MX motor device
 *          in terms of the XAFS electron wavenumber.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2007, 2013, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_XAFS_WAVENUMBER_H__
#define __D_XAFS_WAVENUMBER_H__

#include "mx_motor.h"

/* ===== MX XAFS electron wavenumber motor data structures ===== */

typedef struct {
	MX_RECORD *record;	/* This is a pointer to the record this
				 * structure belongs to.
				 */
	MX_RECORD *energy_motor_record;
} MX_XAFS_WAVENUMBER_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_xafswn_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_xafswn_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_xafswn_motor_print_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_xafswn_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_xafswn_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_xafswn_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_xafswn_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_xafswn_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_xafswn_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_xafswn_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_xafswn_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_xafswn_motor_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_xafswn_motor_get_parameter( MX_MOTOR *motor );

MX_API mx_status_type mxd_xafswn_motor_get_edge_energy(
			MX_XAFS_WAVENUMBER_MOTOR *motor, double *edge_energy );

extern MX_RECORD_FUNCTION_LIST mxd_xafswn_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_xafswn_motor_motor_function_list;

extern long mxd_xafswn_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_xafswn_motor_rfield_def_ptr;

#define MXD_XAFSWN_MOTOR_STANDARD_FIELDS \
  {-1, -1, "energy_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XAFS_WAVENUMBER_MOTOR, energy_motor_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_XAFS_WAVENUMBER_H__ */

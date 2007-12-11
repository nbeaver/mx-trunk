/*
 * Name:    d_monochromator.h 
 *
 * Purpose: Header file for MX motor driver to control X-ray monochromators
 *          This is a pseudomotor that pretends to be the monochromator
 *          'theta' axis.  The value written to this pseudo motor is used
 *          to control the real 'theta' axis and any other MX devices that
 *          are tied into this pseudomotor.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2000, 2002-2003, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MONOCHROMATOR_H__
#define __D_MONOCHROMATOR_H__

#include "mx_motor.h"

/* ===== MX monochromator motor data structures ===== */

typedef struct {
	MX_RECORD *record;
	long num_dependencies;
	MX_RECORD **list_array;
	mx_bool_type *speed_change_permitted;
	mx_bool_type *speed_changed;
	mx_bool_type move_in_progress;
	double move_time;
} MX_MONOCHROMATOR;

/* Positions in the list array. */

#define MXFP_MONO_ENABLED		0
#define MXFP_MONO_DEPENDENCY_TYPE	1
#define MXFP_MONO_PARAMETER_ARRAY	2
#define MXFP_MONO_RECORD_ARRAY		3

typedef struct {
	mx_status_type (*get_position) ( MX_MONOCHROMATOR *,
						MX_RECORD *, long, double * );
	mx_status_type (*set_position) ( MX_MONOCHROMATOR *,
						MX_RECORD *, long, double );
	mx_status_type (*move) (MX_MONOCHROMATOR *, MX_RECORD *, long,
						double, double, int);
} MX_MONOCHROMATOR_FUNCTION_LIST;

/* Parameter dependency types.  These must be in consecutive increasing
 * order starting at 0.
 */

#define MXF_MONO_THETA					0
#define MXF_MONO_ENERGY					1
#define MXF_MONO_POLYNOMIAL				2
#define MXF_MONO_INSERTION_DEVICE_ENERGY		3
#define MXF_MONO_CONSTANT_EXIT_BRAGG_NORMAL		4
#define MXF_MONO_CONSTANT_EXIT_BRAGG_PARALLEL		5
#define MXF_MONO_TABLE					6
#define MXF_MONO_DIFFRACTOMETER_THETA			7
#define MXF_MONO_ENERGY_POLYNOMIAL			8
#define MXF_MONO_OPTION_SELECTOR			9
#define MXF_MONO_THETA_FUNCTION				10
#define MXF_MONO_ENERGY_FUNCTION			11

/* Define all of the interface functions. */

MX_API mx_status_type mxd_monochromator_initialize_type( long type );
MX_API mx_status_type mxd_monochromator_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_monochromator_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_monochromator_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_monochromator_open( MX_RECORD *record );

MX_API mx_status_type mxd_monochromator_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_monochromator_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_monochromator_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_monochromator_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_monochromator_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_monochromator_constant_velocity_move(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_monochromator_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_monochromator_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_monochromator_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_monochromator_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_monochromator_motor_function_list;

extern long mxd_monochromator_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_monochromator_rfield_def_ptr;

#define MXD_MONOCHROMATOR_STANDARD_FIELDS \
  {-1, -1, "num_dependencies", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MONOCHROMATOR, num_dependencies), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "list_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_MONOCHROMATOR, list_array), \
	{sizeof(MX_RECORD *)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}, \
  \
  {-1, -1, "speed_change_permitted", MXFT_BOOL, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_MONOCHROMATOR, speed_change_permitted),\
	{sizeof(int)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "speed_changed", MXFT_BOOL, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MONOCHROMATOR, speed_changed),\
	{sizeof(int)}, NULL, MXFF_VARARGS}

#endif /* __D_MONOCHROMATOR_H__ */

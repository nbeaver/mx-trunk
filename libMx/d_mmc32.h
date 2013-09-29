/*
 * Name:    d_mmc32.h 
 *
 * Purpose: Header file for MX motor driver to control NSLS-built MMC32
 *          motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MMC32_H__
#define __D_MMC32_H__

#include "mx_motor.h"

/* ===== NSLS MMC32 motor data structure ===== */

typedef struct {
	MX_INTERFACE gpib_interface;
	long motor_number;	/* From 0 to 32 */
	double multiplication_factor;
	long start_velocity;
	long peak_velocity;
	long acceleration_steps;
} MX_MMC32;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mmc32_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_mmc32_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_mmc32_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_mmc32_open( MX_RECORD *record );
MX_API mx_status_type mxd_mmc32_close( MX_RECORD *record );

MX_API mx_status_type mxd_mmc32_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_mmc32_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_mmc32_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_mmc32_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_mmc32_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_mmc32_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_mmc32_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_mmc32_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_mmc32_motor_function_list;


extern long mxd_mmc32_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mmc32_rfield_def_ptr;

#define MXD_MMC32_STANDARD_FIELDS \
  {-1, -1, "gpib_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MMC32, gpib_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "motor_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MMC32, motor_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "multiplication_factor", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MMC32, multiplication_factor), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "start_velocity", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MMC32, start_velocity), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "peak_velocity", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MMC32, peak_velocity), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "acceleration_steps", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MMC32, acceleration_steps), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_MMC32_H__ */

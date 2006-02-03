/*
 * Name:    d_aps_18id.h 
 *
 * Purpose: Header file for MX motor driver to control the BioCAT Sector 18-ID
 *          monochromator at the Advanced Photon Source.  This is a pseudomotor
 *          that pretends to be the monochromator 'theta' axis.  The value
 *          written to this pseudo motor is used to control the real 'theta'
 *          axis and any other MX devices that are tied into this pseudomotor.
 *
 *          This is a modification of Bill's d_aps_18id.h file to suit our
 *          purpose of controlling our monochromator with individual motors.
 *          We want to move energy by adjusting Bragg angle with the option of
 *          moving trolley X and/or Y instead of moving them as a single
 *          cooordianted system like the old way.
 *                                       - Shengke Wang 1/1999
 *
 * Authors: William Lavender and Shengke Wang
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_APS_18ID_H__
#define __D_APS_18ID_H__

#include "mx_motor.h"

/* ===== MX Sector 18-ID motor data structures ===== */

typedef struct {
	MX_RECORD *bragg_motor_record;
	MX_RECORD *trolleyY_motor_record;
	MX_RECORD *trolleyY_linked_record;
	MX_RECORD *trolleyX_motor_record;
	MX_RECORD *trolleyX_linked_record;
	MX_RECORD *trolley_setup_record;
	MX_RECORD *gap_energy_motor_record;
	MX_RECORD *gap_linked_record;
	MX_RECORD *gap_offset_record;
	MX_RECORD *gap_harmonic_record;
	MX_RECORD *tune_motor_record;
	MX_RECORD *tune_linked_record;
	MX_RECORD *tune_parameters_record;
	MX_RECORD *piezo_left_motor_record;
	MX_RECORD *piezo_right_motor_record;
	MX_RECORD *piezo_enable_record;
	MX_RECORD *piezo_parameters_record;
	MX_RECORD *timer_record;
	MX_RECORD *bpm_top_scaler_record;
	MX_RECORD *bpm_bottom_scaler_record;
	MX_RECORD *d_spacing_record;
	double angle_scale;  /* To translate betw. radians and native units. */
} MX_APS_18ID_MOTOR;

#define MXF_APS_18ID_NUM_REAL_MOTOR_RECORDS 5

/* tune function types. */

#define MXF_APS_18ID_NO_DEPENDENCE		0
#define MXF_APS_18ID_CUBIC_POLYNOMIAL_FIT	1

/* Define all of the interface functions. */

MX_API mx_status_type mxd_aps_18id_motor_initialize_type( long type );
MX_API mx_status_type mxd_aps_18id_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_aps_18id_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_aps_18id_motor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_aps_18id_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_aps_18id_motor_read_parms_from_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_aps_18id_motor_write_parms_to_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_aps_18id_motor_open( MX_RECORD *record );
MX_API mx_status_type mxd_aps_18id_motor_close( MX_RECORD *record );
MX_API mx_status_type mxd_aps_18id_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_constant_velocity_move(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_18id_motor_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_aps_18id_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_aps_18id_motor_motor_function_list;

extern mx_length_type mxd_aps_18id_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aps_18id_motor_rfield_def_ptr;

#define MXD_APS_18ID_MOTOR_STANDARD_FIELDS \
  {-1, -1, "bragg_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, bragg_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "trolleyY_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, trolleyY_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "trolleyY_linked_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, trolleyY_linked_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "trolleyX_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, trolleyX_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "trolleyX_linked_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, trolleyX_linked_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "trolley_setup_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, trolley_setup_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "gap_energy_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, gap_energy_motor_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "gap_linked_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_18ID_MOTOR, gap_linked_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "gap_offset_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_18ID_MOTOR, gap_offset_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "gap_harmonic_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_18ID_MOTOR, gap_harmonic_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "tune_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_18ID_MOTOR, tune_motor_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "tune_function_type_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, tune_linked_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "tune_parameters_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, tune_parameters_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "piezo_left_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, piezo_left_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "piezo_right_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, piezo_right_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "piezo_enable_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, piezo_enable_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "piezo_parameters_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, piezo_parameters_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "timer_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, timer_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bpm_top_scaler_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, bpm_top_scaler_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bpm_bottom_scaler_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_APS_18ID_MOTOR, bpm_bottom_scaler_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "d_spacing_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_18ID_MOTOR, d_spacing_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
 \
  {-1, -1, "angle_scale", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_18ID_MOTOR, angle_scale), \
	{0}, NULL, MXFF_IN_DESCRIPTION }
  
#endif /* __D_APS_18ID_H__ */

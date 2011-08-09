/*
 * Name:    d_pmactc.h 
 *
 * Purpose: Header file for MX motor driver for controlling a Delta Tau
 *          PMAC motor through Tom Coleman's EPICS interface.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PMACTC_H__
#define __D_PMACTC_H__

#include "mx_motor.h"
#include "mx_clock.h"

/* Define values for the 'motion_state' field below. */

#define MXF_PTC_NO_MOVE_IN_PROGRESS	1
#define MXF_PTC_START_DELAY_IN_PROGRESS	2
#define MXF_PTC_MOVE_IN_PROGRESS	3
#define MXF_PTC_END_DELAY_IN_PROGRESS	4

typedef struct {
	char actual_position_record_name[MXU_EPICS_PVNAME_LENGTH+1];
	char requested_position_record_name[MXU_EPICS_PVNAME_LENGTH+1];
	char in_position_record_name[MXU_EPICS_PVNAME_LENGTH+1];
	char abort_record_name[MXU_EPICS_PVNAME_LENGTH+1];
	char speed_record_name_prefix[MXU_EPICS_PVNAME_LENGTH+1];
	char pmac_name[MXU_EPICS_PVNAME_LENGTH+1];
	long motor_number;
	double speed_scale;
	double start_delay;
	double end_delay;

	long motion_state;

	MX_CLOCK_TICK start_delay_ticks;
	MX_CLOCK_TICK end_delay_ticks;

	MX_CLOCK_TICK end_of_start_delay;
	MX_CLOCK_TICK end_of_end_delay;

	MX_EPICS_PV actual_position_pv;
	MX_EPICS_PV requested_position_pv;
	MX_EPICS_PV in_position_pv;
	MX_EPICS_PV abort_pv;
	MX_EPICS_PV strcmd_pv;
	MX_EPICS_PV strrsp_pv;

	MX_EPICS_PV i16ai_pv;
	MX_EPICS_PV i16ao_pv;
	MX_EPICS_PV i20fan_pv;
	MX_EPICS_PV i20li_pv;
} MX_PMAC_TC_MOTOR;

/* Speeds and accelerations in the PMAC are expressed using milliseconds. */

#define MX_PMAC_TC_MOTOR_SECONDS_PER_MILLISECOND	0.001

/* Define all of the interface functions. */

MX_API mx_status_type mxd_pmac_tc_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_pmac_tc_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_pmac_tc_motor_print_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_pmac_tc_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_tc_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_tc_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_tc_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_tc_motor_constant_velocity_move(MX_MOTOR *motor);

MX_API mx_status_type mxd_pmac_tc_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_tc_motor_set_parameter( MX_MOTOR *motor );

MX_API mx_status_type mxd_pmac_bio_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_bio_motor_set_parameter( MX_MOTOR *motor );

/* SBC-CAT version */

extern MX_RECORD_FUNCTION_LIST mxd_pmac_tc_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_pmac_tc_motor_motor_function_list;

extern long mxd_pmac_tc_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmac_tc_motor_rfield_def_ptr;

/* BioCAT version */

extern MX_RECORD_FUNCTION_LIST mxd_pmac_bio_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_pmac_bio_motor_motor_function_list;

extern long mxd_pmac_bio_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmac_bio_motor_rfield_def_ptr;

#define MXD_PMAC_TC_MOTOR_BASE_FIELDS \
  {-1, -1, "actual_position_record_name", MXFT_STRING, NULL, \
					1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMAC_TC_MOTOR, actual_position_record_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "requested_position_record_name", MXFT_STRING, NULL, \
					1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMAC_TC_MOTOR, requested_position_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "in_position_record_name", MXFT_STRING, NULL, \
					1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMAC_TC_MOTOR, in_position_record_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "abort_record_name", MXFT_STRING, NULL, \
					1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_TC_MOTOR, abort_record_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}

#define MXD_PMAC_TC_MOTOR_STANDARD_FIELDS \
  MXD_PMAC_TC_MOTOR_BASE_FIELDS, \
  \
  {-1, -1, "pmac_name", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_TC_MOTOR, pmac_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "motor_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_TC_MOTOR, motor_number), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "speed_scale", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_TC_MOTOR, speed_scale), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "start_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_TC_MOTOR, start_delay), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "end_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_TC_MOTOR, end_delay), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#define MXD_PMAC_BIO_MOTOR_STANDARD_FIELDS \
  MXD_PMAC_TC_MOTOR_BASE_FIELDS, \
  \
  {-1, -1, "speed_record_name_prefix", MXFT_STRING, NULL, \
					1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMAC_TC_MOTOR, speed_record_name_prefix), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "speed_scale", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_TC_MOTOR, speed_scale), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "start_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_TC_MOTOR, start_delay), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "end_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_TC_MOTOR, end_delay), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_PMACTC_H__ */

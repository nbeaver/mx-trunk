/*
 * Name:    d_aps_gap.h 
 *
 * Purpose: Header file for MX motor driver for controlling an
 *          Advanced Photon Source undulator gap.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_APS_GAP_H__
#define __D_APS_GAP_H__

/* ===== MX APS undulator gap record data structures ===== */

typedef struct {
	int32_t sector_number;
	int32_t motor_subtype;

	MX_EPICS_PV position_pv;
	MX_EPICS_PV destination_pv;
	MX_EPICS_PV busy_pv;
	MX_EPICS_PV access_security_pv;
	MX_EPICS_PV start_pv;
	MX_EPICS_PV stop_pv;
} MX_APS_GAP;

/* The subtypes of the aps_gap motor. */

#define MXT_APS_GAP_MM		1
#define MXT_APS_GAP_KEV		2
#define MXT_APS_TAPER_MM	3
#define MXT_APS_TAPER_KEV	4

/* Define all of the interface functions. */

MX_API mx_status_type mxd_aps_gap_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_aps_gap_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_aps_gap_print_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_aps_gap_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_gap_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_gap_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_aps_gap_get_status( MX_MOTOR *motor );

MX_API mx_status_type mxd_aps_gap_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_aps_gap_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_aps_gap_motor_function_list;

extern mx_length_type mxd_aps_gap_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aps_gap_record_field_def_ptr;

#define MXD_APS_GAP_STANDARD_FIELDS \
  {-1, -1, "sector_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_GAP, sector_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "motor_subtype", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_APS_GAP, motor_subtype), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_APS_GAP_H__ */

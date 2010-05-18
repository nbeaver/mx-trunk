/*
 * Name:    d_linkam_t9x_temp.h
 *
 * Purpose: Header file for the temperature control part of
 *          Linkam T9x series cooling system controllers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_LINKAM_T9X_TEMP_H__
#define __D_LINKAM_T9X_TEMP_H__

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *record;
	MX_RECORD *linkam_t9x_record;
	double initial_speed;		/* In degrees C per second. */
} MX_LINKAM_T9X_TEMPERATURE;

MX_API mx_status_type mxd_linkam_t9x_temp_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_linkam_t9x_temp_open( MX_RECORD *record );

MX_API mx_status_type mxd_linkam_t9x_temp_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_linkam_t9x_temp_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_linkam_t9x_temp_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_linkam_t9x_temp_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_linkam_t9x_temp_get_extended_status( MX_MOTOR *motor);

extern MX_RECORD_FUNCTION_LIST mxd_linkam_t9x_temp_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_linkam_t9x_temp_motor_function_list;

extern long mxd_linkam_t9x_temp_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_linkam_t9x_temp_rfield_def_ptr;

#define MXD_LINKAM_T9X_TEMP_STANDARD_FIELDS \
  {-1, -1, "linkam_t9x_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_LINKAM_T9X_TEMPERATURE, linkam_t9x_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "initial_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_LINKAM_T9X_TEMPERATURE, initial_speed), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY) }

/* === Driver specific functions === */

#endif /* __D_LINKAM_T9X_TEMP_H__ */


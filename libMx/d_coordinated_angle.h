/*
 * Name:    d_coordinated_angle.h
 *
 * Purpose: Header filed for an MX driver that coordinates the moves of
 *          several motors to maintain them all at the same relative angle.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2014, 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_COORDINATED_ANGLE_H__
#define __D_COORDINATED_ANGLE_H__

#define MXU_COORDINATED_ANGLE_NAME_LENGTH	40

/* Values for 'angle_type'. */

#define MXT_COORDINATED_ANGLE_ARC		1
#define MXT_COORDINATED_ANGLE_TANGENT_ARM	2
#define MXT_COORDINATED_ANGLE_SINE_ARM		3

/* Values for 'angle_flags'. */

#define MXF_COORDINATED_ANGLE_REPORT_MEAN_AS_POSITION 0x1

typedef struct {
	MX_RECORD *record;
	char angle_type_name[MXU_COORDINATED_ANGLE_NAME_LENGTH+1];
	unsigned long angle_type;
	unsigned long angle_flags;

	long num_motors;
	MX_RECORD **motor_record_array;
	double *longitudinal_distance_array;
	double *motor_scale_array;
	double *motor_offset_array;

	double *raw_angle_array;
	double mean;
	double standard_deviation;
	double *motor_angle_array;
	double *motor_error_array;
} MX_COORDINATED_ANGLE;

MX_API mx_status_type mxd_coordinated_angle_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_coordinated_angle_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_coordinated_angle_open( MX_RECORD *record );
MX_API mx_status_type mxd_coordinated_angle_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_coordinated_angle_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_coordinated_angle_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_coordinated_angle_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_coordinated_angle_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_coordinated_angle_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_coordinated_angle_motor_function_list;

extern long mxd_coordinated_angle_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_coordinated_angle_rfield_def_ptr;

#define MXLV_COORDINATED_ANGLE_RAW_ANGLE_ARRAY		87701
#define MXLV_COORDINATED_ANGLE_MEAN			87702
#define MXLV_COORDINATED_ANGLE_STANDARD_DEVIATION	87703
#define MXLV_COORDINATED_ANGLE_MOTOR_ANGLE_ARRAY	87704
#define MXLV_COORDINATED_ANGLE_MOTOR_ERROR_ARRAY	87705

#define MXD_COORDINATED_ANGLE_STANDARD_FIELDS \
  {-1, -1, "angle_type_name", MXFT_STRING, \
			NULL, 1, {MXU_COORDINATED_ANGLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COORDINATED_ANGLE, angle_type_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "angle_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COORDINATED_ANGLE, angle_flags),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "num_motors", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COORDINATED_ANGLE, num_motors),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "motor_record_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_COORDINATED_ANGLE, motor_record_array ), \
	{sizeof(MX_RECORD *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_READ_ONLY | MXFF_VARARGS) }, \
  \
  {-1, -1, "longitudinal_distance_array", MXFT_DOUBLE, \
					NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_COORDINATED_ANGLE, longitudinal_distance_array ), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS) }, \
  \
  {-1, -1, "motor_scale_array", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_COORDINATED_ANGLE, motor_scale_array ), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS) }, \
  \
  {-1, -1, "motor_offset_array", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_COORDINATED_ANGLE, motor_offset_array ), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS) }, \
  \
  {MXLV_COORDINATED_ANGLE_RAW_ANGLE_ARRAY, -1, "raw_angle_array", \
				MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_COORDINATED_ANGLE, raw_angle_array ), \
	{sizeof(double)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS) }, \
  \
  {MXLV_COORDINATED_ANGLE_MEAN, -1, "mean", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_COORDINATED_ANGLE, mean ), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_COORDINATED_ANGLE_STANDARD_DEVIATION, -1, "standard_deviation", \
						MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_COORDINATED_ANGLE, standard_deviation ), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_COORDINATED_ANGLE_MOTOR_ANGLE_ARRAY, -1, "motor_angle_array", \
				MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_COORDINATED_ANGLE, motor_angle_array ), \
	{sizeof(double)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS) }, \
  \
  {MXLV_COORDINATED_ANGLE_MOTOR_ERROR_ARRAY, -1, "motor_error_array", \
				MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_COORDINATED_ANGLE, motor_error_array ), \
	{sizeof(double)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS) }


#endif /* __D_COORDINATED_ANGLE_H__ */

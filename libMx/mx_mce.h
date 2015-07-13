/*
 * Name:    mx_mce.h
 *
 * Purpose: MX system header file for multichannel encoders.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006-2007, 2010, 2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MCE_H__
#define __MX_MCE_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_RECORD *record;
	long encoder_type;
	mx_bool_type overflow_set;
	mx_bool_type underflow_set;
	mx_bool_type motor_can_use_this_mce;

	long maximum_num_values;

	long current_num_values;
	long last_measurement_number;
	long measurement_index;

	unsigned long status;
	mx_bool_type start;
	mx_bool_type stop;
	mx_bool_type clear;

	double value;
	double *value_array;

	double scale;
	double offset;

	long num_motors;
	MX_RECORD **motor_record_array;

	char selected_motor_name[ MXU_RECORD_NAME_LENGTH + 1 ];

	long parameter_type;

	mx_bool_type window_is_available;
	mx_bool_type use_window;

	double window[2];
} MX_MCE;

#define MXLV_MCE_CURRENT_NUM_VALUES		1001
#define MXLV_MCE_LAST_MEASUREMENT_NUMBER	1002
#define MXLV_MCE_STATUS				1003
#define MXLV_MCE_START				1004
#define MXLV_MCE_STOP				1005
#define MXLV_MCE_CLEAR				1006
#define MXLV_MCE_MEASUREMENT_INDEX		1007
#define MXLV_MCE_VALUE				1008
#define MXLV_MCE_VALUE_ARRAY			1009
#define MXLV_MCE_NUM_MOTORS			1010
#define MXLV_MCE_MOTOR_RECORD_ARRAY		1011
#define MXLV_MCE_SELECTED_MOTOR_NAME		1012
#define MXLV_MCE_WINDOW_IS_AVAILABLE		1013
#define MXLV_MCE_USE_WINDOW			1014
#define MXLV_MCE_WINDOW				1015

#define MX_MCE_STANDARD_FIELDS \
  {-1, -1, "encoder_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, encoder_type), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "overflow_set", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, overflow_set), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "underflow_set", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, underflow_set), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "maximum_num_values", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, maximum_num_values), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_MCE_CURRENT_NUM_VALUES, -1, "current_num_values", \
				MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, current_num_values), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_LAST_MEASUREMENT_NUMBER, -1, "last_measurement_number", \
				MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, last_measurement_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, status), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_START, -1, "start", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, start), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, stop), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_CLEAR, -1, "clear", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, clear), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_MEASUREMENT_INDEX, -1, "measurement_index", \
				MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, measurement_index), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_VALUE, -1, "value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, value), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_VALUE_ARRAY, -1, "value_array", MXFT_DOUBLE, \
		NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, value_array), \
	{sizeof(double)}, NULL, MXFF_VARARGS }, \
  \
  {-1, -1, "scale", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, scale), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, offset), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_MCE_NUM_MOTORS, -1, "num_motors", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, num_motors), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_MOTOR_RECORD_ARRAY, -1, "motor_record_array", \
	  			MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, motor_record_array), \
	{sizeof(MX_RECORD *)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_MCE_SELECTED_MOTOR_NAME, -1, "selected_motor_name", \
  			MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, selected_motor_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_MCE_WINDOW_IS_AVAILABLE, -1, "window_is_available", \
					MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, window_is_available), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_USE_WINDOW, -1, "use_window", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, use_window), \
	{0}, NULL, 0}, \
  \
  {MXLV_MCE_WINDOW, -1, "window", MXFT_DOUBLE, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MCE, window), \
	{sizeof(double)}, NULL, 0}


/* Encoder types */

#define MXT_MCE_UNKNOWN_ENCODER_TYPE	0

#define MXT_MCE_ABSOLUTE_ENCODER	1
#define MXT_MCE_INCREMENTAL_ENCODER	2
#define MXT_MCE_DELTA_ENCODER		3

/*
 * The structure type MX_MCE_FUNCTION_LIST contains a list of pointers
 * to functions that vary from encoder type to encoder type.
 */

typedef struct {
	mx_status_type ( *get_overflow_status ) ( MX_MCE *encoder );
	mx_status_type ( *reset_overflow_status ) ( MX_MCE *encoder );
	mx_status_type ( *read ) ( MX_MCE *encoder );
	mx_status_type ( *get_current_num_values ) ( MX_MCE *encoder );
	mx_status_type ( *get_last_measurement_number ) ( MX_MCE *encoder );
	mx_status_type ( *get_status ) ( MX_MCE *encoder );
	mx_status_type ( *start ) ( MX_MCE *encoder );
	mx_status_type ( *stop ) ( MX_MCE *encoder );
	mx_status_type ( *clear ) ( MX_MCE *encoder );
	mx_status_type ( *read_measurement ) ( MX_MCE *encoder );
	mx_status_type ( *get_motor_record_array ) ( MX_MCE *encoder );
	mx_status_type ( *connect_mce_to_motor ) ( MX_MCE *encoder,
						MX_RECORD *motor_record );
	mx_status_type ( *get_parameter ) ( MX_MCE *encoder );
	mx_status_type ( *set_parameter ) ( MX_MCE *encoder );
} MX_MCE_FUNCTION_LIST;

MX_API mx_status_type mx_mce_get_pointers( MX_RECORD *record,
					MX_MCE **encoder,
					MX_MCE_FUNCTION_LIST **fl_ptr,
					const char *calling_fname );

MX_API mx_status_type mx_mce_initialize_driver( MX_DRIVER *driver,
				long *maximum_num_values_varargs_cookie );

MX_API mx_status_type mx_mce_fixup_motor_record_array_field( MX_MCE *mce );

MX_API mx_status_type mx_mce_get_overflow_status( MX_RECORD *mce_record,
					mx_bool_type *underflow_set,
					mx_bool_type *overflow_set );

MX_API mx_status_type mx_mce_reset_overflow_status( MX_RECORD *mce_record );

MX_API mx_status_type mx_mce_read( MX_RECORD *mce_record,
			unsigned long *num_values, double **value_array );

MX_API mx_status_type mx_mce_get_current_num_values( MX_RECORD *mce_record,
						unsigned long *num_values );

MX_API mx_status_type mx_mce_get_last_measurement_number(
						MX_RECORD *mce_record,
						long *last_measurement_number );

MX_API mx_status_type mx_mce_get_status( MX_RECORD *mce_record,
					unsigned long *mce_status );

MX_API mx_status_type mx_mce_start( MX_RECORD *mce_record );

MX_API mx_status_type mx_mce_stop( MX_RECORD *mce_record );

MX_API mx_status_type mx_mce_clear( MX_RECORD *mce_record );

MX_API mx_status_type mx_mce_read_measurement( MX_RECORD *mce_record,
						long measurement_index,
						double *mce_value );

MX_API mx_status_type mx_mce_get_motor_record_array( MX_RECORD *mce_record,
			long *num_motors, MX_RECORD ***motor_record_array );

MX_API mx_status_type mx_mce_connect_mce_to_motor( MX_RECORD *mce_record,
						MX_RECORD *motor_record );

MX_API mx_status_type mx_mce_get_window( MX_RECORD *mce_record,
							double *window );

MX_API mx_status_type mx_mce_set_window( MX_RECORD *mce_record,
							double *window );

MX_API mx_status_type mx_mce_get_window_is_available( MX_RECORD *mce_record,
					mx_bool_type *window_is_available );

MX_API mx_status_type mx_mce_set_window_is_available( MX_RECORD *mce_record,
					mx_bool_type window_is_available );

MX_API mx_status_type mx_mce_get_use_window( MX_RECORD *mce_record,
					mx_bool_type *use_window );

MX_API mx_status_type mx_mce_set_use_window( MX_RECORD *mce_record,
					mx_bool_type use_window );

MX_API mx_status_type mx_mce_default_get_parameter_handler( MX_MCE *mce );

MX_API mx_status_type mx_mce_default_set_parameter_handler( MX_MCE *mce );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MCE_H__ */


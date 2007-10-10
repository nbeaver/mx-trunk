/*
 * Name:    mx_autoscale.h
 *
 * Purpose: Header file for MX autoscale device class.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_AUTOSCALE_H__
#define __MX_AUTOSCALE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXF_AUTO_HIGH_LIMIT_TRIPPED	1
#define MXF_AUTO_NO_LIMIT_TRIPPED	0
#define MXF_AUTO_LOW_LIMIT_TRIPPED	(-1)

#define MXF_AUTO_INCREASE_INTENSITY	1
#define MXF_AUTO_NO_CHANGE		0
#define MXF_AUTO_DECREASE_INTENSITY	(-1)

typedef struct {
	MX_RECORD *record;

	MX_RECORD *monitor_record;
	MX_RECORD *control_record;
	MX_RECORD *timer_record;
	mx_bool_type enabled;

	double low_limit;
	double high_limit;
	double low_deadband;
	double high_deadband;

	long monitor_value;
	long get_change_request;
	long change_control;

	long parameter_type;

	long last_limit_tripped;

	unsigned long monitor_offset_index;

	unsigned long num_monitor_offsets;
	double *monitor_offset_array;

} MX_AUTOSCALE;

#define MXLV_AUT_ENABLED		35001
#define MXLV_AUT_LOW_LIMIT		35002
#define MXLV_AUT_HIGH_LIMIT		35003
#define MXLV_AUT_LOW_DEADBAND		35004
#define MXLV_AUT_HIGH_DEADBAND		35005
#define MXLV_AUT_MONITOR_VALUE		35006
#define MXLV_AUT_GET_CHANGE_REQUEST	35007
#define MXLV_AUT_CHANGE_CONTROL		35008
#define MXLV_AUT_MONITOR_OFFSET_INDEX	35009
#define MXLV_AUT_NUM_MONITOR_OFFSETS	35010
#define MXLV_AUT_MONITOR_OFFSET_ARRAY	35011

#define MX_AUTOSCALE_STANDARD_FIELDS \
  {-1, -1, "monitor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, monitor_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "control_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, control_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "timer_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, timer_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_AUT_ENABLED, -1, "enabled", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, enabled), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_AUT_LOW_LIMIT, -1, "low_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, low_limit), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_AUT_HIGH_LIMIT, -1, "high_limit", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, high_limit), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_AUT_LOW_DEADBAND, -1, "low_deadband", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, low_deadband), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_AUT_HIGH_DEADBAND, -1, "high_deadband", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, high_deadband), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_AUT_MONITOR_VALUE, -1, "monitor_value", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, monitor_value), \
	{0}, NULL, 0}, \
  \
  {MXLV_AUT_GET_CHANGE_REQUEST, -1, "get_change_request", \
						MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, get_change_request), \
	{0}, NULL, 0}, \
  \
  {MXLV_AUT_CHANGE_CONTROL, -1, "change_control", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, change_control), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "parameter_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, parameter_type), \
	{0}, NULL, 0}, \
  \
  {MXLV_AUT_MONITOR_OFFSET_INDEX, -1, "monitor_offset_index", \
					MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, monitor_offset_index), \
	{0}, NULL, 0}, \
  \
  {MXLV_AUT_NUM_MONITOR_OFFSETS, -1, "num_monitor_offsets", \
					MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, num_monitor_offsets), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_AUT_MONITOR_OFFSET_ARRAY, -1, "monitor_offset_array", \
					MXFT_DOUBLE, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AUTOSCALE, monitor_offset_array), \
	{sizeof(double)}, NULL, MXFF_VARARGS}

typedef struct {
	mx_status_type ( *read_monitor ) ( MX_AUTOSCALE * );
	mx_status_type ( *get_change_request ) ( MX_AUTOSCALE * );
	mx_status_type ( *change_control ) ( MX_AUTOSCALE * );
	mx_status_type ( *get_offset_index ) ( MX_AUTOSCALE * );
	mx_status_type ( *set_offset_index ) ( MX_AUTOSCALE * );
	mx_status_type ( *get_parameter ) ( MX_AUTOSCALE * );
	mx_status_type ( *set_parameter ) ( MX_AUTOSCALE * );
} MX_AUTOSCALE_FUNCTION_LIST;

MX_API mx_status_type mx_autoscale_read_monitor( MX_RECORD *record,
							long *monitor_value );

MX_API mx_status_type mx_autoscale_get_change_request( MX_RECORD *record,
							long *change_request );

MX_API mx_status_type mx_autoscale_change_control( MX_RECORD *record,
							long change_control );

MX_API mx_status_type mx_autoscale_get_offset_index( MX_RECORD *record,
					unsigned long *monitor_offset_index );

MX_API mx_status_type mx_autoscale_set_offset_index( MX_RECORD *record,
					unsigned long monitor_offset_index );

MX_API mx_status_type mx_autoscale_get_offset_array( MX_RECORD *record,
					unsigned long *num_monitor_offsets,
					double **monitor_offset_array );
					
MX_API mx_status_type mx_autoscale_set_offset_array( MX_RECORD *record,
					unsigned long num_monitor_offsets,
					double *monitor_offset_array );

MX_API mx_status_type mx_autoscale_get_limits( MX_RECORD *record,
				double *low_limit, double *high_limit,
				double *low_deadband, double *high_deadband );

MX_API mx_status_type mx_autoscale_set_limits( MX_RECORD *record,
				double low_limit, double high_limit,
				double low_deadband, double high_deadband );

MX_API mx_status_type mx_autoscale_compute_dynamic_limits(
						MX_RECORD *record,
						double *dynamic_low_limit,
						double *dynamic_high_limit );

MX_API mx_status_type mx_autoscale_default_get_parameter_handler(
						MX_AUTOSCALE *autoscale );
					
MX_API mx_status_type mx_autoscale_default_set_parameter_handler(
						MX_AUTOSCALE *autoscale );

MX_API mx_status_type mx_autoscale_create_monitor_offset_array(
						MX_AUTOSCALE *autoscale );

#ifdef __cplusplus
}
#endif

#endif /* __MX_AUTOSCALE_H__ */


/*
 * Name:   o_toast.h
 *
 * Purpose: MX operation header for a driver that oscillates a motor back
 *          and forth until stopped.  For some kinds of experiments, this
 *          can be thought of as "toasting" the sample in the X-ray beam.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __O_TOAST_H__
#define __O_TOAST_H__

/* Values for 'toast_flags'. */

#define MXSF_TOAST_USE_FINISH_POSITION	0x1

/* Values for 'toast_state'. */

#define MXST_TOAST_ILLEGAL	0

#define MXST_TOAST_IDLE		1
#define MXST_TOAST_FAULT	2
#define MXST_TOAST_TIMED_OUT	3

#define MXST_TOAST_MOVING_TO_HIGH	10
#define MXST_TOAST_MOVING_TO_LOW	11
#define MXST_TOAST_IN_TURNAROUND	12
#define MXST_TOAST_STOPPING		13
#define MXST_TOAST_MOVING_TO_FINISH	14

typedef struct {
	MX_RECORD *record;

	MX_RECORD *motor_record;
	double low_position;
	double high_position;
	double finish_position;
	double turnaround_delay;	/* In seconds. */
	double timeout;			/* In seconds. */
	unsigned long toast_flags;

	MX_CLOCK_TICK interval_finish_tick;

	unsigned long toast_state;
	unsigned long next_toast_state;

	MX_CALLBACK_MESSAGE *callback_message;
} MX_TOAST;

#define MXO_TOAST_STANDARD_FIELDS \
  {-1, -1, "motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TOAST, motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "low_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TOAST, low_position), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "high_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TOAST, high_position), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "finish_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TOAST, finish_position), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "turnaround_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TOAST, turnaround_delay), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "timeout", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TOAST, timeout), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "toast_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TOAST, toast_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "toast_state", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TOAST, toast_state), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "next_toast_state", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TOAST, next_toast_state), \
	{0}, NULL, 0}

MX_API_PRIVATE mx_status_type mxo_toast_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxo_toast_get_status( MX_OPERATION *op );
MX_API_PRIVATE mx_status_type mxo_toast_start( MX_OPERATION *op );
MX_API_PRIVATE mx_status_type mxo_toast_stop( MX_OPERATION *op );

extern MX_RECORD_FUNCTION_LIST mxo_toast_record_function_list;
extern MX_OPERATION_FUNCTION_LIST mxo_toast_operation_function_list;

extern long mxo_toast_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxo_toast_rfield_def_ptr;

#endif /* __O_TOAST_H__ */


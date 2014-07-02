/*
 * Name:   o_biocat_6k_toast.h
 *
 * Purpose: MX operation header for a driver that oscillates a Compumotor
 *          6K-controlled motor back and forth until stopped.
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

#ifndef __O_BIOCAT_6K_TOAST_H__
#define __O_BIOCAT_6K_TOAST_H__

#define MXU_6K_PROGRAM_NAME_LENGTH	40

/* Values for 'toast_flags'. */

#define MXSF_TOAST_USE_FINISH_POSITION	0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *motor_record;
	double low_position;
	double high_position;
	double finish_position;
	double turnaround_delay;	/* In seconds. */
	double timeout;			/* In seconds. */
	unsigned long toast_flags;
	char program_name[MXU_6K_PROGRAM_NAME_LENGTH+1];
	unsigned long task_number;

	char task_prefix[4];
	mx_bool_type move_to_finish_in_progress;
} MX_BIOCAT_6K_TOAST;

#define MXO_BIOCAT_6K_TOAST_STANDARD_FIELDS \
  {-1, -1, "motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIOCAT_6K_TOAST, motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "low_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIOCAT_6K_TOAST, low_position), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "high_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIOCAT_6K_TOAST, high_position), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "finish_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIOCAT_6K_TOAST, finish_position), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "turnaround_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIOCAT_6K_TOAST, turnaround_delay), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "timeout", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIOCAT_6K_TOAST, timeout), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "toast_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIOCAT_6K_TOAST, toast_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "program_name", MXFT_STRING, NULL, 1, {MXU_6K_PROGRAM_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIOCAT_6K_TOAST, program_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "task_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIOCAT_6K_TOAST, task_number), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

MX_API_PRIVATE mx_status_type mxo_biocat_6k_toast_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxo_biocat_6k_toast_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxo_biocat_6k_toast_get_status( MX_OPERATION *op );
MX_API_PRIVATE mx_status_type mxo_biocat_6k_toast_start( MX_OPERATION *op );
MX_API_PRIVATE mx_status_type mxo_biocat_6k_toast_stop( MX_OPERATION *op );

extern MX_RECORD_FUNCTION_LIST mxo_biocat_6k_toast_record_function_list;
extern MX_OPERATION_FUNCTION_LIST mxo_biocat_6k_toast_operation_function_list;

extern long mxo_biocat_6k_toast_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxo_biocat_6k_toast_rfield_def_ptr;

#endif /* __O_BIOCAT_6K_TOAST_H__ */


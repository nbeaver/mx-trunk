/*
 * Name:    d_sis3807.h
 *
 * Purpose: Header for MX pulse generator driver for the Struck SIS 3807.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SIS3807_H__
#define __D_SIS3807_H__

/* Values for the 'sis3807_flags' field. */

#define MXF_SIS3807_RUNT_PULSE_FIX	0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *sis3807_record;
	long channel_number;
	mx_bool_type invert_output;
	unsigned long sis3807_flags;

	MX_CLOCK_TICK finish_time;
} MX_SIS3807_PULSER;

#define MXD_SIS3807_STANDARD_FIELDS \
  {-1, -1, "sis3807_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807_PULSER, sis3807_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807_PULSER, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "invert_output", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807_PULSER, invert_output), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "sis3807_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807_PULSER, sis3807_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_sis3807_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_sis3807_open( MX_RECORD *record );

MX_API mx_status_type mxd_sis3807_busy( MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_sis3807_start( MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_sis3807_stop( MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_sis3807_get_parameter(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_sis3807_set_parameter(
					MX_PULSE_GENERATOR *pulse_generator );

extern MX_RECORD_FUNCTION_LIST mxd_sis3807_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST 
		mxd_sis3807_pulse_generator_function_list;

extern long mxd_sis3807_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sis3807_rfield_def_ptr;

#endif /* __D_SIS3807_H__ */

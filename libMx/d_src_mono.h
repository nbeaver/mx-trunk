/*
 * Name:    d_src_mono.h
 *
 * Purpose: Header file for monochromators at the Synchrotron Radiation Center
 *          (SRC) of the University of Wisconsin-Madison.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SRC_MONO_H__
#define __D_SRC_MONO_H__

/* Values for the 'state' parameter below. */

#define MXS_SRC_MONO_IDLE	1
#define MXS_SRC_MONO_ACTIVE	2
#define MXS_SRC_MONO_TIMED_OUT	3
#define MXS_SRC_MONO_ERROR	4

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	int state;
} MX_SRC_MONO;

MX_API mx_status_type mxd_src_mono_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_src_mono_open( MX_RECORD *record );

MX_API mx_status_type mxd_src_mono_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_src_mono_get_extended_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_src_mono_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_src_mono_motor_function_list;

extern long mxd_src_mono_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_src_mono_rfield_def_ptr;

#define MXD_SRC_MONO_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SRC_MONO, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_SRC_MONO_H__ */


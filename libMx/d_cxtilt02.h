/*
 * Name:    d_cxtilt02.h
 *
 * Purpose: Header file for an MX input drivers that reads one of the angles
 *          from the Crossbow Technology CXTILT02 series of digital
 *          inclinometers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_CXTILT02_H__
#define __D_CXTILT02_H__

#include "mx_analog_input.h"

/* Define values for the 'angle_id' field below.  The 'angle_id' tells you
 * which of the two angles measured by the CXTILT02 to report.
 */

#define MXF_CXTILT02_PITCH	1
#define MXF_CXTILT02_ROLL	2

/* ===== CXTILT02 analog input register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *cxtilt02_record;
	int angle_id;
} MX_CXTILT02_ANGLE;

#define MXD_CXTILT02_ANGLE_STANDARD_FIELDS \
  {-1, -1, "cxtilt02_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CXTILT02_ANGLE, cxtilt02_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "angle_id", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CXTILT02_ANGLE, angle_id), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_cxtilt02_create_record_structures( MX_RECORD *record);

MX_API mx_status_type mxd_cxtilt02_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_cxtilt02_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST mxd_cxtilt02_analog_input_function_list;

extern mx_length_type mxd_cxtilt02_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_cxtilt02_rfield_def_ptr;

#endif /* __D_CXTILT02_H__ */


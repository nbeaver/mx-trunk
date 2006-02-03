/*
 * Name:    d_mardtb_status.h
 *
 * Purpose: Header file for the MarUSA Desktop Beamline's
 *          'status_dump' command.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MARDTB_STATUS_H__
#define __D_MARDTB_STATUS_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *mardtb_record;
	int parameter_number;
} MX_MARDTB_STATUS;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mardtb_status_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_mardtb_status_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_mardtb_status_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_mardtb_status_analog_input_function_list;

extern mx_length_type mxd_mardtb_status_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mardtb_status_rfield_def_ptr;

#define MXD_MARDTB_STATUS_STANDARD_FIELDS \
  {-1, -1, "mardtb_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARDTB_STATUS, mardtb_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "parameter_number", MXFT_INT, NULL, 0, {0}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_MARDTB_STATUS, parameter_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_MARDTB_STATUS_H__ */


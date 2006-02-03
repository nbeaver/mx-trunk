/*
 * Name:    d_mdrive_aio.h
 *
 * Purpose: Header file for MX input and output drivers to read 
 *          the analog input port on an Intelligent Motion Systems
 *          MDrive motor controller.
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

#ifndef __D_MDRIVE_AIO_H__
#define __D_MDRIVE_AIO_H__

#include "mx_analog_input.h"

/* ===== MDrive analog input register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mdrive_record;
} MX_MDRIVE_AINPUT;

#define MXD_MDRIVE_AINPUT_STANDARD_FIELDS \
  {-1, -1, "mdrive_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MDRIVE_AINPUT, mdrive_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mdrive_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mdrive_ain_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_mdrive_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_mdrive_ain_analog_input_function_list;

extern mx_length_type mxd_mdrive_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mdrive_ain_rfield_def_ptr;

#endif /* __D_MDRIVE_AIO_H__ */


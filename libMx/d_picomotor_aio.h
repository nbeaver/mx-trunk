/*
 * Name:    d_picomotor_aio.h
 *
 * Purpose: Header file for the MX input driver to control New Focus Picomotor
 *          analog input ports.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PICOMOTOR_AIO_H__
#define __D_PICOMOTOR_AIO_H__

#include "mx_analog_input.h"

/* ===== Picomotor analog input register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *picomotor_controller_record;
	char driver_name[MXU_PICOMOTOR_DRIVER_NAME_LENGTH+1];
	long channel_number;
} MX_PICOMOTOR_AINPUT;

#define MXD_PICOMOTOR_AINPUT_STANDARD_FIELDS \
  {-1, -1, "picomotor_controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PICOMOTOR_AINPUT, picomotor_controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "driver_name", MXFT_STRING, NULL, \
	  			1, {MXU_PICOMOTOR_DRIVER_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR_AINPUT, driver_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "channel_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR_AINPUT, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_picomotor_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_picomotor_ain_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_picomotor_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_picomotor_ain_analog_input_function_list;

extern long mxd_picomotor_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_picomotor_ain_rfield_def_ptr;

#endif /* __D_PICOMOTOR_AIO_H__ */


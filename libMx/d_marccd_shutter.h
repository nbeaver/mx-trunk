/*
 * Name:    d_marccd_shutter.h
 *
 * Purpose: MX header file for using a MarCCD controlled shutter.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003, 2008, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MARCCD_SHUTTER_H__
#define __D_MARCCD_SHUTTER_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *marccd_record;
} MX_MARCCD_SHUTTER;

#define MXD_MARCCD_SHUTTER_STANDARD_FIELDS \
  {-1, -1, "marccd_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARCCD_SHUTTER, marccd_record),\
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the device functions. */

MX_API mx_status_type mxd_marccd_shutter_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_marccd_shutter_relay_command(
							MX_RELAY *relay );

MX_API mx_status_type mxd_marccd_shutter_get_relay_status(
							MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_marccd_shutter_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_marccd_shutter_rly_function_list;

extern long mxd_marccd_shutter_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_marccd_shutter_rfield_def_ptr;

#endif /* __D_MARCCD_SHUTTER_H__ */

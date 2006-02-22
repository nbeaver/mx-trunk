/*
 * Name:    d_mardtb_shutter.h
 *
 * Purpose: MX header file for the shutter of a MarUSA Desktop Beamline
 *          goniostat controller.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MARDTB_SHUTTER_H__
#define __D_MARDTB_SHUTTER_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mardtb_record;
} MX_MARDTB_SHUTTER;

#define MXD_MARDTB_SHUTTER_STANDARD_FIELDS \
  {-1, -1, "mardtb_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARDTB_SHUTTER, mardtb_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the device functions. */

MX_API mx_status_type mxd_mardtb_shutter_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_mardtb_shutter_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_mardtb_shutter_get_relay_status( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_mardtb_shutter_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_mardtb_shutter_relay_function_list;

extern long mxd_mardtb_shutter_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mardtb_shutter_rfield_def_ptr;

#endif /* __D_MARDTB_SHUTTER_H__ */

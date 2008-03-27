/*
 * Name:    d_bluice_shutter.h
 *
 * Purpose: MX header file for Blu-Ice controlled shutters.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BLUICE_SHUTTER_H__
#define __D_BLUICE_SHUTTER_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bluice_server_record;
	char bluice_name[MXU_BLUICE_NAME_LENGTH+1];

	MX_BLUICE_FOREIGN_DEVICE *foreign_device;
} MX_BLUICE_SHUTTER;

#define MXD_BLUICE_SHUTTER_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_SHUTTER, bluice_server_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "bluice_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_SHUTTER, bluice_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the device functions. */

MX_API mx_status_type mxd_bluice_shutter_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bluice_shutter_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_bluice_shutter_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_bluice_shutter_get_relay_status( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_bluice_shutter_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_bluice_shutter_relay_function_list;

extern long mxd_bluice_shutter_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bluice_shutter_rfield_def_ptr;

#endif /* __D_BLUICE_SHUTTER_H__ */


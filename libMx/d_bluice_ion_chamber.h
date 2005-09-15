/*
 * Name:    d_bluice_ion_chamber.h
 *
 * Purpose: Header file for Blu-Ice ion chambers used as analog_inputs.
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BLUICE_ION_CHAMBER_H__
#define __D_BLUICE_ION_CHAMBER_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bluice_server_record;
	char bluice_name[MXU_BLUICE_NAME_LENGTH+1];

	MX_BLUICE_FOREIGN_ION_CHAMBER *foreign_ion_chamber;
} MX_BLUICE_ION_CHAMBER;

MX_API mx_status_type mxd_bluice_ion_chamber_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bluice_ion_chamber_open( MX_RECORD *record );

MX_API mx_status_type mxd_bluice_ion_chamber_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_bluice_ion_chamber_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_bluice_ion_chamber_analog_input_function_list;

extern long mxd_bluice_ion_chamber_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bluice_ion_chamber_rfield_def_ptr;

#define MXD_BLUICE_ION_CHAMBER_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_BLUICE_ION_CHAMBER, bluice_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bluice_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_ION_CHAMBER, bluice_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_BLUICE_ION_CHAMBER_H__ */


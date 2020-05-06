/*
 * Name:    d_galil_gclib_ainput.h
 *
 * Purpose: Header file for MX Galil controller analog inputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GALIL_GCLIB_AINPUT_H__
#define __D_GALIL_GCLIB_AINPUT_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *galil_gclib_record;
	unsigned long ainput_number;
	unsigned long ainput_type;
} MX_GALIL_GCLIB_AINPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_galil_gclib_ainput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_galil_gclib_ainput_open( MX_RECORD *record );

MX_API mx_status_type mxd_galil_gclib_ainput_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_galil_gclib_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_galil_gclib_ainput_analog_input_function_list;

extern long mxd_galil_gclib_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_ainput_rfield_def_ptr;

#define MXD_GALIL_GCLIB_AINPUT_STANDARD_FIELDS \
  {-1, -1, "galil_gclib_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_GALIL_GCLIB_AINPUT, galil_gclib_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "ainput_number", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB_AINPUT, ainput_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "ainput_type", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB_AINPUT, ainput_type), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_GALIL_GCLIB_AINPUT_H__ */


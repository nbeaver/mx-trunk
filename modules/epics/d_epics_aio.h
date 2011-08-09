/*
 * Name:    d_epics_aio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          EPICS process variables as if they were analog inputs and outputs.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_AIO_H__
#define __D_EPICS_AIO_H__

#include "mx_analog_input.h"
#include "mx_analog_output.h"

typedef struct {
	MX_RECORD *record;

	char epics_variable_name[ MXU_EPICS_PVNAME_LENGTH + 1 ];
	char dark_current_variable_name[ MXU_EPICS_PVNAME_LENGTH + 1 ];

	MX_EPICS_PV epics_pv;
	MX_EPICS_PV dark_current_pv;
} MX_EPICS_AINPUT;

typedef struct {
	MX_RECORD *record;

	char epics_variable_name[ MXU_EPICS_PVNAME_LENGTH + 1 ];

	MX_EPICS_PV epics_pv;
} MX_EPICS_AOUTPUT;

#define MXD_EPICS_AINPUT_STANDARD_FIELDS \
  {-1, -1, "epics_variable_name", MXFT_STRING, NULL, \
				1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_AINPUT, epics_variable_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "dark_current_variable_name", MXFT_STRING, NULL, \
				1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_EPICS_AINPUT, dark_current_variable_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_EPICS_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "epics_variable_name", MXFT_STRING, NULL, \
				1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_AOUTPUT, epics_variable_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_epics_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_ain_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_ain_read( MX_ANALOG_INPUT *dinput );
MX_API mx_status_type mxd_epics_ain_get_dark_current( MX_ANALOG_INPUT *dinput );
MX_API mx_status_type mxd_epics_ain_set_dark_current( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_epics_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_epics_ain_analog_input_function_list;

extern long mxd_epics_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_epics_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_aout_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_aout_read( MX_ANALOG_OUTPUT *doutput );
MX_API mx_status_type mxd_epics_aout_write( MX_ANALOG_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_epics_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_epics_aout_analog_output_function_list;

extern long mxd_epics_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_aout_rfield_def_ptr;

#endif /* __D_EPICS_AIO_H__ */


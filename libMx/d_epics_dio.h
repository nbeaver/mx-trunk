/*
 * Name:    d_epics_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          EPICS process variables as if they were digital I/O registers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_DIO_H__
#define __D_EPICS_DIO_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"


typedef struct {
	MX_RECORD *record;

	char epics_variable_name[ MXU_EPICS_PVNAME_LENGTH + 1 ];

	MX_EPICS_PV epics_pv;
} MX_EPICS_DINPUT;

typedef struct {
	MX_RECORD *record;

	char epics_variable_name[ MXU_EPICS_PVNAME_LENGTH + 1 ];

	MX_EPICS_PV epics_pv;
} MX_EPICS_DOUTPUT;

#define MXD_EPICS_DINPUT_STANDARD_FIELDS \
  {-1, -1, "epics_variable_name", MXFT_STRING, NULL, \
				1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_DINPUT, epics_variable_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_EPICS_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "epics_variable_name", MXFT_STRING, NULL, \
				1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_DOUTPUT, epics_variable_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_epics_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_din_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_epics_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_epics_din_digital_input_function_list;

extern long mxd_epics_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_epics_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_dout_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_epics_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_epics_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_epics_dout_digital_output_function_list;

extern long mxd_epics_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_dout_rfield_def_ptr;

#endif /* __D_EPICS_DIO_H__ */


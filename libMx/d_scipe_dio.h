/*
 * Name:    d_scipe_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          SCIPE detectors as if they were digital input registers and
 *          SCIPE actuators as if they were digital output registers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SCIPE_DIO_H__
#define __D_SCIPE_DIO_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== SCIPE digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *scipe_server_record;

	char scipe_scaler_name[ MX_SCIPE_OBJECT_NAME_LENGTH + 1 ];
} MX_SCIPE_DINPUT;

typedef struct {
	MX_RECORD *scipe_server_record;

	char scipe_actuator_name[ MX_SCIPE_OBJECT_NAME_LENGTH + 1 ];
} MX_SCIPE_DOUTPUT;

#define MXD_SCIPE_DINPUT_STANDARD_FIELDS \
  {-1, -1, "scipe_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_DINPUT, scipe_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scipe_scaler_name", MXFT_STRING, NULL, \
				1, {MX_SCIPE_OBJECT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_DINPUT, scipe_scaler_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_SCIPE_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "scipe_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_DOUTPUT, scipe_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scipe_actuator_name", MXFT_STRING, NULL, \
				1, {MX_SCIPE_OBJECT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_DOUTPUT, scipe_actuator_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_scipe_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_scipe_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_scipe_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_scipe_din_digital_input_function_list;

extern long mxd_scipe_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_scipe_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_scipe_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_scipe_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_scipe_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_scipe_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_scipe_dout_digital_output_function_list;

extern long mxd_scipe_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_scipe_dout_rfield_def_ptr;

#endif /* __D_SCIPE_DIO_H__ */


/*
 * Name:    d_scipe_aio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          SCIPE detectors as if they were analog input registers and
 *          SCIPE actuators as if they were analog output registers.
 *
 * Author:  William Lavender and Steven Weigand
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SCIPE_AIO_H__
#define __D_SCIPE_AIO_H__

#include "mx_analog_input.h"
#include "mx_analog_output.h"

/* ===== SCIPE analog input/output register data structures ===== */

typedef struct {
	MX_RECORD *scipe_server_record;

	char scipe_scaler_name[ MX_SCIPE_OBJECT_NAME_LENGTH + 1 ];
} MX_SCIPE_AINPUT;

typedef struct {
	MX_RECORD *scipe_server_record;

	char scipe_actuator_name[ MX_SCIPE_OBJECT_NAME_LENGTH + 1 ];
} MX_SCIPE_AOUTPUT;

#define MXD_SCIPE_AINPUT_STANDARD_FIELDS \
  {-1, -1, "scipe_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AINPUT, scipe_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scipe_scaler_name", MXFT_STRING, NULL, \
				1, {MX_SCIPE_OBJECT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AINPUT, scipe_scaler_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_SCIPE_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "scipe_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AOUTPUT, scipe_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scipe_actuator_name", MXFT_STRING, NULL, \
				1, {MX_SCIPE_OBJECT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_AOUTPUT, scipe_actuator_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_scipe_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_scipe_ain_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_scipe_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_scipe_ain_analog_input_function_list;

extern long mxd_scipe_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_scipe_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_scipe_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_scipe_aout_read( MX_ANALOG_OUTPUT *aoutput );
MX_API mx_status_type mxd_scipe_aout_write( MX_ANALOG_OUTPUT *aoutput );

extern MX_RECORD_FUNCTION_LIST mxd_scipe_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_scipe_aout_analog_output_function_list;

extern long mxd_scipe_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_scipe_aout_rfield_def_ptr;

#endif /* __D_SCIPE_AIO_H__ */


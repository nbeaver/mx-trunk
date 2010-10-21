/*
 * Name:    d_powerpmac_aio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          Power PMAC variables as if they were analog inputs and outputs.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_POWERPMAC_AIO_H__
#define __D_POWERPMAC_AIO_H__

#include "mx_analog_input.h"
#include "mx_analog_output.h"

/* ===== VME analog input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *powerpmac_record;
	char powerpmac_variable_name[ MXU_POWERPMAC_VARIABLE_NAME_LENGTH + 1 ];
} MX_POWERPMAC_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *powerpmac_record;
	char powerpmac_variable_name[ MXU_POWERPMAC_VARIABLE_NAME_LENGTH + 1 ];
} MX_POWERPMAC_AOUTPUT;

#define MXD_POWERPMAC_AINPUT_STANDARD_FIELDS \
  {-1, -1, "powerpmac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_AINPUT, powerpmac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "powerpmac_variable_name", MXFT_STRING, NULL, \
				1, {MXU_POWERPMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_POWERPMAC_AINPUT, powerpmac_variable_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_POWERPMAC_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "powerpmac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_AOUTPUT, powerpmac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "powerpmac_variable_name", MXFT_STRING, NULL, \
				1, {MXU_POWERPMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_POWERPMAC_AOUTPUT, powerpmac_variable_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_powerpmac_ain_create_record_structures(MX_RECORD *record);

MX_API mx_status_type mxd_powerpmac_ain_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_powerpmac_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_powerpmac_ain_analog_input_function_list;

extern long mxd_powerpmac_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_powerpmac_aout_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_powerpmac_aout_read( MX_ANALOG_OUTPUT *aoutput );
MX_API mx_status_type mxd_powerpmac_aout_write( MX_ANALOG_OUTPUT *aoutput );

extern MX_RECORD_FUNCTION_LIST mxd_powerpmac_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_powerpmac_aout_analog_output_function_list;

extern long mxd_powerpmac_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_aout_rfield_def_ptr;

#endif /* __D_POWERPMAC_AIO_H__ */


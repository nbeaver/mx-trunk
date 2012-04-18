/*
 * Name:    d_powerpmac_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          POWERPMAC variables as if they were digital I/O registers.
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

#ifndef __D_POWERPMAC_DIO_H__
#define __D_POWERPMAC_DIO_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== VME digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *powerpmac_record;
	long card_number;
	char powerpmac_variable_name[ MXU_POWERPMAC_VARIABLE_NAME_LENGTH + 1 ];
} MX_POWERPMAC_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *powerpmac_record;
	long card_number;
	char powerpmac_variable_name[ MXU_POWERPMAC_VARIABLE_NAME_LENGTH + 1 ];
} MX_POWERPMAC_DOUTPUT;

#define MXD_POWERPMAC_DINPUT_STANDARD_FIELDS \
  {-1, -1, "powerpmac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_DINPUT, powerpmac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "card_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_DINPUT, card_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "powerpmac_variable_name", MXFT_STRING, NULL, \
				1, {MXU_POWERPMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_DINPUT, powerpmac_variable_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_POWERPMAC_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "powerpmac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_DOUTPUT, powerpmac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "card_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_DOUTPUT, card_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "powerpmac_variable_name", MXFT_STRING, NULL, \
				1, {MXU_POWERPMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_DOUTPUT, powerpmac_variable_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_powerpmac_din_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_powerpmac_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_powerpmac_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_powerpmac_din_digital_input_function_list;

extern long mxd_powerpmac_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_powerpmac_dout_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_powerpmac_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_powerpmac_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_powerpmac_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_powerpmac_dout_digital_output_function_list;

extern long mxd_powerpmac_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_dout_rfield_def_ptr;

#endif /* __D_POWERPMAC_DIO_H__ */


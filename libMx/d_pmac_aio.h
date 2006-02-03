/*
 * Name:    d_pmac_aio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          PMAC variables as if they were analog inputs and outputs.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PMAC_AIO_H__
#define __D_PMAC_AIO_H__

#include "mx_analog_input.h"
#include "mx_analog_output.h"

/* ===== VME analog input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pmac_record;
	int card_number;
	char pmac_variable_name[ MXU_PMAC_VARIABLE_NAME_LENGTH + 1 ];
} MX_PMAC_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pmac_record;
	int card_number;
	char pmac_variable_name[ MXU_PMAC_VARIABLE_NAME_LENGTH + 1 ];
} MX_PMAC_AOUTPUT;

#define MXD_PMAC_AINPUT_STANDARD_FIELDS \
  {-1, -1, "pmac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_AINPUT, pmac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "card_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_AINPUT, card_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pmac_variable_name", MXFT_STRING, NULL, \
				1, {MXU_PMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_AINPUT, pmac_variable_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_PMAC_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "pmac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_AOUTPUT, pmac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "card_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_AOUTPUT, card_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pmac_variable_name", MXFT_STRING, NULL, \
				1, {MXU_PMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_AOUTPUT, pmac_variable_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_pmac_ain_create_record_structures(MX_RECORD *record);

MX_API mx_status_type mxd_pmac_ain_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_pmac_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_pmac_ain_analog_input_function_list;

extern mx_length_type mxd_pmac_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmac_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_pmac_aout_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_pmac_aout_read( MX_ANALOG_OUTPUT *aoutput );
MX_API mx_status_type mxd_pmac_aout_write( MX_ANALOG_OUTPUT *aoutput );

extern MX_RECORD_FUNCTION_LIST mxd_pmac_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_pmac_aout_analog_output_function_list;

extern mx_length_type mxd_pmac_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmac_aout_rfield_def_ptr;

#endif /* __D_PMAC_AIO_H__ */


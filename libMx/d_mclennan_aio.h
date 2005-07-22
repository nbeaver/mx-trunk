/*
 * Name:    d_mclennan_aio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          Mclennan analog I/O ports.
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

#ifndef __D_MCLENNAN_AIO_H__
#define __D_MCLENNAN_AIO_H__

#include "mx_analog_input.h"
#include "mx_analog_output.h"

/* ===== VME analog input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mclennan_record;
	int port_number;
} MX_MCLENNAN_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mclennan_record;
	int port_number;
} MX_MCLENNAN_AOUTPUT;

#define MXD_MCLENNAN_AINPUT_STANDARD_FIELDS \
  {-1, -1, "mclennan_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCLENNAN_AINPUT, mclennan_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCLENNAN_AINPUT, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_MCLENNAN_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "mclennan_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCLENNAN_AOUTPUT, mclennan_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCLENNAN_AOUTPUT, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_mclennan_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mclennan_ain_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_mclennan_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_mclennan_ain_analog_input_function_list;

extern long mxd_mclennan_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mclennan_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_mclennan_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mclennan_aout_read( MX_ANALOG_OUTPUT *doutput );
MX_API mx_status_type mxd_mclennan_aout_write( MX_ANALOG_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_mclennan_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_mclennan_aout_analog_output_function_list;

extern long mxd_mclennan_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mclennan_aout_rfield_def_ptr;

#endif /* __D_MCLENNAN_AIO_H__ */


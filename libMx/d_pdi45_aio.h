/*
 * Name:    d_pdi45_aio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          Prairie Digital Model 45 analog I/O lines.
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

#ifndef __D_PDI45_AIO_H__
#define __D_PDI45_AIO_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pdi45_record;
	int line_number;
} MX_PDI45_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pdi45_record;
	int line_number;
} MX_PDI45_AOUTPUT;

#define MXD_PDI45_AINPUT_STANDARD_FIELDS \
  {-1, -1, "pdi45_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_AINPUT, pdi45_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "line_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_AINPUT, line_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_PDI45_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "pdi45_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_AOUTPUT, pdi45_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "line_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_AOUTPUT, line_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_pdi45_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pdi45_ain_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_pdi45_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_pdi45_ain_analog_input_function_list;

extern long mxd_pdi45_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_pdi45_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pdi45_aout_read( MX_ANALOG_OUTPUT *doutput );
MX_API mx_status_type mxd_pdi45_aout_write( MX_ANALOG_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_pdi45_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_pdi45_aout_analog_output_function_list;

extern long mxd_pdi45_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_aout_rfield_def_ptr;

#endif /* __D_PDI45_DIO_H__ */


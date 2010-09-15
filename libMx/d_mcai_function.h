/*
 * Name:    d_mcai_function.h
 *
 * Purpose: Header file for an MX driver to compute a linear function
 *          of the values from a multichannel analog input record.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCAI_FUNCTION_H__
#define __D_MCAI_FUNCTION_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mcai_record;
	long num_channels;
	double *real_scale;
	double *real_offset;
} MX_MCAI_FUNCTION;

#define MXD_MCAI_FUNCTION_STANDARD_FIELDS \
  {-1, -1, "mcai_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCAI_FUNCTION, mcai_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_channels", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCAI_FUNCTION, num_channels), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "real_scale", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCAI_FUNCTION, real_scale), \
	{sizeof(double)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "real_offset", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCAI_FUNCTION, real_offset), \
	{sizeof(double)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}

MX_API mx_status_type mxd_mcai_function_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_mcai_function_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_mcai_function_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_mcai_function_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_mcai_function_analog_input_function_list;

extern long mxd_mcai_function_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mcai_function_rfield_def_ptr;

#endif /* __D_MCAI_FUNCTION_H__ */


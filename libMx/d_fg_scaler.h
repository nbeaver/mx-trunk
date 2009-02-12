/*
 * Name:    d_fg_scaler.h
 *
 * Purpose: Header file for software emulated scaler that behaves like
 *          a function generator.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_FG_SCALER_H__
#define __D_FG_SCALER_H__

/* Values for the 'function' field. */

#define MXT_FG_SCALER_SQUARE_WAVE	1
#define MXT_FG_SCALER_SINE_WAVE		2
#define MXT_FG_SCALER_SAWTOOTH_WAVE	3

typedef struct {
	double amplitude;
	double frequency;	/* in Hz */
	long function;

	struct timespec start_time;
	double period;		/* in seconds */
} MX_FG_SCALER;

MX_API mx_status_type mxd_fg_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_fg_scaler_open( MX_RECORD *record );

MX_API mx_status_type mxd_fg_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_fg_scaler_read( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_fg_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_fg_scaler_scaler_function_list;

extern long mxd_fg_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_fg_scaler_rfield_def_ptr;

#define MXD_FG_SCALER_STANDARD_FIELDS \
  {-1, -1, "amplitude", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FG_SCALER, amplitude), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "frequency", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FG_SCALER, frequency), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_FG_SCALER_H__ */


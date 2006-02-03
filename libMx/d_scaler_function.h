/*
 * Name:    d_scaler_function.h 
 *
 * Purpose: Header file for MX scaler driver to use a linear function of
 *          multiple MX scalers to generate a pseudoscaler.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2002-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SCALER_FUNCTION_H__
#define __D_SCALER_FUNCTION_H__

#include "mx_scaler.h"

/* ===== MX scaler function data structures ===== */

typedef struct {
	MX_RECORD *record;
	unsigned long scaler_function_flags;

	char master_scaler_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	MX_RECORD *master_scaler_record;

	long num_records;
	MX_RECORD **record_array;
	double *real_scale;
	double *real_offset;

	/* Scaler arrays */

	long num_scalers;
	MX_RECORD **scaler_record_array;
	double *real_scaler_scale;
	double *real_scaler_offset;

	/* Variable arrays */

	long num_variables;
	MX_RECORD **variable_record_array;
	double *real_variable_scale;
	double *real_variable_offset;

} MX_SCALER_FUNCTION;

/* Values for the scaler_function_flags field. */

/* None as of yet. */

/* Define all of the interface functions. */

MX_API mx_status_type mxd_scaler_function_initialize_type( long type );
MX_API mx_status_type mxd_scaler_function_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_scaler_function_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_scaler_function_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_scaler_function_print_scaler_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_scaler_function_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_scaler_function_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_scaler_function_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_scaler_function_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_scaler_function_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_scaler_function_set_parameter( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_scaler_function_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_scaler_function_scaler_function_list;

extern mx_length_type mxd_scaler_function_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_scaler_function_rfield_def_ptr;

#define MXD_SCALER_FUNCTION_STANDARD_FIELDS \
  {-1, -1, "scaler_function_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SCALER_FUNCTION, scaler_function_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "master_scaler_name", MXFT_STRING, NULL, \
					1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCALER_FUNCTION, master_scaler_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_records", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCALER_FUNCTION, num_records), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "record_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCALER_FUNCTION, record_array),\
	{sizeof(MX_RECORD *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)},\
  \
  {-1, -1, "real_scale", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCALER_FUNCTION, real_scale), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)},\
  \
  {-1, -1, "real_offset", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCALER_FUNCTION, real_offset), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}

#endif /* __D_SCALER_FUNCTION_H__ */

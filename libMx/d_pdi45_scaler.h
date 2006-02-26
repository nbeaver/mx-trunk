/*
 * Name:    d_pdi45_scaler.h
 *
 * Purpose: Header file for MX scaler driver to control a Prairie Digital
 *          Model 45 digital I/O line as an MX scaler.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PDI45_SCALER_H__
#define __D_PDI45_SCALER_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pdi45_record;
	long line_number;
} MX_PDI45_SCALER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_pdi45_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pdi45_scaler_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_pdi45_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_pdi45_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_pdi45_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_pdi45_scaler_set_parameter( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_pdi45_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_pdi45_scaler_scaler_function_list;

extern long mxd_pdi45_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_scaler_rfield_def_ptr;

#define MXD_PDI45_SCALER_STANDARD_FIELDS \
  {-1, -1, "pdi45_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_SCALER, pdi45_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "line_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_SCALER, line_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_PDI45_SCALER_H__ */


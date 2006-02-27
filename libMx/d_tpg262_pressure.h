/*
 * Name:    d_tpg262_pressure.h
 *
 * Purpose: Header file for vacuum gauge pressure measurements with 
 *          a Pfeiffer TPG 261 or TPG 262 vacuum gauge controller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_TPG262_PRESSURE_H__
#define __D_TPG262_PRESSURE_H__

/* ===== TPG262_PRESSURE analog input register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *tpg262_record;
	long gauge_number;
} MX_TPG262_PRESSURE;

#define MXD_TPG262_PRESSURE_STANDARD_FIELDS \
  {-1, -1, "tpg262_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TPG262_PRESSURE, tpg262_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "gauge_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TPG262_PRESSURE, gauge_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_tpg262_pressure_create_record_structures(
							MX_RECORD *record);

MX_API mx_status_type mxd_tpg262_pressure_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_tpg262_pressure_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_tpg262_pressure_analog_input_function_list;

extern long mxd_tpg262_pressure_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_tpg262_pressure_rfield_def_ptr;

#endif /* __D_TPG262_PRESSURE_H__ */


/*
 * Name:    d_adc_table.h 
 *
 * Purpose: Header file for the ADC table used at APS Sector 17.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ADC_TABLE_H__
#define __D_ADC_TABLE_H__

#include "mx_table.h"

#define MX_ADC_TABLE_NUM_MOTORS  6

/* Define labels for the raw motors. */

#define MXF_ADC_TABLE_X1	0
#define MXF_ADC_TABLE_Y1	1
#define MXF_ADC_TABLE_Y2	2
#define MXF_ADC_TABLE_Z1	3
#define MXF_ADC_TABLE_Z2	4
#define MXF_ADC_TABLE_Z3	5

/* See the description of the table geometry for the definition of
 * m1, m2, m3, rx, ry, and rz.
 */

typedef struct {
	MX_RECORD *motor_record_array[ MX_ADC_TABLE_NUM_MOTORS ];

	/* Table geometry parameters. */

	double m1;
	double m2;
	double m3;

	/* Center of rotation relative to the table zero point. */

	double rx;
	double ry;
	double rz;

} MX_ADC_TABLE;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_adc_table_initialize_type( long type );
MX_API mx_status_type mxd_adc_table_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_adc_table_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_adc_table_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_adc_table_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_adc_table_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_adc_table_open( MX_RECORD *record );
MX_API mx_status_type mxd_adc_table_close( MX_RECORD *record );
MX_API mx_status_type mxd_adc_table_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_adc_table_is_busy( MX_TABLE *motor );
MX_API mx_status_type mxd_adc_table_move_absolute( MX_TABLE *motor );
MX_API mx_status_type mxd_adc_table_get_position( MX_TABLE *motor );
MX_API mx_status_type mxd_adc_table_set_position( MX_TABLE *motor );
MX_API mx_status_type mxd_adc_table_soft_abort( MX_TABLE *motor );
MX_API mx_status_type mxd_adc_table_immediate_abort( MX_TABLE *motor );
MX_API mx_status_type mxd_adc_table_positive_limit_hit( MX_TABLE *motor );
MX_API mx_status_type mxd_adc_table_negative_limit_hit( MX_TABLE *motor );

extern MX_RECORD_FUNCTION_LIST mxd_adc_table_record_function_list;
extern MX_TABLE_FUNCTION_LIST mxd_adc_table_table_function_list;

extern mx_length_type mxd_adc_table_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_adc_table_rfield_def_ptr;

#define MXD_ADC_TABLE_STANDARD_FIELDS \
  {-1, -1, "motor_record_array", MXFT_RECORD, NULL, \
						1, {MX_ADC_TABLE_NUM_MOTORS}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ADC_TABLE, motor_record_array), \
	{sizeof(MX_RECORD *)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "m1", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ADC_TABLE, m1), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "m2", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ADC_TABLE, m2), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "m3", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ADC_TABLE, m3), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "rx", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ADC_TABLE, rx), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "ry", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ADC_TABLE, ry), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "rz", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ADC_TABLE, rz), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_ADC_TABLE_H__ */

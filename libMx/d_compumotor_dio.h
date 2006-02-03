/*
 * Name:    d_compumotor_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          COMPUMOTOR variables as if they were digital I/O registers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_COMPUMOTOR_DIO_H__
#define __D_COMPUMOTOR_DIO_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== VME digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *compumotor_interface_record;
	int controller_number;
	int brick_number;
	int first_bit;
	int num_bits;

	int controller_index;
} MX_COMPUMOTOR_DINPUT;

typedef struct {
	MX_RECORD *compumotor_interface_record;
	int controller_number;
	int brick_number;
	int first_bit;
	int num_bits;

	int controller_index;
} MX_COMPUMOTOR_DOUTPUT;

#define MXD_COMPUMOTOR_DINPUT_STANDARD_FIELDS \
  {-1, -1, "compumotor_interface_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_DINPUT, compumotor_interface_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "controller_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_DINPUT, controller_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "brick_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_DINPUT, brick_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "first_bit", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_DINPUT, first_bit), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_bits", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_DINPUT, num_bits), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_COMPUMOTOR_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "compumotor_interface_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_DOUTPUT, compumotor_interface_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "controller_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_DOUTPUT, controller_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "brick_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_DOUTPUT, brick_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "first_bit", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_DOUTPUT, first_bit), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_bits", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_DOUTPUT, num_bits), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_compumotor_din_initialize_type( long type );
MX_API mx_status_type mxd_compumotor_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_din_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_din_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_din_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_din_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_din_open( MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_din_close( MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_compumotor_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_compumotor_din_digital_input_function_list;

extern mx_length_type mxd_compumotor_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_compumotor_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_compumotor_dout_initialize_type( long type );
MX_API mx_status_type mxd_compumotor_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_dout_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_dout_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_dout_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_dout_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_dout_open( MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_dout_close( MX_RECORD *record );
MX_API mx_status_type mxd_compumotor_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_compumotor_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_compumotor_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_compumotor_dout_digital_output_function_list;

extern mx_length_type mxd_compumotor_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_compumotor_dout_rfield_def_ptr;

#endif /* __D_COMPUMOTOR_DIO_H__ */


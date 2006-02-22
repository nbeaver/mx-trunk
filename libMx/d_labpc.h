/*
 * Name:    d_labpc.h
 *
 * Purpose: Header file for MX drivers for the National Instruments
 *          LAB-PC+ data acquisition card.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_LABPC_H__
#define __D_LABPC_H__

#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ============ Analog input channels ============ */

typedef struct {
	int file_handle;
	char filename[MXU_FILENAME_LENGTH + 1];
	int channel;
	int gain;
	int use_unipolar_input;
	int use_dma;
	int trigger_type;
	int use_differential_input;
} MX_LABPC_ADC;

#define MXD_LABPC_ADC_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_ADC, filename), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_ADC, channel), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "gain", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_ADC, gain), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "use_unipolar_input", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_ADC, use_unipolar_input), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "use_dma", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_ADC, use_dma), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "trigger_type", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_ADC, trigger_type), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "use_differential_input", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_ADC, use_differential_input), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

MX_API mx_status_type mxd_labpc_adc_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_adc_open( MX_RECORD *record );
MX_API mx_status_type mxd_labpc_adc_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxd_labpc_adc_record_function_list;

MX_API mx_status_type mxd_labpc_adc_read( MX_ANALOG_INPUT *adc );

extern MX_ANALOG_INPUT_FUNCTION_LIST mxd_labpc_adc_analog_input_function_list;

extern long mxd_labpc_adc_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_labpc_adc_rfield_def_ptr;

/* ============ Analog output channels ============ */

typedef struct {
	int file_handle;
	char filename[MXU_FILENAME_LENGTH + 1];
	int use_unipolar_output;
} MX_LABPC_DAC;

#define MXD_LABPC_DAC_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_DAC, filename), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "use_unipolar_output", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_DAC, use_unipolar_output), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

MX_API mx_status_type mxd_labpc_dac_initialize_type( long type );
MX_API mx_status_type mxd_labpc_dac_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dac_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dac_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dac_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dac_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dac_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dac_open( MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dac_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxd_labpc_dac_record_function_list;

MX_API mx_status_type mxd_labpc_dac_read( MX_ANALOG_OUTPUT *dac );
MX_API mx_status_type mxd_labpc_dac_write( MX_ANALOG_OUTPUT *dac );

extern MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_labpc_dac_analog_output_function_list;

extern long mxd_labpc_dac_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_labpc_dac_rfield_def_ptr;

/* ============ Digital input channels ============ */

typedef struct {
	int file_handle;
	char filename[MXU_FILENAME_LENGTH + 1];
} MX_LABPC_DIN;

#define MXD_LABPC_DIN_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_DIN, filename), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_labpc_din_initialize_type( long type );
MX_API mx_status_type mxd_labpc_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_din_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_din_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_labpc_din_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_din_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_din_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_din_open( MX_RECORD *record );
MX_API mx_status_type mxd_labpc_din_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxd_labpc_din_record_function_list;

MX_API mx_status_type mxd_labpc_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_DIGITAL_INPUT_FUNCTION_LIST mxd_labpc_din_digital_input_function_list;

extern long mxd_labpc_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_labpc_din_rfield_def_ptr;

/* ============ Digital output channels ============ */

typedef struct {
	int file_handle;
	char filename[MXU_FILENAME_LENGTH + 1];
} MX_LABPC_DOUT;

#define MXD_LABPC_DOUT_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABPC_DOUT, filename), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_labpc_dout_initialize_type( long type );
MX_API mx_status_type mxd_labpc_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dout_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dout_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dout_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dout_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dout_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dout_open( MX_RECORD *record );
MX_API mx_status_type mxd_labpc_dout_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxd_labpc_dout_record_function_list;

MX_API mx_status_type mxd_labpc_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_labpc_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_labpc_dout_digital_output_function_list;

extern long mxd_labpc_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_labpc_dout_rfield_def_ptr;

#endif /* __D_LABPC_H__ */

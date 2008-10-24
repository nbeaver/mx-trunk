/*
 * Name:    d_vme_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          VME addresses as if they were digital I/O registers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_VME_DIO_H__
#define __D_VME_DIO_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== VME digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *vme_record;
	char address_mode_name[ MXU_VME_ADDRESS_MODE_LENGTH + 1 ];
	unsigned long address_mode;
	unsigned long crate_number;
	unsigned long address;
	char data_size_name[ MXU_VME_DATA_SIZE_LENGTH + 1 ];
	unsigned long data_size;
} MX_VME_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *vme_record;
	char address_mode_name[ MXU_VME_ADDRESS_MODE_LENGTH + 1 ];
	unsigned long address_mode;
	unsigned long crate_number;
	unsigned long address;
	char data_size_name[ MXU_VME_DATA_SIZE_LENGTH + 1 ];
	unsigned long data_size;
} MX_VME_DOUTPUT;

#define MXD_VME_DINPUT_STANDARD_FIELDS \
  {-1, -1, "vme_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME_DINPUT, vme_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address_mode_name", MXFT_STRING, NULL, \
				1, {MXU_VME_ADDRESS_MODE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME_DINPUT, address_mode_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "crate_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME_DINPUT, crate_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME_DINPUT, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "data_size_name", MXFT_STRING, NULL, \
				1, {MXU_VME_DATA_SIZE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME_DINPUT, data_size_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_VME_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "vme_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME_DOUTPUT, vme_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address_mode_name", MXFT_STRING, NULL, \
				1, {MXU_VME_ADDRESS_MODE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME_DOUTPUT, address_mode_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "crate_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME_DOUTPUT, crate_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME_DOUTPUT, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "data_size_name", MXFT_STRING, NULL, \
				1, {MXU_VME_DATA_SIZE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME_DOUTPUT, data_size_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}


/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_vme_din_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_vme_din_open( MX_RECORD *record );

MX_API mx_status_type mxd_vme_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_vme_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_vme_din_digital_input_function_list;

extern long mxd_vme_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_vme_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_vme_dout_create_record_structures( MX_RECORD *record);
MX_API mx_status_type mxd_vme_dout_open( MX_RECORD *record );

MX_API mx_status_type mxd_vme_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_vme_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_vme_dout_digital_output_function_list;

extern long mxd_vme_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_vme_dout_rfield_def_ptr;

#endif /* __D_VME_DIO_H__ */


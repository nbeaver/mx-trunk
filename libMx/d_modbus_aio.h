/*
 * Name:    d_modbus_aio.h
 *
 * Purpose: Header file for MX input and output drivers for reading and
 *          writing MODBUS coils and registers as analog I/O devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MODBUS_AIO_H__
#define __D_MODBUS_AIO_H__

/* ===== MODBUS analog input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *modbus_record;
	unsigned long modbus_address;
	long num_bits;
	unsigned long modbus_function_code;
} MX_MODBUS_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *modbus_record;
	unsigned long modbus_address;
	long num_bits;
	unsigned long modbus_function_code;
} MX_MODBUS_AOUTPUT;

#define MXD_MODBUS_AINPUT_STANDARD_FIELDS \
  {-1, -1, "modbus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_MODBUS_AINPUT, modbus_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "modbus_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_AINPUT, modbus_address),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_bits", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_AINPUT, num_bits),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "modbus_function_code", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_AINPUT, modbus_function_code),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_MODBUS_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "modbus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_MODBUS_AOUTPUT, modbus_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "modbus_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_AOUTPUT, modbus_address),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_bits", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_AOUTPUT, num_bits),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "modbus_function_code", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_AOUTPUT, modbus_function_code),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_modbus_ain_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_modbus_ain_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_modbus_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_modbus_ain_analog_input_function_list;

extern long mxd_modbus_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_modbus_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_modbus_aout_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_modbus_aout_read( MX_ANALOG_OUTPUT *dinput );
MX_API mx_status_type mxd_modbus_aout_write( MX_ANALOG_OUTPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_modbus_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_modbus_aout_analog_output_function_list;

extern long mxd_modbus_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_modbus_aout_rfield_def_ptr;

#endif /* __D_MODBUS_AIO_H__ */


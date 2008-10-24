/*
 * Name:    d_portio_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          I/O ports as digital input and output devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PORTIO_DIO_H__
#define __D_PORTIO_DIO_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== Port I/O digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *portio_record;
	unsigned long address;
	unsigned long data_size;
} MX_PORTIO_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *portio_record;
	unsigned long address;
	unsigned long data_size;
} MX_PORTIO_DOUTPUT;

#define MXD_PORTIO_DINPUT_STANDARD_FIELDS \
  {-1, -1, "portio_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PORTIO_DINPUT, portio_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PORTIO_DINPUT, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "data_size", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PORTIO_DINPUT, data_size), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_PORTIO_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "portio_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PORTIO_DOUTPUT, portio_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PORTIO_DOUTPUT, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "data_size", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PORTIO_DOUTPUT, data_size), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}


/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_portio_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_portio_din_open( MX_RECORD *record );
MX_API mx_status_type mxd_portio_din_close( MX_RECORD *record );

MX_API mx_status_type mxd_portio_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_portio_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_portio_din_digital_input_function_list;

extern long mxd_portio_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_portio_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_portio_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_portio_dout_open( MX_RECORD *record );
MX_API mx_status_type mxd_portio_dout_close( MX_RECORD *record );

MX_API mx_status_type mxd_portio_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_portio_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_portio_dout_digital_output_function_list;

extern long mxd_portio_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_portio_dout_rfield_def_ptr;

#endif /* __D_PORTIO_DIO_H__ */


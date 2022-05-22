/*
 * Name:    i_modbus_rtu.h
 *
 * Purpose: Header for MX interface driver for MODBUS serial fieldbus
 *          communication using the RTU format.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004, 2006, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_MODBUS_RTU_H__
#define __I_MODBUS_RTU_H__

#define MXI_MODBUS_RTU_SILENT_CHARACTER_TIME	3.5   /* character times */

#define MXI_MODBUS_RTU_MINIMUM_SILENT_TIME_US	1750  /* in microseconds */

#define MXI_MODBUS_RTU_SILENT_TIME_PAD_US	1000  /* in microseconds */

/* Define the data structures used by the MODBUS/TCP interface code. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long address;
	unsigned long modbus_rtu_flags;

	uint8_t send_buffer[MXU_MODBUS_SERIAL_ADU_LENGTH];
	uint8_t receive_buffer[MXU_MODBUS_SERIAL_ADU_LENGTH];

	unsigned long silent_time_us;	/* In microseconds */

	struct timespec character_timespec;
	struct timespec silent_timespec;

	struct timespec padded_silent_timespec;

	struct timespec next_send_timespec;
} MX_MODBUS_RTU;

extern MX_RECORD_FUNCTION_LIST mxi_modbus_rtu_record_function_list;
extern MX_MODBUS_FUNCTION_LIST mxi_modbus_rtu_modbus_function_list;

extern long mxi_modbus_rtu_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_modbus_rtu_rfield_def_ptr;

#define MXI_MODBUS_RTU_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_RTU, rs232_record), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_RTU, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "modbus_rtu_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_RTU, modbus_rtu_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "silent_time_us", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_RTU, silent_time_us), \
	{0}, NULL, 0 }

/* Define all of the interface functions. */

MX_API mx_status_type mxi_modbus_rtu_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_modbus_rtu_open( MX_RECORD *record );

MX_API mx_status_type mxi_modbus_rtu_command( MX_MODBUS *modbus );

#endif /* __I_MODBUS_RTU_H__ */


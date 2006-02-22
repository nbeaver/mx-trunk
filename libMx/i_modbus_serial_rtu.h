/*
 * Name:    i_modbus_serial_rtu.h
 *
 * Purpose: Header for MX interface driver for MODBUS serial fieldbus
 *          communication using the RTU format.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_MODBUS_SERIAL_RTU_H__
#define __I_MODBUS_SERIAL_RTU_H__

/* Define the data structures used by the MODBUS/TCP interface code. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long address;

	uint8_t send_buffer[MXU_MODBUS_SERIAL_ADU_LENGTH];
	uint8_t receive_buffer[MXU_MODBUS_SERIAL_ADU_LENGTH];
} MX_MODBUS_SERIAL_RTU;

extern MX_RECORD_FUNCTION_LIST mxi_modbus_serial_rtu_record_function_list;
extern MX_MODBUS_FUNCTION_LIST mxi_modbus_serial_rtu_modbus_function_list;

extern long mxi_modbus_serial_rtu_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_modbus_serial_rtu_rfield_def_ptr;

#define MXI_MODBUS_SERIAL_RTU_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_SERIAL_RTU, rs232_record), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MODBUS_SERIAL_RTU, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxi_modbus_serial_rtu_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_modbus_serial_rtu_open( MX_RECORD *record );

MX_API mx_status_type mxi_modbus_serial_rtu_command( MX_MODBUS *modbus );

#endif /* __I_MODBUS_SERIAL_RTU_H__ */


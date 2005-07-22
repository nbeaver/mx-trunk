/*
 * Name:    mx_modbus.h
 *
 * Purpose: Header file for MODBUS interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MODBUS_H__
#define __MX_MODBUS_H__

#include "mx_record.h"

/*** MODBUS message length limits ***/

#define MXU_MODBUS_PDU_LENGTH			253

#define MXU_MODBUS_SERIAL_ADU_LENGTH		256
#define MXU_MODBUS_TCP_ADU_LENGTH		260

#define MXU_MODBUS_MAX_COILS_READABLE		2000
#define MXU_MODBUS_MAX_REGISTERS_READABLE	125

#define MXU_MODBUS_MAX_COILS_WRITEABLE		1968
#define MXU_MODBUS_MAX_REGISTERS_WRITEABLE	120

#define MXU_MODBUS_MAX_READ_WRITE_REGISTERS	118

#define MXU_MODBUS_MAX_FIFO_VALUES		31

/*** MODBUS function codes ***/

#define MXF_MOD_READ_COILS				0x01
#define MXF_MOD_READ_DISCRETE_INPUTS			0x02
#define MXF_MOD_READ_HOLDING_REGISTERS			0x03
#define MXF_MOD_READ_INPUT_REGISTERS			0x04
#define MXF_MOD_WRITE_SINGLE_COIL			0x05
#define MXF_MOD_WRITE_SINGLE_REGISTER			0x06
#define MXF_MOD_READ_EXCEPTION_STATUS			0x07
#define MXF_MOD_DIAGNOSTICS				0x08
#define MXF_MOD_GET_COMM_EVENT_COUNTER			0x0b
#define MXF_MOD_GET_COMM_EVENT_LOG			0x0c
#define MXF_MOD_WRITE_MULTIPLE_COILS			0x0f
#define MXF_MOD_WRITE_MULTIPLE_REGISTERS		0x10
#define MXF_MOD_REPORT_SLAVE_ID				0x11
#define MXF_MOD_READ_FILE_RECORD			0x14
#define MXF_MOD_WRITE_FILE_RECORD			0x15
#define MXF_MOD_MASK_WRITE_REGISTER			0x16
#define MXF_MOD_READ_WRITE_MULTIPLE_REGISTERS		0x17
#define MXF_MOD_READ_FIFO_QUEUE				0x18
#define MXF_MOD_ENCAPSULATED_INTERFACE_TRANSPORT	0x2b

/*** MODBUS exception codes ***/

#define MXF_MOD_EXCEPTION				0x80

#define MXF_MOD_EXC_ILLEGAL_FUNCTION			0x01
#define MXF_MOD_EXC_ILLEGAL_DATA_ADDRESS		0x02
#define MXF_MOD_EXC_ILLEGAL_DATA_VALUE			0x03
#define MXF_MOD_EXC_SLAVE_DEVICE_FAILURE		0x04
#define MXF_MOD_EXC_ACKNOWLEDGE				0x05
#define MXF_MOD_EXC_SLAVE_DEVICE_BUSY			0x06
#define MXF_MOD_EXC_MEMORY_PARITY_ERROR			0x08
#define MXF_MOD_EXC_GATEWAY_PATH_UNAVAILABLE		0x0a
#define MXF_MOD_EXC_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND	0x0b

/*** Field indices in the MODBUS protocol data unit (PDU). ***/

#define MXF_MOD_PDU_FUNCTION_CODE			0
#define MXF_MOD_PDU_DATA				1

typedef struct {
	MX_RECORD *record;

	unsigned long modbus_flags;

	size_t request_length;
	size_t response_buffer_length;
	size_t actual_response_length;

	mx_uint8_type *request_pointer;
	mx_uint8_type *response_pointer;
} MX_MODBUS;

#define MX_MODBUS_STANDARD_FIELDS \
  {-1, -1, "modbus_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_MODBUS, modbus_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/*
 * The structure type MX_MODBUS_FUNCTION_LIST contains a list of
 * pointers to functions that vary from interface type to
 * interface type.
 */

typedef struct {
	mx_status_type ( *command ) ( MX_MODBUS *modbus );
} MX_MODBUS_FUNCTION_LIST;

/* ============== Interface function prototypes. ============== */

MX_API mx_status_type mx_modbus_get_pointers( MX_RECORD *modbus_record,
					MX_MODBUS **modbus,
					MX_MODBUS_FUNCTION_LIST **fptr,
					const char *calling_fname );

MX_API mx_status_type mx_modbus_compute_response_length(
					MX_RECORD *modbus_record,
					mx_uint8_type *receive_buffer,
					size_t *response_length );
					
MX_API mx_status_type mx_modbus_command( MX_RECORD *modbus_record,
					mx_uint8_type *request_buffer,
					size_t request_length,
					mx_uint8_type *response_buffer,
					size_t response_buffer_length,
					size_t *actual_response_length );

/* --- */

/* ... not all function codes are implemented yet ... */

MX_API mx_status_type mx_modbus_read_coils( MX_RECORD *modbus_record,
					unsigned int starting_address,
					unsigned int num_coils,
					mx_uint8_type *coil_value_array );

MX_API mx_status_type mx_modbus_read_discrete_inputs( MX_RECORD *modbus_record,
					unsigned int starting_address,
					unsigned int num_inputs,
					mx_uint8_type *input_value_array );

MX_API mx_status_type mx_modbus_read_holding_registers(MX_RECORD *modbus_record,
					unsigned int starting_address,
					unsigned int num_registers,
					mx_uint16_type *register_value_array );

MX_API mx_status_type mx_modbus_read_input_registers( MX_RECORD *modbus_record,
					unsigned int starting_address,
					unsigned int num_registers,
					mx_uint16_type *register_value_array );

MX_API mx_status_type mx_modbus_write_single_coil( MX_RECORD *modbus_record,
					unsigned int output_address,
					unsigned int coil_value );

MX_API mx_status_type mx_modbus_write_single_register( MX_RECORD *modbus_record,
					unsigned int register_address,
					mx_uint16_type register_value );

MX_API mx_status_type mx_modbus_read_exception_status( MX_RECORD *modbus_record,
					mx_uint8_type *exception_status );

MX_API mx_status_type mx_modbus_get_comm_event_counter(MX_RECORD *modbus_record,
					mx_uint16_type *status_word,
					mx_uint16_type *event_count );

MX_API mx_status_type mx_modbus_write_multiple_coils( MX_RECORD *modbus_record,
					unsigned int starting_address,
					unsigned int num_coils,
					mx_uint8_type *coil_value_bit_array );

MX_API mx_status_type mx_modbus_write_multiple_registers(
					MX_RECORD *modbus_record,
					unsigned int starting_address,
					unsigned int num_registers,
					mx_uint16_type *register_value_array );

MX_API mx_status_type mx_modbus_mask_write_register( MX_RECORD *modbus_record,
					unsigned int register_address,
					mx_uint16_type and_mask,
					mx_uint16_type or_mask );

MX_API mx_status_type mx_modbus_read_write_multiple_registers(
					MX_RECORD *modbus_record,
					unsigned int read_starting_address,
					unsigned int num_registers_to_read,
					mx_uint16_type *read_register_array,
					unsigned int write_starting_address,
					unsigned int num_registers_to_write,
					mx_uint16_type *write_register_array );

MX_API mx_status_type mx_modbus_read_fifo_queue( MX_RECORD *modbus_record,
					unsigned int fifo_pointer_address,
					unsigned int max_fifo_values,
					mx_uint16_type *fifo_value_array,
					unsigned int *num_fifo_values_read );

#endif /* __MX_MODBUS_H__ */

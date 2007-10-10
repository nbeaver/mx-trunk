/*
 * Name:    mx_modbus.h
 *
 * Purpose: Header file for MODBUS interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MODBUS_H__
#define __MX_MODBUS_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

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

	uint8_t *request_pointer;
	uint8_t *response_pointer;
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
					uint8_t *receive_buffer,
					size_t *response_length );
					
MX_API mx_status_type mx_modbus_command( MX_RECORD *modbus_record,
					uint8_t *request_buffer,
					size_t request_length,
					uint8_t *response_buffer,
					size_t response_buffer_length,
					size_t *actual_response_length );

/* --- */

/* ... not all function codes are implemented yet ... */

MX_API mx_status_type mx_modbus_read_coils( MX_RECORD *modbus_record,
					unsigned long starting_address,
					unsigned long num_coils,
					uint8_t *coil_value_array );

MX_API mx_status_type mx_modbus_read_discrete_inputs( MX_RECORD *modbus_record,
					unsigned long starting_address,
					unsigned long num_inputs,
					uint8_t *input_value_array );

MX_API mx_status_type mx_modbus_read_holding_registers(MX_RECORD *modbus_record,
					unsigned long starting_address,
					unsigned long num_registers,
					uint16_t *register_value_array );

MX_API mx_status_type mx_modbus_read_input_registers( MX_RECORD *modbus_record,
					unsigned long starting_address,
					unsigned long num_registers,
					uint16_t *register_value_array );

MX_API mx_status_type mx_modbus_write_single_coil( MX_RECORD *modbus_record,
					unsigned long output_address,
					unsigned long coil_value );

MX_API mx_status_type mx_modbus_write_single_register( MX_RECORD *modbus_record,
					unsigned long register_address,
					uint16_t register_value );

MX_API mx_status_type mx_modbus_read_exception_status( MX_RECORD *modbus_record,
					uint8_t *exception_status );

MX_API mx_status_type mx_modbus_get_comm_event_counter(MX_RECORD *modbus_record,
					uint16_t *status_word,
					uint16_t *event_count );

MX_API mx_status_type mx_modbus_write_multiple_coils( MX_RECORD *modbus_record,
					unsigned long starting_address,
					unsigned long num_coils,
					uint8_t *coil_value_bit_array );

MX_API mx_status_type mx_modbus_write_multiple_registers(
					MX_RECORD *modbus_record,
					unsigned long starting_address,
					unsigned long num_registers,
					uint16_t *register_value_array );

MX_API mx_status_type mx_modbus_mask_write_register( MX_RECORD *modbus_record,
					unsigned long register_address,
					uint16_t and_mask,
					uint16_t or_mask );

MX_API mx_status_type mx_modbus_read_write_multiple_registers(
					MX_RECORD *modbus_record,
					unsigned long read_starting_address,
					unsigned long num_registers_to_read,
					uint16_t *read_register_array,
					unsigned long write_starting_address,
					unsigned long num_registers_to_write,
					uint16_t *write_register_array );

MX_API mx_status_type mx_modbus_read_fifo_queue( MX_RECORD *modbus_record,
					unsigned long fifo_pointer_address,
					unsigned long max_fifo_values,
					uint16_t *fifo_value_array,
					unsigned long *num_fifo_values_read );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MODBUS_H__ */

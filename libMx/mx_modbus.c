/*
 * Name:    mx_modbus.c
 *
 * Purpose: MX function library of generic MODBUS operations.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_modbus.h"
#include "mx_driver.h"

MX_EXPORT mx_status_type
mx_modbus_get_pointers( MX_RECORD *modbus_record,
			MX_MODBUS **modbus,
			MX_MODBUS_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_modbus_get_pointers()";

	if ( modbus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The modbus_record pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( modbus_record->mx_class != MXI_MODBUS ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not a MODBUS interface.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			modbus_record->name, calling_fname,
			modbus_record->mx_superclass,
			modbus_record->mx_class,
			modbus_record->mx_type );
	}

	if ( modbus != (MX_MODBUS **) NULL ) {
		*modbus = (MX_MODBUS *) (modbus_record->record_class_struct);

		if ( *modbus == (MX_MODBUS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MODBUS pointer for record '%s' passed by '%s' is NULL.",
				modbus_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_MODBUS_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_MODBUS_FUNCTION_LIST *)
				(modbus_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_MODBUS_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_MODBUS_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				modbus_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_modbus_compute_response_length( MX_RECORD *modbus_record,
					uint8_t *receive_buffer,
					size_t *response_length )
{
	static const char fname[] = "mx_modbus_compute_response_length()";

	uint8_t function_code, byte_count;

	function_code = receive_buffer[0];

	if ( function_code >= MXF_MOD_EXCEPTION ) {
		*response_length = 2;

		return MX_SUCCESSFUL_RESULT;
	}

	byte_count = receive_buffer[1];

	switch( function_code ) {
	case MXF_MOD_READ_COILS:
	case MXF_MOD_READ_DISCRETE_INPUTS:
	case MXF_MOD_READ_HOLDING_REGISTERS:
	case MXF_MOD_READ_INPUT_REGISTERS:
	case MXF_MOD_GET_COMM_EVENT_LOG:
	case MXF_MOD_READ_WRITE_MULTIPLE_REGISTERS:
		*response_length = byte_count + 2;
		break;
	case MXF_MOD_WRITE_SINGLE_COIL:
	case MXF_MOD_WRITE_SINGLE_REGISTER:
	case MXF_MOD_GET_COMM_EVENT_COUNTER:
	case MXF_MOD_WRITE_MULTIPLE_COILS:
	case MXF_MOD_WRITE_MULTIPLE_REGISTERS:
		*response_length = 5;
		break;
	case MXF_MOD_READ_EXCEPTION_STATUS:
		*response_length = 2;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported MODBUS function code %#x seen in receive buffer "
		"for MODBUS serial RTU interface '%s'.",
			function_code, modbus_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_modbus_command( MX_RECORD *modbus_record,
			uint8_t *request_buffer,
			size_t request_length,
			uint8_t *response_buffer,
			size_t response_buffer_length,
			size_t *actual_response_length )
{
	static const char fname[] = "mx_modbus_receive_response()";

	MX_MODBUS *modbus;
	MX_MODBUS_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_MODBUS * );
	mx_status_type mx_status;

	mx_status = mx_modbus_get_pointers( modbus_record,
					&modbus, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( request_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'request_buffer' pointer passed was NULL." );
	}

	if ( response_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'response_buffer' pointer passed was NULL." );
	}

	fptr = fl_ptr->command;

	if ( fptr == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The 'command' function for the '%s' driver "
		"used by MODBUS record '%s' is not yet implemented.",
			mx_get_driver_name( modbus_record ),
			modbus_record->name );
	}

	modbus->request_pointer = request_buffer;

	modbus->request_length = request_length;

	modbus->response_pointer = response_buffer;

	modbus->response_buffer_length = response_buffer_length;

	mx_status = (*fptr)( modbus );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( actual_response_length != NULL ) {
		*actual_response_length = modbus->actual_response_length;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --- */

MX_EXPORT mx_status_type
mx_modbus_read_coils( MX_RECORD *modbus_record,
			unsigned long starting_address,
			unsigned long num_coils,
			uint8_t *coil_value_array )
{
	static const char fname[] = "mx_modbus_read_coils()";

	uint8_t request_buffer[5];
	uint8_t response_buffer[1 + MXU_MODBUS_MAX_COILS_READABLE / 8];
	size_t actual_response_length, actual_data_bytes;
	size_t num_coil_bytes, bytes_to_copy;
	mx_status_type mx_status;

	if ( coil_value_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'coil_value_array' argument passed was NULL." );
	}

	if ( num_coils > MXU_MODBUS_MAX_COILS_READABLE ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested number of coils (%lu) for MODBUS record '%s' "
		"exceeds the maximum allowed value of %d.",
			num_coils, modbus_record->name,
			MXU_MODBUS_MAX_COILS_READABLE );
	}

	if (( num_coils % 8 ) == 0) {
		num_coil_bytes = num_coils / 8;
	} else {
		num_coil_bytes = 1 + num_coils / 8;
	}

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ] = MXF_MOD_READ_COILS;

	request_buffer[ MXF_MOD_PDU_DATA ]   = ( starting_address >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+1 ] = starting_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+2 ] = ( num_coils >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+3 ] = num_coils & 0xff;

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 5,
				response_buffer, sizeof(response_buffer),
				&actual_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	actual_data_bytes = response_buffer[ MXF_MOD_PDU_DATA ];

	if ( actual_data_bytes > num_coil_bytes ) {
		bytes_to_copy = num_coil_bytes;
	} else {
		bytes_to_copy = actual_data_bytes;
	}

	memcpy( coil_value_array,
		&response_buffer[ MXF_MOD_PDU_DATA+1 ],
		bytes_to_copy );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_modbus_read_discrete_inputs( MX_RECORD *modbus_record,
			unsigned long starting_address,
			unsigned long num_inputs,
			uint8_t *input_value_array )
{
	static const char fname[] = "mx_modbus_read_discrete_inputs()";

	uint8_t request_buffer[5];
	uint8_t response_buffer[1 + MXU_MODBUS_MAX_COILS_READABLE / 8];
	size_t actual_response_length, actual_data_bytes;
	size_t num_input_bytes, bytes_to_copy;
	mx_status_type mx_status;

	if ( input_value_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'input_value_array' argument passed was NULL." );
	}

	if ( num_inputs > MXU_MODBUS_MAX_COILS_READABLE ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested number of inputs (%lu) for MODBUS record '%s' "
		"exceeds the maximum allowed value of %d.",
			num_inputs, modbus_record->name,
			MXU_MODBUS_MAX_COILS_READABLE );
	}

	if (( num_inputs % 8 ) == 0) {
		num_input_bytes = num_inputs / 8;
	} else {
		num_input_bytes = 1 + num_inputs / 8;
	}

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_READ_DISCRETE_INPUTS;

	request_buffer[ MXF_MOD_PDU_DATA ]   = ( starting_address >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+1 ] = starting_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+2 ] = ( num_inputs >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+3 ] = num_inputs & 0xff;

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 5,
				response_buffer, sizeof(response_buffer),
				&actual_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	actual_data_bytes = response_buffer[ MXF_MOD_PDU_DATA ];

	if ( actual_data_bytes > num_input_bytes ) {
		bytes_to_copy = num_input_bytes;
	} else {
		bytes_to_copy = actual_data_bytes;
	}

	memcpy( input_value_array,
		&response_buffer[ MXF_MOD_PDU_DATA+1 ],
		bytes_to_copy );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_modbus_read_holding_registers( MX_RECORD *modbus_record,
			unsigned long starting_address,
			unsigned long num_registers,
			uint16_t *register_value_array )
{
	static const char fname[] = "mx_modbus_read_holding_registers()";

	uint8_t request_buffer[5];
	uint8_t response_buffer[ 2*MXU_MODBUS_MAX_REGISTERS_READABLE ];
	size_t i, j, actual_response_length, actual_register_values;
	size_t num_register_bytes, values_to_copy;
	mx_status_type mx_status;

	if ( register_value_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'register_value_array' argument passed was NULL." );
	}

	if ( num_registers > MXU_MODBUS_MAX_REGISTERS_READABLE ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested number of registers (%lu) for MODBUS record '%s' "
		"exceeds the maximum allowed value of %d.",
			num_registers, modbus_record->name,
			MXU_MODBUS_MAX_COILS_READABLE );
	}

	num_register_bytes = 2 * num_registers;

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_READ_HOLDING_REGISTERS;

	request_buffer[ MXF_MOD_PDU_DATA ]   = ( starting_address >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+1 ] = starting_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+2 ] = ( num_registers >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+3 ] = num_registers & 0xff;

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 5,
				response_buffer, sizeof(response_buffer),
				&actual_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	actual_register_values = response_buffer[ MXF_MOD_PDU_DATA ] / 2;

	if ( actual_register_values > num_registers ) {
		values_to_copy = num_registers;
	} else {
		values_to_copy = actual_register_values;
	}

	for ( i = 0; i < values_to_copy; i++ ) {

		j = MXF_MOD_PDU_DATA + 1 + 2*i;

		register_value_array[i] = ( response_buffer[j] << 8 );

		register_value_array[i] |= response_buffer[j+1];
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_modbus_read_input_registers( MX_RECORD *modbus_record,
			unsigned long starting_address,
			unsigned long num_registers,
			uint16_t *register_value_array )
{
	static const char fname[] = "mx_modbus_read_input_registers()";

	uint8_t request_buffer[5];
	uint8_t response_buffer[ 2*MXU_MODBUS_MAX_REGISTERS_READABLE ];
	size_t i, j, actual_response_length, actual_register_values;
	size_t num_register_bytes, values_to_copy;
	mx_status_type mx_status;

	if ( register_value_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'register_value_array' argument passed was NULL." );
	}

	if ( num_registers > MXU_MODBUS_MAX_REGISTERS_READABLE ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested number of registers (%lu) for MODBUS record '%s' "
		"exceeds the maximum allowed value of %d.",
			num_registers, modbus_record->name,
			MXU_MODBUS_MAX_COILS_READABLE );
	}

	num_register_bytes = 2 * num_registers;

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_READ_INPUT_REGISTERS;

	request_buffer[ MXF_MOD_PDU_DATA ]   = ( starting_address >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+1 ] = starting_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+2 ] = ( num_registers >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+3 ] = num_registers & 0xff;

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 5,
				response_buffer, sizeof(response_buffer),
				&actual_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	actual_register_values = response_buffer[ MXF_MOD_PDU_DATA ] / 2;

	if ( actual_register_values > num_registers ) {
		values_to_copy = num_registers;
	} else {
		values_to_copy = actual_register_values;
	}

#if 0
	MX_DEBUG(-2,("%s: num_registers = %d, actual_register_values = %d",
		fname, num_registers, actual_register_values));

	MX_DEBUG(-2,("%s: values_to_copy = %d", fname, values_to_copy));

	{
		int n;

		for ( n = 0; n < actual_response_length; n++ ) {
			MX_DEBUG(-2,("%s: response_buffer[%d] = %#02x",
			fname, n, response_buffer[n]));
		}
	}
#endif

	for ( i = 0; i < values_to_copy; i++ ) {

		j = MXF_MOD_PDU_DATA + 1 + 2*i;

		register_value_array[i] = ( response_buffer[j] << 8 );

		register_value_array[i] |= response_buffer[j+1];
#if 0
		MX_DEBUG(-2,
	("%s: register[%d] = %#04x, response[%d] = %#02x, response[%d] = %#02x",
	 	fname, i, register_value_array[i], j, response_buffer[j],
		j+1, response_buffer[j+1]));
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_modbus_write_single_coil( MX_RECORD *modbus_record,
			unsigned long output_address,
			unsigned long coil_value )
{
	uint8_t request_buffer[5];
	uint8_t response_buffer[5];
	mx_status_type mx_status;

	if ( coil_value != 0 ) {
		coil_value = 0xff;
	}

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ] = MXF_MOD_WRITE_SINGLE_COIL;

	request_buffer[ MXF_MOD_PDU_DATA ]   = ( output_address >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+1 ] = output_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+2 ] = coil_value;
	request_buffer[ MXF_MOD_PDU_DATA+3 ] = 0x00;

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 5,
				response_buffer, sizeof(response_buffer),
				NULL );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_modbus_write_single_register( MX_RECORD *modbus_record,
			unsigned long output_address,
			uint16_t register_value )
{
	uint8_t request_buffer[5];
	uint8_t response_buffer[5];
	mx_status_type mx_status;

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_WRITE_SINGLE_REGISTER;

	request_buffer[ MXF_MOD_PDU_DATA ]   = ( output_address >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+1 ] = output_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+2 ] = ( register_value >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+3 ] = register_value & 0xff;

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 5,
				response_buffer, sizeof(response_buffer),
				NULL );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_modbus_read_exception_status( MX_RECORD *modbus_record,
			uint8_t *exception_status )
{
	static const char fname[] = "mx_modbus_read_exception_status()";

	uint8_t request_buffer[1];
	uint8_t response_buffer[2];
	size_t actual_response_length;
	mx_status_type mx_status;

	if ( exception_status == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'exception_status' argument passed was NULL." );
	}

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_READ_EXCEPTION_STATUS;

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 1,
				response_buffer, sizeof(response_buffer),
				&actual_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( actual_response_length < 2 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The MODBUS command 'read_exception_status' for "
		"MODBUS interface '%s' returned only %d bytes when it "
		"should have returned 2 bytes.",
			modbus_record->name, (int) actual_response_length );
	}

	*exception_status = response_buffer[ MXF_MOD_PDU_DATA ];

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_modbus_get_comm_event_counter( MX_RECORD *modbus_record,
			uint16_t *status_word,
			uint16_t *event_count )
{
	static const char fname[] = "mx_modbus_get_comm_event_counter()";

	uint8_t request_buffer[1];
	uint8_t response_buffer[5];
	size_t actual_response_length;
	mx_status_type mx_status;

	if ( status_word == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'status_word' argument passed was NULL." );
	}
	if ( event_count == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'event_count' argument passed was NULL." );
	}

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_GET_COMM_EVENT_COUNTER;

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 1,
				response_buffer, sizeof(response_buffer),
				&actual_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( actual_response_length < 5 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The MODBUS command 'get_comm_event_counter' for "
		"MODBUS interface '%s' returned only %d bytes when it "
		"should have returned 5 bytes.",
			modbus_record->name, (int) actual_response_length );
	}

	*status_word = ( response_buffer[ MXF_MOD_PDU_DATA ] ) << 8;

	*status_word |= response_buffer[ MXF_MOD_PDU_DATA+1 ];

	*event_count = ( response_buffer[ MXF_MOD_PDU_DATA+2 ] ) << 8;

	*event_count |= response_buffer[ MXF_MOD_PDU_DATA+3 ];

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_modbus_write_multiple_coils( MX_RECORD *modbus_record,
			unsigned long starting_address,
			unsigned long num_coils,
			uint8_t *coil_value_array )
{
	static const char fname[] = "mx_modbus_write_multiple_coils()";

	uint8_t request_buffer[7 + MXU_MODBUS_MAX_COILS_WRITEABLE / 8];
	uint8_t response_buffer[5];
	size_t num_coil_bytes;
	mx_status_type mx_status;

	if ( coil_value_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'coil_value_array' argument passed was NULL." );
	}

	if ( num_coils > MXU_MODBUS_MAX_COILS_WRITEABLE ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested number of coils (%lu) for MODBUS record '%s' "
		"exceeds the maximum allowed value of %d.",
			num_coils, modbus_record->name,
			MXU_MODBUS_MAX_COILS_WRITEABLE );
	}

	if (( num_coils % 8 ) == 0) {
		num_coil_bytes = num_coils / 8;
	} else {
		num_coil_bytes = 1 + num_coils / 8;
	}

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_WRITE_MULTIPLE_COILS;

	request_buffer[ MXF_MOD_PDU_DATA ]   = ( starting_address >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+1 ] = starting_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+2 ] = ( num_coils >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+3 ] = num_coils & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+4 ] = num_coil_bytes;

	memcpy( &request_buffer[ MXF_MOD_PDU_DATA+5 ],
		coil_value_array,
		num_coil_bytes );

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 6 + num_coil_bytes,
				response_buffer, sizeof(response_buffer),
				NULL );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_modbus_write_multiple_registers( MX_RECORD *modbus_record,
			unsigned long starting_address,
			unsigned long num_registers,
			uint16_t *register_value_array )
{
	static const char fname[] = "mx_modbus_write_multiple_registers()";

	uint8_t request_buffer[6 + 2*MXU_MODBUS_MAX_REGISTERS_WRITEABLE];
	uint8_t response_buffer[5];
	size_t i, j, num_register_bytes;
	mx_status_type mx_status;

	if ( register_value_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'register_value_array' argument passed was NULL." );
	}

	if ( num_registers > MXU_MODBUS_MAX_REGISTERS_WRITEABLE ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested number of registers (%lu) for MODBUS record '%s' "
		"exceeds the maximum allowed value of %d.",
			num_registers, modbus_record->name,
			MXU_MODBUS_MAX_REGISTERS_WRITEABLE );
	}

	num_register_bytes = 2 * num_registers;

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_WRITE_MULTIPLE_REGISTERS;

	request_buffer[ MXF_MOD_PDU_DATA ]   = ( starting_address >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+1 ] = starting_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+2 ] = ( num_registers >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+3 ] = num_registers & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+4 ] = num_register_bytes;

	for ( i = 0; i < num_registers; i++ ) {

		j = MXF_MOD_PDU_DATA + 5 + 2*i;

		request_buffer[j] = ( register_value_array[i] >> 8 ) & 0xff;

		request_buffer[j+1] = register_value_array[i] & 0xff;
	}

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 6 + num_register_bytes,
				response_buffer, sizeof(response_buffer),
				NULL );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_modbus_mask_write_register( MX_RECORD *modbus_record,
			unsigned long register_address,
			uint16_t and_mask,
			uint16_t or_mask )
{
	uint8_t request_buffer[7];
	uint8_t response_buffer[7];
	mx_status_type mx_status;

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_MASK_WRITE_REGISTER;

	request_buffer[ MXF_MOD_PDU_DATA ]   = ( register_address >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+1 ] = register_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+2 ] = ( and_mask >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+3 ] = and_mask & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+4 ] = ( or_mask >> 8 ) & 0xff;
	request_buffer[ MXF_MOD_PDU_DATA+5 ] = or_mask & 0xff;

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 7,
				response_buffer, sizeof(response_buffer),
				NULL );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_modbus_read_write_multiple_registers( MX_RECORD *modbus_record,
			unsigned long read_starting_address,
			unsigned long num_registers_to_read,
			uint16_t *read_register_array,
			unsigned long write_starting_address,
			unsigned long num_registers_to_write,
			uint16_t *write_register_array )
{
	static const char fname[] = "mx_modbus_read_write_multiple_registers()";

	uint8_t request_buffer[10+2*MXU_MODBUS_MAX_READ_WRITE_REGISTERS];
	uint8_t response_buffer[2+2*MXU_MODBUS_MAX_READ_WRITE_REGISTERS];
	size_t i, j, num_register_bytes_to_read, num_register_bytes_to_write;
	size_t actual_response_length;
	mx_status_type mx_status;

	if ( read_register_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'read_register_array' argument passed was NULL." );
	}
	if ( write_register_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'write_register_array' argument passed was NULL." );
	}

	if ( num_registers_to_read > MXU_MODBUS_MAX_READ_WRITE_REGISTERS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested number of registers (%lu) to read for "
		"MODBUS record '%s' exceeds the maximum allowed value of %d.",
			num_registers_to_read, modbus_record->name,
			MXU_MODBUS_MAX_READ_WRITE_REGISTERS );
	}
	if ( num_registers_to_write > MXU_MODBUS_MAX_READ_WRITE_REGISTERS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested number of registers (%lu) to write for "
		"MODBUS record '%s' exceeds the maximum allowed value of %d.",
			num_registers_to_write, modbus_record->name,
			MXU_MODBUS_MAX_READ_WRITE_REGISTERS );
	}

	num_register_bytes_to_write = 2 * num_registers_to_write;

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_WRITE_MULTIPLE_REGISTERS;

	/* --- */

	request_buffer[ MXF_MOD_PDU_DATA ]
				= ( read_starting_address >> 8 ) & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+1 ] = read_starting_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+2 ]
				= ( num_registers_to_read >> 8 ) & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+3 ] = num_registers_to_read & 0xff;

	/* --- */

	request_buffer[ MXF_MOD_PDU_DATA+4 ]
				= ( write_starting_address >> 8 ) & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+5 ] = write_starting_address & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+6 ]
				= ( num_registers_to_write >> 8 ) & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+7 ] = num_registers_to_write & 0xff;

	/* --- */

	request_buffer[ MXF_MOD_PDU_DATA+8 ] = num_register_bytes_to_write;

	for ( i = 0; i < num_registers_to_write; i++ ) {

		j = MXF_MOD_PDU_DATA + 9 + 2*i;

		request_buffer[j] = ( write_register_array[i] >> 8 ) & 0xff;

		request_buffer[j+1] = write_register_array[i] & 0xff;
	}

	mx_status = mx_modbus_command( modbus_record,
				request_buffer,
				10 + num_register_bytes_to_write,
				response_buffer, sizeof(response_buffer),
				&actual_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_register_bytes_to_read = response_buffer[ MXF_MOD_PDU_DATA ];

	for ( i = 0; i < num_registers_to_read; i++ ) {

		j = MXF_MOD_PDU_DATA + 1 + 2*i;

		read_register_array[i] = response_buffer[j] << 8;

		read_register_array[i] |= response_buffer[j+1];
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_modbus_read_fifo_queue( MX_RECORD *modbus_record,
			unsigned long fifo_pointer_address,
			unsigned long max_fifo_values,
			uint16_t *fifo_value_array,
			unsigned long *num_fifo_values_read )
{
	static const char fname[] = "mx_modbus_read_fifo_queue()";

	uint8_t request_buffer[3];
	uint8_t response_buffer[ 5+MXU_MODBUS_MAX_FIFO_VALUES ];
	size_t i, j, actual_response_length, values_to_copy;
	mx_status_type mx_status;

	if ( fifo_value_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'fifo_value_array' argument passed was NULL." );
	}
	if ( num_fifo_values_read == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'num_fifo_values_read' argument passed was NULL." );
	}

	if ( max_fifo_values > MXU_MODBUS_MAX_FIFO_VALUES ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested number of fifo values (%lu) for MODBUS record '%s' "
		"exceeds the maximum allowed value of %d.",
			max_fifo_values, modbus_record->name,
			MXU_MODBUS_MAX_FIFO_VALUES );
	}

	request_buffer[ MXF_MOD_PDU_FUNCTION_CODE ]
					= MXF_MOD_READ_INPUT_REGISTERS;

	request_buffer[ MXF_MOD_PDU_DATA ]
					= ( fifo_pointer_address >> 8 ) & 0xff;

	request_buffer[ MXF_MOD_PDU_DATA+1 ] = fifo_pointer_address & 0xff;

	mx_status = mx_modbus_command( modbus_record,
				request_buffer, 3,
				response_buffer, sizeof(response_buffer),
				&actual_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*num_fifo_values_read = ( response_buffer[ MXF_MOD_PDU_DATA+3 ] << 8 );

	*num_fifo_values_read |= response_buffer[ MXF_MOD_PDU_DATA+4 ];

	if ( (*num_fifo_values_read) > max_fifo_values ) {
		values_to_copy = max_fifo_values;
	} else {
		values_to_copy = *num_fifo_values_read;
	}

	for ( i = 0; i < values_to_copy; i++ ) {

		j = MXF_MOD_PDU_DATA + 5 + 2*i;

		fifo_value_array[i] = response_buffer[j] << 8;

		fifo_value_array[i] |= response_buffer[j+1];
	}

	return MX_SUCCESSFUL_RESULT;
}


/*
 * Name:    mx_net.c
 *
 * Purpose: Client side part of MX network protocol.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_array.h"
#include "mx_bit.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_handle.h"
#include "mx_driver.h"

#include "n_tcpip.h"
#include "n_unix.h"

#if ( HAVE_TCPIP == 0 )
#     define htonl(x) (x)
#     define ntohl(x) (x)
#endif

#define NETWORK_DEBUG		FALSE

#define NETWORK_DEBUG_TIMING	FALSE

#if NETWORK_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_receive_message( MX_RECORD *server_record,
				unsigned long buffer_length, void *buffer )
{
	static const char fname[] = "mx_network_receive_message()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_NETWORK_SERVER *, unsigned long, void * );
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Record pointer passed was NULL." );
	}
	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Message buffer pointer passed for record '%s' was NULL.",
			server_record->name );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
			(server_record->class_specific_function_list);

	if ( function_list == (MX_NETWORK_SERVER_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SERVER_FUNCTION_LIST pointer for server record '%s' is NULL.",
			server_record->name );
	}

	fptr = function_list->receive_message;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"receive_message function pointer for server record '%s' is NULL.",
			server_record->name );
	}

	mx_status = ( *fptr ) ( server, buffer_length, buffer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_network_send_message( MX_RECORD *server_record, void *buffer )
{
	static const char fname[] = "mx_network_send_message()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_NETWORK_SERVER *, void * );
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Record pointer passed was NULL." );
	}
	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Message buffer pointer passed for record '%s' was NULL.",
			server_record->name );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
			(server_record->class_specific_function_list);

	if ( function_list == (MX_NETWORK_SERVER_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SERVER_FUNCTION_LIST pointer for server record '%s' is NULL.",
			server_record->name );
	}

	fptr = function_list->send_message;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"send_message function pointer for server record '%s' is NULL.",
			server_record->name );
	}

	mx_status = ( *fptr ) ( server, buffer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_network_connection_is_up( MX_RECORD *server_record, int *connection_is_up )
{
	static const char fname[] = "mx_network_connection_is_up()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_NETWORK_SERVER *, int * );
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}
	if ( connection_is_up == (int *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"connection_is_up pointer is NULL." );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
			(server_record->class_specific_function_list);

	if ( function_list == (MX_NETWORK_SERVER_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SERVER_FUNCTION_LIST pointer for server record '%s' is NULL.",
			server_record->name );
	}

	fptr = function_list->connection_is_up;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"connection_is_up function pointer for server record '%s' is NULL.",
			server_record->name );
	}

	mx_status = ( *fptr ) ( server, connection_is_up );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_network_reconnect_if_down( MX_RECORD *server_record )
{
	static const char fname[] = "mx_network_reconnect_if_down()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_NETWORK_SERVER * );
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Record pointer passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
			(server_record->class_specific_function_list);

	if ( function_list == (MX_NETWORK_SERVER_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SERVER_FUNCTION_LIST pointer for server record '%s' is NULL.",
			server_record->name );
	}

	fptr = function_list->reconnect_if_down;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"reconnect_if_down function pointer for server record '%s' is NULL.",
			server_record->name );
	}

	mx_status = ( *fptr ) ( server );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_network_mark_handles_as_invalid( MX_RECORD *server_record )
{
	static const char fname[] = "mx_network_mark_handles_as_invalid()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_FIELD *nf;
	unsigned long i;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Record pointer passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	/* See if the handles have already been marked invalid. */

	if ( server->network_handles_are_valid == FALSE )
		return MX_SUCCESSFUL_RESULT;

	/* Otherwise, walk through the list and mark the handles as invalid. */

	for ( i = 0; i < server->num_network_fields; i++ ) {
		nf = server->network_field_array[i];

		if ( nf != (MX_NETWORK_FIELD *) NULL ) {
			MX_DEBUG( 2,
		("%s: Marking handle as invalid, network_field '%s' (%ld,%ld)",
				fname, nf->nfname,
				nf->record_handle, nf->field_handle));

			nf->record_handle = MX_ILLEGAL_HANDLE;
			nf->field_handle = MX_ILLEGAL_HANDLE;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_network_display_message_buffer( void *buffer_ptr )
{
	static const char fname[] = "mx_network_display_message_buffer()";

	uint32_t *header;
	unsigned char *buffer, *message;
	uint32_t magic_number, header_length, message_length;
	uint32_t message_type, status_code;
	unsigned long i;
	unsigned char c;

	if ( buffer_ptr == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"Buffer pointer passed was NULL." );
		return;
	}

	buffer = buffer_ptr;
	header = buffer_ptr;

	magic_number   = mx_ntohl( header[ MX_NETWORK_MAGIC ] );
	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code    = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	message = buffer + header_length;

	/**** Print out the header. ****/

	/* Header magic number */

	mx_info( "Magic number:     0x%8lx   (%2x %2x %2x %2x)",
	  (long) magic_number, buffer[0], buffer[1], buffer[2], buffer[3] );

	if ( magic_number != MX_NETWORK_MAGIC_VALUE ) {
		(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Invalid magic number 0x%lx in network buffer header.",
			(long) magic_number );
		return;
	}

	/* Header length */

	i = MX_NETWORK_HEADER_LENGTH * sizeof( uint32_t );

	mx_info( "Header length:  %12lu   (%2x %2x %2x %2x)",
		(unsigned long) header_length,
			buffer[i+0], buffer[i+1],
			buffer[i+2], buffer[i+3] );

	if ( header_length < MX_NETWORK_HEADER_LENGTH_VALUE ) {
		mx_info( "*** Header length %lu was unexpectedly short. ***",
			(unsigned long) header_length );
		return;
	}

	/* Message length */

	i = MX_NETWORK_MESSAGE_LENGTH * sizeof( uint32_t );

	mx_info( "Message length: %12lu   (%2x %2x %2x %2x)",
		(unsigned long) message_length,
			buffer[i+0], buffer[i+1],
			buffer[i+2], buffer[i+3] );

	/* Message type */

	i = MX_NETWORK_MESSAGE_TYPE * sizeof( uint32_t );

	mx_info( "Message type:   %12lu   (%2x %2x %2x %2x)",
		(unsigned long) message_type,
			buffer[i+0], buffer[i+1],
			buffer[i+2], buffer[i+3] );

	/* Status code */

	i = MX_NETWORK_STATUS_CODE * sizeof( uint32_t );

	mx_info( "Status code:    %12lu   (%2x %2x %2x %2x)",
		(unsigned long) status_code,
			buffer[i+0], buffer[i+1],
			buffer[i+2], buffer[i+3] );

	/* Remaining part of header. */

	if ( header_length > MX_NETWORK_HEADER_LENGTH_VALUE ) {
		mx_info( "Remaining header bytes:" );

		for ( i += sizeof( uint32_t ); i < header_length; i++ ) {
			c = buffer[i];

			if ( isprint(c) ) {
				mx_info("  buffer[%lu]  = %02x (%c)",
						i, c, c);
			} else {
				mx_info("  buffer[%lu]  = %02x", i, c);
			}
		}
	}

	/* Print out the message body. */

	mx_info( "Message contents:" );

	for ( i = 0; i < message_length; i++ ) {
		c = message[i];

		if ( isprint(c) ) {
			mx_info("  message[%lu] = %02x (%c)", i, c, c);
		} else {
			mx_info("  message[%lu] = %02x", i, c);
		}
	}
	return;
}

/* ====================================================================== */

static mx_status_type
mx_network_field_get_parameters( MX_RECORD *server_record,
			MX_RECORD_FIELD *local_field,
			long *datatype,
			long *num_dimensions,
			long **dimension_array,
			size_t **data_element_size_array,
			MX_NETWORK_SERVER **server,
			MX_NETWORK_SERVER_FUNCTION_LIST **function_list,
			const char *calling_fname )
{
	static const char fname[] = "mx_network_field_get_parameters()";

	if ( server_record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record pointer passed to '%s' is NULL.",
			calling_fname );
	}
	if ( local_field == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"local_field pointer passed to '%s' is NULL.",
			calling_fname );
	}
	*datatype = local_field->datatype;

	switch( *datatype ) {
	case MXFT_STRING:
	case MXFT_CHAR:
	case MXFT_UCHAR:
	case MXFT_SHORT:
	case MXFT_USHORT:
	case MXFT_BOOL:
	case MXFT_LONG:
	case MXFT_ULONG:
	case MXFT_INT64:
	case MXFT_UINT64:
	case MXFT_FLOAT:
	case MXFT_DOUBLE:
	case MXFT_HEX:
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Data type %ld passed to '%s' is not a legal network datatype.",
			*datatype, calling_fname );
	}

	*num_dimensions = local_field->num_dimensions;
	*dimension_array = local_field->dimension;
	*data_element_size_array = local_field->data_element_size;

	*server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);
	*function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
				(server_record->class_specific_function_list);

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_need_to_get_network_handle( MX_NETWORK_FIELD *nf,
				int *new_handle_needed )
{
	static const char fname[] = "mx_need_to_get_network_handle()";

	MX_NETWORK_SERVER *network_server;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}
	if ( new_handle_needed == (int *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The new_handle_needed pointer passed was NULL." );
	}
	if ( nf->server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The server_record pointer for the MX_NETWORK_FIELD "
		"pointer passed is NULL." );
	}

	network_server = (MX_NETWORK_SERVER *)
			nf->server_record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			nf->server_record->name );
	}

	if ( network_server->server_supports_network_handles == FALSE ) {
		*new_handle_needed = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	if ( ( nf->record_handle == MX_ILLEGAL_HANDLE )
	  || ( nf->field_handle == MX_ILLEGAL_HANDLE ) )
	{
		*new_handle_needed = TRUE;
	} else {
		*new_handle_needed = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_field_init( MX_NETWORK_FIELD *nf,
			MX_RECORD *server_record,
			char *name_format, ... )
{
	static const char fname[] = "mx_network_field_init()";

	va_list args;
	char buffer[1000];

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}
#if 0
	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The server_record pointer passed was NULL." );
	}
#endif
	if ( name_format == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The network field name format string pointer passed was NULL." );
	}

	va_start(args, name_format);
	vsprintf(buffer, name_format, args);
	va_end(args);

	/* Save the network field name. */

	strlcpy( nf->nfname, buffer, MXU_RECORD_FIELD_NAME_LENGTH );

	MX_DEBUG( 2,("%s invoked for network field '%s:%s'",
		fname, server_record->name, nf->nfname));

	/* Initialize the data structures. */

	nf->server_record = server_record;

	nf->record_handle = MX_ILLEGAL_HANDLE;
	nf->field_handle = MX_ILLEGAL_HANDLE;

	if ( nf->server_record != (MX_RECORD *) NULL ) {
		/* Add this network field to the server's list of fields. */

		MX_NETWORK_SERVER *network_server;
		unsigned long num_fields, block_size, new_array_size;

		network_server = (MX_NETWORK_SERVER *)
					nf->server_record->record_class_struct;

		if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_NETWORK_SERVER pointer for record '%s' is NULL.",
				nf->server_record->name );
		}

		num_fields = network_server->num_network_fields;

		if ( (num_fields != 0 )
		  && (network_server->network_field_array == NULL) )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Network server record '%s' says that it has %lu "
			"network record fields, but the network_field_array "
			"pointer is NULL.", nf->server_record->name,
				num_fields );
		}

		block_size = network_server->network_field_array_block_size;

		if ( block_size == 0 ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The network_field_array_block_size for record '%s' "
			"is zero.  This should not happen.",
				nf->server_record->name );
		}

		if ( num_fields == 0 ) {
			network_server->network_field_array =
				(MX_NETWORK_FIELD **)
				malloc( block_size * sizeof(MX_NETWORK_FIELD));

			if ( network_server->network_field_array == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to allocate "
				"an array of %lu MX_NETWORK_FIELD pointers.",
					block_size );
			}
		} else
		if (( num_fields % block_size ) == 0) {
			new_array_size = num_fields + block_size;

			network_server->network_field_array =
				(MX_NETWORK_FIELD **)
				realloc( network_server->network_field_array,
				    new_array_size * sizeof(MX_NETWORK_FIELD));

			if ( network_server->network_field_array == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to resize "
				"an array of %lu MX_NETWORK_FIELD pointers.",
					new_array_size );
			}
		}

		network_server->network_field_array[ num_fields ] = nf;

		network_server->num_network_fields++;

		MX_DEBUG( 2,("%s: Added network field (%lu) '%s,%s'",
			fname, num_fields, nf->server_record->name,
			nf->nfname ));
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_get_array( MX_NETWORK_FIELD *nf,
		long datatype,
		long num_dimensions,
		long *dimension,
		void *value_ptr )
{
	static const char fname[] = "mx_get_array()";

	int new_handle_needed;
	mx_status_type mx_status;

#if 1
	if ( (num_dimensions == 1) && ( datatype == MXFT_CHAR ) ) {
		long local_array_size;
		char *dest_ptr;

		MX_DEBUG(-2,("%s: MARKER #1", fname));
		MX_DEBUG(-2,("%s: datatype = %ld", fname, datatype));
		MX_DEBUG(-2,("%s: num_dimensions = %ld",
					fname, num_dimensions));
		MX_DEBUG(-2,("%s: dimension[0] = %ld", fname, dimension[0]));

		local_array_size = dimension[0] * 1;

		MX_DEBUG(-2,("%s: local_array_size = %ld",
			fname, local_array_size));

		dest_ptr = value_ptr;

		MX_DEBUG(-2,("%s: dest_ptr = %p", fname, dest_ptr));

		MX_DEBUG(-2,("%s: About to read dest_ptr[%ld]",
			fname, local_array_size - 1));
		MX_DEBUG(-2,("%s: dest_ptr[%ld] = %u",
			fname, local_array_size - 1,
			dest_ptr[local_array_size-1] ));
	}
#endif

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}

	mx_status = mx_need_to_get_network_handle( nf, &new_handle_needed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( new_handle_needed ) {
		mx_status = mx_network_field_connect( nf );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if 1
	if ( (num_dimensions == 1) && ( datatype == MXFT_CHAR ) ) {
		long local_array_size;
		char *dest_ptr;

		MX_DEBUG(-2,("%s: MARKER #2", fname));
		MX_DEBUG(-2,("%s: nf name = '%s'", fname, nf->nfname));
		MX_DEBUG(-2,("%s: datatype = %ld", fname, datatype));
		MX_DEBUG(-2,("%s: num_dimensions = %ld",
					fname, num_dimensions));
		MX_DEBUG(-2,("%s: dimension[0] = %ld", fname, dimension[0]));

		local_array_size = dimension[0] * 1;

		MX_DEBUG(-2,("%s: local_array_size = %ld",
			fname, local_array_size));

		dest_ptr = value_ptr;

		MX_DEBUG(-2,("%s: dest_ptr = %p", fname, dest_ptr));

		MX_DEBUG(-2,("%s: About to read dest_ptr[%ld]",
			fname, local_array_size - 1));
		MX_DEBUG(-2,("%s: dest_ptr[%ld] = %u",
			fname, local_array_size - 1,
			dest_ptr[local_array_size-1] ));
	}
#endif

	mx_status = mx_internal_get_array( NULL, NULL, nf,
					datatype, num_dimensions, dimension,
					value_ptr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_put_array( MX_NETWORK_FIELD *nf,
		long datatype,
		long num_dimensions,
		long *dimension,
		void *value_ptr )
{
	static const char fname[] = "mx_put_array()";

	int new_handle_needed;
	mx_status_type mx_status;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}

	mx_status = mx_need_to_get_network_handle( nf, &new_handle_needed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( new_handle_needed ) {
		mx_status = mx_network_field_connect( nf );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_internal_put_array( NULL, NULL, nf,
					datatype, num_dimensions, dimension,
					value_ptr );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_internal_get_array( MX_RECORD *server_record,
		char *remote_record_field_name,
		MX_NETWORK_FIELD *nf,
		long datatype,
		long num_dimensions,
		long *dimension,
		void *value_ptr )
{
	static const char fname[] = "mx_internal_get_array()";

	MX_RECORD_FIELD local_temp_record_field;
	long *dimension_array;
	long local_dimension_array[MXU_FIELD_MAX_DIMENSIONS];
	size_t data_element_size[MXU_FIELD_MAX_DIMENSIONS];
	mx_status_type mx_status;

	if ( dimension != NULL ) {
		dimension_array = dimension;

	} else if ( num_dimensions == 0 ) {
		dimension_array = local_dimension_array;
		dimension_array[0] = 0;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"dimension array pointer is NULL, but num_dimensions (%ld) is greater than 0",
			num_dimensions );
	}

	/* Setting the first element of data_element_size to be zero causes
	 * mx_construct_temp_record_field() to use a default data element
	 * size array.
	 */

	data_element_size[0] = 0L;

	/* Setup a temporary record field to be used by
	 * mx_get_field_array().
	 */

	mx_status = mx_construct_temp_record_field( &local_temp_record_field,
			datatype, num_dimensions, dimension_array,
			data_element_size, value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	if (num_dimensions == 1) {
		long local_array_size;
		char *dest_ptr;

		MX_DEBUG(-2,
		("%s: dimension_array[0] = %ld, data_element_size[0] = %ld",
			fname, dimension_array[0], (long)data_element_size[0]));

		local_array_size = dimension_array[0] * data_element_size[0];

		MX_DEBUG(-2,("%s: local_array_size = %ld",
			fname, local_array_size));

		dest_ptr = value_ptr;

		MX_DEBUG(-2,("%s: dest_ptr = %p", fname, dest_ptr));

		MX_DEBUG(-2,("%s: About to read dest_ptr[%ld]",
			fname, local_array_size - 1));
		MX_DEBUG(-2,("%s: dest_ptr[%ld] = %u",
			fname, local_array_size - 1,
			dest_ptr[local_array_size-1] ));
	}
#endif

	/* Send the request to the server. */

	mx_status = mx_get_field_array( server_record,
			remote_record_field_name,
			nf,
			&local_temp_record_field,
			value_ptr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_internal_put_array( MX_RECORD *server_record,
		char *remote_record_field_name,
		MX_NETWORK_FIELD *nf,
		long datatype,
		long num_dimensions,
		long *dimension,
		void *value_ptr )
{
	static const char fname[] = "mx_internal_put_array()";

	MX_RECORD_FIELD local_temp_record_field;
	long *dimension_array;
	long local_dimension_array[MXU_FIELD_MAX_DIMENSIONS];
	size_t data_element_size[MXU_FIELD_MAX_DIMENSIONS];
	mx_status_type mx_status;

	if ( dimension != NULL ) {
		dimension_array = dimension;

	} else if ( num_dimensions == 0 ) {
		dimension_array = local_dimension_array;
		dimension_array[0] = 0;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"dimension array pointer is NULL, but num_dimensions (%ld) is greater than 0",
			num_dimensions );
	}

	/* Setting the first element of data_element_size to be zero causes
	 * mx_construct_temp_record_field() to use a default data element
	 * size array.
	 */

	data_element_size[0] = 0L;

	/* Setup a temporary record field to be used by
	 * mx_put_field_array().
	 */

	mx_status = mx_construct_temp_record_field( &local_temp_record_field,
			datatype, num_dimensions, dimension_array,
			data_element_size, value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the request to the server. */

	mx_status = mx_put_field_array( server_record,
			remote_record_field_name,
			nf,
			&local_temp_record_field,
			value_ptr );

	return mx_status;
}

/* ====================================================================== */

#define MXU_GET_PUT_ARRAY_ASCII_LOCATION_LENGTH \
	MXU_HOSTNAME_LENGTH + MXU_RECORD_FIELD_NAME_LENGTH + 80

static mx_status_type
mx_get_array_ascii_error_message( uint32_t status_code,
			char *server_name,
			char *record_field_name,
			char *error_message )
{
	char location[ MXU_GET_PUT_ARRAY_ASCII_LOCATION_LENGTH + 1 ];

	sprintf( location,
		"MX GET from server '%s', record field '%s'",
		server_name, record_field_name );

	return mx_error( (long) status_code, location, error_message );
}

static mx_status_type
mx_put_array_ascii_error_message( uint32_t status_code,
			char *server_name,
			char *record_field_name,
			char *error_message )
{
	char location[ MXU_GET_PUT_ARRAY_ASCII_LOCATION_LENGTH + 1 ];

	sprintf( location,
		"MX PUT to server '%s', record field '%s'",
		server_name, record_field_name );

	return mx_error( (long) status_code, location, error_message );
}

MX_EXPORT mx_status_type
mx_get_field_array( MX_RECORD *server_record,
			char *remote_record_field_name,
			MX_NETWORK_FIELD *nf,
			MX_RECORD_FIELD *local_field,
			void *value_ptr )
{
	static const char fname[] = "mx_get_field_array()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	MX_RECORD_FIELD_PARSE_STATUS temp_parse_status;
	mx_status_type ( *token_parser )
		(void *, char *, MX_RECORD *, MX_RECORD_FIELD *,
		MX_RECORD_FIELD_PARSE_STATUS *);
	long datatype, num_dimensions, *dimension_array;
	size_t *data_element_size_array;
	mx_bool_type array_is_dynamically_allocated;
	mx_bool_type use_network_handles;

	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header, *uint32_message;
	char *buffer;
	char *message;
	uint32_t header_length, message_length;
	uint32_t message_type, status_code;
	mx_status_type mx_status;
	char token_buffer[500];

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	static char separators[] = MX_RECORD_FIELD_SEPARATORS;

	use_network_handles = TRUE;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		use_network_handles = FALSE;

		/* We are not using a network handle. */

		if ( (server_record == (MX_RECORD *) NULL)
		  || (remote_record_field_name == (char *) NULL) )
		{
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"If the MX_NETWORK_FIELD argument is NULL, "
			"the server_record and remote_record_field_name "
			"arguments must both not be NULL." );
		}
	} else {
		/* In this case, we ignore the values that were passed
		 * in the server_record and remote_record_field_name
		 * arguments and use the values from the nf structure
		 * instead, since we always prefer to use the values
		 * from MX_NETWORK_FIELD structures.
		 */

		server_record = nf->server_record;
		remote_record_field_name = nf->nfname;
	}

	mx_status = mx_network_field_get_parameters(
			server_record, local_field,
			&datatype, &num_dimensions,
			&dimension_array, &data_element_size_array,
			&server, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( server->server_supports_network_handles == FALSE ) {
		use_network_handles = FALSE;
	}

	MX_DEBUG( 2,
	("%s: server_record = '%s', remote_record_field_name = '%s'",
		fname, server_record->name, remote_record_field_name));

	if ( local_field->flags & MXFF_VARARGS ) {
		array_is_dynamically_allocated = TRUE;
	} else {
		array_is_dynamically_allocated = FALSE;
	}

	mx_status = mx_network_reconnect_if_down( server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*********** Send a 'get array' command. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->uint32_buffer[0]);
	buffer = &(aligned_buffer->char_buffer[0]);

	header[MX_NETWORK_MAGIC] = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH]
				= mx_htonl( MX_NETWORK_HEADER_LENGTH_VALUE );
	header[MX_NETWORK_STATUS_CODE] = mx_htonl( MXE_SUCCESS );

	message = buffer + MX_NETWORK_HEADER_LENGTH_VALUE;

	uint32_message = header + MX_NETWORK_NUM_HEADER_VALUES;

	if ( use_network_handles == FALSE ) {

		/* Use ASCII record field name */

		header[MX_NETWORK_MESSAGE_TYPE]
				= mx_htonl( MX_NETMSG_GET_ARRAY_BY_NAME );

		strlcpy( message, remote_record_field_name,
				MXU_RECORD_FIELD_NAME_LENGTH );

		message_length = (uint32_t) ( strlen( message ) + 1 );
	} else {

		/* Use binary network handle */

		header[MX_NETWORK_MESSAGE_TYPE]
				= mx_htonl( MX_NETMSG_GET_ARRAY_BY_HANDLE );

		uint32_message[0] = mx_htonl( nf->record_handle );
		uint32_message[1] = mx_htonl( nf->field_handle );

		message_length = 2 * sizeof( uint32_t );
	}

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

#if NETWORK_DEBUG
	mx_network_display_message_buffer( buffer );
#endif

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( server_record, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************* Wait for the response. **************/

	mx_status = mx_network_receive_message( server_record,
				MX_NETWORK_MAXIMUM_MESSAGE_SIZE, buffer );

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	if ( use_network_handles == FALSE ) {
		MX_HRT_RESULTS( measurement, fname, remote_record_field_name );
	} else {
		MX_HRT_RESULTS( measurement, fname, "(%ld,%ld) %s",
				nf->record_handle, nf->field_handle,
				remote_record_field_name );
	}
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if NETWORK_DEBUG
	mx_network_display_message_buffer( buffer );
#endif

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code    = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type != mx_server_response(MX_NETMSG_GET_ARRAY_BY_NAME) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {
			switch( status_code ) {
			case MXE_SUCCESS:
				break;
			case MXE_BAD_HANDLE:
				MX_DEBUG( 2,
		("%s: MXE_BAD_HANDLE seen for server '%s', nfname = '%s'.",
				fname, nf->server_record->name, nf->nfname));

				nf->record_handle = MX_ILLEGAL_HANDLE;
				nf->field_handle = MX_ILLEGAL_HANDLE;

				/* Fall through to the default case. */
			default:
				return mx_get_array_ascii_error_message(
					status_code,
					server_record->name,
					remote_record_field_name,
					message );
			}
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_GET_ARRAY_ASCII_RESPONSE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

	message = buffer + header_length;

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the array data
	 * we wanted.
	 */

	switch( status_code ) {
	case MXE_SUCCESS:
		break;
	case MXE_BAD_HANDLE:
		MX_DEBUG( 2,
		("%s: MXE_BAD_HANDLE seen for server '%s', nfname = '%s'.",
			fname, nf->server_record->name, nf->nfname));

		nf->record_handle = MX_ILLEGAL_HANDLE;
		nf->field_handle = MX_ILLEGAL_HANDLE;

		/* Fall through to the default case. */
	default:
		return mx_get_array_ascii_error_message(
				status_code,
				server_record->name,
				remote_record_field_name,
				message );
	}

	/************ Parse the data that was returned. ***************/

	switch( server->data_format ) {
	case MX_NETWORK_DATAFMT_ASCII:

		/* The data was returned using the ASCII MX database format. */

		mx_status = mx_get_token_parser( datatype, &token_parser );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_initialize_parse_status( &temp_parse_status,
						message, separators );

		/* If this is a string field, figure out what the maximum
		 * string token length is.
		 */

		if ( datatype == MXFT_STRING ) {
			temp_parse_status.max_string_token_length =
				mx_get_max_string_token_length( local_field );
		} else {
			temp_parse_status.max_string_token_length = 0L;
		}

		/* Now we are ready to parse the tokens. */

		if ( (num_dimensions == 0) ||
			((datatype == MXFT_STRING) && (num_dimensions == 1)) )
		{
			mx_status = mx_get_next_record_token(
				&temp_parse_status,
				token_buffer, sizeof(token_buffer) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = ( *token_parser ) ( value_ptr,
					token_buffer, NULL, local_field,
					&temp_parse_status );
		} else {
			mx_status = mx_parse_array_description( value_ptr,
				num_dimensions - 1, NULL, local_field,
				&temp_parse_status, token_parser );
		}
		break;

	case MX_NETWORK_DATAFMT_RAW:
		mx_status = mx_copy_buffer_to_array( message, message_length,
				value_ptr, array_is_dynamically_allocated,
				datatype, num_dimensions,
				dimension_array, data_element_size_array,
				NULL,
				server->truncate_64bit_longs );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_error( mx_status.code, fname,
	"Buffer copy for MX_GET of parameter '%s' in server '%s' failed.",
				remote_record_field_name, server_record->name );
		}
		break;

	case MX_NETWORK_DATAFMT_XDR:
#if HAVE_XDR
		mx_status = mx_xdr_data_transfer( MX_XDR_DECODE,
				value_ptr, array_is_dynamically_allocated,
				datatype, num_dimensions,
				dimension_array, data_element_size_array,
				message, message_length, NULL );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_error( mx_status.code, fname,
	"Buffer copy for MX_GET of parameter '%s' in server '%s' failed.",
				remote_record_field_name, server_record->name );
		}
#else
		return mx_error( MXE_UNSUPPORTED, fname,
			"XDR network data format is not supported "
			"on this system." );
#endif
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized network data format type %lu was requested.",
			server->data_format );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_put_field_array( MX_RECORD *server_record,
			char *remote_record_field_name,
			MX_NETWORK_FIELD *nf,
			MX_RECORD_FIELD *local_field,
			void *value_ptr )
{
	static const char fname[] = "mx_put_field_array()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	mx_status_type ( *token_constructor )
		(void *, char *, size_t, MX_RECORD *, MX_RECORD_FIELD *);
	long datatype, num_dimensions, *dimension_array;
	size_t *data_element_size_array;
	mx_bool_type array_is_dynamically_allocated;
	mx_bool_type use_network_handles;

	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header, *uint32_message;
	char *buffer;
	char *message, *ptr;
	unsigned long i, ptr_address, remainder_value, gap_size;
	uint32_t header_length, message_length, message_type, status_code;
	size_t buffer_left, num_bytes_copied;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	use_network_handles = TRUE;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		use_network_handles = FALSE;

		/* We are not using a network handle. */

		if ( (server_record == (MX_RECORD *) NULL)
		  || (remote_record_field_name == (char *) NULL) )
		{
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"If the MX_NETWORK_FIELD argument is NULL, "
			"the server_record and remote_record_field_name "
			"arguments must both not be NULL." );
		}
	} else {
		/* In this case, we ignore the values that were passed
		 * in the server_record and remote_record_field_name
		 * arguments and use the values from the nf structure
		 * instead, since we always prefer to use the values
		 * from MX_NETWORK_FIELD structures.
		 */

		server_record = nf->server_record;
		remote_record_field_name = nf->nfname;
	}

	mx_status = mx_network_field_get_parameters(
			server_record, local_field,
			&datatype, &num_dimensions,
			&dimension_array, &data_element_size_array,
			&server, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( server->server_supports_network_handles == FALSE ) {
		use_network_handles = FALSE;
	}

	if ( local_field->flags & MXFF_VARARGS ) {
		array_is_dynamically_allocated = TRUE;
	} else {
		array_is_dynamically_allocated = FALSE;
	}

	mx_status = mx_network_reconnect_if_down( server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************ Construct a 'put array' command. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->uint32_buffer[0]);
	buffer = &(aligned_buffer->char_buffer[0]);

	header[MX_NETWORK_MAGIC] = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH]
				= mx_htonl( MX_NETWORK_HEADER_LENGTH_VALUE );
	header[MX_NETWORK_STATUS_CODE] = mx_htonl( MXE_SUCCESS );

	message = buffer + MX_NETWORK_HEADER_LENGTH_VALUE;

	uint32_message = header + MX_NETWORK_NUM_HEADER_VALUES;

	if ( use_network_handles == FALSE ) {

		/* Use ASCII record field name */

		header[MX_NETWORK_MESSAGE_TYPE]
				= mx_htonl( MX_NETMSG_PUT_ARRAY_BY_NAME );

		sprintf( message, "%*s", -MXU_RECORD_FIELD_NAME_LENGTH,
				remote_record_field_name );

		message_length = MXU_RECORD_FIELD_NAME_LENGTH;
	} else {

		/* Use binary network handle */

		header[MX_NETWORK_MESSAGE_TYPE]
				= mx_htonl( MX_NETMSG_PUT_ARRAY_BY_HANDLE );

		uint32_message[0] = mx_htonl( nf->record_handle );
		uint32_message[1] = mx_htonl( nf->field_handle );

		message_length = 2 * sizeof( uint32_t );
	}

	ptr = message + message_length;

	MX_DEBUG( 2,("%s: message = %p, ptr = %p, message_length = %lu",
		fname, message, ptr, (unsigned long) message_length));

	buffer_left = MX_NETWORK_MAXIMUM_MESSAGE_SIZE
			- MX_NETWORK_HEADER_LENGTH_VALUE - message_length;

	/* Construct the data to send. */

	switch( server->data_format ) {
	case MX_NETWORK_DATAFMT_ASCII:

		/* Send the data using the ASCII MX database format. */

		mx_status = mx_get_token_constructor( datatype,
						&token_constructor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (num_dimensions == 0)
		  || ((datatype == MXFT_STRING) && (num_dimensions == 1)) )
		{
			mx_status = (*token_constructor) ( value_ptr, ptr,
			    MX_NETWORK_MAXIMUM_MESSAGE_SIZE - ( ptr - buffer ),
			    NULL, local_field );
		} else {
			mx_status = mx_create_array_description( value_ptr, 
			    local_field->num_dimensions - 1, ptr,
			    MX_NETWORK_MAXIMUM_MESSAGE_SIZE - ( ptr - buffer ),
			    NULL, local_field, token_constructor );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* The extra 1 is for the '\0' byte at the end of the string. */

		message_length += 1 + strlen( ptr );

		break;

	case MX_NETWORK_DATAFMT_RAW:
		mx_status = mx_copy_array_to_buffer( value_ptr,
				array_is_dynamically_allocated,
				datatype, num_dimensions,
				dimension_array, data_element_size_array,
				ptr, buffer_left,
				&num_bytes_copied,
				server->truncate_64bit_longs );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_error( mx_status.code, fname,
	"Buffer copy for MX_PUT of parameter '%s' in server '%s' failed.",
				remote_record_field_name, server_record->name );
		}

		message_length += num_bytes_copied;
		break;

	case MX_NETWORK_DATAFMT_XDR:
#if HAVE_XDR
		/* The XDR data pointer 'ptr' must be aligned on a 4 byte
		 * address boundary for XDR data conversion to work correctly
		 * on all architectures.  If the pointer does not point to
		 * an address that is a multiple of 4 bytes, we move it to
		 * the next address that _is_ and fill the bytes inbetween
		 * with zeros.
		 */

		ptr_address = (unsigned long) ptr;

		remainder_value = ptr_address % 4;

		if ( remainder_value != 0 ) {
			gap_size = 4 - remainder_value;

			for ( i = 0; i < gap_size; i++ ) {
				ptr[i] = '\0';
			}

			ptr += gap_size;

			message_length += gap_size;
		}

		/* Now we are ready to do the XDR data conversion. */

		mx_status = mx_xdr_data_transfer( MX_XDR_ENCODE,
				value_ptr,
				array_is_dynamically_allocated,
				datatype, num_dimensions,
				dimension_array, data_element_size_array,
				ptr, buffer_left,
				&num_bytes_copied );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_error( mx_status.code, fname,
	"Buffer copy for MX_PUT of parameter '%s' in server '%s' failed.",
				remote_record_field_name, server_record->name );
		}

		message_length += num_bytes_copied;
#else
		return mx_error( MXE_UNSUPPORTED, fname,
			"XDR network data format is not supported "
			"on this system." );
#endif
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized network data format type %lu was requested.",
			server->data_format );
	}

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

	message_length += MX_NETWORK_HEADER_LENGTH_VALUE;

	MX_DEBUG( 2,("%s: message = '%s'", fname, message));

#if NETWORK_DEBUG
	mx_network_display_message_buffer( buffer );
#endif

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	/*************** Send the message. **************/

	mx_status = mx_network_send_message( server_record, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. ************/

	mx_status = mx_network_receive_message( server_record,
				MX_NETWORK_MAXIMUM_MESSAGE_SIZE, buffer );

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	if ( use_network_handles == FALSE ) {
		MX_HRT_RESULTS( measurement, fname, remote_record_field_name );
	} else {
		MX_HRT_RESULTS( measurement, fname, "(%ld,%ld) %s",
				nf->record_handle, nf->field_handle,
				remote_record_field_name );
	}
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if NETWORK_DEBUG
	mx_network_display_message_buffer( buffer );
#endif

        header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
        message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
        message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
        status_code = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

        if ( message_type != mx_server_response(MX_NETMSG_PUT_ARRAY_BY_NAME) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {
			return mx_put_array_ascii_error_message(
					status_code,
					server_record->name,
					remote_record_field_name,
					message );
		}

                return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_PUT_ARRAY_ASCII_RESPONSE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
        }

        message = buffer + header_length;

        /* If the remote command failed, the message field will include
         * the text of the error message rather than the array data
         * we wanted.
         */

        if ( status_code != MXE_SUCCESS ) {
		return mx_put_array_ascii_error_message(
				status_code,
				server_record->name,
				remote_record_field_name,
				message );
        } else {
		return MX_SUCCESSFUL_RESULT;
	}
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_field_connect( MX_NETWORK_FIELD *nf )
{
	static const char fname[] = "mx_network_field_connect()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header;
	char *buffer, *message;
	uint32_t *message_uint32_array;
	uint32_t header_length, message_length;
	uint32_t message_type, status_code;
	uint32_t header_length_in_32bit_words;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s invoked, nf->nfname = '%s'", fname, nf->nfname));

	if ( nf->server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The server_record pointer for network field (%ld,%ld) '%s' is NULL.",
			nf->record_handle, nf->field_handle, nf->nfname );
	}

	server = (MX_NETWORK_SERVER *) nf->server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			nf->server_record->name );
	}

	mx_status = mx_network_reconnect_if_down( nf->server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************ Send the 'get network handle' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->uint32_buffer[0]);
	buffer = &(aligned_buffer->char_buffer[0]);

	header[MX_NETWORK_MAGIC] = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH]
				= mx_htonl( MX_NETWORK_HEADER_LENGTH_VALUE );
	header[MX_NETWORK_MESSAGE_TYPE] = mx_htonl( MX_NETMSG_GET_NETWORK_HANDLE );
	header[MX_NETWORK_STATUS_CODE] = mx_htonl( MXE_SUCCESS );

	message = buffer + MX_NETWORK_HEADER_LENGTH_VALUE;

	/* Copy the network field name to the outgoing message buffer. */

	strlcpy( message, nf->nfname, MXU_RECORD_FIELD_NAME_LENGTH );

	message_length = (uint32_t) ( strlen( message ) + 1 );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( nf->server_record, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_receive_message( nf->server_record,
				MX_NETWORK_MAXIMUM_MESSAGE_SIZE, buffer );

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, nf->nfname );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type
		!= mx_server_response( MX_NETMSG_GET_NETWORK_HANDLE ) )
	{
		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {

			if ( status_code == MXE_NOT_YET_IMPLEMENTED ) {

				/* If we received an MXE_NOT_YET_IMPLEMENTED
				 * status code, this means that the remote
				 * MX server is not a recent enough version
				 * to implement network handles.  We must 
				 * record this fact so that later GET and
				 * PUT operations will not attempt to use
				 * network handle support.
				 */

				MX_DEBUG( 2,
	("%s: network handle support not implemented for MX server '%s'.",
					fname, server->record->name ));

				server->server_supports_network_handles = FALSE;

				return MX_SUCCESSFUL_RESULT;
			} else {
				/* If we got some other status code, just
				 * return the error message.
				 */

				return mx_error( (long)status_code, fname,
					"%s", message );
			}
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_GET_NETWORK_HANDLE_RESPONSE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

#if 0
	mx_network_display_message_buffer( buffer );
#endif

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long)status_code, fname, message );
	}

	if ( message_length < (2 * sizeof(uint32_t)) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Incomplete message received.  Message length = %ld",
			(long) message_length );
	}

	/***** Get the network handle out of what we were sent. *****/

	header_length_in_32bit_words = (uint32_t)
				( header_length / sizeof(uint32_t) );

	message_uint32_array = header + header_length_in_32bit_words;

	nf->record_handle = 
		(long) mx_ntohl( (unsigned long) message_uint32_array[0] );

	nf->field_handle  = 
		(long) mx_ntohl( (unsigned long) message_uint32_array[1] );

#if NETWORK_DEBUG_TIMING
	MX_DEBUG(-2,
	("%s: server '%s', field '%s' = (%ld,%ld)",
		fname, nf->server_record->name, nf->nfname,
		nf->record_handle, nf->field_handle ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_get_field_type( MX_RECORD *server_record, 
			char *remote_record_field_name,
			long max_dimensions,
			long *datatype,
			long *num_dimensions,
			long *dimension_array )
{
	static const char fname[] = "mx_get_field_type()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header;
	char *buffer, *message;
	uint32_t *message_uint32_array;
	uint32_t header_length, message_length;
	uint32_t message_type, status_code;
	uint32_t i, expected_message_length;
	uint32_t header_length_in_32bit_words;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,("%s invoked, remote_record_field_name = '%s'",
			fname, remote_record_field_name));

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	mx_status = mx_network_reconnect_if_down( server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************ Send the 'get field type' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->uint32_buffer[0]);
	buffer = &(aligned_buffer->char_buffer[0]);

	header[MX_NETWORK_MAGIC] = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH]
				= mx_htonl( MX_NETWORK_HEADER_LENGTH_VALUE );
	header[MX_NETWORK_MESSAGE_TYPE] = mx_htonl( MX_NETMSG_GET_FIELD_TYPE );
	header[MX_NETWORK_STATUS_CODE] = mx_htonl( MXE_SUCCESS );

	message = buffer + MX_NETWORK_HEADER_LENGTH_VALUE;

	strlcpy( message, remote_record_field_name,
					MXU_RECORD_FIELD_NAME_LENGTH );

	message_length = (uint32_t) ( strlen( message ) + 1 );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( server_record, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_receive_message( server_record,
				MX_NETWORK_MAXIMUM_MESSAGE_SIZE, buffer );

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, remote_record_field_name );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type != mx_server_response( MX_NETMSG_GET_FIELD_TYPE ) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {
			return mx_error( (long)status_code, fname,
				"%s", message );
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_GET_FIELD_TYPE_RESPONSE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

#if 0
	mx_network_display_message_buffer( buffer );
#endif

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long)status_code, fname, message );
	}

	if ( message_length < (2 * sizeof(uint32_t)) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Incomplete message received.  Message length = %ld",
			(long) message_length );
	}

	/***** Get the type information out of what we were sent. *****/

	header_length_in_32bit_words = (uint32_t)
				( header_length / sizeof(uint32_t) );

	message_uint32_array = header + header_length_in_32bit_words;

	*datatype = (long) mx_ntohl( (unsigned long) message_uint32_array[0] );
	*num_dimensions
		= (long) mx_ntohl( (unsigned long) message_uint32_array[1] );

	expected_message_length = (uint32_t)
				( sizeof(long) * ( (*num_dimensions) + 2 ) );

	if ( message_length < expected_message_length ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Incomplete dimension array received.  %ld dimension values were received, "
"but %ld dimension values were expected.",
			(long) (message_length - 2), *num_dimensions );
	}
	if ( *num_dimensions < max_dimensions ) {
		max_dimensions = *num_dimensions;
	}
	if ( max_dimensions <= 0 ) {
		dimension_array[0] = (long)
			mx_ntohl( (unsigned long) message_uint32_array[2] );
	}
	for ( i = 0; i < max_dimensions; i++ ) {
		dimension_array[i] = (long)
			mx_ntohl( (unsigned long) message_uint32_array[i+2] );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

/* mx_set_client_info() sends information to the server about the client
 * for debugging and status reporting purposes _only_.  No authentication
 * is done, so this mechanism cannot and _must_ _not_ be used for access
 * control.  Someday we may have SSL/Kerberos/whatever-based authentication,
 * but this function does not do any of that kind of thing just now.
 */

MX_EXPORT mx_status_type
mx_set_client_info( MX_RECORD *server_record,
			char *username,
			char *program_name )
{
	static const char fname[] = "mx_set_client_info()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header;
	char *buffer, *message, *ptr;
	uint32_t header_length, message_length;
	uint32_t message_type, status_code;
	int connection_is_up;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,("%s invoked, username = '%s', program_name = '%s'",
			fname, username, program_name));

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	/* If the network connection is not currently up for some reason,
	 * do not try to send the client information.
	 */

	mx_status = mx_network_connection_is_up( server_record,
						&connection_is_up );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: server '%s', connection_is_up = %d",
		fname, server_record->name, connection_is_up));

	if ( connection_is_up == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/************ Send the 'set client info' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->uint32_buffer[0]);
	buffer = &(aligned_buffer->char_buffer[0]);

	header[MX_NETWORK_MAGIC] = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH]
				= mx_htonl( MX_NETWORK_HEADER_LENGTH_VALUE );
	header[MX_NETWORK_MESSAGE_TYPE] = mx_htonl( MX_NETMSG_SET_CLIENT_INFO );
	header[MX_NETWORK_STATUS_CODE] = mx_htonl( MXE_SUCCESS );

	message = buffer + MX_NETWORK_HEADER_LENGTH_VALUE;

	strlcpy( message, username, MXU_USERNAME_LENGTH );

	strncat( message, " ", 1 );

	strncat( message, program_name, MXU_PROGRAM_NAME_LENGTH );

	strncat( message, " ", 1 );

	message_length = (uint32_t) strlen( message );

	ptr = message + message_length;

	sprintf( ptr, "%lu", mx_process_id() );

	MX_DEBUG( 2,("%s: message = '%s'", fname, message));

	message_length = (uint32_t) ( strlen( message ) + 1 );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( server_record, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_receive_message( server_record,
				MX_NETWORK_MAXIMUM_MESSAGE_SIZE, buffer );

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type != mx_server_response( MX_NETMSG_SET_CLIENT_INFO ) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {
			return mx_error( (long)status_code, fname,
				"%s", message );
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_SET_CLIENT_INFO_RESPONSE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

#if 0
	mx_network_display_message_buffer( buffer );
#endif

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long)status_code, fname, message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_get_option( MX_RECORD *server_record,
			unsigned long option_number,
			unsigned long *option_value )
{
	static const char fname[] = "mx_network_get_option()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header, *uint32_message;
	char *buffer, *message;
	uint32_t header_length, message_length;
	uint32_t message_type, status_code;
	uint32_t header_length_in_32bit_words;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,("%s invoked, option_number = '%#lx'",
			fname, option_number));

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}
	if ( option_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"option_value pointer passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	/************ Send the 'get option' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->uint32_buffer[0]);
	buffer = &(aligned_buffer->char_buffer[0]);

	header[MX_NETWORK_MAGIC] = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH]
				= mx_htonl( MX_NETWORK_HEADER_LENGTH_VALUE );
	header[MX_NETWORK_MESSAGE_TYPE] = mx_htonl( MX_NETMSG_GET_OPTION );
	header[MX_NETWORK_STATUS_CODE] = mx_htonl( MXE_SUCCESS );

	message = buffer + MX_NETWORK_HEADER_LENGTH_VALUE;
	uint32_message = header + MX_NETWORK_NUM_HEADER_VALUES;

	uint32_message[0] = mx_htonl( option_number );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( sizeof(uint32_t) );

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( server_record, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_receive_message( server_record,
				MX_NETWORK_MAXIMUM_MESSAGE_SIZE, buffer );

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type != mx_server_response( MX_NETMSG_GET_OPTION ) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {

			if ( status_code == MXE_NOT_YET_IMPLEMENTED ) {

				/* The remote server is an old server that
				 * does not implement the 'get option'
				 * message.
				 */

				return mx_error_quiet((long) status_code, fname,
	"The MX 'get option' message type is not implemented by server '%s'.",
					server_record->name );
			} else {
				/* Some other error occurred. */

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Unexpected server error for MX server '%s'.  "
	"Server error was '%s -> %s'", server_record->name,
				mx_status_code_string((long) status_code ),
					message );
			}
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_GET_OPTION_RESPONSE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

#if 0
	mx_network_display_message_buffer( buffer );
#endif

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long)status_code, fname, message );
	}

	if ( message_length < sizeof(uint32_t) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Incomplete message received.  Message length = %ld",
			(long) message_length );
	}

	header_length_in_32bit_words = (uint32_t)
				( header_length / sizeof(uint32_t) );

	uint32_message = header + header_length_in_32bit_words;

	*option_value = mx_ntohl( uint32_message[0] );

	MX_DEBUG( 2,("%s invoked, *option_value = '%#lx'",
			fname, *option_value));

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_set_option( MX_RECORD *server_record,
			unsigned long option_number,
			unsigned long option_value )
{
	static const char fname[] = "mx_network_set_option()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header, *uint32_message;
	char *buffer, *message;
	uint32_t header_length, message_length;
	uint32_t message_type, status_code;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,
		("%s invoked, option_number = '%#lx', option_value = '%#lx'",
			fname, option_number, option_value));

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	/************ Send the 'set option' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->uint32_buffer[0]);
	buffer = &(aligned_buffer->char_buffer[0]);

	header[MX_NETWORK_MAGIC] = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH]
				= mx_htonl( MX_NETWORK_HEADER_LENGTH_VALUE );
	header[MX_NETWORK_MESSAGE_TYPE] = mx_htonl( MX_NETMSG_SET_OPTION );
	header[MX_NETWORK_STATUS_CODE] = mx_htonl( MXE_SUCCESS );

	message = buffer + MX_NETWORK_HEADER_LENGTH_VALUE;
	uint32_message = header + MX_NETWORK_NUM_HEADER_VALUES;

	uint32_message[0] = mx_htonl( option_number );
	uint32_message[1] = mx_htonl( option_value );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( 2 * sizeof(uint32_t) );

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( server_record, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_receive_message( server_record,
				MX_NETWORK_MAXIMUM_MESSAGE_SIZE, buffer );

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type != mx_server_response( MX_NETMSG_SET_OPTION ) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {

			if ( status_code == MXE_NOT_YET_IMPLEMENTED ) {

				/* The remote server is an old server that
				 * does not implement the 'set option'
				 * message.
				 */

				return mx_error_quiet((long) status_code, fname,
	"The MX 'set option' message type is not implemented by server '%s'.",
					server_record->name );
			} else {
				/* Some other error occurred. */

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Unexpected server error for MX server '%s'.  "
	"Server error was '%s -> %s'", server_record->name,
				mx_status_code_string((long) status_code ),
					message );
			}
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_SET_OPTION_RESPONSE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

#if 0
	mx_network_display_message_buffer( buffer );
#endif

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long)status_code, fname, message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_request_data_format( MX_RECORD *server_record,
				unsigned long requested_format )
{
	static const char fname[] = "mx_network_request_data_format()";

	MX_NETWORK_SERVER *server;
	unsigned long local_native_data_format, remote_native_data_format;
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	/* If the caller has not requested automatic data format negotiation,
	 * just attempt to set the requested format and fail if it is not
	 * available.
	 */

	if ( requested_format != MX_NETWORK_DATAFMT_NEGOTIATE ) {

		mx_status = mx_network_set_option( server_record,
			MX_NETWORK_OPTION_DATAFMT, requested_format );

		if ( mx_status.code == MXE_SUCCESS ) {
			server->data_format = requested_format;

			return MX_SUCCESSFUL_RESULT;
		} else
		if ( ( mx_status.code == MXE_NOT_YET_IMPLEMENTED )
		  && ( requested_format != MX_NETWORK_DATAFMT_ASCII ) )
		{
			/* The server does not support anything other
			 * than ASCII format.
			 */

			server->data_format = MX_NETWORK_DATAFMT_ASCII;

			return mx_error( mx_status.code, fname,
			"MX server '%s' only supports ASCII data format.",
				server_record->name );
		} else {
			return mx_status;
		}
	}

	/* We have been requested to perform automatic data format negotiation.
	 * To do this, we must first get the local native data format.
	 */

	local_native_data_format = mx_native_data_format();

	/* Find out the native data format of the remote server. */

	mx_status = mx_network_get_option( server_record,
			MX_NETWORK_OPTION_NATIVE_DATAFMT,
			&remote_native_data_format );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_NOT_YET_IMPLEMENTED:

		/* The server is not new enough to have 'get option' and
		 * 'set option' implemented in it, so we have to settle
		 * for ASCII data format.
		 */

		MX_DEBUG( 2,
			("%s: Server '%s' only supports ASCII network format.",
	 		fname, server_record->name ));

		server->data_format = MX_NETWORK_DATAFMT_ASCII;
		
		/* Cannot do any further negotiation, so we just return now. */

		return MX_SUCCESSFUL_RESULT;
	default:
		return mx_status;
	}

	MX_DEBUG( 2,
    ("%s: local_native_data_format = %#lx, remote_native_data_format = %#lx",
     		fname, local_native_data_format, remote_native_data_format));

	/* Try to select a binary data format. */

	if ( local_native_data_format == remote_native_data_format ) {

		/* The local and remote data formats are the same, so
		 * we should try raw format.
		 */

		requested_format = MX_NETWORK_DATAFMT_RAW;
	} else {
		/* Otherwise, we should use XDR format. */

		requested_format = MX_NETWORK_DATAFMT_XDR;
	}

	mx_status = mx_network_set_option( server_record,
			MX_NETWORK_OPTION_DATAFMT, requested_format );

	switch ( mx_status.code ) {
	case MXE_SUCCESS:
		/* If the 'set option' command succeeded, we are done. */

		MX_DEBUG( 2,("%s: binary data format successfully selected.",
			fname));

		server->data_format = requested_format;

		return MX_SUCCESSFUL_RESULT;
	case MXE_NOT_YET_IMPLEMENTED:
		/* The server does not implement 'set option'. */

		MX_DEBUG( 2,("%s: Server '%s' does not support 'set option'.",
			fname, server_record->name));

		server->data_format = MX_NETWORK_DATAFMT_ASCII;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Selecting a binary data format failed, so try ASCII format. */

	mx_status = mx_network_set_option( server_record,
			MX_NETWORK_OPTION_DATAFMT, MX_NETWORK_DATAFMT_ASCII );

	/* Even if selecting ASCII format failed, we have no alternative
	 * to assuming ASCII format is in use.  So we make this assumption
	 * and hope for the best.
	 */

	server->data_format = MX_NETWORK_DATAFMT_ASCII;

	if ( mx_status.code == MXE_SUCCESS ) {
		MX_DEBUG( 2,("%s: ASCII data format successfully selected.",
				fname));
	} else {
		MX_DEBUG( 2,("%s: Assuming ASCII data format is in use.",
				fname));
	}

	return MX_SUCCESSFUL_RESULT;
}


/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_request_64bit_longs( MX_RECORD *server_record )
{
	static const char fname[] = "mx_network_request_64bit_longs()";

	MX_NETWORK_SERVER *server;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	MX_DEBUG( 2,("%s: requesting 64-bit longs for server '%s'",
		fname, server_record->name ));

#if ( MX_WORDSIZE != 64 )
	server->truncate_64bit_longs = FALSE;

	return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The client computer is not a 64-bit computer, so it "
		"cannot request 64-bit longs for network communication." );

#else   /* MX_WORDSIZE == 64 */

	server->truncate_64bit_longs = TRUE;

	switch( server->data_format ) {
	case MX_NETWORK_DATAFMT_RAW:
		/* This is the only supported case. */

		break;
	case MX_NETWORK_DATAFMT_XDR:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Network communication with server '%s' is using XDR data "
		"format which does not support 64-bit network longs.",
			server_record->name );

	case MX_NETWORK_DATAFMT_ASCII:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Network communication with server '%s' is using ASCII data "
		"format which does not support 64-bit network longs.",
			server_record->name );

	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The network communication data structures for server '%s' "
		"claim that they are using unrecognized data format %lu.  "
		"This should _never_ happen and should be reported to "
		"the software developers for MX.",
			server_record->name, server->data_format );
	}

	if ( server->remote_mx_version < 1002000 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The remote server cannot support 64-bit network longs since "
		"it is running MX version %ld.%ld.%ld.  The oldest version "
		"of MX that supports 64-bit network longs is MX 1.2.0.",
			server->remote_mx_version / 1000000,
			( server->remote_mx_version % 1000000 ) / 1000,
			server->remote_mx_version % 1000 );
	}

	mx_status = mx_network_set_option( server_record,
			MX_NETWORK_OPTION_64BIT_LONG, TRUE );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		server->truncate_64bit_longs = FALSE;
		break;
	default:
		server->truncate_64bit_longs = TRUE;

		return mx_status;
	}

	MX_DEBUG( 2,("%s: server->truncate_64bit_longs = %d",
		fname, server->truncate_64bit_longs));

	return MX_SUCCESSFUL_RESULT;

#endif  /* MX_WORDSIZE == 64 */
}

/* ====================================================================== */

/* The most general form of an MX network field address is as follows:
 *
 *	server_name@server_arguments:record_name.field_name
 *
 * where server_name and server_arguments are optional.
 *
 * Examples:
 *
 *      localhost@9727:mx_database.mx_version
 *      myserver:theta.position
 *      192.168.1.1:omega.positive_limit_hit
 *      edge_energy.value
 */

MX_EXPORT mx_status_type
mx_parse_network_field_id( char *network_field_id,
		char *server_name, size_t max_server_name_length,
		char *server_arguments, size_t max_server_arguments_length,
		char *record_name, size_t max_record_name_length,
		char *field_name, size_t max_field_name_length )
{
	static const char fname[] = "mx_parse_network_field_id()";

	char *server_name_ptr, *server_arguments_ptr;
	char *record_name_ptr, *field_name_ptr;
	char *at_sign_ptr, *colon_ptr, *period_ptr;
	size_t server_name_length, server_arguments_length;
	size_t record_name_length, field_name_length;

	if ( (network_field_id == NULL) || (server_name == NULL)
	  || (server_arguments == NULL) || (record_name == NULL)
	  || (field_name == NULL) )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One of the arguments passed to this function was NULL." );
	}

	server_name_length = 0;
	server_arguments_length = 0;
	record_name_length = 0;
	field_name_length = 0;

	/* Look for the first '@' character and the first ':' character
	 * that follows the '@' character.
	 */

	at_sign_ptr = strchr( network_field_id, '@' );

	if ( at_sign_ptr == NULL ) {
		colon_ptr = strchr( network_field_id, ':' );
	} else {
		colon_ptr = strchr( at_sign_ptr, ':' );
	}

	/* In addition, we look for the _last_ '.' character in
	 * the network_field_id.
	 */

	period_ptr = strrchr( network_field_id, '.' );

	if ( period_ptr == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The period '.' character that is supposed to separate "
		"the field name from the record name was not found in the "
		"line containing '%s' in the MX update configuration file.",
			network_field_id );
	}

	field_name_ptr = period_ptr + 1;

	field_name_length = strlen( field_name_ptr ) + 1;

	/* We do several different things depending on whether the
	 * at_sign_ptr and/or the colon_ptr are NULL.
	 */

	server_name_ptr = NULL;		server_name_length = 0;
	server_arguments_ptr = NULL;	server_arguments_length = 0;
	record_name_ptr = NULL;		record_name_length = 0;

	if ( at_sign_ptr == NULL ) {
		if ( colon_ptr == NULL ) {
			/* Everything before the last period is the
			 * record name.
			 */

			MX_DEBUG( 2,("%s: CASE #11", fname));

			record_name_ptr = network_field_id;
			record_name_length = period_ptr - network_field_id + 1;
		} else {
			/* Everything before the first colon is the
			 * server name.
			 */

			MX_DEBUG( 2,("%s: CASE #12", fname));

			server_name_ptr = network_field_id;
			server_name_length = colon_ptr - network_field_id + 1;

			record_name_ptr = colon_ptr + 1;
			record_name_length = period_ptr - record_name_ptr + 1;
		}
	} else {
		/* at_sign_ptr is _not_ NULL */

		if ( colon_ptr == NULL ) {
			/* A name with an '@' character in it, is required
			 * to have a ':' character somewhere after it.  
			 * Therefore, this particular combination is
			 * illegal.
			 */

			MX_DEBUG( 2,("%s: CASE #21", fname));

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The '@' character was used in network field id '%s' "
			"without a ':' character somewhere after it.",
				network_field_id );
		} else {
			/* at_sign_ptr and colon_ptr are both not NULL. */

			MX_DEBUG( 2,("%s: CASE #22", fname));

			server_name_ptr = network_field_id;
			server_name_length = at_sign_ptr - network_field_id + 1;

			server_arguments_ptr = at_sign_ptr + 1;
			server_arguments_length =
					colon_ptr - server_arguments_ptr + 1;

			record_name_ptr = colon_ptr + 1;
			record_name_length = period_ptr - record_name_ptr + 1;
		}
	}

	MX_DEBUG( 2,("%s: #1 server_name_ptr = '%s', server_name_length = %ld",
		fname, server_name_ptr, (long) server_name_length));

	MX_DEBUG( 2,
	("%s: #1 server_arguments_ptr = '%s', server_arguments_length = %ld",
		fname, server_arguments_ptr, (long) server_arguments_length));

	MX_DEBUG( 2,("%s: #1 record_name_ptr = '%s', record_name_length = %ld",
		fname, record_name_ptr, (long) record_name_length));

	MX_DEBUG( 2,("%s: #1 field_name_ptr = '%s', field_name_length = %ld",
		fname, field_name_ptr, (long) field_name_length));

	/* Make sure that the name lengths do not exceed the maximums
	 * specified by the calling routine.
	 */

	if ( server_name_length > max_server_name_length )
		server_name_length = max_server_name_length;

	if ( server_arguments_length > max_server_arguments_length )
		server_arguments_length = max_server_arguments_length;

	if ( record_name_length > max_record_name_length )
		record_name_length = max_record_name_length;

	if ( field_name_length > max_field_name_length )
		field_name_length = max_field_name_length;

	/* Finally, copy the strings to the caller's buffers. */

	if ( server_name_ptr != NULL ) {
		strlcpy( server_name, server_name_ptr, server_name_length );
	} else {
		strlcpy( server_name, "localhost", max_server_name_length );
	}

	if ( server_arguments_ptr != NULL ) {
		strlcpy( server_arguments, server_arguments_ptr,
						server_arguments_length );
	} else {
		strlcpy( server_arguments, "9727",
						max_server_arguments_length );
	}

	if ( record_name_ptr != NULL ) {
		strlcpy( record_name, record_name_ptr, record_name_length );
	} else {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Somehow when we got to here, record_name_ptr was NULL.  "
		"This should not be able to happen." );
	}

	if ( field_name_ptr != NULL ) {
		strlcpy( field_name, field_name_ptr, field_name_length );
	} else {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Somehow when we got to here, field_name_ptr was NULL.  "
		"This should not be able to happen." );
	}

	MX_DEBUG( 2,("%s: #2 server_name = '%s', server_name_length = %ld",
		fname, server_name, (long) server_name_length));

	MX_DEBUG( 2,
	("%s: #2 server_arguments = '%s', server_arguments_length = %ld",
		fname, server_arguments, (long) server_arguments_length));

	MX_DEBUG( 2,("%s: #2 record_name = '%s', record_name_length = %ld",
		fname, record_name, (long) record_name_length));

	MX_DEBUG( 2,("%s: #2 field_name = '%s', field_name_length = %ld",
		fname, field_name, (long) field_name_length));

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_get_mx_server_record( MX_RECORD *record_list,
			char *server_name,
			char *server_arguments,
			MX_RECORD **server_record )
{
	static const char fname[] = "mx_get_mx_server_record()";

	static const char number_digits[] = "0123456789";

	MX_RECORD *current_record, *new_record;
	MX_TCPIP_SERVER *tcpip_server;
	MX_UNIX_SERVER *unix_server;
	int port_number;
	long server_type;
	size_t arguments_length, strspn_length;
	char description[200];
	char format[100];
	mx_status_type mx_status;

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The record_list pointer passed was NULL." );
	}
	if ( server_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The server_name pointer passed was NULL." );
	}
	if ( server_arguments == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The server_arguments pointer passed was NULL." );
	}
	if ( server_record == (MX_RECORD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The server_record pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s: server_name = '%s', server_arguments = '%s'",
			fname, server_name, server_arguments));

	/* Figure out what kind of server we are looking for.
	 *
	 * We assume that if 'server_arguments' contains only the
	 * number digits 0-9, then it is referring to a TCP port 
	 * number.  Otherwise, we assume that it is a Unix domain
	 * socket.
	 */

	port_number = atoi( server_arguments );

	MX_DEBUG( 2,("%s: port_number = %d, server_arguments = '%s'",
			 	fname, port_number, server_arguments));


	arguments_length = strlen( server_arguments );

	strspn_length = strspn( server_arguments, number_digits );

	if ( strspn_length == arguments_length ) {
		server_type = MXN_NET_TCPIP;

		if ( strlen( server_name ) == 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The server name passed for the server "
			"is of zero length." );
		}
	} else {
		server_type = MXN_NET_UNIX;
	}

	MX_DEBUG( 2,("%s: server_type = %ld", fname, server_type));

	/* Be sure to start at the beginning of the record list. */

	record_list = record_list->list_head;

	current_record = record_list->next_record;

	/* Loop through the record list looking for a matching record. */

	while ( current_record != record_list ) {

	    if ( server_type == current_record->mx_type ) {
		/* We found a driver type match, so examine this
		 * record further.
		 */

		MX_DEBUG( 2,("%s: driver match for record '%s'",
			fname, current_record->name));

		switch( server_type ) {
		case MXN_NET_TCPIP:
		    tcpip_server = (MX_TCPIP_SERVER *)
					current_record->record_type_struct;

		    if ( (strcmp(server_name, tcpip_server->hostname) == 0)
		      && (port_number == tcpip_server->port) )
		    {
			/* We have found a matching record, so return it. */

			*server_record = current_record;
			return MX_SUCCESSFUL_RESULT;
		    }
		    break;

		case MXN_NET_UNIX:
		    unix_server = (MX_UNIX_SERVER *)
					current_record->record_type_struct;

		    if (strcmp(server_arguments, unix_server->pathname) == 0 )
		    {
			/* We have found a matching record, so return it. */

			*server_record = current_record;
			return MX_SUCCESSFUL_RESULT;
		    }
		    break;

		default:
		    /* Continue on to the next record. */
		    break;
		}
	    }

	    current_record = current_record->next_record;
	}

	/* If we get this far, there is no matching record in the database,
	 * so we create one now on the spot.
	 */

	switch( server_type ) {
	case MXN_NET_TCPIP:
		sprintf( format,
	"s%%-.%ds server network tcp_server \"\" \"\" 0x0 %%s %%d",
			MXU_RECORD_NAME_LENGTH-2 );

		MX_DEBUG( 2,("%s: format = '%s'", fname, format));
		MX_DEBUG( 2,("%s: server_name = '%s', port_number = %d",
			fname, server_name, port_number));

		sprintf( description, format,
				server_name, server_name, port_number );
		break;

	case MXN_NET_UNIX:
		sprintf( format,
	"s%%-.%ds server network unix_server \"\" \"\" 0x0 %%s",
			MXU_RECORD_NAME_LENGTH-2 );

		MX_DEBUG( 2,("%s: format = '%s'", fname, format));
		MX_DEBUG( 2,("%s: server_name = '%s', server_arguments = '%s'",
			fname, server_name, server_arguments));

		sprintf( description, format, server_name, server_arguments );
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The attempt to create a server record of MX type %ld "
		"was unsuccessful since the requested driver type "
		"is not MXN_NET_TCPIP or MXN_NET_UNIX.  "
		"This should never happen.", server_type );
	}

	MX_DEBUG( 2,("%s: description = '%s'",
			fname, description));

	mx_status = mx_create_record_from_description( record_list,
					description, &new_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_finish_record_initialization( new_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_open_hardware( new_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*server_record = new_record;

	return MX_SUCCESSFUL_RESULT;
}


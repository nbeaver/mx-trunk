/*
 * Name:    i_network_rs232.c
 *
 * Purpose: MX driver for network RS-232 devices.
 *
 * Author:  William Lavender
 *
 * FIXME:   If you use buffered I/O, it will have an adverse effect on
 *          clients that need quick startup times like 'goniostat'.
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001-2006, 2008, 2010, 2012, 2014-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NETWORK_RS232_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_net.h"
#include "i_network_rs232.h"

MX_RECORD_FUNCTION_LIST mxi_network_rs232_record_function_list = {
	NULL,
	mxi_network_rs232_create_record_structures,
	mxi_network_rs232_finish_record_initialization,
	NULL,
	NULL,
	mxi_network_rs232_open,
	NULL,
	NULL,
	mxi_network_rs232_resynchronize
};

MX_RS232_FUNCTION_LIST mxi_network_rs232_rs232_function_list = {
	mxi_network_rs232_getchar,
	mxi_network_rs232_putchar,
	NULL,
	NULL,
	mxi_network_rs232_getline,
	mxi_network_rs232_putline,
	mxi_network_rs232_num_input_bytes_available,
	mxi_network_rs232_discard_unread_input,
	mxi_network_rs232_discard_unwritten_output,
	mxi_network_rs232_get_signal_state,
	mxi_network_rs232_set_signal_state,
	mxi_network_rs232_get_configuration,
	mxi_network_rs232_set_configuration,
	mxi_network_rs232_send_break,
	NULL,
	mxi_network_rs232_get_echo,
	mxi_network_rs232_set_echo
};

MX_RECORD_FIELD_DEFAULTS mxi_network_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_NETWORK_RS232_STANDARD_FIELDS
};

long mxi_network_rs232_num_record_fields
		= sizeof( mxi_network_rs232_record_field_defaults )
			/ sizeof( mxi_network_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_network_rs232_rfield_def_ptr
			= &mxi_network_rs232_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_network_rs232_get_pointers( MX_RS232 *rs232,
			MX_NETWORK_RS232 **network_rs232,
			const char *calling_fname )
{
	static const char fname[] = "mxi_network_rs232_get_pointers()";

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( network_rs232 == (MX_NETWORK_RS232 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( rs232->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*network_rs232 = (MX_NETWORK_RS232 *) rs232->record->record_type_struct;

	if ( *network_rs232 == (MX_NETWORK_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_RS232 pointer for record '%s' is NULL.",
			rs232->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_network_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_network_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_NETWORK_RS232 *network_rs232 = NULL;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	network_rs232 = (MX_NETWORK_RS232 *) malloc( sizeof(MX_NETWORK_RS232) );

	if ( network_rs232 == (MX_NETWORK_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_RS232 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = network_rs232;
	record->class_specific_function_list
				= &mxi_network_rs232_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_rs232_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_network_rs232_finish_record_initialization()";

	MX_RS232 *rs232;
	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	strlcpy(record->network_type_name, "mx", MXU_NETWORK_TYPE_NAME_LENGTH);

#if 0
	/* Force the read and write terminators to zero. */

	if ( rs232->read_terminators != 0 ) {
		mx_warning(
"The read terminators for RS-232 port '%s' should be 0 rather than %#lx.  "
"Now fixing this.", record->name, rs232->read_terminators );

		rs232->read_terminators = 0;
	}

	if ( rs232->write_terminators != 0 ) {
		mx_warning(
"The write terminators for RS-232 port '%s' should be 0 rather than %#lx.  "
"Now fixing this.", record->name, rs232->write_terminators );

		rs232->write_terminators = 0;
	}
#endif

	/* Check to see if the RS-232 parameters are valid. */

	status = mx_rs232_check_port_parameters( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Mark the network_rs232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	/* Initialize the network record field structures. */

	mx_network_field_init( &(network_rs232->discard_unread_input_nf),
		network_rs232->server_record,
		"%s.discard_unread_input", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->discard_unwritten_output_nf),
		network_rs232->server_record,
	    "%s.discard_unwritten_output", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->echo_nf),
		network_rs232->server_record,
		"%s.echo", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->flow_control_nf),
		network_rs232->server_record,
		"%s.flow_control", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->getchar_nf),
		network_rs232->server_record,
		"%s.getchar", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->getline_nf),
		network_rs232->server_record,
		"%s.getline", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->get_configuration_nf),
		network_rs232->server_record,
		"%s.get_configuration", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->num_input_bytes_available_nf),
		network_rs232->server_record,
		"%s.num_input_bytes_available",
		network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->parity_nf),
		network_rs232->server_record,
		"%s.parity", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->putchar_nf),
		network_rs232->server_record,
		"%s.putchar", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->putline_nf),
		network_rs232->server_record,
		"%s.putline", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->read_terminators_nf),
		network_rs232->server_record,
		"%s.read_terminators", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->resynchronize_nf),
		network_rs232->server_record,
		"%s.resynchronize", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->send_break_nf),
		network_rs232->server_record,
		"%s.send_break", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->set_configuration_nf),
		network_rs232->server_record,
		"%s.set_configuration", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->signal_state_nf),
		network_rs232->server_record,
		"%s.signal_state", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->speed_nf),
		network_rs232->server_record,
		"%s.speed", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->stop_bits_nf),
		network_rs232->server_record,
		"%s.stop_bits", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->word_size_nf),
		network_rs232->server_record,
		"%s.word_size", network_rs232->remote_record_name );

	mx_network_field_init( &(network_rs232->write_terminators_nf),
		network_rs232->server_record,
		"%s.write_terminators", network_rs232->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_rs232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_network_rs232_open()";

	MX_RS232 *rs232;
	MX_NETWORK_RS232 *network_rs232 = NULL;
	MX_RECORD *server_record;
	MX_NETWORK_SERVER *server;
	unsigned long remote_mx_version;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	server_record = network_rs232->server_record;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The server_record pointer for network RS-232 record '%s' is NULL.",
			record->name );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for server record '%s ' "
		"used by network RS-232 record '%s' is NULL.",
			server_record->name, record->name );
	}

	remote_mx_version = server->remote_mx_version;

	MX_DEBUG( 2,("%s: Network RS-232 '%s', remote server version = %lu",
		fname, record->name, remote_mx_version));

	if ( remote_mx_version >= MX_VERSION_GETCHAR_PUTCHAR_HAS_CHAR_TYPE ) {
		network_rs232->getchar_putchar_is_char = TRUE;
	} else {
		network_rs232->getchar_putchar_is_char = FALSE;
	}

	MX_DEBUG( 2,("%s: Network RS-232 '%s', getchar_putchar_is char = %d",
		fname, record->name,
		(int) network_rs232->getchar_putchar_is_char ));

#if 1
	/* FIXME: Buffered network RS-232 I/O is not really working yet, so we
	 * force it off here.  (WML)
	 */

	rs232->rs232_flags &= ( ~ MXF_232_UNBUFFERED_IO );
#endif

#if 0
	if ( ( rs232->rs232_flags & MXF_232_UNBUFFERED_IO ) == 0 ) {

		long datatype, num_dimensions;
		long dimension_array[1];
		char rfname[ MXU_RECORD_FIELD_NAME_LENGTH + 1 ];

		/* Set up for buffered network I/O */

		/* Get the lengths of all of the network transfer buffers. */

		/* read */

		snprintf( rfname, sizeof(rfname),
				"%s.read", network_rs232->remote_record_name );

		mx_status = mx_get_field_type( network_rs232->server_record,
					rfname, 1L, &datatype, &num_dimensions,
					dimension_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( num_dimensions != 1 ) || ( datatype != MXFT_CHAR ) ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"Remote record field '%s' is not a 1-dimensional array "
		"of chars as is required by the driver for record '%s'.",
				rfname, record->name );
		}

		if ( dimension_array[0] < 0 ) {
			network_rs232->remote_read_buffer_length = 0;
		} else {
			network_rs232->remote_read_buffer_length
				= dimension_array[0];
		}

		/* write */

		snprintf( rfname, sizeof(rfname),
				"%s.write",
				network_rs232->remote_record_name );

		mx_status = mx_get_field_type( network_rs232->server_record,
					rfname, 1L, &datatype, &num_dimensions,
					dimension_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( num_dimensions != 1 ) || ( datatype != MXFT_CHAR ) ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"Remote record field '%s' is not a 1-dimensional array "
		"of chars as is required by the driver for record '%s'.",
				rfname, record->name );
		}

		if ( dimension_array[0] < 0 ) {
			network_rs232->remote_write_buffer_length = 0;
		} else {
			network_rs232->remote_write_buffer_length
				= dimension_array[0];
		}

		/* getline */

		snprintf( rfname, sizeof(rfname),
				"%s.getline",
				network_rs232->remote_record_name );

		mx_status = mx_get_field_type( network_rs232->server_record,
					rfname, 1L, &datatype, &num_dimensions,
					dimension_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( num_dimensions != 1 ) || ( datatype != MXFT_CHAR ) ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"Remote record field '%s' is not a 1-dimensional array "
		"of chars as is required by the driver for record '%s'.",
				rfname, record->name );
		}

		if ( dimension_array[0] < 0 ) {
			network_rs232->remote_getline_buffer_length = 0;
		} else {
			network_rs232->remote_getline_buffer_length
				= dimension_array[0];
		}

		/* putline */

		snprintf( rfname, sizeof(rfname),
				"%s.putline",
				network_rs232->remote_record_name );

		mx_status = mx_get_field_type( network_rs232->server_record,
					rfname, 1L, &datatype, &num_dimensions,
					dimension_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( num_dimensions != 1 ) || ( datatype != MXFT_CHAR ) ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"Remote record field '%s' is not a 1-dimensional array "
		"of chars as is required by the driver for record '%s'.",
				rfname, record->name );
		}

		if ( dimension_array[0] < 0 ) {
			network_rs232->remote_putline_buffer_length = 0;
		} else {
			network_rs232->remote_putline_buffer_length
				= dimension_array[0];
		}

	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_rs232_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_network_rs232_resynchronize()";

	MX_RS232 *rs232;
	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_bool_type resynchronize;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	resynchronize = TRUE;

	mx_status = mx_put( &(network_rs232->resynchronize_nf),
				MXFT_BOOL, &resynchronize );

	return mx_status;
}

/* The getchar and putchar values are transmitted as ints to protect
 * whitespace characters from being discarded by the token parser.
 */

MX_EXPORT mx_status_type
mxi_network_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_network_rs232_getchar()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	int int_c, datatype;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The character pointer 'c' passed is NULL." );
	}

	if ( network_rs232->getchar_putchar_is_char ) {
		mx_status = mx_get( &(network_rs232->getchar_nf),
							MXFT_CHAR, c );
	} else {
		/* MX 1.1.1 and before defined 'getchar' fields as MXFT_INT. */

		datatype = 6;	/* MXFT_INT */

		mx_status = mx_get( &(network_rs232->getchar_nf),
							datatype, &int_c );

		*c = (char) int_c;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_RS232_DEBUG
	MX_DEBUG(-2, ("%s: received 0x%x, '%c'", fname, *c, *c));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_network_rs232_putchar()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	int int_c, datatype;
	mx_status_type mx_status;

#if MXI_NETWORK_RS232_DEBUG
	MX_DEBUG(-2, ("%s: sending 0x%x, '%c'", fname, c, c));
#endif

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( network_rs232->getchar_putchar_is_char ) {
		mx_status = mx_put( &(network_rs232->putchar_nf),
							MXFT_CHAR, &c );
	} else {
		/* MX 1.1.1 and before defined 'putchar' fields as MXFT_INT. */

		datatype = 6;	/* MXFT_INT */

		int_c = (int) c;

		mx_status = mx_put( &(network_rs232->putchar_nf),
							datatype, &int_c );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_getline( MX_RS232 *rs232,
				char *buffer,
				size_t max_bytes_to_read,
				size_t *bytes_read )
{
	static const char fname[] = "mxi_network_rs232_getline()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	long local_bytes_read;
	long dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dimension_array[0] = max_bytes_to_read;

	mx_status = mx_get_array( &(network_rs232->getline_nf),
				MXFT_STRING, 1, dimension_array, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_read != (size_t *) NULL ) {
		mx_status = mx_get( &(network_rs232->bytes_read_nf),
				MXFT_ULONG, &local_bytes_read );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*bytes_read = local_bytes_read;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_putline( MX_RS232 *rs232,
				char *buffer,
				size_t *bytes_written )
{
	static const char fname[] = "mxi_network_rs232_getline()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	long local_bytes_written;
	long dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dimension_array[0] = strlen(buffer) + 1;

	mx_status = mx_put_array( &(network_rs232->putline_nf),
				MXFT_STRING, 1, dimension_array, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_written != (size_t *) NULL ) {
		mx_status = mx_get( &(network_rs232->bytes_written_nf),
				MXFT_ULONG, &local_bytes_written );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*bytes_written = local_bytes_written;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_network_rs232_num_input_bytes_available()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_rs232->num_input_bytes_available_nf),
			MXFT_ULONG, &(rs232->num_input_bytes_available) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_network_rs232_discard_unread_input()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_bool_type discard_unread_input;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	discard_unread_input = TRUE;

	mx_status = mx_put( &(network_rs232->discard_unread_input_nf),
				MXFT_BOOL, &discard_unread_input );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_discard_unwritten_output( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_network_rs232_discard_unwritten_output()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_bool_type discard_unwritten_output;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	discard_unwritten_output = TRUE;

	mx_status = mx_put( &(network_rs232->discard_unwritten_output_nf),
				MXFT_BOOL, &discard_unwritten_output );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_get_signal_state( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_network_rs232_get_signal_state()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_rs232->signal_state_nf),
			MXFT_HEX, &(rs232->signal_state) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_set_signal_state( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_network_rs232_set_signal_state()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->signal_state_nf),
			MXFT_HEX, &(rs232->signal_state) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_get_configuration( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_network_rs232_get_configuration()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->get_configuration_nf),
			MXFT_BOOL, &(rs232->get_configuration) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_rs232->speed_nf),
			MXFT_LONG, &(rs232->speed) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_rs232->word_size_nf),
			MXFT_LONG, &(rs232->word_size) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_rs232->parity_nf),
			MXFT_CHAR, &(rs232->parity) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_rs232->stop_bits_nf),
			MXFT_LONG, &(rs232->stop_bits) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_rs232->flow_control_nf),
			MXFT_CHAR, &(rs232->flow_control) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_rs232->read_terminators_nf),
			MXFT_HEX, &(rs232->read_terminators) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_rs232->write_terminators_nf),
			MXFT_HEX, &(rs232->write_terminators) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_set_configuration( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_network_rs232_set_configuration()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->speed_nf),
			MXFT_LONG, &(rs232->speed) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->word_size_nf),
			MXFT_LONG, &(rs232->word_size) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->parity_nf),
			MXFT_CHAR, &(rs232->parity) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->stop_bits_nf),
			MXFT_LONG, &(rs232->stop_bits) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->flow_control_nf),
			MXFT_CHAR, &(rs232->flow_control) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->read_terminators_nf),
			MXFT_HEX, &(rs232->read_terminators) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->write_terminators_nf),
			MXFT_HEX, &(rs232->write_terminators) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->set_configuration_nf),
			MXFT_BOOL, &(rs232->set_configuration) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_send_break( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_network_rs232_send_break()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->send_break_nf),
			MXFT_BOOL, &(rs232->send_break) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_get_echo( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_network_rs232_get_echo()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_rs232->echo_nf),
			MXFT_BOOL, &(rs232->echo) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_rs232_set_echo( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_network_rs232_set_echo()";

	MX_NETWORK_RS232 *network_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_network_rs232_get_pointers( rs232,
						&network_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_rs232->echo_nf),
			MXFT_BOOL, &(rs232->echo) );

	return mx_status;
}


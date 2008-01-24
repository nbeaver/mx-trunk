/*
 * Name:    mx_server_connect.c
 *
 * Purpose: This routine sets up a connection to a remote MX server without
 *          the need to explicitly setup an entire MX database.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2006, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SERVER_CONNECT_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_net.h"

MX_EXPORT mx_status_type
mx_connect_to_mx_server( MX_RECORD **server_record,
				char *server_name,
				int server_port,
				unsigned long server_flags )
{
	static const char fname[] = "mx_connect_to_mx_server()";

	MX_RECORD *record_list;
	MX_RECORD *current_record;
	MX_LIST_HEAD *list_head_struct;
	static char description[MXU_RECORD_DESCRIPTION_LENGTH + 1];
	int i, max_retries;
	unsigned long num_servers;
	mx_bool_type new_database;
	mx_status_type mx_status;

#if MX_SERVER_CONNECT_DEBUG
	MX_DEBUG(-2,
	("%s: server_name = '%s', server_port = %d, server_flags = %#lx",
			fname, server_name, server_port, server_flags ));
#endif

	if ( *server_record == NULL ) {

		/* If the value of *server_record passed to us is NULL,
		 * then we need to setup a new MX database from scratch.
		 */

		/* Initialize MX device drivers. */

		mx_status = mx_initialize_drivers();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set up a record list to put this server record in. */

		record_list = mx_initialize_record_list();

		if ( record_list == NULL ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
				"Unable to setup an MX record list." );
		}

		new_database = TRUE;
	} else {
		/* If *server_record is not NULL, then it actually contains
		 * an arbitrary record from an existing MX database.  Every
		 * record in an MX database has a pointer to the list head
		 * record, so get that record.
		 */

		record_list = (*server_record)->list_head;

		if ( record_list == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The list head record pointer is NULL for the "
			"record '%s' which was passed to this routine.",
				(*server_record)->name );
		}

		new_database = FALSE;
	}

	/* Count the existing number of server records in the MX database.
	 * We use that information to construct a unique reserved name for
	 * the server record that we are about to create.
	 */

	num_servers = 0;

	current_record = record_list->next_record;

	while ( current_record != record_list ) {
		if ( current_record->mx_superclass == MXR_SERVER ) {
			num_servers++;
		}
	}

	/* Create a record description for this server. */

	snprintf( description, sizeof(description),
	"mx_serv%lu server network tcpip_server \"\" \"\" %#lx %s %d",
		num_servers, server_flags, server_name, server_port );

#if MX_SERVER_CONNECT_DEBUG
	MX_DEBUG(-2,("%s: description = '%s'", fname, description));
#endif

	/* Now create the record.  This step will overwrite any previous
	 * value for the pointer *server_record.
	 */

	mx_status = mx_create_record_from_description( record_list, description,
							server_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_finish_record_initialization( *server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the record list as ready to be used. */

	if ( new_database ) {
		list_head_struct = mx_get_record_list_head_struct(record_list);

		if ( list_head_struct == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"MX record list head structure is NULL." );
		}

		list_head_struct->list_is_active = TRUE;
		list_head_struct->fixup_records_in_use = FALSE;
	}

	/* Try to connect to the MX server. */

	max_retries = 100;

	for ( i = 0; i < max_retries; i++ ) {
		mx_status = mx_open_hardware( *server_record );

		if ( mx_status.code == MXE_SUCCESS )
			break;                 /* Exit the for() loop. */

		switch( mx_status.code ) {
		case MXE_NETWORK_IO_ERROR:
			break;                 /* Try again. */
		default:
			return mx_status;
			break;
		}
		mx_msleep(1000);
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"%d attempts to connect to the MX server '%s' at port %d have failed.  "
"Update process aborting...", max_retries, server_name, server_port );
	}

#if MX_SERVER_CONNECT_DEBUG
	MX_DEBUG(-2,("%s: Successfully created server record '%s'.",
		fname, (*server_record)->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_create_local_field( MX_NETWORK_FIELD *nf,
				MX_RECORD *server_record,
				char *record_name,
				char *field_name,
				MX_RECORD_FIELD **temp_field )
{
#if MX_SERVER_CONNECT_DEBUG
	static const char fname[] = "mx_create_local_field()";
#endif

	char record_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	long i, datatype, num_dimensions;
	long dimension_array[8];
	size_t *sizeof_array;
	void *value_ptr;
	mx_status_type mx_status;

	/* Initialize the network field data structure. */

	snprintf( record_field_name, sizeof(record_field_name),
			"%s.%s", record_name, field_name );

	mx_status = mx_network_field_init( nf, server_record,
						record_field_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out the dimensions of the record field. */

	/* FIXME! - This should be done using a network field handle. */

	mx_status = mx_get_field_type( server_record, record_field_name,
					1L, &datatype, &num_dimensions,
					dimension_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_SERVER_CONNECT_DEBUG
	MX_DEBUG(-2,("%s: datatype = %s, num_dimensions = %ld", fname,
		mx_get_field_type_string(datatype), num_dimensions));

	if ( mx_get_debug_level() >= 2 ) {
		fprintf( stderr, "%s: ", fname );
		for ( i = 0; i < num_dimensions; i++ ) {
			fprintf( stderr, "%ld ", dimension_array[i] );
		}
		fprintf( stderr, "\n\n" );
	}
#endif

	/* Allocate local memory for the network field value. */

	mx_status = mx_get_datatype_sizeof_array( datatype, &sizeof_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_dimensions == 0 ) {
		value_ptr = malloc( sizeof_array[0] );
	} else {
		value_ptr = mx_allocate_array( num_dimensions,
					dimension_array, sizeof_array );
	}

	if ( value_ptr == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Unable to allocate a local %ld-dimensional "
			"array for record field '%s'.",
				num_dimensions, record_field_name );
	}

	/* Create a local 'temporary' record field to describe the
	 * network field value.  We must use &value_ptr for any value
	 * pointer given to mx_initialize_temp_record_field(), since
	 * the temporary field created has the MXFF_VARARGS bit set.
	 */

	*temp_field = malloc( sizeof(MX_RECORD_FIELD) );

	if ( *temp_field == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		  "Unable to allocate memory for the 'temp_field' structure.");
	}

#if MX_SERVER_CONNECT_DEBUG
	MX_DEBUG(-2,("%s: temp_field = %p", fname, temp_field));
#endif

	mx_status = mx_initialize_temp_record_field( *temp_field,
							datatype,
							num_dimensions,
							dimension_array,
							sizeof_array,
							&value_ptr );
	return mx_status;
}


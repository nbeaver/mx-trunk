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
 * Copyright 2005-2006, 2008, 2010, 2013, 2016-2017, 2021-2022
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SERVER_CONNECT_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_net.h"
#include "n_tcpip.h"

/* NOTE: Among other things, the following value of MXU_SC_MAX_DIMENSIONS
 * will limit the maximum number of dimensions that can be used by the
 * command line programs 'mxget' and 'mxput'.
 */

#define MXU_SC_MAX_DIMENSIONS		64

MX_EXPORT mx_status_type
mx_connect_to_mx_server( MX_RECORD **server_record,
				char *server_name,
				int server_port,
				double timeout_in_seconds,
				unsigned long server_flags )
{
	static const char fname[] = "mx_connect_to_mx_server()";

	MX_RECORD *record_list;
	MX_RECORD *current_record;
	MX_LIST_HEAD *list_head_struct;
	MX_TCPIP_SERVER *tcpip_server;
	static char description[MXU_RECORD_DESCRIPTION_LENGTH + 1];
	int i, max_attempts;
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

		/* Set up an MX database to put this server record in. */

		record_list = mx_initialize_database();

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
	 * the server record that we are about to create.  If we find an
	 * already existing record, then we return the existing record.
	 */

	num_servers = 0;

	current_record = record_list->next_record;

	while ( current_record != record_list ) {
	    if ( ( current_record->mx_superclass == MXR_SERVER )
	      && ( current_record->mx_class == MXN_NETWORK_SERVER ) )
	    {
		num_servers++;

		/* Does current_record match the server parameters
		 * that we are looking for?  If so, then return
		 * with *server_record set to current_record.
		 */

		switch( current_record->mx_type ) {
		case MXN_NET_TCPIP:
		    tcpip_server = current_record->record_type_struct;

		    if ( strcmp(server_name, tcpip_server->hostname) != 0 ) {

		    	/* Not a match, so go on to the next record. */
			break;
		    }
		    if ( server_port != tcpip_server->port ) {

		    	/* Not a match, so go on to the next record. */
			break;
		    }

		    /* WE HAVE A MATCH!  Return the matching server record
		     * to the calling function now.
		     */

		    *server_record = current_record;

		    return MX_SUCCESSFUL_RESULT;
		    break;
		default:
		    break;
		}
	    }
	    current_record = current_record->next_record;
	}

	/* If we get here, then the database did not already have
	 * a matching server record, so we need to create one.
	 */

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
		unsigned long mask;

		list_head_struct = mx_get_record_list_head_struct(record_list);

		if ( list_head_struct == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"MX record list head structure is NULL." );
		}

		list_head_struct->list_is_active = TRUE;
		list_head_struct->fixup_records_in_use = FALSE;

		mask = MXF_NETDBG_SUMMARY | MXF_NETDBG_VERBOSE;

		list_head_struct->network_debug_flags = server_flags & mask;
	}

	/* Try to connect to the MX server. */

	max_attempts = mx_round( timeout_in_seconds );

	if ( max_attempts < 1 ) {
		max_attempts = 1;
	}

	for ( i = 0; i < max_attempts; i++ ) {
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
		return mx_error( MXE_TIMED_OUT, fname,
		"The attempt to connect to the MX server '%s' at port %d has "
		"timed out after %f seconds.",
			server_name, server_port,
			timeout_in_seconds );
	}

#if MX_SERVER_CONNECT_DEBUG
	MX_DEBUG(-2,("%s: Successfully created server record '%s'.",
		fname, (*server_record)->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_create_network_field( MX_NETWORK_FIELD **nf,
				MX_RECORD *server_record,
				char *record_name,
				char *field_name,
				MX_RECORD_FIELD *local_field )
{
	static const char fname[] = "mx_create_network_field()";

	char record_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	long datatype, num_dimensions;
	long *dimension_array;
	size_t sizeof_array[ MXU_FIELD_MAX_DIMENSIONS ];
	void **value_ptr;
	mx_status_type mx_status;

	if ( nf == (MX_NETWORK_FIELD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}

	/* Create a new MX_NETWORK_FIELD structure. */

	*nf = malloc( sizeof(MX_NETWORK_FIELD) );

	if ( (*nf) == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate an MX_NETWORK_FIELD structure." );
	}

	/* Initialize the network field data structure. */

	snprintf( record_field_name, sizeof(record_field_name),
			"%s.%s", record_name, field_name );

	mx_status = mx_network_field_init( *nf, server_record,
						record_field_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the local_field pointer passed to us was not NULL, then we
	 * save that value in the network field structure and then return.
	 */

	if ( local_field != (MX_RECORD_FIELD *) NULL ) {

		(*nf)->local_field = local_field;

		(*nf)->must_free_local_field_on_delete = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, then we need to set up a private MX_RECORD_FIELD
	 * structure.
	 */

	(*nf)->must_free_local_field_on_delete = TRUE;

	/* Allocate dimension_array with malloc().  We will be passing
	 * dimension_array to mx_initialize_temp_record_field() later
	 * on, so it is absolutely required that dimension_array be a
	 * pointer to heap memory and _not_ stack memory.  If we had
	 * declared dimension_array above as 'long dimension_array[8]',
	 * then dimension_array would be a pointer to memory on the
	 * stack, which would result in a crash at some point after
	 * returning from mx_create_network_field().
	 */

	dimension_array = malloc( MXU_SC_MAX_DIMENSIONS * sizeof(long) );

	if ( dimension_array == (long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Unable to allocate memory for an %d element "
			"dimension array.", MXU_SC_MAX_DIMENSIONS );
	}

	/* Find out the dimensions of the remote network field. */

	/* FIXME! - This should be done using a network field handle. */

	mx_status = mx_get_field_type( server_record, record_field_name,
					MXU_SC_MAX_DIMENSIONS,
					&datatype, &num_dimensions,
					dimension_array,
					MXF_GFT_SHOW_STACK_TRACEBACK );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_SERVER_CONNECT_DEBUG
	MX_DEBUG(-2,("%s: datatype = %s, num_dimensions = %ld", fname,
		mx_get_field_type_string(datatype), num_dimensions));

	if ( mx_get_debug_level() >= -2 ) {
		long i;

		fprintf( stderr, "%s: dimension = ", fname );
		for ( i = 0; i < num_dimensions; i++ ) {
			fprintf( stderr, "%ld ", dimension_array[i] );
		}
		fprintf( stderr, "\n\n" );
	}
#endif
	/* Get the correct sizeof_array for this data type so that we
	 * can use it for allocating memory below.
	 */

	mx_status = mx_get_datatype_sizeof_array( datatype,
					sizeof_array, sizeof(sizeof_array) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate memory for a pointer to the network field value. */

	value_ptr = malloc( sizeof(void *) );

	if ( value_ptr == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Unable to allocate memory for a pointer to "
			"the network field value for record field '%s'.",
			record_field_name );
	}

	/* Allocate memory for the network field value itself. */

	if ( num_dimensions == 0 ) {
		*value_ptr = malloc( sizeof_array[0] );
	} else {
		*value_ptr = mx_allocate_array( datatype,
				num_dimensions, dimension_array, sizeof_array );
	}

	if ( *value_ptr == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Unable to allocate a local %ld-dimensional "
			"array for record field '%s'.",
				num_dimensions, record_field_name );
	}

	/* Create the private MX_RECORD_FIELD that is used to describe and
	 * store the network field value.
	 */

	local_field = malloc( sizeof(MX_RECORD_FIELD) );

	if ( local_field == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		  "Unable to allocate memory for the 'local_field' structure.");
	}

	(*nf)->local_field = local_field;

#if MX_SERVER_CONNECT_DEBUG
	MX_DEBUG(-2,("%s: local_field = %p, *value_ptr = %p",
		fname, local_field, *value_ptr));
#endif

	mx_status = mx_initialize_temp_record_field( local_field,
							datatype,
							num_dimensions,
							dimension_array,
							sizeof_array,
							value_ptr );

#if MX_SERVER_CONNECT_DEBUG
	MX_DEBUG(-2,("%s: local_field->data_pointer = %p",
		fname, local_field->data_pointer));
	MX_DEBUG(-2,("%s: *(local_field->data_pointer) = %p", fname,
    mx_read_void_pointer_from_memory_location(local_field->data_pointer) ));
#endif
	return mx_status;
}


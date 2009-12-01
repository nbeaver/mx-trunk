/*
 * Name:    mx_multi.c
 *
 * Purpose: Functions for cross-platform network support.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_MULTI_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_multi.h"

#if HAVE_EPICS
#  include "mx_epics.h"
#endif

MX_EXPORT void
mx_multi_set_debug_flag( MX_RECORD *record_list,
			mx_bool_type flag )
{
	static const char fname[] = "mx_multi_set_debug_flag()";

	MX_LIST_HEAD *list_head;

	/* First handle debug settings that do not require an MX record list. */
#if HAVE_EPICS
	mx_epics_set_debug_flag( flag );
#endif

	if ( record_list == (MX_RECORD *) NULL )
		return;

	/* Set the network debug flag for the MX database. */

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record list %p is NULL.",
			record_list );
		return;
	}

	if ( flag ) {
		list_head->network_debug = TRUE;
	} else {
		list_head->network_debug = FALSE;
	}

	return;
}

MX_EXPORT mx_status_type
mx_multi_create( MX_MULTI_NETWORK_VARIABLE **mnv,
		char *network_variable_description,
		void **network_object )
{
	static const char fname[] = "mx_multi_create()";

	unsigned long network_type = 0;
	char *dup_string, *strstr_ptr;
	char *body_ptr, *colon_ptr, *at_ptr;
	char *variable_name;
	char *host_port_name = NULL;
	char hostname[MXU_HOSTNAME_LENGTH+1];
	long port = -1;
	mx_status_type mx_status;

	MX_RECORD *mx_record;
	MX_NETWORK_FIELD *mx_nf;

#if HAVE_EPICS
	MX_EPICS_PV *epics_pv;
#endif

	if ( mnv == (MX_MULTI_NETWORK_VARIABLE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MULTI_NETWORK_VARIABLE pointer passed was NULL." );
	}
	if ( network_variable_description == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The network variable description pointer passed was NULL." );
	}
	
	if ( strlen(network_variable_description) == 0 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The network variable description passed was of zero length." );
	}

#if MX_MULTI_DEBUG
	MX_DEBUG(-2,("%s: network_variable_description = '%s'",
		fname, network_variable_description));
#endif

	/* Create the multi-network variable object. */

	*mnv = (MX_MULTI_NETWORK_VARIABLE *)
			malloc( sizeof(MX_MULTI_NETWORK_VARIABLE) );

	if ( (*mnv) == (MX_MULTI_NETWORK_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"MX_MULTI_NETWORK_VARIABLE object." );
	}

	/**** Parse the network variable description string ****/

	dup_string = strdup( network_variable_description );

	if ( dup_string == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to create a duplicate "
			"of the string '%s'.", network_variable_description );
	}

	if ( (strstr_ptr = strstr( dup_string, ":/" )) != NULL ) {

		/* This is a URL style name. */

		body_ptr = strstr_ptr + 2;	/* Skip over ':/' */

		*strstr_ptr = '\0';

		if ( *body_ptr == '\0' ) {
			mx_free(dup_string);

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The body of the URL '%s' passed was empty.",
				network_variable_description );
		}

		if ( strcmp( dup_string, "mx" ) == 0 ) {
			network_type = MXT_MNV_MX;

			if ( *body_ptr != '/' ) {

				/* One / in a row --> mx:/my_nf.value */

				strlcpy( hostname, "localhost",
						sizeof(hostname) );
				port = 9727;
				variable_name = body_ptr;
				host_port_name = NULL;
			} else
			if ( strstr( body_ptr, "//" ) == body_ptr ) {

				/* Three /// in a row -->  mx:///my_nf.value */

				strlcpy( hostname, "localhost",
						sizeof(hostname) );
				port = 9727;
				variable_name = body_ptr + 2;
				host_port_name = NULL;
			} else {
				/* Two // in a row */

				/* mx://myhost@myport/my_nf.value */
				/* mx://myhost/my_nf.value */

				host_port_name = body_ptr + 1;

				variable_name = strchr( body_ptr, '/' );

				if ( variable_name != NULL ) {
					variable_name++;
				}
			}
		} else
		if ( strcmp( dup_string, "epics" ) == 0 ) {
			network_type = MXT_MNV_EPICS;

			/* Skip over any additional / characters. */

			variable_name = body_ptr + strspn( body_ptr, "/" );

		} else {
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized network type '%s' found in URL '%s'.",
				dup_string, network_variable_description );

			mx_free(dup_string);
			return mx_status;
		}
	} else
	if ( (strstr_ptr = strstr( dup_string, "::" )) != NULL ) {

		/* This is a VMS Decnet style name. */

		body_ptr = strstr_ptr + 2;	/* Skip over '::' */

		*strstr_ptr = '\0';

		if ( *body_ptr == '\0' ) {
			mx_free(dup_string);

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The body of the URL '%s' passed was empty.",
				network_variable_description );
		}

		if ( strcmp( dup_string, "mx" ) == 0 ) {
			network_type = MXT_MNV_MX;

			colon_ptr = strchr( body_ptr, ':' );

			if ( colon_ptr == NULL ) {

				/* mx::my_nf.value */

				strlcpy( hostname, "localhost",
						sizeof(hostname) );
				port = 9727;
				variable_name = body_ptr;
				host_port_name = NULL;
			} else {
				*colon_ptr = '\0';

				host_port_name = body_ptr;
				variable_name = colon_ptr + 1;
			}
		} else
		if ( strcmp( dup_string, "epics" ) == 0 ) {
			network_type = MXT_MNV_EPICS;

			variable_name = body_ptr;
		} else {
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized network type '%s' found in URL '%s'.",
				dup_string, network_variable_description );

			mx_free(dup_string);
			return mx_status;
		}

	} else {
		/* If we get here, assume that it is an MX variable name. */

		network_type = MXT_MNV_MX;

		colon_ptr = strchr( dup_string, ':' );

		if ( colon_ptr == NULL ) {

			/* my_nf.value */

			strlcpy( hostname, "localhost", sizeof(hostname) );
			port = 9727;

			variable_name = dup_string;
			host_port_name = NULL;
		} else {
			*colon_ptr = '\0';

			host_port_name = dup_string;
			variable_name = colon_ptr + 1;
		}
	}

	switch( network_type ) {
	case MXT_MNV_MX:
		if ( host_port_name != NULL ) {
			at_ptr = strchr( host_port_name, '@' );

			if ( at_ptr == NULL ) {
				port = 9727;
			} else {
				*at_ptr = '\0';
				at_ptr++;

				port = mx_string_to_long( at_ptr );
			}

			strlcpy( hostname, host_port_name, sizeof(hostname) );
		}

#if MX_MULTI_DEBUG
		MX_DEBUG(-2,("%s: MX host '%s', port %ld, nf name '%s'.",
			fname, hostname, port, variable_name));
#endif

		mx_nf = (MX_NETWORK_FIELD *) malloc( sizeof(MX_NETWORK_FIELD) );

		if ( mx_nf == (MX_NETWORK_FIELD *) NULL ) {
			mx_free(dup_string);
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"MX_NETWORK_FIELD object." );
		}

		if ( network_object == NULL ) {
			mx_record = NULL;
		} else {
			mx_record = (MX_RECORD *) (*network_object);
		}

		/* 'mx_record' can be any record in an MX database.
		 * It does not need to be a list head record or an
		 * MX server record, since mx_connect_to_mx_server()
		 * will do everything needed to find the real
		 * MX server record.
		 *
		 * However, after the call returns, then 'mx_record'
		 * will then point to the MX server record corresponding
		 * to the requested hostname and port number.
		 */

		mx_status = mx_connect_to_mx_server( &mx_record,
						hostname, port, 0x0 );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free(dup_string);
			return mx_status;
		}

		mx_status = mx_network_field_init( mx_nf,
						mx_record, variable_name );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free(dup_string);
			return mx_status;
		}

		(*mnv)->network_type = MXT_MNV_MX;
		(*mnv)->private_ptr = mx_nf;

		if ( network_object != NULL ) {
			(*network_object) = mx_record;
		}
		break;

	case MXT_MNV_EPICS:

#if MX_MULTI_DEBUG
		MX_DEBUG(-2,("%s: EPICS PV name = '%s'", fname, variable_name));
#endif

#if ( HAVE_EPICS == FALSE )
		return mx_error( MXE_UNSUPPORTED, fname,
			"Cannot find EPICS PV '%s' since EPICS support is not "
			"compiled into this copy of MX.", variable_name );

#else /* HAVE_EPICS */

		epics_pv = (MX_EPICS_PV *) malloc( sizeof(MX_EPICS_PV) );

		if ( epics_pv == (MX_EPICS_PV *) NULL ) {
			mx_free(dup_string);
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"MX_NETWORK_FIELD object." );
		}

		mx_epics_pvname_init( epics_pv, variable_name );

		(*mnv)->network_type = MXT_MNV_EPICS;
		(*mnv)->private_ptr = epics_pv;

#endif /* HAVE_EPICS */

		break;

	default:
		mx_free(dup_string);
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized network type %lu seen for variable '%s'",
			network_type, network_variable_description );
		break;
	}

	mx_free(dup_string);

	return MX_SUCCESSFUL_RESULT;
}


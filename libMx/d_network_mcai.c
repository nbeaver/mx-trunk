/*
 * Name:    d_network_mcai.c
 *
 * Purpose: MX network multichannel analog input driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_MCAI_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_mcai.h"
#include "mx_net.h"
#include "d_network_mcai.h"

MX_RECORD_FUNCTION_LIST mxd_network_mcai_record_function_list = {
	mxd_network_mcai_initialize_driver,
	mxd_network_mcai_create_record_structures,
	mxd_network_mcai_finish_record_initialization,
	NULL,
	NULL,
	mxd_network_mcai_open
};

MX_MCAI_FUNCTION_LIST
mxd_network_mcai_mcai_function_list = {
	mxd_network_mcai_read
};

MX_RECORD_FIELD_DEFAULTS mxd_network_mcai_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCAI_STANDARD_FIELDS,
	MXD_NETWORK_MCAI_STANDARD_FIELDS
};

long mxd_network_mcai_num_record_fields
		= sizeof( mxd_network_mcai_record_field_defaults )
			/ sizeof( mxd_network_mcai_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_mcai_rfield_def_ptr
			= &mxd_network_mcai_record_field_defaults[0];

static mx_status_type
mxd_network_mcai_get_pointers( MX_MCAI *mcai,
			MX_NETWORK_MCAI **network_mcai,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_mcai_get_pointers()";

	if ( mcai == (MX_MCAI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MCAI pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (network_mcai == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_NETWORK_MCAI pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mcai->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer for MX_MCAI pointer "
		"passed by '%s' was NULL.", calling_fname );
	}

	*network_mcai = (MX_NETWORK_MCAI *) mcai->record->record_type_struct;

	if ( *network_mcai == (MX_NETWORK_MCAI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_NETWORK_MCAI pointer for record '%s' passed by '%s' is NULL.",
			mcai->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mcai_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_channels_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mcai_initialize_driver( driver,
					&maximum_num_channels_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mcai_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_network_mcai_create_record_structures()";

        MX_MCAI *mcai;
        MX_NETWORK_MCAI *network_mcai;

        /* Allocate memory for the necessary structures. */

        mcai = (MX_MCAI *)
			malloc(sizeof(MX_MCAI));

        if ( mcai == (MX_MCAI *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_MCAI structure." );
        }

        network_mcai = (MX_NETWORK_MCAI *) malloc( sizeof(MX_NETWORK_MCAI) );

        if ( network_mcai == (MX_NETWORK_MCAI *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_NETWORK_MCAI structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = mcai;
        record->record_type_struct = network_mcai;
        record->class_specific_function_list
                                = &mxd_network_mcai_mcai_function_list;

        mcai->record = record;
	network_mcai->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mcai_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_mcai_finish_record_initialization()";

	MX_MCAI *mcai;
	MX_NETWORK_MCAI *network_mcai = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mcai = record->record_class_struct;

	mx_status = mx_mcai_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_network_mcai_get_pointers( mcai, &network_mcai, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_mcai->channel_array_nf),
		network_mcai->server_record,
		"%s.channel_array", network_mcai->remote_record_name );

        return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mcai_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_mcai_open()";

	MX_MCAI *mcai;
	MX_NETWORK_MCAI *network_mcai = NULL;
	char nfname[ MXU_RECORD_FIELD_NAME_LENGTH+1 ];
	long remote_maximum_num_channels;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mcai = record->record_class_struct;

	mx_status = mxd_network_mcai_get_pointers( mcai, &network_mcai, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure that the number of channels we are configured for
	 * is compatible with the configuration of the remote record.
	 */

	snprintf( nfname, sizeof(nfname), "%s.maximum_num_channels",
			network_mcai->remote_record_name );

	mx_status = mx_get_by_name( network_mcai->server_record, nfname,
				MXFT_LONG, &remote_maximum_num_channels );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( remote_maximum_num_channels < mcai->maximum_num_channels ) {
		mcai->maximum_num_channels = remote_maximum_num_channels;
	}
	if ( mcai->current_num_channels > mcai->maximum_num_channels ) {
		mcai->current_num_channels = mcai->maximum_num_channels;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mcai_read( MX_MCAI *mcai )
{
	static const char fname[] = "mxd_network_mcai_read()";

	MX_NETWORK_MCAI *network_mcai = NULL;
	long dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxd_network_mcai_get_pointers( mcai, &network_mcai, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dimension_array[0] = mcai->maximum_num_channels;

	mx_status = mx_get_array( &(network_mcai->channel_array_nf),
				MXFT_DOUBLE, 1, dimension_array,
				mcai->channel_array );

	return mx_status;
}


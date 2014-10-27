/*
 * Name:    d_network_sca.c
 *
 * Purpose: MX single channel analyzer driver for network SCAs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2004, 2006-2007, 2010, 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_sca.h"
#include "mx_net.h"
#include "d_network_sca.h"

/* Initialize the SCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_network_sca_record_function_list = {
	NULL,
	mxd_network_sca_create_record_structures,
	mxd_network_sca_finish_record_initialization,
	NULL,
	NULL,
	mxd_network_sca_open,
	NULL,
	NULL,
	mxd_network_sca_resynchronize
};

MX_SCA_FUNCTION_LIST mxd_network_sca_sca_function_list = {
	mxd_network_sca_get_parameter,
	mxd_network_sca_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_network_sca_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCA_STANDARD_FIELDS,
	MXD_NETWORK_SCA_STANDARD_FIELDS
};

long mxd_network_sca_num_record_fields
		= sizeof( mxd_network_sca_record_field_defaults )
		  / sizeof( mxd_network_sca_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_sca_rfield_def_ptr
			= &mxd_network_sca_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_network_sca_get_pointers( MX_SCA *sca,
			MX_NETWORK_SCA **network_sca,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_sca_get_pointers()";

	if ( sca == (MX_SCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCA pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( network_sca == (MX_NETWORK_SCA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_NETWORK_SCA pointer passed by '%s' was NULL",
			calling_fname );
	}

	*network_sca = (MX_NETWORK_SCA *) sca->record->record_type_struct;

	if ( *network_sca == (MX_NETWORK_SCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_SCA pointer for sca record '%s' passed by '%s' is NULL",
				sca->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_network_sca_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_sca_create_record_structures()";

	MX_SCA *sca;
	MX_NETWORK_SCA *network_sca;

	/* Allocate memory for the necessary structures. */

	sca = (MX_SCA *) malloc( sizeof(MX_SCA) );

	if ( sca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCA structure." );
	}

	network_sca = (MX_NETWORK_SCA *) malloc( sizeof(MX_NETWORK_SCA) );

	if ( network_sca == (MX_NETWORK_SCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_SCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = sca;
	record->record_type_struct = network_sca;
	record->class_specific_function_list
		= &mxd_network_sca_sca_function_list;

	sca->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_sca_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_sca_finish_record_initialization()";

	MX_SCA *sca;
	MX_NETWORK_SCA *network_sca;
	mx_status_type mx_status;

	network_sca = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	sca = (MX_SCA *) record->record_class_struct;

	mx_status = mxd_network_sca_get_pointers( sca, &network_sca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy(record->network_type_name, "mx", MXU_NETWORK_TYPE_NAME_LENGTH);

	mx_network_field_init( &(network_sca->gain_nf),
		network_sca->server_record,
		"%s.gain", network_sca->remote_record_name );

	mx_network_field_init( &(network_sca->lower_level_nf),
		network_sca->server_record,
		"%s.lower_level", network_sca->remote_record_name );

	mx_network_field_init( &(network_sca->resynchronize_nf),
		network_sca->server_record,
		"%s.resynchronize", network_sca->remote_record_name );

	mx_network_field_init( &(network_sca->sca_mode_nf),
		network_sca->server_record,
		"%s.sca_mode", network_sca->remote_record_name );

	mx_network_field_init( &(network_sca->time_constant_nf),
		network_sca->server_record,
		"%s.time_constant", network_sca->remote_record_name );

	mx_network_field_init( &(network_sca->upper_level_nf),
		network_sca->server_record,
		"%s.upper_level", network_sca->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_sca_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_sca_open()";

	MX_SCA *sca;
	MX_NETWORK_SCA *network_sca;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	sca = (MX_SCA *) record->record_class_struct;

	mx_status = mxd_network_sca_get_pointers(sca, &network_sca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_sca_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_sca_resynchronize()";

	MX_SCA *sca;
	MX_NETWORK_SCA *network_sca;
	mx_bool_type resynchronize;
	mx_status_type mx_status;

	network_sca = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	sca = (MX_SCA *) record->record_class_struct;

	mx_status = mxd_network_sca_get_pointers(sca, &network_sca, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	resynchronize = TRUE;

	mx_status = mx_put( &(network_sca->resynchronize_nf),
				MXFT_BOOL, &resynchronize );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_sca_get_parameter( MX_SCA *sca )
{
	static const char fname[] = "mxd_network_sca_get_parameter()";

	MX_NETWORK_SCA *network_sca;
	mx_status_type mx_status;

	network_sca = NULL;

	mx_status = mxd_network_sca_get_pointers( sca, &network_sca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sca->parameter_type == MXLV_SCA_LOWER_LEVEL ) {

		mx_status = mx_get( &(network_sca->lower_level_nf),
					MXFT_DOUBLE, &(sca->lower_level) );

	} else
	if ( sca->parameter_type == MXLV_SCA_UPPER_LEVEL ) {

		mx_status = mx_get( &(network_sca->upper_level_nf),
					MXFT_DOUBLE, &(sca->upper_level) );

	} else
	if ( sca->parameter_type == MXLV_SCA_GAIN ) {

		mx_status = mx_get( &(network_sca->gain_nf),
					MXFT_DOUBLE, &(sca->gain) );

	} else
	if ( sca->parameter_type == MXLV_SCA_TIME_CONSTANT ) {

		mx_status = mx_get( &(network_sca->time_constant_nf),
					MXFT_DOUBLE, &(sca->time_constant) );

	} else
	if ( sca->parameter_type == MXLV_SCA_MODE ) {

		mx_status = mx_get( &(network_sca->sca_mode_nf),
					MXFT_LONG, &(sca->sca_mode) );

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			sca->parameter_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_sca_set_parameter( MX_SCA *sca )
{
	static const char fname[] = "mxd_network_sca_set_parameter()";

	MX_NETWORK_SCA *network_sca;
	mx_status_type mx_status;

	network_sca = NULL;

	mx_status = mxd_network_sca_get_pointers( sca, &network_sca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sca->parameter_type == MXLV_SCA_LOWER_LEVEL ) {

		mx_status = mx_put( &(network_sca->lower_level_nf),
					MXFT_DOUBLE, &(sca->lower_level) );

	} else
	if ( sca->parameter_type == MXLV_SCA_UPPER_LEVEL ) {

		mx_status = mx_put( &(network_sca->upper_level_nf),
					MXFT_DOUBLE, &(sca->upper_level) );

	} else
	if ( sca->parameter_type == MXLV_SCA_GAIN ) {

		mx_status = mx_put( &(network_sca->gain_nf),
					MXFT_DOUBLE, &(sca->gain) );

	} else
	if ( sca->parameter_type == MXLV_SCA_TIME_CONSTANT ) {

		mx_status = mx_put( &(network_sca->time_constant_nf),
					MXFT_DOUBLE, &(sca->time_constant) );

	} else
	if ( sca->parameter_type == MXLV_SCA_MODE ) {

		mx_status = mx_put( &(network_sca->sca_mode_nf),
					MXFT_LONG, &(sca->sca_mode) );

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			sca->parameter_type );
	}

	return mx_status;
}


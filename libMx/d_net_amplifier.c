/*
 * Name:    d_net_amplifier.c
 *
 * Purpose: MX driver to control network amplifiers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_amplifier.h"
#include "d_net_amplifier.h"

/* Initialize the amplifier driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_network_amplifier_record_function_list = {
	NULL,
	mxd_network_amplifier_create_record_structures,
	mxd_network_amplifier_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_network_amplifier_resynchronize
};

MX_AMPLIFIER_FUNCTION_LIST mxd_network_amplifier_amplifier_function_list = {
	mxd_network_amplifier_get_gain,
	mxd_network_amplifier_set_gain,
	mxd_network_amplifier_get_offset,
	mxd_network_amplifier_set_offset,
	mxd_network_amplifier_get_time_constant,
	mxd_network_amplifier_set_time_constant,
	mxd_network_amplifier_get_parameter,
	mx_amplifier_default_set_parameter_handler
};

/* Network amplifier data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_network_amplifier_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_NETWORK_AMPLIFIER_STANDARD_FIELDS
};

mx_length_type mxd_network_amplifier_num_record_fields
		= sizeof( mxd_network_amplifier_record_field_defaults )
		  / sizeof( mxd_network_amplifier_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_amplifier_rfield_def_ptr
			= &mxd_network_amplifier_record_field_defaults[0];

/*=======================================================================*/

/* This function is a utility function to consolidate all of the
 * pointer mangling that often has to happen at the beginning of an 
 * mxd_network_amplifier_...  function.  The parameter 'calling_fname'
 * is passed so that error messages will appear with the name of
 * the calling function.
 */

MX_EXPORT mx_status_type
mxd_network_amplifier_get_pointers( MX_AMPLIFIER *amplifier,
			MX_NETWORK_AMPLIFIER **network_amplifier,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_amplifier_get_pointers()";

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AMPLIFIER pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( network_amplifier == (MX_NETWORK_AMPLIFIER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_AMPLIFIER pointer passed by '%s' was NULL",
			calling_fname );
	}

	*network_amplifier = (MX_NETWORK_AMPLIFIER *)
				amplifier->record->record_type_struct;

	if ( *network_amplifier == (MX_NETWORK_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_AMPLIFIER pointer for amplifier record '%s' "
		"passed by '%s' is NULL",
				amplifier->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/


MX_EXPORT mx_status_type
mxd_network_amplifier_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_amplifier_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_NETWORK_AMPLIFIER *network_amplifier;

	/* Allocate memory for the necessary structures. */

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AMPLIFIER structure." );
	}

	network_amplifier = (MX_NETWORK_AMPLIFIER *)
				malloc( sizeof(MX_NETWORK_AMPLIFIER) );

	if ( network_amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_AMPLIFIER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = amplifier;
	record->record_type_struct = network_amplifier;
	record->class_specific_function_list
			= &mxd_network_amplifier_amplifier_function_list;

	amplifier->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_amplifier_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_amplifier_finish_record_initialization()";

	MX_AMPLIFIER *amplifier;
	MX_NETWORK_AMPLIFIER *network_amplifier;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	amplifier = (MX_AMPLIFIER *) record->record_class_struct;

	mx_status = mxd_network_amplifier_get_pointers(
				amplifier, &network_amplifier, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_amplifier->gain_nf),
		network_amplifier->server_record,
		"%s.gain", network_amplifier->remote_record_name );

	mx_network_field_init( &(network_amplifier->gain_range_nf),
		network_amplifier->server_record,
		"%s.gain_range", network_amplifier->remote_record_name );

	mx_network_field_init( &(network_amplifier->offset_nf),
		network_amplifier->server_record,
		"%s.offset", network_amplifier->remote_record_name );

	mx_network_field_init( &(network_amplifier->time_constant_nf),
		network_amplifier->server_record,
		"%s.time_constant", network_amplifier->remote_record_name );

	mx_network_field_init( &(network_amplifier->resynchronize_nf),
		network_amplifier->server_record,
		"%s.resynchronize", network_amplifier->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_amplifier_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_amplifier_resynchronize()";

	MX_AMPLIFIER *amplifier;
	MX_NETWORK_AMPLIFIER *network_amplifier;
	int32_t resynchronize;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	amplifier = (MX_AMPLIFIER *) record->record_class_struct;

	mx_status = mxd_network_amplifier_get_pointers(
				amplifier, &network_amplifier, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	resynchronize = 1;

	mx_status = mx_put( &(network_amplifier->resynchronize_nf),
				MXFT_INT32, &resynchronize );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_amplifier_get_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_network_amplifier_get_gain()";

	MX_NETWORK_AMPLIFIER *network_amplifier;
	double gain;
	mx_status_type mx_status;

	mx_status = mxd_network_amplifier_get_pointers(
				amplifier, &network_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_amplifier->gain_nf),
				MXFT_DOUBLE, &gain );

	amplifier->gain = gain;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_amplifier_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_network_amplifier_set_gain()";

	MX_NETWORK_AMPLIFIER *network_amplifier;
	double new_gain;
	mx_status_type mx_status;

	mx_status = mxd_network_amplifier_get_pointers(
				amplifier, &network_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_gain = amplifier->gain;

	mx_status = mx_put( &(network_amplifier->gain_nf),
				MXFT_DOUBLE, &new_gain );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_amplifier_get_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_network_amplifier_get_offset()";

	MX_NETWORK_AMPLIFIER *network_amplifier;
	double offset;
	mx_status_type mx_status;

	mx_status = mxd_network_amplifier_get_pointers(
				amplifier, &network_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_amplifier->offset_nf),
				MXFT_DOUBLE, &offset );

	amplifier->offset = offset;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_amplifier_set_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_network_amplifier_set_offset()";

	MX_NETWORK_AMPLIFIER *network_amplifier;
	double new_offset;
	mx_status_type mx_status;

	mx_status = mxd_network_amplifier_get_pointers(
				amplifier, &network_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_offset = amplifier->offset;

	mx_status = mx_put( &(network_amplifier->offset_nf),
				MXFT_DOUBLE, &new_offset );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_amplifier_get_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_network_amplifier_get_time_constant()";

	MX_NETWORK_AMPLIFIER *network_amplifier;
	double time_constant;
	mx_status_type mx_status;

	mx_status = mxd_network_amplifier_get_pointers(
				amplifier, &network_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_amplifier->time_constant_nf),
				MXFT_DOUBLE, &time_constant );

	amplifier->time_constant = time_constant;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_amplifier_set_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_network_amplifier_set_time_constant()";

	MX_NETWORK_AMPLIFIER *network_amplifier;
	double new_time_constant;
	mx_status_type mx_status;

	mx_status = mxd_network_amplifier_get_pointers(
				amplifier, &network_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_time_constant = amplifier->time_constant;

	mx_status = mx_put( &(network_amplifier->time_constant_nf),
				MXFT_DOUBLE, &new_time_constant );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_amplifier_get_parameter( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_network_amplifier_get_parameter()";

	MX_NETWORK_AMPLIFIER *network_amplifier;
	mx_length_type dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxd_network_amplifier_get_pointers(
				amplifier, &network_amplifier, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
		("%s invoked for amplifier '%s' for parameter type '%s' (%d).",
		fname, amplifier->record->name,
		mx_get_field_label_string( amplifier->record,
						amplifier->parameter_type ),
		amplifier->parameter_type));

	switch( amplifier->parameter_type ) {
	case MXLV_AMP_GAIN_RANGE:
		dimension_array[0] = MX_AMPLIFIER_NUM_GAIN_RANGE_PARAMS;

		mx_status = mx_get_array(
				&(network_amplifier->gain_range_nf),
				MXFT_DOUBLE, 1, dimension_array,
				amplifier->gain_range );
		break;

	default:
		return mx_amplifier_default_get_parameter_handler( amplifier );
	}

	return mx_status;
}


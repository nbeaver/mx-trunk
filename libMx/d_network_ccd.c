/*
 * Name:    d_network_ccd.c
 *
 * Purpose: MX network CCD driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_ccd.h"

#include "d_network_ccd.h"

MX_RECORD_FUNCTION_LIST mxd_network_ccd_record_function_list = {
	NULL,
	mxd_network_ccd_create_record_structures,
	mxd_network_ccd_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_network_ccd_open
};

MX_CCD_FUNCTION_LIST mxd_network_ccd_ccd_function_list = {
	mxd_network_ccd_start,
	mxd_network_ccd_stop,
	mxd_network_ccd_get_status,
	mxd_network_ccd_readout,
	mxd_network_ccd_dezinger,
	mxd_network_ccd_correct,
	mxd_network_ccd_writefile,
	mxd_network_ccd_get_parameter,
	mxd_network_ccd_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_network_ccd_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CCD_STANDARD_FIELDS,
	MXD_NETWORK_CCD_STANDARD_FIELDS
};

long mxd_network_ccd_num_record_fields
		= sizeof( mxd_network_ccd_record_field_defaults )
			/ sizeof( mxd_network_ccd_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_ccd_rfield_def_ptr
			= &mxd_network_ccd_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_network_ccd_get_pointers( MX_CCD *ccd,
			MX_NETWORK_CCD **network_ccd,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_ccd_get_pointers()";

	MX_RECORD *network_ccd_record;

	if ( ccd == (MX_CCD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CCD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	network_ccd_record = ccd->record;

	if ( network_ccd_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_CCD pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	if ( network_ccd == (MX_NETWORK_CCD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_CCD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*network_ccd = (MX_NETWORK_CCD *)
				network_ccd_record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_network_ccd_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_ccd_create_record_structures()";

	MX_CCD *ccd;
	MX_NETWORK_CCD *network_ccd;

	ccd = (MX_CCD *) malloc( sizeof(MX_CCD) );

	if ( ccd == (MX_CCD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for a MX_CCD structure "
		"for record '%s'.", record->name );
	}

	network_ccd = (MX_NETWORK_CCD *) malloc( sizeof(MX_NETWORK_CCD) );

	if ( network_ccd == (MX_NETWORK_CCD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for a MX_NETWORK_CCD structure"
		"for record '%s'.", record->name );
	}

	record->record_class_struct = ccd;
	record->record_type_struct = network_ccd;

	record->class_specific_function_list =
			&mxd_network_ccd_ccd_function_list;

	ccd->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_ccd_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_ccd_finish_record_initialization()";

	MX_CCD *ccd;
	MX_NETWORK_CCD *network_ccd;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	ccd = (MX_CCD *) record->record_class_struct;

	mx_status = mxd_network_ccd_get_pointers(
				ccd, &network_ccd, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_ccd->bin_size_nf),
		network_ccd->server_record,
		"%s.bin_size", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->ccd_flags_nf),
		network_ccd->server_record,
		"%s.ccd_flags", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->correct_nf),
		network_ccd->server_record,
		"%s.correct", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->data_frame_size_nf),
		network_ccd->server_record,
		"%s.data_frame_size", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->dezinger_nf),
		network_ccd->server_record,
		"%s.dezinger", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->header_variable_contents_nf),
		network_ccd->server_record,
		"%s.header_variable_contents", network_ccd->remote_record_name);

	mx_network_field_init( &(network_ccd->header_variable_name_nf),
		network_ccd->server_record,
		"%s.header_variable_name", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->preset_time_nf),
		network_ccd->server_record,
		"%s.preset_time", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->readout_nf),
		network_ccd->server_record,
		"%s.readout", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->status_nf),
		network_ccd->server_record,
		"%s.status", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->stop_nf),
		network_ccd->server_record,
		"%s.stop", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->writefile_nf),
		network_ccd->server_record,
		"%s.writefile", network_ccd->remote_record_name );

	mx_network_field_init( &(network_ccd->writefile_name_nf),
		network_ccd->server_record,
		"%s.writefile_name", network_ccd->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_ccd_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_ccd_open()";

	MX_CCD *ccd;
	MX_NETWORK_CCD *network_ccd;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ccd = (MX_CCD *) record->record_class_struct;

	mx_status = mxd_network_ccd_get_pointers( ccd, &network_ccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_network_ccd_start( MX_CCD *ccd )
{
	static const char fname[] = "mxd_network_ccd_start()";

	MX_NETWORK_CCD *network_ccd;
	mx_status_type mx_status;

	mx_status = mxd_network_ccd_get_pointers( ccd, &network_ccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	mx_status = mx_put( &(network_ccd->preset_time_nf),
				MXFT_DOUBLE, &(ccd->preset_time) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_ccd_stop( MX_CCD *ccd )
{
	static const char fname[] = "mxd_network_ccd_stop()";

	MX_NETWORK_CCD *network_ccd;
	int stop;
	mx_status_type mx_status;

	mx_status = mxd_network_ccd_get_pointers( ccd, &network_ccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	stop = 1;

	mx_status = mx_put( &(network_ccd->stop_nf), MXFT_INT, &stop );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_ccd_get_status( MX_CCD *ccd )
{
	static const char fname[] = "mxd_network_ccd_get_status()";

	MX_NETWORK_CCD *network_ccd;
	unsigned long status;
	mx_status_type mx_status;

	mx_status = mxd_network_ccd_get_pointers( ccd, &network_ccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	mx_status = mx_get( &(network_ccd->status_nf), MXFT_HEX, &status );

	ccd->status = status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_ccd_readout( MX_CCD *ccd )
{
	static const char fname[] = "mxd_network_ccd_readout()";

	MX_NETWORK_CCD *network_ccd;
	mx_status_type mx_status;

	mx_status = mxd_network_ccd_get_pointers( ccd, &network_ccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	mx_status = mx_put( &(network_ccd->ccd_flags_nf),
				MXFT_HEX, &(ccd->ccd_flags) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_ccd->readout_nf),
				MXFT_INT, &(ccd->readout) );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_ccd_dezinger( MX_CCD *ccd )
{
	static const char fname[] = "mxd_network_ccd_dezinger()";

	MX_NETWORK_CCD *network_ccd;
	mx_status_type mx_status;

	mx_status = mxd_network_ccd_get_pointers( ccd, &network_ccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	mx_status = mx_put( &(network_ccd->ccd_flags_nf),
				MXFT_HEX, &(ccd->ccd_flags) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_ccd->dezinger_nf),
				MXFT_INT, &(ccd->dezinger) );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_ccd_correct( MX_CCD *ccd )
{
	static const char fname[] = "mxd_network_ccd_correct()";

	MX_NETWORK_CCD *network_ccd;
	int correct;
	mx_status_type mx_status;

	mx_status = mxd_network_ccd_get_pointers( ccd, &network_ccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	correct = 1;

	mx_status = mx_put( &(network_ccd->correct_nf), MXFT_INT, &correct );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_ccd_writefile( MX_CCD *ccd )
{
	static const char fname[] = "mxd_network_ccd_writefile()";

	MX_NETWORK_CCD *network_ccd;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_ccd_get_pointers( ccd, &network_ccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, ccd->record->name));

	mx_status = mx_put( &(network_ccd->ccd_flags_nf),
				MXFT_HEX, &(ccd->ccd_flags) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dimension[0] = MXU_FILENAME_LENGTH;

	mx_status = mx_put_array( &(network_ccd->writefile_name_nf),
					MXFT_STRING, 1, dimension,
					ccd->writefile_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_ccd->writefile_nf),
				MXFT_INT, &(ccd->writefile) );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_ccd_get_parameter( MX_CCD *ccd )
{
	static const char fname[] = "mxd_network_ccd_get_parameter()";

	MX_NETWORK_CCD *network_ccd;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_ccd_get_pointers( ccd,
						&network_ccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for CCD '%s', parameter type '%s' (%d)",
		fname, ccd->record->name,
		mx_get_field_label_string( ccd->record,
					ccd->parameter_type ),
		ccd->parameter_type));

	switch( ccd->parameter_type ) {
	case MXLV_CCD_DATA_FRAME_SIZE:
		dimension[0] = 2;

		mx_status = mx_get_array( &(network_ccd->data_frame_size_nf),
					MXFT_INT, 1, dimension,
					&(ccd->data_frame_size) );
		break;
	case MXLV_CCD_BIN_SIZE:
		dimension[0] = 2;

		mx_status = mx_get_array( &(network_ccd->bin_size_nf),
					MXFT_INT, 1, dimension,
					&(ccd->bin_size) );
		break;
	default:
		return mx_ccd_default_get_parameter_handler( ccd );
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_ccd_set_parameter( MX_CCD *ccd )
{
	static const char fname[] = "mxd_network_ccd_set_parameter()";

	MX_NETWORK_CCD *network_ccd;
	long dimension[1];
	mx_status_type mx_status;

	mx_status = mxd_network_ccd_get_pointers( ccd,
					&network_ccd, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for CCD '%s', parameter type '%s' (%d)",
		fname, ccd->record->name,
		mx_get_field_label_string( ccd->record,
					ccd->parameter_type ),
		ccd->parameter_type));

	switch( ccd->parameter_type ) {
	case MXLV_CCD_DATA_FRAME_SIZE:
		dimension[0] = 2;

		mx_status = mx_put_array( &(network_ccd->data_frame_size_nf),
						MXFT_INT, 1, dimension,
						&(ccd->data_frame_size) );
		break;
	case MXLV_CCD_HEADER_VARIABLE_NAME:
		dimension[0] = MXU_CCD_HEADER_NAME_LENGTH;

		mx_status = mx_put_array(
				&(network_ccd->header_variable_name_nf),
				MXFT_STRING, 1, dimension,
				&(ccd->header_variable_name) );

		break;
	case MXLV_CCD_HEADER_VARIABLE_CONTENTS:
		dimension[0] = MXU_BUFFER_LENGTH;

		mx_status = mx_put_array(
				&(network_ccd->header_variable_contents_nf),
				MXFT_STRING, 1, dimension,
				&(ccd->header_variable_contents) );

		break;
	default:
		return mx_ccd_default_set_parameter_handler( ccd );
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}


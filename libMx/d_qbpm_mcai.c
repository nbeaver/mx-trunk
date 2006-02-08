/*
 * Name:    d_qbpm_mcai.c
 *
 * Purpose: MX drivers to read out all 4 channels from an Oxford Danfysik
 *          QBPM beam position monitor at once.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_QBPM_MCAI_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_mcai.h"
#include "mx_amplifier.h"
#include "d_icplus.h"
#include "d_qbpm_mcai.h"

MX_RECORD_FUNCTION_LIST mxd_qbpm_mcai_record_function_list = {
	mxd_qbpm_mcai_initialize_type,
	mxd_qbpm_mcai_create_record_structures,
	mxd_qbpm_mcai_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_qbpm_mcai_open
};

MX_MCAI_FUNCTION_LIST
mxd_qbpm_mcai_mcai_function_list = {
	mxd_qbpm_mcai_read
};

MX_RECORD_FIELD_DEFAULTS mxd_qbpm_mcai_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCAI_STANDARD_FIELDS,
	MXD_QBPM_MCAI_STANDARD_FIELDS
};

mx_length_type mxd_qbpm_mcai_num_record_fields
		= sizeof( mxd_qbpm_mcai_record_field_defaults )
			/ sizeof( mxd_qbpm_mcai_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_qbpm_mcai_rfield_def_ptr
			= &mxd_qbpm_mcai_record_field_defaults[0];

static mx_status_type
mxd_qbpm_mcai_get_pointers( MX_MCAI *mcai,
			MX_QBPM_MCAI **qbpm_mcai,
			MX_ICPLUS **icplus,
			const char *calling_fname )
{
	static const char fname[] = "mxd_qbpm_mcai_get_pointers()";

	MX_RECORD *icplus_record;

	if ( mcai == (MX_MCAI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MCAI pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (qbpm_mcai == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_QBPM_MCAI pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (icplus == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ICPLUS pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mcai->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer for MX_MCAI pointer "
		"passed by '%s' was NULL.", calling_fname );
	}

	*qbpm_mcai = (MX_QBPM_MCAI *) mcai->record->record_type_struct;

	if ( *qbpm_mcai == (MX_QBPM_MCAI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_QBPM_MCAI pointer for record '%s' passed by '%s' is NULL.",
			mcai->record->name, calling_fname );
	}

	icplus_record = (*qbpm_mcai)->icplus_record;

	if ( icplus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ICPLUS pointer for iSeries analog input "
		"record '%s' passed by '%s' is NULL.",
			mcai->record->name, calling_fname );
	}

	if ( icplus_record->mx_type != MXT_AMP_QBPM ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"icplus_record '%s' for QBPM multichannel analog input '%s' is "
		"not a QBPM record.  Instead, it is a '%s' record.",
			icplus_record->name, mcai->record->name,
			mx_get_driver_name( icplus_record ) );
	}

	*icplus = (MX_ICPLUS *) icplus_record->record_type_struct;

	if ( *icplus == (MX_ICPLUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ICPLUS pointer for 'icplus' record '%s' used by QBPM "
	"multichannel analog input record '%s' and passed by '%s' is NULL.",
			icplus_record->name,
			mcai->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_qbpm_mcai_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	mx_length_type num_record_fields;
	mx_length_type maximum_num_channels_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mcai_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
					&maximum_num_channels_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_qbpm_mcai_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_qbpm_mcai_create_record_structures()";

        MX_MCAI *mcai;
        MX_QBPM_MCAI *qbpm_mcai;

        /* Allocate memory for the necessary structures. */

        mcai = (MX_MCAI *)
			malloc(sizeof(MX_MCAI));

        if ( mcai == (MX_MCAI *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_MCAI structure." );
        }

        qbpm_mcai = (MX_QBPM_MCAI *)
				malloc( sizeof(MX_QBPM_MCAI) );

        if ( qbpm_mcai == (MX_QBPM_MCAI *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_QBPM_MCAI structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = mcai;
        record->record_type_struct = qbpm_mcai;
        record->class_specific_function_list
                                = &mxd_qbpm_mcai_mcai_function_list;

        mcai->record = record;
	qbpm_mcai->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_qbpm_mcai_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_mcai_finish_record_initialization( record );

        return mx_status;
}

MX_EXPORT mx_status_type
mxd_qbpm_mcai_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_qbpm_mcai_open()";

	MX_MCAI *mcai;
	MX_QBPM_MCAI *qbpm_mcai;
	MX_ICPLUS *icplus;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mcai = (MX_MCAI *) record->record_class_struct;

	mx_status = mxd_qbpm_mcai_get_pointers( mcai,
					&qbpm_mcai, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( qbpm_mcai->command, ":READ%d:CURRALL?", icplus->address );

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_qbpm_mcai_read( MX_MCAI *mcai )
{
	static const char fname[] = "mxd_qbpm_mcai_read()";

	MX_QBPM_MCAI *qbpm_mcai;
	MX_ICPLUS *icplus;
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_qbpm_mcai_get_pointers( mcai,
					&qbpm_mcai, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_icplus_command( icplus, qbpm_mcai->command,
					response, sizeof( response ),
					MXD_QBPM_MCAI_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lg %lg %lg %lg",
				&(mcai->channel_array[0]),
				&(mcai->channel_array[1]),
				&(mcai->channel_array[2]),
				&(mcai->channel_array[3]) );

	if ( num_items != 4 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not find 4 number in the response to an '%s' "
		"command to QBPM controller '%s' for record '%s'.  "
		"Response = '%s'",
			qbpm_mcai->command,
			icplus->record->name,
			mcai->record->name,
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}


/*
 * Name:    d_i404_mcai.c
 *
 * Purpose: MX drivers to read out all 4 channels from a Pyramid Technical
 *          Consultants I404 digital electrometer at once.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_I404_MCAI_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_mcai.h"
#include "i_i404.h"
#include "d_i404_mcai.h"

MX_RECORD_FUNCTION_LIST mxd_i404_mcai_record_function_list = {
	mxd_i404_mcai_initialize_driver,
	mxd_i404_mcai_create_record_structures,
	mxd_i404_mcai_finish_record_initialization
};

MX_MCAI_FUNCTION_LIST
mxd_i404_mcai_mcai_function_list = {
	mxd_i404_mcai_read
};

MX_RECORD_FIELD_DEFAULTS mxd_i404_mcai_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCAI_STANDARD_FIELDS,
	MXD_I404_MCAI_STANDARD_FIELDS
};

long mxd_i404_mcai_num_record_fields
		= sizeof( mxd_i404_mcai_record_field_defaults )
			/ sizeof( mxd_i404_mcai_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_i404_mcai_rfield_def_ptr
			= &mxd_i404_mcai_record_field_defaults[0];

static mx_status_type
mxd_i404_mcai_get_pointers( MX_MCAI *mcai,
			MX_I404_MCAI **i404_mcai,
			MX_I404 **i404,
			const char *calling_fname )
{
	static const char fname[] = "mxd_i404_mcai_get_pointers()";

	MX_RECORD *i404_record;

	if ( mcai == (MX_MCAI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MCAI pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (i404_mcai == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_I404_MCAI pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (i404 == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_I404 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mcai->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer for MX_MCAI pointer "
		"passed by '%s' was NULL.", calling_fname );
	}

	*i404_mcai = (MX_I404_MCAI *) mcai->record->record_type_struct;

	if ( *i404_mcai == (MX_I404_MCAI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_I404_MCAI pointer for record '%s' passed by '%s' is NULL.",
			mcai->record->name, calling_fname );
	}

	i404_record = (*i404_mcai)->i404_record;

	if ( i404_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_I404 pointer for iSeries analog input "
		"record '%s' passed by '%s' is NULL.",
			mcai->record->name, calling_fname );
	}

	if ( i404_record->mx_type != MXI_CTRL_I404 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"i404_record '%s' for I404 multichannel analog input '%s' is "
		"not a I404 record.  Instead, it is a '%s' record.",
			i404_record->name, mcai->record->name,
			mx_get_driver_name( i404_record ) );
	}

	*i404 = (MX_I404 *) i404_record->record_type_struct;

	if ( *i404 == (MX_I404 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_I404 pointer for 'i404' record '%s' used by I404 "
	"multichannel analog input record '%s' and passed by '%s' is NULL.",
			i404_record->name,
			mcai->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_i404_mcai_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_channels_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mcai_initialize_driver( driver,
					&maximum_num_channels_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_i404_mcai_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_i404_mcai_create_record_structures()";

        MX_MCAI *mcai;
        MX_I404_MCAI *i404_mcai;

        /* Allocate memory for the necessary structures. */

        mcai = (MX_MCAI *)
			malloc(sizeof(MX_MCAI));

        if ( mcai == (MX_MCAI *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_MCAI structure." );
        }

        i404_mcai = (MX_I404_MCAI *) malloc( sizeof(MX_I404_MCAI) );

        if ( i404_mcai == (MX_I404_MCAI *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_I404_MCAI structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = mcai;
        record->record_type_struct = i404_mcai;
        record->class_specific_function_list
                                = &mxd_i404_mcai_mcai_function_list;

        mcai->record = record;
	i404_mcai->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_i404_mcai_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_i404_mcai_finish_record_initialization()";

	MX_MCAI *mcai;
	MX_I404_MCAI *i404_mcai;
	MX_I404 *i404;
	char *name;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mcai = record->record_class_struct;

	mx_status = mx_mcai_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_i404_mcai_get_pointers( mcai,
					&i404_mcai, &i404, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	name = i404_mcai->mcai_type_name;

	if ( strcmp( name, "charge" ) == 0 ) {
		i404_mcai->mcai_type = MXT_I404_MCAI_CHARGE;

		strlcpy( i404_mcai->command, "READ:CHARGE?",
			sizeof(i404_mcai->command) );
	} else
	if ( strcmp( name, "current" ) == 0 ) {
		i404_mcai->mcai_type = MXT_I404_MCAI_CURRENT;

		strlcpy( i404_mcai->command, "READ:CURRENT?",
			sizeof(i404_mcai->command) );
	} else
	if ( strcmp( name, "digital" ) == 0 ) {
		i404_mcai->mcai_type = MXT_I404_MCAI_DIGITAL;

		strlcpy( i404_mcai->command, "READ:DIGITAL?",
			sizeof(i404_mcai->command) );
	} else
	if ( strcmp( name, "high_voltage" ) == 0 ) {
		i404_mcai->mcai_type = MXT_I404_MCAI_HIGH_VOLTAGE;

		strlcpy( i404_mcai->command, "READ:HIVOLTAGE?",
			sizeof(i404_mcai->command) );
	} else
	if ( strcmp( name, "position" ) == 0 ) {
		i404_mcai->mcai_type = MXT_I404_MCAI_POSITION;

		strlcpy( i404_mcai->command, "READ:POSITION?",
			sizeof(i404_mcai->command) );
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal MCAI type '%s' specified for '%s'.",
			name, mcai->record->name );
	}

        return mx_status;
}

MX_EXPORT mx_status_type
mxd_i404_mcai_read( MX_MCAI *mcai )
{
	static const char fname[] = "mxd_i404_mcai_read()";

	MX_I404_MCAI *i404_mcai;
	MX_I404 *i404;
	int num_items;
	char response[80];
	double integration_time;
	unsigned long overrange_flags;
	mx_status_type mx_status;

	mx_status = mxd_i404_mcai_get_pointers( mcai,
					&i404_mcai, &i404, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_i404_command( i404, i404_mcai->command,
					response, sizeof( response ),
					MXD_I404_MCAI_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( i404_mcai->mcai_type ) {
	case MXT_I404_MCAI_CHARGE:
		num_items = sscanf( response,
				"%lg S,%lg C,%lg C,%lg C,%lg C,%lu",
				&integration_time,
				&(mcai->channel_array[0]),
				&(mcai->channel_array[1]),
				&(mcai->channel_array[2]),
				&(mcai->channel_array[3]),
				&overrange_flags );
		break;

	case MXT_I404_MCAI_CURRENT:
		num_items = sscanf( response,
				"%lg S,%lg A,%lg A,%lg A,%lg A,%lu",
				&integration_time,
				&(mcai->channel_array[0]),
				&(mcai->channel_array[1]),
				&(mcai->channel_array[2]),
				&(mcai->channel_array[3]),
				&overrange_flags );
		break;

	case MXT_I404_MCAI_DIGITAL:
	case MXT_I404_MCAI_HIGH_VOLTAGE:
		num_items = sscanf( response, "%lg",
				&(mcai->channel_array[0]) );
		break;

	case MXT_I404_MCAI_POSITION:
		num_items = sscanf( response, "%lg,%lg",
				&(mcai->channel_array[0]),
				&(mcai->channel_array[1]) );
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal MCAI type '%s' specified for '%s'.",
			i404_mcai->mcai_type_name,
			mcai->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}


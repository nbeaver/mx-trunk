/*
 * Name:    d_network_mca.c
 *
 * Purpose: MX multichannel analyzer driver for network MCAs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2006, 2010, 2012 Illinois Institute of Technology
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
#include "mx_mca.h"
#include "mx_net.h"
#include "d_network_mca.h"

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_network_mca_record_function_list = {
	mxd_network_mca_initialize_driver,
	mxd_network_mca_create_record_structures,
	mxd_network_mca_finish_record_initialization,
	mxd_network_mca_delete_record,
	mxd_network_mca_print_structure,
	mxd_network_mca_open,
	mxd_network_mca_close,
	NULL,
	mxd_network_mca_resynchronize
};

MX_MCA_FUNCTION_LIST mxd_network_mca_mca_function_list = {
	mxd_network_mca_start,
	mxd_network_mca_stop,
	mxd_network_mca_read,
	mxd_network_mca_clear,
	mxd_network_mca_busy,
	mxd_network_mca_get_parameter,
	mxd_network_mca_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_network_mca_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_NETWORK_MCA_STANDARD_FIELDS
};

long mxd_network_mca_num_record_fields
		= sizeof( mxd_network_mca_record_field_defaults )
		  / sizeof( mxd_network_mca_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_mca_rfield_def_ptr
			= &mxd_network_mca_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_network_mca_get_pointers( MX_MCA *mca,
			MX_NETWORK_MCA **network_mca,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_mca_get_pointers()";

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The mca pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for mca pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( mca->record->mx_type != MXT_MCA_NETWORK ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The mca '%s' passed by '%s' is not a network mca.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			mca->record->name, calling_fname,
			mca->record->mx_superclass,
			mca->record->mx_class,
			mca->record->mx_type );
	}

	if ( network_mca != (MX_NETWORK_MCA **) NULL ) {

		*network_mca = (MX_NETWORK_MCA *)
				(mca->record->record_type_struct);

		if ( *network_mca == (MX_NETWORK_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_MCA pointer for mca record '%s' passed by '%s' is NULL",
				mca->record->name, calling_fname );
		}
	}

	if ( (*network_mca)->server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX server record pointer for MX network mca '%s' is NULL.",
			mca->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_network_mca_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_channels_varargs_cookie;
	long maximum_num_rois_varargs_cookie;
	long num_soft_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mca_initialize_driver( driver,
					&maximum_num_channels_varargs_cookie,
					&maximum_num_rois_varargs_cookie,
					&num_soft_rois_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mca_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_network_mca_create_record_structures()";

	MX_MCA *mca;
	MX_NETWORK_MCA *network_mca;

	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA structure." );
	}

	network_mca = (MX_NETWORK_MCA *) malloc( sizeof(MX_NETWORK_MCA) );

	if ( network_mca == (MX_NETWORK_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = network_mca;
	record->class_specific_function_list
		= &mxd_network_mca_mca_function_list;

	mca->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mca_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_network_mca_finish_record_initialization()";

	MX_MCA *mca;
	MX_NETWORK_MCA *network_mca;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_network_mca_get_pointers( mca, &network_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->channel_array = ( unsigned long * )
		malloc( mca->maximum_num_channels * sizeof( unsigned long ) );

	if ( mca->channel_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an %ld channel data array.",
			mca->maximum_num_channels );
	}

	mx_status = mx_mca_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_mca->busy_nf),
		network_mca->server_record,
		"%s.busy", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->channel_array_nf),
		network_mca->server_record,
		"%s.channel_array", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->channel_number_nf),
		network_mca->server_record,
		"%s.channel_number", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->channel_value_nf),
		network_mca->server_record,
		"%s.channel_value", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->clear_nf),
		network_mca->server_record,
		"%s.clear", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->current_num_channels_nf),
		network_mca->server_record,
		"%s.current_num_channels", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->current_num_rois_nf),
		network_mca->server_record,
		"%s.current_num_rois", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->energy_offset_nf),
		network_mca->server_record,
		"%s.energy_offset", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->energy_scale_nf),
		network_mca->server_record,
		"%s.energy_scale", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->input_count_rate_nf),
		network_mca->server_record,
		"%s.input_count_rate", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->live_time_nf),
		network_mca->server_record,
		"%s.live_time", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->new_data_available_nf),
		network_mca->server_record,
		"%s.new_data_available", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->num_soft_rois_nf),
		network_mca->server_record,
		"%s.num_soft_rois", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->output_count_rate_nf),
		network_mca->server_record,
		"%s.output_count_rate", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->preset_count_nf),
		network_mca->server_record,
		"%s.preset_count", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->preset_live_time_nf),
		network_mca->server_record,
		"%s.preset_live_time", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->preset_real_time_nf),
		network_mca->server_record,
		"%s.preset_real_time", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->preset_type_nf),
		network_mca->server_record,
		"%s.preset_type", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->real_time_nf),
		network_mca->server_record,
		"%s.real_time", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->resynchronize_nf),
		network_mca->server_record,
		"%s.resynchronize", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->roi_nf),
		network_mca->server_record,
		"%s.roi", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->roi_array_nf),
		network_mca->server_record,
		"%s.roi_array", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->roi_integral_nf),
		network_mca->server_record,
		"%s.roi_integral", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->roi_integral_array_nf),
		network_mca->server_record,
		"%s.roi_integral_array", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->roi_number_nf),
		network_mca->server_record,
		"%s.roi_number", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->soft_roi_nf),
		network_mca->server_record,
		"%s.soft_roi", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->soft_roi_array_nf),
		network_mca->server_record,
		"%s.soft_roi_array", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->soft_roi_integral_nf),
		network_mca->server_record,
		"%s.soft_roi_integral", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->soft_roi_integral_array_nf),
		network_mca->server_record,
		"%s.soft_roi_integral_array", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->soft_roi_number_nf),
		network_mca->server_record,
		"%s.soft_roi_number", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->start_nf),
		network_mca->server_record,
		"%s.start", network_mca->remote_record_name );

	mx_network_field_init( &(network_mca->stop_nf),
		network_mca->server_record,
		"%s.stop", network_mca->remote_record_name );

#if 0
	mca->mca_flags |= MXF_MCA_NO_READ_OPTIMIZATION;
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mca_delete_record( MX_RECORD *record )
{
	MX_MCA *mca;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {

		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	mca = (MX_MCA *) record->record_class_struct;

	if ( mca != NULL ) {

		if ( mca->channel_array != NULL ) {
			free( mca->channel_array );

			mca->channel_array = NULL;
		}
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mca_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_network_mca_print_structure()";

	MX_MCA *mca;
	MX_NETWORK_MCA *network_mca;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mca = (MX_MCA *) (record->record_class_struct);

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCA pointer for record '%s' is NULL.", record->name);
	}

	network_mca = (MX_NETWORK_MCA *) (record->record_type_struct);

	if ( network_mca == (MX_NETWORK_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_NETWORK_MCA pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "MCA parameters for record '%s':\n", record->name);

	fprintf(file, "  MCA type              = NETWORK_MCA.\n\n");
	fprintf(file, "  server                = %s\n",
					network_mca->server_record->name);
	fprintf(file, "  remote record         = %s\n",
					network_mca->remote_record_name);
	fprintf(file, "  maximum # of channels = %ld\n",
					mca->maximum_num_channels);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mca_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_mca_open()";

	MX_MCA *mca;
	MX_NETWORK_MCA *network_mca;
	unsigned long i;
	long remote_num_soft_rois;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_network_mca_get_pointers(mca, &network_mca, fname);

	if ( mx_status.code != MXE_SUCCESS ) {
		mca->busy = FALSE;

		return mx_status;
	}

	/* Set some reasonable defaults. */

	mca->current_num_channels = mca->maximum_num_channels;
	mca->current_num_rois = mca->maximum_num_rois;

	for ( i = 0; i < mca->maximum_num_rois; i++ ) {
		mca->roi_array[i][0] = 0;
		mca->roi_array[i][1] = 0;
	}

	mca->preset_type = MXF_MCA_PRESET_REAL_TIME;
	mca->parameter_type = 0;

	mca->roi[0] = 0;
	mca->roi[1] = 0;
	mca->roi_integral = 0;

	mca->channel_number = 0;
	mca->channel_value = 0;

	mca->real_time = 0.0;
	mca->live_time = 0.0;

	mca->preset_real_time = 0.0;
	mca->preset_live_time = 0.0;

	/* Check to see if the remote value of 'num_soft_rois' is smaller
	 * than the local value.
	 */

	mx_status = mx_get( &(network_mca->num_soft_rois_nf),
				MXFT_LONG, &remote_num_soft_rois );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( remote_num_soft_rois < mca->num_soft_rois ) {

		/* If the remote record has fewer soft ROIs than the
		 * local record, unilaterally reduce the local value
		 * 'num_soft_rois'.  This wastes local memory, but
		 * is safer.
		 */

		mx_warning(
	"'num_soft_rois' (%ld) for record '%s' is larger than the value of "
	"remote record field '%s' (%ld) on server '%s'.  The local value of "
	"'num_soft_rois' will be reduced to %ld.",
			mca->num_soft_rois, record->name,
			network_mca->num_soft_rois_nf.nfname,
			remote_num_soft_rois,
			network_mca->server_record->name,
			remote_num_soft_rois );

		mca->num_soft_rois = remote_num_soft_rois;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mca_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mca_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_mca_resynchronize()";

	MX_MCA *mca;
	MX_NETWORK_MCA *network_mca;
	mx_bool_type resynchronize;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_network_mca_get_pointers(mca, &network_mca, fname);

	if ( mx_status.code != MXE_SUCCESS ) {
		mca->busy = FALSE;

		return mx_status;
	}

	resynchronize = TRUE;

	mx_status = mx_put( &(network_mca->resynchronize_nf),
				MXFT_BOOL, &resynchronize );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mca_start( MX_MCA *mca )
{
	static const char fname[] = "mxd_network_mca_start()";

	MX_NETWORK_MCA *network_mca;
	mx_bool_type start;
	mx_status_type mx_status;

	mx_status = mxd_network_mca_get_pointers( mca, &network_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the preset type. */

	mx_status = mx_put( &(network_mca->preset_type_nf),
				MXFT_LONG, &(mca->preset_type) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the preset interval appropriately depending on the
	 * preset type.
	 */

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_NONE:
		/* Do nothing. */

		break;
	case MXF_MCA_PRESET_LIVE_TIME:
		mx_status = mx_put( &(network_mca->preset_live_time_nf),
				MXFT_DOUBLE, &(mca->preset_live_time) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXF_MCA_PRESET_REAL_TIME:
		mx_status = mx_put( &(network_mca->preset_real_time_nf),
				MXFT_DOUBLE, &(mca->preset_real_time) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported preset type %ld for MCA '%s'.",
			mca->preset_type, mca->record->name );
	}

	/* Tell the counting to start. */

	start = TRUE;

	mx_status = mx_put( &(network_mca->start_nf), MXFT_BOOL, &start );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mca_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_network_mca_stop()";

	MX_NETWORK_MCA *network_mca;
	mx_bool_type stop;
	mx_status_type mx_status;

	mx_status = mxd_network_mca_get_pointers( mca, &network_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop = TRUE;

	mx_status = mx_put( &(network_mca->stop_nf), MXFT_BOOL, &stop );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mca_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_network_mca_read()";

	MX_NETWORK_MCA *network_mca;
	long dimension_array[1];
	unsigned long num_channels;
	mx_status_type mx_status;

	mx_status = mxd_network_mca_get_pointers( mca, &network_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mca_get_num_channels( mca->record, &num_channels );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dimension_array[0] = (long) num_channels;

	mx_status = mx_get_array( &(network_mca->channel_array_nf),
				MXFT_ULONG, 1, dimension_array,
				mca->channel_array );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mca_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_network_mca_clear()";

	MX_NETWORK_MCA *network_mca;
	mx_bool_type clear;
	mx_status_type mx_status;

	mx_status = mxd_network_mca_get_pointers( mca, &network_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	clear = TRUE;

	mx_status = mx_put( &(network_mca->clear_nf), MXFT_BOOL, &clear );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mca_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_network_mca_busy()";

	MX_NETWORK_MCA *network_mca;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_network_mca_get_pointers( mca, &network_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_mca->busy_nf), MXFT_BOOL, &busy );

	mca->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mca_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_network_mca_get_parameter()";

	MX_NETWORK_MCA *network_mca;
	long dimension_array[2];
	mx_status_type mx_status;

	mx_status = mxd_network_mca_get_pointers( mca, &network_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s' for parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string( mca->record,
			mca->parameter_type ),
		mca->parameter_type));

	if ( mca->parameter_type == MXLV_MCA_CURRENT_NUM_CHANNELS ) {

		mx_status = mx_get( &(network_mca->current_num_channels_nf),
				MXFT_LONG, &(mca->current_num_channels) );

		if ( mca->current_num_channels > mca->maximum_num_channels ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The MCA '%s' controlled by server '%s' is reported to have %ld channels, "
"but the record '%s' is only configured to support up to %ld channels.",
				network_mca->remote_record_name,
				network_mca->server_record->name,
				mca->current_num_channels,
				mca->record->name,
				mca->maximum_num_channels );
		}
	} else
	if ( mca->parameter_type == MXLV_MCA_NEW_DATA_AVAILABLE ) {

		mx_status = mx_get( &(network_mca->new_data_available_nf),
					MXFT_ULONG, &(mca->new_data_available));
	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_COUNT ) {

		mx_status = mx_get( &(network_mca->preset_count_nf),
					MXFT_ULONG, &(mca->preset_count) );
	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_LIVE_TIME ) {

		mx_status = mx_get( &(network_mca->preset_live_time_nf),
					MXFT_DOUBLE, &(mca->preset_live_time) );
	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_REAL_TIME ) {

		mx_status = mx_get( &(network_mca->preset_real_time_nf),
					MXFT_DOUBLE, &(mca->preset_real_time) );
	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_TYPE ) {

		mx_status = mx_get( &(network_mca->preset_type_nf),
					MXFT_LONG, &(mca->preset_type) );
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_ARRAY ) {

		mx_status = mx_put( &(network_mca->current_num_rois_nf),
					MXFT_LONG, &(mca->current_num_rois) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = mca->current_num_rois;
		dimension_array[1] = 2;

		mx_status = mx_get_array( &(network_mca->roi_array_nf),
					MXFT_ULONG, 2, dimension_array,
					&(mca->roi_array) );
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_INTEGRAL_ARRAY ) {

		mx_status = mx_put( &(network_mca->current_num_rois_nf),
					MXFT_LONG, &(mca->current_num_rois) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = mca->current_num_rois;

		mx_status = mx_get_array( &(network_mca->roi_integral_array_nf),
						MXFT_ULONG, 1, dimension_array,
						&(mca->roi_integral_array) );
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI ) {

		mx_status = mx_put( &(network_mca->roi_number_nf),
					MXFT_ULONG, &(mca->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_get_array( &(network_mca->roi_nf),
					MXFT_ULONG, 1, dimension_array,
					&(mca->roi) );
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_INTEGRAL ) {

		mx_status = mx_put( &(network_mca->roi_number_nf),
					MXFT_ULONG, &(mca->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get( &(network_mca->roi_integral_nf),
					MXFT_ULONG, &(mca->roi_integral) );
	} else
	if ( mca->parameter_type == MXLV_MCA_SOFT_ROI_ARRAY ) {

		dimension_array[0] = mca->num_soft_rois;
		dimension_array[1] = 2;

		mx_status = mx_get_array( &(network_mca->soft_roi_array_nf),
					MXFT_ULONG, 2, dimension_array,
					&(mca->soft_roi_array) );
	} else
	if ( mca->parameter_type == MXLV_MCA_SOFT_ROI_INTEGRAL_ARRAY ) {

		dimension_array[0] = mca->num_soft_rois;

		mx_status = mx_get_array(
				&(network_mca->soft_roi_integral_array_nf),
				MXFT_ULONG, 1, dimension_array,
				&(mca->soft_roi_integral_array) );
	} else
	if ( mca->parameter_type == MXLV_MCA_SOFT_ROI ) {

		mx_status = mx_put( &(network_mca->soft_roi_number_nf),
					MXFT_ULONG, &(mca->soft_roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_get_array( &(network_mca->soft_roi_nf),
						MXFT_ULONG, 1, dimension_array,
						&(mca->soft_roi) );
	} else
	if ( mca->parameter_type == MXLV_MCA_SOFT_ROI_INTEGRAL ) {

		mx_status = mx_put( &(network_mca->soft_roi_number_nf),
					MXFT_ULONG, &(mca->soft_roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get( &(network_mca->soft_roi_integral_nf),
					MXFT_ULONG, &(mca->soft_roi_integral) );
	} else
	if ( mca->parameter_type == MXLV_MCA_CHANNEL_NUMBER ) {

		mx_status = mx_get( &(network_mca->channel_number_nf),
					MXFT_ULONG, &(mca->channel_number) );
	} else
	if ( mca->parameter_type == MXLV_MCA_CHANNEL_VALUE ) {

		mx_status = mx_get( &(network_mca->channel_value_nf),
					MXFT_ULONG, &(mca->channel_value) );
	} else
	if ( mca->parameter_type == MXLV_MCA_REAL_TIME ) {

		mx_status = mx_get( &(network_mca->real_time_nf),
					MXFT_DOUBLE, &(mca->real_time) );
	} else
	if ( mca->parameter_type == MXLV_MCA_LIVE_TIME ) {

		mx_status = mx_get( &(network_mca->live_time_nf),
					MXFT_DOUBLE, &(mca->live_time) );
	} else
	if ( mca->parameter_type == MXLV_MCA_ENERGY_SCALE ) {

		mx_status = mx_get( &(network_mca->energy_scale_nf),
					MXFT_DOUBLE, &(mca->energy_scale) );
	} else
	if ( mca->parameter_type == MXLV_MCA_ENERGY_OFFSET ) {

		mx_status = mx_get( &(network_mca->energy_offset_nf),
					MXFT_DOUBLE, &(mca->energy_offset) );
	} else
	if ( mca->parameter_type == MXLV_MCA_INPUT_COUNT_RATE ) {

		mx_status = mx_get( &(network_mca->input_count_rate_nf),
					MXFT_DOUBLE, &(mca->input_count_rate) );
	} else
	if ( mca->parameter_type == MXLV_MCA_OUTPUT_COUNT_RATE ) {

		mx_status = mx_get( &(network_mca->output_count_rate_nf),
					MXFT_DOUBLE, &(mca->output_count_rate));
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			mca->parameter_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mca_set_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_network_mca_set_parameter()";

	MX_NETWORK_MCA *network_mca;
	long dimension_array[2];
	mx_status_type mx_status;

	mx_status = mxd_network_mca_get_pointers( mca, &network_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s' for parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string( mca->record,
			mca->parameter_type ),
		mca->parameter_type));

	if ( mca->parameter_type == MXLV_MCA_PRESET_COUNT ) {

		mx_status = mx_put( &(network_mca->preset_count_nf),
					MXFT_ULONG, &(mca->preset_count) );
	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_LIVE_TIME ) {

		mx_status = mx_put( &(network_mca->preset_live_time_nf),
					MXFT_DOUBLE, &(mca->preset_live_time) );
	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_REAL_TIME ) {

		mx_status = mx_put( &(network_mca->preset_real_time_nf),
					MXFT_DOUBLE, &(mca->preset_real_time) );
	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_TYPE ) {

		mx_status = mx_put( &(network_mca->preset_type_nf),
					MXFT_LONG, &(mca->preset_type) );
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_ARRAY ) {

		mx_status = mx_put( &(network_mca->current_num_rois_nf),
					MXFT_LONG, &(mca->current_num_rois) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = mca->current_num_rois;
		dimension_array[1] = 2;

		mx_status = mx_put_array( &(network_mca->roi_array_nf),
						MXFT_ULONG, 2, dimension_array,
						&(mca->roi_array) );
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI ) {

		mx_status = mx_put( &(network_mca->roi_number_nf),
					MXFT_ULONG, &(mca->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_put_array( &(network_mca->roi_nf),
						MXFT_ULONG, 1, dimension_array,
						&(mca->roi) );
	} else
	if ( mca->parameter_type == MXLV_MCA_SOFT_ROI_ARRAY ) {

		dimension_array[0] = mca->num_soft_rois;
		dimension_array[1] = 2;

		mx_status = mx_put_array( &(network_mca->soft_roi_array_nf),
						MXFT_ULONG, 2, dimension_array,
						&(mca->soft_roi_array) );
	} else
	if ( mca->parameter_type == MXLV_MCA_SOFT_ROI ) {

		mx_status = mx_put( &(network_mca->soft_roi_number_nf),
					MXFT_ULONG, &(mca->soft_roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_put_array( &(network_mca->soft_roi_nf),
						MXFT_ULONG, 1, dimension_array,
						&(mca->soft_roi) );
	} else
	if ( mca->parameter_type == MXLV_MCA_CHANNEL_NUMBER ) {

		mx_status = mx_put( &(network_mca->channel_number_nf),
					MXFT_ULONG, &(mca->channel_number) );
	} else
	if ( mca->parameter_type == MXLV_MCA_ENERGY_SCALE ) {

		mx_status = mx_put( &(network_mca->energy_scale_nf),
					MXFT_DOUBLE, &(mca->energy_scale) );
	} else
	if ( mca->parameter_type == MXLV_MCA_ENERGY_OFFSET ) {

		mx_status = mx_put( &(network_mca->energy_offset_nf),
					MXFT_DOUBLE, &(mca->energy_offset) );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			mca->parameter_type );
	}

	return mx_status;
}


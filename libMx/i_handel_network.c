/*
 * Name:    i_handel_network.c
 *
 * Purpose: MX network device for communicating with an XIA DXP multichannel
 *          analyzer system that is controlled by a remote MX server using
 *          the 'xia_xerxes' driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2007, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_HANDEL_NETWORK_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_net.h"
#include "mx_mca.h"
#include "i_handel_network.h"
#include "d_handel_mca.h"

MX_RECORD_FUNCTION_LIST mxi_handel_network_record_function_list = {
	NULL,
	mxi_handel_network_create_record_structures,
	mxi_handel_network_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_handel_network_open,
	NULL,
	NULL,
	mxi_handel_network_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxi_handel_network_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_HANDEL_NETWORK_STANDARD_FIELDS
};

long mxi_handel_network_num_record_fields
		= sizeof( mxi_handel_network_record_field_defaults )
			/ sizeof( mxi_handel_network_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_handel_network_rfield_def_ptr
			= &mxi_handel_network_record_field_defaults[0];

static mx_status_type mxi_handel_network_set_data_available_flags(
					MX_HANDEL_NETWORK *handel_network,
					int flag_value );

static mx_status_type
mxi_handel_network_get_pointers( MX_MCA *mca,
			MX_HANDEL_MCA **handel_mca,
			MX_HANDEL_NETWORK **handel_network,
			const char *calling_fname )
{
	static const char fname[] = "mxi_handel_network_get_pointers()";

	MX_RECORD *handel_record;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( handel_mca == (MX_HANDEL_MCA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_HANDEL_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( handel_network == (MX_HANDEL_NETWORK **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_HANDEL_NETWORK pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*handel_mca = (MX_HANDEL_MCA *) mca->record->record_type_struct;

	handel_record = (*handel_mca)->handel_record;

	if ( handel_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The handel_record pointer for MCA record '%s' is NULL.",
			mca->record->name );
	}

	*handel_network = (MX_HANDEL_NETWORK *) handel_record->record_type_struct;

	if ( (*handel_network) == (MX_HANDEL_NETWORK *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL_NETWORK pointer for XIA DXP record '%s' is NULL.",
			handel_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_handel_network_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_handel_network_create_record_structures()";

	MX_HANDEL_NETWORK *handel_network;

	/* Allocate memory for the necessary structures. */

	handel_network = (MX_HANDEL_NETWORK *) malloc( sizeof(MX_HANDEL_NETWORK) );

	if ( handel_network == (MX_HANDEL_NETWORK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_HANDEL_NETWORK structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;;
	record->record_type_struct = handel_network;

	record->class_specific_function_list = NULL;

	handel_network->record = record;

	handel_network->last_measurement_interval = -1.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_network_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_handel_network_finish_record_initialization()";

	MX_HANDEL_NETWORK *handel_network;

	handel_network = (MX_HANDEL_NETWORK *) record->record_type_struct;

	if ( handel_network == (MX_HANDEL_NETWORK *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL_NETWORK pointer for record '%s' is NULL.",
			record->name );
	}

	handel_network->num_mcas = 0;

	mx_network_field_init( &(handel_network->num_mcas_nf),
		handel_network->server_record,
		"%s.num_mcas", handel_network->remote_record_name );

	mx_network_field_init( &(handel_network->resynchronize_nf),
		handel_network->server_record,
		"%s.resynchronize", handel_network->remote_record_name );

	/* We cannot allocate the following structures until
	 * mxi_handel_network_open() is invoked, so for now we
	 * initialize them to NULL pointers.
	 */

	handel_network->mca_record_array = NULL;
	handel_network->busy_nf = NULL;
	handel_network->channel_array_nf = NULL;
	handel_network->current_num_channels_nf = NULL;
	handel_network->current_num_rois_nf = NULL;
	handel_network->hardware_scas_are_enabled_nf = NULL;
	handel_network->live_time_nf = NULL;
	handel_network->new_data_available_nf = NULL;
	handel_network->new_statistics_available_nf = NULL;
	handel_network->param_value_to_all_channels_nf = NULL;
	handel_network->parameter_name_nf = NULL;
	handel_network->parameter_value_nf = NULL;
	handel_network->preset_clock_tick_nf = NULL;
	handel_network->preset_type_nf = NULL;
	handel_network->real_time_nf = NULL;
	handel_network->roi_nf = NULL;
	handel_network->roi_array_nf = NULL;
	handel_network->roi_integral_nf = NULL;
	handel_network->roi_integral_array_nf = NULL;
	handel_network->roi_number_nf = NULL;
	handel_network->runtime_clock_tick_nf = NULL;
	handel_network->soft_roi_nf = NULL;
	handel_network->soft_roi_array_nf = NULL;
	handel_network->soft_roi_integral_nf = NULL;
	handel_network->soft_roi_integral_array_nf = NULL;
	handel_network->soft_roi_number_nf = NULL;
	handel_network->start_with_preset_nf = NULL;
	handel_network->statistics_nf = NULL;
	handel_network->stop_nf = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/* mxi_handel_network_set_data_available_flags() sets mca->new_data_available
 * and handel_mca->new_statistics_available for all of the MCAs controlled
 * by this XIA remote network interface.
 */

static mx_status_type
mxi_handel_network_set_data_available_flags( MX_HANDEL_NETWORK *handel_network,
					int flag_value )
{
	static const char fname[] =
		"mxi_handel_network_set_data_available_flags()";

	MX_RECORD *mca_record, **mca_record_array;
	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca;
	unsigned long i;

	if ( handel_network == (MX_HANDEL_NETWORK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL_NETWORK pointer passed was NULL." );
	}

	mca_record_array = handel_network->mca_record_array;

	if ( mca_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mca_record_array pointer for 'handel_network' record '%s'.",
			handel_network->record->name );
	}

	for ( i = 0; i < handel_network->num_mcas; i++ ) {

		mca_record = (handel_network->mca_record_array)[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			/* Skip this MCA slot. */

			continue;
		}

		mca = (MX_MCA *) mca_record->record_class_struct;

		if ( mca == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		handel_mca = (MX_HANDEL_MCA *) mca_record->record_type_struct;

		if ( handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_HANDEL_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		mca->new_data_available = flag_value;
		handel_mca->new_statistics_available = flag_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

#define MXI_HANDEL_NETWORK_NF_NULL( x, s ) \
	do { 								\
		(x) = ( MX_NETWORK_FIELD * ) 				\
		    malloc( num_mcas * sizeof(MX_NETWORK_FIELD) );	\
									\
		if ( (x) == (MX_NETWORK_FIELD *) NULL) {		\
			return mx_error( MXE_OUT_OF_MEMORY, fname,	\
			"Ran out of memory trying to allocate a %d "	\
			"element array of MX_NETWORK_FIELD pointers "	\
			"for the %s data structure of record '%s'.", 	\
				num_mcas, (s), record->name );		\
		}							\
									\
		for ( i = 0; i < num_mcas; i++ ) {			\
			mx_network_field_init(				\
				&( (x) [i]), NULL, "" );		\
		}							\
	} while(0)

MX_EXPORT mx_status_type
mxi_handel_network_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_handel_network_open()";

	MX_HANDEL_NETWORK *handel_network;
	int i, num_mcas;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	handel_network = (MX_HANDEL_NETWORK *) (record->record_type_struct);

	if ( handel_network == (MX_HANDEL_NETWORK *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_HANDEL_NETWORK pointer for record '%s' is NULL.",
			record->name);
	}

	/* Find out how many detector channels (MCAs) there are. */

	mx_status = mx_get(&(handel_network->num_mcas_nf), MXFT_ULONG, &num_mcas );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate and initialize an array to store pointers to all of
	 * the MCA records used by this interface.
	 */

	handel_network->num_mcas = num_mcas;

	handel_network->mca_record_array = ( MX_RECORD ** ) malloc(
					num_mcas * sizeof( MX_RECORD * ) );

	if ( handel_network->mca_record_array == ( MX_RECORD ** ) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element array "
		"of MX_RECORD pointers for the mca_record_array data "
		"structure of record '%s'.", num_mcas, record->name );
	}

	for ( i = 0; i < num_mcas; i++ ) {
		handel_network->mca_record_array[i] = NULL;
	}

	MXI_HANDEL_NETWORK_NF_NULL( handel_network->busy_nf,
					"busy_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->channel_array_nf,
					"channel_array_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->channel_number_nf,
					"channel_number_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->channel_value_nf,
					"channel_value_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->current_num_channels_nf,
					"current_num_channels_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->current_num_rois_nf,
					"current_num_rois_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->hardware_scas_are_enabled_nf,
					"hardware_scas_are_enabled_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->live_time_nf,
					"live_time_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->new_data_available_nf,
					"new_data_available_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->new_statistics_available_nf,
					"new_statistics_available_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->param_value_to_all_channels_nf,
					"param_value_to_all_channels_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->parameter_name_nf,
					"parameter_name_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->parameter_value_nf,
					"parameter_value_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->preset_clock_tick_nf,
					"preset_clock_tick_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->preset_type_nf,
					"preset_type_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->real_time_nf,
					"real_time_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->roi_nf,
					"roi_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->roi_array_nf,
					"roi_array_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->roi_integral_nf,
					"roi_integral_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->roi_integral_array_nf,
					"roi_integral_array_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->roi_number_nf,
					"roi_number_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->soft_roi_nf,
					"soft_roi_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->soft_roi_array_nf,
					"soft_roi_array_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->soft_roi_integral_nf,
					"soft_roi_integral_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->soft_roi_integral_array_nf,
					"soft_roi_integral_array_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->soft_roi_number_nf,
					"soft_roi_number_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->runtime_clock_tick_nf,
					"runtime_clock_tick_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->start_with_preset_nf,
					"start_with_preset_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->statistics_nf,
					"statistics_nf" );
	MXI_HANDEL_NETWORK_NF_NULL( handel_network->stop_nf,
					"stop_nf" );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_network_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_handel_network_resynchronize()";

	MX_HANDEL_NETWORK *handel_network;
	mx_bool_type resynchronize;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	handel_network = (MX_HANDEL_NETWORK *) (record->record_type_struct);

	if ( handel_network == (MX_HANDEL_NETWORK *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_HANDEL_NETWORK pointer for record '%s' is NULL.",
			record->name );
	}

	resynchronize = TRUE;

	mx_status = mx_put( &(handel_network->resynchronize_nf),
				MXFT_BOOL, &resynchronize );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_network_is_busy( MX_MCA *mca, mx_bool_type *busy_flag )
{
	static const char fname[] = "mxi_handel_network_is_busy()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_NETWORK *handel_network;
	mx_bool_type busy;
	long i;
	mx_status_type mx_status;

	mx_status = mxi_handel_network_get_pointers( mca,
					&handel_mca, &handel_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = handel_mca->detector_channel;

	mx_status = mx_get( &(handel_network->busy_nf[i]), MXFT_BOOL, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		mca->busy = TRUE;
	} else {
		mca->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_network_get_run_data( MX_MCA *mca,
			char *run_data_name,
			void *value_ptr )
{
	static const char fname[] = "mxi_handel_network_get_run_data()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented." );
}

MX_EXPORT mx_status_type
mxi_handel_network_get_acquisition_values( MX_MCA *mca,
				char *value_name,
				double *value_ptr )
{
	static const char fname[] =
		"mxi_handel_network_get_acquisition_values()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented." );
}

MX_EXPORT mx_status_type
mxi_handel_network_set_acquisition_values( MX_MCA *mca,
				char *value_name,
				double *value_ptr,
				mx_bool_type apply_flag )
{
	static const char fname[] =
		"mxi_handel_network_set_acquisition_values()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented." );
}

MX_EXPORT mx_status_type
mxi_handel_network_set_acq_for_all_channels( MX_MCA *mca,
				char *value_name,
				double *value_ptr,
				mx_bool_type apply_flag )
{
	static const char fname[] =
		"mxi_handel_network_set_acq_for_all_channels()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented." );
}

MX_EXPORT mx_status_type
mxi_handel_network_apply( MX_MCA *mca, mx_bool_type apply_to_all )
{
	static const char fname[] = "mxi_handel_network_apply()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented." );
}

MX_EXPORT mx_status_type
mxi_handel_network_read_parameter( MX_MCA *mca,
			char *parameter_name,
			unsigned long *value_ptr )
{
	static const char fname[] = "mxi_handel_network_read_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_NETWORK *handel_network;
	long i, dimension_array[1];
	unsigned long parameter_value;
	mx_status_type mx_status;

	mx_status = mxi_handel_network_get_pointers( mca,
					&handel_mca, &handel_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = handel_mca->detector_channel;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %ld, parameter '%s'.",
			fname, mca->record->name,
			handel_mca->detector_channel, parameter_name));
	}

	dimension_array[0] = (long) strlen( parameter_name ) + 1L;

	mx_status = mx_put_array( &(handel_network->parameter_name_nf[i]),
					MXFT_STRING, 1, dimension_array,
					parameter_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(handel_network->parameter_value_nf[i]),
					MXFT_ULONG, &parameter_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*value_ptr = (uint32_t) parameter_value;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: value = %lu",
			fname, (unsigned long) *value_ptr));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_network_write_parameter( MX_MCA *mca,
			char *parameter_name,
			unsigned long value )
{
	static const char fname[] = "mxi_handel_network_write_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_NETWORK *handel_network;
	long i, dimension_array[1];
	unsigned long parameter_value;
	mx_status_type mx_status;

	mx_status = mxi_handel_network_get_pointers( mca,
					&handel_mca, &handel_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = handel_mca->detector_channel;

	parameter_value = (unsigned long) value;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %ld, parameter '%s', value = %lu.",
			fname, mca->record->name, handel_mca->detector_channel,
			parameter_name, parameter_value));
	}

	dimension_array[0] = (long) strlen( parameter_name ) + 1L;

	mx_status = mx_put_array( &(handel_network->parameter_name_nf[i]),
					MXFT_STRING, 1, dimension_array,
					parameter_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(handel_network->parameter_value_nf[i]),
					MXFT_ULONG, &parameter_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Write to parameter '%s' succeeded.",
			fname, parameter_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_network_write_param_to_all_channels( MX_MCA *mca,
			char *parameter_name,
			unsigned long value )
{
	static const char fname[] =
			"mxi_handel_network_write_param_to_all_channels()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_NETWORK *handel_network;
	long i, dimension_array[1];
	unsigned long parameter_value;
	mx_status_type mx_status;

	mx_status = mxi_handel_network_get_pointers( mca,
					&handel_mca, &handel_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = handel_mca->detector_channel;

	parameter_value = (unsigned long) value;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %ld, parameter '%s', value = %lu.",
			fname, mca->record->name, handel_mca->detector_channel,
			parameter_name, parameter_value));
	}

	dimension_array[0] = (long) strlen( parameter_name ) + 1L;

	mx_status = mx_put_array( &(handel_network->parameter_name_nf[i]),
					MXFT_STRING, 1, dimension_array,
					parameter_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(handel_network->param_value_to_all_channels_nf[i]),
					MXFT_ULONG, &parameter_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Write to parameter '%s' succeeded.",
			fname, parameter_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_network_start_run( MX_MCA *mca, mx_bool_type clear_flag )
{
	static const char fname[] = "mxi_handel_network_start_run()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_NETWORK *handel_network;
	long i, dimension_array[1];
	double parameter_array[2];
	mx_status_type mx_status;

	mx_status = mxi_handel_network_get_pointers( mca,
					&handel_mca, &handel_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = handel_mca->detector_channel;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s', clear_flag = %d.",
				fname, mca->record->name, (int) clear_flag));
	}

	dimension_array[0] = 2;

	parameter_array[0] = (double) mca->preset_type;

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_LIVE_TIME:
		parameter_array[1] = mca->preset_live_time;
		break;
	case MXF_MCA_PRESET_REAL_TIME:
		parameter_array[1] = mca->preset_real_time;
		break;
	case MXF_MCA_PRESET_COUNT:
		parameter_array[1] = (double) mca->preset_count;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown preset type %ld for MCA '%s'",
			mca->preset_type, mca->record->name );
	}

	mx_status = mxi_handel_network_set_data_available_flags(
						handel_network, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put_array( &(handel_network->start_with_preset_nf[i]),
					MXFT_DOUBLE, 1, dimension_array,
					&parameter_array );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_handel_network_stop_run( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_network_stop_run()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_NETWORK *handel_network;
	mx_bool_type stop;
	long i;
	mx_status_type mx_status;

	mx_status = mxi_handel_network_get_pointers( mca,
					&handel_mca, &handel_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = handel_mca->detector_channel;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s'.", fname, mca->record->name));
	}

	stop = TRUE;

	mx_status = mx_put( &(handel_network->stop_nf[i]), MXFT_BOOL, &stop );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_handel_network_read_spectrum( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_network_read_spectrum()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_NETWORK *handel_network;
	long i, dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxi_handel_network_get_pointers( mca,
					&handel_mca, &handel_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = handel_mca->detector_channel;

	if ( mca->new_data_available == FALSE ) {
		/* See if the new_data_available flag is set in the server. */

		mx_status = mx_get( &(handel_network->new_data_available_nf[i]),
					MXFT_BOOL, &(mca->new_data_available) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( mca->new_data_available ) {
		if ( handel_mca->debug_flag ) {
			MX_DEBUG(-2,
			("%s: reading out %ld channels from MCA '%s'.",
			  fname, mca->current_num_channels, mca->record->name));
		}
	} else {
		if ( handel_mca->debug_flag ) {
			MX_DEBUG(-2,
			("%s: no new spectrum available for MCA '%s'.",
				fname, mca->record->name));
		}

		return MX_SUCCESSFUL_RESULT;
	}

	dimension_array[0] = mca->current_num_channels;

	mx_status = mx_get_array( &(handel_network->channel_array_nf[i]),
					MXFT_ULONG, 1, dimension_array,
					mca->channel_array );

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,(
		"%s: readout from MCA '%s' complete.  mx_status = %ld",
			fname, mca->record->name, mx_status.code));
	}

	mca->new_data_available = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_handel_network_read_statistics( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_network_read_statistics()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_NETWORK *handel_network;
	long i, dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxi_handel_network_get_pointers( mca,
					&handel_mca, &handel_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = handel_mca->detector_channel;

	if ( handel_mca->new_statistics_available == FALSE ) {

	    /* See if the new_statistics_available flag is set in the server. */

		mx_status = mx_get(
			&(handel_network->new_statistics_available_nf[i]),
			MXFT_BOOL, &(handel_mca->new_statistics_available) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( handel_mca->new_statistics_available ) {
		if ( handel_mca->debug_flag ) {
			MX_DEBUG(-2,("%s: reading new statistics for MCA '%s'.",
				fname, mca->record->name));
		}
	} else {
		if ( handel_mca->debug_flag ) {
			MX_DEBUG(-2,
			("%s: no new statistics available for MCA '%s'.",
			 	fname, mca->record->name));
		}

		return MX_SUCCESSFUL_RESULT;
	}

	dimension_array[0] = MX_HANDEL_MCA_NUM_STATISTICS;

	mx_status = mx_get_array( &(handel_network->statistics_nf[i]),
					MXFT_DOUBLE, 1, dimension_array,
					handel_mca->statistics );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->real_time = handel_mca->statistics[ MX_HANDEL_MCA_REAL_TIME ];
	mca->live_time = handel_mca->statistics[ MX_HANDEL_MCA_LIVE_TIME ];
	handel_mca->input_count_rate = 
		handel_mca->statistics[ MX_HANDEL_MCA_INPUT_COUNT_RATE ];
	handel_mca->output_count_rate =
		handel_mca->statistics[ MX_HANDEL_MCA_OUTPUT_COUNT_RATE ];
	handel_mca->num_triggers = mx_round(
		handel_mca->statistics[ MX_HANDEL_MCA_NUM_TRIGGERS ] );
	handel_mca->num_events = mx_round(
		handel_mca->statistics[ MX_HANDEL_MCA_NUM_EVENTS ] );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_handel_network_get_mx_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_network_get_mx_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_NETWORK *handel_network;
	long i, dimension_array[2];
	mx_status_type mx_status;

	mx_status = mxi_handel_network_get_pointers( mca,
					&handel_mca, &handel_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = handel_mca->detector_channel;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		mx_status = mx_get( &(handel_network->current_num_channels_nf[i]),
				MXFT_LONG, &(mca->current_num_channels) );

		if ( mca->current_num_channels > mca->maximum_num_channels ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The MCA '%s' controlled by server '%s' is reported to have %ld channels, "
"but the record '%s' is only configured to support up to %ld channels.",
				handel_mca->mca_label,
				handel_network->server_record->name,
				mca->current_num_channels,
				mca->record->name,
				mca->maximum_num_channels );
		}
		break;

	case MXLV_MCA_PRESET_TYPE:
		mx_status = mx_get( &(handel_network->preset_type_nf[i]),
					MXFT_LONG, &(mca->preset_type) );
		break;

	case MXLV_MCA_ROI_ARRAY:
		mx_status = mx_put( &(handel_network->current_num_rois_nf[i]),
					MXFT_LONG, &(mca->current_num_rois) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = mca->current_num_rois;
		dimension_array[1] = 2;

		mx_status = mx_get_array( &(handel_network->roi_array_nf[i]),
					MXFT_ULONG, 2, dimension_array,
					&(mca->roi_array) );
		break;

	case MXLV_MCA_ROI_INTEGRAL_ARRAY:
		mx_status = mx_put( &(handel_network->current_num_rois_nf[i]),
					MXFT_LONG, &(mca->current_num_rois) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = mca->current_num_rois;

		mx_status = mx_get_array(
				&(handel_network->roi_integral_array_nf[i]),
					MXFT_ULONG, 1, dimension_array,
					&(mca->roi_integral_array) );
		break;

	case MXLV_MCA_ROI:
		mx_status = mx_put( &(handel_network->roi_number_nf[i]),
					MXFT_ULONG, &(mca->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_get_array( &(handel_network->roi_nf[i]),
						MXFT_ULONG, 1, dimension_array,
						&(mca->roi) );
		break;
	
	case MXLV_MCA_ROI_INTEGRAL:
		mx_status = mx_put( &(handel_network->roi_number_nf[i]),
					MXFT_ULONG, &(mca->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get( &(handel_network->roi_integral_nf[i]),
					MXFT_ULONG, &(mca->roi_integral) );
		break;

	case MXLV_MCA_SOFT_ROI_ARRAY:
		dimension_array[0] = mca->num_soft_rois;
		dimension_array[1] = 2;

		mx_status = mx_get_array( &(handel_network->soft_roi_array_nf[i]),
					MXFT_ULONG, 2, dimension_array,
					&(mca->soft_roi_array) );
		break;

	case MXLV_MCA_SOFT_ROI_INTEGRAL_ARRAY:
		dimension_array[0] = mca->num_soft_rois;

		mx_status = mx_get_array(
				&(handel_network->soft_roi_integral_array_nf[i]),
				MXFT_ULONG, 1, dimension_array,
				&(mca->soft_roi_integral_array) );
		break;

	case MXLV_MCA_SOFT_ROI:
		mx_status = mx_put( &(handel_network->soft_roi_number_nf[i]),
					MXFT_ULONG, &(mca->soft_roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_get_array( &(handel_network->soft_roi_nf[i]),
						MXFT_ULONG, 1, dimension_array,
						&(mca->soft_roi) );
		break;

	case MXLV_MCA_SOFT_ROI_INTEGRAL:
		mx_status = mx_put( &(handel_network->soft_roi_number_nf[i]),
					MXFT_ULONG, &(mca->soft_roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get( &(handel_network->soft_roi_integral_nf[i]),
				MXFT_ULONG, &(mca->soft_roi_integral) );
		break;

	case MXLV_MCA_CHANNEL_NUMBER:
		mx_status = mx_get( &(handel_network->channel_number_nf[i]),
				MXFT_ULONG, &(mca->channel_number) );
		break;

	case MXLV_MCA_CHANNEL_VALUE:
		mx_status = mx_get( &(handel_network->channel_value_nf[i]),
				MXFT_ULONG, &(mca->channel_value) );

		break;

	case MXLV_MCA_REAL_TIME:
		mx_status = mx_get( &(handel_network->real_time_nf[i]),
				MXFT_DOUBLE, &(mca->real_time) );

		break;

	case MXLV_MCA_LIVE_TIME:
		mx_status = mx_get( &(handel_network->live_time_nf[i]),
				MXFT_DOUBLE, &(mca->live_time) );
		break;

	default:
		return mx_mca_default_get_parameter_handler( mca );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_network_set_mx_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_network_set_mx_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_NETWORK *handel_network;
	long i, dimension_array[2];
	mx_status_type mx_status;

	mx_status = mxi_handel_network_get_pointers( mca,
					&handel_mca, &handel_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = handel_mca->detector_channel;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		mx_status = mx_put( &(handel_network->current_num_channels_nf[i]),
				MXFT_LONG, &(mca->current_num_channels) );
		break;

	case MXLV_MCA_PRESET_TYPE:
		mx_status = mx_put( &(handel_network->preset_type_nf[i]),
					MXFT_LONG, &(mca->preset_type) );
		break;

	case MXLV_MCA_ROI_ARRAY:
		mx_status = mx_put( &(handel_network->current_num_rois_nf[i]),
					MXFT_LONG, &(mca->current_num_rois) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = mca->current_num_rois;
		dimension_array[1] = 2;

		mx_status = mx_put_array( &(handel_network->roi_array_nf[i]),
					MXFT_ULONG, 2, dimension_array,
					&(mca->roi_array) );
		break;

	case MXLV_MCA_ROI:
		mx_status = mx_put( &(handel_network->roi_number_nf[i]),
					MXFT_ULONG, &(mca->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_put_array( &(handel_network->roi_nf[i]),
					MXFT_ULONG, 1, dimension_array,
					&(mca->roi) );
		break;

	case MXLV_MCA_SOFT_ROI_ARRAY:
		dimension_array[0] = mca->num_soft_rois;
		dimension_array[1] = 2;

		mx_status = mx_put_array( &(handel_network->soft_roi_array_nf[i]),
					MXFT_ULONG, 2, dimension_array,
					&(mca->soft_roi_array) );
		break;

	case MXLV_MCA_SOFT_ROI:
		mx_status = mx_put( &(handel_network->soft_roi_number_nf[i]),
					MXFT_ULONG, &(mca->soft_roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_put_array( &(handel_network->soft_roi_nf[i]),
					MXFT_ULONG, 1, dimension_array,
					&(mca->soft_roi) );
		break;

	case MXLV_MCA_CHANNEL_NUMBER:
		mx_status = mx_put( &(handel_network->channel_number_nf[i]),
					MXFT_ULONG, &(mca->channel_number) );
		break;

	default:
		return mx_mca_default_set_parameter_handler( mca );
	}

	return mx_status;
}


/*
 * Name:    i_xia_network.c
 *
 * Purpose: MX network device for communicating with an XIA DXP multichannel
 *          analyzer system that is controlled by a remote MX server using
 *          the 'xia_xerxes' driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_XIA_NETWORK_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_net.h"
#include "mx_mca.h"
#include "i_xia_network.h"
#include "d_xia_dxp_mca.h"

MX_RECORD_FUNCTION_LIST mxi_xia_network_record_function_list = {
	NULL,
	mxi_xia_network_create_record_structures,
	mxi_xia_network_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_xia_network_open,
	NULL,
	NULL,
	mxi_xia_network_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxi_xia_network_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_XIA_NETWORK_STANDARD_FIELDS
};

long mxi_xia_network_num_record_fields
		= sizeof( mxi_xia_network_record_field_defaults )
			/ sizeof( mxi_xia_network_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_xia_network_rfield_def_ptr
			= &mxi_xia_network_record_field_defaults[0];

static mx_status_type mxi_xia_network_set_data_available_flags(
					MX_XIA_NETWORK *xia_network,
					int flag_value );

static mx_status_type
mxi_xia_network_get_pointers( MX_MCA *mca,
			MX_XIA_DXP_MCA **xia_dxp_mca,
			MX_XIA_NETWORK **xia_network,
			const char *calling_fname )
{
	static const char fname[] = "mxi_xia_network_get_pointers()";

	MX_RECORD *xia_dxp_record;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( xia_dxp_mca == (MX_XIA_DXP_MCA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_XIA_DXP_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( xia_network == (MX_XIA_NETWORK **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_XIA_NETWORK pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*xia_dxp_mca = (MX_XIA_DXP_MCA *) mca->record->record_type_struct;

	xia_dxp_record = (*xia_dxp_mca)->xia_dxp_record;

	if ( xia_dxp_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The xia_dxp_record pointer for MCA record '%s' is NULL.",
			mca->record->name );
	}

	*xia_network = (MX_XIA_NETWORK *) xia_dxp_record->record_type_struct;

	if ( (*xia_network) == (MX_XIA_NETWORK *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_XIA_NETWORK pointer for XIA DXP record '%s' is NULL.",
			xia_dxp_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_xia_network_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_xia_network_create_record_structures()";

	MX_XIA_NETWORK *xia_network;

	/* Allocate memory for the necessary structures. */

	xia_network = (MX_XIA_NETWORK *) malloc( sizeof(MX_XIA_NETWORK) );

	if ( xia_network == (MX_XIA_NETWORK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_XIA_NETWORK structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;;
	record->record_type_struct = xia_network;

	record->class_specific_function_list = NULL;

	xia_network->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_network_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_xia_network_finish_record_initialization()";

	MX_XIA_NETWORK *xia_network;

	xia_network = (MX_XIA_NETWORK *) record->record_type_struct;

	if ( xia_network == (MX_XIA_NETWORK *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_XIA_NETWORK pointer for record '%s' is NULL.",
			record->name );
	}

	xia_network->num_mcas = 0;

	mx_network_field_init( &(xia_network->num_mcas_nf),
		xia_network->server_record,
		"%s.num_mcas", xia_network->remote_record_name );

	mx_network_field_init( &(xia_network->resynchronize_nf),
		xia_network->server_record,
		"%s.resynchronize", xia_network->remote_record_name );

	/* We cannot allocate the following structures until
	 * mxi_xia_network_open() is invoked, so for now we
	 * initialize them to NULL pointers.
	 */

	xia_network->mca_record_array = NULL;
	xia_network->busy_nf = NULL;
	xia_network->channel_array_nf = NULL;
	xia_network->current_num_channels_nf = NULL;
	xia_network->current_num_rois_nf = NULL;
	xia_network->hardware_scas_are_enabled_nf = NULL;
	xia_network->live_time_nf = NULL;
	xia_network->new_data_available_nf = NULL;
	xia_network->new_statistics_available_nf = NULL;
	xia_network->param_value_to_all_channels_nf = NULL;
	xia_network->parameter_name_nf = NULL;
	xia_network->parameter_value_nf = NULL;
	xia_network->preset_clock_tick_nf = NULL;
	xia_network->preset_type_nf = NULL;
	xia_network->real_time_nf = NULL;
	xia_network->roi_nf = NULL;
	xia_network->roi_array_nf = NULL;
	xia_network->roi_integral_nf = NULL;
	xia_network->roi_integral_array_nf = NULL;
	xia_network->roi_number_nf = NULL;
	xia_network->runtime_clock_tick_nf = NULL;
	xia_network->soft_roi_nf = NULL;
	xia_network->soft_roi_array_nf = NULL;
	xia_network->soft_roi_integral_nf = NULL;
	xia_network->soft_roi_integral_array_nf = NULL;
	xia_network->soft_roi_number_nf = NULL;
	xia_network->start_with_preset_nf = NULL;
	xia_network->statistics_nf = NULL;
	xia_network->stop_nf = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/* mxi_xia_network_set_data_available_flags() sets mca->new_data_available
 * and xia_dxp_mca->new_statistics_available for all of the MCAs controlled
 * by this XIA remote network interface.
 */

static mx_status_type
mxi_xia_network_set_data_available_flags( MX_XIA_NETWORK *xia_network,
					int flag_value )
{
	static const char fname[] =
		"mxi_xia_network_set_data_available_flags()";

	MX_RECORD *mca_record, **mca_record_array;
	MX_MCA *mca;
	MX_XIA_DXP_MCA *xia_dxp_mca;
	unsigned long i;

	if ( xia_network == (MX_XIA_NETWORK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_XIA_NETWORK pointer passed was NULL." );
	}

	mca_record_array = xia_network->mca_record_array;

	if ( mca_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mca_record_array pointer for 'xia_network' record '%s'.",
			xia_network->record->name );
	}

	for ( i = 0; i < xia_network->num_mcas; i++ ) {

		mca_record = (xia_network->mca_record_array)[i];

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

		xia_dxp_mca = (MX_XIA_DXP_MCA *) mca_record->record_type_struct;

		if ( xia_dxp_mca == (MX_XIA_DXP_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_XIA_DXP_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		mca->new_data_available = flag_value;
		xia_dxp_mca->new_statistics_available = flag_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

#define MXI_XIA_NETWORK_NF_NULL( x, s ) \
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
mxi_xia_network_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_xia_network_open()";

	MX_XIA_NETWORK *xia_network;
	int i, num_mcas;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	xia_network = (MX_XIA_NETWORK *) (record->record_type_struct);

	if ( xia_network == (MX_XIA_NETWORK *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_XIA_NETWORK pointer for record '%s' is NULL.",
			record->name);
	}

	/* Find out how many detector channels (MCAs) there are. */

	mx_status = mx_get(&(xia_network->num_mcas_nf), MXFT_ULONG, &num_mcas );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate and initialize an array to store pointers to all of
	 * the MCA records used by this interface.
	 */

	xia_network->num_mcas = num_mcas;

	xia_network->mca_record_array = ( MX_RECORD ** ) malloc(
					num_mcas * sizeof( MX_RECORD * ) );

	if ( xia_network->mca_record_array == ( MX_RECORD ** ) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element array "
		"of MX_RECORD pointers for the mca_record_array data "
		"structure of record '%s'.", num_mcas, record->name );
	}

	for ( i = 0; i < num_mcas; i++ ) {
		xia_network->mca_record_array[i] = NULL;
	}

	MXI_XIA_NETWORK_NF_NULL( xia_network->busy_nf,
					"busy_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->channel_array_nf,
					"channel_array_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->channel_number_nf,
					"channel_number_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->channel_value_nf,
					"channel_value_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->current_num_channels_nf,
					"current_num_channels_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->current_num_rois_nf,
					"current_num_rois_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->hardware_scas_are_enabled_nf,
					"hardware_scas_are_enabled_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->live_time_nf,
					"live_time_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->new_data_available_nf,
					"new_data_available_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->new_statistics_available_nf,
					"new_statistics_available_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->param_value_to_all_channels_nf,
					"param_value_to_all_channels_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->parameter_name_nf,
					"parameter_name_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->parameter_value_nf,
					"parameter_value_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->preset_clock_tick_nf,
					"preset_clock_tick_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->preset_type_nf,
					"preset_type_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->real_time_nf,
					"real_time_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->roi_nf,
					"roi_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->roi_array_nf,
					"roi_array_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->roi_integral_nf,
					"roi_integral_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->roi_integral_array_nf,
					"roi_integral_array_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->roi_number_nf,
					"roi_number_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->soft_roi_nf,
					"soft_roi_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->soft_roi_array_nf,
					"soft_roi_array_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->soft_roi_integral_nf,
					"soft_roi_integral_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->soft_roi_integral_array_nf,
					"soft_roi_integral_array_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->soft_roi_number_nf,
					"soft_roi_number_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->runtime_clock_tick_nf,
					"runtime_clock_tick_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->start_with_preset_nf,
					"start_with_preset_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->statistics_nf,
					"statistics_nf" );
	MXI_XIA_NETWORK_NF_NULL( xia_network->stop_nf,
					"stop_nf" );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_network_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_xia_network_resynchronize()";

	MX_XIA_NETWORK *xia_network;
	int resynchronize;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	xia_network = (MX_XIA_NETWORK *) (record->record_type_struct);

	if ( xia_network == (MX_XIA_NETWORK *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_XIA_NETWORK pointer for record '%s' is NULL.",
			record->name );
	}

	resynchronize = 1;

	mx_status = mx_put( &(xia_network->resynchronize_nf),
				MXFT_INT, &resynchronize );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_xia_network_is_busy( MX_MCA *mca,
			int *busy_flag,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_network_is_busy()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_NETWORK *xia_network;
	int busy;
	long i;
	mx_status_type mx_status;

	mx_status = mxi_xia_network_get_pointers( mca,
					&xia_dxp_mca, &xia_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = xia_dxp_mca->detector_channel;

	mx_status = mx_get( &(xia_network->busy_nf[i]), MXFT_INT, &busy );

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
mxi_xia_network_read_parameter( MX_MCA *mca,
			char *parameter_name,
			uint32_t *value_ptr,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_network_read_parameter()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_NETWORK *xia_network;
	long i, dimension_array[1];
	unsigned long parameter_value;
	mx_status_type mx_status;

	mx_status = mxi_xia_network_get_pointers( mca,
					&xia_dxp_mca, &xia_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = xia_dxp_mca->detector_channel;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %d, parameter '%s'.",
			fname, mca->record->name,
			xia_dxp_mca->detector_channel, parameter_name));
	}

	dimension_array[0] = (long) strlen( parameter_name );

	mx_status = mx_put_array( &(xia_network->parameter_name_nf[i]),
					MXFT_STRING, 1, dimension_array,
					parameter_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(xia_network->parameter_value_nf[i]),
					MXFT_ULONG, &parameter_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*value_ptr = (uint32_t) parameter_value;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: value = %lu",
			fname, (unsigned long) *value_ptr));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_network_write_parameter( MX_MCA *mca,
			char *parameter_name,
			uint32_t value,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_network_write_parameter()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_NETWORK *xia_network;
	long i, dimension_array[1];
	unsigned long parameter_value;
	mx_status_type mx_status;

	mx_status = mxi_xia_network_get_pointers( mca,
					&xia_dxp_mca, &xia_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = xia_dxp_mca->detector_channel;

	parameter_value = (unsigned long) value;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %d, parameter '%s', value = %lu.",
			fname, mca->record->name, xia_dxp_mca->detector_channel,
			parameter_name, parameter_value));
	}

	dimension_array[0] = (long) strlen( parameter_name );

	mx_status = mx_put_array( &(xia_network->parameter_name_nf[i]),
					MXFT_STRING, 1, dimension_array,
					parameter_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(xia_network->parameter_value_nf[i]),
					MXFT_ULONG, &parameter_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: Write to parameter '%s' succeeded.",
			fname, parameter_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_network_write_param_to_all_channels( MX_MCA *mca,
			char *parameter_name,
			uint32_t value,
			int debug_flag )
{
	static const char fname[] =
			"mxi_xia_network_write_param_to_all_channels()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_NETWORK *xia_network;
	long i, dimension_array[1];
	unsigned long parameter_value;
	mx_status_type mx_status;

	mx_status = mxi_xia_network_get_pointers( mca,
					&xia_dxp_mca, &xia_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = xia_dxp_mca->detector_channel;

	parameter_value = (unsigned long) value;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %d, parameter '%s', value = %lu.",
			fname, mca->record->name, xia_dxp_mca->detector_channel,
			parameter_name, parameter_value));
	}

	dimension_array[0] = (long) strlen( parameter_name );

	mx_status = mx_put_array( &(xia_network->parameter_name_nf[i]),
					MXFT_STRING, 1, dimension_array,
					parameter_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(xia_network->param_value_to_all_channels_nf[i]),
					MXFT_ULONG, &parameter_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: Write to parameter '%s' succeeded.",
			fname, parameter_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_network_start_run( MX_MCA *mca,
			int clear_flag,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_network_start_run()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_NETWORK *xia_network;
	long i, dimension_array[1];
	double parameter_array[2];
	mx_status_type mx_status;

	mx_status = mxi_xia_network_get_pointers( mca,
					&xia_dxp_mca, &xia_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = xia_dxp_mca->detector_channel;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s', clear_flag = %d.",
				fname, mca->record->name, clear_flag));
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
			"Unknown preset type %d for MCA '%s'",
			mca->preset_type, mca->record->name );
	}

	mx_status = mxi_xia_network_set_data_available_flags(
						xia_network, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put_array( &(xia_network->start_with_preset_nf[i]),
					MXFT_DOUBLE, 1, dimension_array,
					&parameter_array );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_xia_network_stop_run( MX_MCA *mca,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_network_stop_run()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_NETWORK *xia_network;
	int stop;
	long i;
	mx_status_type mx_status;

	mx_status = mxi_xia_network_get_pointers( mca,
					&xia_dxp_mca, &xia_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = xia_dxp_mca->detector_channel;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s'.", fname, mca->record->name));
	}

	stop = 1;

	mx_status = mx_put( &(xia_network->stop_nf[i]), MXFT_INT, &stop );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_xia_network_read_spectrum( MX_MCA *mca,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_network_read_spectrum()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_NETWORK *xia_network;
	long i, dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxi_xia_network_get_pointers( mca,
					&xia_dxp_mca, &xia_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = xia_dxp_mca->detector_channel;

	if ( mca->new_data_available == FALSE ) {
		/* See if the new_data_available flag is set in the server. */

		mx_status = mx_get( &(xia_network->new_data_available_nf[i]),
					MXFT_INT, &(mca->new_data_available) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( mca->new_data_available ) {
		if ( debug_flag ) {
			MX_DEBUG(-2,
			("%s: reading out %ld channels from MCA '%s'.",
			  fname, mca->current_num_channels, mca->record->name));
		}
	} else {
		if ( debug_flag ) {
			MX_DEBUG(-2,
			("%s: no new spectrum available for MCA '%s'.",
				fname, mca->record->name));
		}

		return MX_SUCCESSFUL_RESULT;
	}

	dimension_array[0] = mca->current_num_channels;

	mx_status = mx_get_array( &(xia_network->channel_array_nf[i]),
					MXFT_ULONG, 1, dimension_array,
					mca->channel_array );

	if ( debug_flag ) {
		MX_DEBUG(-2,(
		"%s: readout from MCA '%s' complete.  mx_status = %ld",
			fname, mca->record->name, mx_status.code));
	}

	mca->new_data_available = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_xia_network_read_statistics( MX_MCA *mca,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_network_read_statistics()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_NETWORK *xia_network;
	long i, dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxi_xia_network_get_pointers( mca,
					&xia_dxp_mca, &xia_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = xia_dxp_mca->detector_channel;

	if ( xia_dxp_mca->new_statistics_available == FALSE ) {

	    /* See if the new_statistics_available flag is set in the server. */

		mx_status = mx_get(
			&(xia_network->new_statistics_available_nf[i]),
			MXFT_INT, &(xia_dxp_mca->new_statistics_available) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( xia_dxp_mca->new_statistics_available ) {
		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: reading new statistics for MCA '%s'.",
				fname, mca->record->name));
		}
	} else {
		if ( debug_flag ) {
			MX_DEBUG(-2,
			("%s: no new statistics available for MCA '%s'.",
			 	fname, mca->record->name));
		}

		return MX_SUCCESSFUL_RESULT;
	}

	dimension_array[0] = MX_XIA_DXP_NUM_STATISTICS;

	mx_status = mx_get_array( &(xia_network->statistics_nf[i]),
					MXFT_DOUBLE, 1, dimension_array,
					xia_dxp_mca->statistics );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->real_time = xia_dxp_mca->statistics[ MX_XIA_DXP_REAL_TIME ];
	mca->live_time = xia_dxp_mca->statistics[ MX_XIA_DXP_LIVE_TIME ];
	xia_dxp_mca->input_count_rate = 
		xia_dxp_mca->statistics[ MX_XIA_DXP_INPUT_COUNT_RATE ];
	xia_dxp_mca->output_count_rate =
		xia_dxp_mca->statistics[ MX_XIA_DXP_OUTPUT_COUNT_RATE ];
	xia_dxp_mca->num_fast_peaks = mx_round(
		xia_dxp_mca->statistics[ MX_XIA_DXP_NUM_FAST_PEAKS ] );
	xia_dxp_mca->num_events = mx_round(
		xia_dxp_mca->statistics[ MX_XIA_DXP_NUM_EVENTS ] );
	xia_dxp_mca->num_underflows = mx_round(
		xia_dxp_mca->statistics[ MX_XIA_DXP_NUM_UNDERFLOWS ] );
	xia_dxp_mca->num_overflows = mx_round(
		xia_dxp_mca->statistics[ MX_XIA_DXP_NUM_OVERFLOWS ] );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_xia_network_get_mx_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxi_xia_network_get_mx_parameter()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_NETWORK *xia_network;
	long i, dimension_array[2];
	mx_status_type mx_status;

	mx_status = mxi_xia_network_get_pointers( mca,
					&xia_dxp_mca, &xia_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = xia_dxp_mca->detector_channel;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%d).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		mx_status = mx_get( &(xia_network->current_num_channels_nf[i]),
				MXFT_LONG, &(mca->current_num_channels) );

		if ( mca->current_num_channels > mca->maximum_num_channels ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The MCA '%s' controlled by server '%s' is reported to have %ld channels, "
"but the record '%s' is only configured to support up to %ld channels.",
				xia_dxp_mca->mca_label,
				xia_network->server_record->name,
				mca->current_num_channels,
				mca->record->name,
				mca->maximum_num_channels );
		}
		break;

	case MXLV_MCA_PRESET_TYPE:
		mx_status = mx_get( &(xia_network->preset_type_nf[i]),
					MXFT_INT, &(mca->preset_type) );
		break;

	case MXLV_MCA_ROI_ARRAY:
		mx_status = mx_put( &(xia_network->current_num_rois_nf[i]),
					MXFT_LONG, &(mca->current_num_rois) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = mca->current_num_rois;
		dimension_array[1] = 2;

		mx_status = mx_get_array( &(xia_network->roi_array_nf[i]),
					MXFT_ULONG, 2, dimension_array,
					&(mca->roi_array) );
		break;

	case MXLV_MCA_ROI_INTEGRAL_ARRAY:
		mx_status = mx_put( &(xia_network->current_num_rois_nf[i]),
					MXFT_LONG, &(mca->current_num_rois) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = mca->current_num_rois;

		mx_status = mx_get_array(
				&(xia_network->roi_integral_array_nf[i]),
					MXFT_ULONG, 1, dimension_array,
					&(mca->roi_integral_array) );
		break;

	case MXLV_MCA_ROI:
		mx_status = mx_put( &(xia_network->roi_number_nf[i]),
					MXFT_ULONG, &(mca->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_get_array( &(xia_network->roi_nf[i]),
						MXFT_ULONG, 1, dimension_array,
						&(mca->roi) );
		break;
	
	case MXLV_MCA_ROI_INTEGRAL:
		mx_status = mx_put( &(xia_network->roi_number_nf[i]),
					MXFT_ULONG, &(mca->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get( &(xia_network->roi_integral_nf[i]),
					MXFT_ULONG, &(mca->roi_integral) );
		break;

	case MXLV_MCA_SOFT_ROI_ARRAY:
		dimension_array[0] = mca->num_soft_rois;
		dimension_array[1] = 2;

		mx_status = mx_get_array( &(xia_network->soft_roi_array_nf[i]),
					MXFT_ULONG, 2, dimension_array,
					&(mca->soft_roi_array) );
		break;

	case MXLV_MCA_SOFT_ROI_INTEGRAL_ARRAY:
		dimension_array[0] = mca->num_soft_rois;

		mx_status = mx_get_array(
				&(xia_network->soft_roi_integral_array_nf[i]),
				MXFT_ULONG, 1, dimension_array,
				&(mca->soft_roi_integral_array) );
		break;

	case MXLV_MCA_SOFT_ROI:
		mx_status = mx_put( &(xia_network->soft_roi_number_nf[i]),
					MXFT_ULONG, &(mca->soft_roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_get_array( &(xia_network->soft_roi_nf[i]),
						MXFT_ULONG, 1, dimension_array,
						&(mca->soft_roi) );
		break;

	case MXLV_MCA_SOFT_ROI_INTEGRAL:
		mx_status = mx_put( &(xia_network->soft_roi_number_nf[i]),
					MXFT_ULONG, &(mca->soft_roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get( &(xia_network->soft_roi_integral_nf[i]),
				MXFT_ULONG, &(mca->soft_roi_integral) );
		break;

	case MXLV_MCA_CHANNEL_NUMBER:
		mx_status = mx_get( &(xia_network->channel_number_nf[i]),
				MXFT_ULONG, &(mca->channel_number) );
		break;

	case MXLV_MCA_CHANNEL_VALUE:
		mx_status = mx_get( &(xia_network->channel_value_nf[i]),
				MXFT_ULONG, &(mca->channel_value) );

		break;

	case MXLV_MCA_REAL_TIME:
		mx_status = mx_get( &(xia_network->real_time_nf[i]),
				MXFT_DOUBLE, &(mca->real_time) );

		break;

	case MXLV_MCA_LIVE_TIME:
		mx_status = mx_get( &(xia_network->live_time_nf[i]),
				MXFT_DOUBLE, &(mca->live_time) );
		break;

	default:
		return mx_mca_default_get_parameter_handler( mca );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_network_set_mx_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxi_xia_network_set_mx_parameter()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_NETWORK *xia_network;
	long i, dimension_array[2];
	mx_status_type mx_status;

	mx_status = mxi_xia_network_get_pointers( mca,
					&xia_dxp_mca, &xia_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = xia_dxp_mca->detector_channel;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%d).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		mx_status = mx_put( &(xia_network->current_num_channels_nf[i]),
				MXFT_LONG, &(mca->current_num_channels) );
		break;

	case MXLV_MCA_PRESET_TYPE:
		mx_status = mx_put( &(xia_network->preset_type_nf[i]),
					MXFT_INT, &(mca->preset_type) );
		break;

	case MXLV_MCA_ROI_ARRAY:
		mx_status = mx_put( &(xia_network->current_num_rois_nf[i]),
					MXFT_LONG, &(mca->current_num_rois) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = mca->current_num_rois;
		dimension_array[1] = 2;

		mx_status = mx_put_array( &(xia_network->roi_array_nf[i]),
					MXFT_ULONG, 2, dimension_array,
					&(mca->roi_array) );
		break;

	case MXLV_MCA_ROI:
		mx_status = mx_put( &(xia_network->roi_number_nf[i]),
					MXFT_ULONG, &(mca->roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_put_array( &(xia_network->roi_nf[i]),
					MXFT_ULONG, 1, dimension_array,
					&(mca->roi) );
		break;

	case MXLV_MCA_SOFT_ROI_ARRAY:
		dimension_array[0] = mca->num_soft_rois;
		dimension_array[1] = 2;

		mx_status = mx_put_array( &(xia_network->soft_roi_array_nf[i]),
					MXFT_ULONG, 2, dimension_array,
					&(mca->soft_roi_array) );
		break;

	case MXLV_MCA_SOFT_ROI:
		mx_status = mx_put( &(xia_network->soft_roi_number_nf[i]),
					MXFT_ULONG, &(mca->soft_roi_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dimension_array[0] = 2;

		mx_status = mx_put_array( &(xia_network->soft_roi_nf[i]),
					MXFT_ULONG, 1, dimension_array,
					&(mca->soft_roi) );
		break;

	case MXLV_MCA_CHANNEL_NUMBER:
		mx_status = mx_put( &(xia_network->channel_number_nf[i]),
					MXFT_ULONG, &(mca->channel_number) );
		break;

	default:
		return mx_mca_default_set_parameter_handler( mca );
	}

	return mx_status;
}


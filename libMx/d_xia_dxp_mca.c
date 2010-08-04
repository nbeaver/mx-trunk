/*
 * Name:    d_xia_dxp_mca.c
 *
 * Purpose: MX multichannel analyzer driver for the X-Ray Instrumentation
 *          Associates Mesa2x Data Server.  This driver controls an individual
 *          MCA in a multi-MCA module.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2006, 2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_XIA_DXP_DEBUG		FALSE

#define MXD_XIA_DXP_DEBUG_STATISTICS	FALSE

#define MXD_XIA_DXP_DEBUG_TIMING	FALSE

#define MXD_XIA_VERIFY_PRESETS		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include "mxconfig.h"
#include "mx_osdef.h"

#if HAVE_TCPIP || HAVE_XIA_HANDEL || HAVE_XIA_XERXES

#include "mx_constants.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_hrt.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_mca.h"

#if HAVE_XIA_HANDEL

#include <xia_version.h>

#include <handel.h>
#include <handel_errors.h>
#include <handel_generic.h>

#include "i_xia_handel.h"

#endif /* HAVE_XIA_HANDEL */

#if HAVE_XIA_XERXES

#include <xerxes_errors.h>
#include <xerxes_structures.h>
#include <xerxes.h>

#include "i_xia_xerxes.h"

#endif /* HAVE_XIA_XERXES */

#include "i_xia_network.h"
#include "d_xia_dxp_mca.h"

#if MXD_XIA_DXP_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_xia_dxp_record_function_list = {
	mxd_xia_dxp_initialize_type,
	mxd_xia_dxp_create_record_structures,
	mxd_xia_dxp_finish_record_initialization,
	NULL,
	mxd_xia_dxp_print_structure,
	NULL,
	NULL,
	mxd_xia_dxp_open,
	mxd_xia_dxp_close,
	NULL,
	NULL,
	mxd_xia_dxp_special_processing_setup
};

MX_MCA_FUNCTION_LIST mxd_xia_dxp_mca_function_list = {
	mxd_xia_dxp_start,
	mxd_xia_dxp_stop,
	mxd_xia_dxp_read,
	mxd_xia_dxp_clear,
	mxd_xia_dxp_busy,
	mxd_xia_dxp_get_parameter,
	mxd_xia_dxp_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_xia_dxp_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_XIA_DXP_STANDARD_FIELDS,
#if HAVE_XIA_HANDEL
	MXD_XIA_DXP_HANDEL_STANDARD_FIELDS
#endif
};

#if MX_XIA_DXP_DEBUG_STATISTICS
#   define XIA_DEBUG_STATISTICS(x) \
	MX_DEBUG(-2,("%s: MCA '%s', new_statistics_available = %d", \
		fname, (x)->record->name, (x)->new_statistics_available));
#else
#   define XIA_DEBUG_STATISTICS(x)
#endif

long mxd_xia_dxp_num_record_fields
		= sizeof( mxd_xia_dxp_record_field_defaults )
		  / sizeof( mxd_xia_dxp_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_xia_dxp_rfield_def_ptr
			= &mxd_xia_dxp_record_field_defaults[0];

static mx_status_type mxd_xia_dxp_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/* A private function for the use of the driver. */

static mx_status_type
mxd_xia_dxp_get_pointers( MX_MCA *mca,
			MX_XIA_DXP_MCA **xia_dxp_mca,
			const char *calling_fname )
{
	static const char fname[] = "mxd_xia_dxp_get_pointers()";

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
	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*xia_dxp_mca = (MX_XIA_DXP_MCA *) mca->record->record_type_struct;

	if ( (*xia_dxp_mca)->xia_dxp_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The xia_dxp_record pointer for record '%s' is NULL.",
			mca->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_xia_dxp_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_channels_varargs_cookie;
	long maximum_num_rois_varargs_cookie;
	long num_soft_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mca_initialize_type( record_type,
				&num_record_fields,
				&record_field_defaults,
				&maximum_num_channels_varargs_cookie,
				&maximum_num_rois_varargs_cookie,
				&num_soft_rois_varargs_cookie );

	return mx_status;
}


MX_EXPORT mx_status_type
mxd_xia_dxp_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_xia_dxp_create_record_structures()";

	MX_MCA *mca;
	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	int i;


	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA structure." );
	}

	xia_dxp_mca = (MX_XIA_DXP_MCA *) malloc( sizeof(MX_XIA_DXP_MCA) );

	if ( xia_dxp_mca == (MX_XIA_DXP_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_XIA_DXP_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = xia_dxp_mca;
	record->class_specific_function_list = &mxd_xia_dxp_mca_function_list;

	mca->record = record;
	xia_dxp_mca->record = record;

	xia_dxp_mca->is_busy = NULL;
	xia_dxp_mca->read_parameter = NULL;
	xia_dxp_mca->write_parameter = NULL;
	xia_dxp_mca->write_parameter_to_all_channels = NULL;
	xia_dxp_mca->start_run = NULL;
	xia_dxp_mca->stop_run = NULL;
	xia_dxp_mca->read_spectrum = NULL;
	xia_dxp_mca->read_statistics = NULL;
	xia_dxp_mca->get_baseline_array = NULL;
	xia_dxp_mca->set_gain_change = NULL;
	xia_dxp_mca->set_gain_calibration = NULL;
	xia_dxp_mca->get_acquisition_value = NULL;
	xia_dxp_mca->set_acquisition_value = NULL;
	xia_dxp_mca->get_adc_trace_array = NULL;
	xia_dxp_mca->get_baseline_history_array = NULL;
	xia_dxp_mca->get_mx_parameter = NULL;
	xia_dxp_mca->set_mx_parameter = NULL;

#if XIA_HAVE_OLD_DXP_READOUT_DETECTOR_RUN
	xia_dxp_mca->xerxes_baseline_array = NULL;
#endif

	xia_dxp_mca->adc_trace_step_size = 5000.0;	/* in nanoseconds */
	xia_dxp_mca->adc_trace_length = 0;
	xia_dxp_mca->adc_trace_array = NULL;

	xia_dxp_mca->baseline_history_length = 0;
	xia_dxp_mca->baseline_history_array = NULL;

	xia_dxp_mca->mca_record_array_index = -1;

	xia_dxp_mca->firmware_code_variant = (unsigned long) -1;
	xia_dxp_mca->firmware_code_revision = (unsigned long) -1;
	xia_dxp_mca->hardware_scas_are_enabled = FALSE;

	for ( i = 0; i < MX_XIA_DXP_MCA_MAX_SCAS; i++ ) {
		xia_dxp_mca->sca_has_been_initialized[i] = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_xia_dxp_finish_record_initialization()";

	MX_MCA *mca;
	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) record->record_class_struct;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA pointer for record '%s' is NULL.", record->name );
	}

	if ( mca->maximum_num_rois > MX_XIA_DXP_MCA_MAX_SCAS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested maximum number of ROIs (%ld) for XIA MCA '%s' is greater "
"than the maximum allowed value of %d.",
			mca->maximum_num_rois, record->name,
			MX_XIA_DXP_MCA_MAX_SCAS );
	}

	if ( mca->maximum_num_channels > MX_XIA_DXP_MCA_MAX_BINS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested maximum number of channels (%ld) for XIA MCA '%s' is greater "
"than the maximum allowed value of %d.",
			mca->maximum_num_channels, record->name,
			MX_XIA_DXP_MCA_MAX_BINS );
	}

	mca->channel_array = ( unsigned long * )
		malloc( mca->maximum_num_channels * sizeof( unsigned long ) );

	if ( mca->channel_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an %ld channel data array.",
			mca->maximum_num_channels );
	}

	mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;

	xia_dxp_mca = (MX_XIA_DXP_MCA *) record->record_type_struct;

	xia_dxp_mca->new_statistics_available = TRUE;

	XIA_DEBUG_STATISTICS( xia_dxp_mca );

	mx_status = mx_mca_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We do our own determination of when to read new data. */

	if ( xia_dxp_mca->xia_dxp_record->mx_type == MXI_CTRL_XIA_NETWORK ) {
		mca->mca_flags |= MXF_MCA_NO_READ_OPTIMIZATION;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_xia_dxp_print_structure()";

	MX_MCA *mca;
	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MCA parameters for record '%s':\n", record->name);

	fprintf(file, "  MCA type              = XIA_DXP_MCA.\n\n");
	fprintf(file, "  XIA DXP record        = '%s'\n",
					xia_dxp_mca->xia_dxp_record->name);
	fprintf(file, "  mca label             = %s\n",
					xia_dxp_mca->mca_label);
	fprintf(file, "  maximum # of bins     = %ld\n",
					mca->maximum_num_channels);

	return MX_SUCCESSFUL_RESULT;
}

#define MXD_XIA_NETWORK_NF_INIT( i, x, s ) \
	do {								\
		mx_network_field_init( &( (x)[(i)] ),			\
			xia_network->server_record,			\
			"%s." s, xia_dxp_mca->mca_label );		\
	} while(0)

/* The following routine does XIA network record specific initialization. */

static mx_status_type
mxd_xia_dxp_network_open( MX_MCA *mca,
			MX_XIA_DXP_MCA *xia_dxp_mca,
			MX_RECORD *xia_dxp_record )
{
	static const char fname[] = "mxd_xia_dxp_network_open()";

	MX_XIA_NETWORK *xia_network;
	unsigned long i;

	xia_network = (MX_XIA_NETWORK *) xia_dxp_record->record_type_struct;

	if ( xia_network->num_mcas == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record '%s' used by MCA '%s' has not yet been configured.  "
	"You should verify that the entry for MCA '%s' is listed in the "
	"MX database file _after_ the entry for '%s'.",
			xia_dxp_record->name, mca->record->name,
			mca->record->name, xia_dxp_record->name );
	}

#if HAVE_XIA_HANDEL
	xia_dxp_mca->use_mca_channel_array = TRUE;
#endif

	/* Set up the interface function pointers. */

	xia_dxp_mca->is_busy = mxi_xia_network_is_busy;
	xia_dxp_mca->read_parameter = mxi_xia_network_read_parameter;
	xia_dxp_mca->write_parameter = mxi_xia_network_write_parameter;
	xia_dxp_mca->write_parameter_to_all_channels
				= mxi_xia_network_write_param_to_all_channels;
	xia_dxp_mca->start_run = mxi_xia_network_start_run;
	xia_dxp_mca->stop_run = mxi_xia_network_stop_run;
	xia_dxp_mca->read_spectrum = mxi_xia_network_read_spectrum;
	xia_dxp_mca->read_statistics = mxi_xia_network_read_statistics;
	xia_dxp_mca->get_mx_parameter = mxi_xia_network_get_mx_parameter;
	xia_dxp_mca->set_mx_parameter = mxi_xia_network_set_mx_parameter;

	/* It does not matter which slot in mca_record_array that this
	 * record is put into, so pick the first empty one.
	 */

	for ( i = 0; i < xia_network->num_mcas; i++ ) {

		if ( xia_network->mca_record_array[i] == NULL ) {

			xia_network->mca_record_array[i] = mca->record;

			xia_dxp_mca->detector_channel = (int) i;

			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->busy_nf, "busy" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->channel_array_nf,
				"channel_array" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->channel_number_nf,
				"channel_number" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->channel_value_nf,
				"channel_value" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->current_num_channels_nf,
				"current_num_channels" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->current_num_rois_nf,
				"current_num_rois" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->hardware_scas_are_enabled_nf,
				"hardware_scas_are_enabled" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->live_time_nf, "live_time" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->new_data_available_nf,
				"new_data_available" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->new_statistics_available_nf,
				"new_statistics_available" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->param_value_to_all_channels_nf,
				"param_value_to_all_channels" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->parameter_name_nf,
				"parameter_name" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->parameter_value_nf,
				"parameter_value" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->preset_clock_tick_nf,
				"preset_clock_tick" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->preset_type_nf,
				"preset_type" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->real_time_nf, "real_time" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->roi_nf, "roi" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->roi_array_nf, "roi_array" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->roi_integral_nf,
				"roi_integral" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->roi_integral_array_nf,
				"roi_integral_array" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->roi_number_nf,
				"roi_number" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->soft_roi_nf, "soft_roi" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->soft_roi_array_nf,
				"soft_roi_array" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->soft_roi_integral_nf,
				"soft_roi_integral" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->soft_roi_integral_array_nf,
				"soft_roi_integral_array" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->soft_roi_number_nf,
				"soft_roi_number" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->runtime_clock_tick_nf,
				"runtime_clock_tick" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->start_with_preset_nf,
				"start_with_preset" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->statistics_nf,
				"statistics" );
			MXD_XIA_NETWORK_NF_INIT( i,
				xia_network->stop_nf, "stop" );

			break;		/* Exit the for() loop. */
		}
	}

	if ( i >= xia_network->num_mcas ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Error adding MCA '%s' to the list of MCAs controlled by "
		"XIA network record '%s'.  This MCA would be record %lu "
		"of the MCA record array, but that array only has room "
		"for %lu records.", mca->record->name,
			xia_dxp_record->name, i+1, xia_network->num_mcas );
	}

	return MX_SUCCESSFUL_RESULT;
}

#if HAVE_XIA_XERXES

/* The following routine does XerXes specific initialization. */

static mx_status_type
mxd_xia_dxp_xerxes_open( MX_MCA *mca,
			MX_XIA_DXP_MCA *xia_dxp_mca,
			MX_RECORD *xia_dxp_record )
{
	static const char fname[] = "mxd_xia_dxp_xerxes_open()";

	MX_XIA_XERXES *xia_xerxes;
	int xia_status, num_items;
	unsigned int baseline_length;
	unsigned long i;

	num_items = sscanf( xia_dxp_mca->mca_label, "%d",
					&(xia_dxp_mca->detector_channel) );

	if ( num_items != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Could not get the detector channel from the 'mca_label' "
		"field '%s' for MCA '%s'.  For Xerxes, a valid 'mca_label' "
		"field should just be an integer number.",
			xia_dxp_mca->mca_label, mca->record->name );
	}

	xia_xerxes = (MX_XIA_XERXES *) xia_dxp_record->record_type_struct;

	if ( xia_xerxes->num_mcas == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record '%s' used by MCA '%s' has not yet been configured.  "
	"You should verify that the entry for MCA '%s' is listed in the "
	"MX database file _after_ the entry for '%s'.",
			xia_dxp_record->name, mca->record->name,
			mca->record->name, xia_dxp_record->name );
	}

	/* Set up the interface function pointers. */

	xia_dxp_mca->is_busy = mxi_xia_xerxes_is_busy;
	xia_dxp_mca->read_parameter = mxi_xia_xerxes_read_parameter;
	xia_dxp_mca->write_parameter = mxi_xia_xerxes_write_parameter;
	xia_dxp_mca->write_parameter_to_all_channels
			= mxi_xia_xerxes_write_parameter_to_all_channels;
	xia_dxp_mca->start_run = mxi_xia_xerxes_start_run;
	xia_dxp_mca->stop_run = mxi_xia_xerxes_stop_run;
	xia_dxp_mca->read_spectrum = mxi_xia_xerxes_read_spectrum;
	xia_dxp_mca->read_statistics = mxi_xia_xerxes_read_statistics;
	xia_dxp_mca->get_baseline_array = mxi_xia_xerxes_get_baseline_array;

	/* Search for an empty slot in the MCA array. */

	for ( i = 0; i < xia_xerxes->num_mcas; i++ ) {

		if ( xia_xerxes->mca_record_array[i] == NULL ) {

			xia_dxp_mca->mca_record_array_index = i;

			xia_xerxes->mca_record_array[i] = mca->record;

			xia_dxp_mca->dxp_module = -1;
			xia_dxp_mca->crate = -1;
			xia_dxp_mca->slot = -1;
			xia_dxp_mca->dxp_channel = -1;

			break;		/* Exit the for() loop. */
		}
	}

	if ( i >= xia_xerxes->num_mcas ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"There are too many MCA records for XerXes record '%s'.  "
		"XerXes record '%s' says that there are %ld MCAs, but "
		"MCA record '%s' would be MCA number %ld.",
			xia_xerxes->record->name,
			xia_xerxes->record->name,
			xia_xerxes->num_mcas,
			xia_dxp_mca->record->name,
			i+1 );
	}

	/* See how many bins are in the spectrum. */

	xia_status = dxp_nspec( &(xia_dxp_mca->detector_channel),
					&(xia_dxp_mca->num_spectrum_bins) );

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to get the number of spectrum bins for MCA '%s'.  "
		"Error code = %d, error status = '%s'",
			mca->record->name,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

#if 0
	/* Currently, it is an error for the number of bins in the spectrum
	 * to be different than mca->maximum_num_channels.  This policy
	 * was recommended by Jeff Terry of MR-CAT (APS), since the
	 * XIA system needs a different configuration file if there is a 
	 * different number of channels.
	 */

	if ( xia_dxp_mca->num_spectrum_bins != mca->maximum_num_channels ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The number of spectrum bins (%u) reported by the XIA MCA '%s' is different "
"from the maximum number of channels (%lu) configured for this MCA in the "
"MX database.  You must either reconfigure MX to match the number of "
"channels reported by XIA or load a XIA configuration that is configured "
"for %lu channels.", xia_dxp_mca->num_spectrum_bins,
			mca->maximum_num_channels,
			mca->maximum_num_channels );
	}
#endif

	/* Do we need to allocate memory for a spectrum array or is
	 * mca->channel_array already the right size?
	 */

	if ( ( sizeof(unsigned long) == 4 )
	  && ( mca->maximum_num_channels >= xia_dxp_mca->num_spectrum_bins ) )
	{
		/* We do not need to allocate a spectrum array since
		 * mca->channel_array is already big enough and of the
		 * right data type to hold the data.  This allows us
		 * to skip the step of copying the data from the array
		 * xia_dxp_mca->spectrum_array to mca->channel_array.
		 */

		xia_dxp_mca->use_mca_channel_array = TRUE;

		xia_dxp_mca->spectrum_array = NULL;
	} else {
		/* We _do_ need to allocate a separate spectrum array.
		 * This will happen for one of two reasons:
		 *
		 * 1.  Either the value xia_dxp_mca->num_spectrum_bins
		 *     is bigger than mca->maximum_num_channels which
		 *     means that the spectrum returned by XIA would
		 *     not fit into mca->channel_array.
		 *
		 * 2.  We are running on a computer where unsigned longs
		 *     are not 32 bits, such as on an Alpha.
		 *
		 * Either way, we have to allocate a separate array
		 * and put up with the performance hit of copying the
		 * array each time.
		 */

		xia_dxp_mca->use_mca_channel_array = FALSE;

		xia_dxp_mca->spectrum_array = (unsigned long *) malloc(
		    xia_dxp_mca->num_spectrum_bins * sizeof(unsigned long) );

		if ( xia_dxp_mca->spectrum_array == (unsigned long *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocated a DXP spectrum array of %u unsigned longs",
			xia_dxp_mca->num_spectrum_bins );
		}
	}

	/* Allocate memory for the portable baseline array. */

	xia_status = dxp_nbase( &(xia_dxp_mca->detector_channel),
					&baseline_length );

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to get the number of baseline bins for MCA '%s'.  "
		"Error code = %d, error status = '%s'",
			mca->record->name,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

	xia_dxp_mca->baseline_length = baseline_length;

	xia_dxp_mca->baseline_array = (unsigned long *)
	    malloc( xia_dxp_mca->baseline_length * sizeof(unsigned long) );

	if ( xia_dxp_mca->baseline_array == (unsigned long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocated a DXP baseline array of %u unsigned longs",
			xia_dxp_mca->baseline_length );
	}

#if XIA_HAVE_OLD_DXP_READOUT_DETECTOR_RUN

	/* Allocate memory for the Xerxes-specific baseline array. */

	xia_dxp_mca->xerxes_baseline_array = (unsigned short *)
	    malloc( xia_dxp_mca->baseline_length * sizeof(unsigned short) );

	if ( xia_dxp_mca->xerxes_baseline_array == (unsigned short *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocated a Xerxes baseline array of %u unsigned short",
			xia_dxp_mca->baseline_length );
	}

#endif
	/* Xerxes does not have detector or modules aliases. */

	xia_dxp_mca->detector_alias = NULL;
	xia_dxp_mca->module_alias = NULL;
	xia_dxp_mca->module_type = NULL;

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_XIA_XERXES */

#if HAVE_XIA_HANDEL

/* The following routine does Handel specific initialization. */

static mx_status_type
mxd_xia_dxp_handel_open( MX_MCA *mca,
			MX_XIA_DXP_MCA *xia_dxp_mca,
			MX_RECORD *xia_dxp_record )
{
	static const char fname[] = "mxd_xia_dxp_handel_open()";

	MX_XIA_HANDEL *xia_handel;
	char mca_label_format[40];
	char item_name[40];
	unsigned long ulong_value;
	int xia_status, num_items, display_config;
	unsigned long i;

#if MX_IGNORE_XIA_NULL_STRING
	char *ignore_this_pointer;
#endif

#if MXD_XIA_DXP_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

#if MXD_XIA_DXP_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	/* The following statement exist only to suppress the GCC warning
	 * "warning: `XIA_NULL_STRING' defined but not used".  You should
	 * not actually use the variable 'ignore_this_pointer' for anything.
	 */

#if MX_IGNORE_XIA_NULL_STRING
	ignore_this_pointer = XIA_NULL_STRING;
#endif

	xia_handel = (MX_XIA_HANDEL *) xia_dxp_record->record_type_struct;

	if ( xia_handel->num_mcas == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record '%s' used by MCA '%s' has not yet been configured.  "
	"You should verify that the entry for MCA '%s' is listed in the "
	"MX database file _after_ the entry for '%s'.",
			xia_dxp_record->name, mca->record->name,
			mca->record->name, xia_dxp_record->name );
	}

	display_config = xia_handel->handel_flags &
			MXF_XIA_HANDEL_DISPLAY_CONFIGURATION_AT_STARTUP;

	/* Allocate memory for the detector and module aliases. */

	xia_dxp_mca->detector_alias = (char *)
			malloc( (MAXALIAS_LEN+1) * sizeof(char) );

	if ( xia_dxp_mca->detector_alias == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a module alias string of %d chars.",
			MAXALIAS_LEN+1 );
	}

	xia_dxp_mca->module_alias = (char *)
			malloc( (MAXALIAS_LEN+1) * sizeof(char) );

	if ( xia_dxp_mca->module_alias == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a module alias string of %d chars.",
			MAXALIAS_LEN+1 );
	}

	xia_dxp_mca->module_type = (char *)
			malloc( (MAXALIAS_LEN+1) * sizeof(char) );

	if ( xia_dxp_mca->module_type == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a module type string of %d chars.",
			MAXALIAS_LEN+1 );
	}

	/* Get the detector alias, module alias and channel number. */

	sprintf( mca_label_format, "%%%d %%%d %%d",
				MAXALIAS_LEN, MAXALIAS_LEN );

	num_items = sscanf( xia_dxp_mca->mca_label, "%s %s %d",
				xia_dxp_mca->detector_alias,
				xia_dxp_mca->module_alias,
				&xia_dxp_mca->module_channel );

	if ( num_items != 3 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Could not get the module alias and detector number from "
		"the 'mca_label' field '%s' for MCA '%s'.  For Handel, a "
		"valid 'mca_label' field should look something like "
		"\"detector1 module1 0\".",
			xia_dxp_mca->mca_label, mca->record->name );
	}
			
	if ( display_config ) {
		mx_info(
"MCA '%s': detector alias = '%s', module alias = '%s', module channel = %d",
			xia_dxp_mca->record->name,
			xia_dxp_mca->detector_alias,
			xia_dxp_mca->module_alias,
			xia_dxp_mca->module_channel );
	}

	/* Get the module type. */

	xia_status = xiaGetModuleItem( xia_dxp_mca->module_alias,
					"module_type",
					xia_dxp_mca->module_type );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get the module type for module '%s', "
		"channel %d used by MCA '%s'.  Error code = %d, '%s'",
			xia_dxp_mca->module_alias,
			xia_dxp_mca->module_channel,
			mca->record->name, xia_status,
			mxi_xia_handel_strerror( xia_status ) );
	}

	if ( display_config ) {
		mx_info( "MCA '%s': module_type = '%s'",
			xia_dxp_mca->record->name,
			xia_dxp_mca->module_type );
	}

#if 0
	/* Get the detector alias from the module and channel number. */

	sprintf( item_name, "channel%d_detector", module_channel_number );

	xia_status = xiaGetModuleItem( xia_dxp_mca->module_alias,
					item_name,
					detector_alias );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get the detector alias for module '%s', "
		"channel %d used by MCA '%s'.  Error code = %d, '%s'",
			xia_dxp_mca->module_alias, module_channel_number,
			mca->record->name, xia_status,
			mxi_xia_handel_strerror( xia_status ) );
	}

	if ( strcmp( detector_alias, xia_handel->detector_alias ) != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The detector alias '%s' for MCA '%s' does not match "
		"the detector alias '%s' for XIA interface record '%s'.",
			detector_alias, xia_dxp_mca->record->name,
			xia_handel->detector_alias, xia_dxp_record->name );
	}

	xia_dxp_mca->detector_alias = xia_handel->detector_alias;
#endif

	/* Get the detector channel from the module and channel number. */

	sprintf( item_name, "channel%d_alias", xia_dxp_mca->module_channel );

	xia_status = xiaGetModuleItem( xia_dxp_mca->module_alias,
					item_name,
					&(xia_dxp_mca->detector_channel) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get the detector channel id for module '%s', "
		"channel %d used by MCA '%s'.  Error code = %d, '%s'",
			xia_dxp_mca->module_alias,
			xia_dxp_mca->module_channel,
			mca->record->name, xia_status,
			mxi_xia_handel_strerror( xia_status ) );
	}

	if ( display_config ) {
		mx_info(
		"MCA '%s': det. alias = '%s', det. channel = %d",
			xia_dxp_mca->record->name,
			xia_dxp_mca->detector_alias,
			xia_dxp_mca->detector_channel );
	}

	/* Since we do not know what state the MCA is in, we send
	 * a stop run command.
	 */

	xia_status = xiaStopRun( xia_dxp_mca->detector_channel );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot stop data acquisition for XIA DXP MCA '%s'.  "
		"Error code = %d, '%s'", xia_dxp_mca->record->name,
			xia_status, mxi_xia_handel_strerror( xia_status ) );
	}

	/* Set up the interface function pointers. */

	xia_dxp_mca->is_busy = mxi_xia_handel_is_busy;
	xia_dxp_mca->read_parameter = mxi_xia_handel_read_parameter;
	xia_dxp_mca->write_parameter = mxi_xia_handel_write_parameter;
	xia_dxp_mca->write_parameter_to_all_channels
			= mxi_xia_handel_write_parameter_to_all_channels;
	xia_dxp_mca->start_run = mxi_xia_handel_start_run;
	xia_dxp_mca->stop_run = mxi_xia_handel_stop_run;
	xia_dxp_mca->read_spectrum = mxi_xia_handel_read_spectrum;
	xia_dxp_mca->read_statistics = mxi_xia_handel_read_statistics;
	xia_dxp_mca->get_baseline_array = mxi_xia_handel_get_baseline_array;

	xia_dxp_mca->set_gain_change = mxi_xia_handel_set_gain_change;
	xia_dxp_mca->set_gain_calibration = mxi_xia_handel_set_gain_calibration;
	xia_dxp_mca->get_acquisition_value =
					mxi_xia_handel_get_acquisition_value;
	xia_dxp_mca->set_acquisition_value =
					mxi_xia_handel_set_acquisition_value;
	xia_dxp_mca->get_adc_trace_array = mxi_xia_handel_get_adc_trace_array;
	xia_dxp_mca->get_baseline_history_array =
				mxi_xia_handel_get_baseline_history_array;

	xia_dxp_mca->get_mx_parameter = mxi_xia_handel_get_mx_parameter;
	xia_dxp_mca->set_mx_parameter = mxi_xia_handel_set_mx_parameter;

	xia_dxp_mca->hardware_scas_are_enabled = TRUE;

	/* Search for an empty slot in the MCA array. */

	for ( i = 0; i < xia_handel->num_mcas; i++ ) {

		if ( xia_handel->mca_record_array[i] == NULL ) {

			xia_dxp_mca->mca_record_array_index = i;

			xia_handel->mca_record_array[i] = mca->record;

			xia_dxp_mca->dxp_module = -1;
			xia_dxp_mca->crate = -1;
			xia_dxp_mca->slot = -1;
			xia_dxp_mca->dxp_channel = -1;

			break;		/* Exit the for() loop. */
		}
	}

	if ( i >= xia_handel->num_mcas ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"There are too many MCA records for Handel record '%s'.  "
		"Handel record '%s' says that there are %ld MCAs, but "
		"MCA record '%s' would be MCA number %ld.",
			xia_handel->record->name,
			xia_handel->record->name,
			xia_handel->num_mcas,
			xia_dxp_mca->record->name,
			i+1 );
	}

	/* See how many bins are in the spectrum. */

	xia_status = xiaGetRunData( xia_dxp_mca->detector_channel,
					"mca_length", &ulong_value );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to get the number of spectrum bins for MCA '%s'.  "
		"Error code = %d, error status = '%s'",
			mca->record->name,
			xia_status, mxi_xia_handel_strerror( xia_status ) );
	}

	xia_dxp_mca->num_spectrum_bins = (unsigned int) ulong_value;

	/* Do we need to allocate memory for a spectrum array or is
	 * mca->channel_array already the right size?
	 */

	if ( ( sizeof(unsigned long) == 4 )
	  && ( mca->maximum_num_channels >= xia_dxp_mca->num_spectrum_bins ) )
	{
		/* We do not need to allocate a spectrum array since
		 * mca->channel_array is already big enough and of the
		 * right data type to hold the data.  This allows us
		 * to skip the step of copying the data from the array
		 * xia_dxp_mca->spectrum_array to mca->channel_array.
		 */

		xia_dxp_mca->use_mca_channel_array = TRUE;

		xia_dxp_mca->spectrum_array = NULL;
	} else {
		/* We _do_ need to allocate a separate spectrum array.
		 * This will happen for one of two reasons:
		 *
		 * 1.  Either the value xia_dxp_mca->num_spectrum_bins
		 *     is bigger than mca->maximum_num_channels which
		 *     means that the spectrum returned by XIA would
		 *     not fit into mca->channel_array.
		 *
		 * 2.  We are running on a computer where unsigned longs
		 *     are not 32 bits, such as on an Alpha.
		 *
		 * Either way, we have to allocate a separate array
		 * and put up with the performance hit of copying the
		 * array each time.
		 */

		xia_dxp_mca->use_mca_channel_array = FALSE;

		xia_dxp_mca->spectrum_array = (unsigned long *) malloc(
		    xia_dxp_mca->num_spectrum_bins * sizeof(unsigned long) );

		if ( xia_dxp_mca->spectrum_array == (unsigned long *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocated a DXP spectrum array of %u unsigned longs",
			xia_dxp_mca->num_spectrum_bins );
		}
	}

	/* Allocate memory for the baseline array. */

	xia_status = xiaGetRunData( xia_dxp_mca->detector_channel,
					"baseline_length", &ulong_value );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to get the number of baseline bins for MCA '%s'.  "
		"Error code = %d, error status = '%s'",
			mca->record->name,
			xia_status, mxi_xia_handel_strerror( xia_status ) );
	}

	xia_dxp_mca->baseline_length = (unsigned int) ulong_value;

	xia_dxp_mca->baseline_array = (unsigned long *)
	    malloc( xia_dxp_mca->baseline_length * sizeof(unsigned long) );

	if ( xia_dxp_mca->baseline_array == (unsigned long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a DXP baseline array of %u unsigned longs",
			xia_dxp_mca->baseline_length );
	}

#if MXD_XIA_DXP_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "for MCA '%s'", mca->record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_XIA_HANDEL */

MX_EXPORT mx_status_type
mxd_xia_dxp_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_xia_dxp_open()";

	MX_MCA *mca;
	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	MX_RECORD *xia_dxp_record;
#if HAVE_XIA_HANDEL
	MX_XIA_HANDEL *xia_handel;
#endif
	MX_XIA_NETWORK *xia_network;
	unsigned long i, mca_number;
	int display_config;
	unsigned long codevar, coderev;
	unsigned long sysmicrosec;
	mx_status_type mx_status;

#if ( HAVE_XIA_HANDEL && MXD_XIA_DXP_DEBUG_TIMING )
	MX_HRT_TIMING measurement;
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	display_config = FALSE;

	xia_dxp_record = xia_dxp_mca->xia_dxp_record;

	MX_DEBUG( 2,("%s invoked for interface '%s' type %ld",
		fname, record->name, xia_dxp_record->mx_type));

	switch( xia_dxp_record->mx_type ) {
	case MXI_CTRL_XIA_NETWORK:
		mx_status = mxd_xia_dxp_network_open( mca,
					xia_dxp_mca, xia_dxp_record );

		mca_number = xia_dxp_mca->detector_channel;
		break;

#if HAVE_XIA_HANDEL
	case MXI_CTRL_XIA_HANDEL:
		mx_status = mxd_xia_dxp_handel_open( mca,
					xia_dxp_mca, xia_dxp_record );

		xia_handel = (MX_XIA_HANDEL *)
					xia_dxp_record->record_type_struct;

		display_config = xia_handel->handel_flags &
			MXF_XIA_HANDEL_DISPLAY_CONFIGURATION_AT_STARTUP;
		break;
#else
	case MXI_CTRL_XIA_HANDEL:
		return mx_error( MXE_UNSUPPORTED, fname,
		"XIA Handel support is not compiled into this copy of MX.  "
		"You will need to recompile MX including Handel support "
		"to fix this." );

#endif /* HAVE_XIA_HANDEL */

#if HAVE_XIA_XERXES
	case MXI_CTRL_XIA_XERXES:
		mx_status = mxd_xia_dxp_xerxes_open( mca,
					xia_dxp_mca, xia_dxp_record );
		break;
#else
	case MXI_CTRL_XIA_XERXES:
		return mx_error( MXE_UNSUPPORTED, fname,
		"XIA Xerxes support is not compiled into this copy of MX.  "
		"You will need to recompile MX including Xerxes support "
		"to fix this." );

#endif /* HAVE_XIA_XERXES */

	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The XIA DXP record '%s' for MCA '%s' is not a supported "
		"record type.", xia_dxp_record->name, record->name);
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out what firmware variant and revision is used by this MCA. */

	mx_status = (xia_dxp_mca->read_parameter)( mca,
						"CODEVAR", &codevar,
						MXD_XIA_DXP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	xia_dxp_mca->firmware_code_variant = (int) codevar;

	mx_status = (xia_dxp_mca->read_parameter)( mca,
						"CODEREV", &coderev,
						MXD_XIA_DXP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	xia_dxp_mca->firmware_code_revision = (int) coderev;

	/* Verify that the firmware version in use supports hardware SCAs. */

	if ( xia_dxp_record->mx_type == MXI_CTRL_XIA_NETWORK ) {

		xia_network = ( MX_XIA_NETWORK *)
					xia_dxp_record->record_type_struct;

		mx_status = mx_get(
		    &(xia_network->hardware_scas_are_enabled_nf[mca_number]),
			MXFT_BOOL, &(xia_dxp_mca->hardware_scas_are_enabled) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		/* Otherwise, use some heuristics based on the version of
		 * the firmware being run.
		 */

		if ( xia_dxp_mca->hardware_scas_are_enabled ) {
			if ( ( codevar == 0 ) && ( coderev >= 108 ) ) {
				xia_dxp_mca->hardware_scas_are_enabled = TRUE;
			} else {
				xia_dxp_mca->hardware_scas_are_enabled = FALSE;
			}
		}
	}

	/* If the MCA is controlled via Handel and hardware SCAs are enabled,
	 * try to set the number of SCAs to 16.
	 */

#if HAVE_XIA_HANDEL

	if ( ( xia_dxp_mca->hardware_scas_are_enabled )
	  && (xia_dxp_record->mx_type == MXI_CTRL_XIA_HANDEL ) )
	{
		int xia_status;
		double num_scas;

		num_scas = 16.0;

#if MXD_XIA_DXP_DEBUG_TIMING
		MX_HRT_START( measurement );
#endif

		xia_status = xiaSetAcquisitionValues(
				xia_dxp_mca->detector_channel,
				"number_of_scas",
				(void *) &num_scas );

#if MXD_XIA_DXP_DEBUG_TIMING
		MX_HRT_END( measurement );

		MX_HRT_RESULTS( measurement, fname,
		  "xiaSetAcquisitionValues(..., \"number_of_scas\", ...)" );
#endif

		if ( xia_status != XIA_SUCCESS ) {
			(void) mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"The attempt to set the number of SCAs to 16 for MCA '%s' failed.  "
	"The use of hardware SCAs for this MCA will be disabled.  "
	"Error code = %d, '%s'", mca->record->name,
	xia_status, mxi_xia_handel_strerror( xia_status ) );

			xia_dxp_mca->hardware_scas_are_enabled = FALSE;
		}
	}
#endif /* HAVE_XIA_HANDEL */

	if ( display_config ) {
		mx_info(
	"MCA '%s': codevar = %#lx, coderev = %#lx, hardware SCAs = %d",
			xia_dxp_mca->record->name,
			xia_dxp_mca->firmware_code_variant,
			xia_dxp_mca->firmware_code_revision,
			(int) xia_dxp_mca->hardware_scas_are_enabled );
	}

	/* Initialize the range of bin numbers used by the MCA
	 * unless we are connected via a 'xia_network' record.
	 */

	if ( xia_dxp_record->mx_type != MXI_CTRL_XIA_NETWORK ) {

		if ( xia_dxp_mca->write_parameter == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				xia_dxp_mca->xia_dxp_record->name );
		}

		mx_status = (xia_dxp_mca->write_parameter)( mca,
						"MCALIMLO", 0,
						MXD_XIA_DXP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = (xia_dxp_mca->write_parameter)( mca,
						"MCALIMHI",
						mca->maximum_num_channels - 1L,
						MXD_XIA_DXP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mca->current_num_channels = mca->maximum_num_channels;
	mca->current_num_rois     = mca->maximum_num_rois;

	for ( i = 0; i < mca->maximum_num_rois; i++ ) {
		mca->roi_array[i][0] = 0;
		mca->roi_array[i][1] = 0;
	}

	/* Initialize the saved preset values to illegal values. */

	xia_dxp_mca->old_preset_type       = (unsigned long) MX_ULONG_MAX;
	xia_dxp_mca->old_preset_high_order = (unsigned long) MX_ULONG_MAX;
	xia_dxp_mca->old_preset_low_order  = (unsigned long) MX_ULONG_MAX;

	MX_DEBUG( 2,("%s: MCA '%s' detector channel = %ld",
		fname, record->name, xia_dxp_mca->detector_channel));

	/****************** DXP runtime clock tick ******************/

	/* Find out how long a DXP clock tick is.  If we are an MX network
	 * client, we just need to ask the server.  Otherwise, we have to
	 * figure this out ourselves.
	 */

	if ( xia_dxp_record->mx_type == MXI_CTRL_XIA_NETWORK ) {

		xia_network = ( MX_XIA_NETWORK *)
					xia_dxp_record->record_type_struct;

		mx_status = mx_get(
			&(xia_network->runtime_clock_tick_nf[mca_number]),
			MXFT_DOUBLE, &(xia_dxp_mca->runtime_clock_tick) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		/* MX will attempt to autoconfigure the runtime clock tick
		 * time if the value is set to zero or less in the database
		 * file.  Otherwise, the value in the database file is
		 * assumed to be correct.
		 */

		if ( xia_dxp_mca->runtime_clock_tick <= 0.0 ) {

			/* Figuring out how long a DXP clock tick is makes use
			 * of the undocumented SYSMICROSEC parameter.  Since
			 * this parameter is currently undocumented, it may
			 * not always be present.  Thus, we must be prepared
			 * to cope with the possibility that this parameter
			 * is not there.
			 */

			xia_dxp_mca->runtime_clock_tick = MX_XIA_DXP_CLOCK_TICK;

			mx_status = (xia_dxp_mca->read_parameter)( mca,
						"SYSMICROSEC",
						&sysmicrosec,
						MXD_XIA_DXP_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			MX_DEBUG( 2,("%s: MCA '%s', 'SYSMICROSEC' = %lu",
				fname, mca->record->name,
				(unsigned long) sysmicrosec));

			xia_dxp_mca->runtime_clock_tick
			    *= mx_divide_safely( 20.0, (double) sysmicrosec );

			MX_DEBUG( 2,(
			  "%s: MCA '%s', runtime_clock_tick = %g seconds",
			  fname, mca->record->name,
			  xia_dxp_mca->runtime_clock_tick ));

			if ( xia_dxp_mca->runtime_clock_tick > 1.0e-5 ) {
				mx_warning(
				  "The runtime clock tick interval "
				  "( %g seconds ) for "
				  "MCA '%s' seems to be unusually long.",
				  xia_dxp_mca->runtime_clock_tick,
				  mca->record->name );
			} else
			if ( xia_dxp_mca->runtime_clock_tick < 1.0e-10 ) {
				mx_warning(
				  "The runtime clock tick interval "
				  "( %g seconds ) for "
				  "MCA '%s' seems to be unusually short.",
				  xia_dxp_mca->runtime_clock_tick,
				  mca->record->name );
			}
		}
	}

	/****************** DXP preset clock tick ******************/

	/* The clock tick interval used for calculating presets is not
	 * necessarily the same as the clock tick used for runtime
	 * calculation.  Once again, if we are an MX network client,
	 * we can just ask the server.  Otherwise, we have to figure
	 * this out ourselves.
	 *
	 * MX will attempt to autoconfigure the clock tick time if
	 * the value is set to zero or less in the database file.
	 * Otherwise, the value in the database file is assumed
	 * to be correct.
	 */

	if ( xia_dxp_record->mx_type == MXI_CTRL_XIA_NETWORK ) {

		xia_network = ( MX_XIA_NETWORK *)
					xia_dxp_record->record_type_struct;

		mx_status = mx_get(
			&(xia_network->preset_clock_tick_nf[mca_number]),
				MXFT_DOUBLE, &(xia_dxp_mca->preset_clock_tick));

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else
	if ( xia_dxp_record->mx_type == MXI_CTRL_XIA_XERXES )
	{
		if ( xia_dxp_mca->runtime_clock_tick <= 0.0 ) {

			xia_dxp_mca->preset_clock_tick
					= xia_dxp_mca->runtime_clock_tick;
		}
	} else
	if ( xia_dxp_record->mx_type == MXI_CTRL_XIA_HANDEL ) {

		if ( xia_dxp_mca->runtime_clock_tick <= 0.0 ) {

			struct timespec start_timespec, finish_timespec;
			struct timespec timespec_difference;
			struct timespec timeout_interval_timespec;
			struct timespec current_timespec;
			struct timespec expected_timeout_timespec;
			double initial_clock_tick_estimate;
			double preset_seconds, actual_seconds, ratio;
			int compare_status;
			long integer_ratio;
			mx_bool_type busy;

			MX_DEBUG( 2,(
			  "%s: calibrating preset MCA clock ticks.", fname));

			/* Initially assume that clock ticks are 400 nsec. */

			initial_clock_tick_estimate = 400.0e-9;

			xia_dxp_mca->preset_clock_tick
				= initial_clock_tick_estimate;

			preset_seconds = 0.1;

			/* Timeout after 10 seconds. */

			timeout_interval_timespec.tv_sec = 10;
			timeout_interval_timespec.tv_nsec = 0;

			/* Command the MCA to count for 0.1 seconds and measure
			 * how long it actually takes.
			 */

			start_timespec = mx_high_resolution_time();

			expected_timeout_timespec =
			    mx_add_high_resolution_times(
				start_timespec, timeout_interval_timespec );

			mx_status = mx_mca_start_with_preset( record,
						MXF_MCA_PRESET_REAL_TIME,
						preset_seconds );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Wait until the MCA has stopped counting, but
			 * timeout if we wait longer than the interval 
			 * 'timeout_interval_timespec.tv_sec'.
			 */

			busy = TRUE;

			while ( busy ) {

				mx_status = mx_mca_is_busy( record, &busy );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				current_timespec = mx_high_resolution_time();

				compare_status =
				    mx_compare_high_resolution_times(
					current_timespec,
					expected_timeout_timespec );

				if ( compare_status >= 0 ) {
					return mx_error( MXE_TIMED_OUT, fname,
		"MCA '%s' clock calibration timed out after %lu seconds.",
			    mca->record->name,
			    (unsigned long) timeout_interval_timespec.tv_sec );
				}
			}

			finish_timespec = mx_high_resolution_time();

			timespec_difference =
			    mx_subtract_high_resolution_times(
				finish_timespec, start_timespec );

			actual_seconds =
	  mx_convert_high_resolution_time_to_seconds(timespec_difference);

			ratio = mx_divide_safely( actual_seconds,
						preset_seconds );

			integer_ratio = mx_round( ratio );

			MX_DEBUG( 2,(
			    "%s: preset_seconds = %g, actual_seconds = %g",
				fname, preset_seconds, actual_seconds));

			MX_DEBUG( 2,("%s: ratio = %g, integer_ratio = %ld",
				fname, ratio, integer_ratio));

			/* We currently only allow the values of 1 for
			 * 400e-9 sec and 2 for 800e-9 sec.
			 */

			if ( integer_ratio < 1 ) {
				integer_ratio = 1;
			} else
			if ( integer_ratio > 2 ) {
				integer_ratio = 2;
			}

			xia_dxp_mca->preset_clock_tick =
				initial_clock_tick_estimate
					* (double) integer_ratio;
		}
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"DXP MCA '%s' is an unsupported type %ld device.",
			mca->record->name, xia_dxp_record->mx_type );
	}

	if ( display_config ) {
		mx_info( "MCA '%s': runtime clock tick = %g sec",
			mca->record->name,
			xia_dxp_mca->runtime_clock_tick );

		mx_info( "MCA '%s': preset clock tick = %g sec",
			mca->record->name,
			xia_dxp_mca->preset_clock_tick );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_xia_dxp_close()";

	MX_MCA *mca;
	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	MX_RECORD *xia_dxp_record;
	MX_XIA_NETWORK *xia_network;
	MX_RECORD **mca_record_array;
	unsigned long i, num_mcas;
	mx_status_type mx_status;

#if HAVE_XIA_HANDEL
	MX_XIA_HANDEL *xia_handel;
#endif
#if HAVE_XIA_XERXES
	MX_XIA_XERXES *xia_xerxes;
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Delete our entry in mca_record_array. */

	xia_dxp_record = xia_dxp_mca->xia_dxp_record;

	switch( xia_dxp_record->mx_type ) {
	case MXI_CTRL_XIA_NETWORK:
		xia_network = (MX_XIA_NETWORK *)
					xia_dxp_record->record_type_struct;

		num_mcas = xia_network->num_mcas;
		mca_record_array = xia_network->mca_record_array;
		break;

	case MXI_CTRL_XIA_HANDEL:

#if HAVE_XIA_HANDEL
		xia_handel = (MX_XIA_HANDEL *)
					xia_dxp_record->record_type_struct;

		num_mcas = xia_handel->num_mcas;
		mca_record_array = xia_handel->mca_record_array;

		if ( xia_dxp_mca->baseline_array != NULL ) {
			mx_free( xia_dxp_mca->baseline_array );
		}
		if ( xia_dxp_mca->spectrum_array != NULL ) {
			mx_free( xia_dxp_mca->spectrum_array );
		}
		break;
#else
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"You are trying to close MCA '%s' which uses XIA interface '%s' which is "
"of type MXI_CTRL_XIA_HANDEL.  However, XIA Handel support is not compiled "
"into this copy of MX.  This should never happen and is definitely a program "
"bug that should be reported.", record->name, xia_dxp_record->name );

#endif /* HAVE_XIA_HANDEL */

	case MXI_CTRL_XIA_XERXES:

#if HAVE_XIA_XERXES
		xia_xerxes = (MX_XIA_XERXES *)
					xia_dxp_record->record_type_struct;

		num_mcas = xia_xerxes->num_mcas;
		mca_record_array = xia_xerxes->mca_record_array;

		if ( xia_dxp_mca->baseline_array != NULL ) {
			mx_free( xia_dxp_mca->baseline_array );
		}
		if ( xia_dxp_mca->spectrum_array != NULL ) {
			mx_free( xia_dxp_mca->spectrum_array );
		}
		break;
#else
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"You are trying to close MCA '%s' which uses XIA interface '%s' which is "
"of type MXI_CTRL_XIA_XERXES.  However, XIA Xerxes support is not compiled "
"into this copy of MX.  This should never happen and is definitely a program "
"bug that should be reported.", record->name, xia_dxp_record->name );

#endif /* HAVE_XIA_XERXES */

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"You are trying to close MCA '%s' which uses interface record '%s'.  "
"'%s' is not a XIA interface record.",
			mca->record->name,
			xia_dxp_record->name,
			xia_dxp_record->name );
	}

	for ( i = 0; i < num_mcas; i++ ) {

		if ( record == mca_record_array[i] ) {
			mca_record_array[i] = NULL;

			break;		/* Exit the for() loop. */
		}
	}

	/* Don't treat it as an error if we did not find an entry
	 * in mca_record_array.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_start( MX_MCA *mca )
{
	static const char fname[] = "mxd_xia_dxp_start()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	double preset_time;
	double xia_clock_ticks, xia_high_order_double, xia_low_order_double;
	unsigned long xia_preset_type;
	unsigned long xia_high_order, xia_low_order;
	mx_status_type mx_status;

	MX_RECORD *xia_dxp_record;
	MX_XIA_NETWORK *xia_network;
#if HAVE_XIA_HANDEL
	MX_XIA_HANDEL *xia_handel;
#endif
#if HAVE_XIA_XERXES
	MX_XIA_XERXES *xia_xerxes;
#endif

#if MXD_XIA_VERIFY_PRESETS
	unsigned long returned_preset_type;
	unsigned long returned_xia_high_order, returned_xia_low_order;
#endif /* MXD_XIA_VERIFY_PRESETS */

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Select the preset type. */

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_NONE:
		xia_preset_type = MXF_XIA_PRESET_NONE;
		preset_time = 0.0;
		break;

	case MXF_MCA_PRESET_LIVE_TIME:
		xia_preset_type = MXF_XIA_PRESET_LIVE_TIME;
		preset_time = mca->preset_live_time;
		break;

	case MXF_MCA_PRESET_REAL_TIME:
		xia_preset_type = MXF_XIA_PRESET_REAL_TIME;
		preset_time = mca->preset_real_time;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Preset type %ld is not supported for record '%s'",
			mca->preset_type, mca->record->name );
	}

#if MXD_XIA_DXP_DEBUG
	MX_DEBUG(-2,("%s: xia_preset_type = %lu", fname,
					(unsigned long) xia_preset_type));
#endif
	/* Save the preset value. */

	xia_dxp_record = xia_dxp_mca->xia_dxp_record;

	switch( xia_dxp_record->mx_type ) {
	case MXI_CTRL_XIA_NETWORK:
		xia_network = (MX_XIA_NETWORK *)
					xia_dxp_record->record_type_struct;

		xia_network->last_measurement_interval = preset_time;
		break;

#if HAVE_XIA_HANDEL
	case MXI_CTRL_XIA_HANDEL:
		xia_handel = (MX_XIA_HANDEL *)
					xia_dxp_record->record_type_struct;

		xia_handel->last_measurement_interval = preset_time;
		break;
#endif

#if HAVE_XIA_XERXES
	case MXI_CTRL_XIA_XERXES:
		xia_xerxes = (MX_XIA_XERXES *)
					xia_dxp_record->record_type_struct;

		xia_xerxes->last_measurement_interval = preset_time;
		break;
#endif

	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The XIA DXP record '%s' for MCA '%s' is not a supported "
		"record type.", xia_dxp_record->name, mca->record->name);
	}


	/* Compute the time preset in XIA preset clock ticks. */

	xia_clock_ticks =
		mx_divide_safely( preset_time, xia_dxp_mca->preset_clock_tick );

	/* The preset time is sent to the MCA controller as a 16-bit
	 * high order quantity plus a 16-bit low order quantity.
	 */

	/* 2^16 = 65536 */

	xia_high_order_double = xia_clock_ticks / 65536.0;

	xia_high_order = (unsigned long) xia_high_order_double;

	xia_low_order_double = xia_clock_ticks
				- 65536.0 * (double) xia_high_order;

	xia_low_order = (unsigned long) xia_low_order_double;

	if ( xia_high_order >= 65536 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested preset time of %g seconds would exceed the maximum "
	"allowed counting time for the XIA MCA of %g seconds.", preset_time,
			xia_dxp_mca->preset_clock_tick * pow( 2.0, 32.0 ) );
	}

	MX_DEBUG( 2,("%s: preset_time = %g", fname, preset_time));
	MX_DEBUG( 2,("%s: xia_high_order = %lu, xia_low_order = %lu", fname,
			(unsigned long) xia_high_order,
			(unsigned long) xia_low_order));

	/* If all the preset values for this MCA start are the same as 
	 * the values from the last MCA start, then we do not need to
	 * send any change parameter requests and can go ahead and start
	 * the MCA.
	 */

	if ( ( xia_preset_type == xia_dxp_mca->old_preset_type )
	  && ( xia_high_order  == xia_dxp_mca->old_preset_high_order )
	  && ( xia_low_order   == xia_dxp_mca->old_preset_low_order ) )
	{
		MX_DEBUG( 2,("%s: Presets will NOT be changed.", fname));

		if ( xia_dxp_mca->start_run == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				xia_dxp_mca->xia_dxp_record->name );
		}

		mx_status = (xia_dxp_mca->start_run)( mca, 1,
						MXD_XIA_DXP_DEBUG );

		/* We are done, so return to the caller. */

		return mx_status;
	}

	/* One or more of the preset parameters are different than
	 * before, so we must set them all.
	 */

	MX_DEBUG( 2,("%s: Presets will be changed.", fname));

	/******* Set the preset type. *******/

	if ( xia_dxp_mca->write_parameter_to_all_channels == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			xia_dxp_mca->xia_dxp_record->name );
	}

	mx_status = (xia_dxp_mca->write_parameter_to_all_channels)( mca,
						"PRESET", xia_preset_type,
						MXD_XIA_DXP_DEBUG );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XIA_VERIFY_PRESETS
	/* Read back the preset type to verify it was set correctly. */

	if ( xia_dxp_mca->read_parameter == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			xia_dxp_mca->xia_dxp_record->name );
	}

	mx_status = (xia_dxp_mca->read_parameter)( mca,
						"PRESET", &returned_preset_type,
						MXD_XIA_DXP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( returned_preset_type != xia_preset_type ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The attempt to set the XIA DXP preset type to %lu failed.  "
		"When the value was read back, it had been set to %lu.",
			(unsigned long) xia_preset_type,
			(unsigned long) returned_preset_type );
	}
#endif /* MXD_XIA_VERIFY_PRESETS */

	xia_dxp_mca->old_preset_type = xia_preset_type;

	/******* Set the timer presets. *******/

	mx_status = (xia_dxp_mca->write_parameter_to_all_channels)( mca,
						"PRESETLEN1", xia_low_order,
						MXD_XIA_DXP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = (xia_dxp_mca->write_parameter_to_all_channels)( mca,
						"PRESETLEN0", xia_high_order,
						MXD_XIA_DXP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XIA_VERIFY_PRESETS
	/* Read back and verify the PRESETLENx values. */

	mx_status = (xia_dxp_mca->read_parameter)( mca,
						"PRESETLEN1",
						&returned_xia_low_order,
						MXD_XIA_DXP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = (xia_dxp_mca->read_parameter)( mca,
						"PRESETLEN0",
						&returned_xia_high_order,
						MXD_XIA_DXP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s: returned_xia_low_order = %#lx, returned_xia_high_order = %#lx",
		fname, (unsigned long) returned_xia_low_order,
		(unsigned long) returned_xia_high_order));

	if ( (returned_xia_low_order != xia_low_order)
	  || (returned_xia_high_order != xia_high_order) )
	{
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"The attempt to set PRESETLEN0 and PRESETLEN1 to %#lx and %#lx failed.  "
"When the values were read back, they had been set to %#lx and %#lx.",
			(unsigned long) xia_high_order,
			(unsigned long) xia_low_order,
			(unsigned long) returned_xia_high_order,
			(unsigned long) returned_xia_low_order );
	}
#endif /* MXD_XIA_VERIFY_PRESETS */

	xia_dxp_mca->old_preset_high_order = xia_high_order;
	xia_dxp_mca->old_preset_low_order = xia_low_order;

	xia_dxp_mca->new_statistics_available = TRUE;

	XIA_DEBUG_STATISTICS( xia_dxp_mca );

	/* Start the MCA. */

	if ( xia_dxp_mca->start_run == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			xia_dxp_mca->xia_dxp_record->name );
	}

	mx_status = (xia_dxp_mca->start_run)( mca, 1, MXD_XIA_DXP_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_xia_dxp_stop()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the MCA. */

	if ( xia_dxp_mca->stop_run == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			xia_dxp_mca->xia_dxp_record->name );
	}

	mx_status = (xia_dxp_mca->stop_run)( mca, MXD_XIA_DXP_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_xia_dxp_read()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( xia_dxp_mca->read_spectrum == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			xia_dxp_mca->xia_dxp_record->name );
	}

	mx_status = (xia_dxp_mca->read_spectrum)( mca, MXD_XIA_DXP_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_xia_dxp_clear()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* There doesn't appear to be a way of doing this without
	 * starting a run.
	 */

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_xia_dxp_busy()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( xia_dxp_mca->is_busy == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			xia_dxp_mca->xia_dxp_record->name );
	}

	mx_status = (xia_dxp_mca->is_busy)( mca,
				&(mca->busy),
				MXD_XIA_DXP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->busy == FALSE ) {

		/* Send a stop run command so that the run enabled bit
		 * in the DXP is cleared.
		 */

		if ( xia_dxp_mca->stop_run == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				xia_dxp_mca->xia_dxp_record->name );
		}

		mx_status = (xia_dxp_mca->stop_run)( mca, MXD_XIA_DXP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	MX_DEBUG( 2,("%s: mca->busy = %d", fname, (int) mca->busy));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_xia_dxp_get_parameter()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	if ( xia_dxp_mca->get_mx_parameter != NULL ) {
		mx_status = (xia_dxp_mca->get_mx_parameter)( mca );
	} else {
		mx_status = mxd_xia_dxp_default_get_mx_parameter( mca );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_set_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_xia_dxp_set_parameter()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	if ( xia_dxp_mca->set_mx_parameter != NULL ) {
		mx_status = (xia_dxp_mca->set_mx_parameter)( mca );
	} else {
		mx_status = mxd_xia_dxp_default_set_mx_parameter( mca );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxd_xia_dxp_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_XIA_DXP_STATISTICS:
		case MXLV_XIA_DXP_PARAMETER_VALUE:
		case MXLV_XIA_DXP_PARAM_VALUE_TO_ALL_CHANNELS:
		case MXLV_XIA_DXP_BASELINE_ARRAY:
		case MXLV_XIA_DXP_GAIN_CHANGE:
		case MXLV_XIA_DXP_GAIN_CALIBRATION:
		case MXLV_XIA_DXP_ACQUISITION_VALUE:
		case MXLV_XIA_DXP_ADC_TRACE_ARRAY:
		case MXLV_XIA_DXP_BASELINE_HISTORY_ARRAY:
			record_field->process_function
					    = mxd_xia_dxp_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_xia_dxp_default_get_mx_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_xia_dxp_default_get_mx_parameter()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	unsigned long low_limit, high_limit;
	unsigned long i, j, roi_boundary;
	unsigned long roi[2];
	char name[20];
	mx_status_type mx_status;

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:

		if ( xia_dxp_mca->read_parameter == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				xia_dxp_mca->xia_dxp_record->name );
		}

		mx_status = (xia_dxp_mca->read_parameter)( mca,
						"MCALIMLO", &low_limit,
						MXD_XIA_DXP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = (xia_dxp_mca->read_parameter)( mca,
						"MCALIMHI", &high_limit,
						MXD_XIA_DXP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mca->current_num_channels = (long) high_limit
						- (long) low_limit + 1L;

		if ( mca->current_num_channels > mca->maximum_num_channels ) {
			mx_warning(
"MCA '%s' reports that it currently has %ld channels.  "
"The MX record is currently configured for a maximum of %ld channels, "
"so channels %ld to %ld will be discarded.  "
"This probably means that the MX database is not correctly configured.",
				mca->record->name,
				mca->current_num_channels,
				mca->maximum_num_channels,
				mca->current_num_channels,
				mca->maximum_num_channels - 1 );

			mca->current_num_channels = mca->maximum_num_channels;
		}
		break;
	case MXLV_MCA_ROI:
		i = mca->roi_number;

		sprintf( name, "SCA%luLO", (unsigned long) i );

		if ( xia_dxp_mca->read_parameter == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				xia_dxp_mca->xia_dxp_record->name );
		}

		mx_status = (xia_dxp_mca->read_parameter)( mca,
						name, &roi_boundary,
						MXD_XIA_DXP_DEBUG );
		mca->roi[0] = roi_boundary;

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		sprintf( name, "SCA%luHI", (unsigned long) i );

		mx_status = (xia_dxp_mca->read_parameter)( mca,
						name, &roi_boundary,
						MXD_XIA_DXP_DEBUG );

		mca->roi[1] = roi_boundary;

		break;
	case MXLV_MCA_ROI_INTEGRAL:
		i = mca->roi_number;

		/* Hardware SCA support is not used for this case. */

		mx_status = mx_mca_read( mca->record, NULL, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Make sure the ROI boundaries are correct. */

		mx_status = mx_mca_get_roi( mca->record, i, roi );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,
	("%s: Integrating MCA '%s' ROI %lu integral from bin %lu to bin %lu",
				fname, mca->record->name, (unsigned long) i,
				roi[0], roi[1]));

		mca->roi_integral = 0;

		for ( j = roi[0]; j <= roi[1]; j++ ) {
			mca->roi_integral += mca->channel_array[j];
		}

		MX_DEBUG( 2,("%s: ROI %lu integral = %lu",
			fname, (unsigned long) i, mca->roi_integral));

		break;
	case MXLV_MCA_LIVE_TIME:
	case MXLV_MCA_REAL_TIME:
		mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
							MXD_XIA_DXP_DEBUG );

		XIA_DEBUG_STATISTICS( xia_dxp_mca );

		MX_DEBUG( 2,
		("%s: new_statistics_available = %d, mca->busy = %d",
			fname, (int) xia_dxp_mca->new_statistics_available,
			(int) mca->busy ));

		break;
	default:
		return mx_mca_default_get_parameter_handler( mca );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_default_set_mx_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_xia_dxp_default_set_mx_parameter()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	unsigned long i, low_limit, high_limit;
	char name[20];
	mx_status_type mx_status;

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	if ( xia_dxp_mca->write_parameter == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			xia_dxp_mca->xia_dxp_record->name );
	}

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:

		if ( xia_dxp_mca->read_parameter == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				xia_dxp_mca->xia_dxp_record->name );
		}

		mx_status = (xia_dxp_mca->read_parameter)( mca,
						"MCALIMLO", &low_limit,
						MXD_XIA_DXP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		high_limit = (unsigned long)low_limit
				+ mca->current_num_channels - 1L;

		mx_status = (xia_dxp_mca->write_parameter)( mca,
						"MCALIMHI", high_limit,
						MXD_XIA_DXP_DEBUG );
	case MXLV_MCA_ROI:
		i = mca->roi_number;

		sprintf( name, "SCA%luLO", (unsigned long) i );

		mx_status = (xia_dxp_mca->write_parameter)( mca,
						name, mca->roi[0],
						MXD_XIA_DXP_DEBUG );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		sprintf( name, "SCA%luHI", (unsigned long) i );

		mx_status = (xia_dxp_mca->write_parameter)( mca,
						name, mca->roi[1],
						MXD_XIA_DXP_DEBUG );
		break;

	case MXLV_MCA_ROI_ARRAY:
		for ( i = 0; i < mca->current_num_rois; i++ ) {

			sprintf( name, "SCA%luLO", (unsigned long) i );

			mx_status = (xia_dxp_mca->write_parameter)( mca,
					name, mca->roi_array[i][0],
					MXD_XIA_DXP_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			sprintf( name, "SCA%luHI", (unsigned long) i );

			mx_status = (xia_dxp_mca->write_parameter)( mca,
					name, mca->roi_array[i][1],
					MXD_XIA_DXP_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;
	default:
		return mx_mca_default_set_parameter_handler( mca );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_get_mca_array( MX_RECORD *xia_dxp_record,
				unsigned long *num_mcas,
				MX_RECORD ***mca_record_array )
{
	static const char fname[] = "mxd_xia_dxp_get_mca_array()";

	MX_XIA_NETWORK *xia_network;

#if HAVE_XIA_HANDEL
	MX_XIA_HANDEL *xia_handel;
#endif
#if HAVE_XIA_XERXES
	MX_XIA_XERXES *xia_xerxes;
#endif

	if ( xia_dxp_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The xia_dxp_record pointer passed was NULL." );
	}
	if ( num_mcas == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_mcas pointer passed was NULL." );
	}
	if ( mca_record_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The mca_record_array pointer passed was NULL." );
	}

	switch( xia_dxp_record->mx_type ) {

	case MXI_CTRL_XIA_NETWORK:
		xia_network = (MX_XIA_NETWORK *)
					xia_dxp_record->record_type_struct;

		if ( xia_network == (MX_XIA_NETWORK *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_XIA_NETWORK pointer for record '%s' is NULL.",
				xia_dxp_record->name );
		}

		*num_mcas = xia_network->num_mcas;
		*mca_record_array = xia_network->mca_record_array;
		break;

#if HAVE_XIA_HANDEL
	case MXI_CTRL_XIA_HANDEL:
		xia_handel = (MX_XIA_HANDEL *)
					xia_dxp_record->record_type_struct;

		if ( xia_handel == (MX_XIA_HANDEL *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_XIA_HANDEL pointer for record '%s' is NULL.",
				xia_dxp_record->name );
		}

		*num_mcas = xia_handel->num_mcas;
		*mca_record_array = xia_handel->mca_record_array;
		break;
#endif

#if HAVE_XIA_XERXES
	case MXI_CTRL_XIA_XERXES:
		xia_xerxes = (MX_XIA_XERXES *)
					xia_dxp_record->record_type_struct;

		if ( xia_xerxes == (MX_XIA_XERXES *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_XIA_XERXES pointer for record '%s' is NULL.",
				xia_dxp_record->name );
		}

		*num_mcas = xia_xerxes->num_mcas;
		*mca_record_array = xia_xerxes->mca_record_array;
		break;
#endif

	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"xia_dxp_record '%s' is not a "
		"XIA MDS record or a XIA Xerxes record.",
			xia_dxp_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_get_rate_corrected_roi_integral( MX_MCA *mca,
			unsigned long roi_number,
			double *corrected_roi_value )
{
	static const char fname[] =
		"mxd_xia_dxp_get_rate_corrected_roi_integral()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	unsigned long mca_value;
	double corrected_value, multiplier;
	mx_status_type mx_status;

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( corrected_roi_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The corrected_roi_error pointer passed was NULL." );
	}

	mx_status = mx_mca_get_roi_integral( mca->record,
						roi_number, &mca_value );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	corrected_value = (double) mca_value;

	mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
						MXD_XIA_DXP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	multiplier = mx_divide_safely( xia_dxp_mca->input_count_rate,
					xia_dxp_mca->output_count_rate );

	corrected_value = mx_multiply_safely( corrected_value, multiplier );

	*corrected_roi_value = corrected_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_get_livetime_corrected_roi_integral( MX_MCA *mca,
			unsigned long roi_number,
			double *corrected_roi_value )
{
	static const char fname[] =
		"mxd_xia_dxp_get_livetime_corrected_roi_integral()";

	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	unsigned long mca_value;
	double corrected_value, multiplier;
	mx_status_type mx_status;

	mx_status = mxd_xia_dxp_get_pointers( mca, &xia_dxp_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( corrected_roi_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The corrected_roi_error pointer passed was NULL." );
	}

	mx_status = mx_mca_get_roi_integral( mca->record,
						roi_number, &mca_value );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	corrected_value = (double) mca_value;

	mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
						MXD_XIA_DXP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	multiplier = mx_divide_safely( xia_dxp_mca->input_count_rate,
					xia_dxp_mca->output_count_rate );

	corrected_value = mx_multiply_safely( corrected_value, multiplier );

	multiplier = mx_divide_safely( mca->real_time, mca->live_time );

	corrected_value = mx_multiply_safely( corrected_value, multiplier );

	*corrected_roi_value = corrected_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_read_statistics( MX_MCA *mca,
				MX_XIA_DXP_MCA *xia_dxp_mca,
				int debug_flag )
{
	static const char fname[] = "mxd_xia_dxp_read_statistics()";

	int read_statistics;
	mx_status_type mx_status;

	if ( mca->mca_flags & MXF_MCA_NO_READ_OPTIMIZATION ) {
		read_statistics = TRUE;

	} else if ( xia_dxp_mca->new_statistics_available ) {
		read_statistics = TRUE;

	} else {
		read_statistics = FALSE;
	}

	XIA_DEBUG_STATISTICS( xia_dxp_mca );

	MX_DEBUG( 2,("%s: read_statistics = %d", fname, read_statistics));

	if ( read_statistics ) {

		if ( xia_dxp_mca->read_statistics == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				xia_dxp_mca->xia_dxp_record->name );
		}

		mx_status = (xia_dxp_mca->read_statistics)( mca, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mca->busy == FALSE ) {
			xia_dxp_mca->new_statistics_available = FALSE;
		}
	}

	XIA_DEBUG_STATISTICS( xia_dxp_mca );

	return MX_SUCCESSFUL_RESULT;
}

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif


static mx_status_type
mxd_xia_dxp_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxd_xia_dxp_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MCA *mca;
	MX_XIA_DXP_MCA *xia_dxp_mca = NULL;
	unsigned long parameter_value;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	mca = (MX_MCA *) record->record_class_struct;
	xia_dxp_mca = (MX_XIA_DXP_MCA *) record->record_type_struct;

	MX_DEBUG( 2,("**** %s invoked, operation = %d, label_value = %ld ****",
		fname, operation, record_field->label_value));

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_XIA_DXP_PARAMETER_VALUE:
			if ( xia_dxp_mca->read_parameter == NULL ) {
				return mx_error( MXE_INITIALIZATION_ERROR,
					fname,
		"XIA interface record '%s' has not been initialized properly.",
					xia_dxp_mca->xia_dxp_record->name );
			}

			mx_status = (xia_dxp_mca->read_parameter)( mca,
						xia_dxp_mca->parameter_name,
						&parameter_value,
						MXD_XIA_DXP_DEBUG );

			xia_dxp_mca->parameter_value = parameter_value;

			MX_DEBUG( 2,
		("%s: Read mca '%s' parameter name = '%s', value = %lu",
				fname, mca->record->name,
				xia_dxp_mca->parameter_name,
				xia_dxp_mca->parameter_value));

			break;
		case MXLV_XIA_DXP_STATISTICS:

			mx_status = mxd_xia_dxp_read_statistics(
							mca, xia_dxp_mca,
							MXD_XIA_DXP_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			xia_dxp_mca->statistics[ MX_XIA_DXP_REAL_TIME ]
						= mca->real_time;

			xia_dxp_mca->statistics[ MX_XIA_DXP_LIVE_TIME ]
						= mca->live_time;

			xia_dxp_mca->statistics[ MX_XIA_DXP_INPUT_COUNT_RATE ]
						= xia_dxp_mca->input_count_rate;

			xia_dxp_mca->statistics[ MX_XIA_DXP_OUTPUT_COUNT_RATE ]
					= xia_dxp_mca->output_count_rate;

			xia_dxp_mca->statistics[ MX_XIA_DXP_NUM_FAST_PEAKS ]
					= (double) xia_dxp_mca->num_fast_peaks;

			xia_dxp_mca->statistics[ MX_XIA_DXP_NUM_EVENTS ]
					= (double) xia_dxp_mca->num_events;

			xia_dxp_mca->statistics[ MX_XIA_DXP_NUM_UNDERFLOWS ]
					= (double) xia_dxp_mca->num_underflows;

			xia_dxp_mca->statistics[ MX_XIA_DXP_NUM_OVERFLOWS ]
					= (double) xia_dxp_mca->num_overflows;
			break;
		case MXLV_XIA_DXP_BASELINE_ARRAY:
			MX_DEBUG(-2,
			("%s: getting the adc trace array for mca '%s'",
				fname, mca->record->name ));

			if ( xia_dxp_mca->get_baseline_array == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the baseline array is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (xia_dxp_mca->get_baseline_array)(
					mca, MXD_XIA_DXP_DEBUG );
			break;
		case MXLV_XIA_DXP_ACQUISITION_VALUE:
			MX_DEBUG(-2,
			("%s: get acquisition value '%s' for mca '%s'",
				fname, xia_dxp_mca->acquisition_value_name,
				mca->record->name ));

			if ( xia_dxp_mca->get_acquisition_value == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Getting acquisition values is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (xia_dxp_mca->get_acquisition_value)( mca,
					MXD_XIA_DXP_DEBUG );

			MX_DEBUG(-2,
			("%s: acquisition value '%s' for mca '%s' = %g",
				fname, xia_dxp_mca->acquisition_value_name,
				mca->record->name,
				xia_dxp_mca->acquisition_value));
			break;
		case MXLV_XIA_DXP_ADC_TRACE_ARRAY:
			MX_DEBUG(-2,
			("%s: getting the adc trace array for mca '%s'",
				fname, mca->record->name ));

			if ( xia_dxp_mca->get_adc_trace_array == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the adc trace array is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (xia_dxp_mca->get_adc_trace_array)( mca,
					MXD_XIA_DXP_DEBUG );
			break;
		case MXLV_XIA_DXP_BASELINE_HISTORY_ARRAY:
			MX_DEBUG(-2,
			("%s: getting the adc trace array for mca '%s'",
				fname, mca->record->name ));

			if ( xia_dxp_mca->get_baseline_history_array == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
	"Getting the baseline history array is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (xia_dxp_mca->get_baseline_history_array)(
					mca, MXD_XIA_DXP_DEBUG );
			break;
		default:
			MX_DEBUG(-1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_XIA_DXP_PARAMETER_VALUE:

			MX_DEBUG( 2,
		("%s: Write mca '%s' parameter name = '%s', value = %lu",
				fname, mca->record->name,
				xia_dxp_mca->parameter_name,
				xia_dxp_mca->parameter_value));

			if ( xia_dxp_mca->write_parameter == NULL ) {
				return mx_error( MXE_INITIALIZATION_ERROR,
					fname,
		"XIA interface record '%s' has not been initialized properly.",
					xia_dxp_mca->xia_dxp_record->name );
			}

			mx_status = (xia_dxp_mca->write_parameter)( mca,
				xia_dxp_mca->parameter_name,
				xia_dxp_mca->parameter_value,
				MXD_XIA_DXP_DEBUG );

			break;
		case MXLV_XIA_DXP_PARAM_VALUE_TO_ALL_CHANNELS:
			MX_DEBUG( 2,
("%s: Write to all channels for mca '%s' parameter name = '%s', value = %lu",
				fname, mca->record->name,
				xia_dxp_mca->parameter_name,
				xia_dxp_mca->param_value_to_all_channels));

			if ( xia_dxp_mca->write_parameter_to_all_channels
				== NULL )
			{
				return mx_error( MXE_INITIALIZATION_ERROR,
					fname,
		"XIA interface record '%s' has not been initialized properly.",
					xia_dxp_mca->xia_dxp_record->name );
			}

			mx_status =
			    (xia_dxp_mca->write_parameter_to_all_channels)(
				mca,
				xia_dxp_mca->parameter_name,
				xia_dxp_mca->param_value_to_all_channels,
				MXD_XIA_DXP_DEBUG );

			break;
		case MXLV_XIA_DXP_GAIN_CHANGE:
			MX_DEBUG(-2,
			("%s: gain change for mca '%s' to %g",
				fname, mca->record->name,
				xia_dxp_mca->gain_change ));

			if ( xia_dxp_mca->set_gain_change == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the gain is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (xia_dxp_mca->set_gain_change)( mca,
					MXD_XIA_DXP_DEBUG );
			break;
		case MXLV_XIA_DXP_GAIN_CALIBRATION:
			MX_DEBUG(-2,
			("%s: set gain calibration for mca '%s' to %g",
				fname, mca->record->name,
				xia_dxp_mca->gain_calibration ));

			if ( xia_dxp_mca->set_gain_calibration == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Setting the gain calibration is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (xia_dxp_mca->set_gain_calibration)( mca,
					MXD_XIA_DXP_DEBUG );
			break;
		case MXLV_XIA_DXP_ACQUISITION_VALUE:
			MX_DEBUG(-2,
			("%s: set acquisition value '%s' for mca '%s' to %g",
				fname, xia_dxp_mca->acquisition_value_name,
				mca->record->name,
				xia_dxp_mca->acquisition_value ));

			if ( xia_dxp_mca->set_acquisition_value == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Setting acquisition values is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (xia_dxp_mca->set_acquisition_value)( mca,
					MXD_XIA_DXP_DEBUG );
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

#endif /* HAVE_TCPIP || HAVE_XIA_HANDEL */


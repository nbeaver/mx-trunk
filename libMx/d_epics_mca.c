/*
 * Name:    d_epics_mca.c
 *
 * Purpose: MX multichannel analyzer driver for EPICS multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_mca.h"
#include "d_epics_mca.h"

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_epics_mca_record_function_list = {
	mxd_epics_mca_initialize_type,
	mxd_epics_mca_create_record_structures,
	mxd_epics_mca_finish_record_initialization,
	mxd_epics_mca_delete_record,
	mxd_epics_mca_print_structure,
	NULL,
	NULL,
	mxd_epics_mca_open,
	NULL
};

MX_MCA_FUNCTION_LIST mxd_epics_mca_mca_function_list = {
	mxd_epics_mca_start,
	mxd_epics_mca_stop,
	mxd_epics_mca_read,
	mxd_epics_mca_clear,
	mxd_epics_mca_busy,
	mxd_epics_mca_get_parameter,
	mxd_epics_mca_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_epics_mca_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_EPICS_MCA_STANDARD_FIELDS
};

mx_length_type mxd_epics_mca_num_record_fields
		= sizeof( mxd_epics_mca_record_field_defaults )
		  / sizeof( mxd_epics_mca_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_mca_rfield_def_ptr
			= &mxd_epics_mca_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_epics_mca_get_pointers( MX_MCA *mca,
			MX_EPICS_MCA **epics_mca,
			const char *calling_fname )
{
	const char fname[] = "mxd_epics_mca_get_pointers()";

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

	if ( mca->record->mx_type != MXT_MCA_EPICS ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The mca '%s' passed by '%s' is not an EPICS MCA.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			mca->record->name, calling_fname,
			mca->record->mx_superclass,
			mca->record->mx_class,
			mca->record->mx_type );
	}

	if ( epics_mca != (MX_EPICS_MCA **) NULL ) {

		*epics_mca = (MX_EPICS_MCA *)
				(mca->record->record_type_struct);

		if ( *epics_mca == (MX_EPICS_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_EPICS_MCA pointer for mca record '%s' passed by '%s' is NULL",
				mca->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_epics_mca_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	mx_length_type num_record_fields;
	mx_length_type maximum_num_channels_varargs_cookie;
	mx_length_type maximum_num_rois_varargs_cookie;
	mx_length_type num_soft_rois_varargs_cookie;
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
mxd_epics_mca_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_epics_mca_create_record_structures()";

	MX_MCA *mca;
	MX_EPICS_MCA *epics_mca;

	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA structure." );
	}

	epics_mca = (MX_EPICS_MCA *) malloc( sizeof(MX_EPICS_MCA) );

	if ( epics_mca == (MX_EPICS_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = epics_mca;
	record->class_specific_function_list
		= &mxd_epics_mca_mca_function_list;

	mca->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mca_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_epics_mca_finish_record_initialization()";

	MX_MCA *mca;
	mx_status_type mx_status;

	mca = (MX_MCA *) record->record_class_struct;

	mca->channel_array = (uint32_t *)
		malloc( mca->maximum_num_channels * sizeof(uint32_t) );

	if ( mca->channel_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an %lu channel data array.",
			(unsigned long) mca->maximum_num_channels );
	}

	mx_status = mx_mca_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_delete_record( MX_RECORD *record )
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
mxd_epics_mca_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_epics_mca_print_structure()";

	MX_MCA *mca;
	MX_EPICS_MCA *epics_mca;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mca = (MX_MCA *) (record->record_class_struct);

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCA pointer for record '%s' is NULL.", record->name);
	}

	epics_mca = (MX_EPICS_MCA *) (record->record_type_struct);

	if ( epics_mca == (MX_EPICS_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_EPICS_MCA pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "MCA parameters for record '%s':\n", record->name);

	fprintf(file, "  MCA type              = EPICS_MCA.\n\n");
	fprintf(file, "  EPICS record name     = %s\n",
					epics_mca->epics_record_name);
	fprintf(file, "  maximum # of channels = %lu\n",
				(unsigned long) mca->maximum_num_channels);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mca_open( MX_RECORD *record )
{
	const char fname[] = "mxd_epics_mca_open()";

	MX_MCA *mca;
	MX_EPICS_MCA *epics_mca;
	unsigned long i;
mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_epics_mca_get_pointers(mca, &epics_mca, fname);

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

	mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;
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
	mca->preset_count = 0;

	/* Initialize MX EPICS data structures for all of the process
	 * variables we might use.
	 *
	 * Please note that the following initialization steps do not
	 * invoke Channel Access and cause _no_ network I/O.  All they
	 * do is create the string representation of the process variable
	 * names for later use.
	 */

	mx_epics_pvname_init( &(epics_mca->acqg_pv),
				"%s.ACQG", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->act_pv),
				"%s.ACT", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->eltm_pv),
				"%s.ELTM", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->eras_pv),
				"%s.ERAS", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->ertm_pv),
				"%s.ERTM", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->nuse_pv),
				"%s.NUSE", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->pct_pv),
				"%s.PCT", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->pcth_pv),
				"%s.PCTH", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->pctl_pv),
				"%s.PCTL", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->pltm_pv),
				"%s.PLTM", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->proc_pv),
				"%s.PROC", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->prtm_pv),
				"%s.PRTM", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->stop_pv),
				"%s.STOP", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->strt_pv),
				"%s.STRT", epics_mca->epics_record_name );
	mx_epics_pvname_init( &(epics_mca->val_pv),
				"%s.VAL", epics_mca->epics_record_name );

	epics_mca->num_roi_pvs = mca->maximum_num_rois;

	epics_mca->roi_low_pv_array = (MX_EPICS_PV *)
		malloc( epics_mca->num_roi_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mca->roi_low_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elements for roi_low_pv_array "
	"used by MCA record '%s'.", epics_mca->num_roi_pvs, record->name );
	}

	epics_mca->roi_high_pv_array = (MX_EPICS_PV *)
		malloc( epics_mca->num_roi_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mca->roi_high_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elements for roi_high_pv_array "
	"used by MCA record '%s'.", epics_mca->num_roi_pvs, record->name );
	}

	epics_mca->roi_integral_pv_array = (MX_EPICS_PV *)
		malloc( epics_mca->num_roi_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mca->roi_integral_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elements for roi_integral_pv_array "
	"used by MCA record '%s'.", epics_mca->num_roi_pvs, record->name );
	}

	epics_mca->roi_background_pv_array = (MX_EPICS_PV *)
		malloc( epics_mca->num_roi_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mca->roi_background_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elements for roi_background_pv_array "
	"used by MCA record '%s'.", epics_mca->num_roi_pvs, record->name );
	}

	for ( i = 0; i < epics_mca->num_roi_pvs; i++ ) {
		mx_epics_pvname_init( &(epics_mca->roi_low_pv_array[i]),
			"%s.R%ldLO", epics_mca->epics_record_name, i );

		mx_epics_pvname_init( &(epics_mca->roi_high_pv_array[i]),
			"%s.R%ldHI", epics_mca->epics_record_name, i );

		mx_epics_pvname_init( &(epics_mca->roi_integral_pv_array[i]),
			"%s.R%ld", epics_mca->epics_record_name, i );

		mx_epics_pvname_init( &(epics_mca->roi_background_pv_array[i]),
			"%s.R%ldBG", epics_mca->epics_record_name, i );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mca_start( MX_MCA *mca )
{
	const char fname[] = "mxd_epics_mca_start()";

	MX_EPICS_MCA *epics_mca;
	MX_EPICS_GROUP epics_group;
	long start, preset_counts;
	double preset_real_time, preset_live_time;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the preset interval appropriately depending on the
	 * preset type.
	 */

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_NONE:
		preset_live_time = 0.0;
		preset_real_time = 0.0;
		preset_counts = 0;
		break;
	case MXF_MCA_PRESET_LIVE_TIME:
		preset_live_time = mca->preset_live_time;
		preset_real_time = 0.0;
		preset_counts = 0;
		break;
	case MXF_MCA_PRESET_REAL_TIME:
		preset_live_time = 0.0;
		preset_real_time = mca->preset_real_time;
		preset_counts = 0;
		break;
	case MXF_MCA_PRESET_COUNT:
		preset_live_time = 0.0;
		preset_real_time = 0.0;
		preset_counts = 0;
		break;
		
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported preset type %d for MCA '%s'.",
			mca->preset_type, mca->record->name );
		break;
	}

	/* Save a group of EPICS commands to be started all at once. */

	mx_status = mx_epics_start_group( &epics_group );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Do the actual preset changes. */

	mx_status = mx_group_caput( &epics_group, &(epics_mca->pltm_pv),
				MX_CA_DOUBLE, 1, &preset_live_time );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_epics_delete_group( &epics_group );

		return mx_status;
	}

	mx_status = mx_group_caput( &epics_group, &(epics_mca->prtm_pv),
				MX_CA_DOUBLE, 1, &preset_real_time );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_epics_delete_group( &epics_group );

		return mx_status;
	}

	mx_status = mx_group_caput( &epics_group, &(epics_mca->pct_pv),
				MX_CA_LONG, 1, &preset_counts );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_epics_delete_group( &epics_group );

		return mx_status;
	}

	/* Tell the counting to start. */

	start = 1;

	mx_status = mx_group_caput( &epics_group, &(epics_mca->strt_pv),
				MX_CA_LONG, 1, &start );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_epics_delete_group( &epics_group );

		return mx_status;
	}

	/* Send all of the commands in the group. */

	mx_status = mx_epics_end_group( &epics_group );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_stop( MX_MCA *mca )
{
	const char fname[] = "mxd_epics_mca_stop()";

	MX_EPICS_MCA *epics_mca;
	long stop;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop = 1;

	mx_status = mx_caput( &(epics_mca->stop_pv), MX_CA_LONG, 1, &stop );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_read( MX_MCA *mca )
{
	const char fname[] = "mxd_epics_mca_read()";

	MX_EPICS_MCA *epics_mca;
	mx_length_type num_channels;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mca_get_num_channels( mca->record, &num_channels );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_mca->val_pv),
			MX_CA_LONG, (int) num_channels, mca->channel_array );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_clear( MX_MCA *mca )
{
	const char fname[] = "mxd_epics_mca_clear()";

	MX_EPICS_MCA *epics_mca;
	long erase;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	erase = 1;

	mx_status = mx_caput( &(epics_mca->eras_pv), MX_CA_LONG, 1, &erase );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_busy( MX_MCA *mca )
{
	const char fname[] = "mxd_epics_mca_busy()";

	MX_EPICS_MCA *epics_mca;
	long busy, proc;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The value of the ACQG only reflects the status of the record
	 * the last time that EPICS processed it.  In order to get a
	 * current status, we must force the EPICS record to process
	 * ourselves.
	 */

	proc = 1;

	mx_status = mx_caput( &(epics_mca->proc_pv), MX_CA_LONG, 1, &proc );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now get the updated acquisition status. */

	mx_status = mx_caget( &(epics_mca->acqg_pv), MX_CA_LONG, 1, &busy );

	if ( busy ) {
		mca->busy = TRUE;
	} else {
		mca->busy = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_get_parameter( MX_MCA *mca )
{
	const char fname[] = "mxd_epics_mca_get_parameter()";

	MX_EPICS_MCA *epics_mca;
	MX_EPICS_GROUP epics_group;
	double preset_live_time, preset_real_time;
	unsigned long preset_counts;
	long i;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->parameter_type == MXLV_MCA_CURRENT_NUM_CHANNELS ) {

		mx_status = mx_caget( &(epics_mca->nuse_pv),
				MX_CA_LONG, 1, &(mca->current_num_channels) );

		if ( mca->current_num_channels > mca->maximum_num_channels ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The EPICS MCA '%s' is reported to have %lu channels, "
			"but the record '%s' is only configured to support "
			"up to %lu channels.",
				epics_mca->epics_record_name,
				(unsigned long) mca->current_num_channels,
				mca->record->name,
				(unsigned long) mca->maximum_num_channels );
		}
	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_TYPE ) {

		/* Read all of the EPICS variables that affect 
		 * the preset type.
		 */

		mx_status = mx_epics_start_group( &epics_group );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		mx_status = mx_group_caget( &epics_group, &(epics_mca->pltm_pv),
					MX_CA_DOUBLE, 1, &preset_live_time );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		mx_status = mx_group_caget( &epics_group, &(epics_mca->prtm_pv),
					MX_CA_DOUBLE, 1, &preset_real_time );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		mx_status = mx_group_caget( &epics_group, &(epics_mca->pct_pv),
					MX_CA_LONG, 1, &preset_counts );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		mx_status = mx_epics_end_group( &epics_group );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Figure out which type of preset this is. */

		if ( preset_real_time != 0.0 ) {
			mca->preset_type = MXF_MCA_PRESET_REAL_TIME;
		} else
		if ( preset_live_time != 0.0 ) {
			mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;
		} else
		if ( preset_counts != 0 ) {
			mca->preset_type = MXF_MCA_PRESET_COUNT;
		} else {
			mca->preset_type = MXF_MCA_PRESET_NONE;
		}

	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_COUNT_REGION ) {

		mx_status = mx_caget( &(epics_mca->pctl_pv),
			MX_CA_LONG, 1, &(mca->preset_count_region[0]) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_mca->pcth_pv),
			MX_CA_LONG, 1, &(mca->preset_count_region[1]) );

	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_ARRAY ) {

		mx_status = mx_epics_start_group( &epics_group );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		for ( i = 0; i < mca->current_num_rois; i++ ) {

			mx_status = mx_group_caget( &epics_group,
				&(epics_mca->roi_low_pv_array[i]),
				MX_CA_LONG, 1, &(mca->roi_array[i][0]) );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_epics_delete_group( &epics_group );

				return mx_status;
			}

			mx_status = mx_group_caget( &epics_group,
				&(epics_mca->roi_high_pv_array[i]),
				MX_CA_LONG, 1, &(mca->roi_array[i][1]) );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_epics_delete_group( &epics_group );

				return mx_status;
			}
		}

		mx_status = mx_epics_end_group( &epics_group );

	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_INTEGRAL_ARRAY ) {

		mx_status = mx_epics_start_group( &epics_group );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		for ( i = 0; i < mca->current_num_rois; i++ ) {

			mx_status = mx_group_caget( &epics_group,
				&(epics_mca->roi_integral_pv_array[i]),
				MX_CA_LONG, 1, &(mca->roi_integral_array[i]) );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_epics_delete_group( &epics_group );

				return mx_status;
			}
		}

		mx_status = mx_epics_end_group( &epics_group );


	} else
	if ( mca->parameter_type == MXLV_MCA_ROI ) {

		mx_status = mx_epics_start_group( &epics_group );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_group_caget( &epics_group,
			&(epics_mca->roi_low_pv_array[ mca->roi_number ]),
				MX_CA_LONG, 1, &(mca->roi[0]) );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		mx_status = mx_group_caget( &epics_group,
			&(epics_mca->roi_high_pv_array[ mca->roi_number ]),
				MX_CA_LONG, 1, &(mca->roi[1]) );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		mx_status = mx_epics_end_group( &epics_group );

	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_INTEGRAL ) {

		mx_status = mx_caget( 
			&(epics_mca->roi_integral_pv_array[ mca->roi_number ]),
				MX_CA_LONG, 1, &(mca->roi_integral) );

	} else
	if ( mca->parameter_type == MXLV_MCA_REAL_TIME ) {

		mx_status = mx_caget( &(epics_mca->ertm_pv),
					MX_CA_DOUBLE, 1, &(mca->real_time) );

	} else
	if ( mca->parameter_type == MXLV_MCA_LIVE_TIME ) {

		mx_status = mx_caget( &(epics_mca->eltm_pv),
					MX_CA_DOUBLE, 1, &(mca->live_time) );

	} else
	if ( mca->parameter_type == MXLV_MCA_COUNTS ) {

		mx_status = mx_caget( &(epics_mca->act_pv),
					MX_CA_DOUBLE, 1, &(mca->counts) );

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			mca->parameter_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mca_set_parameter( MX_MCA *mca )
{
	const char fname[] = "mxd_epics_mca_set_parameter()";

	MX_EPICS_MCA *epics_mca;
	MX_EPICS_GROUP epics_group;
	long i;
	short no_background;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->parameter_type == MXLV_MCA_CURRENT_NUM_CHANNELS ) {

		mx_status = mx_caput( &(epics_mca->nuse_pv),
				MX_CA_LONG, 1, &(mca->current_num_channels) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mca->current_num_channels > mca->maximum_num_channels ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The EPICS MCA '%s' is reported to have %lu channels, "
			"but the record '%s' is only configured to support "
			"up to %lu channels.",
				epics_mca->epics_record_name,
				(unsigned long) mca->current_num_channels,
				mca->record->name,
				(unsigned long) mca->maximum_num_channels );
		}

	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_COUNT_REGION ) {

		mx_status = mx_caput( &(epics_mca->pctl_pv),
			MX_CA_LONG, 1, &(mca->preset_count_region[0]) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caput( &(epics_mca->pcth_pv),
			MX_CA_LONG, 1, &(mca->preset_count_region[1]) );

	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_ARRAY ) {

		mx_status = mx_epics_start_group( &epics_group );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		for ( i = 0; i < mca->current_num_rois; i++ ) {

			/* Set the region boundaries for region i. */

			mx_status = mx_group_caput( &epics_group,
				&(epics_mca->roi_low_pv_array[i]),
					MX_CA_LONG, 1, &(mca->roi_array[i][0]));

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_epics_delete_group( &epics_group );

				return mx_status;
			}

			mx_status = mx_group_caput( &epics_group,
				&(epics_mca->roi_high_pv_array[i]),
					MX_CA_LONG, 1, &(mca->roi_array[i][1]));

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_epics_delete_group( &epics_group );

				return mx_status;
			}

			/* At present, we are not using the background
			 * calculation.
			 */

			no_background = -1;

			mx_status = mx_group_caput( &epics_group,
				&(epics_mca->roi_background_pv_array[i]),
					MX_CA_SHORT, 1, &no_background );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_epics_delete_group( &epics_group );

				return mx_status;
			}
		}

		mx_status = mx_epics_end_group( &epics_group );

	} else
	if ( mca->parameter_type == MXLV_MCA_ROI ) {

		mx_status = mx_epics_start_group( &epics_group );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the region boundaries. */

		mx_status = mx_group_caput( &epics_group,
			&(epics_mca->roi_low_pv_array[ mca->roi_number ]),
				MX_CA_LONG, 1, &(mca->roi[0]) );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		mx_status = mx_group_caput( &epics_group,
			&(epics_mca->roi_high_pv_array[ mca->roi_number ]),
				MX_CA_LONG, 1, &(mca->roi[1]) );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		/* At present, we are not using the background calculation. */

		no_background = -1;

		mx_status = mx_group_caput( &epics_group,
		    &(epics_mca->roi_background_pv_array[ mca->roi_number ]),
				MX_CA_SHORT, 1, &no_background );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		mx_status = mx_epics_end_group( &epics_group );

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			mca->parameter_type );
	}

	return mx_status;
}

#endif /* HAVE_EPICS */

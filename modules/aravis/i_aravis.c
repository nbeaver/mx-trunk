/*
 * Name:    i_aravis.c
 *
 * Purpose: MX driver for the Aravis camera interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_ARAVIS_DEBUG_OPEN	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_aravis.h"

MX_RECORD_FUNCTION_LIST mxi_aravis_record_function_list = {
	NULL,
	mxi_aravis_create_record_structures,
	mxi_aravis_finish_record_initialization,
	NULL,
	NULL,
	mxi_aravis_open,
	mxi_aravis_close
};

MX_RECORD_FIELD_DEFAULTS mxi_aravis_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_ARAVIS_STANDARD_FIELDS
};

long mxi_aravis_num_record_fields
		= sizeof( mxi_aravis_record_field_defaults )
			/ sizeof( mxi_aravis_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_aravis_rfield_def_ptr
			= &mxi_aravis_record_field_defaults[0];

static mx_status_type
mxi_aravis_get_pointers( MX_RECORD *record,
				MX_ARAVIS **aravis,
				const char *calling_fname )
{
	static const char fname[] = "mxi_aravis_get_pointers()";

	MX_ARAVIS *aravis_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	aravis_ptr = (MX_ARAVIS *) record->record_type_struct;

	if ( aravis_ptr == (MX_ARAVIS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ARAVIS pointer for record '%s' is NULL.",
			record->name );
	}

	if ( aravis != (MX_ARAVIS **) NULL ) {
		*aravis = aravis_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_aravis_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_aravis_create_record_structures()";

	MX_ARAVIS *aravis;

	/* Allocate memory for the necessary structures. */

	aravis = (MX_ARAVIS *) malloc( sizeof(MX_ARAVIS) );

	if ( aravis == (MX_ARAVIS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_ARAVIS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = aravis;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	aravis->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_aravis_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_aravis_finish_record_initialization()";

	MX_ARAVIS *aravis;
	mx_status_type mx_status;

	mx_status = mxi_aravis_get_pointers( record, &aravis, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_aravis_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_aravis_open()";

	MX_ARAVIS *aravis;
	long i;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxi_aravis_get_pointers( record, &aravis, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_ARAVIS_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name));
#endif
	flags = aravis->aravis_flags;

	/* Initialize Glib here.  Not needed for recent versions of Glib. */

	g_type_init();

	/* How many devices can Aravis see? */

	arv_update_device_list();

	aravis->num_devices = arv_get_n_devices();

	if ( flags & MXF_ARAVIS_SHOW_CAMERA_LIST ) {
		mx_info( "Found %lu Aravis devices.", aravis->num_devices );

		for ( i = 0; i < aravis->num_devices; i++ ) {
			mx_info( "  Device '%s'", arv_get_device_id(i) );
		}
	}

#if MXI_ARAVIS_DEBUG_OPEN
	MX_DEBUG(-2,("%s complete for '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_aravis_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_aravis_close()";

	MX_ARAVIS *aravis;
	mx_status_type mx_status;

	mx_status = mxi_aravis_get_pointers( record, &aravis, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_ARAVIS_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

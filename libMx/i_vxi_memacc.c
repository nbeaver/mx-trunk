/*
 * Name:    i_vxi_memacc.c
 *
 * Purpose: MX driver for NI-VISA VXI/VME access vis the MEMACC resource.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_NI_VISA

#include <stdlib.h>

#include "visa.h"	/* Header file provided by the NI-VISA package. */

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_vme.h"
#include "i_vxi_memacc.h"

MX_RECORD_FUNCTION_LIST mxi_vxi_memacc_record_function_list = {
	mxi_vxi_memacc_initialize_type,
	mxi_vxi_memacc_create_record_structures,
	mxi_vxi_memacc_finish_record_initialization,
	mxi_vxi_memacc_delete_record,
	mxi_vxi_memacc_print_structure,
	mxi_vxi_memacc_read_parms_from_hardware,
	mxi_vxi_memacc_write_parms_to_hardware,
	mxi_vxi_memacc_open,
	mxi_vxi_memacc_close,
	NULL,
	mxi_vxi_memacc_resynchronize
};

MX_VME_FUNCTION_LIST mxi_vxi_memacc_vme_function_list = {
	mxi_vxi_memacc_input,
	mxi_vxi_memacc_output,
	mxi_vxi_memacc_multi_input,
	mxi_vxi_memacc_multi_output,
	mxi_vxi_memacc_get_parameter,
	mxi_vxi_memacc_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxi_vxi_memacc_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VME_STANDARD_FIELDS
};

long mxi_vxi_memacc_num_record_fields
		= sizeof( mxi_vxi_memacc_record_field_defaults )
			/ sizeof( mxi_vxi_memacc_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxi_vxi_memacc_rfield_def_ptr
		= &mxi_vxi_memacc_record_field_defaults[0];

/*--*/

#define MX_VXI_MEMACC_DEBUG	FALSE

/*--*/

#if MX_VXI_MEMACC_DEBUG

/* MX_VXI_MEMACC_DEBUG_WARNING_SUPPRESS is used to suppress uninitialized
 * variable warnings from GCC.
 */

#  define MX_VXI_MEMACC_DEBUG_WARNING_SUPPRESS \
	do { \
		uint8_ptr = NULL; uint16_ptr = NULL; uint32_ptr = NULL; \
	} while(0)

#  define MX_VXI_MEMACC_DEBUG_PRINT \
	do { \
		unsigned long value = 0; \
                                     \
		switch( vme->data_size ) { \
		case MXF_VME_D8:  value = (unsigned long) *uint8_ptr;  break; \
		case MXF_VME_D16: value = (unsigned long) *uint16_ptr; break; \
		case MXF_VME_D32: value = (unsigned long) *uint32_ptr; break; \
		} \
		MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx", \
			fname, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value )); \
	} while(0)

#  define MX_VXI_MEMACC_MULTI_DEBUG_PRINT \
	do { \
		unsigned long value = 0; \
                                     \
		switch( vme->data_size ) { \
		case MXF_VME_D8:  value = (unsigned long) *uint8_ptr;  break; \
		case MXF_VME_D16: value = (unsigned long) *uint16_ptr; break; \
		case MXF_VME_D32: value = (unsigned long) *uint32_ptr; break; \
		} \
	MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx (%lu values)", \
			fname, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value, \
			vme->num_values )); \
	} while(0)
#else

#  define MX_VXI_MEMACC_DEBUG_WARNING_SUPPRESS

#  define MX_VXI_MEMACC_DEBUG_PRINT

#  define MX_VXI_MEMACC_MULTI_DEBUG_PRINT

#endif

/*==========================*/

static mx_status_type
mxi_vxi_memacc_get_pointers( MX_VME *vme,
				MX_VXI_MEMACC **vxi_memacc,
				MX_VXI_MEMACC_CRATE **crate,
				const char calling_fname[] )
{
	const char fname[] = "mxi_vxi_memacc_get_pointers()";

	MX_VXI_MEMACC *vxi_memacc_ptr;

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( vme->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	vxi_memacc_ptr = (MX_VXI_MEMACC *)
					vme->record->record_type_struct;

	if ( vxi_memacc_ptr == (MX_VXI_MEMACC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VXI_MEMACC pointer for record '%s' is NULL.",
				vme->record->name );
	}

	if ( vxi_memacc != (MX_VXI_MEMACC **) NULL ) {
		*vxi_memacc = vxi_memacc_ptr;
	}

	if ( crate != (MX_VXI_MEMACC_CRATE **) NULL ) {
		if ( vxi_memacc_ptr->num_crates == 0 ) {
			return mx_error( MXE_NOT_FOUND, fname,
	"The NI-VISA library did not find any VME crates when it started up." );
		}
		if ( vme->crate >= vxi_memacc_ptr->num_crates ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal VME crate number %lu.  Allowed range = (0 - %lu).",
			vme->crate, vxi_memacc_ptr->num_crates - 1 );
		}

		*crate = &( vxi_memacc_ptr->crate_array[ vme->crate ] );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_vxi_memacc_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_vxi_memacc_create_record_structures()";

	MX_VME *vme;
	MX_VXI_MEMACC *vxi_memacc;

	/* Allocate memory for the necessary structures. */

	vme = (MX_VME *) malloc( sizeof(MX_VME) );

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VME structure." );
	}

	vxi_memacc = (MX_VXI_MEMACC *)
				malloc( sizeof(MX_VXI_MEMACC) );

	if ( vxi_memacc == (MX_VXI_MEMACC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VXI_MEMACC structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = vme;
	record->record_type_struct = vxi_memacc;
	record->class_specific_function_list
			= &mxi_vxi_memacc_vme_function_list;

	vme->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] =
			"mxi_vxi_memacc_finish_record_initialization()";

	MX_VME *vme;
	MX_VXI_MEMACC *vxi_memacc;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_vxi_memacc_get_pointers( vme, &vxi_memacc,
							NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vxi_memacc->num_crates = 0;
	vxi_memacc->crate_array = NULL;

	mx_status = mx_vme_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_delete_record( MX_RECORD *record )
{
        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }
	if ( record->record_type_struct != NULL ) {
		free( record->record_type_struct );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxi_vxi_memacc_print_structure()";

	MX_VME *vme;
	MX_VXI_MEMACC *vxi_memacc;
	MX_VXI_MEMACC_CRATE *crate;
	unsigned long i, num_crates;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_vxi_memacc_get_pointers( vme, &vxi_memacc,
							NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf( file, "VME parameters for interface '%s'\n\n", record->name );

	num_crates = vxi_memacc->num_crates;

	fprintf( file, "  num_crates  = %lu\n", num_crates );

	if ( num_crates > 0 ) {
		fprintf( file, "  crate_array = ( " );

		for ( i = 0; i < num_crates; i++ ) {

			if ( i > 0 ) {
				fprintf( file, " ," );
			}

			crate = &(vxi_memacc->crate_array[i]);
		
			fprintf( file, "%s", crate->description );
		}

		fprintf( file, " )" );
	}

	fprintf( file, "\n\n" );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_open( MX_RECORD *record )
{
	const char fname[] = "mxi_vxi_memacc_open()";

	MX_VME *vme;
	MX_VXI_MEMACC *vxi_memacc;
	MX_VXI_MEMACC_CRATE *crate;
	ViStatus visa_status;
	ViFindList find_list;
	ViUInt32 i, num_matches;
	ViChar instr_desc[300];
	ViChar visa_error_message[MXU_NI_VISA_ERROR_MESSAGE_LENGTH + 1];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_vxi_memacc_get_pointers( vme, &vxi_memacc,
							NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/***********************************************************
	 * Initialize the system by asking for a connection to the *
	 * NI-VISA resource manager.                               *
	 ***********************************************************/

	visa_status = viOpenDefaultRM( &(vxi_memacc->resource_manager) );

	MX_DEBUG( 2,("%s: viOpenDefaultRM() status = %ld",
		fname, (long) visa_status ));

	if ( visa_status < VI_SUCCESS ) {
		viStatusDesc( vxi_memacc->resource_manager,
				visa_status, visa_error_message );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Unable to connect to the NI-VISA resource manager.  "
			"Status code = %#lX.  Reason = '%s'",
			(unsigned long) visa_status,
			visa_error_message );
	}

	/*************************************************
	 * Find the first MEMACC resource known to VISA. *
	 *************************************************/

	visa_status = viFindRsrc( vxi_memacc->resource_manager,
				"?*VXI[0-9]*::?*MEMACC",
				&find_list, &num_matches, instr_desc );

	if ( visa_status < VI_SUCCESS ) {
		viStatusDesc( vxi_memacc->resource_manager,
				visa_status, visa_error_message );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Unable to get a list of resources for record '%s'.  "
			"Status code = %#lX.  Reason = '%s'",
			record->name, (unsigned long) visa_status,
			visa_error_message );
	}

	MX_DEBUG( 2,("%s: viFindRsrc() num_matches = %lu",
			fname, (unsigned long) num_matches));

	if ( num_matches <= 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"NI-VISA does not have any MEMACC resources defined." );
	}

	MX_DEBUG( 2,("%s: resource 0 = '%s'", fname, instr_desc));

	/*****************************************************
	 * Allocate memory for an array of crate structures. *
	 *****************************************************/

	vxi_memacc->crate_array = (MX_VXI_MEMACC_CRATE *)
		malloc((size_t) num_matches * sizeof(MX_VXI_MEMACC_CRATE));

	if ( vxi_memacc->crate_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating a %lu element array of "
		"MX_VXI_MEMACC_CRATE structures for record '%s'.",
			(unsigned long) num_matches,
			record->name );
	}

	strlcpy( vxi_memacc->crate_array[0].description,
			instr_desc, VI_FIND_BUFLEN + 1 );

	vxi_memacc->num_crates = (unsigned long) num_matches;

	/************************************************
	 * Loop through the remaining MEMACC resources. *
	 ************************************************/

	for ( i = 1; i < num_matches; i++ ) {
		visa_status = viFindNext( find_list, instr_desc );

		if ( visa_status < VI_SUCCESS ) {
			viStatusDesc( vxi_memacc->resource_manager,
				visa_status, visa_error_message );

			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Unable to get resource %lu for record '%s'.  "
			"Status code = %#lX.  Reason = '%s'",
				(unsigned long) i, record->name,
				(unsigned long) visa_status,
				visa_error_message );
		}
		MX_DEBUG( 2,("%s: resource %lu = '%s'",
				fname, (unsigned long) i, instr_desc));

		strlcpy( vxi_memacc->crate_array[i].description,
			instr_desc, VI_FIND_BUFLEN + 1 );
	}

	/******************************************
	 * Connect to all of the MEMACC resource. *
	 ******************************************/

	for ( i = 0; i < num_matches; i++ ) {

		crate = &(vxi_memacc->crate_array[i]);

		visa_status = viOpen( vxi_memacc->resource_manager,
				crate->description,
				VI_NULL, VI_NULL,
				&(crate->session) );

		MX_DEBUG( 2,
		("%s: viOpen() %lu status = %ld",
			fname, (unsigned long) i, (long) visa_status));

		if ( visa_status < VI_SUCCESS ) {
			viStatusDesc( crate->session,
				visa_status, visa_error_message );

			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to connect to the '%s' resource for record '%s'.  "
		"Status code = %#lX.  Reason = '%s'",
				crate->description,
				record->name,
				(unsigned long) visa_status,
				visa_error_message );
		}

		crate->old_read_address_increment = -1;
		crate->old_write_address_increment = -1;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_close( MX_RECORD *record )
{
	const char fname[] = "mxi_vxi_memacc_close()";

	MX_VME *vme;
	MX_VXI_MEMACC *vxi_memacc;
	ViStatus visa_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_vxi_memacc_get_pointers( vme, &vxi_memacc,
							NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Shut down the connection to the resource manager.
	 *
	 * According to the documentation for viOpenDefaultRM(),
	 * closing down the Resource Manager session will also
	 * automatically close down any find lists and device
	 * sessions that were created using this resource manager.
	 */

	visa_status = viClose( vxi_memacc->resource_manager );

	MX_DEBUG( 2, ("%s: viClose() status = %ld", fname, (long) visa_status));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_resynchronize( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mxi_vxi_memacc_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_vxi_memacc_open( record );

	return mx_status;
}

/*===============*/

MX_EXPORT mx_status_type
mxi_vxi_memacc_input( MX_VME *vme )
{
	const char fname[] = "mxi_vxi_memacc_input()";

	MX_VXI_MEMACC *vxi_memacc;
	MX_VXI_MEMACC_CRATE *crate;
	ViUInt8 *uint8_ptr;
	ViUInt16 *uint16_ptr;
	ViUInt32 *uint32_ptr;
	ViUInt16 address_space;
	ViStatus visa_status;
	ViChar visa_error_message[MXU_NI_VISA_ERROR_MESSAGE_LENGTH + 1];
	mx_status_type mx_status;

	MX_VXI_MEMACC_DEBUG_WARNING_SUPPRESS;

	mx_status = mxi_vxi_memacc_get_pointers( vme, &vxi_memacc,
							&crate, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		address_space = VI_A16_SPACE;
		break;
	case MXF_VME_A24:
		address_space = VI_A24_SPACE;
		break;
	case MXF_VME_A32:
		address_space = VI_A32_SPACE;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_ptr = (ViUInt8 *) vme->data_pointer;

		visa_status = viIn8( crate->session, address_space,
					vme->address, uint8_ptr );
		break;
	case MXF_VME_D16:
		uint16_ptr = (ViUInt16 *) vme->data_pointer;

		visa_status = viIn16( crate->session, address_space,
					vme->address, uint16_ptr );
		break;
	case MXF_VME_D32:
		uint32_ptr = (ViUInt32 *) vme->data_pointer;

		visa_status = viIn32( crate->session, address_space,
					vme->address, uint32_ptr );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	MX_VXI_MEMACC_DEBUG_PRINT;

	if ( visa_status < VI_SUCCESS ) {
		viStatusDesc( vxi_memacc->resource_manager,
				visa_status, visa_error_message );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"Unable to read a value from record '%s', crate %lu, address %#lx.  "
	"Status code = %#lX.  Reason = '%s'",
			vme->record->name, vme->crate, vme->address,
			(unsigned long) visa_status,
			visa_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_output( MX_VME *vme )
{
	const char fname[] = "mxi_vxi_memacc_output()";

	MX_VXI_MEMACC *vxi_memacc;
	MX_VXI_MEMACC_CRATE *crate;
	ViUInt8 *uint8_ptr;
	ViUInt16 *uint16_ptr;
	ViUInt32 *uint32_ptr;
	ViUInt16 address_space;
	ViStatus visa_status;
	ViChar visa_error_message[MXU_NI_VISA_ERROR_MESSAGE_LENGTH + 1];
	mx_status_type mx_status;

	MX_VXI_MEMACC_DEBUG_WARNING_SUPPRESS;

	mx_status = mxi_vxi_memacc_get_pointers( vme, &vxi_memacc,
							&crate, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		address_space = VI_A16_SPACE;
		break;
	case MXF_VME_A24:
		address_space = VI_A24_SPACE;
		break;
	case MXF_VME_A32:
		address_space = VI_A32_SPACE;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_ptr = (ViUInt8 *) vme->data_pointer;

		visa_status = viOut8( crate->session, address_space,
					vme->address, *uint8_ptr );
		break;
	case MXF_VME_D16:
		uint16_ptr = (ViUInt16 *) vme->data_pointer;

		visa_status = viOut16( crate->session, address_space,
					vme->address, *uint16_ptr );
		break;
	case MXF_VME_D32:
		uint32_ptr = (ViUInt32 *) vme->data_pointer;

		visa_status = viOut32( crate->session, address_space,
					vme->address, *uint32_ptr );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	MX_VXI_MEMACC_DEBUG_PRINT;

	if ( visa_status < VI_SUCCESS ) {
		viStatusDesc( vxi_memacc->resource_manager,
				visa_status, visa_error_message );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"Unable to write a value to record '%s', crate %lu, address %#lx.  "
	"Status code = %#lX.  Reason = '%s'",
			vme->record->name, vme->crate, vme->address,
			(unsigned long) visa_status,
			visa_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_multi_input( MX_VME *vme )
{
	const char fname[] = "mxi_vxi_memacc_multi_input()";

	MX_VXI_MEMACC *vxi_memacc;
	MX_VXI_MEMACC_CRATE *crate;
	ViUInt8 *uint8_ptr;
	ViUInt16 *uint16_ptr;
	ViUInt32 *uint32_ptr;
	ViUInt16 address_space;
	ViStatus visa_status;
	ViChar visa_error_message[MXU_NI_VISA_ERROR_MESSAGE_LENGTH + 1];
	mx_status_type mx_status;

	MX_VXI_MEMACC_DEBUG_WARNING_SUPPRESS;

	mx_status = mxi_vxi_memacc_get_pointers( vme, &vxi_memacc,
							&crate, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if (vme->read_address_increment != crate->old_read_address_increment)
	{
		vme->parameter_type = MXLV_VME_READ_ADDRESS_INCREMENT;

		mx_status = mxi_vxi_memacc_set_parameter( vme );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		address_space = VI_A16_SPACE;
		break;
	case MXF_VME_A24:
		address_space = VI_A24_SPACE;
		break;
	case MXF_VME_A32:
		address_space = VI_A32_SPACE;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_ptr = (ViUInt8 *) vme->data_pointer;

		visa_status = viMoveIn8( crate->session, address_space,
					vme->address, vme->num_values,
					uint8_ptr );
		break;
	case MXF_VME_D16:
		uint16_ptr = (ViUInt16 *) vme->data_pointer;

		visa_status = viMoveIn16( crate->session, address_space,
					vme->address, vme->num_values,
					uint16_ptr );
		break;
	case MXF_VME_D32:
		uint32_ptr = (ViUInt32 *) vme->data_pointer;

		visa_status = viMoveIn32( crate->session, address_space,
					vme->address, vme->num_values,
					uint32_ptr );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	MX_VXI_MEMACC_MULTI_DEBUG_PRINT;

	if ( visa_status < VI_SUCCESS ) {
		viStatusDesc( vxi_memacc->resource_manager,
				visa_status, visa_error_message );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"Unable to read %lu values from record '%s', crate %lu, address %#lx.  "
	"Status code = %#lX.  Reason = '%s'",
			vme->num_values,
			vme->record->name, vme->crate, vme->address,
			(unsigned long) visa_status,
			visa_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_multi_output( MX_VME *vme )
{
	const char fname[] = "mxi_vxi_memacc_multi_output()";

	MX_VXI_MEMACC *vxi_memacc;
	MX_VXI_MEMACC_CRATE *crate;
	ViUInt8 *uint8_ptr;
	ViUInt16 *uint16_ptr;
	ViUInt32 *uint32_ptr;
	ViUInt16 address_space;
	ViStatus visa_status;
	ViChar visa_error_message[MXU_NI_VISA_ERROR_MESSAGE_LENGTH + 1];
	mx_status_type mx_status;

	MX_VXI_MEMACC_DEBUG_WARNING_SUPPRESS;

	mx_status = mxi_vxi_memacc_get_pointers( vme, &vxi_memacc,
							&crate, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if (vme->write_address_increment != crate->old_write_address_increment)
	{
		vme->parameter_type = MXLV_VME_WRITE_ADDRESS_INCREMENT;

		mx_status = mxi_vxi_memacc_set_parameter( vme );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		address_space = VI_A16_SPACE;
		break;
	case MXF_VME_A24:
		address_space = VI_A24_SPACE;
		break;
	case MXF_VME_A32:
		address_space = VI_A32_SPACE;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_ptr = (ViUInt8 *) vme->data_pointer;

		visa_status = viMoveOut8( crate->session, address_space,
					vme->address, vme->num_values,
					uint8_ptr );
		break;
	case MXF_VME_D16:
		uint16_ptr = (ViUInt16 *) vme->data_pointer;

		visa_status = viMoveOut16( crate->session, address_space,
					vme->address, vme->num_values,
					uint16_ptr );
		break;
	case MXF_VME_D32:
		uint32_ptr = (ViUInt32 *) vme->data_pointer;

		visa_status = viMoveOut32( crate->session, address_space,
					vme->address, vme->num_values,
					uint32_ptr );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	MX_VXI_MEMACC_MULTI_DEBUG_PRINT;

	if ( visa_status < VI_SUCCESS ) {
		viStatusDesc( vxi_memacc->resource_manager,
				visa_status, visa_error_message );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"Unable to write %lu values to record '%s', crate %lu, address %#lx.  "
	"Status code = %#lX.  Reason = '%s'",
			vme->num_values,
			vme->record->name, vme->crate, vme->address,
			(unsigned long) visa_status,
			visa_error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_get_parameter( MX_VME *vme )
{
	const char fname[] = "mxi_vxi_memacc_get_parameter()";

	MX_VXI_MEMACC *vxi_memacc;
	MX_VXI_MEMACC_CRATE *crate;
	void *data_ptr;
	ViInt32 int32_value;
	ViStatus visa_status;
	ViChar visa_error_message[MXU_NI_VISA_ERROR_MESSAGE_LENGTH + 1];
	mx_status_type mx_status;

	mx_status = mxi_vxi_memacc_get_pointers( vme, &vxi_memacc,
							&crate, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for VME interface '%s' for parameter type '%s' (%d).",
		fname, vme->record->name,
		mx_get_field_label_string( vme->record, vme->parameter_type ),
		vme->parameter_type));

	switch( vme->parameter_type ) {
	case MXLV_VME_READ_ADDRESS_INCREMENT:
		data_ptr = (void *) &int32_value;

		visa_status = viGetAttribute( crate->session,
						VI_ATTR_SRC_INCREMENT,
						data_ptr );

		if ( visa_status < VI_SUCCESS ) {
			viStatusDesc( crate->session,
				visa_status, visa_error_message );

			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to read attribute VI_ATTR_SRC_INCREMENT "
		"for record '%s', crate %lu, address %#lx.  "
		"Status code = %#lX.  Reason = '%s'",
			vme->record->name, vme->crate, vme->address,
			(unsigned long) visa_status,
			visa_error_message );
		}

		vme->read_address_increment = (unsigned long) int32_value;

		crate->old_read_address_increment =
				vme->read_address_increment;
		break;

	case MXLV_VME_WRITE_ADDRESS_INCREMENT:
		data_ptr = (void *) &int32_value;

		visa_status = viGetAttribute( crate->session,
						VI_ATTR_DEST_INCREMENT,
						data_ptr );

		if ( visa_status < VI_SUCCESS ) {
			viStatusDesc( crate->session,
				visa_status, visa_error_message );

			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to read attribute VI_ATTR_DEST_INCREMENT "
		"for record '%s', crate %lu, address %#lx.  "
		"Status code = %#lX.  Reason = '%s'",
			vme->record->name, vme->crate, vme->address,
			(unsigned long) visa_status,
			visa_error_message );
		}

		vme->write_address_increment = (unsigned long) int32_value;

		crate->old_write_address_increment =
				vme->write_address_increment;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%d) is not supported by the MX driver "
		"for VME interface '%s'.",
			mx_get_field_label_string(
				vme->record, vme->parameter_type ),
			vme->parameter_type, vme->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxi_memacc_set_parameter( MX_VME *vme )
{
	const char fname[] = "mxi_vxi_memacc_set_parameter()";

	MX_VXI_MEMACC *vxi_memacc;
	MX_VXI_MEMACC_CRATE *crate;
	ViInt32 int32_value;
	ViStatus visa_status;
	ViChar visa_error_message[MXU_NI_VISA_ERROR_MESSAGE_LENGTH + 1];
	mx_status_type mx_status;

	mx_status = mxi_vxi_memacc_get_pointers( vme, &vxi_memacc,
							&crate, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for VME interface '%s' for parameter type '%s' (%d).",
		fname, vme->record->name,
		mx_get_field_label_string( vme->record, vme->parameter_type ),
		vme->parameter_type));

	switch( vme->parameter_type ) {
	case MXLV_VME_READ_ADDRESS_INCREMENT:
		int32_value = vme->read_address_increment;

		visa_status = viSetAttribute( crate->session,
						VI_ATTR_SRC_INCREMENT,
						int32_value );

		if ( visa_status < VI_SUCCESS ) {
			viStatusDesc( crate->session,
				visa_status, visa_error_message );

			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to write attribute VI_ATTR_SRC_INCREMENT "
		"for record '%s', crate %lu, address %#lx.  "
		"Status code = %#lX.  Reason = '%s'",
			vme->record->name, vme->crate, vme->address,
			(unsigned long) visa_status,
			visa_error_message );
		}

		crate->old_read_address_increment =
				vme->read_address_increment;
		break;

	case MXLV_VME_WRITE_ADDRESS_INCREMENT:
		int32_value = vme->write_address_increment;

		visa_status = viSetAttribute( crate->session,
						VI_ATTR_DEST_INCREMENT,
						int32_value );

		if ( visa_status < VI_SUCCESS ) {
			viStatusDesc( crate->session,
				visa_status, visa_error_message );

			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to write attribute VI_ATTR_DEST_INCREMENT "
		"for record '%s', crate %lu, address %#lx.  "
		"Status code = %#lX.  Reason = '%s'",
			vme->record->name, vme->crate, vme->address,
			(unsigned long) visa_status,
			visa_error_message );
		}

		crate->old_write_address_increment =
				vme->write_address_increment;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%d) is not supported by the MX driver "
		"for VME interface '%s'.",
			mx_get_field_label_string(
				vme->record, vme->parameter_type ),
			vme->parameter_type, vme->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_NI_VISA */

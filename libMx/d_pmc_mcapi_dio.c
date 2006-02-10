/*
 * Name:    d_pmc_mcapi_dio.c
 *
 * Purpose: MX input and output drivers to control Precision MicroControl
 *          MCAPI digital I/O ports.
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

#define MXD_PMC_MCAPI_DIO_DEBUG  FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_PMC_MCAPI && defined(OS_WIN32)

#include "windows.h"

/* Vendor include file */

#ifndef _WIN32
#define _WIN32
#endif

#include "Mcapi.h"

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_pmc_mcapi.h"
#include "d_pmc_mcapi_dio.h"

/* Initialize the PMC_MCAPI digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_pmc_mcapi_din_record_function_list = {
	NULL,
	mxd_pmc_mcapi_din_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pmc_mcapi_din_open
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_pmc_mcapi_din_digital_input_function_list = {
	mxd_pmc_mcapi_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_pmc_mcapi_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_PMC_MCAPI_DINPUT_STANDARD_FIELDS
};

mx_length_type mxd_pmc_mcapi_din_num_record_fields
		= sizeof( mxd_pmc_mcapi_din_record_field_defaults )
			/ sizeof( mxd_pmc_mcapi_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmc_mcapi_din_rfield_def_ptr
			= &mxd_pmc_mcapi_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_pmc_mcapi_dout_record_function_list = {
	NULL,
	mxd_pmc_mcapi_dout_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pmc_mcapi_dout_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_pmc_mcapi_dout_digital_output_function_list = {
	mxd_pmc_mcapi_dout_read,
	mxd_pmc_mcapi_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_pmc_mcapi_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_PMC_MCAPI_DOUTPUT_STANDARD_FIELDS
};

mx_length_type mxd_pmc_mcapi_dout_num_record_fields
		= sizeof( mxd_pmc_mcapi_dout_record_field_defaults )
			/ sizeof( mxd_pmc_mcapi_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmc_mcapi_dout_rfield_def_ptr
			= &mxd_pmc_mcapi_dout_record_field_defaults[0];

static mx_status_type
mxd_pmc_mcapi_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_PMC_MCAPI_DINPUT **pmc_mcapi_dinput,
			MX_PMC_MCAPI **pmc_mcapi,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmc_mcapi_din_get_pointers()";

	MX_RECORD *pmc_mcapi_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( pmc_mcapi_dinput == (MX_PMC_MCAPI_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PMC_MCAPI_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( pmc_mcapi == (MX_PMC_MCAPI **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PMC_MCAPI pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pmc_mcapi_dinput = (MX_PMC_MCAPI_DINPUT *)
				dinput->record->record_type_struct;

	if ( *pmc_mcapi_dinput == (MX_PMC_MCAPI_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PMC_MCAPI_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	pmc_mcapi_record = (*pmc_mcapi_dinput)->pmc_mcapi_record;

	if ( pmc_mcapi_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PMC_MCAPI pointer for PMC_MCAPI digital input "
		"record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( pmc_mcapi_record->mx_type != MXI_GEN_PMC_MCAPI ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"pmc_mcapi_record '%s' for PMC MCAPI digital input '%s' "
		"is not a PMC MCAPI record.  Instead, it is a '%s' record.",
			pmc_mcapi_record->name, dinput->record->name,
			mx_get_driver_name( pmc_mcapi_record ) );
	}

	*pmc_mcapi = (MX_PMC_MCAPI *)
				pmc_mcapi_record->record_type_struct;

	if ( *pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PMC_MCAPI pointer for PMC MCAPI record '%s' used by "
	"PMC MCAPI digital input record '%s' and passed by '%s' is NULL.",
			pmc_mcapi_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pmc_mcapi_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_PMC_MCAPI_DOUTPUT **pmc_mcapi_doutput,
			MX_PMC_MCAPI **pmc_mcapi,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmc_mcapi_dout_get_pointers()";

	MX_RECORD *pmc_mcapi_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( pmc_mcapi_doutput == (MX_PMC_MCAPI_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PMC_MCAPI_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( pmc_mcapi == (MX_PMC_MCAPI **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PMC_MCAPI pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pmc_mcapi_doutput = (MX_PMC_MCAPI_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *pmc_mcapi_doutput == (MX_PMC_MCAPI_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PMC_MCAPI_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	pmc_mcapi_record = (*pmc_mcapi_doutput)->pmc_mcapi_record;

	if ( pmc_mcapi_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PMC_MCAPI pointer for PMC_MCAPI digital input "
		"record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( pmc_mcapi_record->mx_type != MXI_GEN_PMC_MCAPI ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"pmc_mcapi_record '%s' for PMC MCAPI digital input '%s' "
		"is not a PMC MCAPI record.  Instead, it is a '%s' record.",
			pmc_mcapi_record->name, doutput->record->name,
			mx_get_driver_name( pmc_mcapi_record ) );
	}

	*pmc_mcapi = (MX_PMC_MCAPI *)
				pmc_mcapi_record->record_type_struct;

	if ( *pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PMC_MCAPI pointer for PMC MCAPI record '%s' used by "
	"PMC MCAPI digital input record '%s' and passed by '%s' is NULL.",
			pmc_mcapi_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_pmc_mcapi_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_pmc_mcapi_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_PMC_MCAPI_DINPUT *pmc_mcapi_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        pmc_mcapi_dinput = (MX_PMC_MCAPI_DINPUT *)
				malloc( sizeof(MX_PMC_MCAPI_DINPUT) );

        if ( pmc_mcapi_dinput == (MX_PMC_MCAPI_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PMC_MCAPI_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = pmc_mcapi_dinput;
        record->class_specific_function_list
			= &mxd_pmc_mcapi_din_digital_input_function_list;

        digital_input->record = record;
	pmc_mcapi_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_din_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmc_mcapi_din_read()";

	MX_DIGITAL_INPUT *dinput;
	MX_PMC_MCAPI_DINPUT *pmc_mcapi_dinput;
	MX_PMC_MCAPI *pmc_mcapi;
	short configure_status;
	unsigned long mx_flags;
	WORD mcapi_flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed is NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_pmc_mcapi_din_get_pointers( dinput,
			&pmc_mcapi_dinput, &pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_flags = pmc_mcapi_dinput->pmc_mcapi_dinput_flags;

	mcapi_flags = 0;

	if ( mx_flags & MXF_PMC_MCAPI_DIO_LOW ) {
		mcapi_flags |= MC_DIO_LOW;
	}
	if ( mx_flags & MXF_PMC_MCAPI_DIO_HIGH ) {
		mcapi_flags |= MC_DIO_HIGH;
	}
	if ( mx_flags & MXF_PMC_MCAPI_DIO_LATCH ) {
		mcapi_flags |= MC_DIO_LATCH;
	}

	mcapi_flags |= MC_DIO_INPUT;

	configure_status = MCConfigureDigitalIO( pmc_mcapi->controller_handle,
					pmc_mcapi_dinput->channel_number,
					mcapi_flags );

	if ( configure_status != TRUE ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Could not configure digital input record '%s' with "
		"PMC MCAPI flags %#x", dinput->record->name, mcapi_flags );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_pmc_mcapi_din_read()";

	MX_PMC_MCAPI_DINPUT *pmc_mcapi_dinput;
	MX_PMC_MCAPI *pmc_mcapi;
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_din_get_pointers( dinput,
			&pmc_mcapi_dinput, &pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dinput->value = MCGetDigitalIO( pmc_mcapi->controller_handle,
					pmc_mcapi_dinput->channel_number );

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_pmc_mcapi_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_pmc_mcapi_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_PMC_MCAPI_DOUTPUT *pmc_mcapi_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        pmc_mcapi_doutput = (MX_PMC_MCAPI_DOUTPUT *)
			malloc( sizeof(MX_PMC_MCAPI_DOUTPUT) );

        if ( pmc_mcapi_doutput == (MX_PMC_MCAPI_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PMC_MCAPI_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = pmc_mcapi_doutput;
        record->class_specific_function_list
			= &mxd_pmc_mcapi_dout_digital_output_function_list;

        digital_output->record = record;
	pmc_mcapi_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_dout_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmc_mcapi_dout_read()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_PMC_MCAPI_DOUTPUT *pmc_mcapi_doutput;
	MX_PMC_MCAPI *pmc_mcapi;
	short configure_status;
	unsigned long mx_flags;
	WORD mcapi_flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed is NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_pmc_mcapi_dout_get_pointers( doutput,
			&pmc_mcapi_doutput, &pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_flags = pmc_mcapi_doutput->pmc_mcapi_doutput_flags;

	mcapi_flags = 0;

	if ( mx_flags & MXF_PMC_MCAPI_DIO_LOW ) {
		mcapi_flags |= MC_DIO_LOW;
	}
	if ( mx_flags & MXF_PMC_MCAPI_DIO_HIGH ) {
		mcapi_flags |= MC_DIO_HIGH;
	}
	if ( mx_flags & MXF_PMC_MCAPI_DIO_LATCH ) {
		mcapi_flags |= MC_DIO_LATCH;
	}

	mcapi_flags |= MC_DIO_OUTPUT;

	configure_status = MCConfigureDigitalIO( pmc_mcapi->controller_handle,
					pmc_mcapi_doutput->channel_number,
					mcapi_flags );

	if ( configure_status != TRUE ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Could not configure digital output record '%s' with "
		"PMC MCAPI flags %#x", doutput->record->name, mcapi_flags );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_pmc_mcapi_dout_read()";

	MX_PMC_MCAPI_DOUTPUT *pmc_mcapi_doutput;
	MX_PMC_MCAPI *pmc_mcapi;
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_dout_get_pointers( doutput,
			&pmc_mcapi_doutput, &pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	doutput->value = MCGetDigitalIO( pmc_mcapi->controller_handle,
					pmc_mcapi_doutput->channel_number );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmc_mcapi_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_pmc_mcapi_dout_write()";

	MX_PMC_MCAPI_DOUTPUT *pmc_mcapi_doutput;
	MX_PMC_MCAPI *pmc_mcapi;
	mx_status_type mx_status;

	mx_status = mxd_pmc_mcapi_dout_get_pointers( doutput,
			&pmc_mcapi_doutput, &pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( doutput->value != 0 ) {
		doutput->value = 1;
	}

	MCEnableDigitalIO( pmc_mcapi->controller_handle,
				pmc_mcapi_doutput->channel_number,
				(short) doutput->value );

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_PMC_MCAPI && defined(OS_WIN32) */

/*
 * Name:    d_cm17a_doutput.c
 *
 * Purpose: MX digital output driver for X10 Firecracker (CM17A) home 
 *          automation controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_CM17A_DOUTPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_cm17a.h"
#include "d_cm17a_doutput.h"

/* Initialize the CM17A pressure digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_cm17a_doutput_record_function_list = {
	NULL,
	mxd_cm17a_doutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_cm17a_doutput_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_cm17a_doutput_digital_output_function_list = {
	mxd_cm17a_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_cm17a_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_CM17A_DOUTPUT_STANDARD_FIELDS
};

mx_length_type mxd_cm17a_doutput_num_record_fields
		= sizeof( mxd_cm17a_doutput_record_field_defaults )
			/ sizeof( mxd_cm17a_doutput_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_cm17a_doutput_rfield_def_ptr
			= &mxd_cm17a_doutput_record_field_defaults[0];

static mx_status_type
mxd_cm17a_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_CM17A_DOUTPUT **cm17a_doutput,
			MX_CM17A **cm17a,
			const char *calling_fname )
{
	static const char fname[] = "mxd_cm17a_doutput_get_pointers()";

	MX_RECORD *cm17a_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( cm17a_doutput == (MX_CM17A_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_CM17A_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( cm17a == (MX_CM17A **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_CM17A pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*cm17a_doutput = (MX_CM17A_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *cm17a_doutput == (MX_CM17A_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_CM17A_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	cm17a_record = (*cm17a_doutput)->cm17a_record;

	if ( cm17a_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CM17A pointer for CM17A digital output "
		"record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( cm17a_record->mx_type != MXI_GEN_CM17A ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"cm17a_record '%s' for CM17A digital output '%s' "
		"is not a CM17A record.  Instead, it is a '%s' record.",
			cm17a_record->name, doutput->record->name,
			mx_get_driver_name( cm17a_record ) );
	}

	*cm17a = (MX_CM17A *) cm17a_record->record_type_struct;

	if ( *cm17a == (MX_CM17A *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_CM17A pointer for CM17A record '%s' used by "
	"CM17A digital output record '%s' and passed by '%s' is NULL.",
			cm17a_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_cm17a_doutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_cm17a_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_CM17A_DOUTPUT *cm17a_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *) malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        cm17a_doutput = (MX_CM17A_DOUTPUT *)
				malloc( sizeof(MX_CM17A_DOUTPUT) );

        if ( cm17a_doutput == (MX_CM17A_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_CM17A_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = cm17a_doutput;
        record->class_specific_function_list
			= &mxd_cm17a_doutput_digital_output_function_list;

        digital_output->record = record;
	cm17a_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cm17a_doutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_cm17a_doutput_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_CM17A_DOUTPUT *cm17a_doutput;
	MX_CM17A *cm17a;
	int house_index, device_index;
	uint16_t house_and_device_code;
	mx_status_type mx_status;

	static const uint16_t house_code[16] = {
		0x6000,		/* A */
		0x7000,		/* B */
		0x4000,		/* C */
		0x5000,		/* D */
		0x8000,		/* E */
		0x9000,		/* F */
		0xA000,		/* G */
		0xB000,		/* H */
		0xE000,		/* I */
		0xF000,		/* J */
		0xC000,		/* K */
		0xD000,		/* L */
		0x0000,		/* M */
		0x1000,		/* N */
		0x2000,		/* O */
		0x3000,		/* P */
	};

	static const uint16_t device_code[16] = {
		0x0000,		/*  1 */
		0x0010,		/*  2 */
		0x0008,		/*  3 */
		0x0018,		/*  4 */
		0x0040,		/*  5 */
		0x0050,		/*  6 */
		0x0048,		/*  7 */
		0x0058,		/*  8 */
		0x0400,		/*  9 */
		0x0410,		/* 10 */
		0x0408,		/* 11 */
		0x0418,		/* 12 */
		0x0440,		/* 13 */
		0x0450,		/* 14 */
		0x0448,		/* 15 */
		0x0458,		/* 16 */
	};

	static const uint16_t command_code[4] = {
		0x0000,		/* ON         */
		0x0020,		/* OFF        */
		0x0088,		/* BRIGHT 005 */
		0x0098,		/* DIM    005 */
	};

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_cm17a_doutput_get_pointers( doutput,
					&cm17a_doutput, &cm17a, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the house code and device code are legal. */

	house_index  = cm17a_doutput->house_code - 'A';
	device_index = cm17a_doutput->device_code - 1;

	if ( (house_index < 0) || (house_index >= 16 ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal house code '%c' selected for CM17A digital output '%s'.  "
	"The allowed values are from 'A' to 'P'.",
			cm17a_doutput->house_code, record->name );
	}
	if ( (device_index < 0) || (device_index >= 16 ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal device code %d selected for CM17A digital output '%s'.  "
	"The allowed values are from 1 to 16.",
			(int) cm17a_doutput->device_code, record->name );
	}

	/* Compute the values of all the commands. */

	house_and_device_code =
		house_code[house_index] | device_code[device_index];

	cm17a_doutput->on_command     = command_code[0] | house_and_device_code;
	cm17a_doutput->off_command    = command_code[1] | house_and_device_code;
	cm17a_doutput->bright_command = command_code[2] | house_and_device_code;
	cm17a_doutput->dim_command    = command_code[3] | house_and_device_code;

#if MXD_CM17A_DOUTPUT_DEBUG
	MX_DEBUG(-2,
    ("%s: CM17A doutput '%s', on = %#x, off = %#x, bright = %#x, dim = %#x",
 		fname, record->name,
		cm17a_doutput->on_command, cm17a_doutput->off_command, 
		cm17a_doutput->bright_command, cm17a_doutput->dim_command ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cm17a_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_cm17a_doutput_write()";

	MX_CM17A_DOUTPUT *cm17a_doutput;
	MX_CM17A *cm17a;
	uint16_t command;
	mx_status_type mx_status;

	mx_status = mxd_cm17a_doutput_get_pointers( doutput,
					&cm17a_doutput, &cm17a, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( doutput->value ) {
	case 0:
		command = cm17a_doutput->off_command;
		break;
	case 1:
		command = cm17a_doutput->on_command;
		break;
	case 2:
		command = cm17a_doutput->bright_command;
		break;
	case 3: command = cm17a_doutput->dim_command;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal value %lu for CM17A digital output '%s'.  The allowed "
		"values are 0 (off), 1 (on), 2 (bright), and 3 (dim).",
			(unsigned long) doutput->value, doutput->record->name );
	}

#if MXD_CM17A_DOUTPUT_DEBUG
	MX_DEBUG(-2,("%s: CM17A doutput '%s', command = %#lx",
		fname, doutput->record->name, (unsigned long) command ));
#endif

	mx_status = mxi_cm17a_command( cm17a, command,
					MXD_CM17A_DOUTPUT_DEBUG );

	return mx_status;
}


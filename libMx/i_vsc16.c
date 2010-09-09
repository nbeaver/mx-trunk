/*
 * Name:    i_vsc16.c
 *
 * Purpose: MX driver for Joerger VSC-16 counter/timers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2003, 2005-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_VSC16_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_vme.h"
#include "i_vsc16.h"

MX_RECORD_FUNCTION_LIST mxi_vsc16_record_function_list = {
	NULL,
	mxi_vsc16_create_record_structures,
	mxi_vsc16_finish_record_initialization,
	NULL,
	NULL,
	mxi_vsc16_open
};

MX_RECORD_FIELD_DEFAULTS mxi_vsc16_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_VSC16_STANDARD_FIELDS
};

long mxi_vsc16_num_record_fields
		= sizeof( mxi_vsc16_record_field_defaults )
			/ sizeof( mxi_vsc16_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_vsc16_rfield_def_ptr
			= &mxi_vsc16_record_field_defaults[0];

/*==========================*/

MX_EXPORT mx_status_type
mxi_vsc16_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_vsc16_create_record_structures()";

	MX_VSC16 *vsc16;

	/* Allocate memory for the necessary structures. */

	vsc16 = (MX_VSC16 *) malloc( sizeof(MX_VSC16) );

	if ( vsc16 == (MX_VSC16 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_VSC16 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = vsc16;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	vsc16->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vsc16_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_vsc16_finish_record_initialization()";

	MX_VSC16 *vsc16;
	int i;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vsc16 = (MX_VSC16 *) record->record_type_struct;

	if ( vsc16 == (MX_VSC16 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_VSC16 pointer for record '%s' is NULL.", record->name);
	}

	/* Zero out the list of attached counters. */

	for ( i = 0; i < MX_MAX_VSC16_CHANNELS; i++ ) {
		vsc16->counter_record[i] = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vsc16_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_vsc16_open()";

	MX_VSC16 *vsc16;
	MX_RECORD *vme_record;
	unsigned long crate, base;
	uint16_t manufacturer_id, module_type, serial_number, test_mask;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	MX_DEBUG( 2, ("%s invoked for record '%s'.", fname, record->name ));

	vsc16 = (MX_VSC16 *) (record->record_type_struct);

	if ( vsc16 == (MX_VSC16 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_VSC16 pointer for record '%s' is NULL.", record->name);
	}

	vme_record = vsc16->vme_record;

	if ( vme_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"vme_record pointer for Joerger VSC16 record '%s' is NULL.",
			record->name );
	}

	crate = vsc16->crate_number;
	base  = vsc16->base_address;

	/* Verify that this is a Joerger VME module. */

	mx_status = mx_vme_in16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_MANUFACTURER_ID,
					&manufacturer_id );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Joerger manufacturer ID is 'J' or 0x4a. */

#if !defined(OS_RTEMS)
	/* FIXME: For some reason, RTEMS crashes if the following code
	 * is executed.
	 */

	if ( manufacturer_id != 'J' ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The module in VME crate '%s' at A32 base address %#lx "
	"is not a Joerger VME module.  "
	"The manufacturer id reported was '%c' when it should be 'J'.",
			vme_record->name, base,
			(char) manufacturer_id );
	}
#endif

	/* Verify that this is a Joerger VSC-16 or VSC-8. */

	mx_status = mx_vme_in16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_MODULE_TYPE,
					&module_type );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	test_mask = ( MXF_VSC16_8_CHANNEL | MXF_VSC16_16_CHANNEL );

	if ( ( module_type & test_mask ) == 0 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The module in VME crate '%s' at A32 base address %#lx "
	"is not a Joerger VSC-16 or VSC-8.  "
	"The module type reported was %d, when it should be 8, 9, 16, or 17.",
			vme_record->name, base, (int) module_type );
	}

	vsc16->module_type = module_type;

	if ( module_type & MXF_VSC16_16_CHANNEL ) {
		vsc16->num_counters = 16;
	} else {
		vsc16->num_counters = 8;
	}

	/* Record the serial number for posterity. */

	mx_status = mx_vme_in16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_SERIAL_NUMBER_REGISTER,
					&serial_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vsc16->serial_number = serial_number;

	/* Since we have established that we are talking to the
	 * right kind of module, we can now configure it.
	 */

	/* First, reset the module to a known state. */

	mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_RESET_MODULE,
					0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Disarm the module and set it to disarm on internal IRQ (e.g.,
	 * overflow or underflow).
	 */

	mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_CONTROL_REGISTER,
					0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Program all of the counters to count up. */

	mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_DIRECTION_REGISTER,
					0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Disable VME bus IRQs. */

	mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_IRQ_LEVEL_EN_REGISTER,
					0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable IRQs for all of the individual channels. */

	mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_IRQ_MASK_REGISTER,
					0xffff );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get rid of any left over interrupt request. */

	mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_IRQ_RESET,
					0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}


/*
 * Name:    i_sis3807.c
 *
 * Purpose: MX MCS driver for the Struck SIS3807 multichannel pulse generator.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2005-2007, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_SIS3807_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_vme.h"

#include "i_sis3807.h"

MX_RECORD_FUNCTION_LIST mxi_sis3807_record_function_list = {
	NULL,
	mxi_sis3807_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_sis3807_open,
	mxi_sis3807_close,
	NULL,
	mxi_sis3807_open
};

MX_RECORD_FIELD_DEFAULTS mxi_sis3807_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_SIS3807_STANDARD_FIELDS
};

long mxi_sis3807_num_record_fields
		= sizeof( mxi_sis3807_record_field_defaults )
			/ sizeof( mxi_sis3807_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_sis3807_rfield_def_ptr
			= &mxi_sis3807_record_field_defaults[0];

/* --- */

static mx_status_type
mxi_sis3807_get_pointers( MX_RECORD *sis3807_record,
			MX_SIS3807 **sis3807,
			const char *calling_fname )
{
	static const char fname[] = "mxi_sis3807_get_pointers()";

	if ( sis3807_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( sis3807 == (MX_SIS3807 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SIS3807 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*sis3807 = (MX_SIS3807 *) sis3807_record->record_type_struct;

	if ( *sis3807 == (MX_SIS3807 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SIS3807 pointer for record '%s' is NULL.",
			sis3807_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxi_sis3807_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_sis3807_create_record_structures()";

	MX_SIS3807 *sis3807;

	sis3807 = (MX_SIS3807 *) malloc( sizeof(MX_SIS3807) );

	if ( sis3807 == (MX_SIS3807 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_SIS3807 structure." );
	}

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	record->record_type_struct = sis3807;

	sis3807->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sis3807_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_sis3807_open()";

	MX_SIS3807 *sis3807;
	int i;
	uint32_t module_id_register, control_register, preset_register;
	mx_status_type mx_status;

	sis3807 = NULL;

	mx_status = mxi_sis3807_get_pointers( record, &sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the address mode string. */

#if MXI_SIS3807_DEBUG
	MX_DEBUG(-2,("%s invoked for SIS3807 '%s'", fname, record->name));

	MX_DEBUG(-2,("%s: sis3807->address_mode_name = '%s'",
			fname, sis3807->address_mode_name));
#endif

	mx_status = mx_vme_parse_address_mode( sis3807->address_mode_name,
						&(sis3807->address_mode) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_SIS3807_DEBUG
	MX_DEBUG(-2,("%s: sis3807->address_mode = %#lx",
			fname, sis3807->address_mode));
#endif

	/* Reset the SIS3807. */

	mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_RESET_REG,
				1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the module identification register. */

	mx_status = mx_vme_in32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
			sis3807->base_address + MX_SIS3807_MODULE_ID_REG,
				&module_id_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sis3807->module_id = ( module_id_register >> 16 ) & 0xffff;
	sis3807->firmware_version = ( module_id_register >> 12 ) & 0xf;

	/* Fix an apparent bug in the handling of version numbers. */

	if ( sis3807->firmware_version == 0 ) {
		sis3807->firmware_version = 1;
	}

#if MXI_SIS3807_DEBUG
	MX_DEBUG(-2,("%s: module id = %#lx, firmware version = %#lx",
		fname, sis3807->module_id, sis3807->firmware_version));
#endif

	/* Make firmware version dependent checks. */

	sis3807->burst_register_available = FALSE;
	sis3807->pulse_width_available = FALSE;

	switch( sis3807->firmware_version ) {
	case 1:
		if ( sis3807->num_channels != 4 ) {
			mx_warning(
	"SIS3807 module '%s' is using version %#lx firmware which only "
	"supports 4 channel operation.  Changing num_channels from %ld to 4.",
			sis3807->record->name,
			sis3807->firmware_version,
			sis3807->num_channels );

			sis3807->num_channels = 4;
		}
		break;
	case 2:
		if ( ( sis3807->num_channels != 4 )
		  && ( sis3807->num_channels != 2 ) )
		{
			mx_warning(
	"SIS3807 module '%s' is using version %#lx firmware which only "
	"supports 4 channel or 2 channel operation.  "
	"Changing num_channels from %ld to 4.",
			sis3807->record->name,
			sis3807->firmware_version,
			sis3807->num_channels );

			sis3807->num_channels = 4;
		}
		break;
	case 3:
	case 4:
		if ( sis3807->num_channels != 1 ) {
			mx_warning(
	"SIS3807 module '%s' is using version %#lx firmware which only "
	"supports 1 channel operation.  Changing num_channels from %ld to 1.",
			sis3807->record->name,
			sis3807->firmware_version,
			sis3807->num_channels );

			sis3807->num_channels = 1;
		}

		sis3807->burst_register_available = TRUE;
		sis3807->pulse_width_available = TRUE;

		break;
	default:
		if ( sis3807->num_channels != 1 ) {
			mx_warning(
	"SIS3807 module '%s' is using unrecognized firmware version %#lx.  "
	"Since we do not know how many channels this firmware supports, "
	"we will play it safe and change num_channels from %ld to 1.",
			sis3807->record->name,
			sis3807->firmware_version,
			sis3807->num_channels );

			sis3807->num_channels = 1;
		}
		break;
	}

	/* After a reset, most of the control register bits are set the
	 * way that we want.  The only things we want to change are:
	 * 1.  Select 2 channel mode if requested and available.
	 * 2.  Turn on the factor 8 predivider if requested.
	 * 3.  Turn on the general enable bit.
	 *
	 * The individual channels will not start pulsing until their
	 * individual enable bits are turned on.
	 */

	control_register = 0;

	/* Select 2 channel mode if requested. */

	if ( sis3807->num_channels == 2 ) {
		control_register |= MXF_SIS3807_SET_2_CHANNEL_MODE_BIT;
	}

	/* Turn on the factor 8 predivider if requested. */

	if ( sis3807->enable_factor_8_predivider ) {
		control_register |=
			MXF_SIS3807_ENABLE_COMMON_FACTOR_8_PREDIVIDER_BIT;
	}

	/* Program the control register. */

	mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_CONTROL_REG,
				control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set all the pulser presets to 0.  This means that the default
	 * pulse period is as small as possible.
	 */

	for ( i = 0; i < sis3807->num_channels; i++ ) {

		preset_register =
			(uint32_t) ( sis3807->base_address + 0x40 + 0x4 * i );

		mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				preset_register,
				0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Turn on the general enable bit. */

	mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_CONTROL_REG,
				MXF_SIS3807_GENERAL_ENABLE_BIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the output invert register. */

	if ( sis3807->firmware_version == 1 ) {
		/* Firmware version 1 does not have an output invert register.
		 * Instead we invert outputs by setting bit 2 in the control
		 * register.  In later versions of the firmware, bit 2 is
		 * reused as the "set 2 channel mode bit" which is what we
		 * call it here.
		 */

		if ( sis3807->output_invert_register ) {
			mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_CONTROL_REG,
				MXF_SIS3807_SET_2_CHANNEL_MODE_BIT );
		}
	} else {
		/* Firmware versions 2 and after have
		 * the output invert register.
		 */

		mx_status = mx_vme_out32( sis3807->vme_record,
					sis3807->crate_number,
					sis3807->address_mode,
			sis3807->base_address + MX_SIS3807_OUTPUT_INVERT_REG,
				(uint32_t) sis3807->output_invert_register );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_SIS3807_DEBUG
	MX_DEBUG(-2,("%s: Turning on user LED.", fname));
#endif

	control_register = 1;

	mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_CONTROL_REG,
				control_register );

#if MXI_SIS3807_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_sis3807_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_sis3807_close()";

	MX_SIS3807 *sis3807;
	uint32_t control_register;
	mx_status_type mx_status;

	sis3807 = NULL;

	mx_status = mxi_sis3807_get_pointers( record, &sis3807, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_SIS3807_DEBUG
	MX_DEBUG(-2,("%s: Turning off user LED.", fname));
#endif

	control_register = 1 << 8;

	mx_status = mx_vme_out32( sis3807->vme_record,
				sis3807->crate_number,
				sis3807->address_mode,
				sis3807->base_address + MX_SIS3807_CONTROL_REG,
				control_register );

	return mx_status;
}


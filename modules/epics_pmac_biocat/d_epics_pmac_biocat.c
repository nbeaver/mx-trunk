/*
 * Name:    d_epics_pmac_biocat.c 
 *
 * Purpose: MX motor driver for controlling a Delta Tau PMAC motor through
 *          the BioCAT version of Tom Coleman's EPICS PMAC database.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EPICS_PMAC_BIOCAT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_epics.h"
#include "mx_motor.h"
#include "d_epics_pmac_biocat.h"

MX_RECORD_FUNCTION_LIST mxd_epics_pmac_biocat_record_function_list = {
	NULL,
	mxd_epics_pmac_biocat_create_record_structures,
	mxd_epics_pmac_biocat_finish_record_initialization
};

MX_MOTOR_FUNCTION_LIST mxd_epics_pmac_biocat_motor_function_list = {
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxd_epics_pmac_biocat_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_EPICS_PMAC_BIOCAT_STANDARD_FIELDS
};

long mxd_epics_pmac_biocat_num_record_fields
	= sizeof( mxd_epics_pmac_biocat_record_field_defaults )
		/ sizeof( mxd_epics_pmac_biocat_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_pmac_biocat_rfield_def_ptr
			= &mxd_epics_pmac_biocat_record_field_defaults[0];


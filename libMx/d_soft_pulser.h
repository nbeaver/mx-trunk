/*
 * Name:    d_soft_pulser.h
 *
 * Purpose: Header file for a simulated MX pulse generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_PULSER_H__
#define __D_SOFT_PULSER_H__

#include "mx_pulse_generator.h"

typedef struct {
	MX_RECORD *record;
} MX_SOFT_PULSER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_soft_pulser_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_pulser_open( MX_RECORD *record );

MX_API mx_status_type mxd_soft_pulser_is_busy( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_soft_pulser_arm( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_soft_pulser_trigger( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_soft_pulser_stop( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_soft_pulser_get_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_soft_pulser_set_parameter(
					MX_PULSE_GENERATOR *pulser );

extern MX_RECORD_FUNCTION_LIST mxd_soft_pulser_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST
				mxd_soft_pulser_pulse_generator_function_list;

extern long mxd_soft_pulser_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_pulser_rfield_def_ptr;

#define MXD_SOFT_PULSER_STANDARD_FIELDS

#endif /* __D_SOFT_PULSER_H__ */


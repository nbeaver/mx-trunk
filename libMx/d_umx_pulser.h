/*
 * Name:    d_umx_pulser.h
 *
 * Purpose: Header file for UMX-based microcontroller pulse generators.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_UMX_PULSER_H__
#define __D_UMX_PULSER_H__

#include "mx_pulse_generator.h"

#define MXF_UMX_PULSER_DEBUG			0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *umx_record;
	long pulser_number;
	unsigned long umx_pulser_flags;
} MX_UMX_PULSER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_umx_pulser_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_umx_pulser_open( MX_RECORD *record );

MX_API mx_status_type mxd_umx_pulser_is_busy( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_umx_pulser_arm( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_umx_pulser_trigger(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_umx_pulser_stop( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_umx_pulser_get_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_umx_pulser_set_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_umx_pulser_setup( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_umx_pulser_get_status( MX_PULSE_GENERATOR *pulser );

extern MX_RECORD_FUNCTION_LIST mxd_umx_pulser_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST mxd_umx_pulser_pulser_function_list;

extern long mxd_umx_pulser_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_umx_pulser_rfield_def_ptr;

#define MXD_UMX_PULSER_STANDARD_FIELDS \
  {-1, -1, "umx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_PULSER, umx_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "pulser_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UMX_PULSER, pulser_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "umx_pulser_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_UMX_PULSER, umx_pulser_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_UMX_PULSER_H__ */


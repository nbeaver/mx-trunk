/*
 * Name:    d_gittelson_pulser.h
 *
 * Purpose: Header file for Mark Gittelson's Arduino-based pulse generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GITTELSON_PULSER_H__
#define __D_GITTELSON_PULSER_H__

#include "mx_pulse_generator.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
} MX_GITTELSON_PULSER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_gittelson_pulser_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_gittelson_pulser_open( MX_RECORD *record );

MX_API mx_status_type mxd_gittelson_pulser_is_busy( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_gittelson_pulser_start( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_gittelson_pulser_stop( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_gittelson_pulser_get_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_gittelson_pulser_set_parameter(
					MX_PULSE_GENERATOR *pulser );

extern MX_RECORD_FUNCTION_LIST mxd_gittelson_pulser_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST mxd_gittelson_pulser_pulser_function_list;

extern long mxd_gittelson_pulser_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_gittelson_pulser_rfield_def_ptr;

#define MXD_GITTELSON_PULSER_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GITTELSON_PULSER, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \

#endif /* __D_GITTELSON_PULSER_H__ */


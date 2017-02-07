/*
 * Name:    d_dg645_pulser.h
 *
 * Purpose: Header file for the Stanford Research Systems DG645 Digital
 *          Pulse Generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DG645_PULSER_H__
#define __D_DG645_PULSER_H__

#include "mx_pulse_generator.h"

#define MXU_DG645_PULSER_CONNECTOR_LENGTH	40

typedef struct {
	MX_RECORD *record;

	MX_RECORD *dg645_record;
	char channel_name[MXU_DG645_PULSER_CONNECTOR_LENGTH+1];
	unsigned long dg645_pulser_flags;

} MX_DG645_PULSER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_dg645_pulser_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_dg645_pulser_open( MX_RECORD *record );

MX_API mx_status_type mxd_dg645_pulser_is_busy( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_dg645_pulser_start( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_dg645_pulser_stop( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_dg645_pulser_get_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_dg645_pulser_set_parameter(
					MX_PULSE_GENERATOR *pulser );

extern MX_RECORD_FUNCTION_LIST mxd_dg645_pulser_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST mxd_dg645_pulser_pulser_function_list;

extern long mxd_dg645_pulser_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_dg645_pulser_rfield_def_ptr;

#define MXD_DG645_PULSER_STANDARD_FIELDS \
  {-1, -1, "dg645_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645_PULSER, dg645_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "channel_name", \
		MXFT_STRING, NULL, 1, {MXU_DG645_PULSER_CONNECTOR_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645_PULSER, channel_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "dg645_pulser_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645_PULSER, dg645_pulser_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_DG645_PULSER_H__ */


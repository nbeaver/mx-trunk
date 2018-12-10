/*
 * Name:    d_gittelsohn_pulser.h
 *
 * Purpose: Header file for Mark Gittelsohn's Arduino-based pulse generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GITTELSOHN_PULSER_H__
#define __D_GITTELSOHN_PULSER_H__

#include "mx_pulse_generator.h"

#define MXF_GITTELSOHN_PULSER_DEBUG			0x1
#define MXF_GITTELSOHN_PULSER_AUTO_RESYNC_ON_ERROR	0x2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long gittelsohn_pulser_flags;
	double timeout;
	unsigned long max_retries;

	double firmware_version;

	struct timespec last_internal_start_time;
} MX_GITTELSOHN_PULSER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_gittelsohn_pulser_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_gittelsohn_pulser_open( MX_RECORD *record );

MX_API mx_status_type mxd_gittelsohn_pulser_is_busy( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_gittelsohn_pulser_arm( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_gittelsohn_pulser_trigger(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_gittelsohn_pulser_stop( MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_gittelsohn_pulser_get_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_gittelsohn_pulser_set_parameter(
					MX_PULSE_GENERATOR *pulser );
MX_API mx_status_type mxd_gittelsohn_pulser_setup( MX_PULSE_GENERATOR *pulser );

extern MX_RECORD_FUNCTION_LIST mxd_gittelsohn_pulser_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST mxd_gittelsohn_pulser_pulser_function_list;

extern long mxd_gittelsohn_pulser_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_gittelsohn_pulser_rfield_def_ptr;

#define MXD_GITTELSOHN_PULSER_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GITTELSOHN_PULSER, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "gittelsohn_pulser_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_GITTELSOHN_PULSER, gittelsohn_pulser_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) } , \
  \
  {-1, -1, "max_retries", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GITTELSOHN_PULSER, max_retries), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) } , \
  \
  {-1, -1, "timeout", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GITTELSOHN_PULSER, timeout), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) } , \
  \
  {-1, -1, "firmware_version", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GITTELSOHN_PULSER, firmware_version), \
	{0}, NULL, (MXFF_READ_ONLY) }

#endif /* __D_GITTELSOHN_PULSER_H__ */


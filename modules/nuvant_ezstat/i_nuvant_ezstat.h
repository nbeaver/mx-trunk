/*
 * Name:    i_nuvant_ezstat.h
 *
 * Purpose: Header for NuVant EZstat controller driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NUVANT_EZSTAT_H__
#define __I_NUVANT_EZSTAT_H__

#define MXF_NUVANT_EZSTAT_POTENTIOSTAT_MODE	0
#define MXF_NUVANT_EZSTAT_GALVANOSTAT_MODE	1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *ni_daqmx_record;

	MX_RECORD *ai0_record;
	MX_RECORD *ai1_record;
	MX_RECORD *ai2_record;
	MX_RECORD *ai3_record;

	MX_RECORD *ao0_record;

	MX_RECORD *p00_record;
	MX_RECORD *p01_record;
	MX_RECORD *p10_record;
	MX_RECORD *p11_record;
	MX_RECORD *p12_record;
	MX_RECORD *p13_record;
	MX_RECORD *p14_record;

	unsigned long mode;

} MX_NUVANT_EZSTAT;

#define MXI_NUVANT_EZSTAT_STANDARD_FIELDS \
  {-1, -1, "ni_daqmx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, ni_daqmx_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "ai0_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, ai0_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "ai1_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, ai1_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "ai2_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, ai2_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "ai3_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, ai3_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "ao0_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, ao0_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "p00_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, p00_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "p01_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, p01_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "p10_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, p10_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "p11_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, p11_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "p12_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, p12_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "p13_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, p13_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "p14_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, p14_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

MX_API mx_status_type mxi_nuvant_ezstat_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_nuvant_ezstat_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_nuvant_ezstat_record_function_list;

extern long mxi_nuvant_ezstat_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_nuvant_ezstat_rfield_def_ptr;

#endif /* __I_NUVANT_EZSTAT_H__ */


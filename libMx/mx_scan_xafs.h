/*
 * Name:    mx_scan_xafs.h
 *
 * Purpose: Header file for MX_XAFS_SCAN scan records.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_SCAN_XAFS_H_
#define _MX_SCAN_XAFS_H_

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Note: The fields 'num_regions' and 'num_boundaries' below are, in principle,
 *       redundant.  They are here anyway because the varargs field processing
 *       does not have any provision for doing arithmetic in it.
 */

typedef struct {
	MX_SCAN *scan;	/* Pointer to the parent scan superclass structure. */

	long num_regions;
	long num_energy_regions;
	long num_k_regions;
	long num_boundaries;

	double *region_boundary;
	double *region_step_size;
	double *region_measurement_time;
} MX_XAFS_SCAN;

MX_API_PRIVATE mx_status_type mxs_xafs_scan_initialize_type( long type );
MX_API_PRIVATE mx_status_type mxs_xafs_scan_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxs_xafs_scan_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxs_xafs_scan_delete_record( MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxs_xafs_scan_print_scan_structure(
						FILE *file, MX_RECORD *record);

MX_API_PRIVATE mx_status_type mxs_xafs_scan_prepare_for_scan_start(
							MX_SCAN *scan);
MX_API_PRIVATE mx_status_type mxs_xafs_scan_execute_scan_body(
							MX_SCAN *scan );
MX_API_PRIVATE mx_status_type mxs_xafs_scan_cleanup_after_scan_end(
							MX_SCAN *scan );

extern MX_RECORD_FUNCTION_LIST mxs_xafs_scan_record_function_list;
extern MX_SCAN_FUNCTION_LIST mxs_xafs_scan_scan_function_list;

#define MX_XAFS_SCAN_STANDARD_FIELDS \
  {-1, -1, "num_regions", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_XAFS_SCAN, num_regions), \
	{sizeof(long)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  {-1, -1, "num_energy_regions", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_XAFS_SCAN, num_energy_regions), \
	{sizeof(long)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  {-1, -1, "num_k_regions", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_XAFS_SCAN, num_k_regions), \
	{sizeof(long)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  {-1, -1, "num_boundaries", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_XAFS_SCAN, num_boundaries), \
	{sizeof(long)}, NULL, \
	(MXFF_IN_DESCRIPTION)}, \
  {-1, -1, "region_boundary", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_XAFS_SCAN, region_boundary), \
	{sizeof(double)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  {-1, -1, "region_step_size", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_XAFS_SCAN, region_step_size), \
	{sizeof(double)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  {-1, -1, "region_measurement_time", MXFT_DOUBLE, \
	NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_XAFS_SCAN, region_measurement_time),\
	{sizeof(double)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}

#ifdef __cplusplus
}
#endif

#endif /* _MX_SCAN_XAFS_H_ */


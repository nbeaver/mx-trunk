/*
 * Name:    mx_scan_area_detector.h
 *
 * Purpose: Header file for MX_AREA_DETECTOR_SCAN records.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_SCAN_AREA_DETECTOR_H_
#define _MX_SCAN_AREA_DETECTOR_H_

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_namefix.h"

typedef struct {
	MX_SCAN *scan; /* Pointer to the parent scan superclass structure. */

	mx_bool_type use_inverse_beam;

	MX_RECORD *energy_record;
	long num_energies;
	double *energy_array;

	long num_static_motors;
	MX_RECORD **static_motor_array;
	double *static_motor_positions;

	double *start_position;
	double *step_size;
	long *num_frames;

	long current_energy_number;
	long current_frame_number;

	void *record_type_struct;
} MX_AREA_DETECTOR_SCAN;

#define MX_AREA_DETECTOR_SCAN_STANDARD_FIELDS \
  {-1, -1, "use_inverse_beam", MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR_SCAN, use_inverse_beam), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "energy_record", MXFT_RECORD, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR_SCAN, energy_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_energies", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR_SCAN, num_energies), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "energy_array", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR_SCAN, energy_array), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}, \
  \
  {-1, -1, "num_static_motors", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR_SCAN, num_static_motors), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "static_motor_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
  	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR_SCAN, static_motor_array), \
	{sizeof(MX_RECORD *)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}, \
  \
  {-1, -1, "static_motor_positions", MXFT_DOUBLE, NULL, \
  		1, {MXU_VARARGS_LENGTH}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR_SCAN, static_motor_positions), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}, \
  \
  {-1, -1, "start_position", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR_SCAN, start_position), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}, \
  \
  {-1, -1, "step_size", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR_SCAN, step_size), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}, \
  \
  {-1, -1, "num_frames", MXFT_LONG, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR_SCAN, num_frames), \
	{sizeof(long)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}

typedef struct {
	mx_status_type ( *create_record_structures ) (
				MX_RECORD *record,
				MX_SCAN *scan,
				MX_AREA_DETECTOR_SCAN *ad_scan );
	mx_status_type ( *finish_record_initialization ) (
				MX_RECORD *record );
	mx_status_type ( *prepare_for_scan_start ) (
				MX_SCAN *scan );
	mx_status_type ( *execute_scan_body ) (
				MX_SCAN *scan );
	mx_status_type ( *cleanup_after_scan_end ) (
				MX_SCAN *scan );

	mx_status_type ( *initialize_datafile_naming ) ( MX_SCAN *scan );
	mx_status_type ( *construct_next_datafile_name ) ( MX_SCAN *scan );
} MX_AREA_DETECTOR_SCAN_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mxs_area_detector_scan_initialize_type( long );
MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_create_record_structures( MX_RECORD *record );
MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_finish_record_initialization( MX_RECORD *record);
MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_delete_record( MX_RECORD *record);

MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_print_structure( FILE *file, MX_RECORD *record);

MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_prepare_for_scan_start( MX_SCAN *scan);
MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_execute_scan_body( MX_SCAN *scan);
MX_API_PRIVATE mx_status_type
	mxs_area_detector_scan_cleanup_after_scan_end( MX_SCAN *scan);

extern MX_RECORD_FUNCTION_LIST mxs_area_detector_scan_record_function_list;
extern MX_SCAN_FUNCTION_LIST mxs_area_detector_scan_scan_function_list;

/*---*/

MX_API_PRIVATE mx_status_type
mxs_area_detector_scan_default_initialize_datafile_naming( MX_SCAN *scan );

MX_API_PRIVATE mx_status_type
mxs_area_detector_scan_default_construct_next_datafile_name( MX_SCAN *scan );

/*---*/

MX_API mx_status_type
mx_area_detector_scan_setup_datafile_naming( MX_RECORD *scan_record,
				mx_status_type (*initialize_fn)(MX_SCAN *),
				mx_status_type (*construct_fn)(MX_SCAN *) );

#ifdef __cplusplus
}
#endif

#endif /* _MX_SCAN_AREA_DETECTOR_H_ */


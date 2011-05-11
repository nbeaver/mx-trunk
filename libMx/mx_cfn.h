/*
 * Name:     mx_cfn.h
 *
 * Purpose:  Functions for constructing MX control system filenames.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006-2007, 2009, 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_CFN_H__
#define __MX_CFN_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

				/*  +---- default on Linux/Unix
				 *  |
				 *  v
				 */

#define MX_CFN_PROGRAM	1	/* bin */
#define MX_CFN_CONFIG	2	/* etc */
#define MX_CFN_INCLUDE	3	/* include */
#define MX_CFN_LIBRARY	4	/* lib */
#define MX_CFN_LOGFILE	5	/* log */
#define MX_CFN_RUNFILE	6	/* run */
#define MX_CFN_SYSTEM	7	/* sbin */
#define MX_CFN_STATE	8	/* state */
#define MX_CFN_SCAN	9	/* $HOME */
#define MX_CFN_USER	10	/* $HOME/.mx */
#define MX_CFN_CWD	11	/* . */
#define MX_CFN_ABSOLUTE	12	/* no default */
#define MX_CFN_MODULE	13	/* lib/modules */

MX_API FILE *mx_cfn_fopen( int filename_type,
			const char *filename,
			const char *mode );

MX_API mx_status_type mx_cfn_construct_filename( int filename_type,
					const char *original_filename,
					char *new_filename,
					size_t max_filename_length );

MX_API mx_bool_type mx_is_absolute_filename( const char *filename );

MX_API char *mx_expand_filename_macros( const char *original_filename,
						char *new_filename,
						size_t max_filename_length );

MX_API char *mx_normalize_filename( const char *original_filename,
						char *new_filename,
						size_t max_filename_length );

/*--- Flag bits used by mx_find_file_in_path() ---*/

#define MXF_FPATH_TRY_WITHOUT_EXTENSION		0x1
#define MXF_FPATH_LOOK_IN_CURRENT_DIRECTORY	0x2

MX_API char *mx_find_file_in_path( const char *original_filename,
					const char *extension,
					const char *path_variable_name,
					unsigned long flags );

MX_API int mx_path_variable_split( char *path_variable_name,
					int *argc, char ***argv );

/*---*/

#define mx_cfn_construct_program_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_PROGRAM, (o), (n), (s) )

#define mx_cfn_construct_config_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_CONFIG, (o), (n), (s) )

#define mx_cfn_construct_include_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_INCLUDE, (o), (n), (s) )

#define mx_cfn_construct_library_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_LIBRARY, (o), (n), (s) )

#define mx_cfn_construct_logfile_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_LOGFILE, (o), (n), (s) )

#define mx_cfn_construct_runfile_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_RUNFILE, (o), (n), (s) )

#define mx_cfn_construct_system_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_SYSTEM, (o), (n), (s) )

#define mx_cfn_construct_state_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_STATE, (o), (n), (s) )

#define mx_cfn_construct_scan_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_SCAN, (o), (n), (s) )

#define mx_cfn_construct_user_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_USER, (o), (n), (s) )

#define mx_cfn_construct_cwd_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_CWD, (o), (n), (s) )

#define mx_cfn_construct_absolute_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_ABSOLUTE, (o), (n), (s) )

#define mx_cfn_construct_module_filename(o,n,s) \
		mx_cfn_construct_filename( MX_CFN_MODULE, (o), (n), (s) )

#ifdef __cplusplus
}
#endif

#endif /* __MX_CFN_H__ */


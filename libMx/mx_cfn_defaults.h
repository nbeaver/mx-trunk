/*
 * Name:     mx_cfn_defaults.h
 *
 * Purpose:  Contains default values for MX control system file locations.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_CFN_DEFAULTS_H__
#define __MX_CFN_DEFAULTS_H__

#include "mx_osdef.h"

#if defined( HAVE_LSB )
#   define MX_CFN_PROGRAM_DIR		"/usr/bin"
#   define MX_CFN_CONFIG_DIR		"/etc/mx"
#   define MX_CFN_INCLUDE_DIR		"/usr/include/mx"
#   define MX_CFN_LIBRARY_DIR		"/usr/lib"
#   define MX_CFN_LOGFILE_DIR		"/var/log"
#   define MX_CFN_RUNFILE_DIR		"/var/run"
#   define MX_CFN_SYSTEM_DIR		"/usr/sbin"
#   define MX_CFN_STATE_DIR		"/var/state"
#   define MX_CFN_SCAN_DIR		"${HOME}"
#   define MX_CFN_USER_DIR		"${HOME}/.mx"
#   define MX_CFN_CWD_DIR		"."

#   define MX_CFN_DEFAULT_MXDIR		""

#else
#   define MX_CFN_PROGRAM_DIR		"${MXDIR}/bin"
#   define MX_CFN_CONFIG_DIR		"${MXDIR}/etc"
#   define MX_CFN_INCLUDE_DIR		"${MXDIR}/include"
#   define MX_CFN_LIBRARY_DIR		"${MXDIR}/lib"
#   define MX_CFN_LOGFILE_DIR		"${MXDIR}/log"
#   define MX_CFN_RUNFILE_DIR		"${MXDIR}/run"
#   define MX_CFN_SYSTEM_DIR		"${MXDIR}/sbin"
#   define MX_CFN_STATE_DIR		"${MXDIR}/state"
#   define MX_CFN_SCAN_DIR		"${HOME}"
#   define MX_CFN_USER_DIR		"${HOME}/.mx"
#   define MX_CFN_CWD_DIR		"."

#   define MX_CFN_DEFAULT_MXDIR		"/opt/mx"
#endif

#endif /* __MX_CFN_DEFAULTS_H__ */


/*
 * Name:   mx_dll_dpp_callback.h
 *
 * Purpose: Wrapper for vendor include file for the XGLab DANTE MCA.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_DLL_DPP_CALLBACK_H__
#define __MX_DLL_DPP_CALLBACK_H__

#if defined(OS_WIN32)
#include <windows.h>
#include <io.h>
#endif

/* Vendor include file. */

#if defined(OS_WIN32)
#pragma warning( push )
#pragma warning( disable : 4114 )
#endif

#include "DLL_DPP_Callback.h"

#if defined(OS_WIN32)
#pragma warning( pop )
#endif

#ifdef POLLINGLIB
#error Only the Callback mode library is supported by MX.
#endif

#endif /* __MX_DLL_DPP_CALLBACK_H__ */

/*
 * Name:     mx_namefix.h
 *
 * Purpose:  The linkers for some operating systems can only handle up to
 *           31 characters in externally visible names.  In most cases, we
 *           have defined names that are unique in the first 31 characters,
 *           but there are a few cases where doing this would result in the
 *           use of names that do not fit into the patterns set by other
 *           function names.  For these cases, we remap the name in the
 *           preprocessor to a unique name.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_NAMEFIX_H__
#define __MX_NAMEFIX_H__

#if defined(OS_VMS)

/* mx_area_detector_get_bytes_per_pixel() collides with
 * mx_area_detector_get_bytes_per_frame().
 */

#define mx_area_detector_get_bytes_per_pixel      mx_ad_get_bytes_per_pixel

#endif /* OS_VMS */

#endif /* __MX_NAMEFIX_H__ */


/*
 * Name:   i_pmac_ethernet.h
 *
 * Purpose: Definitions needed for the Delta Tau PMAC ethernet protocol.
 *          The protocol definition was found in the manual for the
 *          Delta Tau Accessory 54E module.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PMAC_ETHERNET_H__
#define __I_PMAC_ETHERNET_H__

typedef struct {
	uint8_t  RequestType;
	uint8_t  Request;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint8_t  bData[1492];
} ETHERNETCMD;

#define VR_DOWNLOAD		0x40
#define VR_UPLOAD		0xC0

#define VR_PMAC_SENDLINE	0xB0
#define VR_PMAC_GETLINE		0xB1
#define VR_PMAC_FLUSH		0xB3
#define VR_PMAC_GETMEM		0xB4
#define VR_PMAC_SETMEM		0xB5
#define VR_PMAC_SETBIT		0xBA
#define VR_PMAC_SETBITS		0xBB
#define VR_PMAC_PORT		0xBE
#define VR_PMAC_GETRESPONSE	0xBF
#define VR_PMAC_READREADY	0xC2
#define VR_CTRL_RESPONSE	0xC4
#define VR_PMAC_GETBUFFER	0xC5
#define VR_PMAC_WRITEBUFFER	0xC6
#define VR_PMAC_WRITEERROR	0xC7
#define VR_FWDOWNLOAD		0xCB
#define VR_IPADDRESS		0xE0

#define ETHERNETCMDSIZE  8

#endif /* __I_PMAC_ETHERNET_H__ */


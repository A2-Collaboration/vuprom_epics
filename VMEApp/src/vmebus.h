/*
 *
 *	Copyright (c) 1992, 1994, 2001 by
 *	Johannes Gutenberg-Universitaet Mainz,
 *	Institut fuer Kernphysik, K.W. Krygier
 *
 *	All rights reserved.
 *
 *	@(#)vmebus.h	12.1 KPH (K.W. Krygier) 94/10/29 96/08/23
 *
 */

#ifndef _VMEBUS_H_
#define _VMEBUS_H_

#include <stddef.h>
#include <sys/types.h>

#define VMESIOOFFSET	0xffff0000
#define	VMESTDOFFSET	0xff000000
#define VMEBUSPC_STDOFFSET	0x9e000000
#define VMEBUSPC_EXTOFFSET	0x80000000
#define VMEBUSPC_SIOOFFSET	0x9f000000

void	*vmebus(off_t padd, size_t size);
void	*vmestd(off_t padd, size_t size);
void	*vmeext(off_t padd, size_t size);
void	*vmesio(off_t padd, size_t size);

void    *OpenVMEbus();

#endif

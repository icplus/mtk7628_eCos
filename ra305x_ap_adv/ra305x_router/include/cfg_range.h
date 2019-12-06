/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    cfg_range.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#ifndef CFG_RANGE_H
#define CFG_RANGE_H

#define CFG_POE_MTU_MIN     546
#define CFG_POE_MTU_MAX     1492

#define CFG_POE_IDLET_MIN   60
#define CFG_POE_IDLET_MAX   3600

#define CFG_PTP_MTU_MIN     546
#define CFG_PTP_MTU_MAX     1460

#define CFG_PTP_IDLET_MIN   60
#define CFG_PTP_IDLET_MAX   3600

#define CFG_SYS_IDLET_MIN   60
#define CFG_SYS_IDLET_MAX   3600

#define CFG_WLN_BEACON_MIN  100
#define CFG_WLN_BEACON_MAX  1000

#define CFG_WLN_PKTSZ_MIN   256
#define CFG_WLN_PKTSZ_MAX   2346

#define CFG_WLN_RTS_MIN     0
#define CFG_WLN_RTS_MAX     2347

#define CFG_WLN_DTIM_MIN    1
#define CFG_WLN_DTIM_MAX    255

#endif /* CFG_RANGE_H */



//==========================================================================
//
//      include/rijndael/rijndael_local.h
//
//==========================================================================
//####BSDCOPYRIGHTBEGIN####
//
// -------------------------------------------
//
// Portions of this software may have been derived from OpenBSD, 
// FreeBSD or other sources, and are covered by the appropriate
// copyright disclaimers included herein.
//
// Portions created by Red Hat are
// Copyright (C) 2002 Red Hat, Inc. All Rights Reserved.
//
// -------------------------------------------
//
//####BSDCOPYRIGHTEND####
//==========================================================================


/*	$KAME: rijndael_local.h,v 1.3 2000/10/02 17:14:27 itojun Exp $	*/

/* the file should not be used from outside */
typedef u_int8_t		BYTE;
typedef u_int8_t		word8;	
typedef u_int16_t		word16;	
typedef u_int32_t		word32;

#define MAXKC		RIJNDAEL_MAXKC
#define MAXROUNDS	RIJNDAEL_MAXROUNDS

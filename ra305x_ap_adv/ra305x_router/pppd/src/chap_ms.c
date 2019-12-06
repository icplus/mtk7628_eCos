/*
 * chap_ms.c - Microsoft MS-CHAP compatible implementation.
 *
 * Copyright (c) 1995 Eric Rosenquist, Strata Software Limited.
 * http://www.strataware.com/
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Eric Rosenquist.  The name of the author may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * Modifications by Lauri Pesonen / lpesonen@clinet.fi, april 1997
 *
 *   Implemented LANManager type password response to MS-CHAP challenges.
 *   Now pppd provides both NT style and LANMan style blocks, and the
 *   prefered is set by option "ms-lanman". Default is to use NT.
 *   The hash text (StdText) was taken from Win95 RASAPI32.DLL.
 *
 *   You should also use DOMAIN\\USERNAME as described in README.MSCHAP80
 */

//#define MSLANMAN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/bsdtypes.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

//#include "pppd.h"
#include "chap.h"
#include "chap_ms.h"
#include <sha1.h>
#include "fsm.h"
//#include "lcp.h"
#ifdef MPPE
#include "mppe.h"
#endif
#include "extra_crypto.h"

#ifndef BZERO
#define BZERO(a,b)  bzero((char *)a, b)
#endif

#ifndef BCOPY
#define BCOPY(a, b, c)  bcopy((char *)a, b, c)
#endif

typedef struct {
    u_char LANManResp[24];
    u_char NTResp[24];
    u_char UseNT;		/* If 1, ignore the LANMan response field */
} MS_ChapResponse;
/* We use MS_CHAP_RESPONSE_LEN, rather than sizeof(MS_ChapResponse),
   in case this struct gets padded. */

typedef struct {
    u_char PeerChallenge[16];
    u_char Reserved[8];
    u_char NTResp[24];
    u_char Flags;
} MS_ChapResponse_v2;

static void	ChallengeResponse __P((u_char *, u_char *, u_char *));
static void	ChapMS_NT __P((char *, int, char *, int, MS_ChapResponse *));
#ifdef MSLANMAN
static void	ChapMS_LANMan __P((char *, int, char *, int, MS_ChapResponse *));
#endif

#ifdef MSLANMAN
bool	ms_lanman = 0;    	/* Use LanMan password instead of NT */
			  	/* Has meaning only with MS-CHAP challenges */
#endif

static void
ChallengeResponse(challenge, pwHash, response)
    u_char *challenge;	/* IN   8 octets */
    u_char *pwHash;	/* IN  16 octets */
    u_char *response;	/* OUT 24 octets */
{
    char    ZPasswordHash[21];

    BZERO(ZPasswordHash, sizeof(ZPasswordHash));
    BCOPY(pwHash, ZPasswordHash, MD4_SIGNATURE_SIZE);

#if 0
    dbglog("ChallengeResponse - ZPasswordHash %.*B",
	   sizeof(ZPasswordHash), ZPasswordHash);
#endif

    DesEncrypt(challenge, ZPasswordHash +  0, response + 0);
    DesEncrypt(challenge, ZPasswordHash +  7, response + 8);
    DesEncrypt(challenge, ZPasswordHash + 14, response + 16);

#if 0
    dbglog("ChallengeResponse - response %.24B", response);
#endif
}

static void
ChapMS_NT(rchallenge, rchallenge_len, secret, secret_len, response)
    char *rchallenge;
    int rchallenge_len;
    char *secret;
    int secret_len;
    MS_ChapResponse    *response;
{
    u_char		hash[MD4_SIGNATURE_SIZE];

    NtPasswordHash(secret, secret_len, hash);
    ChallengeResponse(rchallenge, hash, response->NTResp);
}

#ifdef MSLANMAN
static void
ChapMS_LANMan(rchallenge, rchallenge_len, secret, secret_len, response)
    char *rchallenge;
    int rchallenge_len;
    char *secret;
    int secret_len;
    MS_ChapResponse	*response;
{
    u_char		PasswordHash[MD4_SIGNATURE_SIZE];

    LmPasswordHash(secret, secret_len, PasswordHash);
    ChallengeResponse(rchallenge, PasswordHash, response->LANManResp);
}
#endif

void
ChapMS(cstate, rchallenge, rchallenge_len, secret, secret_len)
    chap_state *cstate;
    char *rchallenge;
    int rchallenge_len;
    char *secret;
    int secret_len;
{
    MS_ChapResponse	response;

#if 0
    CHAPDEBUG((LOG_INFO, "ChapMS: secret is '%.*s'", secret_len, secret));
#endif
    BZERO(&response, sizeof(response));

    /* Calculate both always */
    ChapMS_NT(rchallenge, rchallenge_len, secret, secret_len, &response);

#ifdef MSLANMAN
    ChapMS_LANMan(rchallenge, rchallenge_len, secret, secret_len, &response);

    /* prefered method is set by option  */
    response.UseNT = !ms_lanman;
#else
    response.UseNT = 1;
#endif

#ifdef MPPE
    mppe_gen_master_key(secret, secret_len, rchallenge);
#endif
    BCOPY(&response, cstate->response, MS_CHAP_RESPONSE_LEN);
    cstate->resp_length = MS_CHAP_RESPONSE_LEN;
}

int
ChapMS_Resp(cstate, secret, secret_len, remmd)
    chap_state *cstate;
    char *secret;
    int secret_len;
    u_char *remmd;
{
    MS_ChapResponse local;
    MS_ChapResponse *response = (MS_ChapResponse *)remmd;
    int i;

    BZERO(&local, sizeof(response));

    if(response->UseNT)
    {
      ChapMS_NT(cstate->challenge,cstate->chal_len, secret, secret_len, &local);
      i = memcmp(local.NTResp, response->NTResp, sizeof(local.NTResp));

#ifdef MPPE
      if(i == 0)
        mppe_gen_master_key(secret, secret_len, cstate->challenge);
#endif
      return(i);
    }

#ifdef MSLANMAN
    ChapMS_LANMan(cstate->challenge, cstate->chal_len, secret, secret_len, 
		&local);
    if(memcmp(local.LANManResp, response->LANManResp, 
	sizeof(local.LANManResp)) == 0) {
#ifdef MPPE
      mppe_gen_master_key(secret, secret_len, cstate->challenge);
#endif
      return(0);
    }
#endif /* MSLANMAN */
    return(1);
}

void
ChallengeHash(PeerChallenge, AuthenticatorChallenge, UserName, Challenge)
    char *PeerChallenge;
    char *AuthenticatorChallenge;
    char *UserName;
    char *Challenge;
{
    SHA1_CTX Context;
    u_char Digest[SHA1_DIGEST_LENGTH];
    char *username;
    
    if((username = strrchr(UserName, '\\')) != (char *)NULL)
      ++username;
    else
      username = UserName;
    SHA1Init(&Context);
    SHA1Update(&Context, PeerChallenge, 16);
    SHA1Update(&Context, AuthenticatorChallenge, 16);
    SHA1Update(&Context, username, strlen(username));
    SHA1Final(Digest, &Context);
    BCOPY(Digest, Challenge, 8);
}

void
ChapMS_v2(cstate, AuthenticatorChallenge, AuthenticatorChallengeLen, Password, PasswordLen)
    chap_state *cstate;
    char *AuthenticatorChallenge;
    int AuthenticatorChallengeLen;
    char *Password;
    int PasswordLen;
{
    u_char Challenge[8];
    u_char PasswordHash[MD4_SIGNATURE_SIZE];
    MS_ChapResponse_v2 response;
  
    BZERO(&response, sizeof(response));
    ChapGenChallenge(cstate);
    BCOPY(cstate->challenge, response.PeerChallenge, 
		sizeof(response.PeerChallenge));
    ChallengeHash(response.PeerChallenge, AuthenticatorChallenge, 
		cstate->resp_name, Challenge);
    NtPasswordHash(Password, PasswordLen, PasswordHash);
    ChallengeResponse(Challenge, PasswordHash, response.NTResp);
    BCOPY(&response, cstate->response, MS_CHAP_RESPONSE_LEN);
    cstate->resp_length = MS_CHAP_RESPONSE_LEN;
#ifdef MPPE
    mppe_gen_master_key_v2(Password, PasswordLen, response.NTResp, 0);
#endif
}

int
ChapMS_v2_Resp(cstate, Password, PasswordLen, remmd, UserName)
    chap_state *cstate;
    char *Password;
    int PasswordLen;
    u_char *remmd;
    char *UserName;
{
    u_char Challenge[8];
    u_char PasswordHash[MD4_SIGNATURE_SIZE];
    MS_ChapResponse_v2 response, response1;
    int i;
  
    BCOPY(remmd, &response, MS_CHAP_RESPONSE_LEN);
    ChallengeHash(response.PeerChallenge,cstate->challenge,UserName,Challenge);
    NtPasswordHash(Password, PasswordLen, PasswordHash);
    ChallengeResponse(Challenge, PasswordHash, response1.NTResp);
    i = memcmp(response.NTResp, response1.NTResp, sizeof(response.NTResp));
#ifdef MPPE
    if(i == 0)
      mppe_gen_master_key_v2(Password, PasswordLen, response1.NTResp, 1);
#endif
    return(i);
}

void
ChapMS_v2_Auth(cstate, Password, PasswordLen, remmd, UserName)
    chap_state *cstate;
    char *Password;
    int  PasswordLen;
    u_char *remmd;
    char *UserName;
{
    u_char PasswordHash[MD4_SIGNATURE_SIZE];
    u_char PasswordHashHash[MD4_SIGNATURE_SIZE];
    u_char Challenge[8];
    static char Magic1[] = "Magic server to client signing constant";
    static char Magic2[] = "Pad to make it do more than one iteration";

    SHA1_CTX Context;
    u_char Digest[SHA1_DIGEST_LENGTH];
    MS_ChapResponse_v2 *response = (MS_ChapResponse_v2 *)remmd;
    char   StrResponse[SHA1_DIGEST_LENGTH * 2 + 3], *s;
    int i;
    static char HexDigs[] = "0123456789ABCDEF";
    
    NtPasswordHash(Password, PasswordLen, PasswordHash);
    md4(PasswordHash, sizeof(PasswordHash), PasswordHashHash);

    SHA1Init(&Context);
    SHA1Update(&Context, PasswordHashHash, 16);
    SHA1Update(&Context, response->NTResp, 24);
    SHA1Update(&Context, Magic1, sizeof(Magic1) - 1);
    SHA1Final(Digest, &Context);

    ChallengeHash(response->PeerChallenge,cstate->challenge,UserName,Challenge);
    
    SHA1Init(&Context);
    SHA1Update(&Context, Digest, SHA1_DIGEST_LENGTH);
    SHA1Update(&Context, Challenge, 8);
    SHA1Update(&Context, Magic2, sizeof(Magic2) - 1);
    SHA1Final(Digest, &Context);
    s = strcpy(StrResponse, "S=");
    for(i = 0; i < SHA1_DIGEST_LENGTH; ++i) {
      *s++ = HexDigs[Digest[i] >> 4];
      *s++ = HexDigs[Digest[i] & 0x0F];
    }
    *s = '\0';
    BCOPY(StrResponse, cstate->response, sizeof(StrResponse));
    cstate->resp_length = sizeof(StrResponse) - 1;
}


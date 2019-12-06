/*
 * Microsoft's Get_Key() per mppe draft by mag <mag@bunuel.tii.matav.hu>
 */
#include "sha.h"
#define _SHA1_Init SHA1Init
#define _SHA1_Update SHA1Update
#define _SHA1_Final SHA1Final
   /*
    * Pads used in key derivation
    */
static unsigned char  SHAPad1[40] =
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char  SHAPad2[40] =
        {0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
         0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
         0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
         0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2};

   /*
    * SHAInit(), SHAUpdate() and SHAFinal() functions are an
    * implementation of Secure Hash Algorithm (SHA-1) [7]. These are
    * available in public domain or can be licensed from
    * RSA Data Security, Inc.
    *
    * 1) H is 8 bytes long for 40 bit session keys.
    * 2) H is 16 bytes long for 128 bit session keys.
    * 3) H' is same as H when this routine is called for the first time
    *    for the session.
    * 4) The generated key is returned in H'. This is the "current" key.
    */
void
GetNewKeyFromSHA(StartKey, SessionKey, SessionKeyLength, InterimKey)
    unsigned char *StartKey;
    unsigned char *SessionKey;
    unsigned long SessionKeyLength;
    unsigned char *InterimKey;
{
    SHA_CTX Context;
    unsigned char Digest[SHA_DIGEST_LENGTH];

    _SHA1_Init(&Context);
    _SHA1_Update(&Context, StartKey, SessionKeyLength);
    _SHA1_Update(&Context, SHAPad1, 40);
    _SHA1_Update(&Context, SessionKey, SessionKeyLength);
    _SHA1_Update(&Context, SHAPad2, 40);
    _SHA1_Final(Digest,&Context);
    memcpy(InterimKey, Digest, SessionKeyLength);
}



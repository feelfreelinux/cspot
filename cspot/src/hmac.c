/*
 * $Id$
 *
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "sha1.h"

#define HMAC_SHA1_BLOCKSIZE 64

void sha1_hmac(unsigned char *inputkey, size_t inputkeylen, 
	       unsigned char *inputmsg, size_t msglen,
	       unsigned char *dst)
{
  SHA1_CTX ctx;
  size_t keylen = inputkeylen; 
  unsigned char keydigest[20]; 
  unsigned char *key = inputkey; 
  unsigned char ipad[HMAC_SHA1_BLOCKSIZE];
  unsigned char opad[HMAC_SHA1_BLOCKSIZE];
  unsigned int i; 

  /* Shorten key if needed */ 
  if (keylen > HMAC_SHA1_BLOCKSIZE)
    { 
      SHA1Init(&ctx);
      SHA1Update(&ctx,inputkey,keylen);
      SHA1Final(keydigest,&ctx);
		 
      key = keydigest; 
      keylen = 20; 
    }

  /* Padding */ 
  memset(ipad, 0x36, HMAC_SHA1_BLOCKSIZE);
  memset(opad, 0x5c, HMAC_SHA1_BLOCKSIZE);

  /* Pad key */ 
  for(i = 0; i < keylen; i++) {
    ipad[i] ^= key[i];
    opad[i] ^= key[i];
  }

  /* First */
  SHA1Init(&ctx);
  SHA1Update(&ctx, ipad, HMAC_SHA1_BLOCKSIZE);
  SHA1Update(&ctx, inputmsg, msglen);
  SHA1Final(dst, &ctx);
  
  /* Second */
  SHA1Init(&ctx);
  SHA1Update(&ctx, opad, HMAC_SHA1_BLOCKSIZE);
  SHA1Update(&ctx, dst, 20);
  SHA1Final(dst, &ctx);
}
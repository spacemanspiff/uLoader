#ifndef __RIJNDAEL_H__
#define __RIJNDAEL_H__

#include "types.h"

void aes_set_key ( u8 *key );
void aes_decrypt ( u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len );
void aes_decrypt2 ( u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len, int end );

#endif

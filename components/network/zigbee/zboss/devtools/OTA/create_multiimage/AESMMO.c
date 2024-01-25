/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * www.dsr-zboss.com
 * www.dsr-corporation.com
 * All rights reserved.
 *
 * This is unpublished proprietary source code of DSR Corporation
 * The copyright notice does not evidence any actual or intended
 * publication of such source code.
 *
 * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
 * Corporation
 *
 * Commercial Usage
 * Licensees holding valid DSR Commercial licenses may use
 * this file in accordance with the DSR Commercial License
 * Agreement provided with the Software or, alternatively, in accordance
 * with the terms contained in a written agreement between you and
 * DSR.
 */
/* PURPOSE:
*/
#include "types.h"
#include "AESMMO.h"
#include "AES128.h"

//AES MMO construction 
uint32 AES_MMO(uint8 *M, uint32 Mlen, uint8 *h, uint8 DoBlockChaining)
{

	uint32 j;
	uint32 i, Mlenp;
	uint8 T[16];         //128-bit message block 
	uint8 S[16];         //128-bit message block 
	uint32 blocknum = 1;     //number of message blocks
	uint32 lastnum = 0;      //number of bytes in the last message block
	uint32 lastbits = 0;     //remaining bits in the last byte
	uint8 k[16] = {0x0};


	if(Mlen == 0) return 0;

	Mlenp = Mlen + 128;

    /* Block chaining */
	if (1 == DoBlockChaining)
	{
	  for(j = 0; j < 16; j++)
	    *(k + j) = *(h + j);
	}

	//Prefix-free encoding
    T[0] = (uint8)Mlen;
	T[1] = (uint8)(Mlen >> 8);
	T[2] = (uint8)(Mlen >> 16);
	T[3] = (uint8)(Mlen >> 24);
	S[0] = T[0];
	S[1] = T[1];
	S[2] = T[2];
	S[3] = T[3];
	for(j = 4; j < 16; j++)
	{
		T[j] = 0x0;
        S[j] = 0x0;
	}
	aes_block_enc(T, k);
	for(j = 0; j < 16; j++)
		k[j] = T[j] ^ S[j];

	//Process a number of 128-bit blocks
	for(i = 0; i < blocknum; i++)
	{
		for(j = 0; j < 16; j++)
		{
			T[j] = *(M + 16*i + j);
			S[j] = T[j];
		}
        aes_block_enc(T, k);
		for(j = 0; j < 16; j++)
			k[j] = T[j] ^ S[j];
	}

	//Process the remaining bytes
	for(j = 0; j < lastnum; j++)
	{
		T[j] = *(M + 16*blocknum + j);
		S[j] = T[j];
	}

	//Deal with remaining bits
	if(lastbits == 0)
	{
		T[lastnum] = 0x80;
		S[lastnum] = T[lastnum];
	}
	else
	{
		T[lastnum] = (*(M + 16*blocknum + lastnum) << (8 - lastbits)) | (0x1 << (8 - lastbits - 1));
		S[lastnum] = T[lastnum];
	}
	for(j = lastnum + 1; j < 14; j++)
	{
		T[j] = 0x0;
		S[j] = 0x0;
	}
	T[14] = (uint8)(Mlenp >> 8);
	S[14] = T[14];
	T[15] = (uint8)Mlenp;
	S[15] = T[15];
	aes_block_enc(T, k);
	for(j = 0; j < 16; j++)
		k[j] = T[j] ^ S[j];

	for(j = 0; j < 16; j++)
		*(h + j) = *(k + j);

    /*for(j = 0; j < 4; j++)
	{
		*(h + j) = ((uint32)k[4*j] << 24) | ((uint32)k[4*j+1] << 16) |  ((uint32)k[4*j+2] << 8) |  ((uint32)k[4*j+3]);
		*(h + j + 4) = 0x0;
	}*/
	
	return 1;
}
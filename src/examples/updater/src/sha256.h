//sha256.h
//SHA-256 implementation for updater
//Brad Conte <brad@bradconte.com> without any warranty
//Ported/tweaked by Bryan E. Topp <betopp@betopp.com> 2023

/*
https://github.com/B-Con/crypto-algorithms
This code is released into the public domain free of any restrictions.
The author requests acknowledgement if the code is used, but does not require it.
This code is provided free of any liability and without any quality claims by the author.
*/

/*********************************************************************
* Filename:   sha256.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*********************************************************************/

#ifndef SHA256_H
#define SHA256_H

/*************************** HEADER FILES ***************************/
#include <stdint.h>
#include <stddef.h>

/****************************** MACROS ******************************/
#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/

typedef struct sha256_s {
	uint8_t data[64];
	uint32_t datalen;
	unsigned long long bitlen;
	uint32_t state[8];
} sha256_t;

/*********************** FUNCTION DECLARATIONS **********************/
void sha256_init(sha256_t *ctx);
void sha256_update(sha256_t *ctx, const uint8_t data[], size_t len);
void sha256_final(sha256_t *ctx, uint8_t hash[]);

#endif   // SHA256_H

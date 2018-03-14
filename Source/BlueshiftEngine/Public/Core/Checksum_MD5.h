#pragma once

/*
-------------------------------------------------------------------------------

    Calculates a checksum for a block of data
    using the MD5 message-digest algorithm.

-------------------------------------------------------------------------------
*/

/* MD5 context. */
struct MD5_CTX {
    unsigned int    state[4];
    unsigned int    bits[2];
    unsigned char   in[64];
};

void BE_API MD5_Init(MD5_CTX *ctx);
void BE_API MD5_Update(MD5_CTX *context, unsigned char const *input, size_t inputLen);
void BE_API MD5_Final(MD5_CTX *context, unsigned char digest[16]);

uint32_t BE_API MD5_BlockChecksum(const void *data, int length);

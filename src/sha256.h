#ifndef FB_SHA256_H
#define FB_SHA256_H
#include <stddef.h>
#include <stdint.h>
typedef struct { uint32_t state[8]; uint64_t bytes; size_t used; uint8_t block[64]; } FbSha256;
void fb_sha256_init(FbSha256 *ctx);
void fb_sha256_update(FbSha256 *ctx,const void *data,size_t size);
void fb_sha256_final(FbSha256 *ctx,uint8_t out[32]);
#endif

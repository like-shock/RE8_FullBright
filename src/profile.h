#ifndef FB_PROFILE_H
#define FB_PROFILE_H
#include "build_identity.h"
#define FB_MAX_SITES 16
#define FB_MAX_SITE_BYTES 64
typedef struct { uint32_t rva,size; bool executable; } FbSection;
typedef bool (*FbImageRead)(void*,uint32_t,void*,size_t);
typedef struct { uint32_t size; const FbSection *sections; size_t section_count; void*context; FbImageRead read; } FbImageView;
typedef struct {
    uint32_t rva,scan_rva,scan_size;
    uint8_t pattern_size,original_size;
    uint8_t pattern[FB_MAX_SITE_BYTES],mask[FB_MAX_SITE_BYTES],original[FB_MAX_SITE_BYTES];
} FbSiteSpec;
typedef struct { FbBuildIdentity identity; const FbSiteSpec *sites; size_t site_count; bool candidate; } FbProfile;
typedef struct { uint32_t rva; uint8_t size,original[FB_MAX_SITE_BYTES]; } FbVerifiedSite;
typedef struct { const FbProfile*profile; size_t count; FbVerifiedSite sites[FB_MAX_SITES]; } FbVerifiedPlan;
bool fb_verify_profile(const FbImageView*,const FbBuildIdentity*,const FbProfile*,FbVerifiedPlan*,FbError*);
const FbProfile *fb_find_profile(const FbBuildIdentity*);
#endif

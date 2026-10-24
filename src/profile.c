#include "profile.h"
#include <stdlib.h>
#include <string.h>
const FbProfile *fb_find_profile(const FbBuildIdentity*identity){(void)identity;return NULL;}
static bool in_exec(const FbImageView*v,uint32_t rva,uint32_t n){
    if(rva>v->size||n>v->size-rva)return false;
    for(size_t i=0;i<v->section_count;i++){
        const FbSection*s=&v->sections[i];
        if(s->rva>v->size||s->size>v->size-s->rva)return false;
        if(s->executable&&rva>=s->rva&&rva-s->rva<=s->size&&n<=s->size-(rva-s->rva))return true;
    }return false;
}
bool fb_verify_profile(const FbImageView*v,const FbBuildIdentity*id,const FbProfile*p,FbVerifiedPlan*out,FbError*error){
    if(out)*out=(FbVerifiedPlan){0};
    if(!v||!id||!p||!out||!v->read||!v->sections||!v->section_count||!p->sites||!p->site_count||p->site_count>FB_MAX_SITES)return fb_error(error,1,"invalid_profile");
    if(id->machine!=p->identity.machine||id->machine!=0x8664||id->timestamp!=p->identity.timestamp||id->image_size!=p->identity.image_size||v->size!=id->image_size||memcmp(id->sha256,p->identity.sha256,32))return fb_error(error,2,"identity_mismatch");
    FbVerifiedPlan result={.profile=p};
    for(size_t i=0;i<p->site_count;i++){
        const FbSiteSpec*s=&p->sites[i];
        if(!s->pattern_size||s->pattern_size>FB_MAX_SITE_BYTES||!s->original_size||s->original_size>FB_MAX_SITE_BYTES||s->scan_size<s->pattern_size||s->scan_size>16*1024*1024||!in_exec(v,s->scan_rva,s->scan_size)||!in_exec(v,s->rva,s->original_size))return fb_error(error,3,"invalid_site_bounds");
        bool significant=false;
        for(unsigned j=0;j<s->pattern_size;j++){
            if(s->mask[j]!=0&&s->mask[j]!=255)return fb_error(error,3,"invalid_pattern_mask");
            significant|=s->mask[j]!=0;
        }
        if(!significant)return fb_error(error,3,"empty_pattern");
        for(size_t j=0;j<i;j++)if((uint64_t)s->rva+s->original_size>result.sites[j].rva&&(uint64_t)result.sites[j].rva+result.sites[j].size>s->rva)return fb_error(error,3,"overlapping_sites");
        uint8_t*scan=malloc(s->scan_size);if(!scan)return fb_error(error,4,"allocation_failed");
        if(!v->read(v->context,s->scan_rva,scan,s->scan_size)){free(scan);return fb_error(error,4,"image_read_failed");}
        size_t matches=0;uint32_t match_rva=0;
        for(uint32_t at=0;at<=s->scan_size-s->pattern_size;at++){
            bool match=true;
            for(unsigned j=0;j<s->pattern_size;j++)if(s->mask[j]&&scan[at+j]!=s->pattern[j]){match=false;break;}
            if(match){matches++;match_rva=s->scan_rva+at;if(matches>1)break;}
        }
        free(scan);
        if(matches!=1||match_rva!=s->rva)return fb_error(error,5,"pattern_not_unique_at_site");
        uint8_t current[FB_MAX_SITE_BYTES];
        if(!v->read(v->context,s->rva,current,s->original_size)||memcmp(current,s->original,s->original_size))return fb_error(error,6,"original_bytes_mismatch");
        result.sites[i].rva=s->rva;result.sites[i].size=s->original_size;memcpy(result.sites[i].original,current,s->original_size);result.count++;
    }
    *out=result;if(error)*error=(FbError){0};return true;
}

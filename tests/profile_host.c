#include "profile.h"
#include <assert.h>
#include <string.h>
static uint8_t image[64];
static bool read_image(void*ctx,uint32_t rva,void*out,size_t n){
    (void)ctx;if(rva>sizeof image||n>sizeof image-rva)return false;
    memcpy(out,image+rva,n);return true;
}
int main(void){
    FbBuildIdentity id={.machine=0x8664,.image_size=64,.timestamp=77};id.sha256[0]=1;
    FbSection sec={0,64,true};FbImageView view={64,&sec,1,NULL,read_image};
    FbSiteSpec sites[2]={{.rva=16,.scan_rva=0,.scan_size=64,.pattern_size=3,.original_size=3,
                         .pattern={0x48,0x89,0xd8},.mask={255,255,255},.original={0x48,0x89,0xd8}}};
    FbProfile p={.identity=id,.sites=sites,.site_count=1,.candidate=true};
    FbVerifiedPlan verified={0};FbError error={0};
    memcpy(image+16,sites[0].original,3);
    assert(fb_verify_profile(&view,&id,&p,&verified,&error));assert(verified.count==1);
    id.sha256[0]=2;assert(!fb_verify_profile(&view,&id,&p,&verified,&error));assert(verified.count==0);id=p.identity;
    memcpy(image+32,image+16,3);assert(!fb_verify_profile(&view,&id,&p,&verified,&error));memset(image+32,0,3);
    image[16]=0;assert(!fb_verify_profile(&view,&id,&p,&verified,&error));image[16]=0x48;
    sec.executable=false;assert(!fb_verify_profile(&view,&id,&p,&verified,&error));sec.executable=true;
    sites[0].original[0]=0x49;assert(!fb_verify_profile(&view,&id,&p,&verified,&error));sites[0].original[0]=0x48;
    sites[1]=sites[0];p.site_count=2;assert(!fb_verify_profile(&view,&id,&p,&verified,&error));p.site_count=1;
    sites[0].scan_size=65;assert(!fb_verify_profile(&view,&id,&p,&verified,&error));sites[0].scan_size=64;
    memset(sites[0].mask,0,3);assert(!fb_verify_profile(&view,&id,&p,&verified,&error));
    assert(fb_find_profile(&id)==NULL);return 0;
}

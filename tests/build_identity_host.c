#include "build_identity.h"
#include "sha256.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
static const char *mutate_path;
size_t __real_fread(void *,size_t,size_t,FILE *);
size_t __wrap_fread(void *ptr,size_t size,size_t n,FILE *fp) {
    size_t out=__real_fread(ptr,size,n,fp);
    if (mutate_path) {
        FILE *f=fopen(mutate_path,"ab");
        mutate_path=NULL;
        if (f) { fputc(42,f); fclose(f); }
    }
    return out;
}
static void print_hash(const unsigned char *sha) {
    for(int i=0;i<32;i++) printf("%02x",sha[i]);
}
int main(int argc,char **argv) {
    if(argc!=3)return 2;
    if(!strcmp(argv[1],"sha")) {
        FbSha256 ctx; unsigned char data[4096],sha[32]; size_t n;
        FILE *f=fopen(argv[2],"rb"); if(!f)return 2;
        fb_sha256_init(&ctx);
        while((n=fread(data,1,sizeof data,f)))fb_sha256_update(&ctx,data,n);
        if(ferror(f)){fclose(f);return 2;} fclose(f);fb_sha256_final(&ctx,sha);
        printf("{\"sha256\":\"");print_hash(sha);puts("\"}");return 0;
    }
    setlocale(LC_ALL,"");
    wchar_t path[4096]; if(mbstowcs(path,argv[2],4096)==(size_t)-1)return 2;
    if(!strcmp(argv[1],"mutate"))mutate_path=argv[2];
    FbBuildIdentity id={0}; FbError error={0};
    if(!fb_identify_file(path,&id,&error)) {
        printf("{\"error\":\"%s\"}\n",error.message);return 2;
    }
    printf("{\"machine\":%u,\"timestamp\":%u,\"image_size\":%u,\"sha256\":\"",id.machine,id.timestamp,id.image_size);
    print_hash(id.sha256);puts("\"}");return 0;
}

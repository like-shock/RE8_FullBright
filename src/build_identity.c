#include "build_identity.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <share.h>
#include <io.h>
typedef struct _stat64 FileStat;
#define file_stat(f,s) _fstat64(_fileno(f),s)
#else
#include <unistd.h>
typedef struct stat FileStat;
#define file_stat(f,s) fstat(fileno(f),s)
#endif
static uint16_t u16(const uint8_t*p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool range(uint64_t at,uint64_t n,uint64_t size){return at<=size&&n<=size-at;}
static bool parse(const uint8_t*d,size_t available,uint64_t filesize,FbBuildIdentity*out,FbError*error){
    if(!d||!out||available<64||d[0]!='M'||d[1]!='Z')return fb_error(error,1,"invalid_dos_header");
    size_t pe=u32(d+0x3c);
    if(pe<64||!range(pe,24,available)||memcmp(d+pe,"PE\0\0",4))return fb_error(error,1,"invalid_pe_header");
    uint16_t machine=u16(d+pe+4),sections=u16(d+pe+6),optional=u16(d+pe+20);
    if(machine!=0x8664||!sections||sections>96||optional<240||!range(pe+24,optional,available)||u16(d+pe+24)!=0x20b)return fb_error(error,1,"unsupported_pe");
    size_t table=pe+24+optional;
    uint32_t image=u32(d+pe+24+56),headers=u32(d+pe+24+60);
    if(!range(table,(uint64_t)sections*40,available)||headers<table+(size_t)sections*40||headers>filesize||headers>image)return fb_error(error,1,"invalid_sections");
    for(unsigned i=0;i<sections;i++){
        const uint8_t*s=d+table+i*40;uint32_t vs=u32(s+8),rva=u32(s+12),raw=u32(s+16),off=u32(s+20);
        if(!range(off,raw,filesize)||!range(rva,vs>raw?vs:raw,image)||(raw&&off<headers)||rva<headers)return fb_error(error,1,"section_out_of_bounds");
    }
    FbBuildIdentity result={.machine=machine,.timestamp=u32(d+pe+8),.image_size=image};
    *out=result;if(error)*error=(FbError){0};return true;
}
bool fb_parse_pe(const uint8_t*d,size_t n,FbBuildIdentity*out,FbError*error){return parse(d,n,n,out,error);}
static bool same_file(const FileStat*a,const FileStat*b){
    if(a->st_size!=b->st_size||a->st_mtime!=b->st_mtime||a->st_ctime!=b->st_ctime||a->st_dev!=b->st_dev||a->st_ino!=b->st_ino)return false;
#ifndef _WIN32
    if(a->st_mtim.tv_nsec!=b->st_mtim.tv_nsec||a->st_ctim.tv_nsec!=b->st_ctim.tv_nsec)return false;
#endif
    return true;
}
bool fb_identify_file(const wchar_t*path,FbBuildIdentity*out,FbError*error){
    if(!path||!out)return fb_error(error,1,"invalid_argument");
    FILE*f;
#ifdef _WIN32
    f=_wfsopen(path,L"rb",_SH_DENYWR);
#else
    size_t n=wcstombs(NULL,path,0);if(n==(size_t)-1)return fb_error(error,2,"invalid_path");
    char*p=malloc(n+1);if(!p)return fb_error(error,2,"allocation_failed");
    wcstombs(p,path,n+1);f=fopen(p,"rb");free(p);
#endif
    if(!f)return fb_error(error,2,"file_open_failed");
    FileStat before,after;
    if(file_stat(f,&before)||before.st_size<0){fclose(f);return fb_error(error,2,"file_stat_failed");}
    const size_t cap=1024*1024;
    uint8_t*header=malloc(cap);if(!header){fclose(f);return fb_error(error,2,"allocation_failed");}
    uint8_t buf[65536];size_t nread,kept=0;uint64_t total=0;FbSha256 sha;fb_sha256_init(&sha);
    while((nread=fread(buf,1,sizeof buf,f))){
        size_t take=cap-kept;if(take>nread)take=nread;
        if(take){memcpy(header+kept,buf,take);kept+=take;}
        fb_sha256_update(&sha,buf,nread);total+=nread;
    }
    bool io_error=ferror(f)!=0;int stat_error=file_stat(f,&after);fclose(f);
    bool ok=false;FbBuildIdentity candidate={0};
    if(io_error||stat_error)fb_error(error,2,"file_read_failed");
    else if(total!=(uint64_t)before.st_size||!same_file(&before,&after))fb_error(error,2,"file_changed");
    else if(parse(header,kept,total,&candidate,error)){
        fb_sha256_final(&sha,candidate.sha256);*out=candidate;ok=true;
    }
    free(header);return ok;
}

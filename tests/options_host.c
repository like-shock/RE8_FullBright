#include "options.h"
#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
int main(int argc,char**argv){
    assert(argc==2);setlocale(LC_ALL,"");
    char file[2048];snprintf(file,sizeof file,"%s/config.ini",argv[1]);wchar_t path[2048];mbstowcs(path,file,2048);
    FbConfig c={0};FbError e={0};assert(fb_config_load(path,&c,&e));assert(c.schema_version==1&&c.toggle_vk==0&&!c.detailed_log);
    const char*bad[]={"schema_version=2\n","schema_version=1\ntoggle_vk=999\n","schema_version=1\ndetailed_log=2\n","schema_version=1\nschema_version=1\n","schema_version=1\nenabled=1\n","toggle_vk=120\n","schema_version=1\ntoggle_vk=120junk\n"};
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){
        FILE*f=fopen(file,"wb");assert(f);fputs(bad[i],f);fclose(f);
        c.toggle_vk=120;assert(!fb_config_load(path,&c,&e));assert(c.toggle_vk==0&&!c.detailed_log);
    }
    c=(FbConfig){1,120,true};assert(fb_config_save(path,&c,&e));
    FbConfig read={0};assert(fb_config_load(path,&read,&e));assert(read.toggle_vk==120&&read.detailed_log);
    wchar_t wrong[2048];swprintf(wrong,2048,L"%ls/missing/config.ini",path);assert(!fb_config_save(wrong,&c,&e));
    assert(fb_config_load(path,&read,&e)&&read.toggle_vk==120);
    fb_options_init(&read,true);uint64_t revision;FbConfig pending;
    assert(!fb_options_take_pending(&pending,&revision));c.detailed_log=false;assert(fb_options_set(&c));
    assert(fb_options_take_pending(&pending,&revision));uint64_t old=revision;
    c.toggle_vk=121;assert(fb_options_set(&c));fb_options_save_result(old,true);
    assert(fb_options_take_pending(&pending,&revision));assert(pending.toggle_vk==121);
    fb_options_save_result(revision,false);assert(fb_options_snapshot().save_status==-1);
    FbKeyLatch key={0};fb_hotkey_arm(&key,120,false);
    assert(fb_hotkey_event(&key,120,true,false));assert(!fb_hotkey_event(&key,120,true,true));
    assert(!fb_hotkey_event(&key,120,false,false));assert(fb_hotkey_event(&key,120,true,false));
    fb_hotkey_arm(&key,120,true);assert(!fb_hotkey_event(&key,120,true,false));
    fb_hotkey_event(&key,120,false,false);assert(fb_hotkey_event(&key,120,true,false));
    fb_hotkey_arm(&key,0,false);assert(!fb_hotkey_event(&key,120,true,false));return 0;
}

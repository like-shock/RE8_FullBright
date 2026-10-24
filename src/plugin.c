#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <reframework/API.h>
#include "build_identity.h"
static const REFrameworkPluginFunctions *functions;
static DWORD WINAPI identify_worker(void*unused){
    (void)unused;
    wchar_t path[32768];DWORD length=GetModuleFileNameW(NULL,path,32768);
    if(!length||length>=32768){functions->log_error("RE8 FullBright: executable path unavailable");return 0;}
    FbBuildIdentity identity={0};FbError error={0};
    if(!fb_identify_file(path,&identity,&error)){functions->log_error("RE8 FullBright: identity failed: %s",error.message);return 0;}
    char hash[65];static const char digits[]="0123456789abcdef";
    for(unsigned i=0;i<32;i++){hash[2*i]=digits[identity.sha256[i]>>4];hash[2*i+1]=digits[identity.sha256[i]&15];}hash[64]=0;
    functions->log_info("RE8 FullBright: exe_sha256=%s image_size=%u; UNSUPPORTED_BUILD (registry empty, no hooks)",hash,identity.image_size);
    return 0;
}
__declspec(dllexport) void reframework_plugin_required_version(REFrameworkPluginVersion*v){
    if(v)*v=(REFrameworkPluginVersion){REFRAMEWORK_PLUGIN_VERSION_MAJOR,REFRAMEWORK_PLUGIN_VERSION_MINOR,REFRAMEWORK_PLUGIN_VERSION_PATCH,"RE8"};
}
__declspec(dllexport) bool reframework_plugin_initialize(const REFrameworkPluginInitializeParam*p){
    if(!p||!p->version||!p->functions||p->version->major!=REFRAMEWORK_PLUGIN_VERSION_MAJOR||p->version->minor<REFRAMEWORK_PLUGIN_VERSION_MINOR)return false;
    functions=p->functions;
    if(!functions->log_error||!functions->log_info)return false;
    HMODULE self=NULL;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,(LPCWSTR)&reframework_plugin_initialize,&self))return false;
    functions->log_info("RE8 FullBright: OFF; foundation build; no supported game profiles");
    HANDLE worker=CreateThread(NULL,0,identify_worker,NULL,0,NULL);
    if(!worker){functions->log_error("RE8 FullBright: identity worker unavailable");return false;}
    CloseHandle(worker);return true;
}

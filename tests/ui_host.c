#include "options.h"
#include "runtime.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <assert.h>
int main(void){
    lua_State*L=luaL_newstate();assert(L);luaL_openlibs(L);assert(luaL_dofile(L,"tests/ui_harness.lua")==LUA_OK);
    FbRuntime r;fb_runtime_init(&r,true);FbConfig config={1,0,false};fb_options_init(&config,true);
    assert(fb_ui_attach(L,&r));assert(fb_ui_attach(L,&r));
    assert(luaL_dostring(L,"assert(#callbacks==1); callbacks[1](); assert(draws==1)")==LUA_OK);
    assert(r.requested&&fb_runtime_status(&r)==FB_WAIT_TARGET);
    fb_ui_detach(L);fb_runtime_request(&r,false);
    assert(luaL_dostring(L,"callbacks[1](); assert(draws==1)")==LUA_OK);assert(!r.requested);
    assert(fb_ui_attach(L,&r));assert(luaL_dostring(L,"for _,f in ipairs(callbacks) do f() end; assert(draws==2)")==LUA_OK);
    assert(r.requested);
    assert(luaL_dostring(L,"select_last_key=true; callbacks[#callbacks]()")==LUA_OK);
    assert(fb_options_snapshot().config.toggle_vk==123);
    fb_ui_detach(L);lua_close(L);
    L=luaL_newstate();assert(L);assert(!fb_ui_attach(L,&r));fb_ui_detach(L);lua_close(L);return 0;
}

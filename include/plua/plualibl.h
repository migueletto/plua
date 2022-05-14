#ifndef _PLUALIB_H
#define _PLUALIB_H 1

LUALIB_API void luaopen_plua (lua_State *L) SEC("lib");
LUALIB_API void luaopen_graph (lua_State *L) SEC("lib");

LUALIB_API void luaclose_plua (lua_State *L) SEC("lib");
LUALIB_API void luaclose_graph (lua_State *L) SEC("lib");

#endif

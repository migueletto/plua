// portability types

typedef UInt32 size_t;
typedef Int32 off_t;
typedef void FILE;

#ifndef __va_list__
#define __va_list__
typedef char *va_list;
#endif

typedef double lua_Number;
typedef struct {
  void *api;
} lua_State;
typedef int (*lua_CFunction)(lua_State *L);

typedef struct luaL_reg {
  const char *name;
  lua_CFunction func;
} luaL_reg;

// pseudo-indices

#define LUA_REGISTRYINDEX       (-10000)
#define LUA_GLOBALSINDEX        (-10001)

// pre-defined references

#define LUA_NOREF       (-2)
#define LUA_REFNIL      (-1)
#define LUA_REFREGISTRY 0

// pre-defined tags

#define LUA_ANYTAG      (-1)
#define LUA_NOTAG       (-2)

// option for multiple returns in lua_call

#define LUA_MULTRET     (-1)

// minimum stack available for a C function

#define LUA_MINSTACK    20 
 
// error codes for lua_do*

#define LUA_ERRRUN      1
#define LUA_ERRFILE     2
#define LUA_ERRSYNTAX   3
#define LUA_ERRMEM      4
#define LUA_ERRERR      5

// types returned by `lua_type'

#define LUA_TNONE       (-1)
#define LUA_TUSERDATA   0
#define LUA_TNIL        1
#define LUA_TNUMBER     2   
#define LUA_TSTRING     3   
#define LUA_TTABLE      4   
#define LUA_TFUNCTION   5

// this macro may be the first definition inside each function.
#define PLUA_HEADER PluaAPI *api=L->api

// if the previous macro is used, the following macros may be used to
// to make transparent use of the Plua API.

#define plua_setlibdata(n,d) (api->plua_setlibdata(n,d))
#define plua_getlibdata(n) (api->plua_getlibdata(n))
#define plua_pusherror(L,e) (api->plua_pusherror(L,e))

#define lua_gettop(L) (api->lua_gettop(L))
#define lua_settop(L,i) (api->lua_settop(L,i))
#define lua_pushvalue(L,i) (api->lua_pushvalue(L,i))
#define lua_remove(L,i) (api->lua_remove(L,i))
#define lua_insert(L,i) (api->lua_insert(L,i))
#define lua_replace(L,i) (api->lua_replace(L,i))
#define lua_checkstack(L,i) (api->lua_checkstack(L,i))
#define lua_xmove(L,L2,n) (api->lua_xmove(L,L2,n))

#define lua_type(L,i) (api->lua_type(L,i))
#define lua_typename(L,t) (api->lua_typename(L,t))
#define lua_isnumber(L,i) (api->lua_isnumber(L,i))
#define lua_isstring(L,i) (api->lua_isstring(L,i))
#define lua_iscfunction(L,i) (api->lua_iscfunction(L,i))
#define lua_tag(L,i) (api->lua_tag(L,i))
#define lua_equal(L,i1,i2) (api->lua_equal(L,i1,i2))
#define lua_rawequal(L,i1,i2) (api->lua_rawequal(L,i1,i2))
#define lua_lessthan(L,i1,i2) (api->lua_lessthan(L,i1,i2))
#define lua_tonumber(L,i) (api->lua_tonumber(L,i))
#define lua_tostring(L,i) (api->lua_tostring(L,i))
#define lua_strlen(L,i) (api->lua_strlen(L,i))
#define lua_tocfunction(L,i) (api->lua_tocfunction(L,i))
#define lua_touserdata(L,i) (api->lua_touserdata(L,i))
#define lua_topointer(L,i) (api->lua_topointer(L,i))
#define lua_toboolean(L,i) (api->lua_toboolean(L,i))
#define lua_tothread(L,i) (api->lua_tothread(L,i))

#define lua_pushnil(L) (api->lua_pushnil(L))
#define lua_pushnumber(L,n) (api->lua_pushnumber(L,n))
#define lua_pushlstring(L,s,l) (api->lua_pushlstring(L,s,l))
#define lua_pushstring(L,s) (api->lua_pushstring(L,s))
#define lua_pushcclosure(L,f,n) (api->lua_pushcclosure(L,f,n))
#define lua_pushusertag(L,u,t) (api->lua_pushusertag(L,u,t))
#define lua_pushboolean(L,b) (api->lua_pushboolean(L,b))
#define lua_pushlightuserdata(L,p) (api->lua_pushlightuserdata(L,p))

#define lua_gettable(L,i) (api->lua_gettable(L,i))
#define lua_rawget(L,i) (api->lua_rawget(L,i))
#define lua_rawgeti(L,i,n) (api->lua_rawgeti(L,i,n))
#define lua_newtable(L) (api->lua_newtable(L))
#define lua_getmetatable(L,i) (api->lua_getmetatable(L,i))
#define lua_getfenv(L,i) (api->lua_getfenv(L,i))

#define lua_settable(L,i) (api->lua_settable(L,i))
#define lua_rawset(L,i) (api->lua_rawset(L,i))
#define lua_rawseti(L,i,n) (api->lua_rawseti(L,i,n))
#define lua_setmetatable(L,i) (api->lua_setmetatable(L,i))
#define lua_getfenv(L,i) (api->lua_getfenv(L,i))

#define lua_call(L,a,r) (api->lua_call(L,a,r))
#define lua_pcall(L,i,n,f) (api->lua_call(L,i,n,f))
#define lua_cpcall(L,f,u) (api->lua_call(L,f,u))
#define lua_load(L,r,d,c) (api->lua_load(L,r,d,c))
#define lua_dump(L,w,d) (api->lua_dump(L,w,d))
#define lua_dofile(L,f) (api->lua_dofile(L,f))
#define lua_dostring(L,s) (api->lua_dostring(L,s))
#define lua_dobuffer(L,b,s,n) (api->lua_dobuffer(L,b,s,n))

#define lua_yield(L,n) (api->lua_yield(L,n))
#define lua_resume(L,n) (api->lua_resume(L,n))

#define lua_getgcthreshold(L) (api->lua_getgcthreshold(L))
#define lua_getgccount(L) (api->lua_getgccount(L))
#define lua_setgcthreshold(L,n) (api->lua_setgcthreshold(L,n))

#define lua_version() (api->lua_version())
#define lua_error(L,s) (api->lua5_error(L,s))
#define lua_next(L,i) (api->lua_next(L,i))
#define lua_concat(L,n) (api->lua_concat(L,n))
#define lua_newuserdata(L,s) (api->lua_newuserdata(L,s))

#define luaL_openlib(L,s,a,n) (api->luaL_openlib(L,s,a,n))
#define luaL_checklstring(L,n,l) (api->luaL_checklstring(L,n,l))
#define luaL_optlstring(L,n,d,l) (api->luaL_optlstring(L,n,d,l))
#define luaL_checknumber(L,n) (api->luaL_checknumber(L,n))
#define luaL_optnumber(L,n,d) (api->luaL_optnumber(L,n,d))
#define luaL_checkstack(L,m) (api->luaL_checkstack(L,m))
#define luaL_checktype(L,n,t) (api->luaL_checktype(L,n,t))
#define luaL_checkany(L,n) (api->luaL_checkany(L,n))

#define fopen(n,m) (api->fopen(n,m))
#define fclose(f) (api->fclose(f))
#define feof(f) (api->feof(f))
#define fseek(f,o,m) (api->fseek(f,o,m))
#define ftell(f) (api->ftell(f))
#define fread(b,s,n,f) (api->fread(b,s,n,f))
#define fwrite(b,s,n,f) (api->fwrite(b,s,n,f))
#define fputs(s,f) (api->fputs(s,f))
#define fgets(s,l,f) (api->fgets(s,l,f))
#define malloc(s) (api->malloc(s))
#define calloc(n,s) (api->calloc(n,s))
#define realloc(p,s) (api->realloc(p,s))
#define free(p) (api->free(p))

#define plua_getwindow() (api->plua_getwindow())
#define plua_creategadget(L,x,y,d,h) (api->plua_creategadget(L,x,y,d,h))
#define plua_rgb(r,g,b) (api->plua_rgb(r,g,b))
#define plua_color(f,b) (api->plua_color(f,b))
#define plua_rect(x,y,dx,dy) (api->plua_rect(x,y,dx,dy))
#define plua_box(x,y,dx,dy) (api->plua_box(x,y,dx,dy))
#define plua_line(x1,y1,x2,y2) (api->plua_line(x1,y1,x2,y2))
#define plua_lineto(x,y) (api->plua_lineto(x,y))
#define plua_moveto(x,y) (api->plua_moveto(x,y))
#define plua_print(s) (api->plua_print(s))
#define plua_pos(x,y) (api->plua_pos((x),(y)))
#define plua_textsize(s,x,y) (api->plua_textsize((s),(x),(y)))
#define plua_getrgb(c,r,g,b) (api->plua_getrgb(c,r,g,b))
#define plua_getcolor(f,b) (api->plua_getcolor(f,b))

#define plua_adddriver(p,o,c,r,w,s,t,i) (api->plua_adddriver(p,o,c,r,w,s,t,i))
#define plua_removedriver(i) (plua_removedriver->plua_adddriver(i))


// some useful macros

#define lua_pop(L,n)            lua_settop(L, -(n)-1)

#define lua_pushuserdata(L,u)   lua_pushusertag(L, u, 0)
#define lua_pushcfunction(L,f)  lua_pushcclosure(L, f, 0)

#define lua_isfunction(L,n)     (lua_type(L,n) == LUA_TFUNCTION)
#define lua_istable(L,n)        (lua_type(L,n) == LUA_TTABLE)
#define lua_isuserdata(L,n)     (lua_type(L,n) == LUA_TUSERDATA)
#define lua_isnil(L,n)          (lua_type(L,n) == LUA_TNIL)
#define lua_isnull(L,n)         (lua_type(L,n) == LUA_TNONE)

#define luaL_arg_check(L, cond,numarg,extramsg) if (!(cond)) luaL_argerror(L, numarg,extramsg)
#define luaL_check_string(L,n)  (luaL_checklstring(L, (n), NULL))
#define luaL_opt_string(L,n,d)  (luaL_optlstring(L, (n), (d), NULL))
#define luaL_check_int(L,n)     ((int)luaL_checknumber(L, n))
#define luaL_check_long(L,n)    ((long)luaL_checknumber(L, n))
#define luaL_opt_int(L,n,d)     ((int)luaL_optnumber(L, n,d))
#define luaL_opt_long(L,n,d)    ((long)luaL_optnumber(L, n,d))

#define SEEK_SET fileOriginBeginning
#define SEEK_CUR fileOriginCurrent
#define SEEK_END fileOriginEnd

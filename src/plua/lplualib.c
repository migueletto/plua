#include "p.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "PluaAPI.h"
#include "PluaLib.h"
#include "lstate.h"
#include "lualib.h"
#include "plualibl.h"
#include "lgraphlib.h"
#include "screen.h"
#include "error.h"

static int plua_libload (lua_State *L) SEC("lib");
static void plua_setlibdata (char *name, void *data) SEC("lib");
static void *plua_getlibdata (char *name) SEC("lib");
static int plua_pusherror(lua_State *L, int err) SEC("lib");
static int plua_adddriver(char *prefix, PluaDriverOpen open,
   PluaDriverClose close, PluaDriverRead read, PluaDriverWrite write,
   PluaDriverSeek seek, PluaDriverTell tell) SEC("lib");
static int plua_removedriver(int driver) SEC("lib");

static PluaLib pluaLib[MAX_LIBS];
static PluaAPI pluaAPI;

static int plua_libload (lua_State *L) {
  char *libname, *init;
  UInt16 refnum, i;
  UInt32 type, creator;
  LocalID dbID;
  lua_CFunction f = NULL;
  Err err;

  libname = (char *)luaL_check_string(L, 1);
  init = (char *)luaL_check_string(L, 2);

  for (i = 0; i < MAX_LIBS; i++) {
    if (pluaLib[i].refnum != 0xffff && !StrCompare(libname, pluaLib[i].name)) {
      lua_pushnumber(L, i);
      return 1;
    }
  }

  for (i = 0; i < MAX_LIBS; i++) {
    if (pluaLib[i].refnum == 0xffff)
      break;
  }

  if (i == MAX_LIBS)
    return plua_pusherror(L, dmErrMemError);

  err = SysLibFind(libname, &refnum);

  if (err) {
    if ((dbID = DmFindDatabase(0, libname)) == 0)
      return plua_pusherror(L, dmErrCantFind);

    if ((err = DmDatabaseInfo(0, dbID, NULL, NULL, NULL, NULL, NULL, NULL,
                              NULL, NULL, NULL, &type, &creator)) != 0)
      return plua_pusherror(L, err);

    if ((err = SysLibLoad(type, creator, &refnum)) != 0)
      return plua_pusherror(L, err);
  }

  pluaLib[i].refnum = refnum;
  pluaLib[i].name = strdup(libname);
  pluaLib[i].data = NULL;

  err = PluaLibOpen(refnum, L);
  if (err == 0) {
    f = PluaLibGet(refnum, L, init);
    if (f == NULL)
      err = dmErrInvalidParam;
  }

  if (err != 0) {
    pluaLib[i].refnum = 0xffff;
    if (pluaLib[i].name)
      free(pluaLib[i].name);
    if (pluaLib[i].data)
      free(pluaLib[i].data);
    return plua_pusherror(L, err);
  }

  // no Lua e lua_pushlightuserdata com o ponteiro para a lib
  lua_pushnumber(L, pluaLib[i].refnum);
  lua_pushcclosure(L, f, 1);
  return 1;
}

LUALIB_API void luaopen_plua (lua_State *L) {
  Int16 i;

  for (i = 0; i < MAX_LIBS; i++)
    pluaLib[i].refnum = 0xffff;

  pluaAPI.version = PLUA_API_VERSION;

  // Plua extensions

  pluaAPI.plua_setlibdata = plua_setlibdata;
  pluaAPI.plua_getlibdata = plua_getlibdata;
  pluaAPI.plua_pusherror = plua_pusherror;

  // basic stack manipulation

  pluaAPI.lua_gettop = lua_gettop;
  pluaAPI.lua_settop = lua_settop;
  pluaAPI.lua_pushvalue = lua_pushvalue;
  pluaAPI.lua_remove = lua_remove;
  pluaAPI.lua_insert = lua_insert;
  pluaAPI.lua_replace = lua_replace;
  pluaAPI.lua_checkstack = lua_checkstack;
  pluaAPI.lua_xmove = lua_xmove;

  // access functions (stack -> C)

  pluaAPI.lua_type = lua_type;
  pluaAPI.lua_typename = lua_typename;
  pluaAPI.lua_isnumber = lua_isnumber;
  pluaAPI.lua_isstring = lua_isstring;
  pluaAPI.lua_iscfunction = lua_iscfunction;
  pluaAPI.lua_equal = lua_equal;
  pluaAPI.lua_lessthan = lua_lessthan;
  pluaAPI.lua_tonumber = lua_tonumber;
  pluaAPI.lua_tostring = lua_tostring;
  pluaAPI.lua_strlen = lua_strlen;
  pluaAPI.lua_tocfunction = lua_tocfunction;
  pluaAPI.lua_touserdata = lua_touserdata;
  pluaAPI.lua_topointer = lua_topointer;
  pluaAPI.lua_isuserdata = lua_isuserdata;
  pluaAPI.lua_rawequal = lua_rawequal;
  pluaAPI.lua_toboolean = lua_toboolean;
  pluaAPI.lua_tothread = lua_tothread;

  // push functions (C -> stack)

  pluaAPI.lua_pushnil = lua_pushnil;
  pluaAPI.lua_pushnumber = lua_pushnumber;
  pluaAPI.lua_pushlstring = lua_pushlstring;
  pluaAPI.lua_pushstring = lua_pushstring;
  pluaAPI.lua_pushcclosure = lua_pushcclosure;
  pluaAPI.lua_pushvfstring = lua_pushvfstring;
  pluaAPI.lua_pushfstring = lua_pushfstring;
  pluaAPI.lua_pushboolean = lua_pushboolean;
  pluaAPI.lua_pushlightuserdata = lua_pushlightuserdata;

  // get functions (Lua -> stack)

  pluaAPI.lua_gettable = lua_gettable;
  pluaAPI.lua_rawget = lua_rawget;
  pluaAPI.lua_rawgeti = lua_rawgeti;
  pluaAPI.lua_newtable = lua_newtable;
  pluaAPI.lua_getmetatable = lua_getmetatable;
  pluaAPI.lua_getfenv = lua_getfenv;

  // set functions (stack -> Lua)

  pluaAPI.lua_settable = lua_settable;
  pluaAPI.lua_rawset = lua_rawset;
  pluaAPI.lua_rawseti = lua_rawseti;
  pluaAPI.lua_setmetatable = lua_setmetatable;
  pluaAPI.lua_setfenv = lua_setfenv;

  // "do" functions (run Lua code)

  pluaAPI.lua_call = lua_call;
  pluaAPI.lua_dofile = lua_dofile;
  pluaAPI.lua_dostring = lua_dostring;
  pluaAPI.lua_dobuffer = lua_dobuffer;
  pluaAPI.lua_pcall = lua_pcall;
  pluaAPI.lua_cpcall = lua_cpcall;
  pluaAPI.lua_load = lua_load;
  pluaAPI.lua_dump = lua_dump;

  // coroutine functions

  pluaAPI.lua_yield = lua_yield;
  pluaAPI.lua_resume = lua_resume;

  // garbage-collection functions

  pluaAPI.lua_getgcthreshold = lua_getgcthreshold;
  pluaAPI.lua_getgccount = lua_getgccount;
  pluaAPI.lua_setgcthreshold = lua_setgcthreshold;

  // miscellaneous functions

  pluaAPI.lua_error = lua_error;
  pluaAPI.lua_next = lua_next;
  pluaAPI.lua_concat = lua_concat;
  pluaAPI.lua_newuserdata = lua_newuserdata;
  pluaAPI.lua_version = lua_version;

// auxiliary funtions (lauxlib)

  pluaAPI.luaL_openlib = luaL_openlib;
  pluaAPI.luaL_checkstack = luaL_checkstack;
  pluaAPI.luaL_checktype = luaL_checktype;
  pluaAPI.luaL_checkany = luaL_checkany;
  pluaAPI.luaL_checklstring = luaL_checklstring;
  pluaAPI.luaL_optlstring = luaL_optlstring;
  pluaAPI.luaL_checknumber = luaL_checknumber;
  pluaAPI.luaL_optnumber = luaL_optnumber;

  pluaAPI.fopen = fopen;
  pluaAPI.fclose = fclose;
  pluaAPI.feof = feof;
  pluaAPI.fseek = fseek;
  pluaAPI.ftell = ftell;
  pluaAPI.fread = fread;
  pluaAPI.fwrite = fwrite;
  pluaAPI.fputs = fputs;
  pluaAPI.fgets = fgets;
  pluaAPI.malloc = malloc;
  pluaAPI.calloc = calloc;
  pluaAPI.realloc = realloc;
  pluaAPI.free = free;

  pluaAPI.plua_getwindow = plua_getwindow;
  pluaAPI.plua_creategadget = plua_creategadget;
  pluaAPI.plua_color = plua_pcolor;
  pluaAPI.plua_rgb = plua_prgb;
  pluaAPI.plua_rect = plua_prect;
  pluaAPI.plua_box = plua_pbox;
  pluaAPI.plua_circle = plua_pcircle;
  pluaAPI.plua_disc = plua_pdisc;
  pluaAPI.plua_line = plua_pline;
  pluaAPI.plua_lineto = plua_plineto;
  pluaAPI.plua_moveto = plua_pmoveto;
  pluaAPI.plua_print = plua_print;
  pluaAPI.plua_pos = plua_ppos;
  pluaAPI.plua_textsize = plua_ptextsize;
  pluaAPI.plua_getrgb = plua_getrgb;
  pluaAPI.plua_getcolor = plua_getcolor;

  pluaAPI.plua_adddriver = plua_adddriver;
  pluaAPI.plua_removedriver = plua_removedriver;

  lua_register(L, "loadlib", plua_libload),

  L->api = &pluaAPI;
}

LUALIB_API void luaclose_plua (lua_State *L) {
  UInt16 i, count;

  for (i = 0; i < MAX_LIBS; i++)
    if (pluaLib[i].refnum != 0xffff) {
      PluaLibClose(pluaLib[i].refnum, L, &count);
      SysLibRemove(pluaLib[i].refnum);
      pluaLib[i].refnum = 0xffff;
      if (pluaLib[i].name)
        free(pluaLib[i].name);
      if (pluaLib[i].data)
        free(pluaLib[i].data);
    }
}

static void plua_setlibdata(char *name, void *data)
{
  UInt16 i;

  for (i = 0; i < MAX_LIBS; i++)
    if (pluaLib[i].refnum != 0xffff && !StrCompare(name, pluaLib[i].name)) {
      if (pluaLib[i].data)
        free(pluaLib[i].data);
      pluaLib[i].data = data;
      break;
    }
}

static void *plua_getlibdata(char *name)
{
  UInt16 i;

  for (i = 0; i < MAX_LIBS; i++)
    if (pluaLib[i].refnum != 0xffff && !StrCompare(name, pluaLib[i].name))
      return pluaLib[i].data;

  return NULL;
}

static int plua_pusherror(lua_State *L, int err)
{
  err = err == -1 ? errno : mapPalmOsErr(err);
  lua_pushnil(L);
  lua_pushstring(L, strerror(err));
  return 2;
}

static int plua_adddriver(char *prefix, PluaDriverOpen open,
   PluaDriverClose close, PluaDriverRead read, PluaDriverWrite write,
   PluaDriverSeek seek, PluaDriverTell tell)
{
  DRIVER *f;
  int i;

  if ((f = malloc(sizeof(DRIVER))) == NULL)
    return -1;

  StrNCopy(f->prefix, prefix, 7);
  f->open = open;
  f->close = close;
  f->read = read;
  f->write = write;
  f->seek = seek;
  f->tell = tell;

  if ((i = add_driver(f)) < 0) {
    free(f);
    return -1;
  }

  return i;
}

static int plua_removedriver(int i)
{
  return remove_driver(i);
}

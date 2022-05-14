#include <PalmOS.h>

#include "Plua.h"
#include "PluaAPI.h"

#define LIBNAME "TestLib2"

// IMPORTANT: you can not use ANY global variables, or your library will CRASH.
// Instead you need to store all your "global" variables inside a C struct and
// access them through plua_setlibdata() and plua_getlibdata().

// This struct will hold the "global" variables.

typedef struct {
  UInt32 t;
} TestData;

// This is a sample function exported by this library.
// It will be registered inside PluaLibInit() below.

int lifetime(lua_State *L)
{
  PLUA_HEADER;
  TestData *g;

  // retrieve the libray global data
  g = plua_getlibdata(LIBNAME);

  lua_pushnumber(L, TimGetSeconds() - g->t);
  return 1;
}

// This is another sample function exported by this library.
// It will be registered inside PluaLibInit() below.

int square(lua_State *L)
{
  PLUA_HEADER;
  Int16 n;

  n = luaL_check_int(L, 1);
  lua_pushnumber(L, n*n);

  return 1;
}

// Library initialization function. You may alloc your global data and
// register Lua functions.

static int TestLibInit(lua_State *L)
{
  PLUA_HEADER;
  TestData *g;
  const struct luaL_reg testLib[] = {
    {"lifetime", lifetime},
    {"square", square},
    {NULL, NULL}
  };

  // alloc the global data
  if ((g = MemPtrNew(sizeof(TestData))) == NULL)
    return -1;

  // fill the global data
  g->t = TimGetSeconds(); 

  // register the global data in Plua.
  plua_setlibdata(LIBNAME, g);

  // register C functions in Lua
  luaL_openlib(L, "test", testLib, 0);

  // return sucess
  return 0;
}

// This function is called when the library is first loaded by a Plua program.
// It must return a pointer to the library initialization function.

lua_CFunction PluaLibInit(lua_State *L, char *s)
{
  return TestLibInit;
}

// This function is called when Plua exits. You must unregister all the
// functions and free the global data.

Err PluaLibFinish(lua_State *L)
{
  PLUA_HEADER;

  // unregister C functions in Lua
  lua_pushstring(L, "test");
  lua_gettable(L, LUA_GLOBALSINDEX);
  lua_pushstring(L, "lifetime");
  lua_pushnil(L);
  lua_settable(L, -3);
  lua_pushstring(L, "square");
  lua_pushnil(L);
  lua_settable(L, -3);

  // free the global data
  plua_setlibdata(LIBNAME, NULL);

  // return sucess
  return 0;
}

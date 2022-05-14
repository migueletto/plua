#include <PalmOS.h>

#include "Plua.h"
#include "PluaAPI.h"

#define LIBNAME "ScreenLib"

/*
Zire 31, OS 5.2.8:
160 x 160 CSTN, 12-bit (supports 4,096 colors)

With Plua 2.0b7 the data below is displayed:

Test 1: 5 2 8
Test 2: 0 4 true
Test 3: 160 160
Test 4: 320 320
Test 5: 160 160 8 true
Test 6: 320 320 8 true
Test 7: 5 5 11
Test 8: 10 10 22
Test 9: 32907

         0FED CBA9 8765 4321
0x808B = 1000 0000 1000 1011 = 2,4,8 e 16 bits (o que significa o bit 1 ?)
*/

/*
Zire 22, OS 5.4.9:

Test 1: 5 4 9
Test 2: 0 5 true
Test 3: 160 160
Test 4: 320 320
Test 5: 160 160 16 true
Test 6: 320 320 16 true
Test 7: 5 5 11
Test 8: 5 10 22
Test 9: 32907
*/

int test1(lua_State *L)
{
  PLUA_HEADER;
  UInt32 romVersion;
  UInt16 romVerMajor, romVerMinor, romVerFix;

  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
  romVerMajor = sysGetROMVerMajor(romVersion);
  romVerMinor = sysGetROMVerMinor(romVersion);
  romVerFix = sysGetROMVerFix(romVersion);
  lua_pushnumber(L, romVerMajor);
  lua_pushnumber(L, romVerMinor);
  lua_pushnumber(L, romVerFix);

  return 3;
}

int test2(lua_State *L)
{
  PLUA_HEADER;
  UInt32 winVersion;
  Boolean highDensity;
  Err err;

  err = FtrGet(sysFtrCreator, sysFtrNumWinVersion, &winVersion);
  highDensity = (err == 0 && winVersion >= 4);
  lua_pushnumber(L, err);
  lua_pushnumber(L, winVersion);
  lua_pushboolean(L, highDensity);

  return 3;
}

int test3(lua_State *L)
{
  PLUA_HEADER;
  UInt16 displayWidth, displayHeight;

  WinGetDisplayExtent(&displayWidth, &displayHeight);
  lua_pushnumber(L, displayWidth);
  lua_pushnumber(L, displayHeight);

  return 2;
}

int test4(lua_State *L)
{
  PLUA_HEADER;
  UInt16 displayWidth, displayHeight;
  UInt32 winVersion;
  Boolean highDensity;
  Err err;

  err = FtrGet(sysFtrCreator, sysFtrNumWinVersion, &winVersion);
  highDensity = (err == 0 && winVersion >= 4);

  if (highDensity) WinSetCoordinateSystem(kCoordinatesDouble);
  WinGetDisplayExtent(&displayWidth, &displayHeight);
  if (highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  lua_pushnumber(L, displayWidth);
  lua_pushnumber(L, displayHeight);

  return 2;
}

int test5(lua_State *L)
{
  PLUA_HEADER;
  UInt32 screenWidth, screenHeight, screenDepth;
  Boolean screenColor;

  WinScreenMode(winScreenModeGet, &screenWidth, &screenHeight, &screenDepth, &screenColor);
  lua_pushnumber(L, screenWidth);
  lua_pushnumber(L, screenHeight);
  lua_pushnumber(L, screenDepth);
  lua_pushboolean(L, screenColor);

  return 4;
}

int test6(lua_State *L)
{
  PLUA_HEADER;
  UInt32 screenWidth, screenHeight, screenDepth;
  Boolean screenColor;
  UInt32 winVersion;
  Boolean highDensity;
  Err err;

  err = FtrGet(sysFtrCreator, sysFtrNumWinVersion, &winVersion);
  highDensity = (err == 0 && winVersion >= 4);

  if (highDensity) WinSetCoordinateSystem(kCoordinatesDouble);
  WinScreenMode(winScreenModeGet, &screenWidth, &screenHeight, &screenDepth, &screenColor);
  if (highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  lua_pushnumber(L, screenWidth);
  lua_pushnumber(L, screenHeight);
  lua_pushnumber(L, screenDepth);
  lua_pushboolean(L, screenColor);

  return 4;
}

int test7(lua_State *L)
{
  PLUA_HEADER;
  UInt16 charsWidth, charWidth, charHeight;

  charsWidth = FntCharsWidth("a", 1);
  charWidth = FntCharWidth('a');
  charHeight = FntCharHeight();
  lua_pushnumber(L, charsWidth);
  lua_pushnumber(L, charWidth);
  lua_pushnumber(L, charHeight);

  return 3;
}

int test8(lua_State *L)
{
  PLUA_HEADER;
  UInt16 charsWidth, charWidth, charHeight;
  UInt32 winVersion;
  Boolean highDensity;
  Err err;

  err = FtrGet(sysFtrCreator, sysFtrNumWinVersion, &winVersion);
  highDensity = (err == 0 && winVersion >= 4);

  if (highDensity) WinSetCoordinateSystem(kCoordinatesDouble);
  charsWidth = FntCharsWidth("a", 1);
  charWidth = FntCharWidth('a');
  charHeight = FntCharHeight();
  if (highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  lua_pushnumber(L, charsWidth);
  lua_pushnumber(L, charWidth);
  lua_pushnumber(L, charHeight);

  return 3;
}

int test9(lua_State *L)
{
  PLUA_HEADER;
  UInt32 depths;

  WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &depths, NULL);
  lua_pushnumber(L, depths);

  return 1;
}

static int ScreenLibInit(lua_State *L)
{
  PLUA_HEADER;
  struct luaL_reg *screenLib;

  screenLib = calloc(10, sizeof(luaL_reg));
  screenLib[0].name = "test1";
  screenLib[0].func = test1;
  screenLib[1].name = "test2";
  screenLib[1].func = test2;
  screenLib[2].name = "test3";
  screenLib[2].func = test3;
  screenLib[3].name = "test4";
  screenLib[3].func = test4;
  screenLib[4].name = "test5";
  screenLib[4].func = test5;
  screenLib[5].name = "test6";
  screenLib[5].func = test6;
  screenLib[6].name = "test7";
  screenLib[6].func = test7;
  screenLib[7].name = "test8";
  screenLib[7].func = test8;
  screenLib[8].name = "test9";
  screenLib[8].func = test9;
  screenLib[9].name = NULL;
  screenLib[9].func = NULL;

  luaL_openlib(L, "screen", screenLib, 0);
  free(screenLib);

  return 0;
}

lua_CFunction PluaLibInit(lua_State *L, char *s)
{
  return ScreenLibInit;
}

Err PluaLibFinish(lua_State *L)
{
  PLUA_HEADER;

  lua_pushstring(L, "screen");
  lua_gettable(L, LUA_GLOBALSINDEX);

  lua_pushstring(L, "test1");
  lua_pushnil(L);
  lua_settable(L, -3);
  lua_pushstring(L, "test2");
  lua_pushnil(L);
  lua_settable(L, -3);
  lua_pushstring(L, "test3");
  lua_pushnil(L);
  lua_settable(L, -3);
  lua_pushstring(L, "test4");
  lua_pushnil(L);
  lua_settable(L, -3);
  lua_pushstring(L, "test5");
  lua_pushnil(L);
  lua_settable(L, -3);
  lua_pushstring(L, "test6");
  lua_pushnil(L);
  lua_settable(L, -3);
  lua_pushstring(L, "test7");
  lua_pushnil(L);
  lua_settable(L, -3);
  lua_pushstring(L, "test8");
  lua_pushnil(L);
  lua_settable(L, -3);
  lua_pushstring(L, "test9");
  lua_pushnil(L);
  lua_settable(L, -3);

  return 0;
}

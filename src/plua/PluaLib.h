#ifndef _PLUA_LIB_H
#define _PLUA_LIB_H 1

typedef struct {
  UInt16 refnum;
  char *name;
  void *data;
} PluaLib;

Err PluaLibOpen(UInt16 refNum, lua_State *L) SYS_TRAP(sysLibTrapOpen);
Err PluaLibClose(UInt16 refNum, lua_State *L, UInt16 *count) SYS_TRAP(sysLibTrapClose);
Err PluaLibSleep(UInt16 refNum) SYS_TRAP(sysLibTrapSleep);
Err PluaLibWake(UInt16 refNum) SYS_TRAP(sysLibTrapWake);
lua_CFunction PluaLibGet(UInt16 refNum, lua_State *L, char *s) SYS_TRAP(sysLibTrapCustom);

lua_CFunction PluaLibInit(lua_State *L, char *s);
Err PluaLibFinish(lua_State *L);

#endif

#include "p.h"
#include "app.h"
#include "events.h"
#include "gui.h"
#include "lua.h"
#include "lapi.h"
#include "lauxlib.h"
#include "PluaAPI.h"
#include "lgraphlib.h"
#include "compat.h"
#include "handler.h"

void resethandlers(void)
{
  Int16 id;

  AppSetRegistry(SYSTEM_BASE + nilEvent, NULL);
  AppSetRegistry(SYSTEM_BASE + keyDownEvent, NULL);
  AppSetRegistry(SYSTEM_BASE + penUpEvent, NULL);
  AppSetRegistry(SYSTEM_BASE + penDownEvent, NULL);
  AppSetRegistry(SYSTEM_BASE + penMoveEvent, NULL);
  AppSetRegistry(SYSTEM_BASE + menuEvent, NULL);
  AppSetRegistry(SYSTEM_BASE + ioPendingEvent, NULL);
  AppSetRegistry(SYSTEM_BASE + sampleStopEvent, NULL);

  for (id = FirstInternID; id < LastInternID; id++) {
    AppSetRegistry(CONTROL_BASE + id - FirstInternID, NULL);
  }
}

int gui_sethandler(lua_State *L)
{
  Int16 id;
  UInt32 e;
  Closure *c;

  e = luaL_check_long(L, 1);
  c = (Closure *)lua_toclosure(L, 2);

  switch (e) {
    case nilEvent:
    case keyDownEvent:
    case penUpEvent:
    case penDownEvent:
    case penMoveEvent:
    case menuEvent:
    case ioPendingEvent:
    case sampleStopEvent:
      AppSetRegistry(SYSTEM_BASE + e, c);
      lua_pushboolean(L, 1);
      return 1;
    default:
      if ((id = findcontrol(L, 1)) != -1) {
        AppSetRegistry(CONTROL_BASE + id - FirstInternID, c);
        lua_pushboolean(L, 1);
        return 1;
      }
  }

  return 0;
}

Boolean gethandler(lua_State *L, UInt32 e)
{
  Closure *c = AppGetRegistry(e);

  if (c == NULL)
    return false;

  lua_pushclosure(L, c);
  return true;
}

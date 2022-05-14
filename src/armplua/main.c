#include <PalmOS.h>
#include <SystemMgr.h>

#include "lua.h"

int main(void)
{
  EventType event;
  Boolean PenDown;
  Int16 PenX, PenY;
  lua_State *L;

  L = lua_open();

  do {
    EvtGetEvent(&event, evtWaitForever);
    if (event.eType == penUpEvent) {
      EvtGetPen(&PenX, &PenY, &PenDown);
      WinDrawChars("Hello World", StrLen("Hello World"), PenX, PenY);
    }
    SysHandleEvent(&event);
  } while (event.eType != appStopEvent);

  return 0;
}

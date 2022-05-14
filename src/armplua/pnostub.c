#include <PalmOS.h>
#include "peal.h"

UInt32 PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags)
{
  EventType event;
  PealModule *m;
  void *address;

  if (cmd == sysAppLaunchCmdNormalLaunch) {
    m = PealLoadFromResources('armc', 1000);
    address = PealLookupSymbol(m, "PealArmStart");
    PealCall(m, address, NULL);
    PealUnload(m);
  }

  return 0;
}

#include <PalmOS.h>

#include "app.h"

#define pluaCreator	'LuaP'
#define pluaName	"Plua2"

#define runtimeCreator	'LuaR'
#define runtimeName	"Plua2RT"

static Boolean StartApp(void);
static void EventLoop(void);

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
  if (cmd == sysAppLaunchCmdNormalLaunch) {
    if (StartApp())
      EventLoop();
  }
  return 0;
}

static Boolean StartApp(void)
{
  AppPB *param;
  DmOpenRef dbRef;
  UInt32 type;
  DmSearchStateType search;
  UInt16 cardNo;
  LocalID myID, pluaID;
  char name[64];
  Err err;

  dbRef = DmNextOpenResDatabase(NULL);
  if ((err = DmOpenDatabaseInfo(dbRef, &myID, NULL, NULL, &cardNo, NULL)) != 0
      || (err = DmDatabaseInfo(cardNo, myID,
               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
               &type, NULL)) != 0) {
    ErrAlertCustom(err, NULL, "Could not find myself !\n", NULL);
    return false;
  }

  if (type != sysFileTApplication) {
    ErrAlertCustom(0, "I am not an application ?\n", NULL, NULL);
    return false;
  }

  MemSet(name, sizeof(name), 0);

  if ((((err = DmGetNextDatabaseByTypeCreator(true, &search,sysFileTApplication,
               runtimeCreator, true, &cardNo, &pluaID)) != 0) &&
       ((err = DmGetNextDatabaseByTypeCreator(true, &search,sysFileTApplication,
               pluaCreator, true, &cardNo, &pluaID)) != 0)) ||
      (err = DmDatabaseInfo(cardNo, pluaID, name,
               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
               NULL, NULL)) != 0) {
    ErrAlertCustom(err, NULL, "Plua was not found.\n", NULL);
    return false;
  }

  if (StrCompare(name, pluaName) && StrCompare(name, runtimeName)) {
    ErrAlertCustom(0, "Plua name not expected: ", NULL, name);
    return false;
  }

  if ((param = MemPtrNew(sizeof(AppPB))) == NULL) {
    ErrAlertCustom(0, "Out of memory.", NULL, NULL);
    return false;
  }
  MemPtrSetOwner(param, 0);
  param->magic = pluaCreator;
  param->version = 1;
  param->callerID = myID;

  if ((err = SysUIAppSwitch(cardNo, pluaID, sysAppLaunchCmdNormalLaunch,
               param)) != 0) {
    ErrAlertCustom(err, NULL, "Error while calling Plua.\n", NULL);
    return false;
  }

  return true;
}

static void EventLoop(void)
{
  EventType event;

  do {
    EvtGetEvent(&event, evtWaitForever);
    SysHandleEvent(&event);
  } while (event.eType != appStopEvent);
}

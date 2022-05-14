#include "p.h"
#include "main.h"
#include "events.h"
#include "app.h"
#include "sound.h"
#include "gui.h"
#include "error.h"

#define MemoDBName      "MemoDB"
#define MemoExDBName    "MemosDB-PMem"

static Err StartApplication(void *);
static void EventLoop(void);
static void StopApplication(void);
static Boolean ApplicationHandleEvent(EventPtr);
static Boolean InterceptEvent(EventPtr, Boolean *r, Boolean app);
static void RetrieveVersion(char *);

static UInt16 ROMNumber;
static UInt32 screenDepth;
static char appVersion[8];
static char romVersion[8];
static Int16 lastForm;
static char *memoDBName;

UInt32 PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags)
{
  HostTraceInit();
  switch (cmd) {
    case sysAppLaunchCmdNormalLaunch:
      if (StartApplication(cmdPBP) != 0)
        return 0;
      EventLoop();
      StopApplication();
  }
  HostTraceClose();
  return 0;
}

static Err StartApplication(void *p)
{
  UInt32 value;
  UInt16 ROMVerMajor, ROMVerMinor, ROMVerFix;

  RetrieveVersion(appVersion);
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &value);
  ROMVerMajor = sysGetROMVerMajor(value);
  ROMVerMinor = sysGetROMVerMinor(value);
  ROMVerFix = sysGetROMVerFix(value);
  ROMNumber = ROMVerMajor*10 + ROMVerMinor;

  if (ROMVerFix)
    StrPrintF(romVersion, "%d.%d.%d", ROMVerMajor, ROMVerMinor, ROMVerFix);
  else
    StrPrintF(romVersion, "%d.%d", ROMVerMajor, ROMVerMinor);

  if ((ROMNumber) < 35) {
    FrmCustomAlert(ErrorAlert, "Version", appVersion,
      "requires at least PalmOS 3.5");
    return -1;
  }

  WinScreenMode(winScreenModeGet, NULL, NULL, &screenDepth, NULL);

  if (DmFindDatabase(0, MemoExDBName))
    memoDBName = MemoExDBName;
  else
    memoDBName = MemoDBName;

  return AppInit(p);
}

static void EventLoop(void)
{
  EventType event;

  do {
    ProcessEvent(&event, evtWaitForever, false);
  } while (event.eType != appStopEvent);
}

// ProcessEvent: se retornar true o evento e passado para o programa

Boolean ProcessEvent(EventType *event, Int32 wait, Boolean app)
{
  Err err;
  Boolean pass;

  // Get the next available event.
  HostTraceOutputTL(1, "EvtGetEvent begin");
  EvtGetEvent(event, wait);
  HostTraceOutputTL(1, "EvtGetEvent end: %d", event->eType);

  if (InterceptEvent(event, &pass, app))
    return pass;

  // Give the system a chance to handle the event.
  if (SysHandleEvent(event))
    return false;

  // Give the menu bar a chance to update and handle the event.
  if (MenuHandleEvent(NULL, event, &err))
    return false; 

  // Give the application a chance to handle the event.
  if (ApplicationHandleEvent(event))
    return false;  

  if (FrmDispatchEvent(event)) {
    switch (event->eType) {
      case popSelectEvent:
      case ctlRepeatEvent:
        return true;
      default:
        return false;
    }
  }

  return true;
}

static void StopApplication(void)
{
  AppFinish();
  WinScreenMode(winScreenModeSet, NULL, NULL, &screenDepth, NULL);
}

static Boolean InterceptEvent(EventPtr event, Boolean *pass, Boolean app)
{
  Boolean intercepted = false;

  *pass = false;

  switch (event->eType) {
    case keyDownEvent:
      if (event->data.keyDown.modifiers & commandKeyMask)
        switch (event->data.keyDown.chr) {
          case vchrFind:
            event->eType = helpEvent;
            break;
          case vchrHard1:
          case vchrHard2:
          case vchrHard3:
          case vchrHard4:
          case vchrPageUp:
          case vchrPageDown:
          case vchrRockerLeft:
          case vchrRockerRight:
          case vchrRockerCenter:
            if (app)
              event->eType = fakeKeyDownEvent;
            break;
          case vchrRockerUp:
            if (app) {
              event->eType = fakeKeyDownEvent;
              event->data.keyDown.chr = vchrPageUp;
            }
            break;
          case vchrRockerDown:
            if (app) {
              event->eType = fakeKeyDownEvent;
              event->data.keyDown.chr = vchrPageDown;
            }
            break;
          case vchrNavChange:
            if (app) {
              if (NavSelectPressed(event)) {
                event->eType = fakeKeyDownEvent;
                event->data.keyDown.chr = vchrRockerCenter;
              } else if (NavDirectionPressed(event, Left)) {
                event->eType = fakeKeyDownEvent;
                event->data.keyDown.chr = vchrRockerLeft;
              } else if (NavDirectionPressed(event, Right)) {
                event->eType = fakeKeyDownEvent;
                event->data.keyDown.chr = vchrRockerRight;
              } else
                intercepted = true;
            }
        }
      break;
    case sampleStopEvent:
      SoundStop(event->screenX, true);
      intercepted = true;
      *pass = true;
  }

  return intercepted;
}

static Boolean ApplicationHandleEvent(EventPtr event)
{
  FormPtr frm;
  UInt16 formId;
  Boolean handled;

  handled = false;

  switch (event->eType) {
    case frmLoadEvent:
      formId = event->data.frmLoad.formID;
      frm = FrmInitForm(formId);
      FrmSetActiveForm(frm);
      SetEventHandler(frm, formId);
      AppFrmLoad(frm, formId);
      handled = true;
      break;
    case frmCloseEvent:
      handled = AppFrmClose(event->data.frmClose.formID);
      break;
    case menuOpenEvent:
      RestoreMenu();
      break;
    case vmStartEvent:
      AppVmStart();
      handled = true;
  }

  return handled;
}

static void RetrieveVersion(char *version)
{
  MemHandle h;
  char *s;

  if ((h = DmGetResource(verRsc, appVersionID)) != NULL) {
    if ((s = MemHandleLock(h)) != NULL) {
      StrCopy(version, s);
      MemHandleUnlock(h);
    }
    else
      StrCopy(version, "?.?");
    DmReleaseResource(h);
  } 
  else
    StrCopy(version, "?.?");
}

char *GetAppVersion(void)
{
  return appVersion;
}

char *GetRomVersion(void)
{
  return romVersion;
}

Int16 GetRomVersionNumber(void)
{
  return ROMNumber;
}

Boolean ReadOnlyCheck(Boolean writting, char *op, const char *arg)
{
  if (writting && AppReadOnly()) {
    ErrorMsg("Attempt to %s \"%s\" in read-only mode", op, arg);
    return true;
  }
  return false;
}

void SetLastForm(Int16 formId)
{
  lastForm = formId;
}

Int16 GetLastForm(void)
{
  return lastForm;
}

char *GetMemoName(void)
{
  return memoDBName;
}

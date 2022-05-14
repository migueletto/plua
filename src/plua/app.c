#include "p.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "ldo.h"
#include "plualibl.h"
#include "PluaAPI.h"
#include "PluaLib.h"
#include "compat.h"
#include "gui.h"
#include "main.h"
#include "app.h"
#include "error.h"
#include "screen.h"
#include "lgraphlib.h"
#include "sound.h"
#include "editor.h"
#include "memolist.h"
#include "doclist.h"
#include "streamlist.h"
#include "vfslist.h"
#include "help.h"
#include "text.h"
#include "events.h"
#include "luac.h"
#include "check.h"
#include "handler.h"

#define pluaCreator	'LuaP'
#define pluaLibType	'LuaL'
#define pluaCodeType	'LuaP'
#define pluaCodeID	1
#define pluaExtention	".lua"
#define pluaSrcDir	"Plua"
#define pluaHelpFile    "Plua2Help"

static AppPrefs appPrefs;
static lua_State *luaState;
static char codeBuffer[256];
static AppPB *param, fullParam;
static DmOpenRef codePrc;
static MemHandle fntHandle;
static char appName[dmDBNameLength];

static UInt8 menu_sum[] = {
  0xa2,0x7f,0xde,0xaa,0x67,0x71,0x0e,0x8e,
  0x51,0x73,0x60,0x55,0x1d,0xb0,0x4a,0x17
};

static void InitPrefs(void);
static void ResetErrorHandler(void);

Err AppInit(void *_p)
{
  AppPB *p = (AppPB *)_p;

  UInt16 cardNo;
  LocalID dbID;
  UInt32 value, appCreator;
  MemHandle codeHandle, scrHandle;
  Boolean full, notitle;
  Err err;

  if ((err = checkResource('MBAR', AppMenu, menu_sum)) != 0)
    return -1;

  InitPrefs();

  MemoInitList();
  DocInitList();
  StreamInitList();
  VfsInitList();
  SoundStopAll(false);

  param = NULL;
  codePrc = NULL;
  MemSet(appName, dmDBNameLength, 0);

  file_init();
  mem_init();

  if ((luaState = lua_open()) == NULL) {
    ErrorDialog("Internal error", 0);
    return -1;
  }

  WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &value, NULL);
  if (value & 0x8000)
    value = 16;
  else if (value & 0x0080)
    value = 8;
  else if (value & 0x0008)
    value = 4;
  else if (value & 0x0002)
    value = 2;
  else
    value = 0;
  
  if (value)
    WinScreenMode(winScreenModeSet, NULL, NULL, &value, NULL);

  luaopen_base(luaState);
  luaopen_table(luaState);
  luaopen_io(luaState);
  luaopen_string(luaState);
  luaopen_math(luaState);
  luaopen_bitwise(luaState);
  luaopen_pack(luaState);
  luaopen_plua(luaState);
  luaopen_graph(luaState);

  if ((codeHandle = DmGet1Resource(GetCodeType(), GetCodeID())) != NULL)
    DmReleaseResource(codeHandle);
  full = codeHandle != NULL;
  full = false; // por enquando esta desabilitado

  if (full) {
    SysCurAppDatabase(&cardNo, &dbID);
    param = &fullParam;
    param->magic = GetCreator();
    param->version = 1;
    param->callerID = dbID;

  } else {
    if (!p || p->magic != GetCreator()) {
      vfs_init(GetSrcDir());

      SysCurAppDatabase(&cardNo, &dbID);
      DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
         NULL, NULL, NULL, &appCreator);

      fntHandle = setfixedfont();
      FrmGotoForm(appCreator == GetCreator() ? appPrefs.initialForm : RuntimeForm);
      return 0;
    }

    if ((codePrc = DmOpenDatabase(0, p->callerID, dmModeReadOnly)) == NULL) {
      ErrorDialog("Invalid caller", DmGetLastErr());
      return -1;
    }

    if ((codeHandle = DmGet1Resource(GetCodeType(), GetCodeID())) == NULL) {
      ErrorDialog("Invalid code resource", DmGetLastErr());
      DmCloseDatabase(codePrc);
      codePrc = NULL;
      return -1;
    }

    DmDatabaseInfo(0, p->callerID, appName,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    DmReleaseResource(codeHandle);
    param = p;
  }

  StrPrintF(codeBuffer, "code:/%lu", param->callerID);

  if ((scrHandle = DmGet1Resource(scrType, scrID)) != NULL)
    DmReleaseResource(scrHandle);
  notitle = scrHandle != NULL;

  fntHandle = setfixedfont();
  FrmGotoForm(notitle ? FullGraphicForm : GraphicForm);

  return 0;
}

void AppFinish(void)
{
  SoundStopAll(true);

  if (!param) {
    switch (FrmGetActiveFormID()) {
      case HelpForm:
        if (HelpPreviousForm() != EditForm)
          break;
        // fall-trough
      case EditForm:
        EditFinish();
    }
  }
  FrmCloseAllForms();

  if (luaState) {
    luaclose_graph(luaState);
    luaclose_plua(luaState);
    lua_close(luaState);
    luaState = NULL;
  }

  if (codePrc) {
    DmCloseDatabase(codePrc);
    codePrc = NULL;
  }

  file_finish();
  param = NULL;

  MemoDestroyList();
  DocDestroyList();
  StreamDestroyList();
  VfsDestroyList();

  if (fntHandle != NULL) {
    FntSetFont(stdFont);
    MemHandleUnlock(fntHandle);
    DmReleaseResource(fntHandle);
    fntHandle = NULL;
  }

  PrefSetAppPreferences(GetCreator(), 1, 1, &appPrefs, sizeof(AppPrefs), true);
}

static void InitPrefs(void)
{
  UInt16 len;

  len = sizeof(AppPrefs);
  MemSet(&appPrefs, len, 0);

  if (PrefGetAppPreferences(GetCreator(), 1, &appPrefs, &len, true) == noPreferenceFound) {
    appPrefs.readOnly = false;
    appPrefs.clearOutput = false;
    appPrefs.time = TimGetSeconds();
    appPrefs.salt = TimGetTicks();
    appPrefs.key = 0;
    appPrefs.sig = 0;
    appPrefs.memoIndex = 0;
    appPrefs.docIndex = 0;
    appPrefs.streamIndex = 0;
    appPrefs.vfsIndex = 0;
    appPrefs.initialForm = TextForm;
    PrefSetAppPreferences(GetCreator(), 1, 1, &appPrefs, len, true);
  }
}

AppPrefs *GetPrefs(void)
{
  return &appPrefs;
}

Boolean AppReadOnly(void)
{
  return appPrefs.readOnly;
}

UInt32 GetCreator(void)
{
  return pluaCreator;
}

UInt32 GetCodeType(void)
{
  return pluaCodeType;
}

UInt16 GetCodeID(void)
{
  return pluaCodeID;
}

UInt32 GetLibType(void)
{
  return pluaLibType;
}

char *GetExtention(void)
{
  return pluaExtention;
}

char *GetSrcDir(void)
{
  return pluaSrcDir;
}

char *GetHelpFile(void)
{
  return pluaHelpFile;
}

void *GetContext(void)
{
  return (void *)luaState;
}

void SetCode(char *s)
{
  if (s) StrNCopy(codeBuffer, s, sizeof(codeBuffer)-1);
}

void SetEventHandler(FormPtr frm, Int16 formId)
{
  switch(formId) {
    case TextForm:
      appPrefs.initialForm = formId;
      // fall through
    case EditForm:
    case RuntimeForm:
    case GraphicForm:
    case FullGraphicForm:
    case DialogForm:
      FrmSetEventHandler(frm, TextFormHandleEvent);
      break;
    case MemoForm:
    case DocForm:
    case StreamForm:
    case VfsForm:
      appPrefs.initialForm = formId;
      FrmSetEventHandler(frm, FileFormHandleEvent);
      break;
    case PrefsForm:
      FrmSetEventHandler(frm, PrefsFormHandleEvent);
      break;
    case CompileForm:
      FrmSetEventHandler(frm, CompileFormHandleEvent);
      break;
    case HelpForm:
      FrmSetEventHandler(frm, HelpFormHandleEvent);
  }
}

void AppFrmLoad(FormPtr frm, Int16 formId)
{
  // por enquanto nao e usado
}

Boolean AppFrmClose(Int16 formId)
{
  FormPtr frm;
  Boolean handled = false;

  switch(formId) {
    case TextForm:
    case EditForm:
    case RuntimeForm:
    case GraphicForm:
    case FullGraphicForm:
    case MemoForm:
    case DocForm:
    case StreamForm:
    case VfsForm:

      frm = FrmGetFormPtr(formId);
      FrmEraseForm(frm);
      FrmDeleteForm(frm);
      handled = true;
  }

  return handled;
}

void RestoreMenu(void)
{
  restoremenu();
}

void AppRunBuffer(char *buf, int len)
{
  lua_dobuffer(luaState, buf, len, "text");
  ResetErrorHandler();
}

void AppVmStart(void)
{
  EventType event;
  UInt32 launcher;

  while (EvtEventAvail())
    EvtGetEvent(&event, 0);

  if (luaState && codeBuffer) {
    unclipform();
    lua_dofile(luaState, codeBuffer);
    unclipform();
  }

  SoundStopAll(true);

  if (FrmGetActiveFormID() == DialogForm)
    gui_destroy(luaState);

  while (EvtEventAvail())
    ProcessEvent(&event, 0, false);

  if (param) {
    launcher = PrefGetPreference(prefLauncherAppCreator);
    AppLaunchWithCommand(launcher, sysAppLaunchCmdNormalLaunch, NULL);
  } else {
    ResetErrorHandler();
    resethandlers();
    luaclose_graph(luaState);
    FrmGotoForm(GetLastForm());
  }
}

void AppBreak(void)
{
  ResetErrorHandler();
  luaD_throw(luaState, 0);
}

Err AppCompile(char *in, char *out)
{
  return compile(luaState, in, out);
}

void AppSetRegistry(UInt32 key, void *value)
{
  lua_settop(luaState, 0);
  lua_pushnumber(luaState, key);

  if (value == NULL)
    lua_pushnil(luaState);
  else
    lua_pushclosure(luaState, value);

  lua_rawset(luaState, LUA_REGISTRYINDEX);
  lua_settop(luaState, 0);
}

void *AppGetRegistry(UInt32 key)
{
  Closure *c;

  lua_settop(luaState, 0);
  lua_pushnumber(luaState, key);
  lua_rawget(luaState, LUA_REGISTRYINDEX);

  if (lua_isnil(luaState, -1)) {
    lua_settop(luaState, 0);
    return NULL;
  }

  c = (Closure *)lua_toclosure(luaState, -1);
  lua_settop(luaState, 0);

  return c;
}

char *AppName(void)
{
  return appName;
}

static void ResetErrorHandler(void)
{
  lua_pushnil(luaState);
  lua_setglobal(luaState, "_ALERT");
}

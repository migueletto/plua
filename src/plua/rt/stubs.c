#include "p.h"
#include "lua.h"
#include "help.h"
#include "compile.h"
#include "memolist.h"
#include "doclist.h"
#include "streamlist.h"
#include "vfslist.h"

Boolean FileFormHandleEvent(EventPtr event)
{
  return false;
}

Boolean PrefsFormHandleEvent(EventPtr event)
{
  return false;
}

Boolean CompileFormHandleEvent(EventPtr event)
{
  return false;
}

Boolean HelpFormHandleEvent(EventPtr event)
{
  return false;
}

Int16 MemoGetIndex(char *name)
{
  return 0;
}

UInt16 GetLastChoice(void)
{
  return 0;
}

Int16 EditorSelected(void)
{
  return 0;
}

Boolean EditorAvailable(void)
{
  return false;
}

void SetSrcEdit(Boolean edit)
{
}

Boolean GetSrcEdit(void)
{
  return false;
}

void SetSrcPos(UInt32 pos)
{
}

UInt32 GetSrcPos(void)
{
  return 0;
}

void SetSrcLine(UInt16 line)
{
}

UInt16 GetSrcLine(void)
{
  return 0;
}

char *GetSrcBuf(void)
{
  return NULL;
}

char *GetObjBuf(void)
{
  return NULL;
}

char *GetExeBuf(void)
{
  return NULL;
}

Err ShowHelp(char *word)
{
  return 0;
}

UInt16 HelpPreviousForm(void)
{
  return 0;
}

int compile(lua_State *L, char *in, char *out)
{
  return 0;
}

void MemoInitList(void) {}
void MemoDestroyList(void) {}
void DocInitList(void) {}
void DocDestroyList(void) {}
void StreamInitList(void) {}
void StreamDestroyList(void) {}
void VfsInitList(void) {}
void VfsDestroyList(void) {}

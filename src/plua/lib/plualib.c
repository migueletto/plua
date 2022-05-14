#include <PalmOS.h>

#include "Plua.h"
#include "PluaAPI.h"
#include "../PluaLib.h"

typedef struct {
  UInt16 count;
} PluaLibData;

Err start(UInt16 refnum, SysLibTblEntryType *entry)
{
  extern void *jmptable();
  entry->dispatchTblP = (void *)jmptable;
  entry->globalsP = NULL;
  return 0;
}

Err PluaLibOpen(UInt16 refnum, lua_State *L)
{
  SysLibTblEntryType *entry;
  PluaLibData *p = NULL;
  PluaAPI *api = L->api;

  if (api->version != PLUA_API_VERSION)
    return dmErrInvalidParam;

  entry = SysLibTblEntry(refnum);
  p = (PluaLibData *)entry->globalsP;
	
  if (p) {
    p->count++;
  } else {
    if ((p = MemPtrNew(sizeof(PluaLibData))) == NULL)
      return dmErrMemError;

    entry->globalsP = (void *)p;
    p->count = 1;

    MemPtrSetOwner(p, 0);
  }
	
  return 0;
}

Err PluaLibClose(UInt16 refnum, lua_State *L, UInt16 *count)
{
  SysLibTblEntryType *entry;
  PluaLibData *p;

  entry = SysLibTblEntry(refnum);
  p = (PluaLibData *)entry->globalsP;

  if (!p)
    return dmErrMemError;

  *count = p->count--;
	
  if (!(*count)) {
    PluaLibFinish(L);
    MemPtrFree(p);
    entry->globalsP = NULL;
  }
	
  return 0;
}

Err PluaLibSleep(UInt16 refnum)
{
  return 0;
}

Err PluaLibWake(UInt16 refnum)
{
  return 0;
}

lua_CFunction PluaLibGet(UInt16 refnum, lua_State *L, char *s)
{
  SysLibTblEntryType *entry;
  PluaLibData *p;
	
  entry = SysLibTblEntry(refnum);
  p = (PluaLibData *)entry->globalsP;
	
  if (!p)
    return NULL;

  return PluaLibInit(L, s);
}

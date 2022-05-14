#include "p.h"
#include "lua.h"
#include "lauxlib.h"
#include "compat.h"
#include "ltable.h"
#include "main.h"
#include "file.h"
#include "bmp.h"
#include "hr.h"
#include "PceNativeCall.h"
#include "rsrc.h"

int resource_list(lua_State *L)
{
  FileType file;
  char *s, *name;
  Int16 i;
  Table *table;
  TObject *value;
  UInt32 type;

  s = (char *)luaL_check_string(L, 1);
  type = StringToCreator(s);
  name = (char *)luaL_check_string(L, 2);

  if (CreateResourceList(name, type, &file) == -1)
    return 0;

  table = luaH_new(L, file.n, 0);

  for (i = 0; i < file.n; i++) {
    value = luaH_setnum(L, table, i+1);
    ttype(value) = LUA_TNUMBER;
    setnvalue(value, file.rec[i].index);
  }
  DestroyFileList(&file);

  lua_pushtable(L, table);
  return 1;
}

int resource_get(lua_State *L)
{
  ResourceType *rsrc;
  UInt32 start, end;

  if ((rsrc = resource_valid(L)) == NULL)
    return 0;

  start = luaL_opt_int(L, 2, 1);
  end = luaL_opt_int(L, 3, rsrc->size);

  if (start > rsrc->size || start > end)
    return 0;

  if (end > rsrc->size)
    end = rsrc->size;

  lua_pushlstring(L, ((char *)rsrc->p)+start-1, end-start+1);
  return 1;
}

int resource_size(lua_State *L)
{
  ResourceType *rsrc;
  BitmapPtr bmp;
  UInt16 width, height, rowBytes;
  FILE f;

  if ((rsrc = resource_valid(L)) == NULL)
    return 0;

  switch (rsrc->type) {
    case bitmapRsc:
    case iconType:
    case 'abmp':
      bmp = (BitmapPtr)rsrc->p;
      hrBmpGlueGetDimensions(bmp, &width, &height, &rowBytes);
      lua_pushnumber(L, rsrc->size);
      lua_pushnumber(L, width);
      lua_pushnumber(L, height);
      return 3;

    case 'Wbmp':
      f.type = FILE_RESOURCE;
      f.recordPos = 0;
      f.recordSize = rsrc->size;
      f.record = rsrc->p;

      if (BmpGetSize(&f, &width, &height) == 0) {
        lua_pushnumber(L, rsrc->size);
        lua_pushnumber(L, width);
        lua_pushnumber(L, height);
        return 3;
      }
      // fall-through
  }

  lua_pushnumber(L, rsrc->size);
  return 1;
}

int resource_md5(lua_State *L)
{
  ResourceType *rsrc;
  UInt8 digest[16];

  if ((rsrc = resource_valid(L)) == NULL)
    return 0;

 if (EncDigestMD5(rsrc->p, rsrc->size, digest) != 0)
   return 0;

  lua_pushlstring(L, (char *)digest, 16);
  return 1;
}

int resource_call(lua_State *L)
{
  ResourceType *rsrc;
  Int16 i;
  UInt32 r, processor;
  lua_Number n;
  UInt8 *be, le[8];
  char *s;
  void *arg;

  if (GetRomVersionNumber() < 50)
    return 0;

  FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processor);
  if (!sysFtrNumProcessorIsARM(processor))
    return 0;

  if ((rsrc = resource_valid(L)) == NULL)
    return 0;

  if (lua_isnumber(L, 2)) {
    n = luaL_checknumber(L, 2);
    be = (UInt8 *)(&n);
    for (i = 0; i < sizeof(n); i++)
      le[8-i-1] = be[i];
    arg = le;
  } else if (lua_isstring(L, 2)) {
    s = (char *)luaL_check_string(L, 2);
    arg = s;
  } else
    arg = NULL;

  r = PceNativeCall(rsrc->p, arg);

  lua_pushnumber(L, r);
  return 1;
}

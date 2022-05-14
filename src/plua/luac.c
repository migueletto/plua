#include "p.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"
#include "lundump.h"
#include "lauxlib.h"
#include "lzio.h"
#include "lmem.h"
#include "ldo.h"
#include "luac.h"

static char readerBuffer[BUFSIZ];

static void strip(lua_State *L, Proto* tf);

static const char *reader(lua_State *L, void *u, size_t *size)
{
  *size = fread((void *)readerBuffer, 1, BUFSIZ, (FILE*)u);
  return *size ? readerBuffer : NULL;
}

static int writer(lua_State* L, const void *p, size_t size, void *u)
{
  return fwrite((void *)p, 1, size, (FILE*)u) == size;
}

int compile(lua_State *L, char *in, char *out)
{
  ZIO z;
  FILE *f;
  Closure *cl;
  char source[64];
  int status;

  if ((f = fopen(in, "r")) == NULL)
    return -1;

  sprintf(source, "@%s", in);
  luaZ_init (&z, reader, f, source);

  if ((status = luaD_protectedparser(L, &z, 0)) != 0) {
    fclose(f);
    callalert(L, status);
    return -1;
  } 
  fclose(f);

  cl = (Closure *)lua_toclosure(L, -1);
  strip(L, cl->l.p);

  if ((f = fopen(out, "wb")) == NULL)
    return -1;

  lua_lock(L);
  luaU_dump(L, cl->l.p, writer, f);
  lua_unlock(L);
  fclose(f);

  return 0;
}

static void strip(lua_State* L, Proto* f)
{
  int i, n = f->sizep;

  luaM_freearray(L, f->lineinfo, f->sizelineinfo, int);
  luaM_freearray(L, f->locvars, f->sizelocvars, struct LocVar);
  luaM_freearray(L, f->upvalues, f->sizeupvalues, TString *);
  f->lineinfo = NULL;
  f->sizelineinfo = 0;
  f->locvars = NULL;
  f->sizelocvars = 0;
  f->upvalues = NULL;
  f->sizeupvalues = 0;
  f->source = luaS_newliteral(L, "=(none)");

  for (i = 0; i < n; i++)
    strip(L, f->p[i]);
}

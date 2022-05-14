#include <PalmOS.h>

#include "Plua.h"
#include "PluaAPI.h"

#define LIBNAME "CasLib"

#define CAS_WAVELOW     0xF0
#define CAS_WAVEHIGH    0x10
#define CAS_WAVENULL    0x80

#define CAS_NULL  0
#define CAS_SYNC  1
#define CAS_TYPE  2
#define CAS_LEN   3
#define CAS_BLOCK 4
#define CAS_SUM   5
#define CAS_END   6

#define CAS_SAMPLES_HEADER 3000
#define BUFFERSIZE 16384

typedef struct {
  SndStreamRef streamRef;
  UInt32 state, lastblocktype, blocklen;
  UInt32 rcount, rptr, wcount, wptr;
  UInt8 *rbuf, wbuf[BUFFERSIZE];
} CasData;

static Int16 getcasbyte(CasData *g, UInt8 *b)
{
  if (g->rptr == g->rcount)
    return 0;

  *b = g->rbuf[g->rptr++];

  return 1;
}

static Int16 hascasbyte(CasData *g)
{
  return g->rptr < g->rcount;
}

static void putwavebyte(CasData *g, UInt8 b)
{
  g->wbuf[g->wcount++] = b;
  if (g->wcount == BUFFERSIZE)
    g->wcount = 0;
}

static void putcasbyte(CasData *g, UInt8 b)
{
  Int16 i;

  for (i = 0; i < 8; i++) {
    putwavebyte(g, CAS_WAVEHIGH);
    if (((b >> i) & 0x01) == 0) {
      putwavebyte(g, CAS_WAVEHIGH);
      putwavebyte(g, CAS_WAVELOW);
    }
    putwavebyte(g, CAS_WAVELOW);
  }
}

static UInt8 getwavebyte(CasData *g)
{
  UInt8 b = g->wbuf[g->wptr++];
  if (g->wptr == BUFFERSIZE)
    g->wptr = 0;
  return b;
}

static Int16 getwave(CasData *g, UInt8 *b)
{
  Int16 i;
  UInt8 c;

  if (g->wptr == g->wcount && hascasbyte(g)) {
    switch (g->state) {
      case CAS_NULL:
        for (i = 0; i < CAS_SAMPLES_HEADER; i++)
          putwavebyte(g, CAS_WAVENULL);
        for (i = 0; i < 128; i++)
          putcasbyte(g, 0x55);
        g->state = CAS_SYNC;
        break;

      case CAS_SYNC:
        for (;;) {
          if (getcasbyte(g, &c) == 0)
            break;
          if (c == 0x3C) {
            putcasbyte(g, 0x55);
            putcasbyte(g, 0x3C);
            g->state = CAS_TYPE;
            break;
          }
        }
        break;

      case CAS_TYPE:
        if (getcasbyte(g, &c) == 0)
          break;
        g->lastblocktype = c;
        putcasbyte(g, c);
        g->state = CAS_LEN;
        break;

      case CAS_LEN:
        if (getcasbyte(g, &c) == 0)
          break;
        g->blocklen = c;
        putcasbyte(g, c);
        g->state = CAS_BLOCK;
        break;

      case CAS_BLOCK:
        if (g->blocklen > 0) {
          if (getcasbyte(g, &c) == 0)
            break;
          putcasbyte(g, c);
          g->blocklen--;
        } else
          g->state = CAS_SUM;
        break;

      case CAS_SUM:
        if (getcasbyte(g, &c) == 0)
          break;
        putcasbyte(g, c);
        putcasbyte(g, 0x55);
        g->state = CAS_END;
        break;

      case CAS_END:
        if (getcasbyte(g, &c) == 0)
          break;
        g->state = (g->lastblocktype == 0x00 || g->lastblocktype == 0xFF) ? CAS_NULL : CAS_SYNC;
    }
  }

  if (g->wptr != g->wcount) {
    *b = getwavebyte(g);
    return 1;
  }
  return 0;
}

static Err SoundCallback(void *data, SndStreamRef stream, void *buffer, UInt32 *bufferSize)
{ 
  UInt32 n;
  UInt8 *buf;
  CasData *g;

  g = (CasData *)data;
  buf = (UInt8 *)buffer;

  for (n = 0; n < *bufferSize; n++) {
    if (getwave(g, buf+n) != 1)
      break;
  }

  *bufferSize = n;
  return 0;
}

static int play(lua_State *L)
{
  PLUA_HEADER;
  CasData *g;
  char *cas;
  FILE *f;

  cas = (char *)luaL_check_string(L, 1);

  if ((f = fopen(cas, "r")) == NULL) {
    lua_pushnumber(L, -1);
    return 1;
  }

  g = plua_getlibdata(LIBNAME);

  fseek(f, 0, SEEK_END);
  g->rcount = ftell(f);
  fseek(f, 0, SEEK_SET);

  g->rptr = 0;

  if (g->streamRef) {
    SndStreamDelete(g->streamRef);
    g->streamRef = NULL;
  }

  if (g->rbuf != NULL) {
    free(g->rbuf);
    g->rbuf = NULL;
  }

  if ((g->rbuf = malloc(g->rcount)) == NULL) {
    fclose(f);
    lua_pushnumber(L, -2);
    return 1;
  }
  if (fread(g->rbuf, 1, g->rcount, f) != g->rcount) {
    fclose(f);
    lua_pushnumber(L, -3);
    return 1;
  }
  fclose(f);

  if (SndStreamCreateExtended(&(g->streamRef), sndOutput, sndFormatPCM,
         4800, sndUInt8, sndMono, SoundCallback, g, 0, false) != 0) {
    free(g->rbuf);
    lua_pushnumber(L, -4);
    return 1;
  }

  SndStreamSetVolume(g->streamRef, sndGameVolume);
  g->lastblocktype = 0;
  g->state = CAS_NULL;

  g->wcount = 0;
  g->wptr = 0;

  if (SndStreamStart(g->streamRef) != 0) {
    SndStreamDelete(g->streamRef);
    free(g->wbuf);
    free(g->rbuf);
    lua_pushnumber(L, -5);
    return 1;
  }

  lua_pushnumber(L, 0);
  return 1;
}

static int CasLibInit(lua_State *L)
{
  PLUA_HEADER;
  CasData *g;

  if ((g = malloc(sizeof(CasData))) == NULL)
    return -1;

  plua_setlibdata(LIBNAME, g);

  lua_pushstring(L, "casplay");
  lua_pushcclosure(L, play, 0);
  lua_settable(L, LUA_GLOBALSINDEX);

  return 0;
}

lua_CFunction PluaLibInit(lua_State *L, char *s)
{
  return CasLibInit;
}

Err PluaLibFinish(lua_State *L)
{
  PLUA_HEADER;
  CasData *g;

  g = plua_getlibdata(LIBNAME);

  if (g->streamRef)
    SndStreamDelete(g->streamRef);

  if (g->rbuf != NULL)
    free(g->rbuf);

  plua_setlibdata(LIBNAME, NULL);

  lua_pushstring(L, "casplay");
  lua_pushnil(L);
  lua_settable(L, LUA_GLOBALSINDEX);

  return 0;
}

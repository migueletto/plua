#ifndef _PLUA_API_H
#define _PLUA_API_H 1

#include "gadget.h"

#define PLUA_API_VERSION	0x0500

// This is the Lua/Plua API available to libraries.
// The only way for a library to access Plua internals is by using
// these functions.

typedef int (*PluaDriverOpen)(int id, char *path, int *handle);
typedef int (*PluaDriverClose)(int handle);
typedef int (*PluaDriverRead)(int handle, void *buf, size_t len, size_t *n);
typedef int (*PluaDriverWrite)(int handle, void *buf, size_t len, size_t *n);
typedef int (*PluaDriverSeek)(int handle, off_t offset, int whence);
typedef int (*PluaDriverTell)(int handle, size_t *offset);
typedef int (*PluaDriverStatus)(int handle);

typedef struct {
  unsigned short version;

  // Plua extensions

  void (*plua_setlibdata)(char *name, void *data);
  void *(*plua_getlibdata)(char *name);
  int (*plua_pusherror)(lua_State *L, int err);

  // basic stack manipulation

  int (*lua_gettop)(lua_State *L);
  void (*lua_settop)(lua_State *L, int index);
  void (*lua_pushvalue)(lua_State *L, int index);
  void (*lua_remove)(lua_State *L, int index);
  void (*lua_insert)(lua_State *L, int index);
  void (*lua_replace)(lua_State *L, int idx);
  int (*lua_checkstack)(lua_State *L, int sz);
  void (*lua_xmove)(lua_State *from, lua_State *to, int n);

  // access functions (stack -> C)

  int (*lua_type)(lua_State *L, int index);
  const char *(*lua_typename)(lua_State *L, int t);
  int (*lua_isnumber)(lua_State *L, int index);
  int (*lua_isstring)(lua_State *L, int index);
  int (*lua_iscfunction)(lua_State *L, int index);
  int (*lua_isuserdata)(lua_State *L, int idx);
  int (*lua_equal)(lua_State *L, int index1, int index2);
  int (*lua_rawequal)(lua_State *L, int idx1, int idx2);
  int (*lua_lessthan)(lua_State *L, int index1, int index2);
  lua_Number (*lua_tonumber)(lua_State *L, int index);
  const char *(*lua_tostring)(lua_State *L, int index);
  size_t (*lua_strlen)(lua_State *L, int index);
  lua_CFunction (*lua_tocfunction)(lua_State *L, int index);
  void *(*lua_touserdata)(lua_State *L, int index);
  const void *(*lua_topointer)(lua_State *L, int index);
  int (*lua_toboolean)(lua_State *L, int idx);
  lua_State *(*lua_tothread)(lua_State *L, int idx);

  // push functions (C -> stack)

  void (*lua_pushnil)(lua_State *L);
  void (*lua_pushnumber)(lua_State *L, lua_Number n);
  void (*lua_pushlstring)(lua_State *L, const char *s, size_t len);
  void (*lua_pushstring)(lua_State *L, const char *s);
  void (*lua_pushcclosure)(lua_State *L, lua_CFunction fn, int n);
  void (*lua_pushusertag)(lua_State *L, void *u, int tag);
  const char *(*lua_pushvfstring)(lua_State *L, const char *fmt, va_list argp);
  const char *(*lua_pushfstring)(lua_State *L, const char *fmt, ...);
  void (*lua_pushboolean)(lua_State *L, int b);
  void (*lua_pushlightuserdata)(lua_State *L, void *p);

  // get functions (Lua -> stack)

  void (*lua_gettable)(lua_State *L, int index);
  void (*lua_rawget)(lua_State *L, int index);
  void (*lua_rawgeti)(lua_State *L, int index, int n);
  void (*lua_newtable)(lua_State *L);
  int  (*lua_getmetatable)(lua_State *L, int objindex);
  void (*lua_getfenv)(lua_State *L, int idx);

  // set functions (stack -> Lua)

  void (*lua_settable)(lua_State *L, int index);
  void (*lua_rawset)(lua_State *L, int index);
  void (*lua_rawseti)(lua_State *L, int index, int n);
  int (*lua_setmetatable)(lua_State *L, int objindex);
  int (*lua_setfenv)(lua_State *L, int idx);

  // "do" functions (run Lua code)

  void (*lua_call)(lua_State *L, int nargs, int nresults);
  int  (*lua_pcall)(lua_State *L, int nargs, int nresults, int errfunc);
  int  (*lua_cpcall)(lua_State *L, lua_CFunction func, void *ud);
  int  (*lua_load)(lua_State *L, void *reader, void *dt, const char *chunkname);
  int  (*lua_dump)(lua_State *L, void *writer, void *data);
  int  (*lua_dofile)(lua_State *L, const char *filename);
  int  (*lua_dostring)(lua_State *L, const char *str);
  int  (*lua_dobuffer)(lua_State *L, const char *buff, size_t size, const char *name);

  // coroutine functions

  int (*lua_yield)(lua_State *L, int nresults);
  int (*lua_resume)(lua_State *L, int narg);

  // garbage-collection functions

  int (*lua_getgcthreshold)(lua_State *L);
  int (*lua_getgccount)(lua_State *L); 
  void (*lua_setgcthreshold)(lua_State *L, int newthreshold);

  // miscellaneous functions

  const char *(*lua_version)(void);
  int (*lua_error)(lua_State *L);
  int  (*lua_next)(lua_State *L, int index);
  void (*lua_concat)(lua_State *L, int n);
  void *(*lua_newuserdata)(lua_State *L, size_t size);

  // auxiliary funtions (lauxlib)

  void (*luaL_openlib)(lua_State *L, const char *libname, const luaL_reg *l, int nup);
  const char *(*luaL_checklstring)(lua_State *L, int numArg, size_t *len);
  const char *(*luaL_optlstring)(lua_State *L, int numArg, const char *def, size_t *len);
  lua_Number (*luaL_checknumber)(lua_State *L, int numArg);
  lua_Number (*luaL_optnumber)(lua_State *L, int numArg, lua_Number def);
  void (*luaL_checkstack)(lua_State *L, int space, const char *msg);
  void (*luaL_checktype)(lua_State *L, int narg, int t);
  void (*luaL_checkany)(lua_State *L, int narg);

  // libc funtions

  FILE *(*fopen)(const char *, const char *);
  int (*fclose)(FILE *);
  int (*feof)(FILE *);
  int (*fseek)(FILE *, long, int);
  long (*ftell)(FILE *);
  size_t (*fread)(void *, size_t, size_t, FILE *);
  size_t (*fwrite)(void *, size_t, size_t, FILE *);
  int (*fputs)(const char *, FILE *);
  char *(*fgets)(char *, int, FILE *);
  void *(*malloc)(size_t size);
  void *(*calloc)(size_t number, size_t size);
  void *(*realloc)(void *ptr, size_t size);
  void (*free)(void *ptr);

  // Plua internals

  void *(*plua_getwindow)(void);
  int (*plua_creategadget)(void *context, int width, int height, void *data, GadgetCallback callback);
  void (*plua_color)(long fg, long bg);
  long (*plua_rgb)(int r, int g, int b);
  void (*plua_rect)(int x, int y, int dx, int dy);
  void (*plua_box)(int x, int y, int dx, int dy);
  void (*plua_circle)(int x, int y, int rx, int ry);
  void (*plua_disc)(int x, int y, int rx, int ry);
  void (*plua_line)(int x1, int y1, int x2, int y2);
  void (*plua_lineto)(int x, int y);
  void (*plua_moveto)(int x, int y);
  void (*plua_print)(char *s);
  void (*plua_pos)(int *x, int *y);
  void (*plua_textsize)(char *s, int *w, int *h);
  void (*plua_getrgb)(long c, int *r, int *g, int *b);
  void (*plua_getcolor)(long *fg, long *bg);

  // Plua I/O driver functions

  int (*plua_adddriver)(char *prefix,
                        PluaDriverOpen open, PluaDriverClose close,
                        PluaDriverRead read, PluaDriverWrite write,
                        PluaDriverSeek seek, PluaDriverTell tell,
                        PluaDriverStatus status);
  int (*plua_removedriver)(int i);

} PluaAPI;

#endif

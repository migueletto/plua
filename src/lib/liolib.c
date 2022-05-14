/*
** $Id: liolib.c,v 2.39b 2003/03/19 21:16:12 roberto Exp $
** Standard I/O (and system) library
** See Copyright Notice in lua.h
*/

#ifdef PALMOS
#include  "p.h"
#include  "compat.h"
#include  "file.h"
#endif

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define liolib_c

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "ltable.h"

#ifdef PALMOS
#include "lstring.h"
#endif


typedef struct FileHandle {
  FILE *f;
  int ispipe;
} FileHandle;


/*
** by default, gcc does not get `tmpname'
*/
#ifndef USE_TMPNAME
#ifdef __GNUC__
#define USE_TMPNAME	0
#else
#define USE_TMPNAME	1
#endif
#endif


/*
** by default, posix systems get `popen'
*/
#ifndef USE_POPEN
#ifdef _POSIX_C_SOURCE
#if _POSIX_C_SOURCE >= 2
#define USE_POPEN	1
#endif
#endif
#endif

#ifndef USE_POPEN
#define USE_POPEN	0
#endif




/*
** {======================================================
** FILE Operations
** =======================================================
*/


#if !USE_POPEN
#define pclose(f)    (-1)
#endif


#define FILEHANDLE		"FILE*"

#define IO_INPUT		"_input"
#define IO_OUTPUT		"_output"


static int pushresult (lua_State *L, int i, const char *filename) {
  if (i) {
    lua_pushboolean(L, 1);
    return 1;
  }
  else {
    lua_pushnil(L);
    if (filename)
      lua_pushfstring(L, "%s: %s", filename, strerror(errno));
    else
      lua_pushfstring(L, "%s", strerror(errno));
    lua_pushnumber(L, errno);
    return 3;
  }
}


static FileHandle *topfile (lua_State *L, int findex) {
  FileHandle *fh = (FileHandle *)luaL_checkudata(L, findex, FILEHANDLE);
  if (fh == NULL) luaL_argerror(L, findex, "bad file");
  return fh;
}


static int io_type (lua_State *L) {
  FileHandle *fh = (FileHandle *)luaL_checkudata(L, 1, FILEHANDLE);
  if (fh == NULL) lua_pushnil(L);
  else if (fh->f == NULL)
    lua_pushliteral(L, "closed file");
  else
    lua_pushliteral(L, "file");
  return 1;
}


#define tofile(L,i)   (tofileh(L,i)->f)

static FileHandle *tofileh (lua_State *L, int findex) {
  FileHandle *fh = topfile(L, findex);
  if (fh->f == NULL)
    luaL_error(L, "attempt to use a closed file");
  return fh;
}


#define newfile(L)    (&(newfileh(L)->f))


/*
** When creating file handles, always creates a `closed' file handle
** before opening the actual file; so, if there is a memory error, the
** file is not left opened.
*/
static FileHandle *newfileh (lua_State *L) {
  FileHandle *fh = (FileHandle *)lua_newuserdata(L, sizeof(FileHandle));
  fh->f = NULL;  /* file handle is currently `closed' */
  fh->ispipe = 0;
  luaL_getmetatable(L, FILEHANDLE);
  lua_setmetatable(L, -2);
  return fh;
}


/*
** assumes that top of the stack is the `io' library, and next is
** the `io' metatable
*/
static void registerfile (lua_State *L, FILE *f, const char *name,
                                                 const char *impname) {
  lua_pushstring(L, name);
  *newfile(L) = f;
  if (impname) {
    lua_pushstring(L, impname);
    lua_pushvalue(L, -2);
    lua_settable(L, -6);  /* metatable[impname] = file */
  }
  lua_settable(L, -3);  /* io[name] = file */
}


static int aux_close (lua_State *L) {
  FileHandle *fh = tofileh(L, 1);
  FILE *f = fh->f;
  if (f == stdin || f == stdout || f == stderr)
    return 0;  /* file cannot be closed */
  else {
    int ok = fh->ispipe ? (pclose(f) != -1) : (fclose(f) == 0);
    fh->f = NULL;  /* mark file as closed */
    return ok;
  }
}


static int io_close (lua_State *L) {
  if (lua_isnone(L, 1) && lua_type(L, lua_upvalueindex(1)) == LUA_TTABLE) {
    lua_pushstring(L, IO_OUTPUT);
    lua_rawget(L, lua_upvalueindex(1));
  }
  return pushresult(L, aux_close(L), NULL);
}


static int io_gc (lua_State *L) {
  FileHandle *fh = topfile(L, 1);
  if (fh->f != NULL)  /* ignore closed files */
    aux_close(L);
  return 0;
}


static int io_tostring (lua_State *L) {
  char buff[128];
  FileHandle *fh = topfile(L, 1);
  if (fh->f == NULL)
    strcpy(buff, "closed");
  else
    sprintf(buff, "%p", lua_touserdata(L, 1));
  lua_pushfstring(L, "file (%s)", buff);
  return 1;
}


static int io_open (lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  const char *mode = luaL_optstring(L, 2, "r");
  FILE **pf = newfile(L);
  *pf = fopen(filename, mode);
#ifdef PALMOS
  if (*pf) {
    if ((*pf)->type != FILE_PDB)
      return 1;
    lua_pushnumber(L, dbnumrecs(*pf));
    return 2;
  }
  return pushresult(L, 0, filename);
#else
  return (*pf == NULL) ? pushresult(L, 0, filename) : 1;
#endif
}


#ifndef PALMOS
static int io_popen (lua_State *L) {
#if !USE_POPEN
  luaL_error(L, "`popen' not supported");
  return 0;
#else
  const char *filename = luaL_checkstring(L, 1);
  const char *mode = luaL_optstring(L, 2, "r");
  FileHandle *fh = newfileh(L);
  fh->f = popen(filename, mode);
  fh->ispipe = 1;
  return (fh->f == NULL) ? pushresult(L, 0, filename) : 1;
#endif
}
#endif


static int io_tmpfile (lua_State *L) {
  FILE **pf = newfile(L);
  *pf = tmpfile();
  return (*pf == NULL) ? pushresult(L, 0, NULL) : 1;
}


static FILE *getiofile (lua_State *L, const char *name) {
  lua_pushstring(L, name);
  lua_rawget(L, lua_upvalueindex(1));
  return tofile(L, -1);
}


static int g_iofile (lua_State *L, const char *name, const char *mode) {
  if (!lua_isnoneornil(L, 1)) {
    const char *filename = lua_tostring(L, 1);
    lua_pushstring(L, name);
    if (filename) {
      FILE **pf = newfile(L);
      *pf = fopen(filename, mode);
      if (*pf == NULL) {
        lua_pushfstring(L, "%s: %s", filename, strerror(errno));
        luaL_argerror(L, 1, lua_tostring(L, -1));
      }
    }
    else {
      (void)tofile(L, 1);  /* check that it's a valid file handle */
      lua_pushvalue(L, 1);
    }
    lua_rawset(L, lua_upvalueindex(1));
  }
  /* return current value */
  lua_pushstring(L, name);
  lua_rawget(L, lua_upvalueindex(1));
  return 1;
}


static int io_input (lua_State *L) {
  return g_iofile(L, IO_INPUT, "r");
}


static int io_output (lua_State *L) {
  return g_iofile(L, IO_OUTPUT, "w");
}


static int io_readline (lua_State *L);


static void aux_lines (lua_State *L, int idx, int close) {
  lua_pushliteral(L, FILEHANDLE);
  lua_rawget(L, LUA_REGISTRYINDEX);
  lua_pushvalue(L, idx);
  lua_pushboolean(L, close);  /* close/not close file when finished */
  lua_pushcclosure(L, io_readline, 3);
}


static int f_lines (lua_State *L) {
  (void)tofile(L, 1);  /* check that it's a valid file handle */
  aux_lines(L, 1, 0);
  return 1;
}


static int io_lines (lua_State *L) {
  if (lua_isnoneornil(L, 1)) {  /* no arguments? */
    lua_pushstring(L, IO_INPUT);
    lua_rawget(L, lua_upvalueindex(1));  /* will iterate over default input */
    return f_lines(L);
  }
  else {
    const char *filename = luaL_checkstring(L, 1);
    FILE **pf = newfile(L);
    *pf = fopen(filename, "r");
    luaL_argcheck(L, *pf, 1,  strerror(errno));
    aux_lines(L, lua_gettop(L), 1);
    return 1;
  }
}


/*
** {======================================================
** READ
** =======================================================
*/


static int read_number (lua_State *L, FILE *f) {
#ifdef PALMOS
  return 0;
#else
  lua_Number d;
  if (fscanf(f, LUA_NUMBER_SCAN, &d) == 1) {
    lua_pushnumber(L, d);
    return 1;
  }
  else return 0;  /* read fails */
#endif
}


static int test_eof (lua_State *L, FILE *f) {
  int c = getc(f);
  ungetc(c, f);
  lua_pushlstring(L, NULL, 0);
  return (c != EOF);
}


static int read_line (lua_State *L, FILE *f) {
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  for (;;) {
    size_t l;
    char *p = luaL_prepbuffer(&b);
    if (fgets(p, LUAL_BUFFERSIZE, f) == NULL) {  /* eof? */
      luaL_pushresult(&b);  /* close buffer */
      return (lua_strlen(L, -1) > 0);  /* check whether read something */
    }
    l = strlen(p);
    if (p[l-1] != '\n')
      luaL_addsize(&b, l);
    else {
      luaL_addsize(&b, l - 1);  /* do not include `eol' */
      luaL_pushresult(&b);  /* close buffer */
      return 1;  /* read at least an `eol' */
    }
  }
}


static int read_chars (lua_State *L, FILE *f, size_t n) {
  size_t rlen;  /* how much to read */
  size_t nr;  /* number of chars actually read */
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  rlen = LUAL_BUFFERSIZE;  /* try to read that much each time */
  do {
    char *p = luaL_prepbuffer(&b);
    if (rlen > n) rlen = n;  /* cannot read more than asked */
    nr = fread(p, sizeof(char), rlen, f);
    luaL_addsize(&b, nr);
    n -= nr;  /* still have to read `n' chars */
  } while (n > 0 && nr == rlen);  /* until end of count or eof */
  luaL_pushresult(&b);  /* close buffer */
  return (n == 0 || lua_strlen(L, -1) > 0);
}


static int g_read (lua_State *L, FILE *f, int first) {
  int nargs = lua_gettop(L) - 1;
  int success;
  int n;
  if (nargs == 0) {  /* no arguments? */
    success = read_line(L, f);
    n = first+1;  /* to return 1 result */
  }
  else {  /* ensure stack space for all results and for auxlib's buffer */
    luaL_checkstack(L, nargs+LUA_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (lua_type(L, n) == LUA_TNUMBER) {
        size_t l = (size_t)lua_tonumber(L, n);
        success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
      }
      else {
        const char *p = lua_tostring(L, n);
        luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
        switch (p[1]) {
          case 'n':  /* number */
            success = read_number(L, f);
            break;
          case 'l':  /* line */
            success = read_line(L, f);
            break;
          case 'a':  /* file */
            read_chars(L, f, ~((size_t)0));  /* read MAX_SIZE_T chars */
            success = 1; /* always success */
            break;
          case 'w':  /* word */
            return luaL_error(L, "obsolete option `*w' to `read'");
          default:
            return luaL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (!success) {
    lua_pop(L, 1);  /* remove last result */
    lua_pushnil(L);  /* push nil instead */
  }
  return n - first;
}


static int io_read (lua_State *L) {
  return g_read(L, getiofile(L, IO_INPUT), 1);
}


static int f_read (lua_State *L) {
  return g_read(L, tofile(L, 1), 2);
}

#ifdef PALMOS
static int f_status (lua_State *L) {
  lua_pushboolean(L, iostatus(tofile(L, 1)));
  return 1;
}
#endif

static int io_readline (lua_State *L) {
  FILE *f = *(FILE **)lua_touserdata(L, lua_upvalueindex(2));
  if (f == NULL)  /* file is already closed? */
    luaL_error(L, "file is already closed");
  if (read_line(L, f)) return 1;
  else {  /* EOF */
    if (lua_toboolean(L, lua_upvalueindex(3))) {  /* generator created file? */
      lua_settop(L, 0);
      lua_pushvalue(L, lua_upvalueindex(2));
      aux_close(L);  /* close it */
    }
    return 0;
  }
}

#ifdef PALMOS

static int io_listdb (lua_State *L) {
  FileType file;
  char *s, *suffix;
  Int16 i;
  UInt16 attr;
  Table *table;
  TObject *value;
  UInt32 creator = 0, type = 0;

  s = (char *)luaL_check_string(L, 1);
  creator = StringToCreator(s);
  s = (char *)luaL_check_string(L, 2);
  type = StringToCreator(s);
  suffix = (char *)luaL_opt_string(L, 3, NULL);
  attr = luaL_opt_int(L, 4, 0); // 0=any, 1=resource, 2=stream

  if (CreateFileList(creator, type, &file, suffix, attr) == -1 || file.n == 0)
    return 0;

  table = luaH_new(L, file.n, 0);

  for (i = 0; i < file.n; i++) {
    value = luaH_setnum(L, table, i+1);
    ttype(value) = LUA_TSTRING;
    s = file.fname[i];
    setsvalue(value, luaS_newlstr(L, s, strlen(s)));
  }
  DestroyFileList(&file);

  lua_pushtable(L, table);
  return 1;
}

static int io_getdbcat (lua_State *L) {
  FILE *f = tofile(L, 1);
  Int16 index = luaL_check_int(L, 2);
  char cat[dmCategoryLength];
  if (dbgetcat(f, index, cat) == 0) {
    lua_pushstring(L, cat);
    return 1;
  }
  return pushresult(L, 0, NULL);
}

static int io_setdbcat (lua_State *L) {
  FILE *f = tofile(L, 1);
  Int16 index = luaL_check_int(L, 2);
  char *cat = (char *)luaL_check_string(L, 3);
  if (dbsetcat(f, index, cat) == 0) {
    lua_pushstring(L, cat);
    return 1;
  }
  return pushresult(L, 0, NULL);
}

static int io_openrec (lua_State *L) {
  FILE *f = tofile(L, 1);
  Int32 index = luaL_check_int(L, 2);
  Int32 r = index < 0 ? dbopeninfo(f) : dbopenrec(f, index);
  if (r != -1) {
    lua_pushnumber(L, r);
    return 1;
  }
  return pushresult(L, 0, NULL);
}

static int io_closerec (lua_State *L) {
  FILE *f = tofile(L, 1);
  int r = dbcloserec(f);
  return pushresult(L, r == 0, NULL);
}

static int io_createrec (lua_State *L) {
  FILE *f = tofile(L, 1);
  UInt16 size = luaL_check_int(L, 2);
  Int32 index = dbcreaterec(f, size);
  if (index != -1) {
    lua_pushnumber(L, index);
    return 1;
  }
  return pushresult(L, 0, NULL);
}

static int io_deleterec (lua_State *L) {
  FILE *f = tofile(L, 1);
  Int32 index = luaL_check_int(L, 2);
  int r = dbdeleterec(f, index);
  return pushresult(L, r == 0, NULL);
}

static int io_removerec (lua_State *L) {
  FILE *f = tofile(L, 1);
  Int32 index = luaL_check_int(L, 2);
  int r = dbremoverec(f, index);
  return pushresult(L, r == 0, NULL);
}

static int io_resizerec (lua_State *L) {
  FILE *f = tofile(L, 1);
  Int32 index = luaL_check_int(L, 2);
  UInt16 size = luaL_check_int(L, 3);
  int r = dbresizerec(f, index, size);
  return pushresult(L, r == 0, NULL);
}

static int io_getreccat (lua_State *L) {
  FILE *f = tofile(L, 1);
  Int32 index = luaL_check_int(L, 2);
  Int16 cat = dbgetreccat(f, index);
  if (cat != -1) {
    lua_pushnumber(L, cat);
    return 1;
  }
  return pushresult(L, 0, NULL);
}

static int io_setreccat (lua_State *L) {
  FILE *f = tofile(L, 1);
  Int32 index = luaL_check_int(L, 2);
  Int16 cat = luaL_check_int(L, 3);
  int r = dbsetreccat(f, index, cat);
  if (r != -1) {
    lua_pushnumber(L, cat);
    return 1;
  }
  return pushresult(L, 0, NULL);
}

static int io_getrecid (lua_State *L) {
  FILE *f = tofile(L, 1);
  Int32 index = luaL_check_int(L, 2);
  long id = dbgetrecid(f, index);
  if (id != -1) {
    lua_pushnumber(L, (UInt32)id);
    return 1;
  }
  return pushresult(L, 0, NULL);
}

static int io_listdir (lua_State *L) {
  FileType file;
  char *path, *suffix, *s;
  Int16 i;
  Table *table;
  TObject *value;

  path = (char *)luaL_check_string(L, 1);
  suffix = (char *)luaL_opt_string(L, 2, NULL);

  if (CreateVfsFileList(path, &file, suffix, true) == -1)
    return 0;

  table = luaH_new(L, file.n, 0);

  for (i = 0; i < file.n; i++) {
    value = luaH_setnum(L, table, i+1);
    ttype(value) = LUA_TSTRING;
    s = file.fname[i];
    setsvalue(value, luaS_newlstr(L, s, strlen(s)));
  }
  DestroyFileList(&file);

  lua_pushtable(L, table);
  return 1;
}

static int io_readdir (lua_State *L) {
  FILE *f = tofile(L, 1);
  struct dirent *dirent;

  if (f == stdin || f == stdout || f == stderr)
    return 0;

  if ((dirent = readdir((DIR *)f)) == NULL)
    return pushresult(L, 0, NULL);

  lua_pushstring(L, dirent->d_name);
  lua_pushnumber(L, dirent->d_type);

  return 2;
}

static int io_mkdir (lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  int r = mkdir(filename, 0);
  return pushresult(L, r == 0, filename);
}

#endif

/* }====================================================== */


static int g_write (lua_State *L, FILE *f, int arg) {
  int nargs = lua_gettop(L) - 1;
  int status = 1;
  for (; nargs--; arg++) {
#ifndef PALMOS
    if (lua_type(L, arg) == LUA_TNUMBER) {
      /* optimization: could be done exactly as for strings */
      status = status &&
          fprintf(f, LUA_NUMBER_FMT, lua_tonumber(L, arg)) > 0;
    } else
#endif
    {
      size_t l;
      const char *s = luaL_checklstring(L, arg, &l);
      status = status && (fwrite((void *)s, sizeof(char), l, f) == l);
    }
  }
  return pushresult(L, status, NULL);
}


static int io_write (lua_State *L) {
  return g_write(L, getiofile(L, IO_OUTPUT), 1);
}


static int f_write (lua_State *L) {
  return g_write(L, tofile(L, 1), 2);
}


static int f_seek (lua_State *L) {
  static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
  static const char *const modenames[] = {"set", "cur", "end", NULL};
  FILE *f = tofile(L, 1);
  int op = luaL_findstring(luaL_optstring(L, 2, "cur"), modenames);
  long offset = luaL_optlong(L, 3, 0);
  luaL_argcheck(L, op != -1, 2, "invalid mode");
  op = fseek(f, offset, mode[op]);
  if (op)
    return pushresult(L, 0, NULL);  /* error */
  else {
    lua_pushnumber(L, ftell(f));
    return 1;
  }
}


static int io_flush (lua_State *L) {
  return pushresult(L, fflush(getiofile(L, IO_OUTPUT)) == 0, NULL);
}


static int f_flush (lua_State *L) {
  return pushresult(L, fflush(tofile(L, 1)) == 0, NULL);
}


static const luaL_reg iolib[] = {
  {"input", io_input},
  {"output", io_output},
  {"lines", io_lines},
  {"close", io_close},
  {"flush", io_flush},
  {"open", io_open},
#ifndef PALMOS
  {"popen", io_popen},
#endif
  {"read", io_read},
  {"tmpfile", io_tmpfile},
  {"type", io_type},
  {"write", io_write},
  {NULL, NULL}
};


static const luaL_reg flib[] = {
  {"flush", f_flush},
  {"read", f_read},
  {"lines", f_lines},
  {"seek", f_seek},
  {"write", f_write},
  {"close", io_close},
  {"__gc", io_gc},
  {"__tostring", io_tostring},

#ifdef PALMOS
  {"status", f_status},

  {"getdbcat", io_getdbcat},
  {"setdbcat", io_setdbcat},

  {"openrec", io_openrec},
  {"closerec", io_closerec},
  {"createrec", io_createrec},
  {"deleterec", io_deleterec},
  {"removerec", io_removerec},
  {"resizerec", io_resizerec},
  {"getreccat", io_getreccat},
  {"setreccat", io_setreccat},
  {"getrecid", io_getrecid},

  {"readdir", io_readdir},
#endif

  {NULL, NULL}
};


static void createmeta (lua_State *L) {
  luaL_newmetatable(L, FILEHANDLE);  /* create new metatable for file handles */
  /* file methods */
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -2);  /* push metatable */
  lua_rawset(L, -3);  /* metatable.__index = metatable */
  luaL_openlib(L, NULL, flib, 0);
}

/* }====================================================== */


/*
** {======================================================
** Other O.S. Operations
** =======================================================
*/

#ifndef PALMOS
static int io_execute (lua_State *L) {
  lua_pushnumber(L, system(luaL_checkstring(L, 1)));
  return 1;
}
#endif


static int io_remove (lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  return pushresult(L, remove(filename) == 0, filename);
}


static int io_rename (lua_State *L) {
  const char *fromname = luaL_checkstring(L, 1);
  const char *toname = luaL_checkstring(L, 2);
  return pushresult(L, rename(fromname, toname) == 0, fromname);
}


static int io_tmpname (lua_State *L) {
#if !USE_TMPNAME
  luaL_error(L, "`tmpname' not supported");
  return 0;
#else
  char buff[L_tmpnam];
  if (tmpnam(buff) != buff)
    return luaL_error(L, "unable to generate a unique filename in `tmpname'");
  lua_pushstring(L, buff);
  return 1;
#endif
}


static int io_getenv (lua_State *L) {
  lua_pushstring(L, getenv(luaL_checkstring(L, 1)));  /* if NULL push nil */
  return 1;
}


static int io_clock (lua_State *L) {
  lua_pushnumber(L, ((lua_Number)clock())/(lua_Number)CLOCKS_PER_SEC);
  return 1;
}


/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

static void setfield (lua_State *L, const char *key, int value) {
  lua_pushstring(L, key);
  lua_pushnumber(L, value);
  lua_rawset(L, -3);
}

static void setboolfield (lua_State *L, const char *key, int value) {
  lua_pushstring(L, key);
  lua_pushboolean(L, value);
  lua_rawset(L, -3);
}

static int getboolfield (lua_State *L, const char *key) {
  int res;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  res = lua_toboolean(L, -1);
  lua_pop(L, 1);
  return res;
}


static int getfield (lua_State *L, const char *key, int d) {
  int res;
  lua_pushstring(L, key);
  lua_gettable(L, -2);
  if (lua_isnumber(L, -1))
    res = (int)(lua_tonumber(L, -1));
  else {
    if (d == -2)
      return luaL_error(L, "field `%s' missing in date table", key);
    res = d;
  }
  lua_pop(L, 1);
  return res;
}


static int io_date (lua_State *L) {
  const char *s = luaL_optstring(L, 1, "%c");
#ifdef PALMOS
  time_t t = (time_t)(luaL_optnumber(L, 2, 0xFFFFFFFF));
#else
  time_t t = (time_t)(luaL_optnumber(L, 2, -1));
#endif
  struct tm *stm;
  if (t == (time_t)(-1))  /* no time given? */
    t = time(NULL);  /* use current time */
  if (*s == '!') {  /* UTC? */
    stm = gmtime(&t);
    s++;  /* skip `!' */
  }
  else
    stm = localtime(&t);
  if (stm == NULL)  /* invalid date? */
    lua_pushnil(L);
  else if (strcmp(s, "*t") == 0) {
    lua_newtable(L);
    setfield(L, "sec", stm->tm_sec);
    setfield(L, "min", stm->tm_min);
    setfield(L, "hour", stm->tm_hour);
    setfield(L, "day", stm->tm_mday);
    setfield(L, "month", stm->tm_mon+1);
    setfield(L, "year", stm->tm_year+1900);
    setfield(L, "wday", stm->tm_wday+1);
    setfield(L, "yday", stm->tm_yday+1);
    setboolfield(L, "isdst", stm->tm_isdst);
  }
  else {
    char b[256];
    if (strftime(b, sizeof(b), s, stm))
      lua_pushstring(L, b);
    else
      return luaL_error(L, "`date' format too long");
  }
  return 1;
}


static int io_time (lua_State *L) {
  if (lua_isnoneornil(L, 1))  /* called without args? */
    lua_pushnumber(L, time(NULL));  /* return current time */
  else {
    time_t t;
    struct tm ts;
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);  /* make sure table is at the top */
    ts.tm_sec = getfield(L, "sec", 0);
    ts.tm_min = getfield(L, "min", 0);
    ts.tm_hour = getfield(L, "hour", 12);
    ts.tm_mday = getfield(L, "day", -2);
    ts.tm_mon = getfield(L, "month", -2) - 1;
    ts.tm_year = getfield(L, "year", -2) - 1900;
    ts.tm_isdst = getboolfield(L, "isdst");
    t = mktime(&ts);
    if (t == (time_t)(-1))
      lua_pushnil(L);
    else
      lua_pushnumber(L, t);
  }
  return 1;
}


#ifndef PALMOS
static int io_difftime (lua_State *L) {
  lua_pushnumber(L, difftime((time_t)(luaL_checknumber(L, 1)),
                             (time_t)(luaL_optnumber(L, 2, 0))));
  return 1;
}
#endif

/* }====================================================== */


#ifndef PALMOS
static int io_setloc (lua_State *L) {
  static const int cat[] = {LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY,
                      LC_NUMERIC, LC_TIME};
  static const char *const catnames[] = {"all", "collate", "ctype", "monetary",
     "numeric", "time", NULL};
  const char *l = lua_tostring(L, 1);
  int op = luaL_findstring(luaL_optstring(L, 2, "all"), catnames);
  luaL_argcheck(L, l || lua_isnoneornil(L, 1), 1, "string expected");
  luaL_argcheck(L, op != -1, 2, "invalid option");
  lua_pushstring(L, setlocale(cat[op], l));
  return 1;
}
#endif


static int io_exit (lua_State *L) {
  exit(luaL_optint(L, 1, EXIT_SUCCESS));
  return 0;  /* to avoid warnings */
}

#ifdef PALMOS
static int io_sleep (lua_State *L) {
  lua_Number ms, tps;
  UInt32 t;

  ms = luaL_check_number(L, 1) * 1000.0;
  ms += luaL_opt_number(L, 2, 0.0);
  if (ms < 0.0)
    ms = 0.0;

  tps = (lua_Number)SysTicksPerSecond();
  t = (UInt32)((ms * tps) / 1000.0);

  SysTaskDelay(t);

  return 0;
}

static int io_mem (lua_State *L) {
  UInt32 total = mem_size();
  lua_pushnumber(L, ((total - mem_available()) >> 10));
  lua_pushnumber(L, (total >> 10));
  return 2;
}

static int io_paste (lua_State *L) {
  MemHandle h;
  UInt16 len;
  char *s;

  if ((h = ClipboardGetItem(clipboardText, &len)) == NULL || len == 0)
    return 0;

  if ((s = MemHandleLock(h)) == NULL)
    return 0;

  lua_pushlstring(L, s, len);
  MemHandleUnlock(h);

  return 1;
}

static int io_copy (lua_State *L) {
  UInt32 len;
  char *s;

  if ((s = (char *)luaL_check_lstr(L, 1, &len)) != NULL && len > 0)
    ClipboardAddItem(clipboardText, s, (UInt16)len);

  return 0;
}

static int io_getprefs (lua_State *L) {
  char *s;
  Int16 size;
  UInt16 id;
  UInt32 creator;
  void *prefs;

  errno = 0;
  s = (char *)luaL_check_string(L, 1);
  creator = StringToCreator(s);
  id = luaL_check_int(L, 2);

  size = 0;
  if (PrefGetAppPreferences(creator, id, NULL, &size, true) ==
                      noPreferenceFound || !size) {
    errno = ENOENT;
    return 0;
  }

  if ((prefs = malloc(size)) == NULL) {
    errno = ENOMEM;
    return 0;
  }

  PrefGetAppPreferences(creator, id, prefs, &size, true);
  lua_pushlstring(L, prefs, size);
  free(prefs);

  return 1;
}

static int io_setprefs (lua_State *L) {
  char *s;
  Int16 size;
  UInt16 id;
  UInt32 creator;
  void *prefs;

  s = (char *)luaL_check_string(L, 1);
  creator = StringToCreator(s);
  id = luaL_check_int(L, 2);
  prefs = (void *)luaL_check_string(L, 3);
  size = lua_strlen(L, 3);

  PrefSetAppPreferences(creator, id, 1, prefs, size, true);

  return 0;
}

#endif

static const luaL_reg syslib[] = {
  {"clock",     io_clock},
  {"date",      io_date},
#ifndef PALMOS
  {"difftime",  io_difftime},
  {"execute",   io_execute},
#endif
  {"exit",      io_exit},
#ifdef PALMOS
  {"sleep",     io_sleep},
  {"mem",       io_mem},
  {"getprefs",  io_getprefs},
  {"setprefs",  io_setprefs},
  {"copy",      io_copy},
  {"paste",     io_paste},
  {"listdb",    io_listdb},
  {"listdir",   io_listdir},
  {"mkdir",     io_mkdir},
#endif
  {"getenv",    io_getenv},
  {"remove",    io_remove},
  {"rename",    io_rename},
#ifndef PALMOS
  {"setlocale", io_setloc},
#endif
  {"time",      io_time},
  {"tmpname",   io_tmpname},
  {NULL, NULL}
};

#ifdef PALMOS
static app_constant constants[] = {
  {1, "resourceDB"},
  {2, "streamDB"},
  {-1,""}
};
#endif


/* }====================================================== */



LUALIB_API int luaopen_io (lua_State *L) {

#ifdef PALMOS
  Int16 i;
  lua_pushliteral(L, "_G");
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  for (i = 0; constants[i].value >= 0; i++) {
    lua_pushstring(L, constants[i].name);
    lua_pushnumber(L, constants[i].value);
    lua_settable(L, -3);
  }
  lua_settop(L, 0);
#endif

  luaL_openlib(L, LUA_OSLIBNAME, syslib, 0);
  createmeta(L);
  lua_pushvalue(L, -1);
  luaL_openlib(L, LUA_IOLIBNAME, iolib, 1);
  /* put predefined file handles into `io' table */
  registerfile(L, stdin, "stdin", IO_INPUT);
  registerfile(L, stdout, "stdout", IO_OUTPUT);
  registerfile(L, stderr, "stderr", NULL);
  return 1;
}


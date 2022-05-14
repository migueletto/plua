/*
** $Id: lparser.h,v 1.47 2003/02/11 10:46:24 roberto Exp $
** Lua Parser
** See Copyright Notice in lua.h
*/

#ifndef lparser_h
#define lparser_h

#include "llimits.h"
#include "lobject.h"
#include "ltable.h"
#include "lzio.h"


/*
** Expression descriptor
*/

typedef enum {
  VVOID,	/* no value */
  VNIL,
  VTRUE,
  VFALSE,
  VK,		/* info = index of constant in `k' */
  VLOCAL,	/* info = local register */
  VUPVAL,       /* info = index of upvalue in `upvalues' */
  VGLOBAL,	/* info = index of table; aux = index of global name in `k' */
  VINDEXED,	/* info = table register; aux = index register (or `k') */
  VJMP,		/* info = instruction pc */
  VRELOCABLE,	/* info = instruction pc */
  VNONRELOC,	/* info = result register */
  VCALL		/* info = result register */
} expkind;

typedef struct expdesc {
  expkind k;
#ifdef PALMOS
  long info, aux;
  long t;  /* patch list of `exit when true' */
  long f;  /* patch list of `exit when false' */
#else
  int info, aux;
  int t;  /* patch list of `exit when true' */
  int f;  /* patch list of `exit when false' */
#endif
} expdesc;


struct BlockCnt;  /* defined in lparser.c */


/* state needed to generate code for a given function */
typedef struct FuncState {
  Proto *f;  /* current function header */
  Table *h;  /* table to find (and reuse) elements in `k' */
  struct FuncState *prev;  /* enclosing function */
  struct LexState *ls;  /* lexical state */
  struct lua_State *L;  /* copy of the Lua state */
  struct BlockCnt *bl;  /* chain of current blocks */
#ifdef PALMOS
  long pc;  /* next position to code (equivalent to `ncode') */
  long lasttarget;   /* `pc' of last `jump target' */
  long jpc;  /* list of pending jumps to `pc' */
  long freereg;  /* first free register */
  long nk;  /* number of elements in `k' */
  long np;  /* number of elements in `p' */
  long nlocvars;  /* number of elements in `locvars' */
  long nactvar;  /* number of active local variables */
  expdesc upvalues[MAXUPVALUES];  /* upvalues */
  long actvar[MAXVARS];  /* declared-variable stack */
#else
  int pc;  /* next position to code (equivalent to `ncode') */
  int lasttarget;   /* `pc' of last `jump target' */
  int jpc;  /* list of pending jumps to `pc' */
  int freereg;  /* first free register */
  int nk;  /* number of elements in `k' */
  int np;  /* number of elements in `p' */
  int nlocvars;  /* number of elements in `locvars' */
  int nactvar;  /* number of active local variables */
  expdesc upvalues[MAXUPVALUES];  /* upvalues */
  int actvar[MAXVARS];  /* declared-variable stack */
#endif
} FuncState;


Proto *luaY_parser (lua_State *L, ZIO *z, Mbuffer *buff);


#endif

#include "p.h"
#include "main.h"
#include "app.h"
#include "gui.h"
#include "lua.h"
#include "lauxlib.h"
#include "compat.h"
#include "PluaAPI.h"
#include "lgraphlib.h"
#include "screen.h"
#include "gcontrol.h"
#include "hr.h"
#include "error.h"

static char *cgetstr(lua_State *L, char *field, char *def) SEC("lib");
static Int32 cgetnum(lua_State *L, char *field, Int32 def) SEC("lib");
static Boolean cgetbool(lua_State *L, char *field, Boolean def) SEC("lib");
static Int16 cgettable(lua_State *L, char *field) SEC("lib");
static Closure *cgetfunc(lua_State *L, char *field) SEC("lib");

#ifdef GADGET_CONTROL
static Boolean gad_callback(void *context, GadgetEvent event, int id, void *data, void *arg, RectangleType *rect);
static void call_prologue(lua_State *L, GadgetEvent event, Closure *handler);
#endif

int gui_control(lua_State *L)
{
  char *text, *type, *file, **items;
  UInt16 id, bmp, group, state;
  Int16 style, x, y, width, height, font, limit, alignment, nitems, sel;
  Int16 list, lines, columns, editable, underlined, visible, leftAnchor;
  Closure *handler;

  if (!lua_istable(L, 1))
    return 0;

  lua_settop(L, 1); 
  type = cgetstr(L, "type", "");
  text = cgetstr(L, "text", "");
  bmp = cgetnum(L, "bitmap", 0);
  file = cgetstr(L, "file", "");
  group = cgetnum(L, "group", 0);
  state = cgetnum(L, "state", 0);
  x = cgetnum(L, "x", -1);
  y = cgetnum(L, "y", -1);
  width = cgetnum(L, "width", -1);
  height = cgetnum(L, "height", -1);
  font = cgetnum(L, "font", -1);
  limit = cgetnum(L, "limit", -1);
  lines = cgetnum(L, "lines", -1);
  columns = cgetnum(L, "columns", -1);
  sel = cgetnum(L, "selected", -1);
  editable = cgetbool(L, "editable", true);
  underlined = cgetbool(L, "underlined", true);
  visible = cgetbool(L, "visible", true);
  alignment = cgetbool(L, "alignment", false) ? rightAlign : leftAlign;
  leftAnchor = cgetbool(L, "leftanchor", true);
  list = cgettable(L, "list");
  handler = cgetfunc(L, "handler");

  id = 0;
  style = -1;

  if (!strcmp(type, "button"))
    style = buttonCtl;
  else if (!strcmp(type, "pbutton"))
    style = pushButtonCtl;
  else if (!strcmp(type, "rbutton"))
    style = repeatingButtonCtl;
  else if (!strcmp(type, "selector"))
    style = selectorTriggerCtl;
  else if (!strcmp(type, "checkbox"))
    style = checkboxCtl;

  else if (!strcmp(type, "label")) {
    if ((id = create_label(text, &x, &y, &font, visible)) == 0)
      return 0;

  } else if (!strcmp(type, "slider")) {
    if (limit == -1)
      limit = 10;

    if (state < 1)
      state = 1;
    else if (state > limit)
      state = limit;

    if (width == -1)
      width = 80;

    if ((id = create_slider(limit, state-1, &x, &y, &width, &height)) == 0)
      return 0;

  } else if (!strcmp(type, "field")) {
    if (lines == -1) lines = 1;
    if (columns == -1) columns = 16;
    if (limit == -1) limit = lines * columns;

    if ((id = create_field(lines, columns, limit, text, editable, underlined,
                alignment, &x, &y, &width, &height, &font)) == 0)
      return 0;

  } else if (!strcmp(type, "list")) {
    if (list == -1)
      return 0;

    if ((items = buildlist(L, list, &nitems)) == NULL || nitems < 1)
      return 0;

    if (lines == -1) lines = nitems;
    if (columns == -1) columns = 16;
    if (sel == -1) sel = 1;

    if ((id = create_list(items, nitems, 0, lines, columns, sel, &x, &y, &width, &height, &font, leftAnchor)) == 0)
      return 0;

  } else if (!strcmp(type, "popup")) {
    if (list == -1)
      return 0;

    if ((items = buildlist(L, list, &nitems)) == NULL || nitems < 1)
      return 0;

    if (sel == -1) sel = 1;

    if ((id = create_list(items, nitems, 1, 0, 0, sel, &x, &y, &width, &height, &font, leftAnchor)) == 0)
      return 0;

#ifdef GADGET_CONTROL
  } else if (!strcmp(type, "gadget")) {
    if (width <= 0 || height <= 0)
      return 0;

    if (x != -1 && y != -1)
      plua_pmoveto(x, y);

    // temporario, sera removido no primeiro evento gadgetDraw logo a seguir
    AppSetRegistry(CONTROL_BASE + LastInternID - FirstInternID, handler);

    if ((id = plua_creategadget(L, width, height, NULL, gad_callback)) == -1)
      return 0;
#endif

  } else
    return 0;

  if (style != -1) {
    if ((id = create_control(style, text, bmp, file, group, state,
                &x, &y, &width, &height, &font, leftAnchor, visible)) == 0)
      return 0;
  }

  AppSetRegistry(CONTROL_BASE + id - FirstInternID, handler);

  lua_pushnumber(L, id);
  return 1;
}

char **buildlist(lua_State *L, Int16 pos, Int16 *n)
{
  Int16 i;
  char **items = NULL;

  *n = 0;

  if (!lua_istable(L, pos))
    return NULL;

  if ((*n = luaL_getn(L, pos)) == 0)
    return NULL;

  if ((items = calloc(*n, sizeof(char *))) == NULL) {
    *n = 0;
    return NULL;
  }

  for (i = 0; i < *n; i++) {
    lua_rawgeti(L, pos, i+1);
    items[i] = strdup((char *)luaL_check_string(L, lua_gettop(L)));
    lua_pop(L, 1);
  }

  return items;
}

#ifdef GADGET_CONTROL
static Boolean gad_callback(void *context, GadgetEvent event, int id, void *data, void *arg, RectangleType *rect)
{
  lua_State *L;
  Closure *handler;
  PointType *point;
  char *text, *value;
  Int16 *state;
  int x0, y0;

  handler = AppGetRegistry(CONTROL_BASE + LastInternID - FirstInternID);
  if (handler != NULL)
    AppSetRegistry(CONTROL_BASE + LastInternID - FirstInternID, NULL);
  else
    handler = AppGetRegistry(CONTROL_BASE + id - FirstInternID);

  if (context == NULL || handler == NULL)
    return true;

  L = (lua_State *)context;

  switch (event) {
    case gadgetDraw:
      plua_ppos(&x0, &y0);
      plua_pmoveto(rect->topLeft.x, rect->topLeft.y);
      hrWinSetClip(rect);
      call_prologue(L, event, handler);
      lua_pushnumber(L, rect->extent.x);
      lua_pushnumber(L, rect->extent.y);
      lua_call(L, 3, 0);
      WinResetClip();
      plua_pmoveto(x0, y0);
      break;
    case gadgetErase:
      hrWinEraseRectangle(rect, 0);
      break;
    case gadgetDelete:
      break;
    case gadgetDown:
    case gadgetMove:
      point = (PointType *)arg;
      call_prologue(L, event, handler);
      lua_pushnumber(L, point->x);
      lua_pushnumber(L, point->y);
      lua_call(L, 3, 0);
      break;
    case gadgetUp:
      call_prologue(L, event, handler);
      lua_call(L, 1, 0);
      break;
    case gadgetGetState:
      state = (Int16 *)arg;
      call_prologue(L, event, handler);
      lua_call(L, 1, 0);
      *state = (Int16)lua_tonumber(L, -1);
      break;
    case gadgetSetState:
      state = (Int16 *)arg;
      call_prologue(L, event, handler);
      lua_pushnumber(L, *state);
      lua_call(L, 2, 0);
      break;
    case gadgetGetText:
      text = (char *)arg;
      call_prologue(L, event, handler);
      lua_call(L, 1, 0);
      value = (char *)lua_tostring(L, -1);
      StrNCopy(text, value, 32);
      break;
    case gadgetSetText:
      text = (char *)arg;
      call_prologue(L, event, handler);
      lua_pushstring(L, text);
      lua_call(L, 1, 0);
  }

  return true;
}

static void call_prologue(lua_State *L, GadgetEvent event, Closure *handler)
{
  lua_pushclosure(L, handler);
  lua_pushnumber(L, event);
}
#endif

static char *cgetstr(lua_State *L, char *field, char *def)
{
  char *value;

  lua_pushstring(L, field);
  lua_gettable(L, 1);
  if (lua_isnil(L, -1))
    return def;
  value = (char *)lua_tostring(L, -1);

  return value;
}

static Int32 cgetnum(lua_State *L, char *field, Int32 def)
{
  Int32 value;

  lua_pushstring(L, field);
  lua_gettable(L, 1);
  if (lua_isnil(L, -1))
    return def;
  value = (Int16)lua_tonumber(L, -1);

  return value;
}

static Boolean cgetbool(lua_State *L, char *field, Boolean def)
{
  Boolean value;

  lua_pushstring(L, field);
  lua_gettable(L, 1);
  if (lua_isnil(L, -1))
    return def;
  value = (Boolean)lua_toboolean(L, -1);

  return value;
}

static Int16 cgettable(lua_State *L, char *field)
{
  lua_pushstring(L, field);
  lua_gettable(L, 1);
  if (lua_isnil(L, -1))
    return -1;

  return lua_gettop(L);
}

static Closure *cgetfunc(lua_State *L, char *field)
{
  Closure *value;

  lua_pushstring(L, field);
  lua_gettable(L, 1);
  if (lua_isnil(L, -1))
    return NULL;
  value = (Closure *)lua_toclosure(L, -1);

  return value;
}

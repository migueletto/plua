#include <PalmOS.h>

#include "Plua.h"
#include "PluaAPI.h"

#define LIBNAME "3dlib2"
#define BUTTON_LABEL 32

typedef struct {
  UInt32 text, light, normal, dark, blank;
  FontID font;
  Boolean pressed;
  char label[BUTTON_LABEL];
} Button3D;

static void drawbutton(lua_State *L, RectangleType *rect, Button3D *b3d)
{
  PLUA_HEADER;
  int width, height, d;
  Int16 x, y, dx, dy;
  FontID old;

  d = b3d->pressed ? 1 : 0;
  old = FntSetFont(b3d->font);
  plua_textsize(b3d->label, &width, &height);

  x = rect->topLeft.x;
  y = rect->topLeft.y;
  dx = rect->extent.x;
  dy = rect->extent.y;
  plua_color(b3d->normal, b3d->normal);
  plua_box(x, y, dx,dy); 
  plua_color(b3d->pressed ? b3d->dark : b3d->light, b3d->normal);
  plua_line(x, y, x, y+dy-1);
  plua_line(x, y, x+dx-1, y); 
  plua_color(b3d->pressed ? b3d->light : b3d->dark, b3d->normal);
  plua_line(x+dx-1, y, x+dx-1, y+dy-1);
  plua_line(x, y+dy-1, x+dx-1, y+dy-1);

  plua_color(b3d->text, b3d->normal);
  plua_moveto(rect->topLeft.x + (rect->extent.x - width) / 2 + d,
               rect->topLeft.y + (rect->extent.y - height) / 2 + d);
  plua_print(b3d->label);
  FntSetFont(old);
}

Boolean button3d_callback(void *context, GadgetEvent event, int id,
                          void *data, void *arg, RectangleType *rect)
{
  lua_State *L = (lua_State *)context;
  PLUA_HEADER;
  Button3D *b3d;
  EventType evt;
  Int16 *state;

  b3d = (Button3D *)data;
  
  switch (event) {
    case gadgetDraw:
      drawbutton(L, rect, b3d);
      break;
    case gadgetUp:
      b3d->pressed = false;
      drawbutton(L, rect, b3d);
      MemSet(&evt, sizeof(evt), 0);
      evt.eType = ctlSelectEvent;
      evt.data.ctlSelect.controlID = id;
      evt.data.ctlSelect.on = 1;
      evt.data.ctlSelect.value = 1;
      EvtAddEventToQueue(&evt);
      break;
    case gadgetErase:
      plua_color(b3d->blank, b3d->blank);
      plua_box(rect->topLeft.x, rect->topLeft.y,
                rect->extent.x, rect->extent.y);
      break;
    case gadgetDown:
      b3d->pressed = true;
      drawbutton(L, rect, b3d);
      break;
    case gadgetDelete:
      free(b3d);
      break;
    case gadgetGetState:
      state = (Int16 *)arg;
      *state = b3d->pressed ? 1 : 0;
      break;
    case gadgetSetState:
      state = (Int16 *)arg;
      b3d->pressed = state ? true : false;
      break;
    case gadgetGetText:
      if (arg)
        StrNCopy((char *)arg, b3d->label, BUTTON_LABEL-1);
      break;
    case gadgetSetText:
      if (arg)
        StrNCopy(b3d->label, (char *)arg, BUTTON_LABEL-1);
  }

  return true;
}

int button3d(lua_State *L)
{
  PLUA_HEADER;
  Button3D *b3d;
  char *label;
  Int16 id, width, height;
  long fg, bg;
  int r, g, b;

  if ((b3d = malloc(sizeof(Button3D))) == NULL)
    return 0;

  label = (char *)luaL_check_string(L, 1);
  width = luaL_check_int(L, 2);
  height = luaL_check_int(L, 3);

  StrNCopy(b3d->label, label, BUTTON_LABEL-1);

  plua_getcolor(&fg, &bg);
  plua_getrgb(bg, &r, &g, &b);
  b3d->normal = plua_rgb(r, g, b);
  b3d->light = plua_rgb(1.50*r, 1.50*g, 1.50*b);
  b3d->dark = plua_rgb(0.50*r, 0.50*g, 0.50*b);
  b3d->blank = plua_rgb(255, 255, 255);

  plua_getrgb(fg, &r, &g, &b);
  b3d->text = plua_rgb(r, g, b);
  b3d->font = FntGetFont();
  b3d->pressed = false;

  if ((id = plua_creategadget(L, width, height, b3d, button3d_callback))==-1) {
    free(b3d);
    return 0;
  }

  lua_pushnumber(L, id);
  return 1;
}

// Library initialization function. You may alloc your global data and
// register Lua functions.

static int L3dLibInit(lua_State *L)
{
  PLUA_HEADER;
  const struct luaL_reg lib3d[] = {
    {"button3d", button3d},
    {NULL, NULL}
  };

  // register C functions in Lua
  luaL_openlib(L, "gui", lib3d, 0);

  // return sucess
  return 0;
}

// This function is called when the library is first loaded by a Plua program.
// It must return a pointer to the library initialization function.

lua_CFunction PluaLibInit(lua_State *L, char *s)
{
  return L3dLibInit;
}

// This function is called when Plua exits. You must unregister all the
// functions and free the global data.

Err PluaLibFinish(lua_State *L)
{
  PLUA_HEADER;

  // unregister C functions in Lua
  lua_pushstring(L, "gui");
  lua_gettable(L, LUA_GLOBALSINDEX);
  lua_pushstring(L, "button3d");
  lua_pushnil(L);
  lua_settable(L, -3);

  // return sucess
  return 0;
}

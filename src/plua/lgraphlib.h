#ifndef _LGRAPHLIB_H
#define _LGRAPHLIB_H

// app.c
int gui_destroy(lua_State *L);

// lplualib.c
int plua_creategadget(void *context, int width, int height, void *data, GadgetCallback handler);

// handler.c
Int16 findcontrol(lua_State *L, Int16 pos);

#endif

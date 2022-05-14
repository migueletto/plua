#ifndef _SPRITE_H
#define _SPRITE_H 1

#define MAX_SPRITES 32

typedef struct {
  Boolean set;
  Boolean active;
  Boolean drawn;
  Boolean collided;
  Closure *checkCollision;
  BitmapPtr bmp, mask, pad;
  WinHandle padw;
  Int16 x, y, dx, dy;
  RectangleType rect;
} SpriteObject;

typedef struct {
  WinHandle buffer;
  WinHandle background;
  WinHandle screen;
  Int16 x, y;
  RectangleType rect;
  SpriteObject sprite[MAX_SPRITES];
  Boolean changed;
  Boolean firstDraw;
  Boolean firstDirty;
  RectangleType dirty;
} SpriteWorld;

SpriteWorld *SpriteInit(lua_State *L, WinHandle background, WinHandle screen, Int16 x, Int16 y) SEC("aux");
void SpriteBackground(SpriteWorld *world, WinHandle background) SEC("aux");
void SpriteUpdate(lua_State *L, SpriteWorld *world) SEC("aux");
void SpriteDraw(SpriteWorld *world) SEC("aux");
void SpriteFinish(lua_State *L, SpriteWorld *world) SEC("aux");

Err SpriteInsertObject(lua_State *L, Int16 o, SpriteWorld *world, UInt16 index) SEC("aux");
void SpriteDeleteObject(lua_State *L, SpriteWorld *world, UInt16 index) SEC("aux");

#endif

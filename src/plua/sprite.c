#include "p.h"
#include "main.h"
#include "app.h"
#include "lua.h"
#include "lauxlib.h"
#include "compat.h"
#include "PluaAPI.h"
#include "hr.h"
#include "screen.h"
#include "sprite.h"
#include "error.h"

static Err SpriteLoadBitmap(lua_State *L, Int16 o, SpriteWorld *world,
                            UInt16 index) SEC("aux");
static void SpriteResetDirtyArea(SpriteWorld *world) SEC("aux");
static void SpriteAddDirtyArea(SpriteWorld *world, RectangleType *rect) SEC("aux");
static Boolean SpriteCollision(SpriteWorld *world, RectangleType *rect, SpriteObject *obj1, SpriteObject *obj2) SEC("aux");

static Int16 getnum(lua_State *L, Int16 index, char *field) SEC("aux");
static Boolean getbool(lua_State *L, Int16 index, char *field) SEC("aux");
static Closure *getfunc(lua_State *L, Int16 index, char *field) SEC("aux");

SpriteWorld *SpriteInit(lua_State *L, WinHandle background,
                        WinHandle screen, Int16 x, Int16 y)
{
  DisplayType *disp;
  SpriteWorld *world;
  WinHandle old;
  UInt16 dx, dy, index;
  Err err;

  if (!background || !screen)
    return NULL;

  if ((world = malloc(sizeof(SpriteWorld))) == NULL)
    return NULL;

  disp = getdisplay();

  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);

  world->background = background;
  world->screen = screen;
  world->x = x;
  world->y = y;

  old = WinSetDrawWindow(background);
  WinGetDrawWindowBounds(&world->rect);
  WinSetDrawWindow(old);

  dx = world->rect.extent.x;
  dy = world->rect.extent.y;

  world->buffer = WinCreateOffscreenWindow(dx, dy, nativeFormat, &err);

  if (world->buffer == NULL) {
    if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
    free(world);
    errno = ENOMEM;
    return NULL;
  }

  for (index = 0; index < MAX_SPRITES; index++)
    SpriteDeleteObject(L, world, index);

  SpriteResetDirtyArea(world);
  WinCopyRectangle(world->background, world->buffer, &world->rect,
                   0, 0, winPaint);

  world->changed = true;
  world->firstDraw = true;

  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  return world;
}

void SpriteBackground(SpriteWorld *world, WinHandle background)
{
  if (world && background) {
    world->background = background;
    world->firstDraw = true;

    WinCopyRectangle(world->background, world->buffer, &world->rect,
                     0, 0, winPaint);
  }
}

void SpriteFinish(lua_State *L, SpriteWorld *world)
{
  WinHandle old;
  UInt16 index;

  if (world) {
    old = WinSetDrawWindow(world->buffer);
    WinEraseRectangle(&world->rect, 0);
    WinSetDrawWindow(old);

    WinCopyRectangle(world->buffer, world->screen, &world->rect,
      world->x, world->y, winPaint);

    WinDeleteWindow(world->buffer, false);

    for (index = 0; index < MAX_SPRITES; index++)
      SpriteDeleteObject(L, world, index);

    free(world);
  }
}

Err SpriteInsertObject(lua_State *L, Int16 o, SpriteWorld *world, UInt16 index)
{
  SpriteObject *obj;

  if (!world || index >= MAX_SPRITES) {
    errno = EINVAL;
    return -1;
  }

  obj = &world->sprite[index];
  obj->bmp = NULL;

  if (SpriteLoadBitmap(L, o, world, index) != 0)
    return -1;

  obj->x = getnum(L, o, "x");
  obj->y = getnum(L, o, "y");

  obj->set = true;
  obj->drawn = false;
  obj->rect.topLeft.x = obj->x;
  obj->rect.topLeft.y = obj->y;

  lua_pushnumber(L, SPRITE_BASE + index);
  lua_pushvalue(L, o);
  lua_rawset(L, LUA_REGISTRYINDEX);

  return 0;
}

static Err SpriteLoadBitmap(lua_State *L, Int16 o, SpriteWorld *world,
                            UInt16 index)
{
  DisplayType *disp;
  UInt16 oldIndex, dx, dy, rowBytes, density, err;
  SpriteObject *obj;
  BitmapPtr bmp;
  UInt32 transp;
  Int16 rsrc, i, j, k;
  UInt8 *bmpBits, *maskBits, black, white;
  RGBColorType rgb;

  errno = EINVAL;
  rsrc = getnum(L, o, "bitmap");

  if (rsrc == 0)
    return -1;

  if ((bmp = rsrc2bmp(rsrc)) == NULL)
    return -2;

  obj = &world->sprite[index];
  if (bmp == obj->bmp) {
    errno = 0;
    return 0;
  }

  if (BmpGlueGetCompressionType(bmp) != BitmapCompressionTypeNone ||
      BmpGlueGetBitDepth(bmp) != 8)
    return -3;
  
  disp = getdisplay();

  if (disp->highDensity) {
    density = BmpGetDensity(bmp);
    for (oldIndex = 0; oldIndex < MAX_SPRITES; oldIndex++)
      if (index != oldIndex && world->sprite[oldIndex].set &&
          density != BmpGetDensity(world->sprite[oldIndex].bmp))
        return -4; 
  }

  rgb.r = rgb.g = rgb.b = 0;
  black = WinRGBToIndex(&rgb);
  
  rgb.r = rgb.g = rgb.b = 255;
  white = WinRGBToIndex(&rgb);

  if (!BmpGlueGetTransparentValue(bmp, &transp))
    transp = white;

  hrBmpGlueGetDimensions(bmp, &dx, &dy, &rowBytes);
  
  obj->dx = dx;
  obj->dy = dy;
  obj->rect.extent.x = dx;
  obj->rect.extent.y = dy;

  BmpGlueGetDimensions(bmp, &dx, &dy, &rowBytes); // true res

  if (obj->padw) WinDeleteWindow(obj->padw, false);
  if (obj->pad)  BmpDelete(obj->pad);
  if (obj->mask) BmpDelete(obj->mask);
  obj->mask = NULL;
  obj->pad = NULL;
  obj->padw = NULL;

  errno = ENOMEM;

  if ((obj->mask = BmpCreate(dx, dy, 8, NULL, &err)) == NULL)
    return -5;

  if ((obj->pad = BmpCreate(dx, dy, 8, NULL, &err)) == NULL) {
    BmpDelete(obj->mask);
    obj->mask = NULL;
    return -6;
  }

  if ((obj->padw = WinCreateBitmapWindow(obj->pad, &err)) == NULL) {
    BmpDelete(obj->pad);
    BmpDelete(obj->mask);
    obj->pad = NULL;
    obj->mask = NULL;
    return -7;
  }

  bmpBits = BmpGetBits(bmp);
  maskBits = BmpGetBits(obj->mask);
  BmpGlueGetDimensions(obj->mask, &dx, &dy, &rowBytes); // true res

  for (i = 0, k = 0; i < dy; i++)
    for (j = 0; j < rowBytes; j++, k++)
      maskBits[k] = (bmpBits[k] == transp) ? white : black;

  obj->bmp = bmp;
  errno = 0;

  return 0;
}

void SpriteDeleteObject(lua_State *L, SpriteWorld *world, UInt16 index)
{
  SpriteObject *obj;

  if (world && index < MAX_SPRITES) {
    obj = &world->sprite[index];
    if (obj->padw) WinDeleteWindow(obj->padw, false);
    if (obj->pad)  BmpDelete(obj->pad);
    if (obj->mask) BmpDelete(obj->mask);
    obj->set = false;
    obj->mask = NULL;
    obj->pad = NULL;
    obj->padw = NULL;
    obj->bmp = NULL;

    lua_pushnumber(L, SPRITE_BASE + index);
    lua_pushnil(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
  }
}

void SpriteUpdate(lua_State *L, SpriteWorld *world)
{
  DisplayType *disp;
  UInt16 index, testIndex;
  WinHandle old;
  SpriteObject *obj, *testObj;
  RectangleType intersection;

  if (world) {
    disp = getdisplay();

    if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
    old = WinSetDrawWindow(world->buffer);

    SpriteResetDirtyArea(world);

    for (index = 0; index < MAX_SPRITES; index++) {
      obj = &world->sprite[index];

      if (obj->set) {
        lua_settop(L, 0);
        lua_pushnumber(L, SPRITE_BASE + index);
        lua_rawget(L, LUA_REGISTRYINDEX);

        obj->active = getbool(L, 1, "active");

        if (obj->active) {
          SpriteLoadBitmap(L, 1, world, index);

          obj->x = getnum(L, 1, "x");
          obj->y = getnum(L, 1, "y");
          obj->checkCollision = getfunc(L, 1, "collision");

        } else if (obj->drawn) {
          WinCopyRectangle(world->background, world->buffer, &obj->rect,
                           obj->rect.topLeft.x, obj->rect.topLeft.y, winPaint);
          SpriteAddDirtyArea(world, &obj->rect);

          obj->drawn = false;
        }
      } else
        obj->active = false;
    }

    for (index = 0; index < MAX_SPRITES; index++) {
      obj = &world->sprite[index];
      obj->collided = false;

      if (obj->active) {
        WinCopyRectangle(world->background, world->buffer, &obj->rect,
                         obj->rect.topLeft.x, obj->rect.topLeft.y, winPaint);
        SpriteAddDirtyArea(world, &obj->rect);
      }
    }

    for (index = 0; index < MAX_SPRITES; index++) {
      obj = &world->sprite[index];
      if (obj->active) {
        obj->drawn = true;

        WinPaintBitmap(obj->bmp, obj->x, obj->y);
        obj->rect.topLeft.x = obj->x;
        obj->rect.topLeft.y = obj->y;
        SpriteAddDirtyArea(world, &obj->rect);

        if (obj->checkCollision) {
          for (testIndex = 0; testIndex < index; testIndex++) {
            testObj = &world->sprite[testIndex];

            if (testObj->active && testObj->checkCollision &&
                !obj->collided && !testObj->collided) {

              RctGetIntersection(&obj->rect, &testObj->rect, &intersection);

              if (intersection.extent.x != 0 && intersection.extent.y != 0 &&
                  SpriteCollision(world, &intersection, obj, testObj)) {
                lua_settop(L, 0);
                lua_pushclosure(L, obj->checkCollision);

                lua_pushnumber(L, SPRITE_BASE + index);
                lua_rawget(L, LUA_REGISTRYINDEX);

                lua_pushnumber(L, SPRITE_BASE + testIndex);
                lua_rawget(L, LUA_REGISTRYINDEX);

                WinSetDrawWindow(old);
                if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
                lua_call(L, 2, 0);
                if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
                WinSetDrawWindow(world->buffer);
              }
            }
          }
        }
      }
    }

    WinSetDrawWindow(old);
    if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  }
  lua_settop(L, 0);
}

void SpriteDraw(SpriteWorld *world)
{
  DisplayType *disp;

  if (world && (world->firstDraw || world->changed)) {
    disp = getdisplay();

    if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);

    if (world->firstDraw) {
      world->firstDraw = false;
      WinCopyRectangle(world->buffer, world->screen, &world->rect,
        world->x, world->y, winPaint);
    } else {
      WinCopyRectangle(world->buffer, world->screen, &world->dirty,
        world->x+world->dirty.topLeft.x, world->y+world->dirty.topLeft.y,
        winPaint);
    }

    if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  }
}

static void SpriteResetDirtyArea(SpriteWorld *world)
{
  if (world) {
    world->firstDirty = true;
    world->changed = false;
  }
}

static void SpriteAddDirtyArea(SpriteWorld *world, RectangleType *rect)
{
  if (world && rect) {
    world->changed = true;

    if (world->firstDirty) {
      world->firstDirty = false;
      world->dirty.topLeft.x = rect->topLeft.x;
      world->dirty.topLeft.y = rect->topLeft.y;
      world->dirty.extent.x = rect->extent.x;
      world->dirty.extent.y = rect->extent.y;
    } else {
      if (rect->topLeft.x < world->dirty.topLeft.x) {
        world->dirty.extent.x += world->dirty.topLeft.x - rect->topLeft.x;
        world->dirty.topLeft.x = rect->topLeft.x;
      } else if (rect->topLeft.x + rect->extent.x >
                 world->dirty.topLeft.x + world->dirty.extent.x) {
        world->dirty.extent.x = rect->topLeft.x + rect->extent.x -
                                world->dirty.topLeft.x;
      }

      if (rect->topLeft.y < world->dirty.topLeft.y) {
        world->dirty.extent.y += world->dirty.topLeft.y - rect->topLeft.y;
        world->dirty.topLeft.y = rect->topLeft.y;
      } else if (rect->topLeft.y + rect->extent.y >
                 world->dirty.topLeft.y + world->dirty.extent.y) {
        world->dirty.extent.y = rect->topLeft.y + rect->extent.y -
                                world->dirty.topLeft.y;
      }
    }
  }
}

static Boolean SpriteCollision(SpriteWorld *world, RectangleType *rect,
                               SpriteObject *obj1, SpriteObject *obj2)
{
  DisplayType *disp;
  Int16 x, y , k, dx, dy, rowBytes, x0, x1, y0, y1;
  UInt8 *maskBits, *padBits, white;
  Boolean collision;
  RGBColorType rgb;
  WinHandle old;
  UInt16 mode;

  disp = getdisplay();

  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);

  old = WinSetDrawWindow(obj1->padw);
  mode = WinSetDrawMode(winPaint);

  WinPaintBitmap(obj1->mask, 0, 0);
  WinSetDrawMode(winInvert);
  if (disp->highDensity && BmpGetDensity(obj2->bmp) == kDensityLow)
    WinPaintBitmap(obj2->mask, (obj2->x - obj1->x)/disp->factorX,
                               (obj2->y - obj1->y)/disp->factorY);
  else
    WinPaintBitmap(obj2->mask, (obj2->x - obj1->x),
                               (obj2->y - obj1->y));

  WinSetDrawMode(mode);
  WinSetDrawWindow(old);

  maskBits = BmpGetBits(obj1->mask);
  padBits = BmpGetBits(obj1->pad);
  BmpGlueGetDimensions(obj1->bmp, &dx, &dy, &rowBytes); // true res

  x0 = 0;
  x1 = rowBytes;
  //y0 = rect->topLeft.y - obj1->y;
  //y1 = y0 + rect->extent.y;
  y0 = 0;
  y1 = dy;

  rgb.r = rgb.g = rgb.b = 255;
  white = WinRGBToIndex(&rgb);

  for (y = y0, k = y0*rowBytes, collision = false; y < y1 && !collision; y++)
    for (x = x0; x < x1 && !collision; x++, k++)
      if (maskBits[k] != white && maskBits[k] != padBits[k]) {
        rect->topLeft.x = x;
        rect->topLeft.y = y;
        collision = true;
      }

  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  return collision;
}

static Int16 getnum(lua_State *L, Int16 index, char *field)
{
  lua_pushstring(L, field);
  lua_gettable(L, index);
  return (Int16)lua_tonumber(L, -1);
}

static Boolean getbool(lua_State *L, Int16 index, char *field)
{
  lua_pushstring(L, field);
  lua_gettable(L, index);
  return (Boolean)lua_toboolean(L, -1);
}

static Closure *getfunc(lua_State *L, Int16 index, char *field)
{
  lua_pushstring(L, field);
  lua_gettable(L, index);
  return (Closure *)lua_toclosure(L, -1);
}

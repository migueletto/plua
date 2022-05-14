#include <windows.h>
#include <commctrl.h>

#include "p.h"
#include "lua.h"
#include "compat.h"
#include "lauxlib.h"
#include "lualib.h"
#include "plualibl.h"
#include "lgraphlib.h"
#include "main.h"

#define xi displayPtr->clip0.topLeft.x
#define yi displayPtr->clip0.topLeft.y
#define PI (3.14159265358979323846)
#define RADIANS_PER_DEGREE (PI/180.0)
#define TORAD(a) ((a)*RADIANS_PER_DEGREE)
#define FirstInternID   1800
#define LastInternID    1999
#define FirstExternID   2000
#define LastExternID    4999
#define DX 4

static char *popenbmp = "\
  function popenbmp(id, file) \
    local r, size, width, height \
    r = popenres(\"Tbmp\", id, file) or popenres(\"tAIB\", id, file) \
    if r then \
      size, width, height = pgetsize(r) \
      return r, width, height \
    end \
  end";

extern HWND hwnd;
extern HINSTANCE hinst;
static HBITMAP hbmp = NULL;
static HPEN hpen = NULL;
static HBRUSH hbg = NULL;
static DisplayType *displayPtr = NULL;
static DisplayType appDisplay;
static DisplayType bufDisplay;
static DisplayType dialogDisplay;
static UInt16 controlId = FirstInternID;
static Int16 nControls = 0;
static Int16 nControlsDialog = 0;
static UInt16 dynamicControls[LastInternID-FirstInternID+1];
static HWND ctlWindow[LastInternID-FirstInternID+1];
static UInt16 ctlType[LastInternID-FirstInternID+1];
static HDC handlePool[NUM_HANDLE];
static HBITMAP handlePoolBitmap[NUM_HANDLE];
static SIZE handlePoolSize[NUM_HANDLE];
static Int16 nForms = 0;

static void WinSetClip(void) {
  IntersectClipRect(displayPtr->wh,
    displayPtr->clip.topLeft.x, displayPtr->clip.topLeft.y,
    displayPtr->clip.topLeft.x + displayPtr->clip.extent.x,
    displayPtr->clip.topLeft.y + displayPtr->clip.extent.y);
}

static void WinResetClip(void) {
  SelectClipRgn(displayPtr->wh, NULL);
}

static int g_mode (lua_State *L) {
  lua_pushnumber(L, displayPtr->clip0.extent.x);
  lua_pushnumber(L, displayPtr->clip0.extent.y);
  lua_pushnumber(L, displayPtr->depth);
  lua_pushnumber(L, displayPtr->enableColor);
  return 4;
}

static int g_clear (lua_State *L) {
  UInt32 c;
  HBRUSH aux;
  RECT rect;

  c = luaL_opt_long(L, 1, displayPtr->bg);
  displayPtr->x = 0;
  displayPtr->y = 0;
  displayPtr->heading = 0;
  displayPtr->maxY = 0;

  aux = CreateSolidBrush(c);
  rect.top = displayPtr->clip.topLeft.y;
  rect.bottom = rect.top + displayPtr->clip.extent.y;
  rect.left = displayPtr->clip.topLeft.x;
  rect.right = rect.left + displayPtr->clip.extent.x;
  FillRect(displayPtr->wh, &rect, aux);
  DeleteObject(aux);

  InvalidateRect(hwnd, NULL, FALSE);

  return 0;
}

void plua_pcolor(long fg, long bg)
{
  HPEN hpen1;
  HBRUSH hbg1;

  hpen1 = hpen;
  hpen = CreatePen(PS_SOLID, 0, fg);
  SelectObject(displayPtr->wh, hpen);
  if (hpen1) DeleteObject(hpen1);

  hbg1 = hbg;
  hbg = CreateSolidBrush(bg);
  if (hbg1) DeleteObject(hbg1);

  SetTextColor(displayPtr->wh, fg);
  SetBkColor(displayPtr->wh, bg);

  displayPtr->fg = fg;
  displayPtr->bg = bg;
}

static int g_color (lua_State *L) {
  UInt32 fg, bg;

  fg = luaL_check_long(L, 1);
  bg = luaL_opt_long(L, 2, displayPtr->bg);

  plua_pcolor(fg, bg);

  return 0;
}

long plua_prgb(int r, int g, int b)
{
  r = r > 255 ? 255 : r;
  g = g > 255 ? 255 : g;
  b = b > 255 ? 255 : b;

  return RGB(r, g, b);
}

static int g_rgb (lua_State *L) {
  Int16 r, g, b;

  r = luaL_check_int(L, 1);
  g = luaL_check_int(L, 2);
  b = luaL_check_int(L, 3);

  lua_pushnumber(L, plua_prgb(r, g, b));
  return 1;
}

void plua_ppos(int *x, int *y)
{
  *x = displayPtr->x;
  *y = displayPtr->y;
}

static int g_pos (lua_State *L) {
  lua_pushnumber(L, displayPtr->x);
  lua_pushnumber(L, displayPtr->y);
  return 2;
}

static int g_nl (lua_State *L) {
  TEXTMETRIC tm;

  GetTextMetrics(displayPtr->wh, &tm);
  displayPtr->x = 0;
  displayPtr->y += displayPtr->maxY ? displayPtr->maxY+2 : tm.tmHeight;
  displayPtr->maxY = 0;
  return 0;
}

static int g_tab (lua_State *L) {
  TEXTMETRIC tm;

  GetTextMetrics(displayPtr->wh, &tm);
  Int16 t = lua_isnull(L, 1) ? 1 : luaL_check_int(L, 1);
  displayPtr->x += t*tm.tmAveCharWidth;
  return 0;
}

void plua_pmoveto(int x, int y)
{
  displayPtr->x = x;
  displayPtr->y = y;
  displayPtr->maxY = 0;
}

static int g_moveto (lua_State *L) {
  Int16 x, y;

  x = luaL_check_int(L, 1);
  y = luaL_check_int(L, 2);

  plua_pmoveto(x, y);

  return 0;
}

static void _line (Int16 x1, Int16 y1, Int16 x2, Int16 y2, UInt32 c)
{
  HPEN hpen0, hpen1;

  WinSetClip();

  hpen0 = hpen;
  hpen1 = CreatePen(PS_SOLID, 0, c);
  SelectObject(displayPtr->wh, hpen1);
  MoveToEx(displayPtr->wh, xi+x1, yi+y1, NULL);
  LineTo(displayPtr->wh, xi+x2, yi+y2);
  SelectObject(displayPtr->wh, hpen0);
  DeleteObject(hpen1);

  WinResetClip();

  displayPtr->x = x2;
  displayPtr->y = y2;
  InvalidateRect(hwnd, NULL, FALSE);
}

void plua_pline(int x1, int y1, int x2, int y2)
{
  _line(x1, y1, x2, y2, displayPtr->fg);
}

static int g_line (lua_State *L) {
  Int16 x1, y1, x2, y2;
  UInt32 c;

  x1 = luaL_check_int(L, 1);
  y1 = luaL_check_int(L, 2);
  x2 = luaL_check_int(L, 3);
  y2 = luaL_check_int(L, 4);
  c = luaL_opt_long(L, 5, displayPtr->fg);
  _line(x1, y1, x2, y2, c);

  return 0;
}

void plua_plineto(int x, int y)
{
  _line(displayPtr->x, displayPtr->y, x, y, displayPtr->fg);
}

static int g_lineto (lua_State *L) {
  Int16 x1, y1, x2, y2;
  UInt32 c;

  x1 = displayPtr->x;
  y1 = displayPtr->y;
  x2 = luaL_check_int(L, 1);
  y2 = luaL_check_int(L, 2);
  c = luaL_opt_long(L, 3, displayPtr->fg);
  _line(x1, y1, x2, y2, c);

  return 0;
}

static void _rect(Int16 x, Int16 y, Int16 dx, Int16 dy, Int16 filled, UInt32 c)
{
  RECT rect;
  HBRUSH hbg1;
  HPEN hpen0, hpen1;

  WinSetClip();

  if (filled) {
    hbg1 = CreateSolidBrush(c);
    rect.top = yi+y;
    rect.bottom = yi+y+dy+1;
    rect.left = xi+x;
    rect.right = xi+x+dx+1;
    FillRect(displayPtr->wh, &rect, hbg1);
    DeleteObject(hbg1);
  } else {
    hpen0 = hpen;
    hpen1 = CreatePen(PS_SOLID, 0, c);
    SelectObject(displayPtr->wh, hpen1);
    MoveToEx(displayPtr->wh, xi+x, yi+y, NULL);
    LineTo(displayPtr->wh, xi+x+dx, yi+y);
    LineTo(displayPtr->wh, xi+x+dx, yi+y+dy);
    LineTo(displayPtr->wh, xi+x, yi+y+dy);
    LineTo(displayPtr->wh, xi+x, yi+y);
    SelectObject(displayPtr->wh, hpen0);
    DeleteObject(hpen1);
  }

  WinResetClip();

  displayPtr->x = x+dx+1;
  InvalidateRect(hwnd, NULL, FALSE);
}

void plua_prect(int x, int y, int dx, int dy)
{
  _rect(x, y, dx, dy, 0, displayPtr->fg);
} 

static int g_rect (lua_State *L) {
  Int16 x, y, dx, dy;
  UInt32 c;
  
  x = luaL_check_int(L, 1);
  y = luaL_check_int(L, 2);
  dx = luaL_check_int(L, 3);
  dy = luaL_check_int(L, 4);
  c = luaL_opt_long(L, 5, displayPtr->fg);

  _rect(x, y, dx, dy, 0, c);
  return 0;
}

void plua_pbox(int x, int y, int dx, int dy)
{
  _rect(x, y, dx, dy, 1, displayPtr->fg);
}

static int g_box (lua_State *L) {
  Int16 x, y, dx, dy;
  UInt32 c;
    
  x = luaL_check_int(L, 1);
  y = luaL_check_int(L, 2);
  dx = luaL_check_int(L, 3);
  dy = luaL_check_int(L, 4);
  c = luaL_opt_long(L, 5, displayPtr->fg);

  _rect(x, y, dx, dy, 1, c);
  return 0;
}

static void _circle(Int16 x, Int16 y, Int16 rx, Int16 ry, UInt32 c) {
  HPEN hpen0, hpen1;
  
  WinSetClip();

  hpen0 = hpen;
  hpen1 = CreatePen(PS_SOLID, 0, c);
  SelectObject(displayPtr->wh, hpen1);
  Arc(displayPtr->wh, xi+x-rx, yi+y-ry, xi+x+rx, yi+y+ry,
    xi+x, yi+y-ry, xi+x, yi+y-ry);
  SelectObject(displayPtr->wh, hpen0);
  DeleteObject(hpen1);

  WinResetClip();
}

void plua_pcircle(int x, int y, int rx, int ry)
{ 
  _circle(x, y, rx, ry, displayPtr->fg);
}

static int g_circle (lua_State *L) {
  Int16 x, y, rx, ry;
  UInt32 c;
  
  x = xi+luaL_check_int(L, 1);
  y = yi+luaL_check_int(L, 2);
  rx = luaL_check_int(L, 3);
  ry = luaL_check_int(L, 4);
  c = luaL_opt_long(L, 5, displayPtr->fg);
  
  _circle(x, y, rx, ry, c);
  
  return 0;
}

static void _disc(Int16 x, Int16 y, Int16 rx, Int16 ry, UInt32 c) {
  HPEN hpen0, hpen1;
  HBRUSH hbg0, hbg1;

  WinSetClip();

  hpen0 = hpen;
  hpen1 = CreatePen(PS_SOLID, 0, c);
  SelectObject(displayPtr->wh, hpen1);

  hbg0 = hbg;
  hbg1 = CreateSolidBrush(c);
  SelectObject(displayPtr->wh, hbg1);

  Ellipse(displayPtr->wh, xi+x-rx, yi+y-ry, xi+x+rx, yi+y+ry);

  SelectObject(displayPtr->wh, hpen0);
  DeleteObject(hpen1);

  SelectObject(displayPtr->wh, hbg0);
  DeleteObject(hbg1);

  WinResetClip();
}

void plua_pdisc(int x, int y, int rx, int ry)
{ 
  _disc(x, y, rx, ry, displayPtr->fg);
}

static int g_disc(lua_State *L) {
  Int16 x, y, rx, ry;
  UInt32 c;
  
  x = xi+luaL_check_int(L, 1);
  y = yi+luaL_check_int(L, 2);
  rx = luaL_check_int(L, 3);
  ry = luaL_check_int(L, 4);
  c = luaL_opt_long(L, 5, displayPtr->fg);

  _disc(x, y, rx, ry, c);

  return 0;
}

static int g_fill(lua_State *L) {
  Int16 x, y;
  UInt32 c, c0;
  HBRUSH hbg0, hbg1;
  
  x = xi+luaL_check_int(L, 1);
  y = yi+luaL_check_int(L, 2);
  c = luaL_opt_long(L, 3, displayPtr->fg);
  c0 = GetPixel(displayPtr->wh, x, y);
  
  WinSetClip();

  hbg0 = hbg;
  hbg1 = CreateSolidBrush(c);
  SelectObject(displayPtr->wh, hbg1);

  ExtFloodFill(displayPtr->wh, x, y, c0, FLOODFILLSURFACE);

  SelectObject(displayPtr->wh, hbg0);
  DeleteObject(hbg1);

  WinResetClip();
  
  return 0;
}

static int g_newbuffer (lua_State *L) {
  Int16 i, dx, dy;
  HBITMAP hbmp1;

  dx = luaL_check_int(L, 1);
  dy = luaL_check_int(L, 2);

  for (i = 0; i < NUM_HANDLE; i++)
    if (!handlePool[i])
      break;

  if (i == NUM_HANDLE)
    return 0;

  if ((handlePool[i] = CreateCompatibleDC(appDisplay.wh)) == NULL)
    return 0;

  if ((hbmp1 = CreateCompatibleBitmap(appDisplay.wh, dx, dy)) == NULL) {
    DeleteDC(handlePool[i]);
    handlePool[i] = NULL;
    return 0;
  }
  SelectObject(handlePool[i], hbmp1);
  handlePoolSize[i].cx = dx;
  handlePoolSize[i].cy = dy;
  handlePoolBitmap[i] = hbmp1;

  lua_pushnumber(L, i);
  return 1;
}

static int g_getbuffer (lua_State *L) {
  Int16 i, x, y, dx, dy;
  HBITMAP hbmp1;
  
  x = xi+luaL_check_int(L, 1);
  y = yi+luaL_check_int(L, 2);
  dx = luaL_check_int(L, 3);
  dy = luaL_check_int(L, 4);
  
  for (i = 0; i < NUM_HANDLE; i++)
    if (!handlePool[i])
      break;

  if (i == NUM_HANDLE)
    return 0;

  if ((handlePool[i] = CreateCompatibleDC(appDisplay.wh)) == NULL)
    return 0;

  if ((hbmp1 = CreateCompatibleBitmap(appDisplay.wh, dx, dy)) == NULL) {
    DeleteDC(handlePool[i]);
    handlePool[i] = NULL;
    return 0;
  }
  SelectObject(handlePool[i], hbmp1);
  handlePoolSize[i].cx = dx;
  handlePoolSize[i].cy = dy;
  handlePoolBitmap[i] = hbmp1;

  BitBlt(handlePool[i], 0, 0, dx, dy, displayPtr->wh, x, y, SRCCOPY);

  lua_pushnumber(L, i);
  return 1;
}

static int g_putbuffer (lua_State *L) {
  UInt16 i;
  Int16 x, y;

  i = luaL_check_int(L, 1);
  x = xi+luaL_check_int(L, 2);
  y = yi+luaL_check_int(L, 3);

  if (i >= NUM_HANDLE || !handlePool[i])
    return 0;

  WinSetClip();

  BitBlt(displayPtr->wh, x, y, handlePoolSize[i].cx, handlePoolSize[i].cy,
    handlePool[i], 0, 0, SRCCOPY);

  WinResetClip();

  return 0;
}

static int g_freebuffer (lua_State *L) {
  UInt16 i;

  i = luaL_check_int(L, 1);

  if (i >= NUM_HANDLE || !handlePool[i])
    return 0;

  if (displayPtr->wh == handlePool[i])
    displayPtr = &appDisplay;

  DeleteDC(handlePool[i]);
  handlePool[i] = NULL;

  DeleteObject(handlePoolBitmap[i]);
  handlePoolBitmap[i] = NULL;

  return 0;
}

static int g_usebuffer (lua_State *L) {
  Int16 i;
  
  i = luaL_opt_int(L, 1, -1);
  
  if (i == -1) {
    displayPtr = nForms > 0 ? &dialogDisplay : &appDisplay;
    return 0;
  }
  
  if (i >= NUM_HANDLE || !handlePool[i])
    return 0;
    
  displayPtr = &bufDisplay;
  
  displayPtr->width = handlePoolSize[i].cx;
  displayPtr->height = handlePoolSize[i].cy;

  displayPtr->clip0.topLeft.x = 0;
  displayPtr->clip0.topLeft.y = 0;
  displayPtr->clip0.extent.x = displayPtr->width;
  displayPtr->clip0.extent.y = displayPtr->height;

  displayPtr->clip.topLeft.x = 0;
  displayPtr->clip.topLeft.y = 0;
  displayPtr->clip.extent.x = displayPtr->width;
  displayPtr->clip.extent.y = displayPtr->height;


  displayPtr->wh = handlePool[i];
  displayPtr->depth = appDisplay.depth;
  displayPtr->enableColor = appDisplay.enableColor;
  displayPtr->x = 0;
  displayPtr->y = 0;
  displayPtr->maxY = 0;
  displayPtr->font = 0;
  displayPtr->heading = 0;
  plua_pcolor(plua_prgb(0, 0, 0), plua_prgb(191, 191, 191));

  return 0;
}

static int g_set (lua_State *L) {
  UInt32 c;
  
  displayPtr->x = luaL_check_int(L, 1);
  displayPtr->y = luaL_check_int(L, 2);
  c = luaL_opt_long(L, 3, displayPtr->fg);
  
  _line(xi+displayPtr->x, yi+displayPtr->y+1,
        xi+displayPtr->x, yi+displayPtr->y, c);
  return 0;
}

static int g_get (lua_State *L) {
  Int16 x = luaL_check_int(L, 1);
  Int16 y = luaL_check_int(L, 2);
  UInt32 p;
  
  p = GetPixel(displayPtr->wh, xi+x, yi+y);
  lua_pushnumber(L, p);
  return 1;
}

static int g_setcliping (lua_State *L) {
  Int16 x, y, dx, dy;

  if (lua_isnull(L, 1)) {
    displayPtr->clip.topLeft.x = displayPtr->clip0.topLeft.x;
    displayPtr->clip.topLeft.y = displayPtr->clip0.topLeft.y;
    displayPtr->clip.extent.x = displayPtr->clip0.extent.x;
    displayPtr->clip.extent.y = displayPtr->clip0.extent.y;
  } else {
    x = luaL_check_int(L, 1);
    y = luaL_check_int(L, 2);
    dx = luaL_check_int(L, 3);
    dy = luaL_check_int(L, 4);

    if (x < displayPtr->clip0.topLeft.x) {
      x = displayPtr->clip0.topLeft.x;
      dx -= displayPtr->clip0.topLeft.x - x;
      if (dx < 0) dx = 0;
    }
    if (y < displayPtr->clip0.topLeft.y) {
      y = displayPtr->clip0.topLeft.y;
      dy -= displayPtr->clip0.topLeft.y - y;
      if (dy < 0) dy = 0;
    }
    if (x + dx > displayPtr->clip0.topLeft.x + displayPtr->clip0.extent.x)
      dx = displayPtr->clip0.topLeft.x + displayPtr->clip0.extent.x - x;
    if (y + dy > displayPtr->clip0.topLeft.y + displayPtr->clip0.extent.y)
      dy = displayPtr->clip0.topLeft.y + displayPtr->clip0.extent.y - y;

    displayPtr->clip.topLeft.x = x;
    displayPtr->clip.topLeft.y = y;
    displayPtr->clip.extent.x = dx;
    displayPtr->clip.extent.y = dy;
  }

  return 0;
}

static int g_heading (lua_State *L) {
  displayPtr->heading = luaL_check_int(L, 1);
  return 0;
}
  
static int g_turn (lua_State *L) {
  displayPtr->heading += luaL_check_int(L, 1);
  return 0;
}
  
static int g_walk (lua_State *L) {
  Int16 x, y, d;
  UInt32 c;
  Number gr;

  d = luaL_check_int(L, 1);
  c = luaL_opt_long(L, 2, displayPtr->fg);

  if (d == 0)
    return 0;
    
  x = displayPtr->x;
  y = displayPtr->y;
  gr = TORAD(displayPtr->heading);
  x += (Int16)(d * cos(gr));
  y -= (Int16)(d * sin(gr));

  _line(displayPtr->x, displayPtr->y, x, y, c);

  return 0;
}

static int g_event (lua_State *L) {
  EventType event;
  Int16 x, y;
  UInt16 id;
  Int32 tps, wait, maxWait;
  FILE *f;
  Boolean forever;

  tps = 1000; //SysTicksPerSecond();
  wait = tps / 10;
  maxWait = lua_isnull(L, 1) ? -1 : luaL_check_long(L, 1);
  forever = maxWait < 0;

  if (maxWait >= 0) {
    maxWait = (maxWait * tps) / 1000;
    if (wait > maxWait)
      wait = maxWait;
  }

  while (1) {
    if (!ProcessEvent(&event, wait, 1))
      continue;

    switch (event.eType) {
      case appStopEvent:
        lua_pushnumber(L, appStopEvent);
        return 1;
      case nilEvent:
        if (forever)
          continue;
        maxWait -= wait;
        if (maxWait > 0)
          continue;
        lua_pushnumber(L, event.eType);
        return 1;
      case ctlSelectEvent:
        id = event.data.ctlSelect.controlID;
        if (id < FirstInternID || id > LastExternID)
          continue;
        if (id <= LastInternID && ctlType[id-FirstInternID] == sliderCtl)
          event.data.ctlSelect.value =
            SendMessage(ctlWindow[id-FirstInternID], TBM_GETPOS, 0, 0);
        lua_pushnumber(L, event.eType);
        lua_pushnumber(L, id);
        lua_pushnumber(L, event.data.ctlSelect.on);
        lua_pushnumber(L, event.data.ctlSelect.value);
        return 4;
      case keyDownEvent:
        lua_pushnumber(L, keyDownEvent);
        lua_pushnumber(L, event.data.keyDown.chr);
        return 2;
      case penUpEvent:
      case penDownEvent:
      case penMoveEvent:
        x = event.screenX - xi;
        y = event.screenY - yi;
        lua_pushnumber(L, event.eType);
        lua_pushnumber(L, x);
        lua_pushnumber(L, y);
        return 3;
      default:
        continue;
    }
  }
  return 0;
}

static int g_alert (lua_State *L) {
  char *msg = (char *)luaL_check_string(L, 1);

  if (msg)
    MessageBox(hwnd, msg, "Information", MB_OK);

  return 0;
}

static int g_confirm (lua_State *L) {
  char *msg = (char *)luaL_check_string(L, 1);

  if (msg) {
    Int16 r = MessageBox(hwnd, msg, "Confirmation", MB_YESNO);
    if (r == IDNO)
      return 0;
    lua_pushnumber(L, 1);
    return 1;
  }
  return 0;
}

static UInt16 nextID(void) {
  if (nControls < (LastInternID-FirstInternID+1))
    dynamicControls[nControls++] = controlId;
  return controlId++;
}

static void previousID(void) {
  if (nControls > 0) {
    nControls--;
    controlId--;
  }
}

static int create_control (lua_State *L, Int16 style, char *name, UInt16 resId,
                           UInt8 group) {
  Int16 x, y, dx, dy;
  UInt16 id;
  DWORD ctlStyle;
  HWND ctl;
  char *class;
  TEXTMETRIC tm;
  SIZE size;

  GetTextMetrics(displayPtr->wh, &tm);
  GetTextExtentPoint32(displayPtr->wh, name, strlen(name), &size);

  switch (style) {
    case buttonCtl:
    case repeatingButtonCtl:
    case selectorTriggerCtl:
      class = "BUTTON";
      ctlStyle = BS_PUSHBUTTON;
      dx = 2*tm.tmAveCharWidth + size.cx;
      dy = size.cy + (size.cy*2)/3;
      break;
    case pushButtonCtl:
      class = "BUTTON";
      ctlStyle = BS_AUTORADIOBUTTON;
      dx = 4*tm.tmAveCharWidth + size.cx;
      dy = size.cy /*+ (size.cy*2)/3*/;
      break;
    case checkboxCtl:
      class = "BUTTON";
      ctlStyle = BS_AUTOCHECKBOX;
      dx = 4*tm.tmAveCharWidth + size.cx;
      dy = size.cy /*+ (size.cy*2)/3*/;
      break;
    default:
      return 0;
  }

  x = displayPtr->x;
  y = displayPtr->y;
  id = nextID();

  ctl = CreateWindow(class, name, WS_CHILD | WS_VISIBLE | ctlStyle,
    xi+x, yi+y, dx, dy, hwnd, (HMENU)id, hinst, NULL);

  if (ctl) {
    displayPtr->x += dx + DX;
    if (dy > displayPtr->maxY)
      displayPtr->maxY = dy;

    ctlWindow[nControls-1] = ctl;
    ctlType[nControls-1] = style;

    lua_pushnumber(L, id);
    return 1;
  }

  previousID();
  return 0;
}

static int g_button (lua_State *L) {
  char *name; 
  UInt16 resId;

  name = (char *)luaL_check_string(L, 1);
  resId = lua_gettop(L) > 1 ? luaL_check_int(L, 2) : 0;
  
  return create_control(L, buttonCtl, name, resId, 0);
}

static int g_pushbutton (lua_State *L) {
  char *name;
  UInt8 group;
  UInt16 resId;

  name = (char *)luaL_check_string(L, 1);
  group = lua_gettop(L) > 1 ? luaL_check_int(L, 2) : 0;
  resId = lua_gettop(L) > 2 ? luaL_check_int(L, 3) : 0;

  return create_control(L, pushButtonCtl, name, resId, group);
}

static int g_repeatbutton (lua_State *L) {
  char *name;
  UInt16 resId;

  name = (char *)luaL_check_string(L, 1);
  resId = lua_gettop(L) > 1 ? luaL_check_int(L, 2) : 0;

  return create_control(L, repeatingButtonCtl, name, resId, 0);
}

static int g_selectortrigger (lua_State *L) {
  char *name;
  UInt16 resId;

  name = (char *)luaL_check_string(L, 1);
  resId = lua_gettop(L) > 1 ? luaL_check_int(L, 2) : 0;

  return create_control(L, selectorTriggerCtl, name, resId, 0);
}

static int g_checkbox (lua_State *L) {
  char *name = (char *)luaL_check_string(L, 1);
  return create_control(L, checkboxCtl, name, 0, 0);
}

static int g_field (lua_State *L) {
  Int16 x, y, dx, dy, nlines, ncols, max, height;
  char *value;
  UInt16 id;
  DWORD ctlStyle;
  HWND ctl;
  Boolean editable = true, underlined = true;
  TEXTMETRIC tm;

  nlines = luaL_check_int(L, 1);
  ncols = luaL_check_int(L, 2);
  max = luaL_check_int(L, 3)+1;
  if (max <= 0) max = 1;
  value = (char *)luaL_opt_string(L, 4, "");
  if (lua_gettop(L) > 4) {
    editable = lua_isnil(L, 5) ? false : true;
    underlined = lua_isnil(L, 6) ? false : true;
  }

  if (!nlines || !ncols || !max)
    return 0;

  x = displayPtr->x;
  y = displayPtr->y;
  id = nextID();

  GetTextMetrics(displayPtr->wh, &tm);
  dx = ncols * tm.tmAveCharWidth;
  dy = nlines * tm.tmHeight + 4;

  ctlStyle = WS_BORDER;
  if (nlines > 1) {
    ctlStyle |= ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL;
    dx += GetSystemMetrics(SM_CXVSCROLL);
  } else
    ctlStyle |= ES_AUTOHSCROLL;
  if (!editable)
    ctlStyle |= ES_READONLY;

  ctl = CreateWindow("EDIT", value, WS_CHILD | WS_VISIBLE | ctlStyle,
    xi+x, yi+y, dx, dy, hwnd, (HMENU)id, hinst, NULL); 

  if (ctl) {
    displayPtr->x += dx + DX;
    if (dy > displayPtr->maxY)
      displayPtr->maxY = dy;

    ctlWindow[nControls-1] = ctl;

    lua_pushnumber(L, id); 
    return 1;
  }

  previousID();
  return 0;
}

static char **build_list(lua_State *L, Int16 pos, Int16 *n) {
  Int16 i;
  char **items = NULL;

  *n = 0;

  if (!lua_istable(L, pos))
    return NULL;

  if ((*n = lua_getn(L, pos)) == 0)
    return NULL;

  if ((items = calloc(*n, sizeof(char *))) == NULL) {
    *n = 0;
    return NULL;
  }

  for (i = 0; i < *n; i++) {
    lua_rawgeti(L, pos, i+1);
    items[i] = (char *)luaL_check_string(L, lua_gettop(L));
    lua_pop(L, 1);
  }

  return items;
}

static int g_list (lua_State *L) {
  Int16 x, y, dx, dy, sel, nlines, ncols, height, i, j;
  UInt16 id;
  char **items;
  DWORD ctlStyle;
  HWND ctl;
  TEXTMETRIC tm;

  nlines = luaL_check_int(L, 1);
  ncols = luaL_check_int(L, 2);
  items = build_list(L, 3, &i);
  sel = luaL_opt_int(L, 4, 0);

  if (nlines <= 0 || ncols <= 0) {
    if (items)
      free(items);
    return 0;
  }

  if (sel < 0 || sel >= i)
    sel = 0;

  x = displayPtr->x;
  y = displayPtr->y;
  id = nextID();

  GetTextMetrics(displayPtr->wh, &tm);
  dx = ncols * tm.tmAveCharWidth;
  dy = nlines * tm.tmHeight + 4;

  ctlStyle = WS_BORDER | WS_VSCROLL;

  ctl = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | ctlStyle,
    xi+x, yi+y, dx, dy, hwnd, (HMENU)id, hinst, NULL);

  if (ctl) {
    for (j = 0; j < i; j++)
      SendDlgItemMessage(hwnd, id, LB_ADDSTRING, 0, (LPARAM)items[j]);
    free(items);

    SendDlgItemMessage(hwnd, id, LB_SETCURSEL, sel, 0);

    displayPtr->x += dx + DX;
    if (dy > displayPtr->maxY)
      displayPtr->maxY = dy;

    ctlWindow[nControls-1] = ctl;

    lua_pushnumber(L, id);
    return 1;
  }

  free(items);
  previousID();
  return 0;
}

static int g_popup (lua_State *L) {
  Int16 x, y, dx, dy, sel, nlines, ncols, height, i, j;
  UInt16 id;
  char **items;
  DWORD ctlStyle;
  HWND ctl;
  TEXTMETRIC tm;
  SIZE size;

  items = build_list(L, 1, &i);
  sel = luaL_opt_int(L, 2, 0);

  if (!items || !i) {
    if (items)
      free(items);
    return 0;
  }
    
  id = nextID();
  x = displayPtr->x;
  y = displayPtr->y;
  nlines = i;

  GetTextMetrics(displayPtr->wh, &tm);
  dy = (nlines + 1) * tm.tmHeight + 16;

  for (dx = tm.tmAveCharWidth, j = 0; j < i; j++) {
    GetTextExtentPoint32(displayPtr->wh, items[j], strlen(items[j]), &size);
    if (size.cx > dx)
      dx = size.cx; 
  }
  dx += 32;

  if ((y+dy) > displayPtr->clip0.extent.y) {
    nlines = (displayPtr->clip0.extent.y-y) / (tm.tmHeight + 2);
    dy = nlines * tm.tmHeight + 4;
    dx += 8;
  }

  if (sel < 0 || sel >= i)
    sel = 0;

  ctlStyle = WS_BORDER | WS_VSCROLL | CBS_DROPDOWNLIST;

  ctl = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | ctlStyle,
    xi+x, yi+y, dx, dy, hwnd, (HMENU)id, hinst, NULL);

  if (ctl) {
    for (j = 0; j < i; j++)
      SendDlgItemMessage(hwnd, id, CB_ADDSTRING, 0, (LPARAM)items[j]);
    free(items);

    SendDlgItemMessage(hwnd, id, CB_SETCURSEL, sel, 0);

    displayPtr->x += dx + DX;
    dy = tm.tmHeight + 16;
    if (dy > displayPtr->maxY)
      displayPtr->maxY = dy;

    ctlWindow[nControls-1] = ctl;

    lua_pushnumber(L, id);
    return 1;
  }

  free(items);
  previousID();
  return 0;
}

static int g_title (lua_State *L) {
  char *title = (char *)luaL_opt_string(L, 1, NULL);
  if (title && title[0])
    SetWindowText(hwnd, title);
}

static int create_slider (lua_State *L, Int16 style, UInt16 thumbId,
                          UInt16 backgroundId, Int16 dx, Int16 dy,
                          Int16 min, Int16 max, Int16 page, Int16 value) {
  Int16 x, y;
  UInt16 id;
  HWND ctl;
  DWORD ctlStyle;

  x = displayPtr->x;
  y = displayPtr->y;
  id = nextID();

  ctlStyle = TBS_AUTOTICKS;

  ctl = CreateWindow(TRACKBAR_CLASS, "", WS_CHILD | WS_VISIBLE | ctlStyle,
    xi+x, yi+y, dx, dy, hwnd, (HMENU)id, hinst, NULL);

  if (ctl) {
    SendMessage(ctl, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(min, max));
    SendMessage(ctl, TBM_SETPOS, (WPARAM)1, (LPARAM)value);

    displayPtr->x += dx + DX;
    if (dy > displayPtr->maxY)
      displayPtr->maxY = dy;

    ctlWindow[nControls-1] = ctl;
    ctlType[nControls-1] = style;

    lua_pushnumber(L, id);
    return 1;
  }

  previousID();
  return 0;
}

static int g_slider (lua_State *L) { 
  UInt16 dx, dy, bw, bh;
  Int16 d, min, max, value;
    
  dx = luaL_check_int(L, 1);
  d = luaL_check_int(L, 2);
  value = lua_gettop(L) > 2 ? luaL_check_int(L, 3) : 0;
  dy = 32;

  min = 0;
  max = d-1;
  if (value < min)
    value = min;
  else if (value > max)
    value = max;

  return create_slider(L, sliderCtl, 0, 0, dx, dy, min, max, 1, value);
}

void PrintString (char *s, int n) {
  int i, k;
  TEXTMETRIC tm;

  GetTextMetrics(displayPtr->wh, &tm);

  for (i = 0, k = 0; i < n && s[i]; i++)
    switch (s[i]) {
      case '\t':
        if (i > k)
          print_str(&s[k], i-k);
        displayPtr->x += tm.tmAveCharWidth;
        k = i+1;
        break;
      case '\n':
        if (i > k)
          print_str(&s[k], i-k);
        displayPtr->x = 0;
        displayPtr->y += tm.tmHeight;
        k = i+1;
    }

  if (i > k)
    print_str(&s[k], i-k);
}

void print_str (char *s, int n) {
  SIZE size;

  WinSetClip();
  TextOut(displayPtr->wh, xi+displayPtr->x, yi+displayPtr->y, s, n);
  WinResetClip();
  GetTextExtentPoint32(displayPtr->wh, s, n, &size);
  displayPtr->x += size.cx;
  InvalidateRect(hwnd, NULL, FALSE);
}

static void init(void) {
  HDC aux;
  RECT rect;
  int i;

  displayPtr = &appDisplay;

  GetClientRect(hwnd, &rect);
  displayPtr->width = rect.right - rect.left;
  displayPtr->height = rect.bottom - rect.top;

  displayPtr->clip0.topLeft.x = 0;
  displayPtr->clip0.topLeft.y = 0;
  displayPtr->clip0.extent.x = displayPtr->width;
  displayPtr->clip0.extent.y = displayPtr->height;

  displayPtr->clip.topLeft.x = 0;
  displayPtr->clip.topLeft.y = 0;
  displayPtr->clip.extent.x = displayPtr->width;
  displayPtr->clip.extent.y = displayPtr->height;

  aux = GetDC(hwnd);
  displayPtr->wh = CreateCompatibleDC(aux);
  hbmp = CreateCompatibleBitmap(aux, displayPtr->width, displayPtr->height);
  SelectObject(displayPtr->wh, hbmp);
  ReleaseDC(hwnd, aux);

  displayPtr->depth = GetDeviceCaps(displayPtr->wh, BITSPIXEL);
  displayPtr->enableColor = 1;

  SetBkMode(displayPtr->wh, OPAQUE);
  plua_pcolor(plua_prgb(0, 0, 0), plua_prgb(191, 191, 191));
  FillRect(displayPtr->wh, &rect, hbg);
  InvalidateRect(hwnd, NULL, FALSE);

  displayPtr->x = 0;
  displayPtr->y = 0;
  displayPtr->maxY = 0;
  displayPtr->font = 0;
  displayPtr->heading = 0;

  for (i = 0; i < NUM_HANDLE; i++) {
    handlePool[i] = NULL;
    handlePoolBitmap[i] = NULL;
  }
}

static void finish(void) {
  DeleteDC(displayPtr->wh);
  if (hbg) DeleteObject(hbg);
  if (hpen) DeleteObject(hpen);
  if (hbmp) DeleteObject(hbmp);
  hbg = NULL;
  hpen = NULL;
  hbmp = NULL;
  displayPtr = NULL;
}

void plua_paint(HDC target)
{
  if (appDisplay.wh)
    BitBlt(target, 0, 0, appDisplay.width, appDisplay.height,
      appDisplay.wh, 0, 0, SRCCOPY);
}

static const struct luaL_reg graphlib[] = {
{"pmode", g_mode},
{"pclear", g_clear},
{"pcolor", g_color},
{"prgb", g_rgb},
{"ppos", g_pos},
{"pmoveto", g_moveto},
{"pline", g_line},
{"plineto", g_lineto},
{"pset", g_set},
{"pget", g_get},
{"pclip", g_setcliping},
{"prect", g_rect},
{"pbox", g_box},
{"pcircle", g_circle},
{"pdisc", g_disc},
{"pfill", g_fill},
{"pnewbuffer", g_newbuffer},
{"pgetbuffer", g_getbuffer},
{"pputbuffer", g_putbuffer},
{"pfreebuffer", g_freebuffer},
{"pusebuffer", g_usebuffer},
{"pheading", g_heading},
{"pturn", g_turn},
{"pwalk", g_walk},
{"pevent", g_event},
{"pbutton", g_button},
{"ppbutton", g_pushbutton},
{"prbutton", g_repeatbutton},
{"pselector", g_selectortrigger},
{"pcheckbox", g_checkbox},
{"pslider", g_slider},
{"pfield", g_field},
{"plist", g_list},
{"ppopup", g_popup},
{"pnl", g_nl},
{"ptab", g_tab},
{"palert", g_alert},
{"pconfirm", g_confirm},
{"ptitle", g_title},
{NULL, NULL}
};

typedef struct plua_constan {
  Int32 value;
  char *name; 
} plua_constant;

static plua_constant constants[] = {
  {nilEvent,            "nilEvent"},
  {ctlSelectEvent,      "ctlSelect"},
  {keyDownEvent,        "keyDown"},
  {penDownEvent,        "penDown"},
  {penUpEvent,          "penUp"},
  {penMoveEvent,        "penMove"},
  {appStopEvent,        "appStop"},
  {-1,""}
};

LUALIB_API void lua_graphlibopen(lua_State *L) {
  Int16 i;
  luaL_reg *l;

  init();

  lua_pushstring(L, "ui");
  lua_gettable(L, LUA_GLOBALSINDEX);  // check whether lib already exists
  if (lua_isnil(L, -1)) {  // no?
    lua_pop(L, 1);
    lua_newtable(L);  // create it
    lua_pushstring(L, "ui");
    lua_pushvalue(L, -2);
    lua_settable(L, LUA_GLOBALSINDEX);  // register it with given name
  }
  lua_insert(L, -1);  // move library table to below upvalues

  for (l = (luaL_reg *)graphlib; l->name; l++) {
    lua_pushstring(L, &l->name[1]);
    lua_pushcclosure(L, l->func, 0);
    lua_settable(L, -3);
  }
  for (i = 0; constants[i].value >= 0; i++) {
    lua_pushstring(L, constants[i].name);
    lua_pushnumber(L, constants[i].value);
    lua_settable(L, -3);
  }
  lua_pop(L, 0);  // remove upvalues

  lua_dobuffer(L, popenbmp, strlen(popenbmp), "popenbmp");
}

LUALIB_API void lua_graphlibclose(lua_State *L) {
  finish();
}

#include "p.h"
#include "lua.h"
#include "lapi.h"
#include "ltable.h"
#include "lstring.h"
#include "main.h"
#include "app.h"
#include "events.h"
#include "printstring.h"
#include "error.h"
#include "db.h"
#include "scroll.h"
#include "hr.h"
#include "bmp.h"
#include "fill.h"
#include "sound.h"
#include "rsrc.h"
#include "file.h"
#include "midi.h"
#include "text.h"
#include "gcontrol.h"
#include "screen.h"
#include "sprite.h"
#include "handler.h"
#include "compat.h"
#include "MathLib.h"
#include "gui.h"

#include "lauxlib.h"
#include "lualib.h"
#include "plualibl.h"
#include "lvm.h"

#include "PluaAPI.h"
#include "PluaLib.h"
#include "lgraphlib.h"

#define PI (3.14159265358979323846)
#define RADIANS_PER_DEGREE (PI/180.0)
#define TORAD(a) ((a)*RADIANS_PER_DEGREE)

#define xi displayPtr->clip0.topLeft.x
#define yi displayPtr->clip0.topLeft.y

#define NUM_GADGETS     64
#define NUM_HANDLE      32
#define NUM_SCROLLS     4
#define MAX_FORMS       1

typedef struct {
  void *context;
  UInt16 id;
  void *data;
  GadgetCallback callback;
  RectangleType rect;
} GadgetData;

static int screen_mode(lua_State *L) SEC("graph");
static int screen_clear(lua_State *L) SEC("graph");
static int screen_color(lua_State *L) SEC("graph");
static int screen_rgb(lua_State *L) SEC("graph");
static int screen_pos(lua_State *L) SEC("graph");
static int screen_moveto(lua_State *L) SEC("graph");
static int screen_line(lua_State *L) SEC("graph");
static int screen_lineto(lua_State *L) SEC("graph");
static int screen_setpixel(lua_State *L) SEC("graph");
static int screen_getpixel(lua_State *L) SEC("graph");
static int screen_setcliping(lua_State *L) SEC("graph");
static int screen_rect(lua_State *L) SEC("graph");
static int screen_box(lua_State *L) SEC("graph");
static int screen_circle(lua_State *L) SEC("graph");
static int screen_disc(lua_State *L) SEC("graph");
static int screen_fill(lua_State *L) SEC("graph");
static int screen_font(lua_State *L) SEC("graph");
static int screen_textsize(lua_State *L) SEC("graph");

static int screen_walkjump(Int16 d, UInt32 c, Boolean draw) SEC("graph");
static int screen_walk(lua_State *L) SEC("graph");
static int screen_jump(lua_State *L) SEC("graph");
static int screen_heading(lua_State *L) SEC("graph");
static int screen_turn(lua_State *L) SEC("graph");

static int gui_event(lua_State *L) SEC("graph");
static int gui_label(lua_State *L) SEC("graph");
static int gui_button(lua_State *L) SEC("graph");
static int gui_pushbutton(lua_State *L) SEC("graph");
static int gui_repeatbutton(lua_State *L) SEC("graph");
static int gui_selectortrigger(lua_State *L) SEC("graph");
static int gui_checkbox(lua_State *L) SEC("graph");
static int gui_slider(lua_State *L) SEC("graph");
static int gui_field(lua_State *L) SEC("graph");
static int gui_fieldattr(lua_State *L) SEC("graph");
static int gui_list(lua_State *L) SEC("graph");
static int gui_popup(lua_State *L) SEC("graph");
static int gui_inserttext(lua_State *L) SEC("graph");
static int gui_gettext(lua_State *L) SEC("graph");
static int gui_settext(lua_State *L) SEC("graph");
static int gui_getstate(lua_State *L) SEC("graph");
static int gui_setstate(lua_State *L) SEC("graph");
static int gui_state(lua_State *L, Boolean) SEC("graph");
static int gui_setlist(lua_State *L) SEC("graph");
static int gui_setvisible(lua_State *L) SEC("graph");
static int gui_setfocus(lua_State *L) SEC("graph");
static int gui_nl(lua_State *L) SEC("graph");
static int gui_tab(lua_State *L) SEC("graph");
static int gui_alert(lua_State *L) SEC("graph");
static int gui_confirm(lua_State *L) SEC("graph");
static int gui_input(lua_State *L) SEC("graph");
static int gui_title(lua_State *L) SEC("graph");
static int gui_menu(lua_State *L) SEC("graph");
static int gui_gsi(lua_State *L) SEC("graph");
static int gui_selectdate(lua_State *L) SEC("graph");
static int gui_selecttime(lua_State *L) SEC("graph");
static int gui_selectcolor(lua_State *L) SEC("graph");
static int gui_dialog(lua_State *L) SEC("graph");

static int buffer_read(lua_State *L) SEC("graph");
static int buffer_write(lua_State *L) SEC("graph");
static int buffer_new(lua_State *L) SEC("graph");
static int buffer_get(lua_State *L) SEC("graph");
static int buffer_put(lua_State *L) SEC("graph");
static int buffer_free(lua_State *L) SEC("graph");
static int buffer_use(lua_State *L) SEC("graph");

static int sprite_init(lua_State *L) SEC("graph");
static int sprite_add(lua_State *L) SEC("graph");
static int sprite_remove(lua_State *L) SEC("graph");
static int sprite_update(lua_State *L) SEC("graph");
static int sprite_finish(lua_State *L) SEC("graph");

static int sound_beep(lua_State *L) SEC("graph");
static int sound_tone(lua_State *L) SEC("graph");
static int sound_midi(lua_State *L) SEC("graph");
static int sound_play(lua_State *L) SEC("graph");
static int sound_stop(lua_State *L) SEC("graph");

static int resource_open(lua_State *L) SEC("graph");
static int resource_close(lua_State *L) SEC("graph");
static int resource_draw(lua_State *L) SEC("graph");

static void init(void) SEC("graph");
static void finish(lua_State *L) SEC("graph");
static void field_settext(FieldType *fld, char *s) SEC("graph");
static UInt16 nextID(void) SEC("graph");
static void previousID(void) SEC("graph");
static UInt16 nextSclID(void) SEC("graph");
static Boolean GadgetHandler(FormGadgetTypeInCallback *gad, UInt16 cmd, void *p) SEC("graph");
static Int16 pusherror(lua_State *L, Int16 err) SEC("graph");
static void resizedisplay(void) SEC("graph");
static void printstr(char *s, int n) SEC("graph");
static void setclip(DisplayType *disp) SEC("graph");
static void destroyform(lua_State *L, FormPtr frm);

static void draw_rect(Int16 x, Int16 y, Int16 dx, Int16 dy, Int16 filled, UInt32 c) SEC("graph");
static void draw_line(Int16 x1, Int16 y1, Int16 x2, Int16 y2, UInt32 c) SEC("graph");
static void draw_circle(Int16 x, Int16 y, Int16 rx, Int16 ry, UInt32 c) SEC("graph");
static void draw_disc(Int16 x, Int16 y, Int16 rx, Int16 ry, UInt32 c) SEC("graph");
static void setitems(UInt16 id, void *p);
static void freeitems(UInt16 i);
static void freemenu(void);

static Int16 romNumber;
static DisplayType appDisplay;
static DisplayType bufDisplay;
static DisplayType dialogDisplay;
static DisplayType *displayPtr;
static Boolean fullScreen = false;
static ResourceType *resourcePool[NUM_RESOURCE];
static WinHandle handlePool[NUM_HANDLE];
static Int16 nGadgets = 0;
static GadgetData *gadgetPool[NUM_GADGETS];
static Int16 inGadget = -1;
static UInt16 controlId = FirstInternID;
static Int16 nControls = 0;
static Int16 nControlsDialog = 0;
static UInt16 dynamicControls[LastInternID-FirstInternID+1];
static void *listItems[LastInternID-FirstInternID+1];
static Int16 nScrolls = 0;
static UInt16 scrollIds[NUM_SCROLLS];
static UInt16 fieldIds[NUM_SCROLLS];
static Int16 nMenuItems = 0;
static char **menuItems = NULL;
static UInt16 menuId = AppMenu;
static Int16 nForms = 0;
static UInt16 formStack[MAX_FORMS];
static SpriteWorld *world = NULL;

DisplayType *getdisplay(void)
{
  return displayPtr;
}

void clipform(Int16 ymax)
{
  Int16 ymin;

  displayPtr = &appDisplay;
  resizedisplay();

  ymin = 15;
  if (displayPtr->highDensity) {
    ymin *= displayPtr->factorY;
    ymax *= displayPtr->factorY;
  }
  displayPtr->wh = WinGetActiveWindow();
  RctSetRectangle(&displayPtr->clip0, 0, ymin, displayPtr->width, ymax-ymin);
  RctSetRectangle(&displayPtr->clip, 0, ymin, displayPtr->width, ymax-ymin);
  hrWinEraseRectangle(&displayPtr->clip, 1);
  displayPtr->x = 0;
  displayPtr->y = 0;
  displayPtr->maxY = 0;
  displayPtr->font = stdFont;
  displayPtr->heading = 0;
  FntSetFont(displayPtr->font);
  setcolors();
  fullScreen = false;
}

void unclipform(void)
{
  Int16 ymin, ymax;

  displayPtr = &appDisplay;
  resizedisplay();

  ymin = 0;
  ymax = displayPtr->height;
  displayPtr->wh = WinGetActiveWindow();
  RctSetRectangle(&displayPtr->clip0, 0, ymin, displayPtr->width, ymax-ymin);
  RctSetRectangle(&displayPtr->clip, 0, ymin, displayPtr->width, ymax-ymin);
  displayPtr->x = 0;
  displayPtr->y = 0;
  displayPtr->maxY = 0;
  displayPtr->font = stdFont;
  displayPtr->heading = 0;
  FntSetFont(displayPtr->font);
  setcolors();
  fullScreen = true;
}

static void setclip(DisplayType *disp)
{
  FormPtr frm;
  RectangleType rect;
  Int16 dx, dy;

  displayPtr = disp;

  frm = FrmGetActiveForm();
  FrmGetFormBounds(frm, &rect);
  dx = rect.extent.x;
  dy = rect.extent.y;

  switch (WinGlueGetFrameType(FrmGetWindowHandle(frm))) {
    case noFrame:
      break;
    case simpleFrame:
    case roundFrame:		// corner = 7, frame = 1
    case popupFrame:		// corner = 2,  frame = 1, shadow = 1
      dy -= 2;
      dx -= 2;
      break;
    case simple3DFrame:		// 3d, frame = 2
    case boldRoundFrame:	// corner = 7, frame = 2
    case dialogFrame:		// corner = 3,  frame = 2
      dy -= 4;
      dx -= 4;
  }

  if (displayPtr->highDensity) {
    dx *= displayPtr->factorX;
    dy *= displayPtr->factorY;
  }

  RctSetRectangle(&displayPtr->clip0, 0, 0, dx, dy);
  RctSetRectangle(&displayPtr->clip, 0, 0, dx, dy);
  displayPtr->x = 1*displayPtr->factorX;
  displayPtr->y = (10+3)*displayPtr->factorY;
  displayPtr->maxY = 0;

  displayPtr->font = stdFont;
  displayPtr->heading = 0;
  FntSetFont(displayPtr->font);
  setcolors();

  displayPtr->width = appDisplay.width;
  displayPtr->height = appDisplay.height;
  displayPtr->depth = appDisplay.depth;
  displayPtr->enableColor = appDisplay.enableColor;

  fullScreen = true;
}

static void resizedisplay(void)
{
  if (displayPtr->highDensity) {
    UInt16 width, height;
    WinSetCoordinateSystem(displayPtr->scrCoordinate);
    WinGetDisplayExtent(&width, &height);
    WinSetCoordinateSystem(kCoordinatesStandard);
    displayPtr->width = width;
    displayPtr->height = height;
  }
}

void printstring (char *s, int n)
{
  int i, k;

  for (i = 0, k = 0; i < n && s[i]; i++) {
    switch (s[i]) {
      case '\t':
        if (i > k)
          printstr(&s[k], i-k);
        displayPtr->x += hrFntAverageCharWidth();
        k = i+1;
        break;
      case '\n':
        if (i > k)
          printstr(&s[k], i-k);
        displayPtr->x = 0;
        displayPtr->y += hrFntCharHeight();
        k = i+1;
    }
  }

  if (i > k)
    printstr(&s[k], i-k);
}

static void printstr(char *s, int n)
{
  if (validwindow()) {
    hrWinSetClip(&displayPtr->clip);
    hrWinPaintChars(s, n, xi+displayPtr->x, yi+displayPtr->y);
    WinResetClip();
  }
  displayPtr->x += hrFntCharsWidth(s, n);
}

void plua_print(char *s)
{
  if (s) printstr(s, StrLen(s));
}

static int screen_mode (lua_State *L) {
  lua_pushnumber(L, displayPtr->clip0.extent.x);
  lua_pushnumber(L, displayPtr->clip0.extent.y);
  lua_pushnumber(L, displayPtr->depth);
  lua_pushboolean(L, displayPtr->enableColor);
  return 4;
}

static int screen_clear (lua_State *L) {
  UInt32 c;

  c = luaL_opt_long(L, 1, displayPtr->bg);
  displayPtr->x = 0;
  displayPtr->y = 0;
  displayPtr->heading = 0;
  displayPtr->maxY = 0;

  if (!validwindow())
    return 0;

  setbackcolor(c);
  hrWinFillRectangle(&displayPtr->clip, 1);
  setbackcolor(displayPtr->bg);

  return 0;
}

void plua_getcolor(long *fg, long *bg)
{
  *fg = displayPtr->fg;
  *bg = displayPtr->bg;
}

void plua_pcolor(long fg, long bg)
{
  setforecolor(fg);
  settextcolor(fg);
  setbackcolor(bg);
  displayPtr->fg = fg;
  displayPtr->bg = bg;
}

static int screen_color (lua_State *L) {
  UInt32 fg, bg;

  fg = luaL_check_long(L, 1);
  bg = luaL_opt_long(L, 2, displayPtr->bg);

  plua_pcolor(fg, bg);

  return 0;
}

long plua_prgb(int r, int g, int b)
{
  RGBColorType rgb;

  rgb.r = r > 255 ? 255 : r;
  rgb.g = g > 255 ? 255 : g;
  rgb.b = b > 255 ? 255 : b;

  return displayPtr->depth < 16 ? WinRGBToIndex(&rgb) : RGBToLong(&rgb);
}

void plua_getrgb(long c, int *r, int *g, int *b)
{
  RGBColorType rgb;

  if (displayPtr->depth < 16)
    WinIndexToRGB(c, &rgb);
  else
    LongToRGB(c, &rgb);

  *r = rgb.r;
  *g = rgb.g;
  *b = rgb.b;
}

static int screen_rgb (lua_State *L) {
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

static int screen_pos (lua_State *L) {
  int x, y;
  plua_ppos(&x, &y);
  lua_pushnumber(L, x);
  lua_pushnumber(L, y);
  return 2;
}

void plua_pmoveto(int x, int y)
{
  displayPtr->x = x;
  displayPtr->y = y;
  displayPtr->maxY = 0;
}

static int screen_moveto (lua_State *L) {
  Int16 x, y;
  x = luaL_check_int(L, 1);
  y = luaL_opt_int(L, 2, displayPtr->y);
  plua_pmoveto(x, y);
  return 0;
}

static int screen_setcliping (lua_State *L) {
  Int16 x, y, dx, dy;

  if (lua_isnone(L, 1)) {
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

static void draw_line(Int16 x1, Int16 y1, Int16 x2, Int16 y2, UInt32 c)
{
  if (!validwindow())
    return;

  hrWinSetClip(&displayPtr->clip);
  setforecolor(c);
  hrWinPaintLine(xi+x1, yi+y1, xi+x2, yi+y2);
  setforecolor(displayPtr->fg);
  WinResetClip();
  displayPtr->x = x2;
  displayPtr->y = y2;
}

void plua_pline(int x1, int y1, int x2, int y2)
{
  draw_line(x1, y1, x2, y2, displayPtr->fg);
}

static int screen_line (lua_State *L) {
  Int16 x1, y1, x2, y2;
  UInt32 c;

  x1 = luaL_check_int(L, 1);
  y1 = luaL_check_int(L, 2);
  x2 = luaL_check_int(L, 3);
  y2 = luaL_check_int(L, 4);
  c = luaL_opt_long(L, 5, displayPtr->fg);
  draw_line(x1, y1, x2, y2, c);

  return 0;
}

void plua_plineto(int x, int y)
{
  draw_line(displayPtr->x, displayPtr->y, x, y, displayPtr->fg);
}

static int screen_lineto (lua_State *L) {
  Int16 x1, y1, x2, y2;
  UInt32 c;

  x1 = displayPtr->x;
  y1 = displayPtr->y;
  x2 = luaL_check_int(L, 1);
  y2 = luaL_check_int(L, 2);
  c = luaL_opt_long(L, 3, displayPtr->fg);
  draw_line(x1, y1, x2, y2, c);

  return 0;
}

static int screen_setpixel (lua_State *L) {
  UInt32 c;

  displayPtr->x = luaL_check_int(L, 1);
  displayPtr->y = luaL_check_int(L, 2);
  c = luaL_opt_long(L, 3, displayPtr->fg);

  if (!validwindow())
    return 0;

  hrWinSetClip(&displayPtr->clip);
  setforecolor(c);
  hrWinPaintPixel(xi+displayPtr->x, yi+displayPtr->y);
  setforecolor(displayPtr->fg);
  WinResetClip();
  return 0;
}

static int screen_getpixel (lua_State *L) {
  Int16 x = luaL_check_int(L, 1);
  Int16 y = luaL_check_int(L, 2);
  UInt32 p;

  p = hrWinGetPixel(xi+x, yi+y);
  lua_pushnumber(L, p);
  return 1;
}

static void draw_rect(Int16 x, Int16 y, Int16 dx, Int16 dy, Int16 filled, UInt32 c)
{
  Int16 i;

  if (!validwindow() || dx <= 0 || dy <= 0)
    return;

  x += xi;
  y += yi;

  hrWinSetClip(&displayPtr->clip);
  setforecolor(c);

  if (filled) {
    for (i = 0; i < dy; i++)
      hrWinPaintLine(x, y+i, x+dx-1, y+i);
  } else {
    hrWinPaintLine(x, y, x+dx-1, y);
    hrWinPaintLine(x+dx-1, y, x+dx-1, y+dy-1);
    hrWinPaintLine(x+dx-1, y+dy-1, x, y+dy-1);
    hrWinPaintLine(x, y+dy-1, x, y);
  }

  setforecolor(displayPtr->fg);
  WinResetClip();

  displayPtr->x += dx;
}

void plua_prect(int x, int y, int dx, int dy)
{
  draw_rect(x, y, dx, dy, 0, displayPtr->fg);
}

static int screen_rect (lua_State *L) {
  Int16 x, y, dx, dy;
  UInt32 c;

  x = luaL_check_int(L, 1);
  y = luaL_check_int(L, 2);
  dx = luaL_check_int(L, 3);
  dy = luaL_check_int(L, 4);
  c = luaL_opt_long(L, 5, displayPtr->fg);

  draw_rect(x, y, dx, dy, 0, c);
  return 0;
}

void plua_pbox(int x, int y, int dx, int dy)
{
  draw_rect(x, y, dx, dy, 1, displayPtr->fg);
}

static int screen_box (lua_State *L) {
  Int16 x, y, dx, dy;
  UInt32 c;

  x = luaL_check_int(L, 1);
  y = luaL_check_int(L, 2);
  dx = luaL_check_int(L, 3);
  dy = luaL_check_int(L, 4);
  c = luaL_opt_long(L, 5, displayPtr->fg);

  draw_rect(x, y, dx, dy, 1, c);
  return 0;
}

static void draw_circle(Int16 x, Int16 y, Int16 rx, Int16 ry, UInt32 c)
{
  Int16 i, k;
  lua_Number d;

  if (!validwindow() || rx <= 0 || ry <= 0)
    return;

  hrWinSetClip(&displayPtr->clip);
  setforecolor(c);
  x += xi;
  y += yi;

  for (i = 0; i < ry; i++) {
    d = (lua_Number)i / (lua_Number)ry;
    d = sqrt(1.0 - d * d) * (lua_Number)rx;
    k = (Int16)d;
    hrWinPaintPixel(x+k, y+i);
    hrWinPaintPixel(x-k, y+i);
    hrWinPaintPixel(x+k, y-i);
    hrWinPaintPixel(x-k, y-i);
  }
  for (i = 0; i < rx; i++) {
    d = (lua_Number)i / (lua_Number)rx;
    d = sqrt(1.0 - d * d) * (lua_Number)ry;
    k = (Int16)d;
    hrWinPaintPixel(x+i, y+k);
    hrWinPaintPixel(x+i, y-k);
    hrWinPaintPixel(x-i, y+k);
    hrWinPaintPixel(x-i, y-k);
  }

  setforecolor(displayPtr->fg);
  WinResetClip();
}

void plua_pcircle(int x, int y, int rx, int ry)
{
  draw_circle(x, y, rx, ry, displayPtr->fg);
}

static int screen_circle (lua_State *L) {
  Int16 x, y, rx, ry;
  UInt32 c;

  x = xi+luaL_check_int(L, 1);
  y = yi+luaL_check_int(L, 2);
  rx = luaL_check_int(L, 3);
  ry = luaL_check_int(L, 4);
  c = luaL_opt_long(L, 5, displayPtr->fg);

  draw_circle(x, y, rx, ry, c);

  return 0;
}

static void draw_disc(Int16 x, Int16 y, Int16 rx, Int16 ry, UInt32 c)
{
  Int16 i, k;
  lua_Number d;

  if (!validwindow() || rx <= 0 || ry <= 0)
    return;

  hrWinSetClip(&displayPtr->clip);
  setforecolor(c);

  for (i = 0; i < ry; i++) {
    d = (lua_Number)i / (lua_Number)ry;
    d = sqrt(1.0 - d * d) * (lua_Number)rx;
    k = (Int16)d;
    hrWinPaintLine(x, y+i, x+k, y+i);
    hrWinPaintLine(x, y+i, x-k, y+i);
    hrWinPaintLine(x, y-i, x+k, y-i);
    hrWinPaintLine(x, y-i, x-k, y-i);
  }

  setforecolor(displayPtr->fg);
  WinResetClip();
}

void plua_pdisc(int x, int y, int rx, int ry)
{
  draw_disc(x, y, rx, ry, displayPtr->fg);
}

static int screen_disc(lua_State *L) {
  Int16 x, y, rx, ry;
  UInt32 c;

  x = xi+luaL_check_int(L, 1);
  y = yi+luaL_check_int(L, 2);
  rx = luaL_check_int(L, 3);
  ry = luaL_check_int(L, 4);
  c = luaL_opt_long(L, 5, displayPtr->fg);

  draw_disc(x, y, rx, ry, c);

  return 0;
}

static int screen_fill(lua_State *L) {
  Int16 x, y;
  UInt32 c;

  x = xi+luaL_check_int(L, 1);
  y = yi+luaL_check_int(L, 2);
  c = luaL_opt_long(L, 3, displayPtr->fg);

  if (!validwindow())
    return 0;

  hrWinSetClip(&displayPtr->clip);
  SeedFill(x, y, c);
  WinResetClip();

  return 0;
}

static int screen_font (lua_State *L) {
  UInt16 fontId, rsrc;

  fontId = luaL_check_int(L, 1);

  if (fontId >= 256)
    return 0;

  if (fontId >= 128 && fontId < 255) {
    rsrc = fontId-MIN_RESOURCE;
    if (!resourcePool[rsrc] || (resourcePool[rsrc]->type != 'NFNT' && resourcePool[rsrc]->type != 'nfnt'))
      return 0;
    if (FntDefineFont(fontId, resourcePool[rsrc]->p) != 0)
      return 0;
  }

  displayPtr->font = fontId;
  FntSetFont(displayPtr->font);
  lua_pushnumber(L, hrFntCharWidth('a'));
  lua_pushnumber(L, hrFntCharHeight());
  return 2;
}

static void sync(void)
{
  EventType event;
  while (EvtEventAvail()) ProcessEvent(&event, 0, false);
}

static int gui_dialog (lua_State *L) {
  UInt16 id, current;
  Int16 x, y, w, h;
  FormPtr frm;
  WinHandle wh;
  RectangleType rect;
  char *title;

  if (!fullScreen || nForms == MAX_FORMS)
    return 0;

  x = luaL_check_int(L, 1) / displayPtr->factorX;
  y = luaL_check_int(L, 2) / displayPtr->factorY;
  w = luaL_check_int(L, 3) / displayPtr->factorX;
  h = luaL_check_int(L, 4) / displayPtr->factorY;
  title = (char *)luaL_check_string(L, 5);
  dialogDisplay.movable = lua_gettop(L) > 5 ? (Boolean)lua_toboolean(L, 6) : false;

  if (x < 0 || y < 0 || w <= 8 || h <= 16)
    return 0;

  x += 2; // desconta frame
  y += 2;

  current = FrmGetActiveFormID();
  id = DialogForm;

  formStack[nForms++] = current;
  rect.topLeft.x = x;
  rect.topLeft.y = y;
  rect.extent.x = w;
  rect.extent.y = h;

  frm = FrmInitForm(id);
  FrmSetTitle(frm, title);
  FrmSetActiveForm(frm);
  SetEventHandler(frm, id);
  wh = FrmGetWindowHandle(frm);
  WinSetBounds(wh, &rect);
  WinSetActiveWindow(wh);
  WinSetDrawWindow(wh);
  FrmDrawForm(frm);

  dialogDisplay.wh = wh;
  nControlsDialog = nControls;

  setclip(&dialogDisplay);
  sync();

  lua_pushnumber(L, id);
  return 1;
}

static int sprite_init (lua_State *L) {
  Int16 i, x, y;
  WinHandle wh;

  if (!fullScreen || romNumber < 50)
    return pusherror(L, EINVAL);

  i = luaL_check_int(L, 1);

  if (i >= NUM_HANDLE || !handlePool[i])
    return pusherror(L, EINVAL);

  wh = handlePool[i];
  x = luaL_opt_int(L, 2, 0);
  y = luaL_opt_int(L, 3, 0);

  if (world)
    SpriteBackground(world, wh);
  else if ((world = SpriteInit(L, wh, displayPtr->wh, x, y)) == NULL)
    return pusherror(L, EINVAL);

  lua_pushboolean(L, 1);
  return 1;
}

static int sprite_finish (lua_State *L) {
  if (!fullScreen || romNumber < 50)
    return pusherror(L, EINVAL);

  if (world) {
    SpriteFinish(L, world);
    world = NULL;
  }

  lua_pushboolean(L, 1);
  return 1;
}

static int sprite_add (lua_State *L) {
  Int16 index = luaL_check_int(L, 1);

  if (!fullScreen || !world || index < 1 || index > MAX_SPRITES ||
      romNumber < 50)
    return pusherror(L, EINVAL);

  if (!lua_istable(L, 2))
    return pusherror(L, EINVAL);

  if (SpriteInsertObject(L, 2, world, index-1) != 0)
    return pusherror(L, errno);

  lua_pushboolean(L, 1);
  return 1;
}

static int sprite_remove (lua_State *L) {
  Int16 index = luaL_check_int(L, 1);

  if (!fullScreen || !world || index < 1 || index > MAX_SPRITES ||
      romNumber < 50)
    return pusherror(L, EINVAL);

  SpriteDeleteObject(L, world, index-1);

  lua_pushboolean(L, 1);
  return 1;
}

static int sprite_update (lua_State *L) {
  if (!fullScreen || !world || romNumber < 50)
    return pusherror(L, EINVAL);

  if (validwindow()) {
    SpriteUpdate(L, world);
    SpriteDraw(world);
  }

  lua_pushboolean(L, 1);
  return 1;
}

BitmapPtr rsrc2bmp(Int16 rsrc)
{
  if (rsrc < MIN_RESOURCE || rsrc >= (MIN_RESOURCE+NUM_RESOURCE))
    return NULL;
  
  return (BitmapPtr)resourcePool[rsrc - MIN_RESOURCE]->p;
}

static int resource_open (lua_State *L) {
  MemHandle h = NULL;
  MemPtr p = NULL;
  UInt32 type;
  UInt16 id, rsrc;
  char *name;
  BitmapPtr bmp;

  type = StringToCreator((char *)luaL_check_string(L, 1));
  id = luaL_check_int(L, 2);
  name = (lua_isnone(L, 3) || lua_isnil(L, 3)) ? NULL :
              (char *)luaL_check_string(L, 3);

  for (rsrc = 0; rsrc < NUM_RESOURCE; rsrc++)
    if (!resourcePool[rsrc])
      break;

  if (rsrc == NUM_RESOURCE)
    return 0;

  if (name) {
    Err err;
    DmOpenRef dbRef = DbOpenByName(name, dmModeReadOnly, &err);
    if (dbRef) {
      h = DmGetResource(type, id);
      DbClose(dbRef);
    }
  } else
    h = DmGetResource(type, id);

  if (h == NULL)
    return 0;

  if ((p = MemHandleLock(h)) == NULL) {
    DmReleaseResource(h);
    return 0;
  }

  if ((resourcePool[rsrc] = malloc(sizeof(ResourceType))) == NULL) {
    MemHandleUnlock(h);
    DmReleaseResource(h);
  }

  resourcePool[rsrc]->type = type;
  resourcePool[rsrc]->h = h;
  resourcePool[rsrc]->p = p;
  resourcePool[rsrc]->size = MemPtrSize(resourcePool[rsrc]->p);

  if (displayPtr->highDensity && (type == bitmapRsc || type == iconType)) {
    bmp = p;
    while (bmp && BmpGetDensity(bmp) != kDensityDouble)
      bmp = BmpGetNextBitmapAnyDensity(bmp);
    if (bmp && BmpGetDensity(bmp) == kDensityDouble)
      resourcePool[rsrc]->p = bmp;
  }

  lua_pushnumber(L, MIN_RESOURCE+rsrc);
  return 1;
}

ResourceType *resource_valid(lua_State *L) {
  int rsrc;

  rsrc = luaL_check_int(L, 1);
  if (rsrc < MIN_RESOURCE || rsrc >= (MIN_RESOURCE+NUM_RESOURCE))
    return NULL;

  rsrc -= MIN_RESOURCE;

  return resourcePool[rsrc];
}

static int resource_close (lua_State *L) {
  int rsrc;

  rsrc = luaL_check_int(L, 1);
  if (rsrc < MIN_RESOURCE || rsrc >= (MIN_RESOURCE+NUM_RESOURCE))
    return 0;

  rsrc -= MIN_RESOURCE;

  if (!resourcePool[rsrc])
    return 0;

  MemHandleUnlock(resourcePool[rsrc]->h);
  DmReleaseResource(resourcePool[rsrc]->h);
  free(resourcePool[rsrc]);
  resourcePool[rsrc] = NULL;

  return 0;
}

static int resource_draw (lua_State *L) {
  ResourceType *rsrc;
  UInt16 mode, old, width, height, rowBytes;
  UInt32 value;
  BitmapPtr bmp;
  RectangleType rect;
  WinHandle wh;
  FILE f;

  if ((rsrc = resource_valid(L)) == NULL)
    return 0;

  if (!validwindow())
    return 0;

  mode = luaL_opt_int(L, 2, winPaint);
  if (mode > winPaintInverse)
    mode = winPaint;

  width = height = 0;
  hrWinSetClip(&displayPtr->clip);

  switch (rsrc->type) {
    case bitmapRsc:
    case iconType:
    case 'abmp':
      bmp = (BitmapPtr)rsrc->p;
      hrBmpGlueGetDimensions(bmp, &width, &height, &rowBytes);

      if (BmpGlueGetTransparentValue(bmp, &value))
        mode = winPaint;

      old = WinSetDrawMode(mode);
      hrWinPaintBitmap(bmp, xi+displayPtr->x, yi+displayPtr->y);
      WinSetDrawMode(old);
      break;

    case 'Wbmp':
    case 'JPEG':
      f.type = FILE_RESOURCE;
      f.recordPos = 0;
      f.recordSize = rsrc->size;
      f.record = rsrc->p;

      if ((wh = BmpRead(&f, &width, &height)) != NULL) {
        rect.topLeft.x = 0;
        rect.topLeft.y = 0;
        rect.extent.x = width;
        rect.extent.y = height;

        if (displayPtr->highDensity) WinSetCoordinateSystem(displayPtr->scrCoordinate);
        WinCopyRectangle(wh, displayPtr->wh, &rect,
           xi+displayPtr->x, yi+displayPtr->y, mode);
        if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);

        WinDeleteWindow(wh, false);
      }
  }

  WinResetClip();

  displayPtr->x += width;
  if (height > displayPtr->maxY)
    displayPtr->maxY = height;

  return 0;
}

void *plua_getwindow(void) {
  return (void *)displayPtr->wh;
}

static int buffer_read (lua_State *L) {
  UInt16 i, width, height, rowBytes;
  Int16 err;
  char *name;
  FILE *f;
  WinHandle old;
  BitmapPtr bmp;

  if (!fullScreen)
    return 0;

  for (i = 0; i < NUM_HANDLE; i++)
    if (!handlePool[i])
      break;

  if (i == NUM_HANDLE)
    return 0;

  name = (char *)luaL_check_string(L, 1);

  if ((f = fopen(name, "r")) == NULL)
    return pusherror(L, errno);

  width = displayPtr->width;
  height = displayPtr->height;

  if (f->type == FILE_RESOURCE &&
      (f->resType == bitmapRsc || f->resType == iconType ||
       f->resType == 'abmp')) {

    bmp = (BitmapPtr)f->record;
    hrBmpGlueGetDimensions(bmp, &width, &height, &rowBytes);

    if (displayPtr->highDensity) WinSetCoordinateSystem(displayPtr->scrCoordinate);
    handlePool[i] = WinCreateOffscreenWindow(width, height, nativeFormat, &err);

    if (handlePool[i]) {
      old = WinSetDrawWindow(handlePool[i]);
      WinPaintBitmap(bmp, 0, 0);
      WinSetDrawWindow(old);
    } else
      errno = ENOMEM;

    if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);

  } else
    handlePool[i] = BmpRead(f, &width, &height);

  if (handlePool[i] == NULL) {
    err = errno;
    fclose(f);
    return pusherror(L, err);
  }

  fclose(f);
  lua_pushnumber(L, i);
  lua_pushnumber(L, width);
  lua_pushnumber(L, height);

  return 3;
}

static int buffer_write (lua_State *L) {
  Int16 i, err;
  UInt32 size;
  char *name;
  WinHandle wh;
  FILE *f;

  if (!fullScreen)
    return 0;

  name = (char *)luaL_check_string(L, 1);
  i = luaL_opt_int(L, 2, -1);

  if ((f = fopen(name, "w")) == NULL)
    return pusherror(L, errno);

  if (i < 0)
    wh = displayPtr->wh;
  else {
    if (i >= NUM_HANDLE || !handlePool[i])
      return 0;
    wh = handlePool[i];
  }

  if (BmpWrite(f, wh, &size) != 0) {
    err = errno;
    fclose(f);
    return pusherror(L, err);
  }
  fclose(f);

  lua_pushnumber(L, size);
  return 1;
}

static int buffer_new (lua_State *L) {
  Int16 i, dx, dy;
  Err err;

  if (!fullScreen)
    return 0;

  dx = luaL_check_int(L, 1);
  dy = luaL_check_int(L, 2);

  for (i = 0; i < NUM_HANDLE; i++)
    if (!handlePool[i])
      break;

  if (i == NUM_HANDLE)
    return 0;

  if (displayPtr->highDensity) WinSetCoordinateSystem(displayPtr->scrCoordinate);
  handlePool[i] = WinCreateOffscreenWindow(dx, dy, nativeFormat, &err);
  if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);

  if (handlePool[i] == NULL)
    return 0;

  lua_pushnumber(L, i);
  return 1;
}

static int buffer_get (lua_State *L) {
  Int16 i, x, y, dx, dy;
  RectangleType rect;
  Err err;

  if (!fullScreen)
    return 0;

  x = xi+luaL_check_int(L, 1);
  y = yi+luaL_check_int(L, 2);
  dx = luaL_check_int(L, 3);
  dy = luaL_check_int(L, 4);
  RctSetRectangle(&rect, x, y, dx, dy);

  for (i = 0; i < NUM_HANDLE; i++)
    if (!handlePool[i])
      break;
    
  if (i == NUM_HANDLE)
    return 0;

  if (displayPtr->highDensity) WinSetCoordinateSystem(displayPtr->scrCoordinate);
  handlePool[i] = WinCreateOffscreenWindow(dx, dy, nativeFormat, &err);

  if (handlePool[i] == NULL) {
    if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
    return 0;
  }

  WinCopyRectangle(displayPtr->wh, handlePool[i], &rect, 0, 0, winPaint);
  if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);

  lua_pushnumber(L, i);
  return 1;
}

static int buffer_put (lua_State *L) {
  RectangleType rect;
  UInt16 i, mode;
  Int16 x, y;
  WinHandle old;

  if (!fullScreen)
    return 0;

  i = luaL_check_int(L, 1);
  x = xi+luaL_check_int(L, 2);
  y = yi+luaL_check_int(L, 3);
  mode = luaL_opt_int(L, 4, winPaint);
  if (mode > winPaintInverse)
    mode = winPaint;

  if (i >= NUM_HANDLE || !handlePool[i])
    return 0;

  if (!validwindow())
    return 0;

  hrWinSetClip(&displayPtr->clip);
  if (displayPtr->highDensity) WinSetCoordinateSystem(displayPtr->scrCoordinate);

  old = WinSetDrawWindow(handlePool[i]);
  WinGetDrawWindowBounds(&rect);
  WinSetDrawWindow(old);

  WinCopyRectangle(handlePool[i], displayPtr->wh, &rect, x, y, mode);
  if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  WinResetClip();
  return 0;
}

static int buffer_free (lua_State *L) {
  UInt16 i;

  if (!fullScreen)
    return 0;

  i = luaL_check_int(L, 1);

  if (i >= NUM_HANDLE || !handlePool[i])
    return 0;

  if (displayPtr->wh == handlePool[i]) {
    displayPtr = &appDisplay;
    WinSetDrawWindow(displayPtr->wh);
  }

  WinDeleteWindow(handlePool[i], false);
  handlePool[i] = NULL;

  return 0;
}

static int buffer_use (lua_State *L) {
  Int16 i;
  WinHandle old;

  if (!fullScreen || !validwindow())
    return 0;

  i = luaL_opt_int(L, 1, -1);

  if (i == -1) {
    displayPtr = nForms > 0 ? &dialogDisplay : &appDisplay;
    WinSetDrawWindow(displayPtr->wh);
    setforecolor(displayPtr->fg);
    settextcolor(displayPtr->fg);
    setbackcolor(displayPtr->bg);
    FntSetFont(displayPtr->font);
    return 0;
  }

  if (i >= NUM_HANDLE || !handlePool[i])
    return 0;

  displayPtr = &bufDisplay;

  if (displayPtr->highDensity) WinSetCoordinateSystem(displayPtr->scrCoordinate);

  old = WinSetDrawWindow(handlePool[i]);
  WinGetDrawWindowBounds(&displayPtr->clip0);
  WinGetDrawWindowBounds(&displayPtr->clip);
  WinSetDrawWindow(old);

  if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);

  displayPtr->wh = handlePool[i];
  displayPtr->width = displayPtr->clip.extent.x;
  displayPtr->height = displayPtr->clip.extent.x;
  displayPtr->depth = appDisplay.depth;
  displayPtr->enableColor = appDisplay.enableColor;
  displayPtr->x = 0;
  displayPtr->y = 0;
  displayPtr->maxY = 0;
  displayPtr->font = stdFont;
  displayPtr->heading = 0;
  setcolors();

  WinSetDrawWindow(displayPtr->wh);

  return 0;
}

static int sound_beep (lua_State *L) {
  SndPlaySystemSound(luaL_check_int(L, 1));
  return 0;
}

static int sound_tone (lua_State *L) {
  SndCommandType cmd;

  cmd.cmd = sndCmdFreqDurationAmp;
  cmd.param1 = luaL_check_int(L, 1);
  cmd.param2 = luaL_check_int(L, 2);
  cmd.param3 = lua_isnone(L, 3) ? sndDefaultAmp : luaL_check_int(L, 3);

  SndDoCmd(NULL, &cmd, true);
  return 0;
}

static int sound_midi (lua_State *L) {
  char *name;
  Int32 amp;

  name = (char *)luaL_check_string(L, 1);
  amp = luaL_opt_int(L, 2, -1);

  if (amp < 0 || amp > sndMaxAmp)
    amp = sndGameVolume;

  if (MidiPlay(name, amp) != 0)
    return pusherror(L, errno);

  lua_pushboolean(L, 1);
  return 1;
}

static int sound_play (lua_State *L) {
  char *name;
  Int32 amp, duration;
  UInt16 slot;

  name = (char *)luaL_check_string(L, 1);
  slot = luaL_opt_int(L, 2, 1);
  amp = luaL_opt_int(L, 3, -1);

  if (amp < 0 || amp > sndMaxAmp)
    amp = sndGameVolume;

  if ((duration = SoundPlay(slot-1, name, amp)) < 0)
    return pusherror(L, errno);

  // converte duracao para segundos
  lua_pushnumber(L, ((lua_Number)duration) / 100.0);
  return 1;
}

static int sound_stop (lua_State *L) {
  UInt16 slot;

  slot = luaL_opt_int(L, 1, 1);
  SoundStop(slot-1, true);

  return 0;
}

static int gui_event (lua_State *L) {
  EventType event;
  Int16 x, y;
  UInt16 id, min, max;
  UInt32 tps, wait, t;
  Int32 arg;
  Boolean forever, hasHandler;

  tps = SysTicksPerSecond();
  wait = tps / 4;
  forever = true;
  t = clock();
  arg = 0;

  if (lua_gettop(L) >= 1 && !lua_isnil(L, 1)) {
    arg = luaL_check_long(L, 1);
    if (arg >= 0) {
      forever = false;
      arg = (arg * tps) / 1000; // converte em ticks
      t += arg;
      if (wait > arg)
        wait = arg;
    }
  }

  while (true) {
    if (!ProcessEvent(&event, wait, true))
      continue;

    switch (event.eType) {
      case appStopEvent:
        lua_pushnumber(L, appStopEvent);
        return 1;

      case nilEvent:
        if (iocheck() > 0) {
          hasHandler = gethandler(L, SYSTEM_BASE + ioPendingEvent);
          lua_pushnumber(L, ioPendingEvent);
          if (!hasHandler) return 1;
          lua_call(L, 1, 0);
          t += arg;
          continue;
        }

        if (forever) continue;
        if (clock() < t) continue;

        hasHandler = gethandler(L, SYSTEM_BASE + event.eType);
        lua_pushnumber(L, event.eType);
        if (!hasHandler) return 1;
        lua_call(L, 1, 0);
        t += arg;
        continue;

      case ctlSelectEvent:
        id = event.data.ctlSelect.controlID;
        if (id < FirstInternID || id > LastInternID)
          continue;

        hasHandler = gethandler(L, CONTROL_BASE + id - FirstInternID);
        lua_pushnumber(L, event.eType);
        lua_pushnumber(L, id);

        min = max = 0;
        CtlGetSliderValues(event.data.ctlSelect.pControl,
          &min, &max, NULL, NULL);

        if (min == max)
          // controle nao e um slider
          lua_pushnumber(L, event.data.ctlSelect.on ? 1 : 0);
        else
          // controle e um slider
          lua_pushnumber(L, event.data.ctlSelect.value+1);

        if (!hasHandler) return 3;
        lua_call(L, 3, 0);
        t += arg;
        continue;

      case ctlRepeatEvent:
        id = event.data.ctlRepeat.controlID;
        if (id < FirstInternID || id > LastInternID)
          continue;

        hasHandler = gethandler(L, CONTROL_BASE + id - FirstInternID);
        lua_pushnumber(L, event.eType);
        lua_pushnumber(L, id);

        min = max = 0;
        CtlGetSliderValues(event.data.ctlRepeat.pControl,
          &min, &max, NULL, NULL);

        if (min == max) {
          // controle e um repeating button
          if (!hasHandler) return 2;
          lua_call(L, 2, 0);
          t += arg;
          continue;
        }

        // controle e um feedback slider
        lua_pushnumber(L, event.data.ctlRepeat.value+1);

        if (!hasHandler) return 3;
        lua_call(L, 3, 0);
        t += arg;
        continue;

      case lstSelectEvent:
        id = event.data.lstSelect.listID;
        if (id < FirstInternID || id > LastInternID)
          continue;

        hasHandler = gethandler(L, CONTROL_BASE + id - FirstInternID);
        lua_pushnumber(L, event.eType);
        lua_pushnumber(L, id);
        lua_pushnumber(L, event.data.lstSelect.selection+1);

        if (!hasHandler) return 3;
        lua_call(L, 3, 0);
        t += arg;
        continue;

      case popSelectEvent:
        id = event.data.popSelect.listID;
        if (id < FirstInternID || id > LastInternID)
          continue;

        hasHandler = gethandler(L, CONTROL_BASE + id - FirstInternID);
        lua_pushnumber(L, event.eType);
        lua_pushnumber(L, id);
        lua_pushnumber(L, event.data.popSelect.selection+1);

        if (!hasHandler) return 3;
        lua_call(L, 3, 0);
        t += arg;
        continue;

      case keyDownEvent:
      case fakeKeyDownEvent:
        hasHandler = gethandler(L, SYSTEM_BASE + keyDownEvent);
        lua_pushnumber(L, keyDownEvent);
        lua_pushnumber(L, event.data.keyDown.chr);

        if (!hasHandler) return 2;
        lua_call(L, 2, 0);
        t += arg;
        continue;

      case penUpEvent:
      case penDownEvent:
      case penMoveEvent:
        hasHandler = gethandler(L, SYSTEM_BASE + event.eType);
        x = (event.screenX - xi)*displayPtr->factorX;
        y = (event.screenY - yi)*displayPtr->factorY;
        lua_pushnumber(L, event.eType);
        lua_pushnumber(L, x);
        lua_pushnumber(L, y);

        if (!hasHandler) return 3;
        lua_call(L, 3, 0);
        t += arg;
        continue;

      case menuEvent:
        id = event.data.menu.itemID;
        if (menuId == AppMenu) {
          if (id < FirstCmd || id > LastCmd)
            continue;
        }

        hasHandler = gethandler(L, SYSTEM_BASE + event.eType);
        lua_pushnumber(L, event.eType);
        lua_pushnumber(L, menuId == AppMenu ? id-FirstCmd+1 : id);

        if (!hasHandler) return 2;
        lua_call(L, 2, 0);
        t += arg;
        continue;

      case sampleStopEvent:
        hasHandler = gethandler(L, SYSTEM_BASE + event.eType);
        lua_pushnumber(L, event.eType);
        lua_pushnumber(L, event.screenX+1);

        if (!hasHandler) return 2;
        lua_call(L, 2, 0);
        t += arg;
        continue;

      default:
        continue;
    }
  }
  return 0;
}

static Boolean GadgetHandler(FormGadgetTypeInCallback *gad, UInt16 cmd, void *p)
{
  UInt16 i, n, index, id;
  FormPtr frm;
  GadgetData *gdata;

  if (cmd == formGadgetDeleteCmd)
    return true;

  frm = FrmGetActiveForm();

  if (romNumber < 40) {
    index = frmInvalidObjectId;
    n = FrmGetNumberOfObjects(frm);
    for (i = 0; i < n; i++)
      if (FrmGetObjectPtr(frm, i) == gad) {
        index = i;
        break;
      }
  } else
    index = FrmGetObjectIndexFromPtr(frm, gad);

  if (index == frmInvalidObjectId)
    return true;

  id = FrmGetObjectId(frm, index);
  gdata = FrmGetGadgetData(frm, index);

  switch (cmd) {
    case formGadgetDrawCmd:
      gdata->callback(gdata->context, gadgetDraw, id, gdata->data, NULL,
         &gdata->rect);
      break;
    case formGadgetEraseCmd:
      gdata->callback(gdata->context, gadgetErase, id, gdata->data, NULL,
         &gdata->rect);
      break;
    case formGadgetHandleEventCmd:
      break;
  }

  return true;
}

int plua_creategadget(void *context, int width, int height, void *data, GadgetCallback callback)
{
  Int16 x, y;
  UInt16 id, index;
  FormPtr frm;
  FormGadgetType *gad;
  GadgetData *gdata;

  if (nGadgets == NUM_GADGETS)
    return -1;

  if (displayPtr->wh != WinGetDrawWindow() ||
      (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh) ||
      !fullScreen)
    return -1;

  if ((gdata = malloc(sizeof(GadgetData))) == NULL)
    return -1;

  x = displayPtr->x;
  y = displayPtr->y;
  id = nextID();
  frm = FrmGetActiveForm();

  if (displayPtr->highDensity) WinSetCoordinateSystem(displayPtr->scrCoordinate);
  gad = FrmNewGadget(&frm, id, x, y, width, height);
  if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);

  FrmSetActiveForm(frm);
  SetEventHandler(frm, FrmGetActiveFormID());
  displayPtr->wh = FrmGetWindowHandle(frm);
  WinSetActiveWindow(displayPtr->wh);

  if (!gad) {
    previousID();
    free(gdata);
    return -1;
  }

  gdata->context = context;
  gdata->id = id;
  gdata->data = data;
  gdata->callback = callback;
  gdata->rect.topLeft.x = x;
  gdata->rect.topLeft.y = y;
  gdata->rect.extent.x = width;
  gdata->rect.extent.y = height;

  gadgetPool[nGadgets++] = gdata;

  index = FrmGetObjectIndex(frm, id);
  FrmSetGadgetData(frm, index, gdata);
  FrmSetGadgetHandler(frm, index, GadgetHandler);

  gdata->callback(context, gadgetDraw, id, data, NULL, &gdata->rect);

  if (height > displayPtr->maxY)
    displayPtr->maxY = height;
  displayPtr->x += width + displayPtr->factorX;

  return id;
}

Boolean gadget_event(EventPtr event)
{
  Int16 i, x, y;
  GadgetData *gdata;
  PointType point;
  Boolean handled = false;

  x = (event->screenX - xi)*displayPtr->factorX;
  y = (event->screenY - yi)*displayPtr->factorY;

  switch (event->eType) {
    case penDownEvent:
      if (inGadget >= 0) {
        gdata = gadgetPool[inGadget];
        gdata->callback(gdata->context, gadgetUp, gdata->id, gdata->data, NULL, &gdata->rect);
        inGadget = -1;
      }

      for (i = nGadgets-1; i >= 0; i--) {
        gdata = gadgetPool[i];
        if (RctPtInRectangle(x, y, &gdata->rect)) {
          point.x = x - gdata->rect.topLeft.x;
          point.y = y - gdata->rect.topLeft.y;
          gdata->callback(gdata->context, gadgetDown, gdata->id, gdata->data, &point, &gdata->rect);
          inGadget = i;
          break;
        }
      }

      if (inGadget >= 0)
        handled = true;
      break;
    case penUpEvent:
      if (inGadget >= 0) {
        gdata = gadgetPool[inGadget];
        gdata->callback(gdata->context, gadgetUp, gdata->id, gdata->data, NULL, &gdata->rect);
        inGadget = -1;
        handled = true;
      }
      break;
    case penMoveEvent:
      if (inGadget >= 0) {
        gdata = gadgetPool[inGadget];
        if (RctPtInRectangle(x, y, &gdata->rect)) {
          point.x = x - gdata->rect.topLeft.x;
          point.y = y - gdata->rect.topLeft.y;
          gdata->callback(gdata->context, gadgetMove, gdata->id, gdata->data, &point, &gdata->rect);
        }
        handled = true;
      }
  }

  return handled;
}

UInt16 create_control(Int16 style, char *name, UInt16 resId, char *file,
                      UInt8 group, Int16 state,
                      Int16 *px, Int16 *py, Int16 *pwidth, Int16 *pheight,
                      Int16 *pfont, Boolean leftAnchor, Boolean visible)
{
  Int16 dx, dy, x, y, lx, ly, dw, dh, width, height, font, dv;
  UInt16 id, rowBytes, bw, bh, index;
  ControlType *ctl;
  FormPtr frm;
  MemHandle h;
  BitmapPtr p;
  DmOpenRef dbRef;
  Err err;

  if (displayPtr->wh != WinGetDrawWindow() ||
      (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh) ||
      !fullScreen)
    return 0;

  dv = visible ? 0 : displayPtr->height;

  x = (px && *px >= 0) ? *px : displayPtr->x;
  y = (py && *py >= 0) ? *py : displayPtr->y;
  font = (pfont && *pfont != -1) ? *pfont : displayPtr->font;
  width = 0;
  height = 0;
  ctl = NULL;

  id = nextID();
  frm = FrmGetActiveForm();
  FntSetFont(font);

  switch (style) {
    case buttonCtl:
      dx = 5;
      dy = 2;
      dw = 0;
      dh = 1;
      break;
    case repeatingButtonCtl:
      dx = 0;
      dy = 0;
      dw = 0;
      dh = 0;
      break;
    case selectorTriggerCtl:
      dx = 5;
      dy = 2;
      dw = 0;
      dh = 1;
      break;
    case pushButtonCtl:
      dx = 1;
      dy = 2;
      dw = 0;
      dh = 1;
      break;
    case checkboxCtl:
      dx = 4;
      dy = 0;
      dw = 2*hrFntCharWidth('w');
      dh = 2;
      break;
    default:
      FntSetFont(displayPtr->font);
      return 0;
  }

  if (resId) {
    dbRef = file ? DbOpenByName(file, dmModeReadOnly, &err) : NULL;

    h = DmGetResource(bitmapRsc, resId);
    if (!h) h = DmGetResource(iconType, resId);
    if (!h) h = DmGetResource('abmp', resId);

    if (h) {
      p = MemHandleLock(h);
      if (p) {
        BmpGlueGetDimensions(p, &bw, &bh, &rowBytes);
        MemHandleUnlock(h);
        DmReleaseResource(h);

        if (displayPtr->highDensity) {
          lx = (xi+x+displayPtr->factorX) / displayPtr->factorX;
          ly = (yi+y+dh*displayPtr->factorY) / displayPtr->factorY;
        } else {
          lx = xi+x+1;
          ly = yi+y+dh;
        }
        width = (pwidth && *pwidth >= 0) ? *pwidth : bw + 4;
        height = (pheight && *pheight >= 0) ? *pheight : bh + 4;

        ctl = (ControlType *)CtlNewGraphicControl((void **)&frm, id, style,
                 resId, 0, lx, ly+dv, width, height, group, leftAnchor);
        if (ctl && dv) {
          index = FrmGetObjectIndex(frm, id);
          FrmHideObject(frm, index);
          FrmSetObjectPosition(frm, index, lx, ly);
        }
      } else
        DmReleaseResource(h);
    }

    if (dbRef) DbClose(dbRef);
  }

  if (!ctl) {
    width = (pwidth && *pwidth >= 0) ? *pwidth :
             hrFntCharWidth('w') + hrFntCharsWidth(name, StrLen(name)) + dw;
    height = (pheight && *pheight >= 0) ? *pheight : hrFntCharHeight() + 2;

    ctl = hrCtlNewControl((void **)&frm, id, style, name,
              xi+x+1, yi+y+dh+dv, width, height, font, group, leftAnchor);
    if (ctl && dv) {
      index = FrmGetObjectIndex(frm, id);
      FrmHideObject(frm, index);
      FrmSetObjectPosition(frm, index, xi+x+1, yi+y+dh);
    }
  }

  FrmSetActiveForm(frm);
  SetEventHandler(frm, FrmGetActiveFormID());

  displayPtr->wh = FrmGetWindowHandle(frm);
  WinSetActiveWindow(displayPtr->wh);
  FntSetFont(displayPtr->font);

  if (ctl) {
    RectangleType rect;
    CtlSetValue(ctl, state);
    hrWinSetClip(&displayPtr->clip0);
    CtlDrawControl(ctl);
    WinResetClip();
    hrFrmGetObjectBounds(frm, FrmGetObjectIndex(frm, id), &rect);
    if (rect.extent.y + dy*displayPtr->factorY > displayPtr->maxY)
      displayPtr->maxY = rect.extent.y + dy*displayPtr->factorY;
    displayPtr->x += rect.extent.x + dx*displayPtr->factorX;

    if (px) *px = x;   
    if (py) *py = y;   
    if (pwidth) *pwidth = width;   
    if (pheight) *pheight = height;   
    if (pfont) *pfont = font;   

    return id;
  }

  previousID();
  return 0;
}

static int gui_button (lua_State *L) {
  char *name, *file;
  UInt16 resId, id;

  name = (char *)luaL_check_string(L, 1);
  resId = lua_gettop(L) > 1 ? luaL_check_int(L, 2) : 0;
  file = lua_gettop(L) > 2 ? (char *)luaL_check_string(L, 3) : NULL;

  if ((id = create_control(buttonCtl, name, resId, file, 0, 0,
                           NULL, NULL, NULL, NULL, NULL, true, true)) == 0)
    return 0;

  lua_pushnumber(L, id);
  return 1;
}

static int gui_pushbutton (lua_State *L) {
  char *name, *file;
  UInt8 group;
  UInt16 resId, id;

  name = (char *)luaL_check_string(L, 1);
  group = lua_gettop(L) > 1 ? luaL_check_int(L, 2) : 0;
  resId = lua_gettop(L) > 2 ? luaL_check_int(L, 3) : 0;
  file = lua_gettop(L) > 3 ? (char *)luaL_check_string(L, 4) : NULL;

  if ((id = create_control(pushButtonCtl, name, resId, file, group, 0,
                           NULL, NULL, NULL, NULL, NULL, true, true)) == 0)
    return 0;

  lua_pushnumber(L, id);
  return 1;
}

static int gui_repeatbutton (lua_State *L) {
  char *name, *file;
  UInt16 resId, id;

  name = (char *)luaL_check_string(L, 1);
  resId = lua_gettop(L) > 1 ? luaL_check_int(L, 2) : 0;
  file = lua_gettop(L) > 2 ? (char *)luaL_check_string(L, 3) : NULL;

  if ((id = create_control(repeatingButtonCtl, name, resId, file, 0, 0,
                           NULL, NULL, NULL, NULL, NULL, true, true)) == 0)
    return 0;

  lua_pushnumber(L, id);
  return 1;
}

static int gui_selectortrigger (lua_State *L) {
  char *name, *file;
  UInt16 resId, id;

  name = (char *)luaL_check_string(L, 1);
  resId = lua_gettop(L) > 1 ? luaL_check_int(L, 2) : 0;
  file = lua_gettop(L) > 2 ? (char *)luaL_check_string(L, 3) : NULL;

  if ((id = create_control(selectorTriggerCtl, name, resId, file, 0, 0,
                           NULL, NULL, NULL, NULL, NULL, true, true)) == 0)
    return 0;

  lua_pushnumber(L, id);
  return 1;
}

static int gui_checkbox (lua_State *L) {
  char *name = (char *)luaL_check_string(L, 1);
  UInt16 id;

  if ((id = create_control(checkboxCtl, name, 0, NULL, 0, 0,
                           NULL, NULL, NULL, NULL, NULL, true, true)) == 0)
    return 0;

  lua_pushnumber(L, id);
  return 1;
}

UInt16 create_slider (Int16 d, Int16 value, Int16 *px, Int16 *py, Int16 *width, Int16 *height)
{
  Int16 x, y, min, max;
  UInt16 id, thumbId, backgroundId, bw, bh;
  FormPtr frm;
  SliderControlType *ctl = NULL;
  RectangleType rect;

  if (displayPtr->wh != WinGetDrawWindow() ||
      (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh) ||
      !fullScreen)
    return 0;
    
  if (romNumber < 50) {
    thumbId = 13350;
    backgroundId = 13351;
    *height = 0;

    if (bmpsize(backgroundId, &bw, &bh) < 0)
      return 0;
    if (bh > *height) *height = bh;

    if (bmpsize(thumbId, &bw, &bh) < 0)
      return 0;
    if (bh > *height) *height = bh;

    if (*width < bw || d < 2)
      return 0;
  } else {
    thumbId = 0;
    backgroundId = 0;
    *height = 15*displayPtr->factorY;
  }

  min = 0;
  max = d-1;
  if (value < min)
    value = min;
  else if (value > max)
    value = max;

  x = (px && *px >= 0) ? *px : displayPtr->x;
  y = (py && *py >= 0) ? *py : displayPtr->y;
  id = nextID();
  frm = FrmGetActiveForm();

  ctl = hrCtlNewSliderControl((void **)&frm, id, sliderCtl,
            thumbId, backgroundId,
            xi+x+1, yi+y+1, *width, *height, min, max, 1, value);

  FrmSetActiveForm(frm);
  SetEventHandler(frm, FrmGetActiveFormID());

  displayPtr->wh = FrmGetWindowHandle(frm);
  WinSetActiveWindow(displayPtr->wh);

  if (ctl) {
    hrWinSetClip(&displayPtr->clip0);
    CtlDrawControl((ControlType *)ctl);
    WinResetClip();

    hrFrmGetObjectBounds(frm, FrmGetObjectIndex(frm, id), &rect);
    if (rect.extent.y > displayPtr->maxY)
      displayPtr->maxY = rect.extent.y+2*displayPtr->factorY;
    displayPtr->x += rect.extent.x+2*displayPtr->factorX;

    if (px) *px = x;
    if (py) *py = y;

    return id;
  }

  previousID();
  return 0;
}

static int gui_slider (lua_State *L) {
  Int16 d, value;
  UInt16 id, width, height;
    
  width = luaL_check_int(L, 1);
  d = luaL_check_int(L, 2);
  value = lua_gettop(L) > 2 ? luaL_check_int(L, 3) : 1;

  if (width == 0 || d < 2 || value < 1)
    return 0;


  if ((id = create_slider(d, value-1, NULL, NULL, &width, &height)) == 0)
    return 0;

  lua_pushnumber(L, id);
  return 1;
}

UInt16 create_label(char *name, Int16 *px, Int16 *py, Int16 *pfont, Boolean visible)
{
  Int16 x, y, font, dv;
  UInt16 id;
  FormPtr frm;
  FormLabelType *lbl;

  if (displayPtr->wh != WinGetDrawWindow() ||
      (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh) ||
      !fullScreen)
    return 0;

  if (!name)
    return 0;

  dv = visible ? 0 : displayPtr->height;

  x = (px && *px >= 0) ? *px : displayPtr->x;
  y = (py && *py >= 0) ? *py : displayPtr->y;
  font = (pfont && *pfont != -1) ? *pfont : displayPtr->font;

  id = nextID();
  frm = FrmGetActiveForm();
  lbl = hrFrmNewLabel(&frm, id, name, xi+x, yi+y+1+dv, font);

  FrmSetActiveForm(frm);
  SetEventHandler(frm, FrmGetActiveFormID());

  displayPtr->wh = FrmGetWindowHandle(frm);
  WinSetActiveWindow(displayPtr->wh);

  if (lbl) {
    hrWinSetClip(&displayPtr->clip0);
    FrmShowObject(frm, FrmGetObjectIndex(frm, id));
    WinResetClip();

    FntSetFont(font);

    if (hrFntCharHeight() > displayPtr->maxY)
      displayPtr->maxY = hrFntCharHeight();
    displayPtr->x += hrFntCharsWidth(name, StrLen(name)) + 4*displayPtr->factorX;

    FntSetFont(displayPtr->font);

    if (px) *px = x;
    if (py) *py = y;
    if (pfont) *pfont = font;

    return id;
  }

  previousID();
  return 0;
}

static int gui_label(lua_State *L)
{
  char *name = (char *)luaL_check_string(L, 1);
  UInt16 id;
  
  if ((id = create_label(name, NULL, NULL, NULL, true)) == 0)
    return 0;

  lua_pushnumber(L, id);
  return 1;
}

static void field_settext(FieldType *fld, char *s)
{
  Int16 size, max;
  MemHandle h, oldh;
  MemPtr p;
      
  if (!fld || ! s)
    return;
  max = FldGetMaxChars(fld);
  size = StrLen(s)+1;
  if (size > max) size = max;
  h = MemHandleNew(size);
  if (!h)   
    return;
  p = MemHandleLock(h);
  if (!p) {
    MemHandleFree(h);
    return;
  }
  MemMove((char *)p, s, size);
  ((char *)p)[size-1] = 0;
  MemHandleUnlock(h);

  oldh = FldGetTextHandle(fld);

  FldSetTextHandle(fld, h);
  FldSetInsertionPoint(fld, 0);
  FldSetInsPtPosition(fld, 0);

  if (oldh)
    MemHandleFree(oldh);
}

UInt16 create_field(Int16 nlines, Int16 ncols, Int16 max, char *value,
                    Boolean editable, Boolean underlined, Int16 alignment,
                    Int16 *px, Int16 *py, Int16 *pwidth, Int16 *pheight,
                    Int16 *pfont)
{
  UInt16 id, sclid, index;
  Int16 x, y, width, fldwidth, sclwidth, height, font;
  FieldType *fld;
  ScrollBarType *scl;
  RectangleType rect;
  FieldAttrType attr;
  FormPtr frm;

  if (displayPtr->wh != WinGetDrawWindow() ||
      (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh) ||
      !fullScreen)
    return 0;

  if (nlines <= 0 || ncols <= 0 || max <= 0)
    return 0;

  x = (px && *px >= 0) ? *px : displayPtr->x;
  y = (py && *py >= 0) ? *py : displayPtr->y;
  height = (pheight && *pheight >= 0) ? *pheight : nlines * hrFntCharHeight();
  font = (pfont && *pfont >= 0) ? *pfont : displayPtr->font;

  if (nlines > 1 && (sclid = nextSclID()) != 0) {
    sclwidth = 7*displayPtr->factorX;
  } else {
    sclwidth = 0;
    sclid = 0;
  }

  if (pwidth && *pwidth > sclwidth) {
    fldwidth = *pwidth - sclwidth;
    width = *pwidth;
  } else {
    fldwidth = ncols * hrFntCharWidth('a');
    width = fldwidth + sclwidth;
  }

  id = nextID();
  frm = FrmGetActiveForm();
  FntSetFont(font);

  fld = hrFldNewField((void **)&frm, id, xi+x+1, yi+y+1, fldwidth, height,
            font, max, true, underlined, nlines == 1, false,
            alignment, false, nlines > 1, false);

  FrmSetActiveForm(frm);
  SetEventHandler(frm, FrmGetActiveFormID());

  displayPtr->wh = FrmGetWindowHandle(frm);
  WinSetActiveWindow(displayPtr->wh);

  if (fld) {
    field_settext(fld, value);

    hrWinSetClip(&displayPtr->clip0);
    FldDrawField(fld);
    WinResetClip();

    if (height > displayPtr->maxY)
      displayPtr->maxY = height + 4*displayPtr->factorY;
    displayPtr->x += fldwidth;

    if (sclid) {
      setsclid(id, sclid);
      index = FrmGetObjectIndex(frm, sclid);
      scl = (ScrollBarType *)FrmGetObjectPtr(frm, index);
      hrRctSetRectangle(&rect,
         xi+x + fldwidth + 3*displayPtr->factorX,
         yi+y + hrFntCharHeight()/2,
         sclwidth, height);
      FrmSetObjectBounds(frm, index, &rect);

      hrWinSetClip(&displayPtr->clip0);
      FrmShowObject(frm, index);
      WinResetClip();

      displayPtr->x += 3*displayPtr->factorX + sclwidth;
      UpdateScrollBar(fld, frm, sclid);
    }
    displayPtr->x += 4*displayPtr->factorX;

    FldGetAttributes(fld, &attr);
    attr.editable = editable;
    FldSetAttributes(fld, &attr);
    hrWinSetClip(&displayPtr->clip0);
    FldDrawField(fld);
    WinResetClip();

    if (px) *px = x;
    if (py) *py = y;
    if (pwidth) *pwidth = width;   
    if (pheight) *pheight = height;   
    if (pfont) *pfont = font;

    FntSetFont(displayPtr->font);

    return id;
  }

  FntSetFont(displayPtr->font);
  previousID();
  return 0;
}

static int gui_field (lua_State *L) {
  UInt16 id;
  char *value;
  Int16 nlines, ncols, max;
  Int16 alignment = -1;
  Boolean editable = true, underlined = true;

  nlines = luaL_check_int(L, 1);
  ncols = luaL_check_int(L, 2);
  max = luaL_check_int(L, 3)+1;
  if (max <= 0) max = 1;
  value = (char *)luaL_opt_string(L, 4, "");
  if (lua_gettop(L) > 4)
    editable = lua_isnil(L, 5) ? false : true;
  if (lua_gettop(L) > 5)
    underlined = lua_isnil(L, 6) ? false : true;
  if (lua_gettop(L) > 6)
    alignment = luaL_check_int(L, 7);

  if (alignment < 0) alignment = leftAlign;
  else if (alignment > 0) alignment = rightAlign;
  else alignment = centerAlign;

  if ((id = create_field(nlines, ncols, max, value, editable, underlined,
              alignment, NULL, NULL, NULL, NULL, NULL)) == 0)
    return 0;

  lua_pushnumber(L, id);
  return 1;
}

static int gui_gsi (lua_State *L) {
  Int16 x, y;
  FormPtr frm;
  FrmGraffitiStateType *gsi;

  if (displayPtr->wh != WinGetDrawWindow() ||
      (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh) ||
      !fullScreen)
    return 0;

  x = displayPtr->x;
  y = displayPtr->y;

  frm = FrmGetActiveForm();
  gsi = FrmNewGsi(&frm, x, y);
  FrmSetActiveForm(frm);
  SetEventHandler(frm, FrmGetActiveFormID());
  displayPtr->wh = FrmGetWindowHandle(frm);
  WinSetActiveWindow(displayPtr->wh);
  GsiSetLocation(x, y);
  GsiEnable(true);

  return 0;
}

static int gui_fieldattr (lua_State *L) {
  Int16 id;
  UInt16 index;
  FormObjectKind type;
  FormPtr frm = FrmGetActiveForm();

  if ((id = findcontrol(L, 1)) == -1)
    return 0;

  index = FrmGetObjectIndex(frm, id);
  type = FrmGetObjectType(frm, index);

  if (type == frmFieldObj) {
    FieldType *fld;
    FieldAttrType attr;
    Int16 editable = lua_isnil(L, 2) ? 0 : 1;
    Int16 underlined = lua_isnil(L, 3) ? 0 : 1;

    fld = (FieldType *)FrmGetObjectPtr(frm, index);
    FldGetAttributes(fld, &attr);
    attr.editable = editable;
    attr.underlined = underlined;
    FldSetAttributes(fld, &attr);

    hrWinSetClip(&displayPtr->clip0);
    FldDrawField(fld);
    WinResetClip();
  }

  return 0;
}

UInt16 create_list (char **items, Int16 i, Int16 popup,
                    Int16 nlines, Int16 ncols, Int16 sel,
                    Int16 *px, Int16 *py, Int16 *pwidth, Int16 *pheight,
                    Int16 *pfont, Boolean leftAnchor)
{
  Int16 x, y, width, height, w, j, font;
  UInt16 id, popupId;
  FormPtr frm;
  ListType *lst;
  ControlType *ctl;
  Err err;

  if (displayPtr->wh != WinGetDrawWindow() ||
      (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh) ||
      !fullScreen)
    return 0;

  x = (px && *px >= 0) ? *px : displayPtr->x;
  y = (py && *py >= 0) ? *py : displayPtr->y;
  font = (pfont && *pfont != -1) ? *pfont : displayPtr->font;

  if (popup) {
    nlines = i;
    height = nlines * hrFntCharHeight() + 2*displayPtr->factorY;
    for (width = hrFntCharWidth('a'), j = 0; j < i; j++) {
      w = hrFntCharsWidth(items[j], StrLen(items[j]));
      if (w > width)
        width = w;
    }
    width += 4*displayPtr->factorX;

    if ((y+height) > displayPtr->clip0.extent.y) {
      nlines = (displayPtr->clip0.extent.y - y - 2*displayPtr->factorY) / hrFntCharHeight();
      height = nlines * hrFntCharHeight() + 2*displayPtr->factorY;
      width += hrFntCharWidth('w'); // seta de scroll
    }
  } else {
    height = nlines * hrFntCharHeight() + 2*displayPtr->factorY;
    width = ncols * hrFntCharWidth('a');
  }

  if (sel < 1 || sel > i) sel = 1;

  frm = FrmGetActiveForm();

  if (popup) {
    popupId = nextID();
    ctl = hrCtlNewControl((void **)&frm, popupId, popupTriggerCtl, items[sel-1],
            xi+x+1, yi+y+1, width + 2*hrFntCharWidth('a'), hrFntCharHeight(),
            font, 0, leftAnchor);

    FrmSetActiveForm(frm);
    SetEventHandler(frm, FrmGetActiveFormID());
    displayPtr->wh = FrmGetWindowHandle(frm);
    WinSetActiveWindow(displayPtr->wh);

    if (!ctl) {
      previousID();
      if (items) free(items);
      return 0;
    }

    hrWinSetClip(&displayPtr->clip0);
    CtlDrawControl(ctl);
    WinResetClip();
    x += 4*displayPtr->factorX;

    if (pheight) *pheight = hrFntCharHeight()+2*displayPtr->factorY;
    if (pwidth) *pwidth = width + 2*hrFntCharWidth('a');   

  } else {
    popupId = 0;
    ctl = NULL;

    if (pheight) *pheight = height;   
    if (pwidth) *pwidth = width;   
  }

  id = nextID();
  err = hrLstNewList((void **)&frm, id, xi+x+1, yi+y+1, width, height, font, nlines, popupId);

  FrmSetActiveForm(frm);
  SetEventHandler(frm, FrmGetActiveFormID());
  displayPtr->wh = FrmGetWindowHandle(frm);
  WinSetActiveWindow(displayPtr->wh);

  if (!err) {
    RectangleType rect;
    lst = (ListType *)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, id));
    if (items && i) {
      LstSetListChoices(lst, items, i);
      LstSetSelection(lst, sel-1);
    }

    if (!popup) {
      LstMakeItemVisible(lst, sel-1);
      hrWinSetClip(&displayPtr->clip0);
      LstDrawList(lst);
      WinResetClip();
    }

    hrFrmGetObjectBounds(frm, FrmGetObjectIndex(frm, id), &rect);
    if (popup)
      height = hrFntCharHeight()+2*displayPtr->factorY;
    if (height > displayPtr->maxY)
      displayPtr->maxY = height;
    displayPtr->x += rect.extent.x + 8*displayPtr->factorX;
    if (popup)
      displayPtr->x += 8*displayPtr->factorX;

    if (px) *px = x;   
    if (py) *py = y;   
    if (pfont) *pfont = font;   

    setitems(id, items);

    return id;
  }

  if (popup)
    previousID();
  previousID();

  return 0;
}

static int gui_list (lua_State *L) {
  char **items;
  Int16 nitems, sel, nlines, ncols;
  UInt16 id;

  nlines = luaL_check_int(L, 1);
  ncols = luaL_check_int(L, 2);
  items = buildlist(L, 3, &nitems);
  sel = luaL_opt_int(L, 4, 1);

  if (items == NULL || nitems < 1 ||
      (id = create_list(items, nitems, 0, nlines, ncols, sel, NULL, NULL, NULL, NULL, NULL, true)) == 0)
    return 0;

  lua_pushnumber(L, id);
  return 1;
}

static int gui_popup (lua_State *L) {
  char **items;
  Int16 nitems, sel;
  UInt16 id;

  items = buildlist(L, 1, &nitems);
  sel = luaL_opt_int(L, 2, 1);

  if (items == NULL || nitems < 1 ||
      (id = create_list(items, nitems, 1, 0, 0, sel, NULL, NULL, NULL, NULL, NULL, true)) == 0)
    return 0;

  lua_pushnumber(L, id);
  return 1;
}

static int gui_gettext (lua_State *L) {
  char *s, buf[32];
  FieldType *fld;
  ControlType *ctl;
  ListType *lst;
  FormPtr frm;
  Int16 id;
  UInt16 index;
  FormObjectKind type;
  GadgetData *gdata;

  if ((id = findcontrol(L, 1)) == -1)
    return 0;

  frm = FrmGetActiveForm();
  index = FrmGetObjectIndex(frm, id);
  type = FrmGetObjectType(frm, index);

  switch (type) {
    case frmFieldObj:
      fld = (FieldType *)FrmGetObjectPtr(frm, index);
      s = FldGetTextPtr(fld);
      break;
    case frmControlObj:
      ctl = (ControlType *)FrmGetObjectPtr(frm, index);
      s = (char *)CtlGetLabel(ctl);
      break;
    case frmListObj:
      lst = (ListType *)FrmGetObjectPtr(frm, index);
      s = LstGetSelectionText(lst, LstGetSelection(lst));
      break;
    case frmLabelObj:
      s = (char *)FrmGetLabel(frm, id);
      break;
    case frmGadgetObj:
      gdata = FrmGetGadgetData(frm, index);
      gdata->callback(L, gadgetGetText, id, gdata->data, buf, &gdata->rect);
      s = buf;
      break;
    default:
      return 0;
  }
  lua_pushstring(L, s ? s : "");
  return 1;
}

static int gui_setfocus (lua_State *L) {
  Int16 id;
  UInt16 index, focusIndex;
  FormPtr frm;
  FieldPtr fld;

  if ((id = findcontrol(L, 1)) == -1)
    return 0;

  frm = FrmGetActiveForm();
  index = FrmGetFocus(frm);
  focusIndex = FrmGetObjectIndex(frm, id);

  if (index != noFocus && FrmGetObjectType(frm, index) == frmFieldObj &&
      index != focusIndex) {
    fld = (FieldPtr)FrmGetObjectPtr(frm, index);
    FldReleaseFocus(fld);
  }

  FrmSetFocus(frm, focusIndex);
  return 0;
}

Int16 findcontrol(lua_State *L, Int16 pos) {
  Int16 i, id;

  id = luaL_check_int(L, pos);

  for (i = nControlsDialog; i < nControls; i++)
    if (id == dynamicControls[i])
      return id;

  return -1;
}

static int gui_settext (lua_State *L) {
  char *s, *r;
  FieldType *fld;
  ControlType *ctl;
  FormPtr frm;
  Int16 id, sclid;
  UInt16 index;
  FormObjectKind type;
  FieldAttrType attr;
  Boolean editable;
  GadgetData *gdata;

  if ((id = findcontrol(L, 1)) == -1)
    return 0;

  frm = FrmGetActiveForm();
  index = FrmGetObjectIndex(frm, id);
  type = FrmGetObjectType(frm, index);

  switch (type) {
    case frmFieldObj:
      s = (char *)luaL_check_string(L, 2);
      fld = (FieldType *)FrmGetObjectPtr(frm, index);
      FldGetAttributes(fld, &attr);
      editable = attr.editable;
      attr.editable = true;
      FldSetAttributes(fld, &attr);
      field_settext(fld, s);
      hrWinSetClip(&displayPtr->clip0);
      FldDrawField(fld);
      sclid = getsclid(id);
      UpdateScrollBar(fld, frm, sclid);
      WinResetClip();
      attr.editable = editable;
      FldSetAttributes(fld, &attr);
      break;
    case frmControlObj:
      s = (char *)luaL_check_string(L, 2);
      ctl = (ControlType *)FrmGetObjectPtr(frm, index);
      CtlSetLabel(ctl, s);
      break;
    case frmLabelObj:
      s = (char *)luaL_check_string(L, 2);
      r = (char *)FrmGetLabel(frm, id);
      if (r && StrLen(s) <= StrLen(r))
        FrmCopyLabel(frm, id, s);
      break;
    case frmListObj:
      gui_setlist(L);
      break;
    case frmGadgetObj:
      s = (char *)luaL_check_string(L, 2);
      gdata = FrmGetGadgetData(frm, index);
      gdata->callback(L, gadgetSetText, id, gdata->data, s, &gdata->rect);
  }

  return 0;
}

static int gui_setlist (lua_State *L) {
  char **items;
  ControlType *ctl;
  ListType *lst;
  FormPtr frm;
  Int16 i, j, id, sel, top, y, w;
  UInt16 index, popupIndex, width, height;
  FormObjectKind type;
  RectangleType rect;
  Boolean isPopup;

  if ((id = findcontrol(L, 1)) == -1)
    return 0;

  frm = FrmGetActiveForm();
  index = FrmGetObjectIndex(frm, id);
  type = FrmGetObjectType(frm, index);

  switch (type) {
    case frmListObj:
      items = buildlist(L, 2, &i);
      if (items == NULL || i < 1)
        return 0;
      lst = (ListType *)FrmGetObjectPtr(frm, index);
      sel = LstGetSelection(lst)+1;
      top = LstGlueGetTopItem(lst);
      if (i < sel)
        sel = i ? i : 1;

      setitems(id, items);

      LstSetListChoices(lst, items, i);
      LstSetSelection(lst, sel-1);
      isPopup = false;

      popupIndex = FrmGetObjectIndex(frm, id-1);
      if (FrmGetObjectType(frm, popupIndex) == frmControlObj) {
        ctl = (ControlType *)FrmGetObjectPtr(frm, popupIndex);
        if (CtlGlueGetControlStyle(ctl) == popupTriggerCtl) {
          CtlSetLabel(ctl, items ? items[sel-1] : "");
          isPopup = true;

          hrFrmGetObjectBounds(frm, index, &rect);
          y = rect.topLeft.y;
          height = i * hrFntCharHeight() + 2*displayPtr->factorY;
          j = i;
          if ((y+height) > displayPtr->clip0.extent.y)
            j = (displayPtr->clip0.extent.y - y - 2 * displayPtr->factorY) / hrFntCharHeight();
          LstSetHeight(lst, j);

          for (width = hrFntCharWidth('a'), j = 0; j < i; j++) {
            w = hrFntCharsWidth(items[j], StrLen(items[j]));
            if (w > width)
              width = w;
          }
          width += 4*displayPtr->factorX;
          hrFrmGetObjectBounds(frm, index, &rect);
          rect.extent.x = width;
          hrRctSetRectangle(&rect, rect.topLeft.x, rect.topLeft.y, rect.extent.x, rect.extent.y);
          FrmSetObjectBounds(frm, index, &rect);
        }
      }
      if (!isPopup) {
        LstMakeItemVisible(lst, sel-1);
        hrWinSetClip(&displayPtr->clip0);
        LstDrawList(lst);
        WinResetClip();
      }
  }

  return 0;
}

static int gui_inserttext (lua_State *L) {
  char *s;
  Int16 id;
  UInt16 index;
  FieldType *fld;
  FormObjectKind type;
  FormPtr frm = FrmGetActiveForm();

  if ((id = findcontrol(L, 1)) == -1)
    return 0;

  s = (char *)luaL_check_string(L, 2);

  index = FrmGetObjectIndex(frm, id);
  type = FrmGetObjectType(frm, index);

  if (type != frmFieldObj)
    return 0;

  fld = (FieldType *)FrmGetObjectPtr(frm, index);
  if (s[0])
    FldInsert(fld, s, strlen(s));
  else {
    UInt16 start, end;
    FldGetSelection(fld, &start, &end);
    FldDelete(fld, start, end);
  }

  return 0;
}

static int gui_state (lua_State *L, Boolean set) {
  ControlType *ctl;
  ListType *lst;
  FieldType *fld;
  FormPtr frm;
  Int16 id, state;
  UInt16 index, popupIndex, start, end, min, max;
  Boolean isPopup;
  FormObjectKind type;
  GadgetData *gdata;

  if ((id = findcontrol(L, 1)) == -1)
    return 0;

  state = set ? luaL_check_int(L, 2) : 0;
  if (state < 0) state = 0;

  frm = FrmGetActiveForm();
  index = FrmGetObjectIndex(frm, id);
  type = FrmGetObjectType(frm, index);

  switch (type) {
    case frmControlObj:
      ctl = (ControlType *)FrmGetObjectPtr(frm, index);

      min = max = 0;
      CtlGetSliderValues(ctl, &min, &max, NULL, NULL);

      if (min == max) {
        if (set)
          CtlSetValue(ctl, state);
        else
          state = CtlGetValue(ctl);
      } else {
        if (set)
          CtlSetValue(ctl, state < 1 ? 1 : state-1);
        else
          state = CtlGetValue(ctl)+1;
      }
      break;
    case frmListObj:
      lst = (ListType *)FrmGetObjectPtr(frm, index);
      if (set) {
        LstSetSelection(lst, state-1);
        isPopup = false;
        popupIndex = FrmGetObjectIndex(frm, id-1);
        if (FrmGetObjectType(frm, popupIndex) == frmControlObj) {
          ctl = (ControlType *)FrmGetObjectPtr(frm, popupIndex);
          if (CtlGlueGetControlStyle(ctl) == popupTriggerCtl) {
            CtlSetLabel(ctl, LstGetSelectionText(lst, LstGetSelection(lst)));
            isPopup = true;
          }
        }
        if (!isPopup) {
          LstMakeItemVisible(lst, state-1);
          hrWinSetClip(&displayPtr->clip0);
          LstDrawList(lst);
          WinResetClip();
        } else
          LstMakeItemVisible(lst, state-1);
      } else
        state = LstGetSelection(lst)+1;
      break;
    case frmFieldObj:
      fld = (FieldType *)FrmGetObjectPtr(frm, index);
      if (set) {
        if (lua_isnone(L, 3)) {
          FldSetInsertionPoint(fld, state-1);
          FldSetScrollPosition(fld, state-1);
          UpdateScrollBar(fld, frm, getsclid(id));
          break;
        }
        start = state;
        end = luaL_check_int(L, 3);
        FldSetSelection(fld, start-1, end);
        FldSetScrollPosition(fld, start-1);
        UpdateScrollBar(fld, frm, getsclid(id));
      } else {
        FldGetSelection(fld, &start, &end);
        if (start == end) {
          state = start+1;
          break;
        }
        start++;
      }
      lua_pushnumber(L, start);
      lua_pushnumber(L, end);
      return 2;
    case frmGadgetObj:
      gdata = FrmGetGadgetData(frm, index);
      if (set)
        gdata->callback(L, gadgetSetState, id,
                        gdata->data, &state, &gdata->rect);
      else
        gdata->callback(L, gadgetGetState, id,
                        gdata->data, &state, &gdata->rect);
      break;
    default:
      return 0;
  }

  lua_pushnumber(L, state);
  return 1;
}

static int gui_getstate (lua_State *L) {
  return gui_state(L, false);
}

static int gui_setstate (lua_State *L) {
  return gui_state(L, true);
}

static int gui_setvisible (lua_State *L) {
  Int16 id, sclid;
  UInt16 index, sclIndex, popupIndex;
  Boolean visible;
  FormPtr frm;
  ControlType *ctl;
  FormObjectKind type;
  GadgetData *gdata;
  Coord x, y;

  if ((id = findcontrol(L, 1)) == -1)
    return 0;

  visible = (Boolean)lua_toboolean(L, 2);

  frm = FrmGetActiveForm();
  index = FrmGetObjectIndex(frm, id);
  type = FrmGetObjectType(frm, index);

  switch (type) {
    case frmLabelObj:
      FrmGetObjectPosition(frm, index, &x, &y);
      if (visible) {
        if (y >= displayPtr->height)
          FrmSetObjectPosition(frm, index, x, y-displayPtr->height);
        FrmShowObject(frm, index);
      } else
        FrmHideObject(frm, index);
      break;
    case frmGadgetObj:
      gdata = FrmGetGadgetData(frm, index);
      gdata->callback(L, visible ? gadgetDraw : gadgetErase, id,
        gdata->data, NULL, &gdata->rect);
      break;
    case frmFieldObj:
      sclid = getsclid(id);
      sclIndex = 0;

      if (sclid)
        sclIndex = FrmGetObjectIndex(frm, sclid);

      if (visible) {
        FrmShowObject(frm, index);
        if (sclid)
          FrmShowObject(frm, sclIndex);
      } else {
        FrmHideObject(frm, index);
        if (sclid)
          FrmHideObject(frm, sclIndex);
      }
      break;
    case frmListObj:
      popupIndex = FrmGetObjectIndex(frm, id-1);
      if (FrmGetObjectType(frm, popupIndex) == frmControlObj) {
        ctl = (ControlType *)FrmGetObjectPtr(frm, popupIndex);
        if (CtlGlueGetControlStyle(ctl) == popupTriggerCtl) {
          if (visible)
            FrmShowObject(frm, popupIndex);
          else
            FrmHideObject(frm, popupIndex);
        }
      }
      // fall-through

    default:
      if (visible)
        FrmShowObject(frm, index);
      else
        FrmHideObject(frm, index);
  }

  return 0;
}

static int gui_nl (lua_State *L) {
  displayPtr->x = 0;
  displayPtr->y += displayPtr->maxY ? displayPtr->maxY+2*displayPtr->factorY : 8*displayPtr->factorY;
  displayPtr->maxY = 0;
  return 0;
}

static int gui_tab (lua_State *L) {
  Int16 t = lua_isnone(L, 1) ? 1 : luaL_check_int(L, 1);
  displayPtr->x += t*8*displayPtr->factorX;
  return 0;
}

void plua_ptextsize(char *s, int *w, int *h)
{
  *w = hrFntCharsWidth(s, StrLen(s));
  *h = hrFntCharHeight();
}

static int screen_textsize (lua_State *L) {
  char *s;
  int w, h;

  s = (char *)luaL_check_string(L, 1);
  plua_ptextsize(s, &w, &h);
  lua_pushnumber(L, w);
  lua_pushnumber(L, h);

  return 2;
}

static int gui_alert (lua_State *L) {
  char *msg = (char *)luaL_check_string(L, 1);
  Boolean error = (lua_gettop(L) > 1) ? (Boolean)lua_toboolean(L, 2) : false;

  if (!(displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh && msg)) {
    FrmCustomAlert(error ? ErrorAlert : InfoAlert, msg, "", "");
    displayPtr->wh = WinGetDrawWindow();
    sync();
  }
  return 0;
}

static int gui_confirm (lua_State *L) {
  char *msg = (char *)luaL_check_string(L, 1);
  Int16 r = 0;

  if (!(displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh) && msg) {
    r = FrmCustomAlert(ConfirmAlert, msg, "", "");
    displayPtr->wh = WinGetDrawWindow();
    sync();
  }

  lua_pushboolean(L, r ? 0 : 1);
  return 1;
}

static int gui_input (lua_State *L) {
  FieldType *fld;
  char *msg, *fill, *s;
  UInt16 r, index;
  FormType *frm;

  if (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh)
    return 0;

  msg = lua_isnone(L, 1) ? NULL : (char *)luaL_check_string(L, 1);
  fill = lua_isnone(L, 2) ? NULL : (char *)luaL_check_string(L, 2);

  frm = FrmInitForm(InputForm);
  index = FrmGetObjectIndex(frm, inputFld);

  if (msg)
    FrmSetTitle(frm, msg);
  if (fill) {
    fld = (FieldPtr)FrmGetObjectPtr(frm, index);
    FldInsert(fld, fill, StrLen(fill));
  }

  FrmSetFocus(frm, index);

  r = FrmDoDialog(frm);

  if (r == okBtn) {
    fld = (FieldPtr)FrmGetObjectPtr(frm, index);
    s = FldGetTextPtr(fld);
    lua_pushstring(L, s ? s : "");
    FrmDeleteForm(frm);
    displayPtr->wh = WinGetDrawWindow();
    sync();

    return 1;
  }
  FrmDeleteForm(frm);
  displayPtr->wh = WinGetDrawWindow();
  sync();

  return 0;
}

static int screen_heading (lua_State *L) {
  if (lua_gettop(L) >= 1) {
    displayPtr->heading = luaL_checknumber(L, 1);
    return 1;
  }
  lua_pushnumber(L, displayPtr->heading);
  return 1;
}

static int screen_turn (lua_State *L) {
  displayPtr->heading += luaL_checknumber(L, 1);
  return 0;
}

static int screen_walkjump(Int16 d, UInt32 c, Boolean draw)
{
  Int16 x, y, aux;

  x = displayPtr->x;
  y = displayPtr->y;
  aux = d * cos(displayPtr->heading);
  x += aux;
  aux = d * sin(displayPtr->heading);
  y -= aux;

  if (draw)
    draw_line(displayPtr->x, displayPtr->y, x, y, c);
  else {
    displayPtr->x = x;
    displayPtr->y = y;
  }

  return 0;
}

static int screen_walk (lua_State *L) {
  Int16 d = luaL_check_int(L, 1);
  UInt32 c = luaL_opt_long(L, 2, displayPtr->fg);
  return screen_walkjump(d, c, true);
}

static int screen_jump (lua_State *L) {
  Int16 d = luaL_check_int(L, 1);
  return screen_walkjump(d, 0, false);
}

UInt16 create_title(char *title)
{
  FormPtr frm;
  Int16 h, m;

  if (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh)
    return 0;

  if (fullScreen && FrmGetActiveFormID() != FullGraphicForm) {
    frm = FrmGetActiveForm();

    m = WinGlueGetFrameType(FrmGetWindowHandle(frm)) == dialogFrame ? 10 : 14;
    h = m*displayPtr->factorY;

    if (title) {
      FrmSetTitle(frm, title);
      FrmDrawForm(frm);
      displayPtr->y = h+2*displayPtr->factorY;
    } else {
      RectangleType rect;
      FrmSetTitle(frm, "");
      if (displayPtr->wh == WinGetDrawWindow()) {
        RctSetRectangle(&rect,
          displayPtr->clip.topLeft.x, displayPtr->clip.topLeft.y,
          displayPtr->clip.extent.x, h);
        hrWinEraseRectangle(&rect, 1);
      }
    }
  }

  return 0;
}

static int gui_title (lua_State *L) {
  char *title;

  title = (char *)luaL_opt_string(L, 1, NULL);
  create_title(title);

  return 0;
}

static int gui_menu (lua_State *L) {
  char **items;
  MenuBarType *menu;

  if (displayPtr->wh != appDisplay.wh)
    return 0;

  if (!fullScreen)
    return 0;

  freemenu();
  menuId = AppMenu;
  items = NULL;

  if (!lua_isnone(L, 1)) {
    if (lua_istable(L, 1)) {
      items = buildlist(L, 1, &nMenuItems);
      if (nMenuItems > (LastCmd-FirstCmd+1))
        nMenuItems = LastCmd-FirstCmd+1;
    }
  }

  menuItems = items;

  menu = MenuGetActiveMenu();
  if (menu)
    MenuDispose(menu);
  if ((menu = MenuInit(menuId)) == NULL) {
    menuId = AppMenu;
    menu = MenuInit(menuId);
  }
  MenuSetActiveMenu(menu);

  if (menuId != AppMenu)
    return 0;

  restoremenu();
  return 0;
}

void freemenu(void)
{
  Int16 i;

  if (menuItems) {
    for (i = 0; i < nMenuItems; i++) {
      if (menuItems[i])
        free(menuItems[i]);
    }
    free(menuItems);
  }
  nMenuItems = 0;
  menuItems = NULL;
}

void restoremenu(void)
{
  Int16 i;
  char shortcut, *text;

  for (i = 0; i < nMenuItems; i++) {
    if (menuItems[i][0] && menuItems[i][1] == ':') {
      shortcut = menuItems[i][0];
      text = &menuItems[i][2];
    } else {
      shortcut = 0;
      text = menuItems[i];
    }
    if (i == 0)
      MenuAddItem(AboutCmd, SepCmd, 0, "-");
    MenuAddItem(SepCmd+i, SepCmd+i+1, shortcut, text);
  }
}

static int gui_selectdate (lua_State *L) {
  Int16 day, month, year, r;
  char *s;

  if (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh)
    return 0;

  s = lua_isnone(L, 1) ? "" : (char *)luaL_check_string(L, 1);
  if (lua_isnone(L, 2)) {
    DateTimeType datetime;
    TimSecondsToDateTime(TimGetSeconds(), &datetime);
    year = datetime.year;
    month = datetime.month;
    day = datetime.day;
  } else {
    year = luaL_check_int(L, 2);
    month = 1;
    day = 1;
    if (!lua_isnone(L, 3)) {
      month = luaL_check_int(L, 3);
      if (!lua_isnone(L, 4))
        day = luaL_check_int(L, 4);
    }
  }

  r = SelectDay(selectDayByDay, &month, &day, &year, s);
  sync();

  if (!r)
    return 0;

  lua_pushnumber(L, year);
  lua_pushnumber(L, month);
  lua_pushnumber(L, day);
  return 3;
}

static int gui_selecttime (lua_State *L) {
  Int16 hour, minute, r;
  char *s;

  if (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh)
    return 0;

  s = lua_isnone(L, 1) ? "" : (char *)luaL_check_string(L, 1);
  if (lua_isnone(L, 2)) {
    DateTimeType datetime;
    TimSecondsToDateTime(TimGetSeconds(), &datetime);
    hour = datetime.hour;
    minute = datetime.minute;
  } else {
    hour = luaL_check_int(L, 2);
    minute = lua_isnone(L, 3) ? 0 : luaL_check_int(L, 3);
  }

  r = SelectOneTime(&hour, &minute, s);
  sync();

  if (!r)
    return 0;

  lua_pushnumber(L, hour);
  lua_pushnumber(L, minute);
  return 2;
}

static int gui_selectcolor (lua_State *L) {
  RGBColorType rgb;
  WinHandle wh;
  UInt32 c;
  char *s;
  Err err;
  Boolean r;

  if (displayPtr->wh != appDisplay.wh && displayPtr->wh != dialogDisplay.wh)
    return 0;

  s = (char *)luaL_check_string(L, 1);
  c = luaL_check_long(L, 2);

  if (displayPtr->depth < 16)
    WinIndexToRGB(c, &rgb);
  else
    LongToRGB(c, &rgb);

  if (displayPtr->depth == 16) {
    if (displayPtr->highDensity) WinSetCoordinateSystem(displayPtr->scrCoordinate);
    wh = WinCreateOffscreenWindow(displayPtr->width, displayPtr->height,
            nativeFormat, &err);
    if (wh)
      WinCopyRectangle(displayPtr->wh, wh, &displayPtr->clip0, 0, 0, winPaint);
    if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  } else
    wh = NULL;

  r = UIPickColor(NULL, &rgb, 0, s, NULL);

  if (wh) {
    if (displayPtr->highDensity) WinSetCoordinateSystem(displayPtr->scrCoordinate);
    WinCopyRectangle(wh, displayPtr->wh, &displayPtr->clip0, 0, 0, winPaint);
    if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
    WinDeleteWindow(wh, false);
  }

  sync();

  if (!r)
    return 0;

  lua_pushnumber(L, displayPtr->depth < 16 ? WinRGBToIndex(&rgb) : RGBToLong(&rgb));
  return 1;
}

static void init(void)
{
  RGBColorType rgb;
  UInt32 version, white, black, p;
  UInt16 i;

  displayPtr = &appDisplay;

  romNumber = GetRomVersionNumber();

  displayPtr->highDensity = false;
  displayPtr->scrCoordinate = kCoordinatesDouble;
  displayPtr->movable = false;

  if (FtrGet(sysFtrCreator, sysFtrNumWinVersion, &version) == 0 && version >= 4)
    displayPtr->highDensity = true;

  hrWinScreenMode(winScreenModeGet, &displayPtr->width, &displayPtr->height,
     &displayPtr->depth, &displayPtr->enableColor);

  resizedisplay();

  if (displayPtr->highDensity) {
    rgb.r = rgb.g = rgb.b = 0;
    black = displayPtr->depth < 16 ? WinRGBToIndex(&rgb) : RGBToLong(&rgb);
    rgb.r = rgb.g = rgb.b = 255;
    white = displayPtr->depth < 16 ? WinRGBToIndex(&rgb) : RGBToLong(&rgb);

    setforecolor(black);
    hrWinPaintPixel(0, 0);
    hrWinPaintPixel(1, 0);
    setforecolor(white);
    hrWinPaintPixel(0, 0);
    p = hrWinGetPixel(1, 0);
    hrWinPaintPixel(1, 0);

    if (p == white) displayPtr->width = 160;
  }

  if (displayPtr->width == 160 || displayPtr->height == 160) {
    if (displayPtr->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
    displayPtr->width = 160;
    displayPtr->height = 160;
    displayPtr->highDensity = false;
  }

  if (displayPtr->highDensity) {
    displayPtr->factorX = displayPtr->width/160;
    displayPtr->factorY = displayPtr->height/160;
  } else {
    displayPtr->factorX = 1;
    displayPtr->factorY = 1;
    displayPtr->scrCoordinate = kCoordinatesStandard;
  }

  for (i = 0; i < NUM_RESOURCE; i++)
    resourcePool[i] = NULL;

  for (i = 0; i < NUM_HANDLE; i++)
    handlePool[i] = NULL;

  for (i = 0; i < LastInternID-FirstInternID+1; i++)
    listItems[i] = NULL;

  bufDisplay.highDensity = dialogDisplay.highDensity = appDisplay.highDensity;
  bufDisplay.scrCoordinate = dialogDisplay.scrCoordinate = appDisplay.scrCoordinate;
  bufDisplay.factorX = dialogDisplay.factorX = appDisplay.factorX;
  bufDisplay.factorY = dialogDisplay.factorY = appDisplay.factorY;
  bufDisplay.movable = dialogDisplay.movable = appDisplay.movable;
}


static void finish(lua_State *L) {
  Int16 i;

  if (world)
    SpriteFinish(L, world);
  world = NULL;

  displayPtr = &appDisplay;

  for (i = 0; i < NUM_RESOURCE; i++) {
    if (resourcePool[i]) {
      MemHandleUnlock(resourcePool[i]->h);
      DmReleaseResource(resourcePool[i]->h);
      free(resourcePool[i]);
      resourcePool[i] = NULL;
    }
  }

  for (i = 0; i < NUM_HANDLE; i++) {
    if (handlePool[i]) {
      WinDeleteWindow(handlePool[i], false);
      handlePool[i] = NULL;
    }
  }

  for (i = 0; i < LastInternID-FirstInternID+1; i++)
    freeitems(i);

  freemenu();
}

static void freeitems(UInt16 i)
{
  Int16 n, j;
  char **items;

  if (listItems[i]) {
    items = (char **)listItems[i];
    n = MemPtrSize(listItems[i]) / sizeof(char *);
    for (j = 0; j < n; j++) {
      if (items[j])
        free(items[j]);
    }
    free(listItems[i]);
    listItems[i] = NULL;
  }
}

static void setitems(UInt16 id, void *p)
{
  Int16 i;

  for (i = 0; i < nControls; i++) {
    if (dynamicControls[i] == id) {
      freeitems(i);
      listItems[i] = p;
      break;
    }
  }
}

static UInt16 nextID(void)
{
  if (nControls < (LastInternID-FirstInternID+1))
    dynamicControls[nControls++] = controlId;
  return controlId++;
}

static void previousID(void)
{
  if (nControls > 0) {
    nControls--;
    controlId--;
  }
}

static UInt16 nextSclID(void)
{
  UInt16 id = 0;

  if (nScrolls < NUM_SCROLLS) {
    id = graphicScl + nScrolls;
    nScrolls++;
  }

  return id;
}

void setsclid(UInt16 fieldId, UInt16 sclid)
{
  scrollIds[nScrolls-1] = sclid;
  fieldIds[nScrolls-1] = fieldId;
}

UInt16 getsclid(UInt16 fieldId)
{
  Int16 i;

  for (i = 0; i < nScrolls; i++)
    if (fieldIds[i] == fieldId)
      return scrollIds[i];

  return 0;
}

UInt16 getfldid(UInt16 sclid)
{
  Int16 i;

  for (i = 0; i < nScrolls; i++)
    if (scrollIds[i] == sclid)
      return fieldIds[i];

  return 0;
}

static void destroyform(lua_State *L, FormPtr frm)
{
  Int16 i;
  UInt16 index;
  FormObjectKind type;

  for (i = nControls-1; i >= nControlsDialog; i--) {
    index = FrmGetObjectIndex(frm, dynamicControls[i]);
    if (index == frmInvalidObjectId)
      continue;
    type = FrmGetObjectType(frm, index);
    switch (type) {
      case frmListObj:
        freeitems(i);
        break;
      case frmFieldObj: {
        FieldType *fld = (FieldType *)FrmGetObjectPtr(frm, index);
        if (fld) {
          if (FldGetVisibleLines(fld) > 1)
            nScrolls--;
          FldFreeMemory(fld);
        }
        break;
      }
      case frmGadgetObj: {
        GadgetData *gdata = FrmGetGadgetData(frm, index);
        gdata->callback(gdata->context, gadgetDelete, dynamicControls[i], gdata->data, NULL, &gdata->rect);
        FrmSetGadgetHandler(frm, index, NULL);
        FrmSetGadgetData(frm, index, NULL);
        free(gdata);
        nGadgets--;
        break;
      }
    }

    AppSetRegistry(CONTROL_BASE + dynamicControls[i] - FirstInternID, NULL);

    nControls--;
    controlId--;
  }

  FrmEraseForm(frm);
  FrmDeleteForm(frm);
  nControlsDialog = 0;
}

int gui_destroy (lua_State *L) {
  FormPtr frm;
  UInt16 id;

  if (displayPtr->wh != WinGetDrawWindow() || !fullScreen)
    return 0;

  if (nForms == 0) {
    frm = FrmGetActiveForm();
    id = FrmGetFormId(frm);
    destroyform(L, frm);

    // em alguns casos o FrmEraseForm nao apaga o form (???)
    hrWinFillRectangle(&displayPtr->clip, 1);

    frm = FrmInitForm(id);
    FrmSetActiveForm(frm);
    SetEventHandler(frm, id);

    displayPtr->wh = FrmGetWindowHandle(frm);
    WinSetActiveWindow(displayPtr->wh);
    WinSetDrawWindow(displayPtr->wh);
    GsiEnable(false);

  } else {
    frm = FrmGetActiveForm();
    destroyform(L, frm);

    nForms--;
    id = formStack[nForms];

    frm = FrmGetFormPtr(id);
    FrmSetActiveForm(frm);
    SetEventHandler(frm, id);

    displayPtr = &appDisplay;
    displayPtr->wh = FrmGetWindowHandle(frm);
    WinSetActiveWindow(displayPtr->wh);
    WinSetDrawWindow(displayPtr->wh);
    FntSetFont(displayPtr->font);
    setcolors();
  }

  sync();
  return 0;
}

void resetform(void)
{
  controlId = FirstInternID;
  nControls = 0;
  nScrolls = 0;
  nGadgets = 0;
  nForms = 0;
}

MemHandle setfixedfont(void)
{
  MemHandle fntHandle;
  MemPtr fntPtr;

  fntHandle = displayPtr->highDensity ? DmGet1Resource('nfnt', FixedFontHigh) : DmGet1Resource('NFNT', FixedFontLow);

  if (fntHandle != NULL) {
    fntPtr = MemHandleLock(fntHandle);
    FntDefineFont(255, fntPtr);
  }

  return fntHandle;
}

static Int16 pusherror(lua_State *L, Int16 err)
{
  lua_pushnil(L);
  lua_pushstring(L, strerror(err));
  lua_pushnumber(L, err);
  return 3;
}

static const struct luaL_reg screenlib[] = {
{"mode", screen_mode},
{"clear", screen_clear},
{"color", screen_color},
{"rgb", screen_rgb},
{"pos", screen_pos},
{"moveto", screen_moveto},
{"line", screen_line},
{"lineto", screen_lineto},
{"setpixel", screen_setpixel},
{"getpixel", screen_getpixel},
{"clip", screen_setcliping},
{"rect", screen_rect},
{"box", screen_box},
{"circle", screen_circle},
{"disc", screen_disc},
{"fill", screen_fill},
{"font", screen_font},
{"textsize", screen_textsize},
{"walk", screen_walk},
{"jump", screen_jump},
{"heading", screen_heading},
{"turn", screen_turn},
{NULL, NULL}
};


static const struct luaL_reg guilib[] = {
{"event", gui_event},
{"label", gui_label},
{"control", gui_control},
{"button", gui_button},
{"pbutton", gui_pushbutton},
{"rbutton", gui_repeatbutton},
{"selector", gui_selectortrigger},
{"checkbox", gui_checkbox},
{"slider", gui_slider},
{"field", gui_field},
{"fieldattr", gui_fieldattr},
{"list", gui_list},
{"popup", gui_popup},
{"gettext", gui_gettext},
{"settext", gui_settext},
{"setlist", gui_setlist},
{"inserttext", gui_inserttext},
{"getstate", gui_getstate},
{"setstate", gui_setstate},
{"setvisible", gui_setvisible},
{"setfocus", gui_setfocus},
{"nl", gui_nl},
{"tab", gui_tab},
{"alert", gui_alert},
{"confirm", gui_confirm},
{"input", gui_input},
{"destroy", gui_destroy},
{"title", gui_title},
{"menu", gui_menu},
{"gsi", gui_gsi},
{"selectdate", gui_selectdate},
{"selecttime", gui_selecttime},
{"selectcolor", gui_selectcolor},
{"dialog", gui_dialog},
{"sethandler", gui_sethandler},
{NULL, NULL}
};

static const struct luaL_reg bufferlib[] = {
{"read",  buffer_read},
{"write", buffer_write},
{"new",   buffer_new},
{"get",   buffer_get},
{"put",   buffer_put},
{"free",  buffer_free},
{"use",   buffer_use},
{NULL, NULL}
};

static const struct luaL_reg spritelib[] = {
{"init",   sprite_init},
{"finish", sprite_finish},
{"add",    sprite_add},
{"remove", sprite_remove},
{"update", sprite_update},
{NULL, NULL}
};

static const struct luaL_reg soundlib[] = {
{"beep", sound_beep},
{"tone", sound_tone},
{"midi", sound_midi},
{"play", sound_play},
{"stop", sound_stop},
{NULL, NULL}
};

static const struct luaL_reg resourcelib[] = {
{"list",  resource_list},
{"open",  resource_open},
{"close", resource_close},
{"get",   resource_get},
{"size",  resource_size},
{"draw",  resource_draw},
{"md5",   resource_md5},
{"call",  resource_call},
{NULL, NULL}
};

static app_constant constants[] = {
  {nilEvent,		"nilEvent"},
  {ioPendingEvent,	"ioPending"},
  {appStopEvent,	"appStop"},
  {sampleStopEvent,	"sampleStop"},
  {ctlSelectEvent,	"ctlSelect"},
  {lstSelectEvent,	"lstSelect"},
  {ctlRepeatEvent,	"ctlRepeat"},
  {popSelectEvent,	"popSelect"},
  {menuEvent,		"menuSelect"},
  {keyDownEvent,	"keyDown"},
  {penDownEvent,	"penDown"},
  {penUpEvent,		"penUp"},
  {penMoveEvent,	"penMove"},

  {pageUpChr,		"hardKeyUp"},
  {pageDownChr,		"hardKeyDown"},
  {vchrRockerLeft,	"hardKeyLeft"},
  {vchrRockerRight,	"hardKeyRight"},
  {vchrRockerCenter,	"hardKeyCenter"},
  {vchrHard1,		"hardKey1"},
  {vchrHard2,		"hardKey2"},
  {vchrHard3,		"hardKey3"},
  {vchrHard4,		"hardKey4"},
  {-1,""}
};


LUALIB_API void luaopen_graph (lua_State *L) {
  Int16 i;
  static char *gui_main = "function gui.main(t) repeat until gui.event(t) == appStop end";

  init();

  luaL_openlib(L, "gui",      guilib, 0);
  luaL_openlib(L, "screen",   screenlib, 0);
  luaL_openlib(L, "buffer",   bufferlib, 0);
  luaL_openlib(L, "sprite",   spritelib, 0);
  luaL_openlib(L, "sound",    soundlib, 0);
  luaL_openlib(L, "resource", resourcelib, 0);

  lua_pushliteral(L, "_G");
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  for (i = 0; constants[i].value >= 0; i++) {
    lua_pushstring(L, constants[i].name);
    lua_pushnumber(L, constants[i].value);
    lua_settable(L, -3);
  }
  lua_pushstring(L, "_APP_NAME");
  lua_pushstring(L, AppName());
  lua_settable(L, -3);

  lua_dobuffer(L, gui_main, strlen(gui_main), "gui_main");
}

LUALIB_API void luaclose_graph (lua_State *L) {
  finish(L);
}

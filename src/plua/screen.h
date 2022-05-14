#ifndef _SCREEN_H
#define _SCREEN_H

typedef struct {
  WinHandle wh;
  UInt32 width, height, depth;
  Boolean enableColor;
  Int16 x, y, maxY;
  UInt32 fg, bg;
  UInt16 font;
  RectangleType clip, clip0;
  double heading;
  Boolean highDensity;
  Int16 scrCoordinate;
  Int16 factorX;
  Int16 factorY;
  Boolean movable;
} DisplayType;

DisplayType *getdisplay(void);
void clipform(Int16);
void unclipform(void);
void resetform(void);
void restoremenu (void);
void setsclid(UInt16 fieldId, UInt16 sclId);
UInt16 getsclid(UInt16 fieldId);
UInt16 getfldid(UInt16 sclId);
BitmapPtr rsrc2bmp(Int16 rsrc);
MemHandle setfixedfont(void);

UInt16 create_title(char *title);
UInt16 create_control(Int16 style, char *name, UInt16 resId, char *file, UInt8 group, Int16 state, Int16 *x, Int16 *y, Int16 *width, Int16 *height, Int16 *font, Boolean leftAnchor, Boolean visible);
UInt16 create_slider (Int16 d, Int16 value, Int16 *x, Int16 *y, Int16 *width, Int16 *height);
UInt16 create_label(char *name, Int16 *px, Int16 *py, Int16 *pfont, Boolean visible);
UInt16 create_field(Int16 nlines, Int16 ncols, Int16 max, char *value,
                    Boolean editable, Boolean underlined, Int16 alignment,
                    Int16 *px, Int16 *py, Int16 *pwidth, Int16 *pheight,
                    Int16 *pfont);
UInt16 create_list(char **items, Int16 i, Int16 popup,
                   Int16 nlines, Int16 ncols, Int16 sel,
                   Int16 *px, Int16 *py, Int16 *pwidth, Int16 *pheight,
                   Int16 *pfont, Boolean leftAnchor);
Boolean gadget_event(EventPtr event);

void *plua_getwindow(void);
void plua_pcolor(long fg, long bg);
long plua_prgb(int r, int g, int b);
void plua_prect(int x, int y, int dx, int dy);
void plua_pbox(int x, int y, int dx, int dy);
void plua_pcircle(int x, int y, int rx, int ry);
void plua_pdisc(int x, int y, int rx, int ry);
void plua_pline(int x1, int y1, int x2, int y2);
void plua_plineto(int x, int y);
void plua_pmoveto(int x, int y);
void plua_print(char *s);
void plua_ppos(int *x, int *y);
void plua_ptextsize(char *s, int *w, int *h);
void plua_getrgb(long c, int *r, int *g, int *b);
void plua_getcolor(long *fg, long *bg);

#endif

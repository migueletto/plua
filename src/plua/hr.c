#include "p.h"
#include "screen.h"
#include "hr.h"

void LongToRGB(UInt32 c, RGBColorType *rgb) {
  rgb->r = (c & 0xff0000) >> 16;
  rgb->g = (c & 0x00ff00) >> 8;
  rgb->b = (c & 0x0000ff);
}

UInt32 RGBToLong(RGBColorType *rgb) {
  return (((UInt32)rgb->r) << 16) |
         (((UInt32)rgb->g) << 8) |
          ((UInt32)rgb->b);
}

Err hrWinScreenMode(WinScreenModeOperation op, UInt32 *width, UInt32 *height, UInt32 *depth, Boolean *enableColor)
{
  Err err;
  DisplayType *disp = getdisplay();

  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  err = WinScreenMode(op, width, height, depth, enableColor);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  return err;
}

void hrWinEraseRectangle(RectangleType *r, UInt16 cornerDiam)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinEraseRectangle(r, cornerDiam);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinSetClip(RectangleType *r)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinSetClip(r);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinPaintChars(const Char *chars, Int16 len, Coord x, Coord y)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinPaintChars(chars, len, x, y);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinDrawChars(const Char *chars, Int16 len, Coord x, Coord y)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinDrawChars(chars, len, x, y);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinFillRectangle(RectangleType *r, UInt16 cornerDiam)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinFillRectangle(r, cornerDiam);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinInvertRectangle(RectangleType *r, UInt16 cornerDiam)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinInvertRectangle(r, cornerDiam);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinPaintLine(Coord x1, Coord y1, Coord x2, Coord y2)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinPaintLine(x1, y1, x2, y2);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinPaintPixel(Coord x, Coord y)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinPaintPixel(x, y);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

UInt32 hrWinGetPixel(Coord x, Coord y)
{
  UInt32 p;
  DisplayType *disp = getdisplay();

  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);

  if (disp->depth < 16)
    p = WinGetPixel(x, y);
  else {
    RGBColorType rgb;
    WinGetPixelRGB(x, y, &rgb);
    p = RGBToLong(&rgb);
  }

  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  return p;
}

void hrWinPaintRectangleFrame(FrameType frame, RectangleType *r)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinPaintRectangleFrame(frame, r);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinPaintBitmap(BitmapType *bitmap, Coord x, Coord y)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinPaintBitmap(bitmap, x, y);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinDrawRectangleFrame(FrameType frame, RectangleType *r)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinDrawRectangleFrame(frame, r);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinEraseRectangleFrame(FrameType frame, RectangleType *r)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinEraseRectangleFrame(frame, r);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinDrawLine(Coord x1, Coord y1, Coord x2, Coord y2)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinDrawLine(x1, y1, x2, y2);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrWinEraseLine(Coord x1, Coord y1, Coord x2, Coord y2)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  WinEraseLine(x1, y1, x2, y2);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
}

void hrFldGetBounds(FieldType *fld, RectangleType *r)
{
  DisplayType *disp = getdisplay();
  FldGetBounds(fld, r);
  if (disp->highDensity) {
    r->topLeft.x *= disp->factorX;
    r->topLeft.y *= disp->factorY;
    r->extent.x *= disp->factorX;
    r->extent.y *= disp->factorY ;
  }
}

void hrFrmGetObjectBounds(FormType *frm, UInt16 objIndex, RectangleType *r)
{
  DisplayType *disp = getdisplay();
  FrmGetObjectBounds(frm, objIndex, r);
  if (disp->highDensity) {
    r->topLeft.x *= disp->factorX;
    r->topLeft.y *= disp->factorY;
    r->extent.x *= disp->factorX;
    r->extent.y *= disp->factorY;
  }
}

ControlType *hrCtlNewControl(void **form, UInt16 ID, ControlStyleType style, const Char *text, Coord x, Coord y, Coord width, Coord height, UInt16 font, UInt8 group, Boolean leftAnchor)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) {
    x = (x+1) / disp->factorX;
    y = (y+1) / disp->factorY;
    width /= disp->factorX;
    height /= disp->factorY;
  }
  return CtlNewControl(form, ID, style, text, x, y, width, height, font, group, leftAnchor);
}

GraphicControlType *hrCtlNewGraphicControl(void **form, UInt16 ID, ControlStyleType style, DmResID bitmapID, DmResID selectedBitmapID, Coord x, Coord y, Coord width, Coord height, UInt8 group, Boolean leftAnchor)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) {
    x = (x+1) / disp->factorX;
    y = (y+1) / disp->factorY;
    width /= disp->factorX;
    height /= disp->factorY;
  }
  return CtlNewGraphicControl(form, ID, style, bitmapID, selectedBitmapID, x, y, width, height, group, leftAnchor);
}

SliderControlType *hrCtlNewSliderControl(void **form, UInt16 ID, ControlStyleType style, DmResID thumbID, DmResID backgroundID, Coord x, Coord y, Coord width, Coord height, UInt16 minValue, UInt16 maxValue, UInt16 pageSize, UInt16 value)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) {
    x = (x+1) / disp->factorX;
    y = (y+1) / disp->factorY;
    width /= disp->factorX;
    height /= disp->factorY;
  }
  return CtlNewSliderControl(form, ID, style, thumbID, backgroundID, x, y, width, height, minValue, maxValue, pageSize, value);
}

FieldType *hrFldNewField(void **form, UInt16 id, Coord x, Coord y, Coord width, Coord height, UInt16 font, UInt32 maxChars, Boolean editable, Boolean underlined, Boolean singleLine, Boolean dynamicSize,JustificationType justification, Boolean autoShift, Boolean hasScrollBar, Boolean numeric)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) {
    x /= disp->factorX;
    y /= disp->factorY;
    width /= disp->factorX;
    height /= disp->factorY;
  }
  return FldNewField(form, id, x, y, width, height, font, maxChars, editable,
                     underlined, singleLine, dynamicSize, justification,
                     autoShift, hasScrollBar, numeric);
}

void hrRctSetRectangle(RectangleType *r, Coord left, Coord top, Coord width, Coord height)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) {
    left /= disp->factorX;
    top /= disp->factorY;
    width /= disp->factorX;
    height /= disp->factorY;
  }
  RctSetRectangle(r, left, top, width, height);
}

Err hrLstNewList(void **form, UInt16 id, Coord x, Coord y, Coord width, Coord height, UInt16 font, Int16 visibleItems, Int16 triggerId)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) {
    x = (x+1) / disp->factorX;
    y = (y+1) / disp->factorY;
    width /= disp->factorX;
    height /= disp->factorY;
  }
  return LstNewList(form, id, x, y, width, height, font, visibleItems, triggerId);
}

FormLabelType *hrFrmNewLabel(FormType **form, UInt16 id, const char *text, Coord x, Coord y, FontID font)
{
  DisplayType *disp = getdisplay();
  if (disp->highDensity) {
    x = (x+1) / disp->factorX;
    y = (y+1) / disp->factorY;
  }
  return FrmNewLabel(form, id, text, x, y, font);
}

Int16 hrFntCharHeight(void)
{
  Int16 h;
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  h = FntCharHeight();
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  return h;
}

Int16 hrFntCharWidth(char c)
{
  Int16 w;
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  w = FntCharWidth(c);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  return w;
}

Int16 hrFntAverageCharWidth(void)
{
  Int16 w;
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  w = FntAverageCharWidth();
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  return w;
}

Int16 hrFntCharsWidth(char *s, Int16 len)
{
  Int16 w, i;
  DisplayType *disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
  for (i = 0, w = 0; i < len; i++)
    w += FntCharWidth(s[i]);
  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  return w;
}

void hrBmpGlueGetDimensions(const BitmapType *bmp, Coord *width, Coord *height, UInt16 *rowBytes)
{
  DisplayType *disp = getdisplay();
  BmpGlueGetDimensions(bmp, width, height, rowBytes);
  if (disp->highDensity && BmpGetDensity(bmp) == kDensityLow) {
    *width *= disp->factorX;
    *height *= disp->factorY;
  }
}

void setcolors(void)
{
  DisplayType *disp = getdisplay();
  RGBColorType rgb;

  rgb.r = 0;
  rgb.g = 0;
  rgb.b = 0;
  disp->fg = disp->depth < 16 ? WinRGBToIndex(&rgb) : RGBToLong(&rgb);

  rgb.r = 255;
  rgb.g = 255;
  rgb.b = 255;
  disp->bg = disp->depth < 16 ? WinRGBToIndex(&rgb) : RGBToLong(&rgb);
}

void setforecolor(UInt32 c)
{
  DisplayType *disp = getdisplay();

  if (disp->depth < 16)
    WinSetForeColor(c);
  else {
    RGBColorType rgb, old;
    LongToRGB(c, &rgb);
    WinSetForeColorRGB(&rgb, &old);
  }
}

void settextcolor(UInt32 c)
{
  DisplayType *disp = getdisplay();

  if (disp->depth < 16)
    WinSetTextColor(c);
  else {
    RGBColorType rgb, old;
    LongToRGB(c, &rgb);
    WinSetTextColorRGB(&rgb, &old);
  }
}

void setbackcolor(UInt32 c)
{
  DisplayType *disp = getdisplay();

  if (disp->depth < 16)
    WinSetBackColor(c);
  else {
    RGBColorType rgb, old;
    LongToRGB(c, &rgb);
    WinSetBackColorRGB(&rgb, &old);
  }
}

Boolean validwindow(void)
{
  return FrmGetWindowHandle(FrmGetActiveForm()) == WinGetActiveWindow();
}

#include "p.h"
#include "hr.h"
#include "screen.h"
#include "endian.h"
#include "bmp.h"

#define LINES_PER_BLOCK 32

Err BmpGetSize(FILE *f, UInt16 *width, UInt16 *height)
{
  WindowsBitmap bmp;

  if (fread(&bmp, 1, sizeof(bmp), f) != sizeof(bmp))
    return -1;

  bmp.compression = ByteSwap32(bmp.compression);

  if (bmp.type != 0x424d || bmp.planes != 0x0100 || bmp.compression != BMP_RGB){
    errno = EINVAL;
    return -1;
  }

  *width = ByteSwap32(bmp.width);
  *height = ByteSwap32(bmp.height);

  fseek(f, 0, SEEK_SET);
  errno = 0;

  return 0;
}

WinHandle BmpRead(FILE *f, UInt16 *width, UInt16 *height)
{
  DisplayType *disp;
  WinHandle wh, oldWh = NULL;
  BitmapType *pbmp;
  WindowsBitmap bmp;
  WindowsBitmapColor *palette, *c;
  UInt32 colorsize, linesize, blocksize, fg, err;
  RGBColorType rgb, oldRgb;
  UInt16 x, y, cx, x0, bit, word;
  UInt8 *block, *blockend, *line, depth;
  Err aux;

  if (fread(&bmp, 1, sizeof(bmp), f) != sizeof(bmp))
    return NULL;

  bmp.fileSize = ByteSwap32(bmp.fileSize);
  bmp.dataOffset = ByteSwap32(bmp.dataOffset);
  bmp.headerSize = ByteSwap32(bmp.headerSize);
  bmp.width = ByteSwap32(bmp.width);
  bmp.height = ByteSwap32(bmp.height);
  bmp.bpp = ByteSwap16(bmp.bpp);
  bmp.compression = ByteSwap32(bmp.compression);
  bmp.dataSize = ByteSwap32(bmp.dataSize);
  bmp.colors = ByteSwap32(bmp.colors);

  if (bmp.type != 0x424d || bmp.planes != 0x0100 || bmp.compression != BMP_RGB){
    errno = EINVAL;
    return NULL;
  }

  switch (bmp.bpp) {
    case 1:
    case 4:
    case 8:
    case 24:
      break;
    default:
      errno = EINVAL;
      return NULL;
  }

  if (bmp.colors == 0)
    bmp.colors = 1 << bmp.bpp;

  if (bmp.colors > 256)
    bmp.colors = 0;

  colorsize = bmp.colors * sizeof(WindowsBitmapColor);
  linesize = ((bmp.width * bmp.bpp + 31) / 32) * 4;
  blocksize = LINES_PER_BLOCK * linesize;

  if (linesize * bmp.height != bmp.dataSize) {
    errno = EINVAL;
    return NULL;
  }

  if (bmp.colors) {
    if ((palette = calloc(bmp.colors, sizeof(WindowsBitmapColor))) == NULL)
      return NULL;

    if (fread(palette, 1, colorsize, f) != colorsize) {
      err = errno;
      free(palette);
      errno = err;
      return NULL;
    }
  } else
    palette = NULL;

  if ((block = calloc(blocksize, 1)) == NULL) {
    if (palette) free(palette);
    errno = ENOMEM;
    return NULL;
  }

  if (*width == 0 || *width > bmp.width)
    *width = bmp.width;
  if (*height == 0 || *height > bmp.height)
    *height = bmp.height;

  disp = getdisplay();

  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);

  if ((wh = WinCreateOffscreenWindow(*width, *height, nativeFormat, &aux)) == NULL) {
    if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
    free(block);
    if (palette) free(palette);
    errno = ENOMEM;
    return NULL;
  }

  oldWh = WinSetDrawWindow(wh);
  pbmp = WinGetBitmap(wh);
  depth = BmpGlueGetBitDepth(pbmp);

  if (depth != 8 && depth != 16) {
    if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
    WinSetDrawWindow(oldWh);
    WinDeleteWindow(wh, false);
    free(block);
    if (palette) free(palette);
    errno = EINVAL;
    return NULL;
  }

  blockend = block + blocksize;
  line = blockend;

  for (y = 0; y < bmp.height; y++) {
    if (line >= blockend) {
      if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
      if (fread(block, 1, blocksize, f) < 0) {
        err = errno;
        WinSetDrawWindow(oldWh);
        WinDeleteWindow(wh, false);
        free(block);
        if (palette) free(palette);
        errno = err;
        return NULL;
      }
      if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);
      line = block;
    }
    bit = 7;
    for (x = 0, x0 = 0; x < bmp.width; x++) {
      switch (bmp.bpp) {
        case 1:
          cx = (line[x0] >> bit) & 0x01;
          if (bit == 0)
            bit = 7, x0++;
          else
            bit--;
          break;
        case 4:
          if (x % 2)
            cx = line[x0] & 0x0F, x0++;
          else
            cx = line[x0] >> 4;
          break;
        case 8:
          cx = line[x0++];
          break;
        case 16:
          word = ((UInt16)line[x0+1] << 8) | line[x0];
          x0 += 2;
          rgb.r = (((word & 0x7C00) >> 7) | ((word & 0x1C00) >> 10));
          rgb.g = (((word & 0x03E0) >> 2) | ((word & 0x00e0) >> 5));
          rgb.b = (((word & 0x001F) << 3) | (word & 0x0007));
          cx = 0;
          break;
        default: // 24
          rgb.b = line[x0++];
          rgb.g = line[x0++];
          rgb.r = line[x0++];
          cx = 0;
      }

      if (bmp.colors) {
        if (cx >= bmp.colors)
          cx = 0;
        c = &palette[cx];
        rgb.r = c->red;
        rgb.g = c->green;
        rgb.b = c->blue;
      }

      if (depth == 8) {
        fg = WinRGBToIndex(&rgb);
        WinSetForeColor(fg);
      } else
        WinSetForeColorRGB(&rgb, &oldRgb);

      if (x < *width && (bmp.height - y - 1) < *height)
        WinPaintPixel(x, bmp.height - y - 1);
    }
    line += linesize;
  }

  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  WinSetDrawWindow(oldWh);

  free(block);
  if (palette) free(palette);

  errno = 0;
  return wh;
}

Err BmpWrite(FILE *f, WinHandle wh, UInt32 *size)
{
  DisplayType *disp;
  WinHandle oldWh;
  BitmapType *pbmp;
  WindowsBitmap bmp;
  WindowsBitmapColor *palette;
  UInt32 colorsize, linesize, blocksize;
  RGBColorType rgb, *rgbt;
  UInt16 err, i, x, y, x0, width, height, colors, rowBytes;
  UInt8 *block, *blockend, *line, depth;

  bmp.type = 0x424d;
  bmp.reserved = 0;
  bmp.planes = 0x0100;
  bmp.compression = BMP_RGB;

  pbmp = WinGetBitmap(wh);
  depth = BmpGlueGetBitDepth(pbmp);
  BmpGlueGetDimensions(pbmp, &width, &height, &rowBytes);

  switch (depth) {
    case 8:
    case 16:
      break;
    default:
      errno = EINVAL;
      return -1;
  }

  bmp.bpp = 8;
  bmp.width = width;
  bmp.height = height;

  colors = 256;
  bmp.colors = colors;
  bmp.iColors = colors;

  colorsize = colors * sizeof(WindowsBitmapColor);
  linesize = ((width * 8 + 31) / 32) * 4;
  blocksize = LINES_PER_BLOCK * linesize;

  bmp.headerSize = 0x28;
  bmp.dataOffset = sizeof(bmp) + colorsize;
  bmp.dataSize = height * linesize;
  bmp.fileSize = bmp.dataOffset + bmp.dataSize;
  bmp.hRes = 0x0EC4;
  bmp.vRes = 0x0EC4;

  *size = bmp.fileSize;

  bmp.fileSize = ByteSwap32(bmp.fileSize);
  bmp.dataOffset = ByteSwap32(bmp.dataOffset);
  bmp.headerSize = ByteSwap32(bmp.headerSize);
  bmp.width = ByteSwap32(bmp.width);
  bmp.height = ByteSwap32(bmp.height);
  bmp.bpp = ByteSwap16(bmp.bpp);
  bmp.dataSize = ByteSwap32(bmp.dataSize); 
  bmp.vRes = ByteSwap32(bmp.vRes); 
  bmp.hRes = ByteSwap32(bmp.hRes); 
  bmp.colors = ByteSwap32(bmp.colors);
  bmp.iColors = ByteSwap32(bmp.iColors);

  if (fwrite(&bmp, 1, sizeof(bmp), f) != sizeof(bmp))
    return -1; 

  if (colors) {
    if ((rgbt = calloc(colors, sizeof(RGBColorType))) == NULL)
      return -1;

    if (WinPalette(winPaletteGet, 0, colors, rgbt) != 0) {
      free(rgbt);
      return -1;
    }

    if ((palette = calloc(colors, sizeof(WindowsBitmapColor))) == NULL) {
      free(rgbt);
      errno = ENOMEM;
      return -1;
    }

    for (i = 0; i < colors; i++) {
      palette[i].red = rgbt[i].r;
      palette[i].green = rgbt[i].g;
      palette[i].blue = rgbt[i].b;
    }

    free(rgbt);

    if (fwrite(palette, 1, colorsize, f) != colorsize) {
      free(palette);
      errno = ENOMEM;
      return -1; 
    }

    free(palette);
  }

  if ((block = calloc(blocksize, 1)) == NULL)
    return -1;

  disp = getdisplay();

  oldWh = WinSetDrawWindow(wh);
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);

  blockend = block + blocksize;
  line = block;

  for (y = height; y > 0; y--) {
    for (x = 0, x0 = 0; x < width; x++) {
      if (depth == 8) {
        line[x0++] = WinGetPixel(x, y-1);
      } else { // 16
        WinGetPixelRGB(x, y-1, &rgb);
        line[x0++] = WinRGBToIndex(&rgb);
      }
    }
    line += linesize;

    if (line >= blockend) {
      if (fwrite(block, 1, blocksize, f) != blocksize) {
        err = errno;
        free(block);
        errno = err;
        if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
        WinSetDrawWindow(oldWh);
        return -1;
      }
      line = block;
    }
  }

  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
  WinSetDrawWindow(oldWh);
    
  if (line > block) {
    blocksize = line - block;
    if (fwrite(block, 1, blocksize, f) != blocksize) {
      err = errno;
      free(block);
      errno = err;
      return -1;
    }
  }

  free(block);

  errno = 0;
  return 0;
}

Err bmpsize(UInt16 id, UInt16 *bw, UInt16 *bh)
{
  MemHandle h;
  BitmapPtr p;
  UInt16 rb;

  if ((h = DmGetResource(bitmapRsc, id)) == NULL &&
      (h = DmGetResource(iconType, id)) == NULL &&
      (h = DmGetResource('abmp', id)) == NULL)
    return -1;

  if ((p = MemHandleLock(h)) == NULL) {
    DmReleaseResource(h);
    return -1;
  }

  hrBmpGlueGetDimensions(p, bw, bh, &rb);
  MemHandleUnlock(h);
  DmReleaseResource(h);

  return 0;
}

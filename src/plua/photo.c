#include "p.h"
#include "hr.h"
#include "screen.h"
#include "error.h"
#include "bmp.h"
#include "photo.h"

#include <PalmPhoto.h>

static Err photoPrepare(FILE *f, PalmPhotoFileLocation *loc);
static PalmPhotoHandle photoOpen(FILE *f, UInt16 *width, UInt16 *height);
static Boolean displayCallback(void *userData);
static Err writeCallback(void *buf, UInt32 *size, void *userData);
static Boolean windowsBitmap(FILE *f);

typedef struct {
  UInt16 dx, dy;
  UInt16 x, y, c;
} BmpPos;

static Int16 refNum;

Err PhotoInit(void)
{
  Err err;

  err = SysLibFind(PalmPhotoLibName, &refNum);
  if (err) err = SysLibLoad(PalmPhotoLibTypeID, PalmPhotoLibCreatorID, &refNum);
  if (!err) err = PalmPhotoLibOpen(refNum);
  if (err) refNum = -1;

  return err;
}

void PhotoFinish(void)
{
  if (refNum != -1) {
    PalmPhotoLibClose(refNum);
    refNum = -1;
  }
}

Err PhotoGetSize(FILE *f, UInt16 *width, UInt16 *height)
{
  PalmPhotoHandle h;

  if (windowsBitmap(f))
    return BmpGetSize(f, width, height);

  if ((h = photoOpen(f, width, height)) == NULL)
    return -1;

  PalmPhotoCloseImage(refNum, h);
  return 0;
}

WinHandle PhotoRead(FILE *f, UInt16 *width, UInt16 *height)
{
  PalmPhotoHandle h;
  PalmPhotoDisplayParam param;
  DisplayType *disp;
  WinHandle wh;
  Err err;

  if (windowsBitmap(f))
    return BmpRead(f, width, height);

  if ((h = photoOpen(f, width, height)) == NULL)
    return NULL;

  disp = getdisplay();
  if (disp->highDensity) WinSetCoordinateSystem(disp->scrCoordinate);

  if ((wh = WinCreateOffscreenWindow(*width, *height, nativeFormat, &err)) == NULL) {
    if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);
    ErrorMsg("PhotoRead: window = %d", err);
    PalmPhotoCloseImage(refNum, h);
    errno = mapPalmOsErr(err);
    return NULL;
  }

  param.winH = wh;
  param.rect.topLeft.x = 0;
  param.rect.topLeft.y = 0;
  param.rect.extent.x = *width;
  param.rect.extent.y = *height;
  param.displayCallback = displayCallback;

  err = PalmPhotoDisplayImage(refNum, h, &param);
  errno = mapPalmOsErr(err);
  PalmPhotoCloseImage(refNum, h);

  if (disp->highDensity) WinSetCoordinateSystem(kCoordinatesStandard);

  if (err != 0)
    ErrorMsg("PhotoRead: display = %d", err);

  return wh;
}

Err PhotoWrite(char *name, Int16 format, WinHandle wh, UInt32 *size)
{
  PalmPhotoCreateParam cparam;
  PalmPhotoReadWriteParam wparam;
  PalmPhotoHandle h;
  BitmapType *pbmp;
  WinHandle oldWh;
  UInt16 width, height, rowBytes, depth;
  BmpPos pos;
  FILE *f;
  Err err;

  if ((f = fopen(name, "w")) == NULL)
    return -1;

  switch (format) {
    case PHOTO_BMP:
      BmpWrite(f, wh, size);
      err = errno;
      fclose(f);
      errno = err;
      break;

    case PHOTO_JPG:
       if (photoPrepare(f, &cparam.fileLocation) == 0) {
         fclose(f);

         pbmp = WinGetBitmap(wh);
         depth = BmpGlueGetBitDepth(pbmp);
         BmpGlueGetDimensions(pbmp, &width, &height, &rowBytes);

         cparam.imageInfo.width = width;
         cparam.imageInfo.height = height;
         cparam.imageInfo.bitsPerPixel = depth;
         cparam.imageInfo.fileFormat = palmPhotoJPEGFileFormat;
         cparam.imageInfo.imageQuality = palmPhotoMediumQuality;

         if ((h = PalmPhotoCreateImage(refNum, &cparam, &err)) != NULL) {
           wparam.imageFormat = palmPhotoRGB888FileFormat;
           wparam.rwCallback = writeCallback;
           wparam.userDataP = &pos;
           pos.dx = width;
           pos.dy = height;
           pos.x = pos.y = pos.c = 0;

           oldWh = WinSetDrawWindow(wh);
           err = PalmPhotoWriteImage(refNum, h, &wparam);
           WinSetDrawWindow(oldWh);
           errno = mapPalmOsErr(err);

         } else {
           errno = mapPalmOsErr(err);
         }

       } else
         fclose(f);
       break;

    default:
      errno = EINVAL;
  }

  return errno ? -1 : 0;
}

static Err photoPrepare(FILE *f, PalmPhotoFileLocation *loc)
{
  UInt16 cardNo;
  LocalID dbID;

  if (refNum == -1) {
    ErrorMsg("photoPrepare: refNum = -1");
    errno = EINVAL;
    return -1;
  }

  MemSet(loc, sizeof(PalmPhotoFileLocation), 0);

  switch (f->type) {
    case FILE_STREAM:
      DmOpenDatabaseInfo(f->dbRef, &dbID, NULL, NULL, &cardNo, NULL);
      DmDatabaseInfo(cardNo, dbID, loc->file.StreamFile.name,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        &loc->file.StreamFile.type, &loc->file.StreamFile.creator);
      loc->fileLocationType = palmPhotoStreamLocation;
      break;
    case FILE_VFS:
      loc->fileLocationType = palmPhotoVFSLocation;
      loc->file.VFSFile.volumeRef = f->vol;
      StrNCopy(loc->file.VFSFile.name, f->realname, PALM_PHOTO_MAX_PATH);
      break;
    case FILE_RESOURCE:
      loc->fileLocationType = palmPhotoMemoryLocation;
      loc->file.MemoryFile.bufferP = f->record;
      loc->file.MemoryFile.bufferSize = f->recordSize;
      break;
    default:
      ErrorMsg("photoPrepare: type = %d", f->type);
      errno = EINVAL;
      return -1;
  }

  errno = 0;
  return 0;
}

static PalmPhotoHandle photoOpen(FILE *f, UInt16 *width, UInt16 *height)
{
  PalmPhotoHandle h;
  PalmPhotoFileLocation loc;
  PalmPhotoImageInfo info;
  Err err;

  if (photoPrepare(f, &loc) != 0)
    return NULL;

  if ((h = PalmPhotoOpenImage(refNum, &loc, &err)) == NULL) {
    ErrorMsg("photoOpen: open = %d", err);
    errno = mapPalmOsErr(err);
    return NULL;
  }

  if ((err = PalmPhotoGetImageInfo(refNum, h, &info)) != 0) {
    ErrorMsg("photoOpen: info = %d", err);
    PalmPhotoCloseImage(refNum, h);
    errno = mapPalmOsErr(err);
    return NULL;
  }

  *width = info.width;
  *height = info.height;

  errno = 0;
  return h;
}

static Boolean windowsBitmap(FILE *f)
{
  UInt8 buf[2];

  if (fread(buf, 1, 2, f) != 2)
    return false;

  fseek(f, 0, SEEK_SET);

  return (buf[0] == 0x42 && buf[1] == 0x4d);
}

static Boolean displayCallback(void *userData)
{
  return true;
}

static Err writeCallback(void *buf, UInt32 *size, void *userData)
{
 // During write operations, *size contains the max size of the
 // buffer used for writing. *size is set to the amount of data to
 // write before the callback returns. Setting *size to 0 means
 // no more data to write and the write process should stop.
 // The write calllback can be stopped at any time by the
 // callback returning the error code palmPhotoLibErrAbortReadWrite.

  RGBColorType rgb;
  BmpPos *pos;
  UInt32 i;
  UInt8 *b;

  pos = (BmpPos *)userData;
  b = (UInt8 *)buf;

  for (i = 0; pos->y < pos->dy;) {
    for (; pos->x < pos->dx;) {
      WinGetPixelRGB(pos->x, pos->y, &rgb);
      switch (pos->c) {
        case 0:
          b[i++] = rgb.r;
          if (i >= *size) break;
          // fall-through
        case 1:
          b[i++] = rgb.g;
          if (i >= *size) break;
          // fall-through
        case 2:
          b[i++] = rgb.b;
          pos->x++;
          if (pos->x == pos->dx) {
            pos->x = 0;
            pos->y++;
          }
      }
      pos->c = (pos->c + 1) % 3;
      if (i >= *size) break;
    }
    if (i >= *size) break;
  }
  *size = i;

  return 0;
}

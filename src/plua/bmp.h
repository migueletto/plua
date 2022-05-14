typedef struct {
  UInt16 type;
  UInt32 fileSize;
  UInt32 reserved;
  UInt32 dataOffset;
  UInt32 headerSize;
  UInt32 width;
  UInt32 height;
  UInt16 planes;
  UInt16 bpp;
  UInt32 compression;
  UInt32 dataSize;
  UInt32 hRes;
  UInt32 vRes;
  UInt32 colors;
  UInt32 iColors;
} WindowsBitmap;

typedef struct {
  UInt8 blue;
  UInt8 green;
  UInt8 red;
  UInt8 pad;
} WindowsBitmapColor;

#define BMP_RGB		0
#define BMP_RLE4	1
#define BMP_RLE8	2
#define BBMPTFIELDS	3

Err BmpGetSize(FILE *f, UInt16 *width, UInt16 *height) SEC("aux");
WinHandle BmpRead(FILE *f, UInt16 *width, UInt16 *height) SEC("aux");
Err BmpWrite(FILE *f, WinHandle wh, UInt32 *size) SEC("aux");
Err bmpsize(UInt16 id, UInt16 *bw, UInt16 *bh);


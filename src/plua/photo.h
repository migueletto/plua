#define PHOTO_BMP 1
#define PHOTO_JPG 2

Err PhotoInit(void);
void PhotoFinish(void);

Err PhotoGetSize(FILE *f, UInt16 *width, UInt16 *height);
WinHandle PhotoRead(FILE *f, UInt16 *width, UInt16 *height);
Err PhotoWrite(char *name, Int16 format, WinHandle wh, UInt32 *size);

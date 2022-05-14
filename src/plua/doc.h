#define DocAppID	'REAd'
#define DocDBType	'TEXt'

#define BACKSIZE 	2048

typedef struct {
  Int16  version;
  Int16  unknown1;
  Int32  textSize;
  Int16  numRecs;
  Int16  size;
  Int32  unknown2;
} DocHeader;

typedef struct {
  DmOpenRef dbRef;
  UInt16 currentRec;
  char *currentBuf;
  UInt16 currentIndex;
  UInt16 currentSize;
  UInt16 numRecs;
  UInt8 state;
  UInt8 aux;
  UInt16 auxs;
  char *backBuf;
  UInt16 backIndex;
  Int32 index;
  DocHeader header;
} DocType;

DocType *DocOpen(char *name, UInt16 mode, Err *err) SEC("aux");
Err DocCreate(char *name) SEC("aux");
Err DocClose(DocType *docRef) SEC("aux");
Int32 DocRead(DocType *docRef, char *buf, Int32 n) SEC("aux");
Boolean DocEOF(DocType *docRef) SEC("aux");

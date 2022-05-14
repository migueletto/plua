#include "p.h"
#include "doc.h"
#include "db.h"

static Int32 copytext(char *dst, Int32 n, DocType *docRef) SEC("aux");
static Int32 uncompress(char *dst, Int32 n, DocType *docRef) SEC("aux");

Err DocCreate(char *name)
{
  DocHeader header, *rec;
  DmOpenRef docRef;
  UInt16 index;
  UInt8 buf, *b;
  Err err;

  if ((err = DbCreate(name, DocDBType, DocAppID)) != 0)
    return err;

  if ((docRef = DbOpenByName(name, dmModeWrite, &err)) == NULL)
    return err;

  if ((err = DbCreateRec(docRef, &index, sizeof(header), 0)) != 0) {
    DbClose(docRef);
    return err;
  }

  if ((rec = (DocHeader *)DbOpenRec(docRef, index, &err)) == NULL) {
    DbClose(docRef);
    return err;
  }

  header.version = 1;
  header.unknown1 = 0;
  header.textSize = 1;
  header.numRecs = 1;
  header.size = 4096;
  header.unknown2 = 0;

  if ((err = DmWrite(rec, 0, &header, sizeof(header))) != 0) {
    DbCloseRec(docRef, index, (char *)rec);
    DbClose(docRef);
    return err;
  }
  DbCloseRec(docRef, index, (char *)rec);

  if ((err = DbCreateRec(docRef, &index, 1, 0)) != 0) {
    DbClose(docRef);
    return err;
  }

  if ((b = (UInt8 *)DbOpenRec(docRef, index, &err)) == NULL) {
    DbClose(docRef);
    return err;
  }

  buf = '\n';

  if ((err = DmWrite(b, 0, &buf, 1)) != 0) {
    DbCloseRec(docRef, index, (char *)b);
    DbClose(docRef);
    return err;
  }
  DbCloseRec(docRef, index, (char *)b);
  DbClose(docRef);

  return 0;
}

DocType *DocOpen(char *name, UInt16 mode, Err *err)
{
  DocHeader *header;
  DocType *docRef;

  if ((docRef = (DocType *)malloc(sizeof(DocType))) == NULL) {
    *err = memErrNotEnoughSpace;
    return NULL;
  }

  if ((docRef->dbRef = DbOpenByName(name, mode, err)) == NULL) {
    free(docRef);
    return NULL;
  }

  if ((header = (DocHeader *)DbOpenRec(docRef->dbRef, 0, err)) == NULL) {
    DocClose(docRef->dbRef);
    free(docRef);
    return NULL;
  }

  if (header->version == 1)
    docRef->backBuf = NULL;
  else if ((docRef->backBuf = malloc(BACKSIZE)) == NULL) {
    DbCloseRec(docRef->dbRef, 0, (char *)header);
    DocClose(docRef->dbRef);
    free(docRef);
    *err = memErrNotEnoughSpace;
    return NULL;
  }

  docRef->header.version = header->version;
  docRef->header.unknown1 = header->unknown1;
  docRef->header.textSize = header->textSize;
  docRef->header.numRecs = header->numRecs;
  docRef->header.size = header->size;
  docRef->header.unknown2 = header->unknown2;

  DbCloseRec(docRef->dbRef, 0, (char *)header);

  docRef->currentRec = 0;
  docRef->currentBuf = NULL;
  docRef->currentIndex = 0;
  docRef->currentSize = 0;
  docRef->numRecs = DbNumRecords(docRef->dbRef);
  docRef->state = 0;
  docRef->backIndex = 0;
  docRef->index = 0;

  *err = 0;
  return docRef;
}

Err DocClose(DocType *docRef)
{
  Err err = 0;

  if (!docRef)
    return -1; // XXX

  if (docRef->backBuf)
    free(docRef->backBuf);

  if (docRef->dbRef) {
    if (docRef->currentRec && docRef->currentBuf)
      DbCloseRec(docRef->dbRef, docRef->currentRec, docRef->currentBuf);
    err = DbClose(docRef->dbRef);
  }

  free(docRef);
  return err;
}

Int32 DocRead(DocType *docRef, char *buf, Int32 n)
{
  Int32 t, k;
  Err err;

  if (!docRef || !docRef->dbRef || !buf || n < 0)
    return -1;

  if ((docRef->index + n) > docRef->header.textSize)
    n = docRef->header.textSize - docRef->index;

  for (t = 0; n;) {
    if (docRef->currentIndex == docRef->currentSize) {
      if (docRef->currentBuf)
        DbCloseRec(docRef->dbRef, docRef->currentRec, docRef->currentBuf);
      docRef->currentRec++;
      docRef->currentBuf = NULL;
      docRef->currentIndex = 0;
      docRef->currentSize = 0;
      if (docRef->currentRec == docRef->numRecs)
        break;
      if ((docRef->currentBuf = DbOpenRec(docRef->dbRef,
              docRef->currentRec, &err)) == NULL)
	break;
      docRef->currentSize = MemPtrSize(docRef->currentBuf);
    }

    k = docRef->header.version == 1 ?
	    copytext(&buf[t], n, docRef) :
	    uncompress(&buf[t], n, docRef);
    t += k;
    n -= k;
  }

  docRef->index += t;
  return t;
}

Boolean DocEOF(DocType *docRef)
{
  if (!docRef || !docRef->dbRef)
    return true;

  return docRef->currentRec == docRef->numRecs;
}

static Int32 copytext(char *dst, Int32 n, DocType *docRef)
{
  Int32 i;

  for (i = 0; i < n && docRef->currentIndex < docRef->currentSize; i++)
    dst[i] = docRef->currentBuf[docRef->currentIndex++];

  return i;
}

static Int32 uncompress(char *dst, Int32 n, DocType *docRef)
{
  Int32 i;
  UInt16 m;
  UInt8 c;

  for (i = 0; i < n && docRef->currentIndex < docRef->currentSize;) {
    switch (docRef->state) {
      case 0:
        c = docRef->currentBuf[docRef->currentIndex++];
        if (c >= 0x01 && c <= 0x08) {
          docRef->aux = c;
          docRef->state = 1;
        }
        else if (c >= 0x80 && c <= 0xBF) {
          docRef->aux = c;
          docRef->state = 2;
        }
        else if (c >= 0xC0 && c <= 0xFF) {
          docRef->backBuf[docRef->backIndex % BACKSIZE] = dst[i++] = ' ';
	  docRef->backIndex++;
          docRef->aux = c & 0x7F;
          docRef->state = 3;
        }
        else {
          docRef->backBuf[docRef->backIndex % BACKSIZE] = dst[i++] = c;
	  docRef->backIndex++;
	}
	break;
      case 1:
        c = docRef->currentBuf[docRef->currentIndex++];
        docRef->backBuf[docRef->backIndex % BACKSIZE] = dst[i++] = c;
	docRef->backIndex++;
	docRef->aux--;
	if (docRef->aux == 0)
          docRef->state = 0;
	break;
      case 2:
        c = docRef->currentBuf[docRef->currentIndex++];
	m = (((((UInt16)(docRef->aux)) << 8) | (UInt16)c) & 0x3fff) >> 3;
	docRef->backIndex = docRef->backIndex % BACKSIZE;
	docRef->auxs = m < docRef->backIndex ? docRef->backIndex-m :
		                               docRef->backIndex+BACKSIZE-m;
	docRef->aux = (c & 0x07) + 3;
        docRef->state = 4;
	break;
      case 3:
        docRef->backBuf[docRef->backIndex % BACKSIZE] = dst[i++] = docRef->aux;
	docRef->backIndex++;
        docRef->state = 0;
	break;
      case 4:
        dst[i] = docRef->backBuf[docRef->auxs % BACKSIZE];
        docRef->backBuf[docRef->backIndex % BACKSIZE] = dst[i];
	docRef->auxs++;
	docRef->backIndex++;
	i++;
	docRef->aux--;
	if (docRef->aux == 0)
          docRef->state = 0;
	break;
    }
  }

  return i;
}

#include "p.h"
#include "main.h"
#include "app.h"
#include "link.h"
#include "db.h"
#include "gui.h"
#include "error.h"

#define appPrefixType   'Exec'
#define appPrefixID     5001

#define appIconType     'Icon'
#define appIconID       5001
#define appSmallIconID  5002

typedef struct {
  UInt32 type;
  UInt16 id;
} ResTypeID;

static ResTypeID rsrc[] = {
  {'code', 0},
  {'code', 1},
  {'data', 0},
  {'pref', 0},
  {'MBAR', AppMenu},
  {'tFRM', GraphicForm},
  {'tFRM', AboutForm},
  {'tFRM', InputForm},
  {'tFRM', DialogForm},
  {'tFRM', FullGraphicForm},
  {'Talt', ErrorAlert},
  {'Talt', InfoAlert},
  {'Talt', ConfirmAlert},
  {0, 0}
};

#define MAX_CODE 10

Int16 link(UInt32 creator, char *execName, char *version, char *objName,
           Boolean noTitle, Boolean full)
{
  MemHandle prefixHandle, resHandle;
  MemPtr prefixPtr, resPtr;
  DmOpenRef execRef;
  FILE *f;
  UInt16 attr;
  UInt32 size;
  uchar *p, buf[64];
  Int32 i, n;
  Err err;

  FileDelete(0, execName);

  if (full) {

    if ((err = DmCreateDatabase(0, execName, creator,
           sysFileTApplication, true)) != 0) {
      ErrorDialog("Link error 1", err);
      return -1;
    }

    if ((execRef = DbOpenByName(execName, dmModeWrite, &err)) == NULL) {
      ErrorDialog("Link error 2", err);
      FileDelete(0, execName);
      return -1;
    }

    for (i = 0; rsrc[i].type; i++) {
      if ((err = addResource(execRef, rsrc[i].type, rsrc[i].id,
                    rsrc[i].type, rsrc[i].id)) != 0) {
        ErrorDialog("Link error 3", err);
        DbClose(execRef);
        FileDelete(0, execName);
        return -1;
      }
    }

    for (i = 0; i < MAX_CODE; i++)
      addResource(execRef, 'code', 0x8001 + i, 'code', 0x8001 + i);

    for (i = 0; i < MAX_CODE; i++)
      addResource(execRef, 'rloc', 0x8001 + i, 'rloc', 0x8001 + i);

    DbClose(execRef);

  } else {

    if ((prefixHandle = DmGetResource(appPrefixType, appPrefixID)) == NULL) {
      ErrorDialog("Link error 1", DmGetLastErr());
      return -1;
    }

    if ((prefixPtr = MemHandleLock(prefixHandle)) == NULL) {
      ErrorDialog("Link error 2", 0);
      DmReleaseResource(prefixHandle);
      return -1;
    }

    size = MemPtrSize(prefixPtr);

    if ((p = malloc(size)) == NULL) {
      ErrorDialog("Link error 3", 0);
      MemHandleUnlock(prefixHandle);
      DmReleaseResource(prefixHandle);
      return -1;
    }

    if ((err = MemMove(p, prefixPtr, size)) != 0) {
      ErrorDialog("Link error 4", err);
      MemHandleUnlock(prefixHandle);
      DmReleaseResource(prefixHandle);
      return -1;
    }

    MemHandleUnlock(prefixHandle);
    DmReleaseResource(prefixHandle);

    for (i = 0; execName[i] && i < 32; i++)
      p[i] = execName[i];
    for (; i < 32; i++)
      p[i] = '\0';

    p[64] = (uchar)(creator >> 24);
    p[65] = (uchar)(creator >> 16);
    p[66] = (uchar)(creator >> 8);
    p[67] = (uchar)creator;

    if ((err = DmCreateDatabaseFromImage(p)) != 0) {
      ErrorDialog("Link error 5", err);
      free(p);
      return -1;
    }

    free(p);
  }

  if ((execRef = DbOpenByName(execName, dmModeWrite, &err)) == NULL) {
    ErrorDialog("Link error 6", err);
    FileDelete(0, execName);
    return -1;
  }

  if ((f = fopen(objName, "r")) == NULL) {
    ErrorDialog("Link error 7", 0);
    FileDelete(0, execName);
    return -1;
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if ((resHandle = DmNewResource(execRef, GetCodeType(),
         GetCodeID(), size)) == NULL) {
    ErrorDialog("Link error 8", DmGetLastErr());
    fclose(f);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  }

  if ((resPtr = MemHandleLock(resHandle)) == NULL) {
    ErrorDialog("Link error 9", 0);
    DmReleaseResource(resHandle);
    fclose(f);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  }

  for (i = 0; i < size; i += n) {
    n = fread(buf, 1, sizeof(buf), f);
    if (!n)
      break;
    if ((err = DmWrite(resPtr, i, buf, n)) != 0) {
      ErrorDialog("Link error 10", err);
      MemHandleUnlock(resHandle);
      DmReleaseResource(resHandle);
      fclose(f);
      DbClose(execRef);
      FileDelete(0, execName);
      return -1;
    }
  }

  MemHandleUnlock(resHandle);
  DmReleaseResource(resHandle);
  fclose(f);

  if (i != size) {
    ErrorDialog("Link error 11", 0);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  }

  n = StrLen(version)+1;

  if ((err = addResourcePtr(execRef, version, n, verRsc, appVersionID)) != 0) {
    ErrorDialog("Link error 12", err);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  }

  if ((err = addResource(execRef, appIconType, appIconID, iconType, 1000)) != 0) {
    ErrorDialog("Link error 13", err);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  }

  if ((err = addResource(execRef, appIconType, appSmallIconID, iconType, 1001)) != 0) {
    ErrorDialog("Link error 13", err);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  }

  size = 0;
  if (noTitle && (err = addResourcePtr(execRef, &size, sizeof(size), scrType, scrID)) != 0) {
    ErrorDialog("Link error 14", err);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  }

  if ((err = DbGetAttributes(execRef, &attr)) != 0 ||
      (err = DbSetAttributes(execRef, attr | dmHdrAttrBackup)) != 0) {
    ErrorDialog("Link error 15", err);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  }

  DbClose(execRef);
  return 0;
}

Int16 linklib(UInt32 creator, char *execName, char *version, char *objName)
{
  MemHandle resHandle;
  MemPtr resPtr;
  DmOpenRef execRef;
  FILE *f;
  UInt16 attr;
  UInt32 size;
  uchar buf[64];
  Int32 i, n;
  Err err; 

  FileDelete(0, execName);

  if ((err = DbResCreate(execName, GetLibType(), creator)) != 0) {
    ErrorDialog("Link error 6", err);
    FileDelete(0, execName);
    return -1;
  }

  if ((execRef = DbOpenByName(execName, dmModeWrite, &err)) == NULL) {
    ErrorDialog("Link error 6", err);
    FileDelete(0, execName);
    return -1;
  }

  if ((f = fopen(objName, "r")) == NULL) {
    ErrorDialog("Link error 7", 0);
    FileDelete(0, execName);
    return -1;
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if ((resHandle = DmNewResource(execRef, GetCodeType(),
         GetCodeID(), size)) == NULL) {
    ErrorDialog("Link error 8", DmGetLastErr());
    fclose(f);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  } 

  if ((resPtr = MemHandleLock(resHandle)) == NULL) {
    ErrorDialog("Link error 9", 0);
    DmReleaseResource(resHandle);
    fclose(f);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  } 

  for (i = 0; i < size; i += n) {
    n = fread(buf, 1, sizeof(buf), f);
    if (!n)
      break;
    if ((err = DmWrite(resPtr, i, buf, n)) != 0) {
      ErrorDialog("Link error 10", err);
      MemHandleUnlock(resHandle);
      DmReleaseResource(resHandle);
      fclose(f);
      DbClose(execRef);
      FileDelete(0, execName);
      return -1;
    }
  }

  if (i != size) {
    ErrorDialog("Link error 11", 0);
    DbClose(execRef); 
    FileDelete(0, execName);
    return -1;
  }

  n = StrLen(version)+1;

  if ((err = addResourcePtr(execRef, version, n, verRsc, appVersionID)) != 0) {
    ErrorDialog("Link error 12", err);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  }

  MemHandleUnlock(resHandle);
  DmReleaseResource(resHandle);
  fclose(f);

  if ((err = DbGetAttributes(execRef, &attr)) != 0 ||
      (err = DbSetAttributes(execRef, attr | dmHdrAttrBackup)) != 0) {
    ErrorDialog("Link error 15", err);
    DbClose(execRef);
    FileDelete(0, execName);
    return -1;
  }

  DbClose(execRef);
  return 0;
}

Err addResource(DmOpenRef dbRef, UInt32 srcType, UInt32 srcId,
                UInt32 dstType, UInt32 dstId)
{
  MemHandle srcHandle, dstHandle;
  MemPtr srcPtr, dstPtr;
  LocalID dbID;
  DmOpenRef auxRef;
  UInt16 cardNo, dstIndex;
  UInt32 size;
  Err err;

  // garante que o resource sera pesquisado primeiro no PRC
  SysCurAppDatabase(&cardNo, &dbID);
  auxRef = DbOpen(dbID, dmModeReadOnly, &err);

  if ((dstIndex = DmFindResource(dbRef, dstType, dstId, NULL)) <
       dmMaxRecordIndex) {
    if ((err = DmRemoveResource(dbRef, dstIndex)) != 0) {
      DbClose(auxRef);
      return err;
    }
  }

  if ((srcHandle = DmGetResource(srcType, srcId)) == NULL)
    return DmGetLastErr();

  if ((srcPtr = MemHandleLock(srcHandle)) == NULL) {
    DmReleaseResource(srcHandle);
    DbClose(auxRef);
    return -1;
  }

  size = MemPtrSize(srcPtr);

  if ((dstHandle = DmNewResource(dbRef, dstType, dstId, size)) == NULL) {
    err = DmGetLastErr();
    MemHandleUnlock(srcHandle);
    DmReleaseResource(srcHandle);
    DbClose(auxRef);
    return err;
  }

  if ((dstPtr = MemHandleLock(dstHandle)) == NULL) {
    DmReleaseResource(dstHandle);
    MemHandleUnlock(srcHandle);
    DmReleaseResource(srcHandle);
    DbClose(auxRef);
    return -1;
  }

  DmWrite(dstPtr, 0, srcPtr, size);
  MemHandleUnlock(dstHandle);
  DmReleaseResource(dstHandle);

  MemHandleUnlock(srcHandle);
  DmReleaseResource(srcHandle);

  DbClose(auxRef);
  return 0;
}

Err addResourcePtr(DmOpenRef dbRef, MemPtr srcPtr, UInt32 size,
                   UInt32 dstType, UInt32 dstId)
{
  MemHandle dstHandle;
  MemPtr dstPtr;

  if ((dstHandle = DmNewResource(dbRef, dstType, dstId, size)) == NULL)
    return DmGetLastErr();

  if ((dstPtr = MemHandleLock(dstHandle)) == NULL) {
    DmReleaseResource(dstHandle);
    return -1;
  }

  DmWrite(dstPtr, 0, srcPtr, size);
  MemHandleUnlock(dstHandle);
  DmReleaseResource(dstHandle);

  return 0;
}

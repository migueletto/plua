#include "p.h"
#include "main.h"
#include "db.h"
#include "file.h"

#define memoNameSize 40

static Int16 compare(void *e1, void *e2, Int32 other) SEC("aux");
static Int16 lcompare(void *e1, void *e2, Int32 other) SEC("aux");
static Int16 comparen(void *e1, void *e2, Int32 other) SEC("aux");

static Int16 compare(void *e1, void *e2, Int32 other)
{
  return StrCompare(((FileRecType *)e1)->name, ((FileRecType *)e2)->name);
}

static Int16 lcompare(void *e1, void *e2, Int32 other)
{
  return StrCompare(((FileRecType *)e1)->lname, ((FileRecType *)e2)->lname);
}

static Int16 comparen(void *e1, void *e2, Int32 other)
{
  Int32 i1 = ((FileRecType *)e1)->index;
  Int32 i2 = ((FileRecType *)e2)->index;
  return i1 - i2;
}

Err CreateMemoList(char *dbname, FileType *file, char *prefix, char *suffix)
{
  FILE *f;
  Int16 i;

  if (!file)
    return -1;

  if ((f = dbopen(GetMemoName(), "r")) == NULL)
    return -1;
    
  file->n = CountMemoFiles(f->dbRef, prefix, suffix);
  file->fname = NULL;
  file->rec = NULL;

  if (file->n) {
    if ((file->fname = malloc(file->n * sizeof(char *))) == NULL) {
      file->n = 0;
      fclose(f);
      return -1;
    }
    if ((file->rec = malloc(file->n * sizeof(FileRecType))) == NULL) {
      file->n = 0;
      free(file->fname);
      file->fname = NULL;
      fclose(f);
      return -1;
    }
    if (ListMemoFiles(f->dbRef, prefix, suffix, file->n,file->rec) != file->n) {
      file->n = 0;
      free(file->rec);
      file->rec = NULL;
      free(file->fname);
      file->fname = NULL;
      fclose(f);
      return -1;
    }

    SysQSort(file->rec, file->n, sizeof(FileRecType), compare, 0);

    for (i = 0; i < file->n; i++)
      file->fname[i] = file->rec[i].name;
  }

  fclose(f);
  return 0;
}

Int16 CountMemoFiles(DmOpenRef dbRef, char *prefix, char *suffix)
{
  char *memo;
  UInt16 index;
  Int16 i, iname, plen, slen;
  UInt32 numRecs;
  char buf[memoNameSize];
  Err err;

  numRecs = DbNumRecords(dbRef);
  if (numRecs == 0)
    return 0;

  plen = prefix ? StrLen(prefix) : 0;
  slen = suffix ? StrLen(suffix) : 0;

  for (index = 0, iname = 0; index < numRecs; index++) {
    if ((memo = DbOpenRec(dbRef, index, &err)) != NULL) {
      MemSet(buf, sizeof(buf), 0);
      StrNCopy(buf, memo, sizeof(buf)-1);
      DbCloseRec(dbRef, index, memo);

      for (i = 0; buf[i]; i++)
        if (buf[i] == 10 || buf[i] == 13) {
          buf[i] = 0;
          break;
        }

      if (i > (slen + plen) &&
          (!plen || !StrNCompare(buf, prefix, plen)) &&
          (!slen || !StrNCompare(&buf[StrLen(buf)-slen], suffix, slen)))
        iname++;
    }
  }

  return iname;
}

Int16 ListMemoFiles(DmOpenRef dbRef, char *prefix, char *suffix,
                    Int16 n, FileRecType *rec)
{
  char *memo;
  UInt16 index;
  Int16 i, iname, plen, slen;
  UInt32 numRecs; 
  char buf[memoNameSize];
  Err err;
  
  numRecs = DbNumRecords(dbRef);
  if (numRecs == 0)
    return 0; 
    
  plen = prefix ? StrLen(prefix) : 0;
  slen = suffix ? StrLen(suffix) : 0;
  
  for (index = 0, iname = 0; index < numRecs && iname < n; index++) {
    if ((memo = DbOpenRec(dbRef, index, &err)) != NULL) {
      MemSet(buf, sizeof(buf), 0);
      StrNCopy(buf, memo, sizeof(buf)-1);
      DbCloseRec(dbRef, index, memo);

      for (i = 0; buf[i]; i++)
        if (buf[i] == 10 || buf[i] == 13) {
          buf[i] = 0; 
          break; 
        }

      if (i > (slen + plen) &&
          (!plen || !StrNCompare(buf, prefix, plen)) &&
          (!slen || !StrNCompare(&buf[StrLen(buf)-slen], suffix, slen))) {
        MemSet(rec[iname].name, dmDBNameLength, 0);
        StrNCopy(rec[iname].name, &buf[plen], dmDBNameLength-1);
        rec[iname].index = index;
        iname++; 
      }
    }
  } 
  
  return iname;
}

Err CreateFileList(UInt32 creator, UInt32 type, FileType *file, char *suffix, UInt16 attrf)
{
  Int16 i;

  if (!file)
    return -1;

  file->n = CountFiles(creator, type, suffix, attrf);
  file->fname = NULL;
  file->rec = NULL;

  if (file->n) {
    if ((file->fname = malloc(file->n * sizeof(char *))) == NULL) {
      file->n = 0;
      return -1;
    }
    if ((file->rec = malloc(file->n * sizeof(FileRecType))) == NULL) {
      file->n = 0;
      free(file->fname);
      file->fname = NULL;
      return -1;
    }
    if (ListFiles(creator, type, suffix, file->n, file->rec, attrf) != file->n) {
      file->n = 0;
      free(file->rec);
      file->rec = NULL;
      free(file->fname);
      file->fname = NULL;
      return -1;
    }

    SysQSort(file->rec, file->n, sizeof(FileRecType), compare, 0);

    for (i = 0; i < file->n; i++)
      file->fname[i] = file->rec[i].name;
  }

  return 0;
}

Int16 CountFiles(UInt32 creator, UInt32 type, char *suffix, UInt16 attrf)
{
  Boolean newSearch;
  DmSearchStateType stateInfo;
  UInt16 cardNo, attr;
  LocalID dbID;
  Int16 i, k, m;
  char name[dmDBNameLength];
 
  k = suffix ? StrLen(suffix) : 0;

  switch (attrf) {
    case 1:
      attrf = dmHdrAttrResDB;
      break;
    case 2:
      attrf = dmHdrAttrStream;
      break;
    default:
      attrf = 0;
  }

  for (i = 0, newSearch = true; ; newSearch = false) {
    if (DmGetNextDatabaseByTypeCreator(newSearch, &stateInfo, type, creator,
          false, &cardNo, &dbID) != 0)
      break;
    MemSet(name, dmDBNameLength, 0);
    if (DmDatabaseInfo(cardNo, dbID, name, &attr,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != 0)
      return i;
    if (attrf && !(attr & attrf))
      continue;
    if (k) {
      m = StrLen(name);
      if (m <= k || StrCompare(suffix, &name[m-k]))
        continue;
    }
    i++;
  }
 
  return i;
}

Int16 ListFiles(UInt32 creator, UInt32 type, char *suffix,
                Int16 n, FileRecType *rec, UInt16 attrf) 
{
  Boolean newSearch;
  DmSearchStateType stateInfo;
  UInt16 cardNo, attr;
  LocalID dbID;
  Int16 i, k, m;
  char name[dmDBNameLength];
 
  k = suffix ? StrLen(suffix) : 0;

  switch (attrf) {
    case 1:
      attrf = dmHdrAttrResDB;
      break;
    case 2:
      attrf = dmHdrAttrStream;
      break;
    default:
      attrf = 0;
  }

  for (i = 0, newSearch = true; i < n; newSearch = false) {
    if (DmGetNextDatabaseByTypeCreator(newSearch, &stateInfo, type, creator,
          false, &cardNo, &dbID) != 0)
      break;
    MemSet(name, dmDBNameLength, 0);
    if (DmDatabaseInfo(cardNo, dbID, name, &attr,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != 0)
      return -1;
    if (attrf && !(attr & attrf))
      continue;
    if (k) {
      m = StrLen(name);
      if (m <= k || StrCompare(suffix, &name[m-k]))
        continue;
    }
    rec[i].cardNo = cardNo;
    rec[i].dbID = dbID;
    MemSet(rec[i].name, dmDBNameLength, 0);
    StrNCopy(rec[i].name, name, dmDBNameLength-1);
    i++;
  }
 
  return i;
}

Err CreateStreamFileList(FileType *file, char *suffix)
{
  Int16 i;

  if (!file)
    return -1;
    
  file->n = CountStreamFiles(suffix);
  file->fname = NULL;
  file->rec = NULL;
  
  if (file->n) {
    if ((file->fname = malloc(file->n * sizeof(char *))) == NULL) {
      file->n = 0;
      return -1;
    }
    if ((file->rec = malloc(file->n * sizeof(FileRecType))) == NULL) {
      file->n = 0;
      free(file->fname);
      file->fname = NULL;
      return -1;
    }
    if (ListStreamFiles(suffix, file->n, file->rec) != file->n) {
      file->n = 0;
      free(file->rec);
      file->rec = NULL;
      free(file->fname);
      file->fname = NULL;
      return -1;
    }

    SysQSort(file->rec, file->n, sizeof(FileRecType), compare, 0);

    for (i = 0; i < file->n; i++)
      file->fname[i] = file->rec[i].name;
  }

  return 0;
}

Int16 CountStreamFiles(char *suffix)
{
  Boolean newSearch;
  DmSearchStateType stateInfo;
  UInt16 cardNo, attr;
  LocalID dbID; 
  Int16 i, k, m;
  char name[dmDBNameLength];

  k = suffix ? StrLen(suffix) : 0;

  for (i = 0, newSearch = true; ; newSearch = false) {
    if (DmGetNextDatabaseByTypeCreator(newSearch, &stateInfo,
          sysFileTFileStream, 0, false, &cardNo, &dbID) != 0)
      break;
    if (k) {
      MemSet(name, dmDBNameLength, 0);
      if (DmDatabaseInfo(cardNo, dbID, name, &attr,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != 0)
        return i;
      if (!(attr & dmHdrAttrStream))
        continue;
      m = StrLen(name);
      if (m <= k || StrCompare(suffix, &name[m-k]))
        continue;
    }
    i++;
  }
    
  return i;
}   

Int16 ListStreamFiles(char *suffix, Int16 n, FileRecType *rec)
{ 
  Boolean newSearch;
  DmSearchStateType stateInfo;
  UInt16 cardNo;
  LocalID dbID; 
  Int16 i, k, m, attr;
  char name[dmDBNameLength];
  
  k = suffix ? StrLen(suffix) : 0;

  for (i = 0, newSearch = true; i < n; newSearch = false) {
    if (DmGetNextDatabaseByTypeCreator(newSearch, &stateInfo,
          sysFileTFileStream, 0, false, &cardNo, &dbID) != 0)
      break;
    MemSet(name, dmDBNameLength, 0);
    if (DmDatabaseInfo(cardNo, dbID, name, &attr,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != 0)
      return -1;
    if (!(attr & dmHdrAttrStream))
      continue;
    if (k) {
      m = StrLen(name);
      if (m <= k || StrCompare(suffix, &name[m-k]))
        continue; 
    }
    rec[i].cardNo = cardNo;
    rec[i].dbID = dbID;
    MemSet(rec[i].name, dmDBNameLength, 0);
    StrNCopy(rec[i].name, name, dmDBNameLength-1);
    i++;
  }

  return i;
}

Err CreateVfsFileList(char *path, FileType *file, char *suffix, Boolean dirs)
{
  Int16 i;

  if (!file)
    return -1;

  file->n = CountVfsFiles(path, suffix, dirs);
  file->fname = NULL;
  file->rec = NULL;

  if (file->n) {
    if ((file->fname = malloc(file->n * sizeof(char *))) == NULL) {
      file->n = 0;
      return -1;
    }
    if ((file->rec = malloc(file->n * sizeof(FileRecType))) == NULL) {
      file->n = 0;
      free(file->fname);
      file->fname = NULL;
      return -1;
    }
    if (ListVfsFiles(path, suffix, file->n, file->rec, dirs) != file->n) {
      file->n = 0;
      free(file->rec);
      file->rec = NULL;
      free(file->fname);
      file->fname = NULL;
      return -1;
    }

    SysQSort(file->rec, file->n, sizeof(FileRecType), lcompare, 0);

    for (i = 0; i < file->n; i++)
      file->fname[i] = file->rec[i].lname ? file->rec[i].lname : file->rec[i].name;
  }

  return 0;
}

Int16 CountVfsFiles(char *path, char *suffix, Boolean dirs)
{
  Int16 i, k, m;
  struct dirent *ent;
  DIR *d;

  k = suffix ? StrLen(suffix) : 0;

  if ((d = opendir(path)) == NULL)
    return 0;

  for (i = 0;;) {
    if ((ent = readdir(d)) == NULL)
      break;
    switch (ent->d_type) {
      case DT_REG:
        break;
      case DT_DIR:
        if (!dirs) continue;
        StrCat(ent->d_name, "/");
        break;
      default:
        continue;
    }
    if (k) {
      m = StrLen(ent->d_name);
      if (m <= k || StrCompare(suffix, &ent->d_name[m-k]))
        continue;
    }
    i++;
  }

  closedir(d);

  return i;
}

Int16 ListVfsFiles(char *path, char *suffix, Int16 n, FileRecType *rec,
                   Boolean dirs) 
{
  Int16 i, k, m, len;
  struct dirent *ent;
  DIR *d;

  k = suffix ? StrLen(suffix) : 0;

  if ((d = opendir(path)) == NULL)
    return 0;

  for (i = 0; i < n;) {
    if ((ent = readdir(d)) == NULL)
      break;
    switch (ent->d_type) {
      case DT_REG:
        break;
      case DT_DIR:
        if (!dirs) continue;
        StrCat(ent->d_name, "/");
        break;
      default:
        continue;
    }
    if (k) {
      m = StrLen(ent->d_name);
      if (m <= k || StrCompare(suffix, &ent->d_name[m-k]))
        continue;
    }
    rec[i].cardNo = 0;
    rec[i].dbID = 0;
    MemSet(rec[i].name, dmDBNameLength, 0);
    len = StrLen(ent->d_name);
    if (len < dmDBNameLength) {
      rec[i].lname = NULL;
      StrCopy(rec[i].name, ent->d_name);
    } else {
      rec[i].lname = calloc(len+1, sizeof(char));
      StrCopy(rec[i].lname, ent->d_name);
    }
    i++;
  }

  closedir(d);

  return i;
}

Err CreateResourceList(char *name, UInt32 type, FileType *file)
{
  DmOpenRef dbRef;
  UInt16 attr, index, i;
  DmResID resId;
  Err err;

  file->n = 0;
  file->fname = NULL;
  file->rec = NULL;

  if ((dbRef = DbOpenByName(name, dmModeReadOnly, &err)) == NULL)
    return -1;

  if (DbGetAttributes(dbRef, &attr) != 0 || !(attr & dmHdrAttrResDB)) {
    DbClose(dbRef);
    return -1;
  }

  for (file->n = 0;; file->n++) {
    index = DmFindResourceType(dbRef, type, file->n);
    if (index == 0xFFFF)
      break;
  }

  if (file->n > 0) {
    if ((file->rec = malloc(file->n * sizeof(FileRecType))) == NULL) {
      file->n = 0;
      DbClose(dbRef);
      return -1;
    }

    for (i = 0; i < file->n; i++) {
      index = DmFindResourceType(dbRef, type, i);

      if (DmResourceInfo(dbRef, index, NULL, &resId, NULL) != 0) {
        free(file->rec);
        file->rec = NULL;
        file->n = 0;
        DbClose(dbRef);
        return -1;
      }
      file->rec[i].index = resId;
    }

    SysQSort(file->rec, file->n, sizeof(FileRecType), comparen, 0);
  }

  DbClose(dbRef);
  return 0;
}

Err DestroyFileList(FileType *file)
{ 
  int i;

  if (!file)
    return -1;
  
  if (file->fname) 
    free(file->fname);

  if (file->rec) {
    for (i = 0; i < file->n; i++) {
      if (file->rec[i].lname)
        free(file->rec[i].lname);
    }
    free(file->rec);
  }
    
  file->n = 0;
  file->fname = NULL;
  file->rec = NULL;
    
  return 0;
}

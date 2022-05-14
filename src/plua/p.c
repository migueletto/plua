#include "p.h"
#include "main.h"
#include "app.h"
#include "events.h"
#include "db.h"
#include "file.h"
#include "doc.h"
#include "serial.h"
#include "printstring.h"
#include "error.h"

static FILE defaultStdin  = {FILE_NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, false, 0, 0, 0, 0, 0, 0, NULL, false, 0, 0, NULL};
static FILE defaultStdout  = {FILE_SCREEN, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, true, 0, 0, 0, 0, 0, 0, NULL, false, 0, 0, NULL};
static FILE defaultStderr  = {FILE_DIALOG, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, true, 0, 0, 0, 0, 0, 0, NULL, false, 0, 0, NULL};

static FILE *openFiles[MAX_FILES];
static Int16 ioFile;
static Boolean hasVFS;
static struct dirent dirEntry;
static DRIVER *installedDrivers[MAX_DRIVERS];
static Int16 netRef;
static char hostName[256];
static NetSocketAddrINType netAddr;
static NetHostInfoBufType hostInfo;
static UInt16 dynamicHeapID;
static UInt32 dynamicHeapUsableSize;
static struct tm localTime;
static DateTimeType dateTime;
static char strtodBuffer[80];
static char fprintfBuffer[256];
static char strDate[32];
static char strTime[32];
static UInt32 initialClock;

FILE *stdin;
FILE *stdout;
FILE *stderr;
int errno;
UInt32 CLOCKS_PER_SEC;

static Int16 find_volume(UInt16 vol);
static FILE *open_resource(DmOpenRef dbRef, UInt32 type, UInt32 creator);
static FILE *add_file(FILE *f);
static void remove_file(FILE *f);

void _exit(int status)
{
  AppBreak();
}

double strtod(const char *nptr, char **endptr)
{
  FlpCompDouble dd;
  Int16 k, i, start, len, before, after;

  if (endptr)
    *endptr = (char *)nptr;

  if (!nptr || !nptr[0])
    return 0;
  for (k = 0; nptr[k] == ' ' || nptr[k] == '\t' || nptr[k] == '\n'; k++);
  start = k;
  if (nptr[k] == '+' || nptr[k] == '-')
    k++;
  before = after = 0;
  if (nptr[k] != '.') {
    for (i = 0; nptr[k] >= '0' && nptr[k] <= '9'; k++, i++);
    if (i == 0)
      return 0;
    before = i;
  }
  if (nptr[k] == '.') {
    for (k++, i = 0; nptr[k] >= '0' && nptr[k] <= '9'; k++, i++);
    after = i;
  }
  if (before == 0 && after == 0)
    return 0;
  if (nptr[k] == 'e' || nptr[k] == 'E') {
    k++;
    if (nptr[k] == '+' || nptr[k] == '-')
      k++;
    for (i = 0; nptr[k] >= '0' && nptr[k] <= '9'; k++, i++);
    if (i == 0)
      return 0;
  }

  len = k - start;
  if (len >= sizeof(strtodBuffer))
    len = sizeof(strtodBuffer)-1;

  memset(strtodBuffer, 0, sizeof(strtodBuffer));
  strncpy(strtodBuffer, &nptr[start], len);
  FlpBufferAToF(&dd.fd, strtodBuffer);

  if (endptr)
    *endptr = (char *)&nptr[k];

  return dd.d;
}

void mem_init(void)
{
  MemHandle h = MemHandleNew(16);

  dynamicHeapID = MemHandleHeapID(h);
  MemHandleFree(h);
  dynamicHeapUsableSize = MemHeapSize(dynamicHeapID);
  // reserva 4K para OS
  if (dynamicHeapUsableSize > 4096) dynamicHeapUsableSize -= 4096;
}

size_t mem_size(void)
{
  return dynamicHeapUsableSize;
}

size_t mem_available(void)
{
  UInt32 f, m;
  MemHeapFreeBytes(dynamicHeapID, &f, &m);
  // reserva 4K para OS
  if (f > 4096) f -= 4096;
  return f;
}

size_t mem_threshold(size_t used, size_t threshold)
{
  size_t m = used + mem_available();
  if (threshold > m)
    return m;
  return threshold;
}

void *malloc(size_t size)
{
  void *ptr = MemPtrNew(size);

  if (ptr) {
    MemSet(ptr, size, 0);
    errno = 0;
  } else
    errno = ENOMEM;
  return ptr;
}

void *calloc(size_t number, size_t size)
{
  return malloc(number * size);
}

void *realloc(void *ptr, size_t size)
{
  void *new_ptr;

  if ((new_ptr = malloc(size)) == NULL)
    return NULL;

  if (ptr) {
    size_t old_size = MemPtrSize(ptr);
    MemMove(new_ptr, ptr, size >= old_size ? old_size : size);
    free(ptr);
  }

  return new_ptr;
}

void free(void *ptr)
{
  if (ptr)
    MemPtrFree(ptr);
  errno = 0;
}

size_t strcspn(const char *s, const char *r)
{
  int i;
  // so funciona quando r tem apenas 1 caracter
  char c = r[0];
  if (!s) return 0;
  for (i = 0; s[i] && s[i] != c; i++);
  return i;
}   
    
void *memchr(const void *b, int c, size_t len)
{   
  int i; 
  unsigned char *ub = (unsigned char *)b;
  unsigned char uc = (unsigned char)c;
  for (i = 0; i < len; i++)
    if (ub[i] == uc)
      return &ub[i];
  return NULL;
}   
    
char *strpbrk(const char *s, const char *charset)
{   
  int i, j;
  for (i = 0; s[i]; i++)
    for (j = 0; charset[j]; j++)
      if (s[i] == charset[j])
        return (char *)&s[i];
  return NULL;
}   

char *strdup(const char *str)
{
  char *dst;
  Int16 len;

  if (!str)
    return NULL;

  len = strlen(str);
  if ((dst = malloc(len+1)) == NULL)
    return NULL;

  strncpy(dst, str, len);
  dst[len] = 0;

  return dst;
}

char *strerror(int errnum)
{
  char *s;
  switch (errnum) {
    case 0:		        s = "Success"; break;
    case ENOENT:		s = "No such file or directory"; break;
    case EINTR:			s = "Interrupted system call"; break;
    case EIO:			s = "Input/output error"; break;
    case EBADF:			s = "Bad file descriptor"; break;
    case ENOMEM:		s = "Cannot allocate memory"; break;
    case EACCES:		s = "Permission denied"; break;
    case EBUSY:			s = "Device busy"; break;
    case EEXIST:		s = "File exists"; break;
    case ENODEV:		s = "Operation not supported by device"; break;
    case EINVAL:		s = "Invalid argument"; break;
    case EMFILE:		s = "Too many open files"; break;
    case EFBIG:			s = "File too large"; break;
    case ENOTDIR:		s = "Not a directory"; break;
    case EISDIR:		s = "Is a directory"; break;
    case ENOSPC:		s = "No space left on device"; break;
    case ENETDOWN:		s = "Network is down"; break;
    case ENOTCONN:		s = "Socket is not connected"; break;
    case EMSGSIZE:		s = "Message too long"; break;
    case EINPROGRESS:		s = "Operation now in progress"; break;
    case ENXIO:			s = "Device not configured"; break;
    case EOPNOTSUPP:		s = "Operation not supported"; break;
    case EADDRINUSE:		s = "Address already in use"; break;
    case ETIMEDOUT:		s = "Operation timed out"; break;
    case EISCONN:		s = "Socket is already connected"; break;
    case ECONNRESET:		s = "Connection reset by peer"; break;
    case ESOCKTNOSUPPORT:	s = "Socket type not supported"; break;
    case EPROTONOSUPPORT:	s = "Protocol not supported"; break;
    case ENETUNREACH:		s = "Network is unreachable"; break;
    case EWOULDBLOCK:		s = "Operation would block"; break;
    case EALREADY:		s = "Operation already in progress"; break;
    case EADDRNOTAVAIL:		s = "Can't assign requested address"; break;
    case ECONNREFUSED:		s = "Connection refused"; break;
    case EHOSTUNREACH:		s = "No route to host"; break;
    default:			s = "Unknown error";

  }
  return s;
}

static Int16 find_volume(UInt16 vol)
{
  Int16 i, aux;
  UInt32 iterator;
  Boolean found;
  Err err;

  found = false;

  for (i = 0, iterator = vfsIteratorStart; iterator != vfsIteratorStop; i++) {
    if ((err = VFSVolumeEnumerate(&aux, &iterator)) != 0) {
      errno = mapPalmOsErr(err);
      return -1;
    }
    if (i == vol) {
      found = true;
      break;
    }
  }

  if (!found) {
    errno = EINVAL;
    return -1;
  }

  return aux;
}

DIR *opendir(const char *name)
{
  FILE *f;

  if ((f = fopen(name, "r")) == NULL)
    return NULL;

  if (f->type != FILE_VFSDIR) {
    fclose(f);
    errno = ENOTDIR;
    return NULL;
  }

  return (DIR *)f;
}

int closedir(DIR *f)
{
  return fclose((FILE *)f);
}

int mkdir(const char *name, mode_t mode)
{
  Int16 vol;
  Err err;

  if (!hasVFS || StrLen(name) < 6 ||
      name[3] < '0' || name[3] > '9' || name[4] != ':') {
    errno = EINVAL;
    return -1;
  }

  vol = name[3] - '0';

  if ((vol = find_volume(vol)) == -1)
    return -1;

  if ((err = VFSDirCreate(vol, &name[5])) != 0) {
    errno = mapPalmOsErr(err);
    return -1;
  }

  return 0;
}

int rmdir(const char *name)
{
  Int16 vol;
  Err err;

  if (!hasVFS || StrLen(name) < 6 || StrNCompare(name, "vfs", 3) ||
      name[3] < '0' || name[3] > '9' || name[4] != ':') {
    errno = EINVAL;
    return -1;
  }

  vol = name[3] - '0';

  if ((vol = find_volume(vol)) == -1)
    return -1;

  if ((err = VFSFileDelete(vol, &name[5])) != 0) {
    errno = mapPalmOsErr(err);
    return -1;
  }

  return 0;
}

struct dirent *readdir(DIR *f)
{
  FileInfoType info;
  Err err;

  if (!f || f->type != FILE_VFSDIR) {
    errno = ENOTDIR;
    return NULL;
  }

  if (f->iterator == vfsIteratorStop) {
    errno = 0;
    return NULL;
  }

  info.nameP = dirEntry.d_name;
  info.nameBufLen = sizeof(dirEntry.d_name);

  if ((err = VFSDirEntryEnumerate(f->fref, &f->iterator, &info)) != 0) {
    if (err == expErrEnumerationEmpty)
      errno = 0;
    else
      errno = mapPalmOsErr(err);
    return NULL;
  }

  /*if (info.attributes & vfsFileAttrLink)
    dirEntry.d_type = DT_LNK; // XXX
  else*/ if (info.attributes & vfsFileAttrDirectory)
    dirEntry.d_type = DT_DIR;
  else
    dirEntry.d_type = DT_REG;

  return &dirEntry;
}

FILE *fopen(const char *name, const char *mode0)
{
  Err err;
  FileHand fh;
  FileRef fref;
  DmOpenRef dbRef;
  DocType *doc;
  FileType memos;
  FILE *f;
  DRIVER *driver;
  Boolean writable = true;
  UInt32 type, creator, attr, baud, len, openMode = 0;
  UInt16 index, mindex, id, port;
  Int16 i, j, vol;
  char *p, *s, mode[8], stype[5];

  if (!name || !name[0] || !mode0 || !mode0[0] || StrLen(mode0) > 3) {
    errno = EINVAL;
    return NULL;
  }

  for (i = 0, j = 0; mode0[i]; i++) {
    switch (mode0[i]) {
      case 'r':
      case 'w':
      case 'a':
      case '+':
        mode[j++] = mode0[i];
        break;
      case 'b':
        continue;
      default:
        errno = EINVAL;
        return NULL;
    }
  }
  mode[j] = 0;

  if (!StrNCompare(name, "memo:", 5)) {
    errno = EINVAL;

    if (name[5] != '/')
      return NULL;

    if (StrCompare(mode, "r") &&
        StrCompare(mode, "r+") &&
        StrCompare(mode, "w"))
      return NULL;

    if (CreateMemoList(GetMemoName(), &memos, NULL, NULL) == -1)
      return NULL;

    for (i = 0; i < memos.n; i++) {
      if (!StrNCompare(&name[6], memos.rec[i].name, StrLen(&name[6])))
        break;
      if (!StrNCompare("-- ", memos.rec[i].name, 3) &&
          !StrCompare(&name[6], &memos.rec[i].name[3]))
        break;
    }

    if (i == memos.n) {
      DestroyFileList(&memos);
      errno = ENOENT;
      return NULL;
    }

    mindex = memos.rec[i].index;
    DestroyFileList(&memos);

    if ((f = dbopen(GetMemoName(), mode)) == NULL)
      return NULL;

    if (dbopenrec(f, mindex) == -1) {
      err = errno;
      dbclose(f);
      errno = err;
      return NULL;
    }

    f->type = FILE_MEMO;
    f->recordOffset = 0;
    f->recordPos = f->recordOffset;
    for (p = (char *)f->record;
         f->recordSize >= f->recordOffset && p[f->recordSize-1] == 0;
         f->recordSize--);

    return f;
  }

  if (!StrNCompare(name, "code:", 5)) {
    LocalID callerID;

    errno = EINVAL;

    if (name[5] != '/')
      return NULL;

    if (StrCompare(mode, "r"))
      return NULL;

    callerID = StrAToI(&name[6]);
    if ((dbRef = DmOpenDatabase(0, callerID, dmModeReadOnly)) == NULL) {
      errno = ENOENT;
      return NULL;
    }
    return open_resource(dbRef, GetCodeType(), GetCodeID());
  }

  if (!StrNCompare(name, "rsrc:", 5)) {
    errno = EINVAL;

    // 012345678901234
    // rsrc:/Tbmp/1000

    if (name[5] != '/' || name[10] != '/' || StrLen(name) < 12)
      return NULL;

    if (StrCompare(mode, "r"))
      return NULL;

    StrNCopy(stype, &name[6], 4);
    stype[4] = 0;
    type = StringToCreator(stype);
    id = StrAToI(&name[11]);

    return open_resource(NULL, type, id);
  }

  if (!StrNCompare(name, "vfs", 3) && name[4] == ':') {
    errno = EINVAL;

    if (name[3] < '0' && name[3] > '9')
      return NULL;

    if (name[5] != '/')
      return NULL;

    vol = name[3] - '0';

    if ((vol = find_volume(vol)) == -1)
      return NULL;

    if (StrCompare(mode, "r") &&
        StrCompare(mode, "r+") &&
        StrCompare(mode, "w") &&
        StrCompare(mode, "a"))
      return NULL;

    switch (mode[0]) {
      case 'r':
        openMode = mode[1] == '+' ? (vfsModeReadWrite | vfsModeCreate) : vfsModeRead;
        writable = openMode & vfsModeReadWrite;
        break;
      case 'w':
        // vfsModeReadWrite por causa do VFSFileGetAttributes
        openMode = vfsModeReadWrite | vfsModeCreate | vfsModeTruncate;
        break;
      case 'a':
        openMode = vfsModeReadWrite | vfsModeCreate;
        break;
      default:
        return NULL;
    }

    if (ReadOnlyCheck(writable, "open for writing file", name)) {
      errno = EACCES;
      return NULL;
    }

    if ((err = VFSFileOpen(vol, &name[5], openMode, &fref)) != 0) {
      errno = mapPalmOsErr(err);
      return NULL;
    }

    if ((err = VFSFileGetAttributes(fref, &attr)) != 0) {
      errno = mapPalmOsErr(err);
      VFSFileClose(fref);
      return NULL;
    }

    if (mode[0] == 'a' && (err = VFSFileSeek(fref, vfsOriginEnd, 0)) != 0) {
      errno = mapPalmOsErr(err);
      VFSFileClose(fref);
      return NULL;
    }

    if ((f = malloc(sizeof(FILE))) == NULL) {
      VFSFileClose(fref);
      return NULL;
    } 
  
    if (attr & vfsFileAttrDirectory) {
      f->type = FILE_VFSDIR;
      f->iterator = vfsIteratorStart;
    } else {
      f->type = FILE_VFS;
    }

    f->vol = vol;
    f->fref = fref;
    f->realname = strdup(&name[5]);
    f->writable = writable;

    return add_file(f);
  }

  if (!StrNCompare(name, "tcp:", 4) || !StrNCompare(name, "udp:", 4)) {
    Int16 aux, dnsTimeout = 8, connectTimeout = 8, linger = -1;

    errno = EINVAL;

    // 01234
    // tcp:/host:port

    if (name[4] != '/' || name[5] == ':')
      return NULL;

    if ((s = StrChr(&name[5], ':')) == NULL || s[1] == 0)
      return NULL;

    len = s - name - 5;
    if (len >= sizeof(hostName)) len = sizeof(hostName)-1;
    StrNCopy(hostName, &name[5], len);
    hostName[len] = 0;
    port = StrAToI(s+1);

    if ((s = StrChr(&s[1], '/')) != NULL) {
      if (s[1] == 0) return NULL;
      aux = StrAToI(s+1);
      if (aux >= 0) dnsTimeout = aux;

      if ((s = StrChr(&s[1], '/')) != NULL) {
        if (s[1] == 0) return NULL;
        aux = StrAToI(s+1);
        if (aux >= 0) connectTimeout = aux;

        if ((s = StrChr(&s[1], '/')) != NULL) {
          if (s[1] == 0) return NULL;
          aux = StrAToI(s+1);
          if (aux >= 0) linger = aux;
        }
      }
    }

    return netopen(hostName, port, name[0] == 't' ? SOCK_STREAM : SOCK_DGRAM, dnsTimeout, connectTimeout, linger);
  }

  if (!StrNCompare(name, "srm:", 4)) {
    errno = EINVAL;

    // 0         1         2
    // 012345678901234567890
    // srm:/serial/9600/8N1

    if (name[4] != '/' || name[5] == '/')
      return NULL;

    if ((s = StrChr(&name[5], '/')) == NULL || s[1] == 0)
      return NULL;

    len = s - name - 5;
    if (len >= sizeof(hostName)) len = sizeof(hostName)-1;
    StrNCopy(hostName, &name[5], len);
    hostName[len] = 0;
    baud = StrAToI(s+1);

    if ((s = StrChr(&s[1], '/')) == NULL || s[1] == 0)
      return NULL;

    return seropen(hostName, baud, s+1);
  }

  for (i = 0; i < MAX_DRIVERS; i++) {
    if ((driver = installedDrivers[i]) != NULL) {
      len = StrLen(driver->prefix);
      if (!StrNCompare(driver->prefix, name, len)) {
        int id, handle;
        index = len;
        if (name[index] >= '0' && name[index] <= '9')
          id = name[index++] - '0';
        else
          id = 0;
        if (name[index++] == ':') {
          if (!driver->open) {
            errno = ENODEV;
            return NULL;
          }
          if ((err = driver->open(id, (char *)&name[index], &handle)) != 0) {
            errno = mapPalmOsErr(err);
            return NULL;
          }
          if ((f = malloc(sizeof(FILE))) == NULL) {
            if (driver->close)
              driver->close(handle);
            return NULL;
          }
          f->type = FILE_DRIVER;
          f->writable = true;
          f->driver = i;
          f->handle = handle;
          return add_file(f);
        }
      }
    }
  }

  err = DbGetTypeCreator((char *)name, &type, &creator);

  if (err == 0 && (type == sysFileTApplication || type == GetLibType())) {
    if (StrCompare(mode, "r")) {
      errno = EINVAL;
      return NULL;
    }
    if ((dbRef = DbOpenByName((char *)name, dmModeReadOnly, &err)) == NULL) {
      errno = mapPalmOsErr(err);
      return NULL;
    }
    return open_resource(dbRef, GetCodeType(), GetCodeID());
  }

  if (err == 0 && type == DocDBType && creator == DocAppID) {
    if (StrCompare(mode, "r")) {
      errno = EINVAL;
      return NULL;
    }
    if ((doc = DocOpen((char *)name, dmModeReadOnly, &err)) == NULL) {
      errno = mapPalmOsErr(err);
      return NULL;
    }
    if ((f = malloc(sizeof(FILE))) == NULL) {
      DocClose(doc);
      return NULL;
    }
    f->type = FILE_DOC;
    f->doc = doc;
    f->writable = false;
    f->ungetDone = false;
    f->recordOffset = 0;
    f->recordPos = 0;
    return add_file(f);
  }

  if (!StrNCompare(name, "db:", 3)) {
    errno = EINVAL;

    // 01234
    // db:/dbName/recIndex
    // se recIndex nao for especificado, chama apenas dbopen
    // se recIndex for especificado, chama dbopen e dbopenrec

    if (name[3] != '/')
      return NULL;

    if (StrCompare(mode, "r") &&
        StrCompare(mode, "r+") &&
        StrCompare(mode, "w"))
      return NULL;

    if ((s = StrChr(&name[4], '/')) != NULL) {
      len = s - name - 4;
      if (len >= sizeof(hostName)) len = sizeof(hostName)-1;
      StrNCopy(hostName, &name[4], len);
      hostName[len] = 0;
      index = StrAToI(s+1);

      if ((f = dbopen(hostName, mode)) == NULL)
        return NULL;

      if (dbopenrec(f, index) == -1) {
        err = errno;
        dbclose(f);
        errno = err;
        return NULL;
      }
    } else {
      if ((f = dbopen(&name[4], mode)) == NULL)
        return NULL;
    }

    return f;
  }

  if (StrCompare(mode, "r") &&
      StrCompare(mode, "r+") &&
      StrCompare(mode, "w") &&
      StrCompare(mode, "a")) {
    errno = EINVAL;
    return NULL;
  }

  switch (mode[0]) {
    case 'r':
      if (DmFindDatabase(0, name) == 0) {
        errno = ENOENT;
        return NULL;
      }
      openMode = mode[1] == '+' ? fileModeUpdate : fileModeReadOnly;
      writable = openMode == fileModeUpdate;
      break;
    case 'w':
      openMode = fileModeReadWrite;
      break;
    case 'a':
      openMode = fileModeAppend;
      break;
    default:
      errno = EINVAL;
      return NULL;
  }

  if (ReadOnlyCheck(writable, "open for writing file", name)) {
    errno = EACCES;
    return NULL;
  }

  openMode |= fileModeAnyTypeCreator;

  fh = FileOpen(0, (char *)name, 0, 0, openMode, &err);
  if (err) {
    errno = mapPalmOsErr(err);
    return NULL;
  }

  if (writable && GetRomVersionNumber() < 50) {
    len = sizeof(dbRef);
    if ((err = FileControl(fileOpGetOpenDbRef, fh, &dbRef, &len)) != 0) {
      FileClose(fh);
      errno = mapPalmOsErr(err);
      return NULL;
    }
  
    if ((err = DbSetAttributes(dbRef,dmHdrAttrBackup|dmHdrAttrStream)) != 0) {
      FileClose(fh);
      errno = mapPalmOsErr(err);
      return NULL;
    }
  }

  if ((f = malloc(sizeof(FILE))) == NULL) {
    FileClose(fh);
    return NULL;
  }

  f->type = FILE_STREAM;
  f->f = fh;
  f->writable = writable;
  return add_file(f);
}

static FILE *open_resource(DmOpenRef dbRef, UInt32 type, UInt32 creator)
{
  MemHandle resHandle;
  MemPtr resPtr;
  FILE *f;

  if (dbRef)
    resHandle = DmGet1Resource(type, creator);
  else
    resHandle = DmGetResource(type, creator);

  if (resHandle == NULL) {
    if (dbRef) DmCloseDatabase(dbRef);
    errno = ENOENT;
    return NULL;
  }

  if ((resPtr = MemHandleLock(resHandle)) == NULL) {
    DmReleaseResource(resHandle);
    if (dbRef) DmCloseDatabase(dbRef);
    errno = ENOMEM;
    return NULL;
  }

  if ((f = malloc(sizeof(FILE))) == NULL) {
    MemPtrUnlock(resPtr);
    DmReleaseResource(resHandle);
    if (dbRef) DmCloseDatabase(dbRef);
    return NULL;
  }

  f->type = FILE_RESOURCE;
  f->dbRef = dbRef;
  f->writable = false;
  f->nRecords = 1;
  f->index = 0;
  f->recordOffset = 0;
  f->recordPos = 0;
  f->recordSize = MemPtrSize(resPtr);
  f->record = resPtr;
  f->res = resHandle;
  f->resType = type;
  return add_file(f);
}

FILE *freopen(const char *name, const char *mode, FILE *f)
{
  errno = 0;
  return f;
}

int fclose(FILE *f)
{
  int r = 0;
  int aux;
  DRIVER *driver;

  if (!f) {
    errno = EBADF;
    return -1;
  }

  switch (f->type) {
    case FILE_STREAM:
      aux = f->f ? mapPalmOsErr(FileClose(f->f)) : 0;
      r = aux ? -1 : 0;
      if (f->tmpname) {
        FileDelete(0, f->tmpname);
        free(f->tmpname);
      }
      remove_file(f);
      free(f);
      errno = aux;
      break;
    case FILE_PDB:
    case FILE_MEMO:
      r = dbclose(f);
      break;
    case FILE_DOC:
      aux = mapPalmOsErr(DocClose(f->doc));
      r = aux ? -1 : 0;
      remove_file(f);
      free(f);
      errno = aux;
      break;
    case FILE_RESOURCE:
      MemPtrUnlock(f->record);
      DmReleaseResource(f->res);
      if (f->dbRef) DmCloseDatabase(f->dbRef);
      remove_file(f);
      free(f);
      errno = 0;
      break;
    case FILE_SERIAL:
      r = serclose(f);
      break;
    case FILE_DRIVER:
      if ((driver = installedDrivers[f->driver]) == NULL)
        aux = EINVAL;
      else
        aux = driver->close ? mapPalmOsErr(driver->close(f->handle)) : ENODEV;
      r = aux ? -1 : 0;
      remove_file(f);
      free(f);
      errno = aux;
      break;
    case FILE_VFS:
    case FILE_VFSDIR:
      aux = f->fref ? mapPalmOsErr(VFSFileClose(f->fref)) : 0;
      r = aux ? -1 : 0;
      if (f->realname)
        free(f->realname);
      remove_file(f);
      free(f);
      errno = aux;
      break;
    case FILE_TCP:
    case FILE_UDP:
      r = netclose(f);
  }

  return r;
}

FILE *seropen(const char *name, ulong baud, const char *mode)
{
  UInt32 port;
  UInt16 bits, parity, stopBits;
  Int16 handle;
  FILE *f;
  Err err;

  if (!StrCompare(name, "cradle"))
    port = serPortCradlePort;
  else if (!StrCompare(name, "serial"))
    port = serPortCradleRS232Port;
  else if (!StrCompare(name, "usb"))
    port = serPortCradleUSBPort;
  else if (!StrCompare(name, "ir"))
    port = serPortIrPort;
  else
    port = StringToCreator((char *)name);

  if (StrLen(mode) != 3) {
    errno = EINVAL;
    return NULL;
  }

  switch (mode[0]) {
    case '5':
    case '6':
    case '7':
    case '8':
      bits = mode[0] - '0'; break;
    default:
      errno = EINVAL;
      return NULL;
  }

  switch (mode[1]) {
    case 'N': parity = 0; break;
    case 'E': parity = 1; break;
    case 'O': parity = 2; break;
    default:
      errno = EINVAL;
      return NULL;
  }

  switch (mode[2]) {
    case '1':
    case '2':
      stopBits = mode[2] - '0'; break;
    default:
      errno = EINVAL;
      return NULL;
  }

  if ((handle = SerialOnline(port, baud, bits, parity, stopBits,
                             0, 0, &err)) == -1) {
    errno = mapPalmOsErr(err);
    return NULL;
  }

  if ((f = malloc(sizeof(FILE))) == NULL) {
    SerialOffline(handle);
    return NULL;
  }

  f->type = FILE_SERIAL;
  f->handle = handle;
  f->writable = true;

  ioFile++;

  return add_file(f);
}

int serclose(FILE *f)
{
  Err err;

  if (!f || f->type != FILE_SERIAL) {
    errno = EINVAL;
    return -1;
  }

  err = SerialOffline(f->handle);
  remove_file(f);
  free(f);

  ioFile--;

  errno = mapPalmOsErr(err);
  return 0;
}

Int16 netinit(void) {
  Err err, ifErr;
  Int16 ref;

  if (SysLibFind("Net.lib", &ref) != 0)
    return -1;

  if (((err = NetLibOpen(ref, &ifErr)) != 0 && err != netErrAlreadyOpen) || ifErr)
    return -1;

  return ref;
}

FILE *netopen(const char *host, ushort port, ushort protocol, int dnsTimeout, int connectTimeout, int linger) {
  FILE *f;
  NetHostInfoPtr hinfop;
  NetSocketTypeEnum type;
  NetSocketLingerType lingerType;
  Err err, ifErr, aux;
  UInt16 len;
  UInt8 aiu;

  if (netRef == -1) {
    if ((netRef = netinit()) == -1) {
      errno = EINVAL;
      return NULL;
    }
  }

  if ((f = malloc(sizeof(FILE))) == NULL)
    return NULL;
    
  f->writable = true;

  switch (protocol) {
    case SOCK_STREAM: type = netSocketTypeStream; f->type = FILE_TCP; break;
    case SOCK_DGRAM: type = netSocketTypeDatagram; f->type = FILE_UDP; break;
    default:
      free(f);
      errno = EINVAL;
      return NULL;
  }

  if (((err = NetLibConnectionRefresh(netRef, true, &aiu, &ifErr)) != 0 && err != netErrAlreadyOpen) || ifErr) {
    errno = mapPalmOsErr(err);
    return NULL;
  }

  len = sizeof(netAddr);
  netAddr.family = netSocketAddrINET;
  netAddr.port = NetHToNS(port);
  netAddr.addr = NetLibAddrAToIN(netRef, host);

  if (netAddr.addr == -1) {
    if ((hinfop = NetLibGetHostByName(netRef, host, &hostInfo, dnsTimeout*SysTicksPerSecond(), &err)) == NULL) {
      free(f);
      errno = mapPalmOsErr(err);
      return NULL;
    }
    netAddr.addr = hostInfo.address[0];
  }
  
  if ((f->sock = NetLibSocketOpen(netRef, netSocketAddrINET, type, 0, SysTicksPerSecond(), &err)) == -1) {
    free(f);
    errno = mapPalmOsErr(err);
    return NULL;
  }

  if (NetLibSocketConnect(netRef, f->sock, (NetSocketAddrType *)&netAddr, len,
        connectTimeout*SysTicksPerSecond(), &err) == -1) {
    NetLibSocketClose(netRef, f->sock, -1, &aux);
    free(f);
    errno = mapPalmOsErr(err);
    return NULL;
  }

  lingerType.onOff = linger < 0 ? 0 : 1;
  lingerType.time = linger < 0 ? 0 : linger;

  if (NetLibSocketOptionSet(netRef, f->sock, netSocketOptLevelSocket, netSocketOptSockLinger,
        &lingerType, sizeof(lingerType), SysTicksPerSecond(), &err) == -1) {
    NetLibSocketClose(netRef, f->sock, -1, &aux);
    free(f);
    errno = mapPalmOsErr(err);
    return NULL;
  }

  ioFile++;
  return add_file(f);
}

int netclose(FILE *f) {
  Err err;

  if (netRef == -1 || !f || (f->type != FILE_TCP && f->type != FILE_UDP)) {
    errno = EINVAL;
    return -1;
  }

  NetLibSocketShutdown(netRef, f->sock, netSocketDirBoth, SysTicksPerSecond(), &err);
  NetLibSocketClose(netRef, f->sock, -1, &err);
  remove_file(f);
  free(f);

  ioFile--;
  errno = mapPalmOsErr(err);
  return errno ? -1 : 0;
}

int iostatus(FILE *f)
{
  Int16 r;
  UInt16 width;
  NetFDSetType readfds;
  DRIVER *driver;
  Err err;

  switch (f->type) {
    case FILE_SERIAL:
      if (SerialCheck(f->handle))
        return 1;
      break;
    case FILE_TCP:
    case FILE_UDP:
      netFDZero(&readfds);
      netFDSet(f->sock, &readfds);
      width = f->sock;
      r = NetLibSelect(netRef, width+1, &readfds, NULL, NULL, 0, &err);
      if (r > 0 && err == 0 && netFDIsSet(f->sock, &readfds))
        return 1;
      break;
    case FILE_DRIVER:
      if ((driver = installedDrivers[f->driver]) != NULL &&
          driver->status && driver->status(f->handle))
        return 1;
  }

  return 0;
}

int iocheck(void)
{
  Int16 i, n, r;
  UInt16 width;
  NetFDSetType readfds;
  DRIVER *driver;
  Err err;

  for (n = 0, i = 0; n < ioFile && i < MAX_FILES; i++) {
    if (!openFiles[i])
      continue;

    switch (openFiles[i]->type) {
      case FILE_SERIAL:
        n++;
        if (SerialCheck(openFiles[i]->handle))
          return 1;
        break;
      case FILE_TCP:
      case FILE_UDP:
        n++;
        netFDZero(&readfds);
        netFDSet(openFiles[i]->sock, &readfds);
        width = openFiles[i]->sock;
        r = NetLibSelect(netRef, width+1, &readfds, NULL, NULL, 0, &err);
        if (r > 0 && err == 0 && netFDIsSet(openFiles[i]->sock, &readfds))
          return 1;
        break;
      case FILE_DRIVER:
        n++;
        if ((driver = installedDrivers[openFiles[i]->driver]) != NULL &&
            driver->status && driver->status(openFiles[i]->handle))
          return 1;
    }
  }

  return 0;
}

int remove(const char *path)
{
  LocalID id;

  if (ReadOnlyCheck(true, "remove file", path)) {
    errno = EACCES;
    return -1;
  }

  if (!StrNCompare(path, "vfs", 3))
    return rmdir(path);

  if ((id = DmFindDatabase(0, path)) == 0) {
    errno = ENOENT;
    return -1;
  }

  errno = mapPalmOsErr(DmDeleteDatabase(0, id));
  return errno ? -1 : 0;
}

int rename(const char *from, const char *to)
{
  LocalID id;
  Int16 vol;
  Err err;

  if (ReadOnlyCheck(true, "rename file", from)) {
    errno = EACCES;
    return -1;
  }

  if (!StrNCompare(from, "vfs", 3) && !StrNCompare(to, "vfs", 3)) {
    if (!hasVFS || StrLen(from) < 6 || StrLen(to) < 6 ||
        from[3] < '0' || from[3] > '9' || from[3] != to[3] ||
        from[4] != ':' || to[4] != ':') {
      errno = EINVAL;
      return -1;
    }

    vol = from[3] - '0';

    if ((vol = find_volume(vol)) == -1)
      return -1;

    if ((err = VFSFileRename(vol, &from[5], &to[5])) != 0) {
      errno = mapPalmOsErr(err);
      return -1;
    }

    return 0;
  }

  if (!StrNCompare(from, "vfs", 3) || !StrNCompare(to, "vfs", 3)) {
    errno = EINVAL;
    return -1;
  }

  if ((id = DmFindDatabase(0, from)) == 0) {
    errno = ENOENT;
    return -1;
  }

  if ((err = DmSetDatabaseInfo(0, id, to, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL)) != 0) {
    errno = mapPalmOsErr(err);
    return -1;
  }

  return 0;
}

char *getenv(const char *name)
{
  return NULL;
}

int feof(FILE *f)
{
  int r = 0;

  errno = 0;

  if (!f)
    return 1;

  switch (f->type) {
    case FILE_STREAM:
      r = FileEOF(f->f);
      break;
    case FILE_PDB:
    case FILE_MEMO:
    case FILE_RESOURCE:
      r = f->recordPos >= f->recordSize;
      break;
    case FILE_DOC:
      r = DocEOF(f->doc);
      break;
    case FILE_VFS:
      r = VFSFileEOF(f->fref) == vfsErrFileEOF;
      break;
    case FILE_VFSDIR:
      r = f->iterator == vfsIteratorStop;
  }

  return r;
}

int ferror(FILE *f)
{
  return 0;
}

int fseek(FILE *f, long offset, int whence)
{
  int r = 0;
  DRIVER *driver;
  Err err;

  if (!f) {
    errno = EBADF;
    return -1;
  }

  switch (f->type) {
    case FILE_STREAM:
      errno = mapPalmOsErr(FileSeek(f->f, offset, whence));
      r = errno ? -1 : 0;
      break;
    case FILE_PDB:
    case FILE_MEMO:
    case FILE_RESOURCE:
      switch (whence) {
        case SEEK_SET:
          if (offset >= 0)
            f->recordPos = offset + f->recordOffset;
          break;
        case SEEK_CUR:
          f->recordPos = offset < 0 ?
                         f->recordPos - (UInt32)(-offset) :
                         f->recordPos + (UInt32)offset;
          break;
        case SEEK_END:
          f->recordPos = f->recordSize + offset;
          break;
      }
      if (f->recordPos > f->recordSize)
        f->recordPos = f->recordSize;
      else if (f->recordPos < f->recordOffset)
        f->recordPos = f->recordOffset;
      errno = 0;
      r = 0;
      break;
    case FILE_DOC:
      ErrorMsg("DOC fseek not implemented");
      errno = EBADF;
      r = -1;
      break;
    case FILE_DRIVER:
      r = -1;
      if ((driver = installedDrivers[f->driver]) == NULL)
        errno = EINVAL;
      else if (!driver->seek)
        errno = ENODEV;
      else {
        errno = mapPalmOsErr(driver->seek(f->handle, offset, whence));
        r = errno ? -1 : 0;
      }
      break;
    case FILE_VFS:
      switch (whence) {
        case SEEK_SET: whence = vfsOriginBeginning; break;
        case SEEK_CUR: whence = vfsOriginCurrent; break;
        case SEEK_END: whence = vfsOriginEnd;
      }
      err = VFSFileSeek(f->fref, whence, offset);
      errno = (err == vfsErrFileEOF) ? 0 : mapPalmOsErr(err);
      r = errno ? -1 : 0;
  }

  return r;
}

long ftell(FILE *f)
{
  Err err;
  DRIVER *driver;
  long p = 0;

  if (!f) {
    errno = EBADF;
    return -1;
  }

  switch (f->type) {
    case FILE_STREAM:
      p = FileTell(f->f, NULL, &err);
      errno = (err == vfsErrFileEOF) ? 0 : mapPalmOsErr(err);
      break;
    case FILE_PDB:
    case FILE_DOC:
    case FILE_MEMO:
    case FILE_RESOURCE:
      p = f->recordPos - f->recordOffset;
      errno = 0;
      break;
    case FILE_DRIVER:
      p = -1;
      if ((driver = installedDrivers[f->driver]) == NULL)
        errno = EINVAL;
      else if (!driver->tell)
        errno = ENODEV;
      else {
        errno = mapPalmOsErr(driver->tell(f->handle, &p));
        if (errno)
          p = -1;
      }
      break;
    case FILE_VFS:
      errno = mapPalmOsErr(VFSFileTell(f->fref, &p));
  }

  return p;
}

int fflush(FILE *f)
{
  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (f->type == FILE_STREAM) {
    errno = mapPalmOsErr(FileFlush(f->f));
    return errno ? -1 : 0;
  }

  errno = 0;
  return 0;
}

size_t fread(void *buf, size_t size, size_t n, FILE *f)
{
  Err err;
  Int32 m;
  size_t total, r = 0;
  DRIVER *driver;

  if (!f || !buf || !size || !n) {
    errno = EINVAL;
    return r;
  }

  errno = 0;

  switch (f->type) {
    case FILE_STREAM:
      r = FileRead(f->f, buf, size, n, &err);
      errno = (err == fileErrEOF) ? 0 : mapPalmOsErr(err);
      break;
    case FILE_PDB:
    case FILE_MEMO:
    case FILE_RESOURCE:
      r = dbread(buf, size, n, f);
      break;
    case FILE_DOC:
      if (f->ungetDone) {
        char *cbuf = (char *)buf;
        cbuf[0] = f->ungetChar;
        m = 1;
        total = n*size;
        if (total  > 1) {
          m = DocRead(f->doc, &cbuf[1], total - 1);
          if (m != -1)
            m++;
        }
        f->ungetDone = false;
      } else
        m = DocRead(f->doc, buf, n*size);
      r = m < 0 ? 0 : m;
      f->recordPos += r;
      break;
    case FILE_SERIAL:
      m = SerialReceive(f->handle, buf, size*n, &err);
      r = m < 0 ? 0 : m;
      errno = mapPalmOsErr(err);
      break;
    case FILE_DRIVER:
      r = 0;
      if ((driver = installedDrivers[f->driver]) == NULL)
        errno = EINVAL;
      else if (!driver->read)
        errno = ENODEV;
      else
        errno = mapPalmOsErr(driver->read(f->handle, buf, size*n, &r));
      break;
    case FILE_VFS:
      err = VFSFileRead(f->fref, size*n, buf, &m);
      errno = (err == vfsErrFileEOF) ? 0 : mapPalmOsErr(err);
      r = m < 0 ? 0 : m;
      break;
    case FILE_TCP:
    case FILE_UDP:
      if (netRef == -1) {
        errno = EINVAL;
        return r;
      }
      m = NetLibReceive(netRef, f->sock, buf, size*n, 0, 0, 0, 0, &err);
      errno = mapPalmOsErr(err);
      r = m < 0 ? 0 : m;
  }

  EvtResetAutoOffTimer();
  return r;
}

size_t fwrite(void *buf, size_t size, size_t n, FILE *f)
{
  Err err;
  Int32 m;
  size_t r = 0;
  DRIVER *driver;

  if (!f || !buf || !size || !n || !f->writable) {
    errno = EINVAL;
    return r;
  }

  errno = 0;

  switch (f->type) {
    case FILE_SCREEN:
      printstring((char *)buf, size*n);
      r = n;
      break;
    case FILE_DIALOG:
      ErrorMsg((char *)buf);
      r = n;
      break;
    case FILE_STREAM:
      r = FileWrite(f->f, buf, size, n, &err);
      errno = mapPalmOsErr(err);
      break;
    case FILE_PDB:
    case FILE_MEMO:
      r = dbwrite(buf, size, n, f);
      break;
    case FILE_SERIAL:
      m = SerialSend(f->handle, buf, size*n, &err);
      r = m < 0 ? 0 : n;
      errno = mapPalmOsErr(err);
      break;
    case FILE_DRIVER:
      r = 0;
      if ((driver = installedDrivers[f->driver]) == NULL)
        errno = EINVAL; 
      else if (!driver->write)
        errno = ENODEV;
      else
        errno = mapPalmOsErr(driver->write(f->handle, buf, size*n, &r));
      break;
    case FILE_VFS:
      errno = mapPalmOsErr(VFSFileWrite(f->fref, size*n, buf, &m));
      r = m < 0 ? 0 : n;
      break;
    case FILE_TCP:
    case FILE_UDP:
      if (netRef == -1) {
        errno = EINVAL;
        return r;
      }
      m = NetLibSend(netRef, f->sock, buf, size*n, 0, 0, 0, 0, &err);
      errno = mapPalmOsErr(err);
      r = m < 0 ? 0 : n;
  }

  EvtResetAutoOffTimer();
  return r;
}

int fgetc(FILE *f)
{
  char c;
  int n = fread(&c, 1, 1, f);
  return n == 1 ? c : EOF;
}

int fputc(int c, FILE *f)
{
  char ch = (char)c;
  int n = fwrite(&ch, 1, 1, f);
  return n == 1 ? c : EOF;
}

int ungetc(int c, FILE *f)
{
  if (c != EOF) {
    if (f->type == FILE_DOC) {
      f->ungetDone = true;
      f->ungetChar = (uchar)c;
      f->recordPos--;
      errno = 0;
    } else
      fseek(f, -1, SEEK_CUR);
  }
  return c;
}

int fputs(const char *s, FILE *f)
{
  fwrite((void *)s, 1, strlen(s), f);
  return 0;
}

char *fgets(char *s, int size, FILE *f)
{
  int i;

  errno = 0;

  if (!s || size <= 1)
    return NULL;

  for (i = 0; i < size-1; i++) {
    if (!fread(&s[i], 1, 1, f)) {
      s[i] = '\0';
      break;
    }
    if (s[i] == '\n') {
      s[i+1] = '\0';
      i++;
      break;
    }
  }

  return i ? s : NULL;
}

FILE *tmpfile(void)
{
  FILE *f;
  UInt32 p, q;
  UInt16 p1, p2, q1, q2;
  char name[32];

  p = GetCreator();
  p1 = p >> 16;
  p2 = p & 0xFFFF;
  q = clock();
  q1 = q >> 16;
  q2 = q & 0xFFFF;

  sprintf(name, "tmp%04X%04X%04X%04X", p1, p2, q1, q2);

  if ((f = fopen(name, "w")) == NULL)
    return NULL;

  f->tmpname = strdup(name);

  return f;
}

int fprintf(FILE *f, const char *fmt, ...)
{
  va_list argp;
  int n;

  va_start(argp, fmt);
  n = vsprintf(fprintfBuffer, fmt, argp);
  va_end(argp);
  fwrite((void *)fprintfBuffer, 1, strlen(fprintfBuffer), f);
  return n;
}

int sprintf(char * buf, const char *fmt, ...)
{
  va_list argp;
  int n;
                 
  va_start(argp, fmt);
  n = vsprintf(buf, fmt, argp);  
  va_end(argp);
  return n;
}

FILE *dbopen(const char *name, const char *mode)
{
  FILE *f;
  DmOpenRef dbRef;
  UInt32 type = 'data';
  UInt16 openMode = 0, attr;
  Boolean writable = true;
  Err err;

  if (!name || !name[0] || !mode || !mode[0]) {
    errno = EINVAL;
    return NULL;
  }

  switch (mode[0]) {
    case 'r':
      openMode = StrChr(mode, '+') != NULL ? dmModeReadWrite : dmModeReadOnly;
      writable = openMode == dmModeReadWrite;
      break;
    case 'w':
      openMode = dmModeWrite;
      break;
    default:
      errno = EINVAL;
      return NULL;
  }

  if (ReadOnlyCheck(writable, "open for writing DB", name)) {
    errno = EACCES;
    return NULL;
  }

  if (writable &&
      DmFindDatabase(0, name) == 0 &&
      (err = DbCreate((char *)name, type, GetCreator())) != 0) {
    errno = mapPalmOsErr(err);
    return NULL;
  }

  if (!(dbRef = DbOpenByName((char *)name, openMode, &err))) {
    errno = mapPalmOsErr(err);
    return NULL;
  }

  if ((err = DbGetAttributes(dbRef, &attr)) != 0) {
    errno = mapPalmOsErr(err);
    return NULL;
  }

  if (attr & dmHdrAttrResDB) {
    DbClose(dbRef);
    errno = EINVAL;
    return NULL;
  }

  if ((f = malloc(sizeof(FILE))) == NULL) {
    DbClose(dbRef);
    return NULL;
  }

  f->type = FILE_PDB;
  f->nRecords = DbNumRecords(dbRef);
  f->dbRef = dbRef;
  f->writable = writable;
  f->index = 0;
  f->recordOffset = 0;
  f->recordPos = 0;
  f->recordSize = 0;
  f->record = NULL;

  return add_file(f);
}

long dbnumrecs(FILE *f)
{
  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO))
    return -1;

  return f->nRecords;
}

int dbclose(FILE *f)
{
  Err err;

  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  dbcloserec(f);
  err = DbClose(f->dbRef);
  remove_file(f);
  free(f);

  errno = mapPalmOsErr(err);
  return errno ? -1 : 0;
}

int dbgetcat(FILE *f, UInt16 index, char *cat)
{
  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (index >= dmRecNumCategories || !cat) {
    errno = EINVAL;
    return -1;
  }

  CategoryGetName(f->dbRef, index, cat);
  return 0;
}

int dbsetcat(FILE *f, UInt16 index, char *cat)
{
  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (index >= dmRecNumCategories || !cat) {
    errno = EINVAL;
    return -1;
  }

  CategorySetName(f->dbRef, index, cat);
  return 0;
}

int dbgetreccat(FILE *f, UInt16 index)
{
  UInt16 attr;

  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (index >= f->nRecords) {
    errno = EINVAL;
    return -1;
  }

  errno = mapPalmOsErr(DbGetRecAttributes(f->dbRef, index, &attr));
  if (errno != 0)
    return -1;

  return attr & dmRecAttrCategoryMask;
}

int dbsetreccat(FILE *f, UInt16 index, UInt16 cat)
{
  UInt16 attr;

  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (index >= f->nRecords || cat >= dmRecNumCategories) {
    errno = EINVAL;
    return -1;
  }

  if (!f->writable) {
    errno = EACCES;
    return -1;
  }

  errno = mapPalmOsErr(DbGetRecAttributes(f->dbRef, index, &attr));
  if (errno != 0)
    return -1;

  attr = (attr & (~dmRecAttrCategoryMask)) | (cat & dmRecAttrCategoryMask);

  errno = mapPalmOsErr(DbSetRecAttributes(f->dbRef, index, attr));
  if (errno != 0)
    return -1;

  return 0;
}

long dbgetrecid(FILE *f, UInt16 index)
{
  UInt32 id;

  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (index >= f->nRecords) {
    errno = EINVAL;
    return -1;
  }

  errno = mapPalmOsErr(DbGetRecID(f->dbRef, index, &id));
  if (errno != 0)
    return -1;

  return id;
}

long dbopenrec(FILE *f, UInt16 index)
{
  Err err;

  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (f->record)
    dbcloserec(f);

  if (index >= f->nRecords) {
    errno = EINVAL;
    return -1;
  }

  if ((f->record = DbOpenRec(f->dbRef, index, &err)) == NULL) {
    errno = mapPalmOsErr(err);
    return -1;
  }

  f->index = index;
  f->recordSize = MemPtrSize(f->record);
  f->recordPos = 0;

  errno = 0;
  return f->recordSize;
}

long dbopeninfo(FILE *f)
{
  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (f->record)
    dbcloserec(f);

  if ((f->record = DbOpenAppInfo(f->dbRef)) == NULL) {
    errno = EINVAL;
    return -1;
  }

  f->index = 0xFFFF;
  f->recordSize = MemPtrSize(f->record);
  f->recordPos = 0;

  errno = 0;
  return f->recordSize;
}

long dbcreaterec(FILE *f, UInt16 size)
{
  UInt16 index;
  Err err;

  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (!f->writable) {
    errno = EACCES;
    return -1;
  }

  if ((err = DbCreateRec(f->dbRef, &index, size, 0)) != 0) {
    errno = mapPalmOsErr(err);
    return -1;
  }

  f->nRecords = DbNumRecords(f->dbRef);

  errno = 0;
  return index;
}

int dbdeleterec(FILE *f, UInt16 index)
{
  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (!f->writable) {
    errno = EACCES;
    return -1;
  }

  if ((f->record && index == f->index) || index >= f->nRecords) {
    errno = EINVAL;
    return -1;
  }

  if (f->record)
    dbcloserec(f);

  errno = mapPalmOsErr(DbDeleteRec(f->dbRef, index));
  return errno ? -1 : 0;
}

int dbremoverec(FILE *f, UInt16 index)
{
  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (!f->writable) {
    errno = EACCES;
    return -1;
  }

  if ((f->record && index == f->index) || index >= f->nRecords) {
    errno = EINVAL;
    return -1;
  }

  if (f->record)
    dbcloserec(f);

  errno = mapPalmOsErr(DbRemoveRec(f->dbRef, index));
  return errno ? -1 : 0;
}

int dbresizerec(FILE *f, UInt16 index, UInt32 size)
{
  Boolean reopen = false;
  Err err;

  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  if (!f->writable) {
    errno = EACCES;
    return -1;
  }

  if (index >= f->nRecords) {
    errno = EINVAL;
    return -1;
  }

  if (f->record && index == f->index) {
    dbcloserec(f);
    reopen = true;
  }

  if ((err = DbResizeRec(f->dbRef, index, size)) != 0) {
    errno = mapPalmOsErr(err);
    return -1;
  }

  if (reopen)
    return dbopenrec(f, index) == -1 ? -1 : 0;

  errno = 0;
  return 0;
}

int dbcloserec(FILE *f)
{
  if (!f || (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EBADF;
    return -1;
  }

  errno = 0;

  if (f->record) {
    if (f->index == 0xFFFF) {
      DbCloseAppInfo(f->record);

    } else {
      if (f->writable && f->type == FILE_MEMO)
        DbResizeRec(f->dbRef, f->index, StrLen(f->record)+1);
      errno = mapPalmOsErr(DbCloseRec(f->dbRef, f->index, f->record));
    }

    f->record = NULL;
    f->index = 0;
  }

  return errno ? -1 : 0;
}

size_t dbread(void *buf, size_t size, size_t n, FILE *f)
{
  size_t total;

  if (!f || !buf || !size || !n || !f->record) {
    errno = EINVAL;
    return 0;
  }

  if (f->type != FILE_PDB && f->type != FILE_MEMO &&
      f->type != FILE_RESOURCE) {
    errno = EBADF;
    return 0;
  }

  errno = 0;
  total = size * n;

  if ((f->recordPos + total) > f->recordSize)
    total = f->recordSize - f->recordPos;

  MemMove(buf, ((UInt8 *)f->record) + f->recordPos, total);
  f->recordPos += total;

  return total / size;
}

size_t dbwrite(void *buf, size_t size, size_t n, FILE *f)
{
  size_t total;
  Err err;

  if (!f || !buf || !size || !n || !f->writable || !f->record ||
      (f->type != FILE_PDB && f->type != FILE_MEMO)) {
    errno = EINVAL;
    return 0;
  }

  errno = 0;
  total = size * n;

  if ((f->recordPos + total) > f->recordSize) {
    errno = EFBIG;
    total = f->recordSize - f->recordPos;
  }

  if ((err = DmWriteCheck(f->record, f->recordPos, total)) != 0) {
    errno = mapPalmOsErr(err);
    return 0;
  }

  if ((err = DmWrite(f->record, f->recordPos, buf, total)) != 0) {
    errno = mapPalmOsErr(err);
    return 0;
  }

  f->recordPos += total;

  return total / size;
}

clock_t clock(void)
{
  UInt32 now;

  now = TimGetTicks();
  errno = 0;
  return now >= initialClock ? now - initialClock :
                               0xFFFFFFFFL - initialClock + now + 1;
}

time_t time(time_t *t)
{
  time_t _t = TimGetSeconds();

  if (t)
    *t = _t;
  errno = 0;
  return _t;
}

static struct tm *gettime(const time_t *t, Int32 d)
{
  if (!t) {
    errno = EINVAL;
    return NULL;
  }

  TimSecondsToDateTime(*t + d, &dateTime);
  localTime.tm_sec = dateTime.second;
  localTime.tm_min = dateTime.minute;
  localTime.tm_hour = dateTime.hour;
  localTime.tm_mday = dateTime.day;
  localTime.tm_mon = dateTime.month-1;
  localTime.tm_year = dateTime.year - 1900;
  localTime.tm_wday = dateTime.weekDay;
  
  errno = 0;
  return &localTime;
}

static Int32 deltatime(void)
{
  // retorna o ajuste de tempo em minutos

  Int32 d = 0;

  if (GetRomVersionNumber() >= 40) {
    d += ((Int32)PrefGetPreference(prefTimeZone));
    d += ((Int32)PrefGetPreference(prefDaylightSavingAdjustment));
  } else {
    d += (((Int32)PrefGetPreference(prefMinutesWestOfGMT)) - 720);
    if (PrefGetPreference(prefDaylightSavings) != dsNone)
      d += 60;
  }

  return d;
}

struct tm *gmtime(const time_t *t)
{
  return gettime(t, deltatime()*60);
}

struct tm *localtime(const time_t *t)
{ 
  return gettime(t, 0);
}

time_t mktime(struct tm *tm)
{
  if (!tm) {
    errno = EINVAL;
    return 0;
  }

  dateTime.second = tm->tm_sec;
  dateTime.minute = tm->tm_min;
  dateTime.hour = tm->tm_hour;
  dateTime.day = tm->tm_mday;
  dateTime.month = tm->tm_mon+1;
  dateTime.year = tm->tm_year + 1900;
  dateTime.weekDay = tm->tm_wday;

  errno = 0;
  return TimDateTimeToSeconds(&dateTime);
}

size_t strftime(char *buf, size_t maxsize, const char *format,
                const struct tm *tm)
{
  DateFormatType dateFormat;
  TimeFormatType timeFormat;
  Int16 i, k, s;
  Int32 d;
  Boolean neg;
  char c;

  errno = 0;

  if (!buf || !maxsize || !format || !tm)
    return 0;

  dateFormat = (DateFormatType)PrefGetPreference(prefLongDateFormat);
  timeFormat = (TimeFormatType)PrefGetPreference(prefTimeFormat);

  for (s = 0, i = 0, k = 0; (c = format[i]); i++)
    if (!s) {
      if (c != '%') {
        buf[k++] = c;
        if (k == maxsize)
          return 0;
      } else
        s = 1;
    } else {
      switch (c) {
      case '%':
        buf[k++] = c;
        break;
      case 'C':
        StrPrintF(&buf[k], "%02d", (tm->tm_year + 1900)/ 100);
        k += 2;
        break;
      case 'c':
        DateToAscii(tm->tm_mon+1, tm->tm_mday, tm->tm_year + 1900,
          dateFormat, strDate);
        TimeToAscii(tm->tm_hour, tm->tm_min, timeFormat, strTime);
        StrPrintF(&buf[k], "%s  %s", strDate, strTime);
        k += StrLen(strDate) + StrLen(strTime) + 2;
        break;
      case 'D':
        StrPrintF(&buf[k], "%02d/%02d/%02d",
            tm->tm_mon+1, tm->tm_mday, (tm->tm_year + 1900) % 100);
        k += 8;
        break;
      case 'd':
      case 'e':
        StrPrintF(&buf[k], "%02d", tm->tm_mday);
        k += 2;
        break;
      case 'H':
      case 'k':
        StrPrintF(&buf[k], "%02d", tm->tm_hour);
        k += 2;
        break;
      case 'M':
        StrPrintF(&buf[k], "%02d", tm->tm_min);
        k += 2;
        break;
      case 'm':
        StrPrintF(&buf[k], "%02d", tm->tm_mon+1);
        k += 2;
        break;
      case 'n':
        buf[k++] = '\n';
        break;
      case 'S':
        StrPrintF(&buf[k], "%02d", tm->tm_sec);
        k += 2;
        break;
      case 'T':
        StrPrintF(&buf[k], "%02d:%02d:%02d",
            tm->tm_hour, tm->tm_min, tm->tm_sec);
        k += 8;
        break;
      case 't':
        buf[k++] = '\t';
        break;
      case 'u':
        StrPrintF(&buf[k++], "%d", tm->tm_wday+1);
        break;
      case 'w':
        StrPrintF(&buf[k++], "%d", tm->tm_wday);
        break;
      case 'Y':
        StrPrintF(&buf[k], "%d", tm->tm_year + 1900);
        k += 4;
        break;
      case 'y':
        StrPrintF(&buf[k], "%02d", (tm->tm_year + 1900) % 100);
        k += 2;
        break;
      case 'z':
        d = deltatime();
        if (d < 0) {
          neg = true;
          d = -d;
        } else
          neg = false;
        StrPrintF(&buf[k], neg ? "-%02d%02d" : "+%02d%02d",
          (Int16)(d/60), (Int16)(d%60));
        k += 5;
        break;
      default:
        return 0;
      }
      s = 0;
    }

  buf[k] = '\0';

  return k;
}

int tolower(int c)
{
  unsigned char src = c;
  if (src >= 'A' && src <= 'Z')
    src += 32;
  else if (src >= 128) {
    char dsts[2], srcs[2];
    srcs[0] = src;
    srcs[1] = 0;
    dsts[0] = 0;
    StrToLower(dsts, srcs);
    src = dsts[0];
  }
  return (int)src;
}

int toupper(int c)
{
  unsigned char src = c;
  if (src >= 'a' && src <= 'z')
    src -= 32;
  else if (src >= 128)
    src = TxtGlueUpperChar(src);
  return (int)src;
}

void file_init(void)
{
  UInt32 version;

  stdin  = &defaultStdin;
  stdout = &defaultStdout;
  stderr = &defaultStderr;
  netRef = -1;
  ioFile = 0;
  errno = 0;
  initialClock = TimGetTicks();
  CLOCKS_PER_SEC = SysTicksPerSecond();

  MemSet(openFiles, sizeof(openFiles), 0);
  MemSet(installedDrivers, sizeof(installedDrivers), 0);
  hasVFS = FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &version) == 0;
}

void file_finish(void)
{
  Int16 i;

  for (i = 0; i < MAX_FILES; i++)
    if (openFiles[i]) {
      fclose(openFiles[i]);
      openFiles[i] = NULL;
    }
}

void vfs_init(char *dir)
{
  UInt32 iterator;
  UInt16 aux;
  char path[128];

  if (hasVFS && dir != NULL) {
    iterator = vfsIteratorStart;
    if (VFSVolumeEnumerate(&aux, &iterator) == 0) {
      strcpy(path, "/PALM/Programs");
      VFSDirCreate(aux, path);
      strcat(path, "/");
      strcat(path, dir);
      VFSDirCreate(aux, path);
      strcat(path, "/src");
      VFSDirCreate(aux, path);
    }
  }
}

int has_vfs(void)
{
  return hasVFS;
}

void CreatorToString(UInt32 creator, char *s)
{
  s[0] = (char)(creator >> 24);
  s[1] = (char)(creator >> 16);
  s[2] = (char)(creator >> 8);
  s[3] = (char)creator;
  s[4] = '\0';   
}

UInt32 StringToCreator(char *s)
{
  UInt32 creator;

  if (!s || StrLen(s) != 4)
    return 0;

  creator = (((UInt32)s[0]) << 24) |
            (((UInt32)s[1]) << 16) |
            (((UInt32)s[2]) << 8) |
            ((UInt32)s[3]);
  return creator;
}

static FILE *add_file(FILE *f)
{
  Int16 i;

  for (i = 0; i < MAX_FILES; i++) {
    if (!openFiles[i]) {
      openFiles[i] = f;
      errno = 0;
      return f;
    }
  }

  fclose(f);
  ErrorMsg("File table overflow");
  errno = EMFILE;
  return NULL;
}

static void remove_file(FILE *f)
{
  Int16 i;

  for (i = 0; i < MAX_FILES; i++) {
    if (openFiles[i] == f) {
      openFiles[i] = NULL;
      return;
    }
  }

  ErrorMsg("Closed file was not open !");
}

int add_driver(DRIVER *f)
{
  Int16 i;

  for (i = 0; i < MAX_DRIVERS; i++) {
    if (!installedDrivers[i]) {
      installedDrivers[i] = f;
      errno = 0;
      return i;
    }
  }

  ErrorMsg("Driver table overflow");
  errno = EMFILE;
  return -1;
}

int remove_driver(int i)
{
  Int16 j;

  errno = 0;

  if (i >= 0 && i < MAX_DRIVERS && installedDrivers[i]) {
    for (j = 0; j < MAX_FILES; j++)
      if (openFiles[j] && openFiles[j]->type == FILE_DRIVER &&
          openFiles[j]->driver == i) {
        errno = EBUSY;
        return -1;
      }
    free(installedDrivers[i]);
    installedDrivers[i] = NULL;
    return 0;
  }

  ErrorMsg("Removed driver was not installed !");
  errno = EINVAL;
  return -1;
}

int mapPalmOsErr(Err err)
{
  switch (err) {
    case fileErrMemError:
    case memErrNotEnoughSpace:
    case serErrNoMem:
    case dmErrMemError:
    case netErrOutOfMemory:
    case sndErrMemory:
      err = ENOMEM;
      break;
    case fileErrInvalidParam:
    case serErrBadParam:
    case serErrNotSupported:
    case memErrInvalidParam:
    case dmErrInvalidParam:
    case dmErrIndexOutOfRange:
    case dmErrWriteOutOfBounds:
    case dmErrInvalidDatabaseName:
    case netErrParamErr:
    case netErrTooManyInterfaces:
    case netErrConfigTooMany:
    case vfsErrBufferOverflow:
    case vfsErrNoFileSystem:
    case vfsErrBadName:
    case expErrNotOpen:
    case sndErrBadParam:
    case sndErrQFull:
    case sndErrFormat:
    case sndErrBadChannel:
    case sndErrBadStream:
      err = EINVAL;
      break;
    case fileErrNotFound:
    case dmErrCantFind:
    case dmErrResourceNotFound:
    case dmErrRecordDeleted:
    case vfsErrFileNotFound:
    case vfsErrDirectoryNotFound:
    case expErrEnumerationEmpty:
      err = ENOENT;
      break;
    case fileErrPermissionDenied:
    case fileErrReadOnly:
    case fileErrInUse:
    case serErrAlreadyOpen:
    case memErrWriteProtect:
    case dmErrReadOnly:
    case dmErrRecordBusy:
    case dmErrAlreadyOpenForWrites:
    case dmErrOpenedByAnotherTask:
    case dmErrDatabaseProtected:
    case vfsErrFilePermissionDenied:
    case sndErrOpen:
      err = EACCES;
      break;
    case fileErrNotStream:
    case fileErrInvalidDescriptor:
    case serErrBadPort:
    case serErrBadConnID:
    case serErrNotOpen:
    case vfsErrFileBadRef:
    case vfsErrVolumeBadRef:
      err = EBADF;
      break;
    case dmErrAlreadyExists:
    case vfsErrFileAlreadyExists:
      err = EEXIST;
      break;
    case vfsErrNotADirectory:
      err = ENOTDIR;
      break;
    case vfsErrIsADirectory:
      err = EISDIR;
      break;
    case vfsErrVolumeFull:
      err = ENOSPC;
      break;
    case sndErrInterrupted:
      err = EINTR;
      break;
    case netErrNotOpen:
      err = ENETDOWN;
      break;
    case netErrSocketNotOpen:
    case netErrSocketNotConnected:
      err = ENOTCONN;
      break;
    case netErrMessageTooBig:
      err = EMSGSIZE;
      break;
    case netErrSocketBusy:
      err = EINPROGRESS;
      break;
    case netErrInvalidInterface:
    case netErrInterfaceNotFound:
    case netErrConfigNotFound:
    case netErrConfigBadName:
    case netErrConfigNotAlias:
    case netErrConfigCantPointToAlias:
    case netErrConfigEmpty:
      err = ENXIO;
      break;
    case netErrUnimplemented:
      err = EOPNOTSUPP;
      break;
    case netErrQuietTimeNotElapsed:
    case netErrPortInUse:
      err = EADDRINUSE;
      break;
    case serErrTimeOut:
    case netErrTimeout:
      err = ETIMEDOUT;
      break;
    case netErrSocketAlreadyConnected:
      err = EISCONN;
      break;
    case netErrSocketClosedByRemote:
      err = ECONNREFUSED;
      break;
    case netErrWrongSocketType:
      err = ESOCKTNOSUPPORT;
      break;
    case netErrUnknownProtocol:
      err = EPROTONOSUPPORT;
      break;
    case netErrNoInterfaces:
    case netErrUnreachableDest:
      err = ENETUNREACH;
      break;
    case netErrWouldBlock:
      err = EWOULDBLOCK;
      break;
    case netErrAlreadyInProgress:
      err = EALREADY;
      break;
    case netErrPPPAddressRefused:
      err = EADDRNOTAVAIL;
      break;
    case netErrIPNoRoute:
      err = EHOSTUNREACH;
      break;
    case 0:
      break;
    default:
      ErrorMsg("ERROR %d", err);
      err = EUNKNOWN;
  }
  return err;
}

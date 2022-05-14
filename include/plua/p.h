#ifndef _P_H
#define _P_H

#include <PalmOS.h>
#include <VFSMgr.h>
#include <TxtGlue.h>
#include <BmpGlue.h>
#include <WinGlue.h>
#include <CtlGlue.h>
#include <LstGlue.h>
#include <SerialMgrOld.h>
#include <Common/System/palmOneNavigator.h>

#ifdef MULTILINK
#define SEC(name)
#define const
#else
#define SEC(name) __attribute__ ((section (name)))
#endif

#define RAND_MAX	sysRandomMax
#define EXIT_SUCCESS	0
#define EXIT_FAILURE	-1
#define MAX_FILES	64
#define MAX_LIBS	32
#define MAX_DRIVERS	32

typedef	UInt8		u_char;
typedef	UInt16		u_short;
typedef	UInt16		u_int;
typedef	UInt32		u_long;

typedef	UInt8		uchar;
typedef	UInt16		ushort;
typedef	UInt16		uint;
typedef	UInt32		ulong;

typedef UInt32		size_t;
typedef UInt32		time_t;
typedef UInt32		mode_t;
typedef UInt32		clock_t;
typedef Int32		ptrdiff_t;
typedef Int32		off_t;
typedef ErrJumpBuf	jmp_buf;

#define FILE_NULL       0
#define FILE_SCREEN     1
#define FILE_DIALOG     2
#define FILE_STREAM     3
#define FILE_PDB        4
#define FILE_MEMO       5
#define FILE_DOC        6
#define FILE_RESOURCE   7
#define FILE_SERIAL     8
#define FILE_VFS        9
#define FILE_VFSDIR     10
#define FILE_TCP        11
#define FILE_UDP        12
#define FILE_DRIVER     13

typedef struct _file {
  Int8 type;
  FileHand f;
  DmOpenRef dbRef;
  void *doc;
  FileRef fref;
  NetSocketRef sock;
  MemHandle res;
  char *tmpname;
  UInt16 vol, driver, handle;
  Boolean writable;
  UInt32 nRecords, index, recordSize, recordOffset, recordPos, iterator;
  MemPtr record;
  Boolean ungetDone;
  char ungetChar;
  UInt32 resType;
  char *realname;
} FILE;

typedef struct _file DIR;

struct dirent {
  unsigned char d_type;
  char d_name[256];
};

#define DT_UNKNOWN     0
#define DT_FIFO        1
#define DT_CHR         2
#define DT_DIR         4 
#define DT_BLK         6
#define DT_REG         8
#define DT_LNK         10
#define DT_SOCK        12
#define DT_WHT         14

FILE *fopen(const char *, const char *) SEC("aux");

typedef struct {
  char prefix[8];
  int (*open)(int id, char *path, int *handle);
  int (*close)(int handle);
  int (*read)(int handle, void *buf, size_t len, size_t *n);
  int (*write)(int handle, void *buf, size_t len, size_t *n);
  int (*seek)(int handle, off_t offset, int whence);
  int (*tell)(int handle, size_t *offset);
  int (*status)(int handle);
} DRIVER;

#define SOCK_STREAM	1
#define SOCK_DGRAM	2

struct tm {
  int tm_sec;     /* seconds (0 - 60) */
  int tm_min;     /* minutes (0 - 59) */
  int tm_hour;    /* hours (0 - 23) */
  int tm_mday;    /* day of month (1 - 31) */
  int tm_mon;     /* month of year (0 - 11) */
  int tm_year;    /* year - 1900 */
  int tm_wday;    /* day of week (Sunday = 0) */
  int tm_yday;    /* day of year (0 - 365) */
  int tm_isdst;   /* is summer time in effect? */
  char *tm_zone;  /* abbreviation of timezone name */
  long tm_gmtoff; /* offset from UTC in seconds */
};

typedef struct {
  Int32 value;
  char *name;
} app_constant;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
extern int errno;
extern UInt32 CLOCKS_PER_SEC;

#define SEEK_SET fileOriginBeginning
#define SEEK_CUR fileOriginCurrent
#define SEEK_END fileOriginEnd

#define IOCTL_RX_CARRIER	1
#define IOCTL_TX_CARRIER	2
#define IOCTL_INVERTED		3

#define BUFSIZ 256

int add_driver(DRIVER *f) SEC("aux");
int remove_driver(int i) SEC("aux");

void _exit(int status);
char *strerror(int errnum);
FILE *fopen(const char *, const char *) SEC("aux");
FILE *freopen(const char *, const char *, FILE *);
int fclose(FILE *);
int feof(FILE *);
int ferror(FILE *);
int fseek(FILE *, long, int) SEC("aux");
long ftell(FILE *) SEC("aux");
int fflush(FILE *);
size_t fread(void *, size_t, size_t, FILE *) SEC("aux");
size_t fwrite(void *, size_t, size_t, FILE *) SEC("aux");
int fgetc(FILE *);
int fputc(int , FILE *);
int ungetc(int, FILE *);
int fputs(const char *, FILE *);
char *fgets(char *, int, FILE *);
FILE *tmpfile(void);
char *getenv(const char *name);

#define exit(s) _exit(s)
#define getc(f) fgetc(f)

DIR *opendir(const char *) SEC("aux");
int closedir(DIR *) SEC("aux");
struct dirent *readdir(DIR *) SEC("aux");
int mkdir(const char *, mode_t) SEC("aux");
int rmdir(const char *) SEC("aux");

int remove(const char *);
int rename(const char *, const char *);

FILE *dbopen(const char *, const char *) SEC("aux");
long dbnumrecs(FILE *) SEC("aux");
int dbclose(FILE *) SEC("aux");
int dbgetcat(FILE *, UInt16, char *) SEC("aux");
int dbsetcat(FILE *, UInt16, char *) SEC("aux");
long dbopenrec(FILE *, UInt16) SEC("aux");
long dbcreaterec(FILE *, UInt16) SEC("aux");
int dbdeleterec(FILE *, UInt16) SEC("aux");
int dbremoverec(FILE *, UInt16) SEC("aux");
int dbresizerec(FILE *, UInt16, UInt32) SEC("aux");
int dbcloserec(FILE *) SEC("aux");
int dbgetreccat(FILE *, UInt16) SEC("aux");
int dbsetreccat(FILE *, UInt16, UInt16) SEC("aux");
long dbgetrecid(FILE *, UInt16) SEC("aux");
long dbopeninfo(FILE *f) SEC("aux");

size_t dbread(void *, size_t, size_t, FILE *) SEC("aux");
size_t dbwrite(void *, size_t, size_t, FILE *) SEC("aux");

FILE *seropen(const char *name, ulong baud, const char *mode) SEC("aux");
int serclose(FILE *) SEC("aux");

Int16 netinit(void) SEC("aux");
FILE *netopen(const char *host, ushort port, ushort protocol, int dnsTimeout, int connectTimeout, int linger) SEC("aux");
FILE *netserver(ushort port, ushort protocol) SEC("aux");
int netclose(FILE *) SEC("aux");

clock_t clock(void) SEC("aux");
time_t time(time_t *) SEC("aux");
struct tm *localtime(const time_t *) SEC("aux");
struct tm *gmtime(const time_t *) SEC("aux");
time_t mktime(struct tm *tm) SEC("aux");
size_t strftime(char *buf, size_t maxsize, const char *format, const struct tm *timeptr) SEC("aux");

double strtod(const char *nptr, char **endptr);

void mem_init(void);
size_t mem_size(void);
size_t mem_available(void);
size_t mem_threshold(size_t used, size_t threshold);

void *malloc(size_t size);
void *calloc(size_t number, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

#define longjmp(a,b)            ErrLongJump(a,b)
#define setjmp(a)               ErrSetJump(a)

#define atoi(x)			StrAToI(x)
#define atol(x)			StrAToI(x)

#define rand()			SysRandom(0)
#define srand(x)		SysRandom(x)

#define strcpy(x,y)		StrCopy((Char*)(x),(Char*)(y))
#define strncpy(x,y,z)		StrNCopy(x,y,z)
#define strcmp(x,y)		StrCompare(x,y)
#define strncmp(x,y,z)		StrNCompare(x,y,z)
#define strcoll(x,y)		StrCompare(x,y)
#define strcat(x,y)		StrCat(x,y)
#define strncat(x,y,z)		StrNCat(x,y,z)
#define strlen(x)		StrLen((Char*)(x))
#define strchr(x,y)		StrChr(x,y)
#define strstr(x,y)		StrStr(x,y)

char *strdup(const char *str);
size_t strcspn(const char *s, const char *r);
void *memchr(const void *b, int c, size_t len);
char *strpbrk(const char *s, const char *charset);


#define memcpy(x,y,z)		(MemMove(x,(void *)y,z) ? x : x)
#define memset(x,y,z)		(MemSet(x,z,y))
#define memcmp(x,y,z)		(MemCmp(x,y,z))
#define index(x,y)		StrChr(x,y)

#define STDIN_FILENO		sysFileDescStdIn
#define STDOUT_FILENO		-1	

#define BITS_INT		32
#define INT_MAX			0x7FFFFFFFUL

#define UCHAR_MAX		0xFF
#define ULONG_MAX		0xFFFFFFFFUL

typedef	NetFDSetType		fd_set;
#define	FD_SETSIZE		netFDSetSize

#define	FD_SET(n,p)		netFDSet(n,p)
#define	FD_CLR(n,p)		netFDClr(n,p)
#define	FD_ISSET(n,p)		netFDIsSet(n,p)
#define	FD_ZERO(p)		netFDZero(p)

#define	bcopy(b1,b2,len)	MemMove(b2,b1,len)
#define	bzero(b,len)		MemSet(b,len,0)
#define	bcmp(b1,b2,len) 	MemCmp(b1,b2,len)

#define isascii(c)		(((unsigned) c)<=0x7f)
#define toascii(c)		(((unsigned) c)&0x7f)

#define isalnum(c)		(isascii(c) && TxtCharIsAlNum(c))
#define isalpha(c)		(isascii(c) && TxtCharIsAlpha(c))
#define iscntrl(c)		(isascii(c) && TxtCharIsCntrl(c))
#define isdigit(c)		(isascii(c) && TxtCharIsDigit(c))
#define isgraph(c)		(isascii(c) && TxtCharIsGraph(c))
#define islower(c)		(isascii(c) && TxtCharIsLower(c))
#define isprint(c)		(isascii(c) && TxtCharIsPrint(c))
#define ispunct(c)		(isascii(c) && TxtCharIsPunct(c))
#define isspace(c)		(isascii(c) && TxtCharIsSpace(c))
#define isupper(c)		(isascii(c) && TxtCharIsUpper(c))
#define isxdigit(c)		(isascii(c) && TxtCharIsDigit(c))

int tolower(int c);
int toupper(int c);

#ifndef __va_list__
#define __va_list__
typedef char * va_list;		// from <va_list.h>
#endif /* __va_list__ */

#define __va_start(parm)	(va_list) (&parm + 1)

#define va_start(ap, parm)	ap = __va_start(parm)
#define va_end(ap)

#define va_arg(ap, type)	(*(((type *) (ap += sizeof(type))) - 1))

int fprintf(FILE *, const char *, ...);
int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list ap);

void file_init(void);
void file_finish(void);

void vfs_init(char *);
int has_vfs(void);

int iocheck(void);
int iostatus(FILE *f);

void CreatorToString(UInt32, char *);
UInt32 StringToCreator(char *);
int mapPalmOsErr(Err err);

#define	ENOENT		2		// No such file or directory
#define	EINTR		4		// Interrupted system call
#define	EIO		5		// Input/output error
#define ENXIO		6
#define	EBADF		9		// Bad file descriptor
#define	ENOMEM		12		// Cannot allocate memory
#define	EACCES		13		// Permission denied
#define	EBUSY		16		// Device busy
#define	EEXIST		17		// File exists
#define	ENODEV		19		// Operation not supported by device
#define ENOTDIR         20		// Not a directory
#define EISDIR          21		// Is a directory
#define	EINVAL		22		// Invalid argument
#define	EMFILE		24		// Too many open files
#define	EFBIG		27		// File too large
#define	ENOSPC		28		// No space left on device
#define EWOULDBLOCK	35
#define EINPROGRESS	36
#define EALREADY	37
#define EMSGSIZE	40
#define EPROTONOSUPPORT	43
#define ESOCKTNOSUPPORT	44
#define EOPNOTSUPP	45
#define EADDRINUSE	48
#define EADDRNOTAVAIL	49
#define ENETDOWN	50
#define ENETUNREACH	51
#define ECONNRESET	54
#define EISCONN		56
#define ENOTCONN	57
#define ETIMEDOUT	60
#define	ECONNREFUSED	61
#define EHOSTUNREACH	65

#define	EUNKNOWN	999

#endif


#ifdef NOT_DEFINED
#define	EPERM		1		/* Operation not permitted */
#define	ENOENT		2		/* No such file or directory */
#define	EINTR		4		/* Interrupted system call */
#define	ESRCH		3		/* No such process */
#define	EIO		5		/* Input/output error */
#define	ENXIO		6		/* Device not configured */
#define	E2BIG		7		/* Argument list too long */
#define	ENOEXEC		8		/* Exec format error */
#define	EBADF		9		/* Bad file descriptor */
#define	ECHILD		10		/* No child processes */
#define	EDEADLK		11		/* Resource deadlock avoided */
					/* 11 was EAGAIN */
#define	ENOMEM		12		/* Cannot allocate memory */
#define	EACCES		13		/* Permission denied */
#define	EFAULT		14		/* Bad address */
#define	ENOTBLK		15		/* Block device required */
#define	EBUSY		16		/* Device busy */
#define	EEXIST		17		/* File exists */
#define	EXDEV		18		/* Cross-device link */
#define	ENODEV		19		/* Operation not supported by device */
#define	ENOTDIR		20		/* Not a directory */
#define	EISDIR		21		/* Is a directory */
#define	EINVAL		22		/* Invalid argument */
#define	ENFILE		23		/* Too many open files in system */
#define	EMFILE		24		/* Too many open files */
#define	ENOTTY		25		/* Inappropriate ioctl for device */
#define	ETXTBSY		26		/* Text file busy */
#define	EFBIG		27		/* File too large */
#define	ENOSPC		28		/* No space left on device */
#define	ESPIPE		29		/* Illegal seek */
#define	EROFS		30		/* Read-only file system */
#define	EMLINK		31		/* Too many links */
#define	EPIPE		32		/* Broken pipe */

/* math software */
#define	EDOM		33		/* Numerical argument out of domain */
#define	ERANGE		34		/* Result too large */

/* non-blocking and interrupt i/o */
#define	EAGAIN		35		/* Resource temporarily unavailable */
#define	EWOULDBLOCK	EAGAIN		/* Operation would block */
#define	EINPROGRESS	36		/* Operation now in progress */
#define	EALREADY	37		/* Operation already in progress */

/* ipc/network software -- argument errors */
#define	ENOTSOCK	38		/* Socket operation on non-socket */
#define	EDESTADDRREQ	39		/* Destination address required */
#define	EMSGSIZE	40		/* Message too long */
#define	EPROTOTYPE	41		/* Protocol wrong type for socket */
#define	ENOPROTOOPT	42		/* Protocol not available */
#define	EPROTONOSUPPORT	43		/* Protocol not supported */
#define	ESOCKTNOSUPPORT	44		/* Socket type not supported */
#define	EOPNOTSUPP	45		/* Operation not supported */
#define	EPFNOSUPPORT	46		/* Protocol family not supported */
#define	EAFNOSUPPORT	47		/* Address family not supported by protocol family */
#define	EADDRINUSE	48		/* Address already in use */
#define	EADDRNOTAVAIL	49		/* Can't assign requested address */

/* ipc/network software -- operational errors */
#define	ENETDOWN	50		/* Network is down */
#define	ENETUNREACH	51		/* Network is unreachable */
#define	ENETRESET	52		/* Network dropped connection on reset */
#define	ECONNABORTED	53		/* Software caused connection abort */
#define	ECONNRESET	54		/* Connection reset by peer */
#define	ENOBUFS		55		/* No buffer space available */
#define	EISCONN		56		/* Socket is already connected */
#define	ENOTCONN	57		/* Socket is not connected */
#define	ESHUTDOWN	58		/* Can't send after socket shutdown */
#define	ETOOMANYREFS	59		/* Too many references: can't splice */
#define	ETIMEDOUT	60		/* Operation timed out */
#define	ECONNREFUSED	61		/* Connection refused */

#define	ELOOP		62		/* Too many levels of symbolic links */
#define	ENAMETOOLONG	63		/* File name too long */

/* should be rearranged */
#define	EHOSTDOWN	64		/* Host is down */
#define	EHOSTUNREACH	65		/* No route to host */
#define	ENOTEMPTY	66		/* Directory not empty */

/* quotas & mush */
#define	EPROCLIM	67		/* Too many processes */
#define	EUSERS		68		/* Too many users */
#define	EDQUOT		69		/* Disc quota exceeded */

/* Network File System */
#define	ESTALE		70		/* Stale NFS file handle */
#define	EREMOTE		71		/* Too many levels of remote in path */
#define	EBADRPC		72		/* RPC struct is bad */
#define	ERPCMISMATCH	73		/* RPC version wrong */
#define	EPROGUNAVAIL	74		/* RPC prog. not avail */
#define	EPROGMISMATCH	75		/* Program version wrong */
#define	EPROCUNAVAIL	76		/* Bad procedure for program */

#define	ENOLCK		77		/* No locks available */
#define	ENOSYS		78		/* Function not implemented */

#define	EFTYPE		79		/* Inappropriate file type or format */
#define	EAUTH		80		/* Authentication error */
#define	ENEEDAUTH	81		/* Need authenticator */
#define	EIDRM		82		/* Identifier removed */
#define	ENOMSG		83		/* No message of desired type */
#define	EOVERFLOW	84		/* Value too large to be stored in data type */
#define	ECANCELED	85		/* Operation canceled */
#define	EILSEQ		86		/* Illegal byte sequence */
#define	ELAST		86		/* Must be equal largest errno */

#endif

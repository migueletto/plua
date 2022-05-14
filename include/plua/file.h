#ifndef _FILE_H
#define _FILE_H 1

struct FileRecType {
  UInt16 cardNo;
  LocalID dbID;
  char name[dmDBNameLength];
  char *lname;  // para VFS
  UInt16 index; // para memo
};
typedef struct FileRecType FileRecType;

struct FileType {
  Int16 n;
  char **fname;
  FileRecType *rec;
};
typedef struct FileType FileType;

Err CreateMemoList(char *, FileType *, char *, char *) SEC("aux");
Int16 CountMemoFiles(DmOpenRef, char *, char *) SEC("aux");
Int16 ListMemoFiles(DmOpenRef, char *, char *, Int16, FileRecType *) SEC("aux");

Err CreateFileList(UInt32, UInt32, FileType *, char *, UInt16) SEC("aux");
Int16 CountFiles(UInt32, UInt32, char *, UInt16) SEC("aux");
Int16 ListFiles(UInt32, UInt32, char *, Int16, FileRecType *, UInt16) SEC("aux");

Err CreateStreamFileList(FileType *, char *) SEC("aux");
Int16 CountStreamFiles(char *) SEC("aux");
Int16 ListStreamFiles(char *, Int16, FileRecType *) SEC("aux");

Err CreateVfsFileList(char *, FileType *, char *, Boolean) SEC("aux");
Int16 CountVfsFiles(char *, char *, Boolean) SEC("aux");
Int16 ListVfsFiles(char *, char *, Int16, FileRecType *, Boolean) SEC("aux");

Err CreateResourceList(char *, UInt32, FileType *) SEC("aux");

Err DestroyFileList(FileType *) SEC("aux");

#endif

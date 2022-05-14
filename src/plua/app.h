#ifndef _APP_H
#define _APP_H 1

#define scrType     'ScrT'
#define scrID       1

#define CONTROL_BASE    1024UL
#define SPRITE_BASE     1536UL
#define SYSTEM_BASE     2048UL

// CONTROL: 1024 -> 1223  (1223  = 1024 + LastInternID - FirstInternID)
// SPRITE:  1536 -> 1567  (1567  = 1024 + 31)
// SYSTEM : 2048 -> 34815 (34815 = 2048 + lastUserEvent)

typedef struct AppPB {
  UInt32 magic;
  UInt16 version;
  LocalID callerID;
} AppPB;

typedef struct {
  Boolean readOnly;
  Boolean clearOutput;
  UInt32 time;
  UInt32 salt;
  UInt32 key;
  UInt32 sig;
  UInt16 memoIndex;
  UInt16 docIndex;
  UInt16 streamIndex;
  UInt16 vfsIndex;
  UInt16 initialForm;
} AppPrefs;

Err AppInit(void *);
void AppFinish(void);
void AppBreak(void);
Boolean AppFrmClose(Int16);
void AppFrmLoad(FormPtr, Int16);
void AppRunBuffer(char *buf, int len);
void AppVmStart(void);
Boolean AppReadOnly(void);
Err AppCompile(char *in, char *out);
void AppSetRegistry(UInt32 key, void *value);
void *AppGetRegistry(UInt32 key);
char *AppName(void);

AppPrefs *GetPrefs(void);
void SetEventHandler(FormPtr, Int16);
void RestoreMenu(void);

void *GetContext(void);
void SetCode(char *);

#endif

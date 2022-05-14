#ifndef _MAIN_H
#define _MAIN_H 1

UInt32 GetCreator(void);
char *GetAppVersion(void);
char *GetRomVersion(void);
Int16 GetRomVersionNumber(void);
char *GetMemoName(void);
UInt32 GetCodeType(void);
UInt16 GetCodeID(void);
UInt32 GetLibType(void);
char *GetExtention(void);
char *GetSrcDir(void);
char *GetHelpFile(void);
Boolean ProcessEvent(EventType *event, Int32 wait, Boolean app);
Boolean ReadOnlyCheck(Boolean writting, char *op, const char *arg);
void SetLastForm(Int16);
Int16 GetLastForm(void);

#endif

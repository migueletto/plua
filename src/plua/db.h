DmOpenRef DbOpen(LocalID dbID, UInt16 mode, Err *err) SEC("aux");
DmOpenRef DbOpenByName(char *name, UInt16 mode, Err *err) SEC("aux");
Err DbClose(DmOpenRef dbRef) SEC("aux");
Err DbCreate(char *name, UInt32 type, UInt32 creator) SEC("aux");
Err DbResCreate(char *name, UInt32 type, UInt32 creator) SEC("aux");
Err DbGetTypeCreator(char *name, UInt32 *type, UInt32 *creator) SEC("aux");
Err DbGetAttributes(DmOpenRef dbRef, UInt16 *attr) SEC("aux");
Err DbSetAttributes(DmOpenRef dbRef, UInt16 attr) SEC("aux");

char *DbOpenRec(DmOpenRef dbRef, UInt16 index, Err *err) SEC("aux");
Err DbCloseRec(DmOpenRef dbRef, UInt16 index, char *rec) SEC("aux");
Err DbCreateRec(DmOpenRef dbRef, UInt16 *index, UInt16 size, UInt16 category) SEC("aux");
Err DbDeleteRec(DmOpenRef dbRef, UInt16 index) SEC("aux");
Err DbRemoveRec(DmOpenRef dbRef, UInt16 index) SEC("aux");
Err DbResizeRec(DmOpenRef dbRef, UInt16 index, UInt32 size) SEC("aux");
Err DbGetRecAttributes(DmOpenRef dbRef, UInt16 index, UInt16 *attr) SEC("aux");
Err DbSetRecAttributes(DmOpenRef dbRef, UInt16 index, UInt16 attr) SEC("aux");
Err DbGetRecID(DmOpenRef dbRef, UInt16 index, UInt32 *uid) SEC("aux");
UInt16 DbNumRecords(DmOpenRef dbRef) SEC("aux");

char *DbOpenAppInfo(DmOpenRef dbRef) SEC("aux");
void DbCloseAppInfo(char *info) SEC("aux");

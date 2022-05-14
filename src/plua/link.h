Int16 link(UInt32 creator, char *execName, char *version, char *objName, Boolean noTitle, Boolean full) SEC("aux");
Int16 linklib(UInt32 creator, char *execName, char *version, char *objName) SEC("aux");
Err addResource(DmOpenRef dbRef, UInt32 srcType, UInt32 srcId, UInt32 dstType, UInt32 dstId) SEC("aux");
Err addResourcePtr(DmOpenRef dbRef, MemPtr srcPtr, UInt32 size, UInt32 dstType, UInt32 dstId) SEC("aux");

#define copyResource(dbRef, srcType, srcId) addResource(dbRef, srcType, srcId, srcType, srcId)

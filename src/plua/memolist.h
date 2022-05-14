#define memoRecSize 4096

void MemoInitList(void) SEC("aux");
void MemoDestroyList(void) SEC("aux");
Int16 MemoGetList(ListPtr list, char *prefix, char *suffix) SEC("aux");
Int16 MemoRefreshList(ListPtr list, char *prefix, char *suffix) SEC("aux");
char *MemoGetName(UInt16 index) SEC("aux");
Int16 MemoGetIndex(char *name) SEC("aux");

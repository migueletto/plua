#define VfsPath "vfs0:/PALM/Programs/Plua/src"

void VfsInitList(void) SEC("aux");
void VfsDestroyList(void) SEC("aux");
Int16 VfsGetList(ListPtr list, char *suffix) SEC("aux");
char *VfsGetName(UInt16 index) SEC("aux");

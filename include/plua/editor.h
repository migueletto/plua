#define EDITOR_MEMO	1
#define EDITOR_DOC	2
#define EDITOR_STREAM	3
#define EDITOR_VFS	4

Err EditorLaunch(char *name);
void EditorSelect(Int16 type);
Int16 EditorSelected(void);
Boolean EditorAvailable(void);
void EditorEdit(void);

void SetSrcEdit(Boolean edit);
Boolean GetSrcEdit(void);

void SetSrcLine(UInt16 line);
UInt16 GetSrcLine(void);

#include "p.h"
#include "doc.h"
#include "doclist.h"
#include "file.h"

static FileType docList;

void DocInitList(void)
{
  MemSet(&docList, sizeof(FileType), 0);
}

void DocDestroyList(void)
{
  DestroyFileList(&docList);
}

Int16 DocGetList(ListPtr list, char *suffix)
{
  DestroyFileList(&docList);

  if (CreateFileList(DocAppID, DocDBType, &docList, suffix, 0) == -1)
    return -1;

  if (docList.fname && docList.n)
    LstSetListChoices(list, docList.fname, docList.n);
  else
    LstSetListChoices(list, NULL, 0);

  return docList.n;
}

char *DocGetName(UInt16 index)
{
  if (index >= docList.n)
    return NULL;

  return docList.rec[index].name;
}

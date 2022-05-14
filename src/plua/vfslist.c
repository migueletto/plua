#include "p.h"
#include "vfslist.h"
#include "file.h"

static FileType vfsList;

void VfsInitList(void)
{
  MemSet(&vfsList, sizeof(FileType), 0);
}

void VfsDestroyList(void)
{
  DestroyFileList(&vfsList);
}

Int16 VfsGetList(ListPtr list, char *suffix)
{
  DestroyFileList(&vfsList);

  if (CreateVfsFileList(VfsPath, &vfsList, suffix, false) == -1)
    return -1;

  if (vfsList.fname && vfsList.n)
    LstSetListChoices(list, vfsList.fname, vfsList.n);
  else
    LstSetListChoices(list, NULL, 0);

  return vfsList.n;
}

char *VfsGetName(UInt16 index)
{
  if (index >= vfsList.n)
    return NULL;

  return vfsList.rec[index].name;
}

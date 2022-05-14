#include "p.h"
#include "streamlist.h"
#include "file.h"

static FileType streamList;

void StreamInitList(void)
{
  MemSet(&streamList, sizeof(FileType), 0);
}

void StreamDestroyList(void)
{
  DestroyFileList(&streamList);
}

Int16 StreamGetList(ListPtr list, char *suffix)
{
  DestroyFileList(&streamList);

  if (CreateStreamFileList(&streamList, suffix) == -1)
    return -1;

  if (streamList.fname && streamList.n)
    LstSetListChoices(list, streamList.fname, streamList.n);
  else
    LstSetListChoices(list, NULL, 0);

  return streamList.n;
}

char *StreamGetName(UInt16 index)
{
  if (index >= streamList.n)
    return NULL;

  return streamList.rec[index].name;
}

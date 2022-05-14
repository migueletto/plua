#include "p.h"
#include "main.h"
#include "memolist.h"
#include "file.h"

static FileType memoList;

void MemoInitList(void)
{
  MemSet(&memoList, sizeof(FileType), 0);
}

void MemoDestroyList(void)
{
  DestroyFileList(&memoList);
}

Int16 MemoGetList(ListPtr list, char *prefix, char *suffix)
{
  DestroyFileList(&memoList);

  if (CreateMemoList(GetMemoName(), &memoList, prefix, suffix) == -1)
    return -1;

  if (memoList.fname && memoList.n)
    LstSetListChoices(list, memoList.fname, memoList.n);
  else
    LstSetListChoices(list, NULL, 0);

  return memoList.n;
}

char *MemoGetName(UInt16 index)
{
  if (index >= memoList.n)
    return NULL;

  return memoList.rec[index].name;
}

Int16 MemoGetIndex(char *name)
{
  Int16 i;

  for (i = 0; i < memoList.n; i++)
    if (!StrCompare(name, memoList.rec[i].name))
      return memoList.rec[i].index;

  return -1;
}

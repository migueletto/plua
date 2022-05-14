#include "p.h"
#include "error.h"
#include "gui.h"
#include "p.h"
#include "editor.h"

static char errorBuf[256];

void ErrorDialog(char *msg, Err err)
{
  ErrorDialogEx(msg, err, 0);
}

void ErrorDialogEx(char *msg, Err err, UInt16 num)
{
  char cod[64], s[8];

  if (err) {
    StrPrintF(cod, "\nError code: %d", err);
    s[0] = 0;
    if (num)
      StrPrintF(s, "(%x)", num);
    FrmCustomAlert(ErrorAlert, msg, cod, s);
  }
  else
    FrmCustomAlert(InfoAlert, msg, "", "");
}

void ErrorMsg(const char *fmt, ...)
{
  va_list argp;

  va_start(argp, fmt);
  vsprintf(errorBuf, fmt, argp);
  va_end(argp);

  if (GetSrcLine() > 0 && EditorAvailable() && errorBuf[0] != '[') {
    if (FrmCustomAlert(ErrorGotoAlert, errorBuf, "", "") == 1)
      SetSrcEdit(true);
    else {
      SetSrcEdit(false);
      SetSrcLine(0);
    }
  } else {
    FrmCustomAlert(ErrorAlert, errorBuf, "", "");
    SetSrcEdit(false);
    SetSrcLine(0);
  }
}

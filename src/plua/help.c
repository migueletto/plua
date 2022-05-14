#include "p.h"
#include "gui.h"
#include "main.h"
#include "app.h"
#include "db.h"
#include "scroll.h"
#include "events.h"
#include "help.h"

static Int16 searchf(void const *e1, void const *e2, Int32 len) SEC("aux");
static void fillhelp(FieldPtr fld, Int32 i) SEC("aux");

static char *helpWord;
static Int32 helpDef;
static UInt16 helpPreviousForm;

Err ShowHelp(char *word)
{
  DmOpenRef dbRef;
  MemHandle hi, hw;
  Boolean found;
  UInt16 *pi, aux[2], form;
  char *pw;
  Int32 pos;
  Err err;

  if ((dbRef = DbOpenByName(GetHelpFile(), dmModeReadOnly, &err)) == NULL)
    return -1;

  hi = DmGet1Resource(helpIndex, helpId);
  hw = DmGet1Resource(helpWords, helpId);
  DbClose(dbRef);

  if (!hi || !hw) {
    if (hi) DmReleaseResource(hi);
    if (hw) DmReleaseResource(hw);
    return -1;
  }

  pi = MemHandleLock(hi);
  pw = MemHandleLock(hw);

  if (!pi || !pw) {
    if (pi) MemPtrUnlock((MemPtr)pi);
    if (pw) MemPtrUnlock((MemPtr)pw);
    DmReleaseResource(hi);
    DmReleaseResource(hw);
    return -1;
  }

  helpDef = -1;

  if (word) {
    aux[0] = 0xFFFF;
    aux[1] = 0;
    helpWord = word;

    found = SysBinarySearch(pi, MemPtrSize(pi)/(2*sizeof(UInt16)),
                            2*sizeof(UInt16), (SearchFuncPtr)searchf,
                            aux, (Int32)pw, &pos, true);
    if (found)
      helpDef = pi[pos*2+1];
    else
      return -1;
  }
    
  MemPtrUnlock((MemPtr)pi);
  MemPtrUnlock((MemPtr)pw);
  DmReleaseResource(hi);
  DmReleaseResource(hw);

  form = FrmGetActiveFormID();
  if (form != HelpForm) {
    helpPreviousForm = form;
    FrmPopupForm(HelpForm);
  }

  return 0;
}

UInt16 HelpPreviousForm(void)
{
  return helpPreviousForm;
}

static Int16 searchf(void const *e1, void const *e2, Int32 other)
{
  UInt16 *pi1, *pi2;
  char *pw, *w1, *w2;

  pi1 = (UInt16 *)e1;
  pi2 = (UInt16 *)e2;
  pw = (char *)other;
  w1 = pi1[0] == 0xFFFF ? helpWord : &pw[pi1[0]];
  w2 = pi2[0] == 0xFFFF ? helpWord : &pw[pi2[0]];

  return StrCompare(w1, w2);
}

Boolean HelpFormHandleEvent(EventPtr event)
{
  FormPtr frm;
  FieldPtr fld;
  UInt16 index, start, end, len;
  char word[32], *text;
  Boolean handled;

  frm = FrmGetActiveForm();
  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      index = FrmGetObjectIndex(frm, helpFld);
      fld = (FieldPtr)FrmGetObjectPtr(frm, index);
  
      fillhelp(fld, helpDef);

      FrmDrawForm(frm);
      UpdateScrollBar(fld, frm, helpScl);
      break;

    case keyDownEvent:
      if (!(event->data.keyDown.modifiers & commandKeyMask))
        return true;

      if ((event->data.keyDown.chr == vchrPageUp ||
           event->data.keyDown.chr == vchrPageDown)) {

        index = FrmGetObjectIndex(frm, helpFld);
        fld = (FieldPtr)FrmGetObjectPtr(frm, index);
        len = FldGetVisibleLines(fld) - 1;
        if (event->data.keyDown.chr == vchrPageUp) len = -len;
        ScrollField(frm, helpFld, helpScl, len, true);
        handled = true;
      }
      break;

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
        case okBtn:
          FrmReturnToForm(helpPreviousForm);
          handled = true;
          break;
        case gotoBtn:
          index = FrmGetObjectIndex(frm, helpFld);
          fld = (FieldPtr)FrmGetObjectPtr(frm, index);

          FldGetSelection(fld, &start, &end);
          text = FldGetTextPtr(fld);

          if (text && start < end) {
            len = end - start;
            if (len >= sizeof(word))
              len = sizeof(word)-1;
            StrNCopy(word, &text[start], len);
            word[len] = 0;
            if (ShowHelp(word) == 0) {
              fillhelp(fld, helpDef);
              FldSetScrollPosition(fld, 0);
              FldDrawField(fld);
              FrmSetFocus(frm, FrmGetObjectIndex(frm, okBtn));
            }
          }
          handled = true;
          break;
        case indexBtn:
          index = FrmGetObjectIndex(frm, helpFld);
          fld = (FieldPtr)FrmGetObjectPtr(frm, index);

          if (ShowHelp(NULL) == 0) {
            fillhelp(fld, helpDef);
            FldSetScrollPosition(fld, 0);
            FldDrawField(fld);
            FrmSetFocus(frm, FrmGetObjectIndex(frm, okBtn));
          }
          handled = true;
      }
      break;

    case fldChangedEvent:
      UpdateScrollBar(event->data.fldChanged.pField, frm, helpScl);
      handled = true;
      break;

    case sclRepeatEvent:
      ScrollField(frm, helpFld, helpScl, 
                  event->data.sclRepeat.newValue -
                  event->data.sclRepeat.value, false);
      break;
  }

  return handled;
}

static void fillhelp(FieldPtr fld, Int32 i)
{
  DmOpenRef dbRef;
  MemHandle hd, hf, old;
  char *pd, *s;
  Int16 k, n;
  Err err;

  FldEraseField(fld);
  FldDelete(fld, 0, FldGetTextLength(fld));

  dbRef = DbOpenByName(GetHelpFile(), dmModeReadOnly, &err);
  hd = DmGet1Resource((i == -1) ? helpWords : helpDefs, helpId);
  DbClose(dbRef);

  pd = MemHandleLock(hd);

  if (i == -1) {
    old = FldGetTextHandle(fld);
    n = MemPtrSize(pd);
    hf = MemHandleNew(n);
    s = MemHandleLock(hf);
    MemMove(s, pd, n);
    for (k = 0; k < n-1; k++)
      if (s[k] == 0) s[k] = '\n';
    MemHandleUnlock(hf);
    FldSetTextHandle(fld, hf);
    if (old)
      MemHandleFree(old);
  } else {
    FldInsert(fld, &pd[i], StrLen(&pd[i]));
    FldReleaseFocus(fld);
  }

  MemPtrUnlock((MemPtr)pd);
  DmReleaseResource(hd);

  FldSetScrollPosition(fld, 0);
}

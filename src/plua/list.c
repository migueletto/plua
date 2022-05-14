#include "p.h"
#include "gui.h"
#include "main.h"
#include "app.h"
#include "doc.h"
#include "memolist.h"
#include "doclist.h"
#include "streamlist.h"
#include "vfslist.h"
#include "compile.h"
#include "editor.h"
#include "events.h"
#include "error.h"

static char srcBuffer[256];
static char tmpBuffer[256];
static char objBuffer[64];
static char exeBuffer[64];
static Int16 numItems;
static char *choiceNames[4];
static UInt16 lastChoice = MemoForm;

Boolean FileFormHandleEvent(EventPtr event)
{
  FormPtr frm;
  UInt16 formId, index = 0;
  ListPtr lst, choose;
  FieldType *fld;
  Boolean handled, readOnly;
  Int32 mindex;
  UInt16 id;
  Int16 len;
  char *s, *ext;
  FILE *f;
  AppPrefs *prefs;

  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      prefs = GetPrefs();
      frm = FrmGetActiveForm();
      formId = FrmGetActiveFormID();
      lastChoice = formId;
      choose = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,chooseList));
      choiceNames[0] = "Memo";
      choiceNames[1] = "Doc";
      choiceNames[2] = "Stream";
      choiceNames[3] = "Card";
      len = has_vfs() ? 4 : 3;
      LstSetHeight(choose, len);
      LstSetListChoices(choose, choiceNames, len);

      lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fileList));
      switch (formId) {
        case MemoForm:
          LstSetSelection(choose, 0);
          CtlSetLabel((ControlPtr)FrmGetObjectPtr(frm,
             FrmGetObjectIndex(frm, chooseCtl)),
                LstGetSelectionText(choose, 0));
          numItems = MemoGetList(lst, "-- ", GetExtention());
          index = prefs->memoIndex;
          EditorSelect(EDITOR_MEMO);
          if (GetSrcEdit()) EditorEdit();
          break;
        case DocForm:
          LstSetSelection(choose, 1);
          CtlSetLabel((ControlPtr)FrmGetObjectPtr(frm,
             FrmGetObjectIndex(frm, chooseCtl)),
                LstGetSelectionText(choose, 1));
          numItems = DocGetList(lst, GetExtention());
          index = prefs->docIndex;
          EditorSelect(EDITOR_DOC);
          if (GetSrcEdit()) EditorEdit();
          break;
        case StreamForm:
          LstSetSelection(choose, 2);
          CtlSetLabel((ControlPtr)FrmGetObjectPtr(frm,
             FrmGetObjectIndex(frm, chooseCtl)),
                LstGetSelectionText(choose, 2));
          numItems = StreamGetList(lst, GetExtention());
          index = prefs->streamIndex;
          EditorSelect(EDITOR_STREAM);
          if (GetSrcEdit()) EditorEdit();
          break;
        case VfsForm:
          LstSetSelection(choose, 3);
          CtlSetLabel((ControlPtr)FrmGetObjectPtr(frm,
             FrmGetObjectIndex(frm, chooseCtl)),
                LstGetSelectionText(choose, 3));
          numItems = VfsGetList(lst, GetExtention());
          index = prefs->vfsIndex;
          EditorSelect(EDITOR_VFS);
          if (GetSrcEdit()) EditorEdit();
      }
      if (numItems) {
        if (index >= numItems)
          index = numItems-1;
        LstSetSelection(lst, index);
        LstMakeItemVisible(lst, index);
      }
      FrmDrawForm(frm);
      SetSrcEdit(false);
      handled = true;
      break;

    case keyDownEvent:
      if (event->data.keyDown.modifiers & commandKeyMask) {
        frm = FrmGetActiveForm();
        lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fileList));
        len = LstGetVisibleItems(lst) - 1;
        switch (event->data.keyDown.chr) {
          case vchrPageUp:
            LstScrollList(lst, winUp, len);
            handled = true;
            break;
          case vchrPageDown:
            LstScrollList(lst, winDown, len);
            handled = true;
        }
      }
      break;

    case menuEvent:
      // transforma menuEvent em ctlSelectEvent
      event->data.ctlSelect.controlID = event->data.menu.itemID;

      // fall-through

    case ctlSelectEvent:
      switch (id = event->data.ctlSelect.controlID) {
        case newBtn:
          frm = FrmInitForm(InputForm);
          index = FrmGetObjectIndex(frm, inputFld);
          FrmSetTitle(frm, "New file name");
          FrmSetFocus(frm, index);
          MemSet(srcBuffer, sizeof(srcBuffer), 0);
          ext = GetExtention();

          if (FrmDoDialog(frm) == okBtn) {
            fld = (FieldPtr)FrmGetObjectPtr(frm, index);
            if ((s = FldGetTextPtr(fld)) != NULL && s[0]) {
              StrNCopy(srcBuffer, s, 128);
              s = StrStr(srcBuffer, ext);
              len = StrLen(ext);
              if (!s || s[len] || StrStr(srcBuffer, "/")) {
                ErrorMsg("Invalid file name");
                srcBuffer[0] = 0;
              }
            }
          }
          FrmDeleteForm(frm);

          if (srcBuffer[0]) {
            prefs = GetPrefs();
            readOnly = prefs->readOnly;
            prefs->readOnly = false;

            frm = FrmGetActiveForm();
            lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fileList));
            index = LstGetSelection(lst);

            switch (FrmGetActiveFormID()) {
              case MemoForm:
                if ((f = dbopen(GetMemoName(), "w")) != NULL) {
                  len = 3+StrLen(srcBuffer)+2; // "-- " + name + '\n' + '\0'
                  if ((mindex = dbcreaterec(f, len)) != -1)
                    if (dbopenrec(f, mindex) == len) {
                      dbwrite("-- ", 1, 3, f);
                      dbwrite(srcBuffer, 1, StrLen(srcBuffer), f);
                      dbwrite("\n", 1, 2, f);
                    }
                  dbclose(f);
                  numItems = MemoGetList(lst, "-- ", ext);
                }
                break;
              case DocForm:
                if ((f = fopen(srcBuffer, "r")) == NULL) {
                  if (DocCreate(srcBuffer) == 0)
                    numItems = DocGetList(lst, ext);
                } else {
                  fclose(f);
                  ErrorMsg("File exists");
                }
                break;
              case StreamForm:
                if ((f = fopen(srcBuffer, "r")) == NULL) {
                  if ((f = fopen(srcBuffer, "w")) != NULL) {
                    fclose(f);
                    numItems = StreamGetList(lst, ext);
                  }
                } else {
                  fclose(f);
                  ErrorMsg("File exists");
                }
                break;
              case VfsForm:
                StrCopy(tmpBuffer, VfsPath);
                StrCat(tmpBuffer, "/");
                StrCat(tmpBuffer, srcBuffer);
                if ((f = fopen(tmpBuffer, "r")) == NULL) {
                  if ((f = fopen(tmpBuffer, "w")) != NULL) {
                    fclose(f);
                    numItems = VfsGetList(lst, ext);
                  }
                } else {
                  fclose(f);
                  ErrorMsg("File exists");
                }
            }
            prefs->readOnly = readOnly;
            if (numItems) {
              if (index >= numItems)
                index = numItems-1;
              LstSetSelection(lst, index);
              LstMakeItemVisible(lst, index);
            }
            LstDrawList(lst);
          }
          break;

        case okBtn:
        case compileBtn:
        case editBtn:
        case deleteBtn:
          if (numItems > 0) {
            char *selectionName;
            prefs = GetPrefs();
            frm = FrmGetActiveForm();
            lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fileList));
            index = LstGetSelection(lst);
            formId = FrmGetActiveFormID();
            switch (formId) {
              case MemoForm:
                selectionName = LstGetSelectionText(lst, index);
                StrPrintF(objBuffer, "%s", selectionName);
                s = StrStr(objBuffer, GetExtention());
                if (s)
                  s[0] = '_';
                StrPrintF(exeBuffer, "%s", selectionName);
                s = StrStr(exeBuffer, GetExtention());
                if (s)
                  s[0] = '\0';
                StrPrintF(srcBuffer, "memo:/%s", selectionName);
                prefs->memoIndex = index;
                break;
              case DocForm:
                StrPrintF(objBuffer, "%s", DocGetName(index));
                s = StrStr(objBuffer, GetExtention());
                if (s)
                  s[0] = '_';
                StrPrintF(exeBuffer, "%s", DocGetName(index));
                s = StrStr(exeBuffer, GetExtention());
                if (s)
                  s[0] = '\0';
                StrPrintF(srcBuffer, "%s", DocGetName(index));
                prefs->docIndex = index;
                break;
              case StreamForm:
                StrPrintF(objBuffer, "%s", StreamGetName(index));
                s = StrStr(objBuffer, GetExtention());
                if (s)
                  s[0] = '_';
                StrPrintF(exeBuffer, "%s", StreamGetName(index));
                s = StrStr(exeBuffer, GetExtention());
                if (s)
                  s[0] = '\0';
                StrPrintF(srcBuffer, "%s", StreamGetName(index));
                prefs->streamIndex = index;
                break;
              case VfsForm:
                StrPrintF(objBuffer, "%s", VfsGetName(index));
                s = StrStr(objBuffer, GetExtention());
                if (s)
                  s[0] = '_';
                StrPrintF(exeBuffer, "%s", VfsGetName(index));
                s = StrStr(exeBuffer, GetExtention());
                if (s)
                  s[0] = '\0';
                StrPrintF(srcBuffer, "%s/%s", VfsPath, VfsGetName(index));
                prefs->vfsIndex = index;
            }
            switch (id) {
              case okBtn:
                SetLastForm(formId);
                SetCode(srcBuffer);
                FrmGotoForm(GraphicForm);
                break;
              case compileBtn:
                SetLastForm(formId);
                FrmPopupForm(CompileForm);
                break;
              case editBtn:
                EditorLaunch(srcBuffer);
                break;
              case deleteBtn:
                if (FrmCustomAlert(ConfirmAlert, "Delete file ?", "", "") == 0) {
                  readOnly = prefs->readOnly;
                  prefs->readOnly = false;
                  ext = GetExtention();
                  switch (formId) {
                    case MemoForm:
                      if ((f = dbopen(GetMemoName(), "w")) != NULL) {
                        if ((mindex = MemoGetIndex(&srcBuffer[6])) != -1)
                          dbdeleterec(f, mindex);
                        dbclose(f);
                      }
                      numItems = MemoGetList(lst, "-- ", ext);
                      break;
                    case DocForm:
                      remove(srcBuffer);
                      numItems = DocGetList(lst, ext);
                      break;
                    case StreamForm:
                      remove(srcBuffer);
                      numItems = StreamGetList(lst, ext);
                      break;
                    case VfsForm:
                      remove(srcBuffer);
                      numItems = VfsGetList(lst, ext);
                      break;
                  }
                  prefs->readOnly = readOnly;
                  if (numItems) {
                    if (index >= numItems)
                      index = numItems-1;
                    LstSetSelection(lst, index);
                    LstMakeItemVisible(lst, index);
                  }
                  LstDrawList(lst);
                }
            }
          }
          handled = true;
          break;

        case cancelBtn:
          FrmGotoForm(TextForm);
          handled = true;
      }
      break;

    case lstSelectEvent:
      switch (event->data.lstSelect.listID) {
        case fileList:
          prefs = GetPrefs();
          frm = FrmGetActiveForm();
          lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fileList));
          switch (FrmGetActiveFormID()) {
            case MemoForm:   prefs->memoIndex = LstGetSelection(lst); break;
            case DocForm:    prefs->docIndex = LstGetSelection(lst); break;
            case StreamForm: prefs->streamIndex = LstGetSelection(lst); break;
            case VfsForm:    prefs->vfsIndex = LstGetSelection(lst); break;
          }
          handled = false;
      }
      break;

    case popSelectEvent:
      switch (event->data.popSelect.listID) {
        case chooseList:
          formId = FrmGetActiveFormID();
          switch (event->data.popSelect.selection) {
            case 0: formId = MemoForm; break;
            case 1: formId = DocForm; break;
            case 2: formId = StreamForm; break;
            case 3: formId = VfsForm; break;
          }
          if (formId != FrmGetActiveFormID())
            FrmGotoForm(formId);
          handled = false;
      }
  }

  return handled;
}

UInt16 GetLastChoice(void)
{
  return lastChoice;
}

char *GetSrcBuf(void)
{
  return srcBuffer;
}

char *GetObjBuf(void)
{
  return objBuffer;
}

char *GetExeBuf(void)
{
  return exeBuffer;
}

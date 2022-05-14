#include "p.h"
#include "gui.h"
#include "main.h"
#include "app.h"
#include "screen.h"
#include "events.h"
#include "scroll.h"
#include "db.h"
#include "memolist.h"
#include "vfslist.h"
#include "compile.h"
#include "editor.h"
#include "help.h"
#include "text.h"
#include "hr.h"
#include "error.h"

#define TMPFILE "editor.tmp"
#define VFSTMPFILE VfsPath "/" TMPFILE

#define MAXCHARS 32767

static EventType startEvent;
static char savedText[1024];
static Int16 textIndex = -1;
static Boolean firstTextForm = true;
static DmOpenRef memo;

#ifdef DIALOG_MOVE
static Boolean dialog_event(EventPtr event);
#endif

Boolean TextFormHandleEvent(EventPtr event)
{
  FormPtr frm;
  FieldPtr editFld;
  Boolean handled;
  UInt16 id, sclid, fldid, index, len, start, end, i, line;
  UInt32 pos, size;
  MemHandle srcHandle;
  AppPrefs *p;
  char word[32], *name, *code, *title;
  RectangleType rect;
  FILE *f;
  Err err;

  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      frm = FrmGetActiveForm();
      switch (id = FrmGetActiveFormID()) {
        case RuntimeForm:
          FrmDrawForm(frm);
          break;
        case TextForm:
          index = FrmGetObjectIndex(frm, sourceFld);
          editFld = (FieldPtr)FrmGetObjectPtr(frm, index);
          FrmDrawForm(frm);
          FrmSetFocus(frm, index);
          FldGetBounds(editFld, &rect);
          clipform(rect.topLeft.y - 1);
          if (!firstTextForm)
            FldInsert(editFld, savedText, StrLen(savedText));
          firstTextForm = false;
          break;
        case EditForm:
          index = FrmGetObjectIndex(frm, sourceFld);
          editFld = (FieldPtr)FrmGetObjectPtr(frm, index);
          name = GetSrcBuf();
          title = name;
          pos = 0;
          memo = NULL;
          code = NULL;
          switch (EditorSelected()) {
            case EDITOR_MEMO:
              title = &name[6];
              if ((textIndex = MemoGetIndex(title)) != -1) {
                memo = DbOpenByName(GetMemoName(), dmModeReadWrite, &err);
                if (memo) {
                  srcHandle = DmGetRecord(memo, textIndex);
                  FldSetTextHandle(editFld, srcHandle);
                  FldSetMaxChars(editFld, memoRecSize-1);
                  code = FldGetTextPtr(editFld);
                }
              }
              break;

            case EDITOR_VFS:
              title = &name[StrLen(VfsPath)];
              // fall-through
            case EDITOR_STREAM:
              f = fopen(name, "r");
              fseek(f, 0, SEEK_END);
              size = ftell(f);
              fseek(f, 0, SEEK_SET);
              if ((code = malloc(size+1)) != NULL) {
                fread(code, 1, size, f);
                FldSetTextPtr(editFld, code);
                FldSetMaxChars(editFld, MAXCHARS);
              } else
                ErrorMsg("Not enough memory");
              fclose(f);
          }
          if (code) {
            size = StrLen(code);
            line = GetSrcLine();
            for (pos = 0, i = 1; pos < size && code[pos] && i < line; pos++) {
              if (code[pos] == '\n')
                i++;
            }
          }
          FldSetInsertionPoint(editFld, pos);
          FldSetScrollPosition(editFld, pos);
          UpdateScrollBar(editFld, frm, sourceScl);
          FrmSetTitle(frm, title);
          FrmDrawForm(frm);
          FrmSetFocus(frm, index);
          SetSrcLine(0);
          break;
        case GraphicForm:
        case FullGraphicForm:
          FrmDrawForm(frm);
          EvtCopyEvent(event, &startEvent);
          startEvent.eType = vmStartEvent;
          EvtAddEventToQueue(&startEvent);
      }
      handled = true;
      break;

    case penDownEvent:
    case penUpEvent:
    case penMoveEvent:
      switch (FrmGetActiveFormID()) {
        case RuntimeForm:
        case GraphicForm:
        case FullGraphicForm:
          if (validwindow())
            handled = gadget_event(event);
          break;
        case DialogForm:
          if (validwindow()) {
            handled = gadget_event(event);
#ifdef DIALOG_MOVE
            if (!handled)
              handled = dialog_event(event);
#endif
          }
      }
      break;

    case helpEvent:
    case keyDownEvent:
      if (event->data.keyDown.modifiers & commandKeyMask &&
          (event->data.keyDown.chr == vchrFind ||
           event->data.keyDown.chr == vchrPageUp ||
           event->data.keyDown.chr == vchrPageDown)) {

        frm = FrmGetActiveForm();
        index = FrmGetFocus(frm);

        if (index != noFocus)
          switch (FrmGetObjectType(frm, index)) {
            case frmFieldObj:
              editFld = (FieldPtr)FrmGetObjectPtr(frm, index);
              if (editFld) {
                if (event->data.keyDown.chr == findChr) {
                  id = FrmGetActiveFormID();
                  if (id == TextForm || id == EditForm) {
                    FldGetSelection(editFld, &start, &end);
                    code = FldGetTextPtr(editFld);
                    if (code && start < end) {
                      len = end - start;
                      if (len >= sizeof(word))
                        len = sizeof(word)-1;
                      StrNCopy(word, &code[start], len);
                      word[len] = 0;
                      ShowHelp(word);
                    } else
                      ShowHelp(NULL);
                  }
                } else {
                  len = FldGetVisibleLines(editFld) - 1;
                  id = FrmGetObjectId(frm, index);
                  if (id == sourceFld)
                    sclid = sourceScl;
                  else
                    sclid = getsclid(id);
                  if (event->data.keyDown.chr == vchrPageUp) len = -len;
                  ScrollField(frm, id, sclid, len, true);
                }
                handled = true;
              }
          }
      }
      break;

    case menuEvent:
      id = event->data.menu.itemID;

      frm = FrmGetActiveForm();
      index = FrmGetFocus(frm);
      if (index != noFocus && FrmGetObjectType(frm, index) == frmFieldObj) {
        editFld = (FieldPtr)FrmGetObjectPtr(frm, index);
        if (editFld && EditMenu(editFld, id)) {
          handled = true;
          break;
        }
      }

      switch (id) {
        case PrefsCmd:
          FrmPopupForm(PrefsForm);
          handled = true;
          break;
        case AboutCmd:
          frm = FrmInitForm(AboutForm);
          FrmDoDialog(frm);
          FrmDeleteForm(frm);
          handled = true;
      }
      break;

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
	case clearBtn:
          frm = FrmGetActiveForm();
          index = FrmGetObjectIndex(frm, sourceFld);
          editFld = (FieldPtr)FrmGetObjectPtr(frm, index);
          FldDelete(editFld, 0, FldGetTextLength(editFld));
          UpdateScrollBar(editFld, frm, sourceScl);
          p = GetPrefs();
          if (p->clearOutput) {
            FldGetBounds(editFld, &rect);
            clipform(rect.topLeft.y - 1);
          }
          handled = true;
          break;
	case runBtn:
          frm = FrmGetActiveForm();
          index = FrmGetObjectIndex(frm, sourceFld);
          editFld = (FieldPtr)FrmGetObjectPtr(frm, index);
          FldGetSelection(editFld, &start, &end);
          code = FldGetTextPtr(editFld);
          if (code) {
            if (start < end)
              len = end - start;
            else {
              start = 0;
              len = StrLen(code);
            }
            if (code[start] && len)
              AppRunBuffer(&code[start], len);
          }
          handled = true;
          break;
	case fileBtn:
          frm = FrmGetActiveForm();
          index = FrmGetObjectIndex(frm, sourceFld);
          editFld = (FieldPtr)FrmGetObjectPtr(frm, index);
          code = FldGetTextPtr(editFld);
          if (code && code[0])
            StrNCopy(savedText, code, StrLen(code));
          else
            savedText[0] = 0;

          FrmGotoForm(GetLastChoice());
          handled = true;
          break;
	case okBtn:
          switch (FrmGetActiveFormID()) {
            case EditForm:
              EditFinish();
              SetSrcEdit(false);
              switch (EditorSelected()) {
                case EDITOR_MEMO:
                  FrmGotoForm(MemoForm);
                  break;
                case EDITOR_STREAM:
                  FrmGotoForm(StreamForm);
                  break;
                case EDITOR_VFS:
                  FrmGotoForm(VfsForm);
              }
              handled = true;
          }
      }
      break;

    case fldChangedEvent:
      id = event->data.fldChanged.fieldID;

      if (id == sourceFld)
        sclid = sourceScl;
      else
        sclid = getsclid(id);

      UpdateScrollBar(event->data.fldChanged.pField, FrmGetActiveForm(), sclid);
      handled = true;
      break;

    case sclRepeatEvent:
      id = event->data.sclRepeat.scrollBarID;

      if (id == sourceScl)
        fldid = sourceFld;
      else
        fldid = getfldid(id);

      ScrollField(FrmGetActiveForm(), fldid, id,
                  event->data.sclRepeat.newValue -
                  event->data.sclRepeat.value, false);
  }

  return handled;
}

void EditFinish(void)
{
  FormPtr frm;
  FieldPtr editFld;
  UInt16 sel, index;
  UInt32 size, r;
  char *name, *code;
  char *tmpfile;
  FILE *f;
  AppPrefs *prefs;
  Boolean readOnly;
  Err err;

  frm = FrmGetActiveForm();
  index = FrmGetObjectIndex(frm, sourceFld);
  editFld = (FieldPtr)FrmGetObjectPtr(frm, index);

  switch (sel = EditorSelected()) {
    case EDITOR_MEMO:
      if (memo) {
        FldCompactText(editFld);
        DmReleaseRecord(memo, textIndex, true);
        DbClose(memo);
        memo = NULL;
        FldSetTextHandle(editFld, NULL);
      }
      break;
    case EDITOR_STREAM:
    case EDITOR_VFS:
      err = 0;
      if ((code = FldGetTextPtr(editFld)) != NULL) {
        size = StrLen(code);
        prefs = GetPrefs();
        readOnly = prefs->readOnly;
        prefs->readOnly = false;
        tmpfile = (sel == EDITOR_STREAM) ? TMPFILE : VFSTMPFILE;
        if ((f = fopen(tmpfile, "w")) != NULL) {
          r = size ? fwrite(code, 1, size, f) : 0;
          fclose(f);
          if (r == size) {
            name = GetSrcBuf();
            remove(name);
            rename(tmpfile, name);
          } else
            err = -3;
        } else
          err = -2;

        prefs->readOnly = readOnly;
        FldSetTextPtr(editFld, NULL);
        free(code);
      } else
        err = -1;

      if (err)
        ErrorMsg("The file could not be saved (%d)", err);
  }
}

Boolean EditMenu(FieldPtr fld, UInt16 id)
{
  Boolean handled = true;

  switch (id) {
    case UndoCmd:
      FldUndo(fld);
      break;
    case CutCmd:
      FldCut(fld);
      break;
    case CopyCmd:
      FldCopy(fld);
      break;
    case PasteCmd:
      FldPaste(fld);
      break;
    case SelectCmd:
      FldSetSelection(fld, 0, FldGetTextLength(fld));
      break;
    case KeyboardCmd:
      SysKeyboardDialog(kbdDefault);
      break;
    case GraffitiCmd:
      SysGraffitiReferenceDialog(referenceDefault);
      break;
    default:
      handled = false;
  }

  return handled;
}

#ifdef DIALOG_MOVE
static Boolean dialog_event(EventPtr event)
{
  Int16 romNumber, x, y, dx, dy;
  FormPtr frm;
  WinHandle wh, old;
  Boolean handled = false;
  static Int16 xmax = 0, ymax = 0;
  static DisplayType *disp = NULL;
  static Boolean dialogMoving = false;
  static Int16 dragX = 0, dragY = 0;
  static RectangleType dialogPos;

  romNumber = GetRomVersionNumber();
  if (romNumber >= 60)
    return handled;

  switch (event->eType) {
    case penDownEvent:
      if (!dialogMoving && getdisplay()->movable) {
        x = event->screenX;
        y = event->screenY;
        WinWindowToDisplayPt(&x, &y);

        frm = FrmGetActiveForm();
        wh = FrmGetWindowHandle(frm);
        if (romNumber < 40) {
          old = WinSetDrawWindow(wh);
          WinGetDrawWindowBounds(&dialogPos);
          WinSetDrawWindow(old);
        } else
          WinGetBounds(wh, &dialogPos);

        if (x >= dialogPos.topLeft.x &&
            x < (dialogPos.topLeft.x + dialogPos.extent.x) &&
            y >= dialogPos.topLeft.y &&
            y < (dialogPos.topLeft.y + 10)) {
          dialogMoving = true;
          dragX = x;
          dragY = y;
          FrmEraseForm(frm);
          WinInvertRectangleFrame(dialogFrame, &dialogPos);
          disp = getdisplay();
          xmax = disp->width/disp->factorX - dialogPos.extent.x - 2;
          ymax = disp->height/disp->factorY - dialogPos.extent.y -2;
          handled = true;
        }
      }
      break;
    case penMoveEvent:
      if (dialogMoving) {
        x = event->screenX;
        y = event->screenY;
        WinWindowToDisplayPt(&x, &y);
        dx = x - dragX;
        dy = y - dragY;

        WinInvertRectangleFrame(dialogFrame, &dialogPos);

        dialogPos.topLeft.x += dx;
        dialogPos.topLeft.y += dy;

        if (dialogPos.topLeft.x < 2)
          dialogPos.topLeft.x = 2;
        else if (dialogPos.topLeft.x >= xmax)
          dialogPos.topLeft.x = xmax;

        if (dialogPos.topLeft.y < 2)
          dialogPos.topLeft.y = 2;
        else if (dialogPos.topLeft.y >= ymax)
          dialogPos.topLeft.y = ymax;

        WinInvertRectangleFrame(dialogFrame, &dialogPos);

        dragX = x;
        dragY = y;
        handled = true;
      }
      break;
    case penUpEvent:
      if (dialogMoving) {
        WinInvertRectangleFrame(dialogFrame, &dialogPos);
        frm = FrmGetActiveForm();
        wh = FrmGetWindowHandle(frm);
        WinSetBounds(wh, &dialogPos);
        FrmDrawForm(frm);
        dialogMoving = false;
        handled = true;
      }
  }

  return handled;
}
#endif

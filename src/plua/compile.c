#include "p.h"
#include "gui.h"
#include "main.h"
#include "app.h"
#include "link.h"
#include "compile.h"
#include "editor.h"
#include "db.h"
#include "text.h"
#include "events.h"
#include "error.h"

Boolean CompileFormHandleEvent(EventPtr event)
{
  FormPtr frm;
  ControlType *ctl;
  FieldPtr fld;
  UInt16 index, id, appType;
  UInt32 type, creator;
  DmOpenRef dbRef;
  MemHandle h;
  MemPtr p;
  char *s, *name, version[8], *screator, buf[8];
  AppPrefs *prefs;
  Boolean readOnly, noTitle, handled, failed;
  Err err;

  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      frm = FrmGetActiveForm();

      name = GetExeBuf();
      StrCopy(version, "1.0");

      if (DbGetTypeCreator(name, &type, &creator) == 0) {
        fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,creatorFld));
        CreatorToString(creator, buf);
        FldInsert(fld, buf, StrLen(buf));

        appType = type == GetLibType() ? libSel : appSel;

        if ((dbRef = DbOpenByName(name, dmModeReadOnly, &err)) != NULL) {
          if ((h = DmGet1Resource(verRsc, appVersionID)) != NULL) {
            if ((p = MemHandleLock(h)) != NULL) {
              StrNCopy(version, (char *)p, sizeof(version)-1);
              MemHandleUnlock(h);
            }
            DmReleaseResource(h);
          }

          if ((h = DmGet1Resource(scrType, scrID)) != NULL) {
            DmReleaseResource(h);
            FrmSetControlValue(frm, FrmGetObjectIndex(frm, noTitleCtl), 1);
          }

          DbClose(dbRef);
        }
        FrmSetFocus(frm, FrmGetObjectIndex(frm, versionFld));
      } else {
        FrmSetFocus(frm, FrmGetObjectIndex(frm, creatorFld));
        appType = appSel;
      }

      ctl = (ControlType *)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,appType));
      CtlSetValue(ctl, 1);
      fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, nameFld));
      FldInsert(fld, name, StrLen(name));
      fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, versionFld));
      FldInsert(fld, version, StrLen(version));
      FrmHideObject(frm, FrmGetObjectIndex(frm, waitLbl));
      FrmDrawForm(frm);
      handled = true;
      break;

    case menuEvent:
      id = event->data.menu.itemID;

      frm = FrmGetActiveForm();
      index = FrmGetFocus(frm);
      if (index != noFocus && FrmGetObjectType(frm, index) == frmFieldObj) {
        fld = (FieldPtr)FrmGetObjectPtr(frm, index);
        if (fld && EditMenu(fld, id)) {
          handled = true;
          break;
        }
      }

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
        case okBtn:
          frm = FrmGetActiveForm();
          fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,
                              creatorFld));
          screator = FldGetTextPtr(fld);
          if (!screator || StrLen(screator) != 4) {
            ErrorDialog("Creator ID must be exactly 4 characters.", 0);
            break;
          }
          creator = StringToCreator(screator);
          fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, nameFld));
          name = FldGetTextPtr(fld);
          fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,
                              versionFld));
          s = FldGetTextPtr(fld);

          if (!name || !s || !StrLen(name) || !StrLen(s)) {
            ErrorDialog("Both name and version must be filled.", 0);
            break;
          }
          StrNCopy(version, s, sizeof(version)-1);

          noTitle = FrmGetControlValue(frm,
             FrmGetObjectIndex(frm, noTitleCtl)) == 1 ? true : false;

          prefs = GetPrefs();
          readOnly = prefs->readOnly;
          prefs->readOnly = false;

          FrmShowObject(frm, FrmGetObjectIndex(frm, waitLbl));

          failed = AppCompile(GetSrcBuf(), GetObjBuf()) == -1;

          if (!failed) {
            ctl = (ControlType *)FrmGetObjectPtr(frm,
                      FrmGetObjectIndex(frm, appSel));
            failed = CtlGetValue(ctl) ?
               link(creator, name, version, GetObjBuf(), noTitle, false) == -1 :
               linklib(creator, name, version, GetObjBuf()) == -1;
          }

          FileDelete(0, GetObjBuf());   
          prefs->readOnly = readOnly;

          // fall-through

        case cancelBtn:
          FrmReturnToForm(GetLastForm());

          if (GetSrcEdit()) {
            switch (GetLastForm()) {
              case MemoForm:
              case DocForm:
                EditorEdit();
            }
          }
          handled = true;
      }
  }

  return handled;
}

#include "p.h"
#include "gui.h"
#include "app.h"
#include "events.h"

Boolean PrefsFormHandleEvent(EventPtr event)
{
  FormPtr frm;
  Boolean handled;
  AppPrefs *prefs;

  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      prefs = GetPrefs();
      frm = FrmGetActiveForm();
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, readOnlyCtl),
          prefs->readOnly);
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, clearOutputCtl),
          prefs->clearOutput);
      FrmDrawForm(frm);
      handled = true;
      break;

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
        case okBtn:
          FrmReturnToForm(TextForm);
          handled = true;
          break;

        case readOnlyCtl:
          prefs = GetPrefs();
          prefs->readOnly = !prefs->readOnly;
          handled = true;
          break;

        case clearOutputCtl:
          prefs = GetPrefs();
          prefs->clearOutput = !prefs->clearOutput;
          handled = true;
      }
  }

  return handled;
}

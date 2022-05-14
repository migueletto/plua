#include "p.h"
#include "gui.h"
#include "main.h"
#include "app.h"
#include "doc.h"
#include "editor.h"
#include "compile.h"
#include "vfslist.h"
#include "events.h"
#include "error.h"

#define SRCEDIT_CREATOR	'LedX'

/*
#define BLOCK_SIZE 1024
#define MAX_BLOCKS 32

typedef struct {
  char *block;
  UInt16 len, size;
} EditorBlock;

typedef struct {
  FormPtr frm;
  UInt16 id, index;
  UInt16 x, y, width, height;
  RectangleType rect;
  UInt16 oldFont, fw, fh;
  UInt16 rows, cols, row, col, xcol, yrow, line, nlines, nblines;
  AppPrefs *prefs;
  char *filename;
  EditorBlock *list;
  UInt16 nblocks, iblock, pos, iblock0, pos0;
} EditorGadget;
*/

static Boolean srcEdit = false;
static UInt16 srcLine = 0;
static Int16 editorType = 0;
//static EditorGadget gadget;

/*
static Boolean GadgetCallback(FormGadgetTypeInCallback *gad, UInt16 cmd, void *param);
void EditorFinish(void);
static void setline(UInt16 line);
static void scroll(Int32 lines);
static void restorepos(void);
static void savepos(void);
static char nextchar(void);
static char prevchar(void);
static char validchar(char c);
static void eraseeol(Int16 col, Int16 row);
static void eraseeos(Int16 row);
static void updatescroll(void);
*/

void EditorSelect(Int16 type)
{
  editorType = type;
}

Int16 EditorSelected(void)
{
  return editorType;
}

Boolean EditorAvailable(void)
{
  UInt16 cardNo;
  LocalID dbID;
  DmSearchStateType stateInfo;

  switch (editorType) {
    case EDITOR_MEMO:
      return true;
    case EDITOR_DOC:
      if (DmGetNextDatabaseByTypeCreator(true, &stateInfo,
            sysFileTApplication, SRCEDIT_CREATOR, true, &cardNo, &dbID) == 0)
        return true;
  }

  return false;
}

void EditorEdit(void)
{
  EventType event;

  MemSet(&event, sizeof(EventType), 0);
  event.eType = ctlSelectEvent;
  event.data.ctlSelect.controlID = editBtn;
  EvtAddUniqueEventToQueue(&event, 0, true);
}

Err EditorLaunch(char *name)
{
  UInt16 cardNo;
  LocalID dbID;
  UInt16 attr, i;
  UInt32 type, creator, pos;
  DmSearchStateType stateInfo;
  GoToParamsType *p;
  FILE *f;
  UInt8 b;
  Err err;

  switch (editorType) {
    case EDITOR_MEMO:
      FrmGotoForm(EditForm); 
      return 0;
    case EDITOR_DOC:
      if (EditorAvailable())
        break;
      // fall-through
    default:
      ErrorMsg("There is no editor for this file type");
      return -1;
  }

  cardNo = 0;

  if ((dbID = DmFindDatabase(cardNo, name)) == 0) {
    SetSrcLine(0);
    ErrorMsg("File %s was not found", name);
    return -1;
  }

  if ((err = DmDatabaseInfo(cardNo, dbID, name, &attr,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, &type, &creator)) != 0) {
    SetSrcLine(0);
    ErrorMsg("File info error: %d", err);
    return -1;
  }

  if (type == DocDBType && creator == DocAppID) {

    if ((p = malloc(sizeof(GoToParamsType))) == NULL) {
      SetSrcLine(0);
      return -1;
    }

    MemSet(p, sizeof(GoToParamsType), 0);
    p->dbCardNo = cardNo;
    p->dbID = dbID;

    if (DmGetNextDatabaseByTypeCreator(true, &stateInfo,
           sysFileTApplication, SRCEDIT_CREATOR, true, &cardNo, &dbID) != 0) {
      free(p);
      SetSrcLine(0);
      ErrorMsg("Editor was not found");
      return -1;
    }

    pos = 0;

    if (srcLine > 1 && (f = fopen(name, "r")) != NULL) {
      for (i = 1; i < srcLine; pos++) {
        if (fread(&b, 1, 1, f) != 1)
          break;
        if (b == '\n')
          i++;
      }
      fclose(f);
    }

    p->matchPos = pos;
    p->searchStrLen = 0;
    MemPtrSetOwner(p, 0);

    if ((err = SysUIAppSwitch(cardNo, dbID, sysAppLaunchCmdGoTo, p)) != 0) {
      free(p);
      ErrorMsg("Error launching editor: %d", err);
    }
  }

  SetSrcLine(0);
  return -1;
}

void SetSrcEdit(Boolean edit)
{
  srcEdit = edit;
}

Boolean GetSrcEdit(void)
{
  return srcEdit;
}

void SetSrcLine(UInt16 line)
{
  srcLine = line;
}

UInt16 GetSrcLine(void)
{
  return srcLine;
}

/*
Boolean EditFormHandleEvent(EventPtr event)
{
  char *title;
  UInt16 i, j, len;
  Int32 lines;
  char c;
  FILE *f;
  Boolean handled;

  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      MemSet(&gadget, sizeof(gadget), 0);

      gadget.frm = FrmGetActiveForm();
      gadget.id = sourceFld;
      gadget.index = FrmGetObjectIndex(gadget.frm, gadget.id);
      gadget.prefs = GetPrefs();

      FrmGetObjectBounds(gadget.frm, gadget.index, &gadget.rect);
      gadget.x = gadget.rect.topLeft.x;
      gadget.y = gadget.rect.topLeft.y;
      gadget.width = gadget.rect.extent.x;
      gadget.height = gadget.rect.extent.y;

      gadget.oldFont = FntSetFont(255);
      gadget.fw = FntCharWidth('a');
      gadget.fh = FntCharHeight();
      FntSetFont(gadget.oldFont);
      gadget.cols = gadget.width / gadget.fw;
      gadget.rows = gadget.height / gadget.fh;
      gadget.line = 0;
      gadget.nlines = 0;
      gadget.nblines = 0;

      gadget.filename = GetSrcBuf();
      switch (editorType) {
        case EDITOR_MEMO:
          title = &gadget.filename[6];
          break;
        case EDITOR_VFS:
          title = &gadget.filename[StrLen(VfsPath)];
          break;
        default:
          title = gadget.filename;
      }

      if ((gadget.list = malloc(MAX_BLOCKS * sizeof(EditorBlock))) != NULL) {
        if ((f = fopen(gadget.filename, "r")) != NULL) {
          c = 0;
          for (i = 0; i < MAX_BLOCKS;) {
            if ((gadget.list[i].block = malloc(BLOCK_SIZE * 2)) != NULL) {
              gadget.list[i].size = BLOCK_SIZE * 2;
              len = fread(gadget.list[i].block+BLOCK_SIZE, 1, BLOCK_SIZE, f);
              gadget.list[i].len = 0;
              for (j = 0; j < len; j++) {
                if ((c = validchar(gadget.list[i].block[BLOCK_SIZE+j])) != 0) {
                  gadget.list[i].block[gadget.list[i].len++] = c;
                  if (c == '\n')
                    gadget.nlines++;
                }
              }
              if (gadget.list[i].len)
                i++;
              else {
                free(gadget.list[i].block);
                gadget.list[i].block = NULL;
              }
              if (feof(f))
                break;
            } else
              break;
          }
          fclose(f);
          gadget.nblocks = i;
          if (c)
            gadget.nlines++;
        }
        setline(GetSrcLine());
      }
      SetSrcLine(0);

      updatescroll();
      FrmSetGadgetHandler(gadget.frm, gadget.index, GadgetCallback);
      FrmSetTitle(gadget.frm, title);
      FrmDrawForm(gadget.frm);
      break;

    case penDownEvent:
    case penUpEvent:
    case penMoveEvent:
      if (RctPtInRectangle(event->screenX, event->screenY, &gadget.rect)) {
        event->screenX -= gadget.x;
        event->screenY -= gadget.y;
        GadgetCallback(NULL, formGadgetHandleEventCmd, event);
        handled = true;
      }
      break;

    case keyDownEvent:
      if (!(event->data.keyDown.modifiers & commandKeyMask)) {
        GadgetCallback(NULL, formGadgetHandleEventCmd, event);
        handled = true;
      } else {
        lines = 0;
        switch (event->data.keyDown.chr) {
          case vchrPageUp:
            lines = -(gadget.rows-1);
            break;
          case vchrPageDown:
            lines = gadget.rows-1;
        }
        if (lines) {
          scroll(lines);
          GadgetCallback(NULL, formGadgetDrawCmd, NULL);
        }
      }
      break;

    case helpEvent:
    case menuEvent:
      break;

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
        case okBtn:
          EditorFinish();
          SetSrcEdit(false);
          switch (editorType) {
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
      break;

    case sclRepeatEvent:
      scroll(event->data.sclRepeat.newValue - event->data.sclRepeat.value);
      GadgetCallback(NULL, formGadgetDrawCmd, NULL);
  }

  return handled;
}

static void setline(UInt16 line)
{
  Int16 i;
  char c;

  gadget.iblock = 0;
  gadget.pos = 0;
  savepos();

  if (line > 1) {
    for (i = 0, c = 0; i < line-1;) {
      c = nextchar();
      if (c == 0)
        break;
      if (c == '\n')
        i++;
    }
    if (c == '\n') {
      nextchar();
      gadget.line = line;
    } else {
      restorepos();
      gadget.line = 0;
    }
  }
}

static void scroll(Int32 lines)
{
  Int16 n;
  char c;

  //  N=LF, P=pos
  //  aaaN
  //  bbbN
  // PcccN
  //  dddN
  //  eeeN

  if (lines < 0) {
    for (n = -lines+1, c = 0; n > 0;) {
      c = prevchar();
      if (c == 0)
        break;
      if (c == '\n') {
        n--;
        gadget.line--;
      }
    }
    if (c == '\n') {
      nextchar();
      nextchar();
    }

  } else if (lines > 0) {
    for (n = lines, c = 0; n > 0;) {
      c = nextchar();
      if (c == 0)
        break;
      if (c == '\n') {
        n--;
        gadget.line++;
      }
    }
  }
}

static void savepos(void)
{
  gadget.iblock0 = gadget.iblock;
  gadget.pos0 = gadget.pos;
}

static void restorepos(void)
{
  gadget.iblock = gadget.iblock0;
  gadget.pos = gadget.pos0;
}

static char nextchar(void)
{
  if (gadget.iblock >= gadget.nblocks)
    return 0;
  if (gadget.pos >= gadget.list[gadget.iblock].len) {
    gadget.iblock++;
    gadget.pos = 0;
    if (gadget.iblock >= gadget.nblocks)
      return 0;
  }
  return gadget.list[gadget.iblock].block[gadget.pos++];
}

static char prevchar(void)
{
  if (gadget.nblocks == 0)
    return 0;
  if (gadget.iblock == 0) {
    if (gadget.pos == 0)
      return 0;
  } else {
    if (gadget.pos == 0) {
      gadget.iblock--;
      gadget.pos = gadget.list[gadget.iblock].len;
    }
  }
  return gadget.list[gadget.iblock].block[gadget.pos--];
}

static char validchar(char c)
{
  if (c < 32) {
    switch (c) {
      case '\n':
        return c;
    }
    return 0;
  }
  return c;
}

static void eraseeol(Int16 col, Int16 row)
{
  RectangleType rect;

  if (col < gadget.cols) {
    RctSetRectangle(&rect, gadget.x+col*gadget.fw, gadget.y+row*gadget.fh, (gadget.cols-col)*gadget.fw, gadget.fh);
    WinEraseRectangle(&rect, 0);
  }
}

static void eraseeos(Int16 row)
{
  RectangleType rect;

  if (row < gadget.rows) {
    RctSetRectangle(&rect, gadget.x, gadget.y+row*gadget.fh, gadget.cols*gadget.fw, gadget.fh);
    WinEraseRectangle(&rect, 0);
  }
}

static Boolean GadgetCallback(FormGadgetTypeInCallback *p, UInt16 cmd, void *param)
{
  EventType *event;
  UInt16 row, col, x, y;
  char c;

  if (cmd == formGadgetDeleteCmd)
    return true;

  switch (cmd) {
    case formGadgetDrawCmd:
      gadget.oldFont = FntSetFont(255);
      savepos();
      for (row = 0, col = 0, x = 0, y = 0; row < gadget.rows;) {
        if ((c = nextchar()) == 0)
          break;
        if (c == '\n') {
          eraseeol(col, row);
          row++;
          col = 0;
          y += gadget.fh;
          x = 0;
        } else {
          if (col < gadget.cols)
            WinPaintChar(c, gadget.x+x, gadget.y+y);
          col++;
          x += gadget.fw;
        }
      }
      eraseeol(col, row);
      if (row < gadget.rows) {
        row++;
        gadget.nblines = gadget.rows - row;
        eraseeos(row);
      }
      restorepos();
      FntSetFont(gadget.oldFont);
      break;
    case formGadgetEraseCmd:
      WinEraseRectangle(&gadget.rect, 0);
      break;
    case formGadgetHandleEventCmd:
      event = (EventType *)param; 
      switch (event->eType) {
        case frmGadgetEnterEvent:
          break;
        case penDownEvent:
          gadget.col = event->screenX / gadget.fw;
          gadget.row = event->screenY / gadget.fh;
          gadget.xcol = gadget.col * gadget.fw;
          gadget.yrow = gadget.row * gadget.fh;
          break;
        case penUpEvent:
          break;
        case penMoveEvent:
          break;
        case keyDownEvent:
          c = validchar(event->data.keyDown.chr);
          if (c) {
            gadget.oldFont = FntSetFont(255);
            WinPaintChar(c, gadget.x+gadget.xcol, gadget.y+gadget.yrow);
            FntSetFont(gadget.oldFont);
            gadget.xcol += gadget.fw;
          }
      }
  }

  return true;
}

static void updatescroll(void)
{
  UInt16 maxValue, index;
  IndexedColorType bg;
  ScrollBarPtr bar;

  index = FrmGetObjectIndex(gadget.frm, sourceScl);
  bar = (ScrollBarPtr)FrmGetObjectPtr(gadget.frm, index);

  if (gadget.nlines > gadget.rows)
    maxValue = (gadget.nlines - gadget.rows) + gadget.nblines;
  else
    maxValue = gadget.line;

  bg = WinSetBackColor(0);
  SclSetScrollBar(bar, gadget.line, 0, maxValue, gadget.rows-1);
  WinSetBackColor(bg);
}

void EditorFinish(void)
{
  UInt16 i;

  if (gadget.list) {
    for (i = 0; i < MAX_BLOCKS; i++) {
      if (gadget.list[i].block)
        free(gadget.list[i].block);
    }
    free(gadget.list);
  }
  MemSet(&gadget, sizeof(gadget), 0);
}
*/

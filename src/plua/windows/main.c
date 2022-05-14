#include <windows.h>
#include <commctrl.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <p.h>
#include <plualibl.h>
#include <lgraphlib.h>
#include <plua_windows.h>
#include "main.h"

LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);
int PluaOpen(char *filename);
int PluaClose(void);

char szClassName[ ] = "PluaRT";
lua_State *L = NULL;
int finish = 0;
HINSTANCE hinst;
HWND hwnd;
static EventType event;
static int eventAvailable = 0;

int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nWinMode)
{
  MSG messages;
  WNDCLASS wincl;
  int dx, dy;

  hinst = hThisInstance;

  wincl.style = 0;
  wincl.lpfnWndProc = WindowProcedure;     
  wincl.cbClsExtra = 0;
  wincl.cbWndExtra = 0;
  wincl.hInstance = hThisInstance;
  wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
  wincl.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
  wincl.lpszMenuName = NULL;
  wincl.lpszClassName = szClassName;

  if (!RegisterClass(&wincl))
    return 0;

  dx = 320 + 2*GetSystemMetrics(SM_CXFIXEDFRAME) +
             2*GetSystemMetrics(SM_CXBORDER);
  dy = 320 + 2*GetSystemMetrics(SM_CYFIXEDFRAME) +
             2*GetSystemMetrics(SM_CYBORDER) +
               GetSystemMetrics(SM_CYCAPTION);

  hwnd = CreateWindow (
         szClassName,         // Classname
         szClassName,         // Title Text
         WS_OVERLAPPEDWINDOW, // default window
         CW_USEDEFAULT,       // Windows decides the position
         CW_USEDEFAULT,       // where the window ends up on the screen
         dx,                  // The programs width
         dy,                  // and height in pixels
         HWND_DESKTOP,        // The window is a child-window to desktop
         NULL,                // No menu
         hThisInstance,       // Program Instance handler
         NULL                 // No Window Creation data
         );

  InitCommonControls();

  ShowWindow(hwnd, nWinMode);
  UpdateWindow(hwnd);

  PluaOpen(lpszArgument);
      
  while (GetMessage(&messages, NULL, 0, 0) && !finish) {
    TranslateMessage(&messages);
    DispatchMessage(&messages);
  }

  PluaClose();

  return 0;
}

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message,
                                  WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hdc;
  void plua_paint(HDC);
    
  switch (message) {
    case WM_PAINT:
      hdc = BeginPaint(hwnd, &ps);
      plua_paint(hdc);
      EndPaint(hwnd, &ps);
      break;
    case WM_CHAR:
      event.eType = keyDownEvent;
      event.data.keyDown.chr = wParam;
      eventAvailable = 1;
      break;
    case WM_LBUTTONDOWN:
      event.eType = penDownEvent;
      event.screenX = LOWORD(lParam);
      event.screenY = HIWORD(lParam);
      eventAvailable = 1;
      break;
    case WM_LBUTTONUP:
      event.eType = penUpEvent;
      event.screenX = LOWORD(lParam);
      event.screenY = HIWORD(lParam);
      eventAvailable = 1;
      break;
    case WM_MOUSEMOVE:
      if (wParam & MK_LBUTTON) {
        event.eType = penMoveEvent;
        event.screenX = LOWORD(lParam);
        event.screenY = HIWORD(lParam);
        eventAvailable = 1;
      }
      break;
    case WM_COMMAND:
      event.eType = ctlSelectEvent;
      event.data.ctlSelect.controlID = LOWORD(wParam);
      event.data.ctlSelect.on = 1;
      event.data.ctlSelect.value = 0;
      eventAvailable = 1;
      break;

      // wNotifyCode = HIWORD(wParam); // notification code 
      // hwndCtl = (HWND)lParam;      // handle of control 

    case WM_HSCROLL:	// slider
      switch (LOWORD(wParam)) {
        case TB_TOP:
        case TB_BOTTOM:
        case TB_LINEDOWN:
        case TB_THUMBPOSITION:
        case TB_THUMBTRACK:
        case TB_PAGEUP:
        case TB_PAGEDOWN:
          event.eType = ctlSelectEvent;
          event.data.ctlSelect.controlID = LOWORD(wParam);
          event.data.ctlSelect.on = 1;
          event.data.ctlSelect.value = 0; // preenchido no g_event
          eventAvailable = 1;
      }
      break;
    case WM_CLOSE:
      DestroyWindow(hwnd);
      break;
    case WM_DESTROY:
      finish = 1;
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hwnd, message, wParam, lParam);
  }

  return 0;
}

Boolean ProcessEvent(EventType *evt, Int32 wait, Boolean app)
{
  MSG messages;

  if (finish) {
    evt->eType = appStopEvent;
    return 1;
  }

  if (!PeekMessage(&messages, NULL, 0, 0, PM_NOREMOVE)) {
    Sleep(wait);
    evt->eType = nilEvent;
    return 1;
  }

  if (!GetMessage(&messages, NULL, 0, 0))
    return 0;

  eventAvailable = 0;
  TranslateMessage(&messages);
  DispatchMessage(&messages);

  if (eventAvailable) {
    memcpy(evt, &event, sizeof(event));
    eventAvailable = 0;
    return 1;
  }

  return 0;
}

static const luaL_reg lualibs[] = {
  {"base", luaopen_base},
  {"table", luaopen_table},
  {"io", luaopen_io},
  {"string", luaopen_string},
  {"math", luaopen_math},
  {"debug", luaopen_debug},
  {"loadlib", luaopen_loadlib},
  {NULL, NULL}
};

int plua_fputs(const char *s, FILE *f)
{
  fwrite((void *)s, 1, strlen(s), f);
  return 0;
}

size_t plua_fwrite(const void *p, size_t size, size_t n, FILE *f)
{
  size_t r;

  if (f == stdout) {
    PrintString((char *)p, size*n);
    r = n;
  } else if (f == stderr) {
    MessageBox(hwnd, (char *)p, "Information", MB_OK);
    r = n;
  } else
    r = fwrite(p, size, n, f);

  return r;
}

static void l_message (const char *pname, const char *msg) {
  MessageBox(hwnd, msg, pname ? pname : "Error" , MB_OK);
}

static int report (int status) {
  const char *msg;
  if (status) {
    msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "(error with no message)";
    l_message(NULL, msg);
    lua_pop(L, 1);
  }
  return status;
}

static int lcall (int narg, int clear) {
  int status;
  int base = lua_gettop(L) - narg;  // function index
  lua_pushliteral(L, "_TRACEBACK");
  lua_rawget(L, LUA_GLOBALSINDEX);  // get traceback function
  lua_insert(L, base);  // put it under chunk and args
  status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
  lua_remove(L, base);  // remove traceback function
  return status;
}

static int docall (int status) {
  if (status == 0) status = lcall(0, 1);
  return report(status);
}

static int file_input (const char *name) {
  return docall(luaL_loadfile(L, name));
}

int PluaOpen(char *filename)
{
  int status;
  const luaL_reg *lib;
  
  if ((L = lua_open()) == NULL) {
    l_message(NULL, "cannot create state: not enough memory");
    return -1;
  }  

  for (lib = lualibs; lib->func; lib++) {
    lib->func(L);
    lua_settop(L, 0);
  }
  lua_bwlibopen(L);
  lua_packlibopen(L);
  lua_graphlibopen(L);

  if (filename && filename[0])
    file_input(filename);

  return 0;
}

int PluaClose(void)
{
  if (L) {
    lua_graphlibclose(L);
    lua_close(L);
  }
  L = NULL;
  
  return 0;
}

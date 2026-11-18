/*
 * win32.h -- the Win32 imports the port and the runtime call directly.
 * The original built against <windows.h>; we cannot, so these are the
 * hand-written prototypes, one home per name. Plain types only (no ULong,
 * no Mac types) so any TU can include this without pulling a subsystem in.
 */
#ifndef WIN32_H
#define WIN32_H
#include "int32.h"

#ifdef __cplusplus
extern "C" {
#endif

/* user32 window messages */
#define WM_DESTROY          0x0002
#define WM_MOVE             0x0003
#define WM_SIZE             0x0005
#define WM_PAINT            0x000f
#define WM_CLOSE            0x0010
#define WM_QUIT             0x0012
#define WM_ENDSESSION       0x0016
#define WM_ACTIVATEAPP      0x001c
#define WM_SETCURSOR        0x0020
#define WM_GETMINMAXINFO    0x0024
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_CHAR             0x0102
#define WM_SYSKEYDOWN       0x0104
#define WM_SYSKEYUP         0x0105
#define WM_SYSCHAR          0x0106
#define WM_MOUSEMOVE        0x0200
#define WM_LBUTTONDOWN      0x0201
#define WM_LBUTTONUP        0x0202
#define WM_LBUTTONDBLCLK    0x0203
#define WM_RBUTTONDOWN      0x0204
#define WM_RBUTTONUP        0x0205
#define WM_RBUTTONDBLCLK    0x0206
#define WM_MBUTTONDOWN      0x0207
#define WM_MBUTTONUP        0x0208
#define WM_MBUTTONDBLCLK    0x0209
#define WM_MOUSEWHEEL       0x020a
#define WM_QUERYNEWPALETTE  0x030f

/* user32 window styles, virtual keys and other constants */
#define WS_POPUP            0x80000000UL
#define WS_VISIBLE          0x10000000UL
#define WS_CAPTION          0x00c00000UL
#define WS_BORDER           0x00800000UL
#define WS_SYSMENU          0x00080000UL
#define WS_MINIMIZEBOX      0x00020000UL
#define IDC_ARROW           32512
#define VK_SHIFT            0x10
#define VK_CONTROL          0x11
#define VK_MENU             0x12
#define VK_CAPITAL          0x14
#define VK_SCROLL           0x91

/* gdi32 */
#define BLACK_BRUSH         4
#define LOGPIXELSY          90
#define FW_NORMAL           400
#define FW_BOLD             700
#define SRCCOPY             0x00cc0020UL

/* kernel32 files */
#define GENERIC_READ        0x80000000UL
#define OPEN_EXISTING       3
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_BEGIN          0
#define INVALID_HANDLE_VALUE ((void *)-1)
#define FILE_CURRENT        1
#define GENERIC_WRITE       0x40000000UL
#define FILE_SHARE_READ     0x00000001
#define CREATE_NEW          1
#define FILE_ATTRIBUTE_READONLY 0x0001
#define FILE_ATTRIBUTE_HIDDEN 0x0002
#define FILE_ATTRIBUTE_OFFLINE 0x1000
#define SEM_NOOPENFILEERRORBOX 0x8000

/* kernel32 error codes (winerror.h) */
#define ERROR_SUCCESS                   0
#define ERROR_FILE_NOT_FOUND            2
#define ERROR_PATH_NOT_FOUND            3
#define ERROR_TOO_MANY_OPEN_FILES       4
#define ERROR_ACCESS_DENIED             5
#define ERROR_INVALID_HANDLE            6
#define ERROR_NOT_ENOUGH_MEMORY         8
#define ERROR_INVALID_ACCESS            12
#define ERROR_OUTOFMEMORY               14
#define ERROR_INVALID_DRIVE             15
#define ERROR_CURRENT_DIRECTORY         16
#define ERROR_NO_MORE_FILES             18
#define ERROR_WRITE_PROTECT             19
#define ERROR_BAD_UNIT                  20
#define ERROR_SEEK                      25
#define ERROR_SECTOR_NOT_FOUND          27
#define ERROR_SHARING_VIOLATION         32
#define ERROR_LOCK_VIOLATION            33
#define ERROR_HANDLE_EOF                38
#define ERROR_HANDLE_DISK_FULL          39
#define ERROR_REM_NOT_LIST              51
#define ERROR_DUP_NAME                  52
#define ERROR_BAD_NETPATH               53
#define ERROR_DEV_NOT_EXIST             55
#define ERROR_FILE_EXISTS               80
#define ERROR_CANNOT_MAKE               82
#define ERROR_OUT_OF_STRUCTURES         84
#define ERROR_DISK_CHANGE               107
#define ERROR_DRIVE_LOCKED              108
#define ERROR_OPEN_FAILED               110
#define ERROR_DISK_FULL                 112
#define ERROR_NO_MORE_SEARCH_HANDLES    113
#define ERROR_INVALID_NAME              123
#define ERROR_NO_VOLUME_LABEL           125
#define ERROR_NEGATIVE_SEEK             131
#define ERROR_SEEK_ON_DEVICE            132
#define ERROR_DIR_NOT_ROOT              144
#define ERROR_DIR_NOT_EMPTY             145
#define ERROR_PATH_BUSY                 148
#define ERROR_BAD_PATHNAME              161
#define ERROR_BUSY                      170
#define ERROR_ALREADY_EXISTS            183
#define ERROR_FILE_INVALID              1006
#define ERROR_NO_MORE_USER_HANDLES      1158
#define ERROR_NO_VOLUME_ID              1173
#define ERROR_CANCELLED                 1223

/* shell32 special folders, advapi32 tokens */
#define CSIDL_PERSONAL          0x0005
#define CSIDL_APPDATA           0x001a
#define CSIDL_INTERNET_CACHE    0x0020
#define CSIDL_COMMON_APPDATA    0x0023
#define CSIDL_FLAG_CREATE       0x8000
#define TOKEN_QUERY             0x0008

/* kernel32 threads and waits, wsock32 */
#define THREAD_PRIORITY_HIGHEST 2
#define THREAD_PRIORITY_TIME_CRITICAL 15
#define INFINITE            0xffffffff
#define WAIT_TIMEOUT        0x102

/* Win32's RECT: four longs in left/top/right/bottom order, which is what
   DirectDraw's Blt and the window calls expect. Not the Mac Rect. */
typedef struct WinRect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} WinRect;

/* Win32's WNDCLASSA, spelled the way the SDK spells it because that is what a
   caller of RegisterClassA would have written. */
typedef struct WNDCLASSA {
    uint32_t          style;                    /* +0x00 */
    void                   *lpfnWndProc;             /* +0x04 */
    int32_t                   cbClsExtra;               /* +0x08 */
    int32_t                   cbWndExtra;               /* +0x0c */
    void                   *hInstance;               /* +0x10 */
    void                   *hIcon;                   /* +0x14 */
    void                   *hCursor;                 /* +0x18 */
    void                   *hbrBackground;           /* +0x1c */
    const char             *lpszMenuName;            /* +0x20 */
    const char             *lpszClassName;           /* +0x24 */
} WNDCLASSA;

typedef struct OPENFILENAMEA {
    uint32_t lStructSize;      /* +0x00 */
    void *hwndOwner;                /* +0x04 */
    void *hInstance;                /* +0x08 */
    const char *lpstrFilter;        /* +0x0c */
    char *lpstrCustomFilter;        /* +0x10 */
    uint32_t nMaxCustFilter;   /* +0x14 */
    uint32_t nFilterIndex;     /* +0x18 */
    char *lpstrFile;                /* +0x1c */
    uint32_t nMaxFile;         /* +0x20 */
    char *lpstrFileTitle;           /* +0x24 */
    uint32_t nMaxFileTitle;    /* +0x28 */
    const char *lpstrInitialDir;    /* +0x2c */
    const char *lpstrTitle;         /* +0x30 */
    uint32_t Flags;            /* +0x34 */
    unsigned short nFileOffset;     /* +0x38 */
    unsigned short nFileExtension;  /* +0x3a */
    const char *lpstrDefExt;        /* +0x3c */
    int32_t lCustData;                 /* +0x40 */
    void *lpfnHook;                 /* +0x44 */
    const char *lpTemplateName;     /* +0x48 */
} OPENFILENAMEA;                    /* 0x4c = 76 bytes */

typedef struct PAINTSTRUCT {
    void *hdc;                  /* +0x00 */
    int fErase;                 /* +0x04 */
    WinRect rcPaint;            /* +0x08 */
    int fRestore;               /* +0x18 */
    int fIncUpdate;             /* +0x1c */
    char rgbReserved[32];       /* +0x20 */
} PAINTSTRUCT;

typedef struct MemoryStatus {
    uint32_t dwLength;
    uint32_t dwMemoryLoad;
    uint32_t dwTotalPhys;
    uint32_t dwAvailPhys;
    uint32_t dwTotalPageFile;
    uint32_t dwAvailPageFile;
    uint32_t dwTotalVirtual;
    uint32_t dwAvailVirtual;
} MemoryStatus;

/* gdi32 / user32 -- the port's drawing and window calls */
__declspec(dllimport) void * __stdcall GetDC(void *hwnd);
__declspec(dllimport) int __stdcall ReleaseDC(void *hwnd, void *hdc);
__declspec(dllimport) int __stdcall DeleteObject(void *obj);
__declspec(dllimport) int __stdcall UpdateWindow(void *hwnd);
__declspec(dllimport) int __stdcall MoveWindow(void *hwnd, int x, int y,
                                               int w, int h, int repaint);
__declspec(dllimport) void * __stdcall SetFocus(void *hwnd);
__declspec(dllimport) int __stdcall SetForegroundWindow(void *hWnd);
__declspec(dllimport) unsigned short __stdcall RegisterClassA(WNDCLASSA *wc);
__declspec(dllimport) int __stdcall GetWindowRect(void *hwnd, WinRect *r);
                                                         /* IAT 0x0058f8e0 */
__declspec(dllimport) void * __stdcall GetMenu(void *hwnd);
                                                         /* IAT 0x0058f8d4 */
__declspec(dllimport) void * __stdcall GetStockObject(int which);
                                                         /* IAT 0x0058f61c GDI32 */
__declspec(dllimport) void * __stdcall SelectObject(void *hdc, void *hobj);
__declspec(dllimport) void * __stdcall CreateCompatibleDC(void *hdc);
__declspec(dllimport) int __stdcall DeleteDC(void *hdc);
__declspec(dllimport) void * __stdcall SelectPalette(void *hdc, void *hpal, int background);
__declspec(dllimport) unsigned int __stdcall RealizePalette(void *hdc);
__declspec(dllimport) int __stdcall GetDeviceGammaRamp(void *hdc, void *ramp);
__declspec(dllimport) int __stdcall SetDeviceGammaRamp(void *hdc, void *ramp);
__declspec(dllimport) unsigned int __stdcall SetPaletteEntries(void *hpal, unsigned int start,
                                                               unsigned int count,
                                                               const void *entries);
__declspec(dllimport) unsigned int __stdcall SetDIBColorTable(void *hdc, unsigned int start,
                                                              unsigned int count,
                                                              const void *colors);
__declspec(dllimport) int __stdcall OffsetRgn(void *rgn, int dx, int dy);
__declspec(dllimport) void * __stdcall CreateWindowExA(uint32_t exStyle,
    const char *className, const char *windowName, uint32_t style,
    int x, int y, int w, int h, void *parent, void *menu, void *instance,
    void *param);
__declspec(dllimport) int32_t __stdcall DefWindowProcA(void *hwnd,
    uint32_t msg, uint32_t wParam, int32_t lParam);
                                                         /* IAT 0x0058f8f0 */
__declspec(dllimport) int32_t __stdcall GetWindowLongA(void *hwnd, int index);
                                                         /* IAT 0x0058f8d0 */
__declspec(dllimport) int __stdcall AdjustWindowRectEx(WinRect *r,
    uint32_t style, int menu, uint32_t exStyle);
                                                         /* IAT 0x0058f8d8 */
__declspec(dllimport) int __stdcall SystemParametersInfoA(unsigned int action,
    unsigned int param, void *pv, unsigned int flags);
__declspec(dllimport) void * __stdcall LoadCursorA(void *inst,
                                                   const char *name);
__declspec(dllimport) int __stdcall ShowCursor(int show);
__declspec(dllimport) int __stdcall QueryPerformanceCounter(long long *count);
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(long long *freq);
__declspec(dllimport) void * __stdcall CreateDIBSection(void *hdc, void *pbmi,
                                                        unsigned int usage,
                                                        void **ppvBits,
                                                        void *hSection,
                                                        uint32_t offset);

/* kernel32 -- the runtime's process, module and synchronisation calls */
__declspec(dllimport) void __stdcall Sleep(uint32_t ms);
__declspec(dllimport) uint32_t __stdcall GetLastError(void);
                                                         /* IAT 0x0058f724 */
__declspec(dllimport) uint32_t __stdcall GetFileAttributesA(const char *path);
                                                         /* IAT 0x0058f7d0 */
__declspec(dllimport) uint32_t __stdcall GetLogicalDrives(void);
__declspec(dllimport) unsigned int __stdcall SetErrorMode(unsigned int mode);
__declspec(dllimport) unsigned int __stdcall GetDriveTypeA(const char *root);
__declspec(dllimport) int __stdcall GetVolumeInformationA(const char *root, char *name,
                                                          uint32_t nameSize,
                                                          uint32_t *serial,
                                                          uint32_t *maxComponent,
                                                          uint32_t *fsFlags,
                                                          char *fsName,
                                                          uint32_t fsNameSize);
__declspec(dllimport) int __stdcall CloseHandle(void *h);
__declspec(dllimport) int __stdcall FlushFileBuffers(void *h);  /* IAT 0x0058f874 */
__declspec(dllimport) int __stdcall ReadFile(void *h, void *buffer, uint32_t count,
                                             uint32_t *read, void *overlapped);
__declspec(dllimport) int __stdcall WriteFile(void *h, const void *buffer,
                                              uint32_t count, uint32_t *written,
                                              void *overlapped);
__declspec(dllimport) uint32_t __stdcall SetFilePointer(void *h, int32_t distance,
                                                             int32_t *distanceHigh,
                                                             uint32_t method);
__declspec(dllimport) int __stdcall SetEndOfFile(void *h);
__declspec(dllimport) void * __stdcall CreateFileA(const char *path, uint32_t access,
                                                   uint32_t share, void *sa,
                                                   uint32_t disposition,
                                                   uint32_t attributes, void *tmpl);
__declspec(dllimport) uint32_t __stdcall GetFileSize(void *h, uint32_t *high);
__declspec(dllimport) void * __stdcall FindResourceA(void *module, const char *name,
                                                     const char *type);
__declspec(dllimport) uint32_t __stdcall SizeofResource(void *module, void *res);
__declspec(dllimport) void * __stdcall LoadResource(void *module, void *res);
__declspec(dllimport) void * __stdcall LockResource(void *data);
__declspec(dllimport) int __stdcall DeleteFileA(const char *path);
__declspec(dllimport) int __stdcall RemoveDirectoryA(const char *path);
__declspec(dllimport) void * __stdcall GetModuleHandleA(const char *name);
__declspec(dllimport) uint32_t __stdcall WaitForSingleObject(void *h,
                                                                  uint32_t ms);
__declspec(dllimport) int __stdcall ReleaseMutex(void *h);
__declspec(dllimport) void * __stdcall LoadLibraryA(const char *name);
__declspec(dllimport) void * __stdcall GetProcAddress(void *module,
                                                      const char *name);
__declspec(dllimport) void * __stdcall CreateMutexA(void *sa, int owned,
                                                    const char *name);
__declspec(dllimport) uint32_t __stdcall GetSystemDirectoryA(char *buf,
                                                      uint32_t size);
__declspec(dllimport) uint32_t __stdcall GetModuleFileNameA(void *module,
                                                      char *out,
                                                      uint32_t size);
__declspec(dllimport) uint32_t __stdcall GetTickCount(void);
__declspec(dllimport) uint32_t __stdcall GetCurrentThreadId(void);
__declspec(dllimport) void __stdcall InitializeCriticalSection(void *section);
__declspec(dllimport) void __stdcall EnterCriticalSection(void *section);
__declspec(dllimport) void __stdcall LeaveCriticalSection(void *section);
__declspec(dllimport) void __stdcall DeleteCriticalSection(void *section);
__declspec(dllimport) void __stdcall OutputDebugStringA(const char *text);

/* kernel32 threads and events */
typedef uint32_t (__stdcall *LPTHREAD_START_ROUTINE)(void *param);
__declspec(dllimport) void * __stdcall CreateThread(void *sa, uint32_t stack,
    LPTHREAD_START_ROUTINE start, void *param, uint32_t flags,
    uint32_t *threadId);
__declspec(dllimport) int __stdcall SetThreadPriority(void *thread, int priority);
__declspec(dllimport) uint32_t __stdcall SuspendThread(void *thread);
__declspec(dllimport) uint32_t __stdcall ResumeThread(void *thread);
__declspec(dllimport) int __stdcall TerminateThread(void *thread, uint32_t code);
__declspec(dllimport) void * __stdcall CreateEventA(void *sa, int manual, int initial,
                                                   const char *name);
__declspec(dllimport) int __stdcall SetEvent(void *event);

/* kernel32 clock, files and pointer probes */
typedef struct SYSTEMTIME {
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
} SYSTEMTIME;

typedef struct SECURITY_ATTRIBUTES {
    uint32_t nLength;
    void *lpSecurityDescriptor;
    int bInheritHandle;
} SECURITY_ATTRIBUTES;

typedef struct FILETIME {
    uint32_t dwLowDateTime;
    uint32_t dwHighDateTime;
} FILETIME;

__declspec(dllimport) int __stdcall FileTimeToSystemTime(const FILETIME *ft, SYSTEMTIME *st);
                                                         /* IAT 0x0058f7cc */
__declspec(dllimport) int __stdcall FileTimeToLocalFileTime(const FILETIME *ft,
                                                            FILETIME *localTime);
                                                         /* IAT 0x0058f7c8 */

typedef struct WIN32_FIND_DATAA {
    uint32_t dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    uint32_t nFileSizeHigh;
    uint32_t nFileSizeLow;
    uint32_t dwReserved0;
    uint32_t dwReserved1;
    char cFileName[260];
    char cAlternateFileName[14];
} WIN32_FIND_DATAA;

__declspec(dllimport) void * __stdcall FindFirstFileA(const char *pattern,
                                                      WIN32_FIND_DATAA *data);
__declspec(dllimport) int __stdcall FindNextFileA(void *find, WIN32_FIND_DATAA *data);
__declspec(dllimport) int __stdcall FindClose(void *find);
__declspec(dllimport) void __stdcall GetSystemTime(SYSTEMTIME *st);
__declspec(dllimport) void __stdcall GetLocalTime(SYSTEMTIME *st);
__declspec(dllimport) uint32_t __stdcall GetWindowsDirectoryA(char *buf,
                                                                   uint32_t size);
__declspec(dllimport) int __stdcall CreateDirectoryA(const char *path,
                                                     SECURITY_ATTRIBUTES *sa);
__declspec(dllimport) int __stdcall IsBadReadPtr(const void *p, uint32_t size);
__declspec(dllimport) int __stdcall IsBadWritePtr(void *p, uint32_t size);

/* kernel32 thread-local storage */
__declspec(dllimport) uint32_t __stdcall TlsAlloc(void);
__declspec(dllimport) void * __stdcall TlsGetValue(uint32_t index);
__declspec(dllimport) int __stdcall TlsSetValue(uint32_t index, void *value);
__declspec(dllimport) int __stdcall TlsFree(uint32_t index);

/* kernel32 heap, file mapping, paths, processes and the console */
typedef int (__stdcall *PHANDLER_ROUTINE)(uint32_t ctrlType);
__declspec(dllimport) void * __stdcall GetProcessHeap(void);
__declspec(dllimport) void * __stdcall HeapAlloc(void *heap, uint32_t flags,
                                                 uint32_t bytes);
__declspec(dllimport) int __stdcall HeapFree(void *heap, uint32_t flags, void *mem);
__declspec(dllimport) uint32_t __stdcall GlobalSize(void *mem);           /* IAT 0x0058f7b8 */
__declspec(dllimport) int __stdcall VirtualUnlock(void *address, uint32_t size);
                                                         /* IAT 0x0058f7bc */
__declspec(dllimport) void * __stdcall GlobalFree(void *mem);                  /* IAT 0x0058f7c0 */
__declspec(dllimport) void * __stdcall GlobalAlloc(unsigned int flags, uint32_t bytes);
__declspec(dllimport) int __stdcall VirtualLock(void *address, uint32_t size);
__declspec(dllimport) void * __stdcall CreateFileMappingA(void *file,
    SECURITY_ATTRIBUTES *sa, uint32_t protect, uint32_t sizeHigh,
    uint32_t sizeLow, const char *name);
__declspec(dllimport) uint32_t __stdcall GetFullPathNameA(const char *name,
    uint32_t bufLen, char *buf, char **filePart);
__declspec(dllimport) uint32_t __stdcall GetCurrentDirectoryA(uint32_t bufLen,
                                                                   char *buf);
__declspec(dllimport) int __stdcall SetCurrentDirectoryA(const char *path);
__declspec(dllimport) uint32_t __stdcall GetTempPathA(uint32_t bufLen, char *buf);
__declspec(dllimport) uint32_t __stdcall SleepEx(uint32_t ms, int alertable);
__declspec(dllimport) int __stdcall FreeLibrary(void *module);
__declspec(dllimport) int __stdcall SetConsoleCtrlHandler(PHANDLER_ROUTINE handler, int add);
__declspec(dllimport) uint32_t __stdcall GetConsoleTitleA(char *title,
                                                               uint32_t size);
__declspec(dllimport) int __stdcall SetConsoleTitleA(const char *title);

/* user32 */
__declspec(dllimport) short __stdcall GetKeyState(int nVirtKey);   /* IAT 0x0058f8a8 */
__declspec(dllimport) int __stdcall IsIconic(void *hwnd);          /* IAT 0x0058f8dc */
__declspec(dllimport) void __stdcall PostQuitMessage(int code);
__declspec(dllimport) int __stdcall GetSystemMetrics(int index);

typedef struct POINT {
    int32_t x;
    int32_t y;
} POINT;

__declspec(dllimport) int __stdcall ClientToScreen(void *hwnd, POINT *point);

__declspec(dllimport) int __stdcall FillRect(void *hdc, const WinRect *r,
    void *brush);
__declspec(dllimport) int __stdcall DrawTextA(void *hdc, const char *text,
                                              int count, WinRect *r,
                                              unsigned int format);
__declspec(dllimport) unsigned int __stdcall RegisterWindowMessageA(const char *name);
__declspec(dllimport) void * __stdcall GetDesktopWindow(void);
__declspec(dllimport) void * __stdcall GetForegroundWindow(void);
__declspec(dllimport) int __stdcall PostMessageA(void *hwnd, unsigned int msg,
    unsigned int wParam, int32_t lParam);
__declspec(dllimport) int __stdcall WaitMessage(void);
__declspec(dllimport) int __stdcall TranslateMessage(const void *msg);
__declspec(dllimport) int32_t __stdcall DispatchMessageA(const void *msg);
__declspec(dllimport) void * __stdcall SetCapture(void *hwnd);
__declspec(dllimport) int __stdcall ReleaseCapture(void);

/* Win32's MSG, the record PeekMessageA fills. */
typedef struct WinMsg {
    void *hwnd;                                  /* +0x00 */
    unsigned int message;                        /* +0x04 */
    unsigned int wParam;                         /* +0x08 */
    int32_t lParam;                                 /* +0x0c */
    uint32_t time;                          /* +0x10 */
    POINT pt;                                    /* +0x14 */
} WinMsg;
__declspec(dllimport) void * __stdcall FindWindowA(const char *className,
                                                   const char *windowName);
__declspec(dllimport) int32_t __stdcall SetWindowLongA(void *hwnd, int index, int32_t value);
__declspec(dllimport) int __stdcall SetWindowPos(void *hwnd, void *insertAfter,
    int x, int y, int cx, int cy, unsigned int flags);
__declspec(dllimport) int __stdcall MessageBoxA(void *hwnd, const char *text,
                                                const char *caption, unsigned int type);
__declspec(dllimport) int __stdcall ClipCursor(const WinRect *r);
__declspec(dllimport) void * __stdcall CopyIcon(void *icon);
__declspec(dllimport) void * __stdcall SetCursor(void *cursor);

/* gdi32 */
__declspec(dllimport) void * __stdcall CreateFontA(int height, int width,
        int escapement, int orientation, int weight, uint32_t italic,
        uint32_t underline, uint32_t strikeOut, uint32_t charSet,
        uint32_t outPrecision, uint32_t clipPrecision,
        uint32_t quality, uint32_t pitchAndFamily, const char *faceName);
__declspec(dllimport) int __stdcall GetDeviceCaps(void *hdc, int index);
__declspec(dllimport) int __stdcall SetBkMode(void *hdc, int mode);

typedef struct TEXTMETRICA {
    int32_t tmHeight;
    int32_t tmAscent;
    int32_t tmDescent;
    int32_t tmInternalLeading;
    int32_t tmExternalLeading;
    int32_t tmAveCharWidth;
    int32_t tmMaxCharWidth;
    int32_t tmWeight;
    int32_t tmOverhang;
    int32_t tmDigitizedAspectX;
    int32_t tmDigitizedAspectY;
    unsigned char tmFirstChar;
    unsigned char tmLastChar;
    unsigned char tmDefaultChar;
    unsigned char tmBreakChar;
    unsigned char tmItalic;
    unsigned char tmUnderlined;
    unsigned char tmStruckOut;
    unsigned char tmPitchAndFamily;
    unsigned char tmCharSet;
} TEXTMETRICA;

__declspec(dllimport) int __stdcall GetTextMetricsA(void *hdc, TEXTMETRICA *tm);
__declspec(dllimport) uint32_t __stdcall SetTextColor(void *hdc, uint32_t color);
__declspec(dllimport) void * __stdcall CreateRectRgn(int left, int top, int right, int bottom);
__declspec(dllimport) int __stdcall SelectClipRgn(void *hdc, void *hrgn);
__declspec(dllimport) uint32_t __stdcall GetPixel(void *hdc, int x, int y);
__declspec(dllimport) uint32_t __stdcall SetPixel(void *hdc, int x, int y,
                                                       uint32_t color);
__declspec(dllimport) void * __stdcall CreatePalette(const void *logPalette);
#define RGB(r, g, b) \
    ((uint32_t)(((unsigned char)(r) | ((unsigned short)((unsigned char)(g)) << 8)) | \
                     (((uint32_t)(unsigned char)(b)) << 16)))
__declspec(dllimport) int __stdcall BitBlt(void *dst, int x, int y, int width, int height,
                                           void *src, int srcX, int srcY, uint32_t rop);

/* user32, the interface tool */
__declspec(dllimport) int __stdcall InvalidateRect(void *hwnd, const WinRect *r, int erase);
__declspec(dllimport) int __stdcall PeekMessageA(void *msg, void *hwnd, unsigned int first,
                                                 unsigned int last, unsigned int remove);

/* gdi32's LOGFONTA */
typedef struct LOGFONTA {
    int32_t lfHeight;                  /* +0x00 */
    int32_t lfWidth;                   /* +0x04 */
    int32_t lfEscapement;              /* +0x08 */
    int32_t lfOrientation;             /* +0x0c */
    int32_t lfWeight;                  /* +0x10 */
    unsigned char lfItalic;         /* +0x14 */
    unsigned char lfUnderline;      /* +0x15 */
    unsigned char lfStrikeOut;      /* +0x16 */
    unsigned char lfCharSet;        /* +0x17 */
    unsigned char lfOutPrecision;   /* +0x18 */
    unsigned char lfClipPrecision;  /* +0x19 */
    unsigned char lfQuality;        /* +0x1a */
    unsigned char lfPitchAndFamily; /* +0x1b */
    char lfFaceName[32];            /* +0x1c */
} LOGFONTA;                         /* 0x3c */

__declspec(dllimport) void * __stdcall BeginPaint(void *hwnd, PAINTSTRUCT *ps);
__declspec(dllimport) int __stdcall EndPaint(void *hwnd, PAINTSTRUCT *ps);

/* comdlg32 */
__declspec(dllimport) uint32_t __stdcall CommDlgExtendedError(void); /* IAT 0x0058fa78 */
__declspec(dllimport) int __stdcall GetOpenFileNameA(OPENFILENAMEA *ofn); /* IAT 0x0058fa74 */

/* kernel32 */
__declspec(dllimport) int __stdcall MulDiv(int a, int b, int c);
__declspec(dllimport) void __stdcall SetLastError(uint32_t err); /* IAT 0x0058f720 */
__declspec(dllimport) void __stdcall GlobalMemoryStatus(MemoryStatus *ms); /* IAT 0x0058f71c */
__declspec(dllimport) int __stdcall LoadStringA(void *hInstance, unsigned int id,
                                                char *buf, int bufMax); /* IAT 0x0058f90c */

/* shell32 */
__declspec(dllimport) void * __stdcall ShellExecuteA(void *hwnd, const char *op,
    const char *file, const char *params, const char *dir, int show);

/* advapi32 */
#define HKEY_CURRENT_USER   ((void *)0x80000001)
#define KEY_READ            0x20019
__declspec(dllimport) int32_t __stdcall RegQueryValueExA(void *key, const char *name,
    uint32_t *reserved, uint32_t *type, unsigned char *data,
    uint32_t *size);
__declspec(dllimport) int32_t __stdcall RegCloseKey(void *key);
__declspec(dllimport) int32_t __stdcall RegCreateKeyExA(void *key, const char *subKey,
    uint32_t reserved, char *className, uint32_t options,
    uint32_t access, void *security, void **result, uint32_t *disposition);
__declspec(dllimport) int32_t __stdcall RegSetValueExA(void *key, const char *name,
    uint32_t reserved, uint32_t type, const unsigned char *data,
    uint32_t size);

/* winmm */
__declspec(dllimport) uint32_t __stdcall timeGetTime(void);

/* WAVEFORMATEX: 18 bytes, packed as the SDK packs it */
#pragma pack(push, 1)
typedef struct WaveFormat {
    unsigned short formatTag;
    unsigned short channels;
    uint32_t samplesPerSec;
    uint32_t avgBytesPerSec;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
    unsigned short cbSize;
} WaveFormat;
#pragma pack(pop)

/* WAVEHDR */
typedef struct WaveHdr {
    char *data;
    uint32_t bufferLength;
    uint32_t bytesRecorded;
    uint32_t user;
    uint32_t flags;
    uint32_t loops;
    struct WaveHdr *next;
    uint32_t reserved;
} WaveHdr;

/* WAVEOUTCAPSA */
typedef struct WaveOutCaps {
    unsigned short mid;
    unsigned short pid;
    uint32_t driverVersion;
    char name[32];
    uint32_t formats;
    unsigned short channels;
    unsigned short reserved1;
    uint32_t support;
} WaveOutCaps;

__declspec(dllimport) uint32_t __stdcall waveOutGetNumDevs(void);
__declspec(dllimport) uint32_t __stdcall waveOutOpen(void **out, uint32_t device,
    const WaveFormat *format, uint32_t callback, uint32_t instance,
    uint32_t flags);
__declspec(dllimport) uint32_t __stdcall waveOutClose(void *wave);
__declspec(dllimport) uint32_t __stdcall waveOutReset(void *wave);
__declspec(dllimport) uint32_t __stdcall waveOutPrepareHeader(void *wave, WaveHdr *hdr,
                                                                   uint32_t size);
__declspec(dllimport) uint32_t __stdcall waveOutUnprepareHeader(void *wave, WaveHdr *hdr,
                                                                     uint32_t size);
__declspec(dllimport) uint32_t __stdcall waveOutWrite(void *wave, WaveHdr *hdr,
                                                           uint32_t size);
__declspec(dllimport) uint32_t __stdcall waveOutGetDevCapsA(uint32_t device,
    WaveOutCaps *caps, uint32_t size);
__declspec(dllimport) uint32_t __stdcall waveOutGetVolume(void *wave,
                                                               uint32_t *volume);
__declspec(dllimport) uint32_t __stdcall waveOutSetVolume(void *wave,
                                                               uint32_t volume);

#ifdef __cplusplus
}
#endif

#endif /* WIN32_H */

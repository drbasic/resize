// Win32Project1.cpp : Defines the entry point for the application.
//
#include "stdafx.h"

#include "resize.h"

#include <algorithm>
#include <sstream>

#define MAX_LOADSTRING 100

namespace {
RECT initial_rect;
bool initial_rect_set = false;
constexpr long kStickySize = 10;
const double scale = 0.75;
}

// Global Variables:
HINSTANCE hInst;                     // current instance
WCHAR szTitle[MAX_LOADSTRING];       // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING]; // the main window class name

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // TODO: Place code here.

  // Initialize global strings
  LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadStringW(hInstance, IDC_WIN32PROJECT1, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);

  // Perform application initialization:
  if (!InitInstance(hInstance, nCmdShow)) {
    return FALSE;
  }

  HACCEL hAccelTable =
      LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32PROJECT1));

  MSG msg;

  // Main message loop:
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance) {
  WNDCLASSEXW wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32PROJECT1));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WIN32PROJECT1);
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

  return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
  hInst = hInstance; // Store instance handle in our global variable

  HWND hWnd =
      CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                    0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

  if (!hWnd) {
    return FALSE;
  }

  SetWindowPos(hWnd, nullptr, 100, 100, 800, 800 * scale, 0);
  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return TRUE;
}

bool InOneMonitor(const RECT &rect, HMONITOR *monitor) {
  *monitor =
      ::MonitorFromPoint(POINT{rect.left, rect.top}, MONITOR_DEFAULTTONULL);
  if (*monitor == nullptr)
    return false;
  return *monitor == ::MonitorFromPoint(POINT{rect.right, rect.top},
                                        MONITOR_DEFAULTTONULL) &&
         *monitor == ::MonitorFromPoint(POINT{rect.left, rect.bottom},
                                        MONITOR_DEFAULTTONULL) &&
         *monitor == ::MonitorFromPoint(POINT{rect.right, rect.bottom},
                                        MONITOR_DEFAULTTONULL);
}

RECT GetMonitorRect(HMONITOR monitor) {
  MONITORINFO mon_info{};
  mon_info.cbSize = sizeof(mon_info);
  if (!::GetMonitorInfoW(monitor, &mon_info))
    return RECT{};
  return mon_info.rcWork;
}

long Sticky(long coord, long edge) {
  if (std::abs(coord - edge) < kStickySize)
    return edge;
  return coord;
}

bool ProcessResize(HWND hwnd, WPARAM wParam, RECT *rect) {
  HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
  const RECT monitor_rect = GetMonitorRect(monitor);
  const auto monitor_height = monitor_rect.bottom - monitor_rect.top + 1;
  const auto monitor_width = monitor_rect.right - monitor_rect.left + 1;

  if (false) {
    rect->left = Sticky(rect->left, monitor_rect.left);
    rect->top = Sticky(rect->top, monitor_rect.top);
    rect->right = Sticky(rect->right, monitor_rect.right);
    rect->bottom = Sticky(rect->bottom, monitor_rect.bottom);
  }

  if (!initial_rect_set) {
    initial_rect = *rect;
    initial_rect_set = true;
  }

  const auto max_window_width =
      std::min<long>(monitor_height / scale, monitor_width);
  const auto max_window_height =
      std::min<long>(monitor_width * scale, monitor_height);

  const auto user_width = rect->right - rect->left + 1;
  const auto user_height = rect->bottom - rect->top + 1;

  switch (wParam) {
  case WMSZ_RIGHT:
  case WMSZ_LEFT: {
    // Ширина не может превышать размеров экрана.
    const long new_width = std::min<long>(user_width, max_window_width);
    // Пересчитываем по известной ширине высоту.
    const long new_height =
        std::min<long>(new_width * scale, max_window_height);

    // Задаем вертикальное положение окна.
    rect->top = initial_rect.top;
    rect->bottom = rect->top + new_height;
    if (rect->bottom > monitor_rect.bottom) {
      // Если оказалось что нижняя граница ушла ниже границы экрана - смещаем
      // окно вверх.
      rect->bottom = monitor_rect.bottom;
      rect->top = rect->bottom - new_height;
    }

    if (wParam == WMSZ_RIGHT) {
      // Если пользователь держится за правую грань - двигаем правую.
      rect->right = rect->left + new_width;
    } else {
      // Если пользователь держится за левую грань - двигаем левую.
      rect->left = rect->right - new_width;
    }
  } break;

  case WMSZ_BOTTOM:
  case WMSZ_TOP: {
    // Высота не может превышать размеров экрана.
    const long new_height = std::min<long>(user_height, max_window_height);
    // Пересчитываем по известной высоте ширину.
    const long new_width = std::min<long>(new_height / scale, max_window_width);

    // Возвращаем окно в первоначальное положение, а потом задаем новые размеры
    // и смещаем чтобы не вылезать за пределы экрана.
    *rect = initial_rect;

    // В зависмости от грани, за которую держится пользователь двигаем верх или
    // низ окна.
    if (wParam == WMSZ_BOTTOM)
      rect->bottom = rect->top + new_height;
    else
      rect->top = rect->bottom - new_height;

    // Ширину изменям за счет левой и правой границы симметрично.
    const long rect_center_x = (rect->right + rect->left) / 2;
    rect->left = rect_center_x - new_width / 2;
    rect->right = rect->left + new_width;

    // Не даем вылезти за границы окна.
    if (rect->left < monitor_rect.left) {
      rect->left = monitor_rect.left;
      rect->right = rect->left + new_width;
    } else if (rect->right > monitor_rect.right) {
      rect->right = monitor_rect.right;
      rect->left = rect->right - new_width;
    }
  } break;

  case WMSZ_BOTTOMRIGHT:
  case WMSZ_BOTTOMLEFT:
  case WMSZ_TOPRIGHT:
  case WMSZ_TOPLEFT: {
    // Считаем какие размеры получаются если курсор лежит на вертикальной
    // границе.
    const long new_width_from_user_width =
        std::min<long>(user_width, max_window_width);
    const long new_height_from_user_width =
        std::min<long>(new_width_from_user_width * scale, max_window_height);
    // Считаем какие размеры получаются если курсор лежит на нижней границе.
    const long new_height_from_user_heigth =
        std::min<long>(user_height, max_window_height);
    const long new_width_from_user_heigth =
        std::min<long>(new_height_from_user_heigth / scale, max_window_width);
    // Какой прямоугольник больше - там и лежит курсор. Его и берем.
    const bool on_vert_edge =
        new_width_from_user_width > new_width_from_user_heigth;
    const long new_user_width =
        on_vert_edge ? new_width_from_user_width : new_width_from_user_heigth;
    const long new_user_height =
        on_vert_edge ? new_height_from_user_width : new_height_from_user_heigth;

    const bool left_resize =
        (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_BOTTOMLEFT);
    const bool top_resize =
        (wParam == WMSZ_TOPRIGHT) || (wParam == WMSZ_TOPLEFT);
    long max_width = 0;
    if (left_resize) {
      // Обрезаем прямоугольник чтобы он вмещался в экран. По левому краю.
      max_width = std::min<long>(initial_rect.right - monitor_rect.left,
                                 new_user_width);
    } else {
      // Обрезаем прямоугольник чтобы он вмещался в экран. По правому краю.
      max_width = std::min<long>(monitor_rect.right - initial_rect.left,
                                 new_user_width);
    }
    long max_height = max_width * scale;
    if (top_resize) {
      // По верхнему краю.
      max_height =
          std::min<long>(initial_rect.bottom - monitor_rect.top, max_height);
    } else {
      // По нижнему краю.
      max_height =
          std::min<long>(monitor_rect.bottom - initial_rect.top, max_height);
    }
    max_width = max_height / scale;

    // Готово. Задаем положение окна.
    rect->top =
        (top_resize) ? initial_rect.bottom - max_height : initial_rect.top;
    rect->left =
        (left_resize) ? initial_rect.right - max_width : initial_rect.left;
    rect->bottom =
        (top_resize) ? initial_rect.bottom : initial_rect.top + max_height;
    rect->right =
        (left_resize) ? initial_rect.right : initial_rect.left + max_width;
  } break;
  }

  return true;
}
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  switch (message) {
  case WM_COMMAND: {
    int wmId = LOWORD(wParam);
    // Parse the menu selections:
    switch (wmId) {
    case IDM_ABOUT:
      DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
      break;
    case IDM_EXIT:
      DestroyWindow(hWnd);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
  } break;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    // TODO: Add any drawing code that uses hdc here...
    EndPaint(hWnd, &ps);
  } break;
  case WM_NCMOUSEMOVE:
  case WM_MOUSEMOVE: {
    if (!(wParam & MK_LBUTTON)) {
      initial_rect_set = false;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
  } break;
  case WM_SIZING: {
    return ProcessResize(hWnd, wParam, reinterpret_cast<RECT *>(lParam));
  } break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);
  switch (message) {
  case WM_INITDIALOG:
    return (INT_PTR)TRUE;

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
      EndDialog(hDlg, LOWORD(wParam));
      return (INT_PTR)TRUE;
    }
    break;
  }
  return (INT_PTR)FALSE;
}

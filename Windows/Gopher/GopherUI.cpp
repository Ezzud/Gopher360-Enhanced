#include "GopherUI.h"
#include "Gopher.h"
#include "CXBOXController.h"
#include "ConfigFile.h"

#include <sstream>
#include <shellapi.h>
#include <strsafe.h>
#include <gdiplus.h>
#include <cstring>
#include "resource.h"
#include "Version.h"
#include <uxtheme.h>
#include <commctrl.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")

#define ID_CONTROLLER 100
#define ID_STATUS 101
#define ID_ENABLE 102
#define ID_SETTINGS 103
#define ID_ENABLED_TEXT 104
#define ID_MAPPING_BASE 200
#define ID_TRAY_OPEN 300
#define ID_TRAY_EXIT 301
#define ID_SETTING_REDUCE 400
#define ID_SETTING_BOOT 401
#define ID_SETTING_APPLY 402
#define ID_SETTING_CANCEL 403
#define ID_SETTING_RESET 404
#define ID_OSK_MAPPING 500
#define ID_MOUSE_MAPPING_BASE 510
#define ID_LINK_EZZUD 600
#define ID_LINK_TYLEMAGNE 601
#define ID_LINK_REPOSITORY 602
#define ID_SETTINGS_REPO_LABEL 603
#define ID_SETTINGS_VERSION_LABEL 604
#define ID_SETTINGS_ORIGINAL_LABEL 605
#define ID_MAIN_FOOTER_LABEL 606
#define ID_MAIN_FOOTER_ORIGINAL 607

GopherUI *GopherUI::_instance = NULL;

namespace
{
  const char *MAPPING_KEYS[] = {
    "GAMEPAD_DPAD_UP", "GAMEPAD_DPAD_DOWN", "GAMEPAD_DPAD_LEFT", "GAMEPAD_DPAD_RIGHT",
    "GAMEPAD_START", "GAMEPAD_BACK", "GAMEPAD_LEFT_THUMB", "GAMEPAD_RIGHT_THUMB",
    "GAMEPAD_LEFT_SHOULDER", "GAMEPAD_RIGHT_SHOULDER", "GAMEPAD_A", "GAMEPAD_B",
    "GAMEPAD_X", "GAMEPAD_Y", "GAMEPAD_TRIGGER_LEFT", "GAMEPAD_TRIGGER_RIGHT"
  };
  const char *MAPPING_NAMES[] = {
    "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "Start", "Back",
    "Left stick press", "Right stick press", "Left shoulder", "Right shoulder",
    "A", "B", "X", "Y", "Left trigger", "Right trigger"
  };
  const int MAPPING_COUNT = sizeof(MAPPING_KEYS) / sizeof(MAPPING_KEYS[0]);
  const char *MOUSE_MAPPING_KEYS[] = { "CONFIG_MOUSE_LEFT", "CONFIG_MOUSE_RIGHT", "CONFIG_MOUSE_MIDDLE" };
  const char *MOUSE_MAPPING_NAMES[] = { "Left click", "Right click", "Middle click" };
  const int MOUSE_MAPPING_COUNT = sizeof(MOUSE_MAPPING_KEYS) / sizeof(MOUSE_MAPPING_KEYS[0]);
  const COLORREF BACKGROUND = RGB(24, 27, 32);
  const COLORREF PANEL = RGB(34, 38, 45);
  const COLORREF TEXT = RGB(230, 234, 240);
  const COLORREF ENABLED = RGB(72, 190, 145);
  const COLORREF DISABLED = RGB(220, 105, 105);
  const COLORREF TOGGLE_ON = RGB(55, 135, 220);
  const COLORREF TOGGLE_OFF = RGB(85, 92, 104);
  const COLORREF BUTTON = RGB(47, 53, 63);
  const COLORREF BUTTON_HOVER = RGB(61, 70, 83);
  const COLORREF BORDER = RGB(70, 78, 91);

  void setFont(HWND control, int size, bool bold);
  void setLinkFont(HWND control, int size);

  HWND createHyperlink(HWND parent, HINSTANCE instance, int id, const char *text, int x, int y, int width, int height)
  {
    HWND link = CreateWindowA("STATIC", text, WS_CHILD | WS_VISIBLE | SS_NOTIFY,
      x, y, width, height, parent, (HMENU)id, instance, NULL);
    setLinkFont(link, 10);
    return link;
  }

  void drawMappingButton(const DRAWITEMSTRUCT *draw)
  {
    bool pressed = (draw->itemState & ODS_SELECTED) != 0;
    HBRUSH fill = CreateSolidBrush(pressed ? BUTTON_HOVER : BUTTON);
    FillRect(draw->hDC, &draw->rcItem, fill);
    DeleteObject(fill);
    HPEN borderPen = CreatePen(PS_SOLID, 1, BORDER);
    HGDIOBJ oldPen = SelectObject(draw->hDC, borderPen);
    HGDIOBJ oldBrush = SelectObject(draw->hDC, GetStockObject(NULL_BRUSH));
    RoundRect(draw->hDC, draw->rcItem.left, draw->rcItem.top, draw->rcItem.right, draw->rcItem.bottom, 6, 6);
    SelectObject(draw->hDC, oldBrush);
    SelectObject(draw->hDC, oldPen);
    DeleteObject(borderPen);
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, TEXT);
    TCHAR value[128];
    GetWindowText(draw->hwndItem, value, ARRAYSIZE(value));
    DrawText(draw->hDC, value, -1, const_cast<RECT *>(&draw->rcItem), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  }

  std::string controllerKeyText(unsigned long value, const char *key)
  {
    if (strcmp(key, "CONFIG_MOUSE_LEFT") == 0) return "Left click";
    if (strcmp(key, "CONFIG_MOUSE_RIGHT") == 0) return "Right click";
    if (strcmp(key, "CONFIG_MOUSE_MIDDLE") == 0) return "Middle click";
    if (strcmp(key, "CONFIG_OSK") == 0 && value == XINPUT_GAMEPAD_BACK) return "Select";
    if (value == 0) return strcmp(key, "CONFIG_MOUSE_LEFT") == 0 ? "Left click" : "Unassigned";

    HKL layout = GetKeyboardLayout(0);
    UINT scanCode = MapVirtualKeyEx(value, MAPVK_VK_TO_VSC_EX, layout);
    WCHAR name[128];
    if (scanCode != 0 && GetKeyNameTextW((LONG)(scanCode << 16), name, ARRAYSIZE(name)) > 0)
    {
      char converted[128];
      WideCharToMultiByte(CP_ACP, 0, name, -1, converted, ARRAYSIZE(converted), NULL, NULL);
      return converted;
    }
    if (value >= 'A' && value <= 'Z') return std::string(1, (char)value);
    if (value >= '0' && value <= '9') return std::string(1, (char)value);
    if (value >= VK_F1 && value <= VK_F24)
    {
      char functionName[16];
      StringCchPrintfA(functionName, ARRAYSIZE(functionName), "F%lu", value - VK_F1 + 1);
      return functionName;
    }
    switch (value)
    {
      case VK_BROWSER_BACK: return "Browser Back";
      case VK_BROWSER_FORWARD: return "Browser Forward";
      case VK_BROWSER_REFRESH: return "Browser Refresh";
      case VK_BROWSER_STOP: return "Browser Stop";
      case VK_BROWSER_SEARCH: return "Browser Search";
      case VK_BROWSER_FAVORITES: return "Browser Favorites";
      case VK_BROWSER_HOME: return "Browser Home";
      case VK_VOLUME_MUTE: return "Mute";
      case VK_VOLUME_DOWN: return "Volume Down";
      case VK_VOLUME_UP: return "Volume Up";
      case VK_MEDIA_NEXT_TRACK: return "Next Track";
      case VK_MEDIA_PREV_TRACK: return "Previous Track";
      case VK_MEDIA_STOP: return "Media Stop";
      case VK_MEDIA_PLAY_PAUSE: return "Play/Pause";
      case VK_RETURN: return "Enter";
      case VK_ESCAPE: return "Escape";
      case VK_TAB: return "Tab";
      case VK_SPACE: return "Space";
      case VK_BACK: return "Backspace";
      case VK_DELETE: return "Delete";
      case VK_INSERT: return "Insert";
      case VK_HOME: return "Home";
      case VK_END: return "End";
      case VK_PRIOR: return "Page Up";
      case VK_NEXT: return "Page Down";
      case VK_LEFT: return "Left Arrow";
      case VK_RIGHT: return "Right Arrow";
      case VK_UP: return "Up Arrow";
      case VK_DOWN: return "Down Arrow";
      case VK_LWIN: return "Left Windows";
      case VK_RWIN: return "Right Windows";
      case VK_LSHIFT: return "Left Shift";
      case VK_RSHIFT: return "Right Shift";
      case VK_LCONTROL: return "Left Control";
      case VK_RCONTROL: return "Right Control";
      case VK_LMENU: return "Left Alt";
      case VK_RMENU: return "Right Alt";
    }
    char buffer[32];
    StringCchPrintfA(buffer, ARRAYSIZE(buffer), "Virtual key %lu", value);
    return buffer;
  }

  std::string keyText(const ConfigFile &config, const char *key)
  {
    unsigned long value = strtoul(config.getValueOfKey<std::string>(key, "0").c_str(), NULL, 0);
    return controllerKeyText(value, key);
  }

  std::string xinputButtonText(DWORD value)
  {
    switch (value)
    {
      case XINPUT_GAMEPAD_DPAD_UP: return "D-pad Up";
      case XINPUT_GAMEPAD_DPAD_DOWN: return "D-pad Down";
      case XINPUT_GAMEPAD_DPAD_LEFT: return "D-pad Left";
      case XINPUT_GAMEPAD_DPAD_RIGHT: return "D-pad Right";
      case XINPUT_GAMEPAD_START: return "Start";
      case XINPUT_GAMEPAD_BACK: return "Select";
      case XINPUT_GAMEPAD_LEFT_THUMB: return "Left stick press";
      case XINPUT_GAMEPAD_RIGHT_THUMB: return "Right stick press";
      case XINPUT_GAMEPAD_LEFT_SHOULDER: return "Left shoulder";
      case XINPUT_GAMEPAD_RIGHT_SHOULDER: return "Right shoulder";
      case XINPUT_GAMEPAD_A: return "A";
      case XINPUT_GAMEPAD_B: return "B";
      case XINPUT_GAMEPAD_X: return "X";
      case XINPUT_GAMEPAD_Y: return "Y";
      default: return "Unassigned";
    }
  }

  std::string xinputMappingText(const ConfigFile &config, const char *key)
  {
    DWORD value = strtoul(config.getValueOfKey<std::string>(key, "0").c_str(), NULL, 0);
    return xinputButtonText(value);
  }

  void setFont(HWND control, int size, bool bold)
  {
    LOGFONT font;
    ZeroMemory(&font, sizeof(font));
    font.lfHeight = -size;
    font.lfWeight = bold ? FW_SEMIBOLD : FW_NORMAL;
    StringCchCopy(font.lfFaceName, LF_FACESIZE, TEXT("Segoe UI"));
    HFONT handle = CreateFontIndirect(&font);
    SendMessage(control, WM_SETFONT, (WPARAM)handle, TRUE);
  }

  void setLinkFont(HWND control, int size)
  {
    LOGFONT font;
    ZeroMemory(&font, sizeof(font));
    font.lfHeight = -size;
    font.lfWeight = FW_NORMAL;
    font.lfUnderline = TRUE;
    StringCchCopy(font.lfFaceName, LF_FACESIZE, TEXT("Segoe UI"));
    HFONT handle = CreateFontIndirect(&font);
    SendMessage(control, WM_SETFONT, (WPARAM)handle, TRUE);
  }

  void openLink(int id)
  {
    const char *url = id == ID_LINK_EZZUD ? "https://github.com/ezzud" :
      id == ID_LINK_TYLEMAGNE ? "https://github.com/tylemagne/gopher360" :
      "https://github.com/ezzud/gopher360-enhanced";
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
  }

  /**
   * Paints a flat dark action button for the settings dialog.
   * Params: draw is the owner-draw information; label is the button caption; primary selects the accent action.
   * Returns: none.
   */
  void drawSettingsButton(const DRAWITEMSTRUCT *draw, const TCHAR *label, bool primary)
  {
    bool pressed = (draw->itemState & ODS_SELECTED) != 0;
    HBRUSH fill = CreateSolidBrush(primary ? TOGGLE_ON : (pressed ? BUTTON_HOVER : BUTTON));
    FillRect(draw->hDC, &draw->rcItem, fill);
    DeleteObject(fill);
    HPEN borderPen = CreatePen(PS_SOLID, 1, primary ? TOGGLE_ON : BORDER);
    HGDIOBJ oldPen = SelectObject(draw->hDC, borderPen);
    HGDIOBJ oldBrush = SelectObject(draw->hDC, GetStockObject(NULL_BRUSH));
    Rectangle(draw->hDC, draw->rcItem.left, draw->rcItem.top, draw->rcItem.right, draw->rcItem.bottom);
    SelectObject(draw->hDC, oldBrush);
    SelectObject(draw->hDC, oldPen);
    DeleteObject(borderPen);
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, TEXT);
    DrawText(draw->hDC, label, -1, const_cast<RECT *>(&draw->rcItem), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  }
}

/**
 * Creates the UI controller and stores the shared runtime objects.
 * Params: gopher is the input engine; config is the editable configuration; controllers are the four XInput slots.
 * Returns: none.
 */
GopherUI::GopherUI(Gopher *gopher, ConfigFile *config, const std::vector<CXBOXController *> &controllers)
  : _gopher(gopher), _config(config), _controllers(controllers), _window(NULL), _controllerSelect(NULL),
    _statusLabel(NULL), _enabledLabel(NULL), _enabledButton(NULL), _settingsWindow(NULL), _reduceCheck(NULL), _bootCheck(NULL),
    _capturing(-1), _settingsDefaults(false), _lastConnected(false), _lastEnabled(true), _capturingController(false),
    _controllerCaptureMapping(-1),
    _previousControllerButtons(0), _controllerRefreshTicks(0),
    _settingsImage(NULL), _settingsStream(NULL), _gdiplusToken(0), _inputRunning(false)
{
  ZeroMemory(&_tray, sizeof(_tray));
  _instance = this;
}

/**
 * Creates the main window and controller mapping controls.
 * Params: instance is the process module handle.
 * Returns: none.
 */
void GopherUI::createMainWindow(HINSTANCE instance)
{
  WNDCLASS wc;
  ZeroMemory(&wc, sizeof(wc));
  wc.hInstance = instance;
  wc.lpfnWndProc = windowProc;
  wc.lpszClassName = TEXT("Gopher360MainWindow");
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
  wc.hbrBackground = CreateSolidBrush(BACKGROUND);
  RegisterClass(&wc);

  _window = CreateWindowEx(0, wc.lpszClassName, TEXT("Gopher360"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
    CW_USEDEFAULT, CW_USEDEFAULT, 660, 750, NULL, NULL, instance, NULL);
  SetWindowLongPtr(_window, GWLP_USERDATA, (LONG_PTR)this);

  _controllerSelect = CreateWindow(TEXT("COMBOBOX"), NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS,
    58, 18, 420, 280, _window, (HMENU)ID_CONTROLLER, instance, NULL);
  _statusLabel = CreateWindow(TEXT("STATIC"), TEXT("No controller"), WS_CHILD | WS_VISIBLE,
    490, 18, 140, 24, _window, (HMENU)ID_STATUS, instance, NULL);
  CreateWindow(TEXT("BUTTON"), TEXT(""), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
    12, 11, 38, 38, _window, (HMENU)ID_SETTINGS, instance, NULL);
  createMappingRows();
  int actionY = 135 + MAPPING_COUNT * 24 + 46;
  HINSTANCE rowInstance = (HINSTANCE)GetWindowLongPtr(_window, GWLP_HINSTANCE);
  HWND section = CreateWindowA("STATIC", "Controller actions", WS_CHILD | WS_VISIBLE,
    40, actionY - 42, 260, 22, _window, NULL, rowInstance, NULL);
  setFont(section, 12, true);
  HWND separator = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
    40, actionY - 16, 500, 2, _window, NULL, rowInstance, NULL);
  for (int index = 0; index < MOUSE_MAPPING_COUNT; ++index)
  {
    int rowY = actionY + index * 24;
    HWND label = CreateWindowA("STATIC", MOUSE_MAPPING_NAMES[index], WS_CHILD | WS_VISIBLE,
      40, rowY, 260, 22, _window, NULL, rowInstance, NULL);
    HWND button = CreateWindow(TEXT("BUTTON"), TEXT(""), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
      360, rowY - 2, 180, 23, _window, (HMENU)(ID_MOUSE_MAPPING_BASE + index), rowInstance, NULL);
    setFont(label, 13, false);
    setFont(button, 12, false);
    SetWindowTextA(button, xinputMappingText(*_config, MOUSE_MAPPING_KEYS[index]).c_str());
  }
  int oskY = actionY + MOUSE_MAPPING_COUNT * 24;
  HWND oskLabel = CreateWindowA("STATIC", "Show visual keyboard", WS_CHILD | WS_VISIBLE,
    40, oskY, 260, 22, _window, NULL, instance, NULL);
  HWND oskButton = CreateWindow(TEXT("BUTTON"), TEXT(""), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
    360, oskY - 2, 180, 23, _window, (HMENU)ID_OSK_MAPPING, instance, NULL);
  setFont(oskLabel, 13, false);
  setFont(oskButton, 12, false);
  SetWindowTextA(oskButton, xinputButtonText((DWORD)strtoul(_config->getValueOfKey<std::string>("CONFIG_OSK", "0").c_str(), NULL, 0)).c_str());
  HWND footerLabel = CreateWindowA("STATIC", "Gopher360-Enhanced v" GOPHER_VERSION_DISPLAY " - Created by", WS_CHILD | WS_VISIBLE,
    24, 666, 205, 18, _window, (HMENU)ID_MAIN_FOOTER_LABEL, instance, NULL);
  setFont(footerLabel, 10, false);
  createHyperlink(_window, instance, ID_LINK_EZZUD, "Ezzud", 231, 666, 35, 18);
  HWND footerOriginal = CreateWindowA("STATIC", " - Gopher360 by", WS_CHILD | WS_VISIBLE,
    270, 666, 85, 18, _window, (HMENU)ID_MAIN_FOOTER_ORIGINAL, instance, NULL);
  setFont(footerOriginal, 10, false);
  createHyperlink(_window, instance, ID_LINK_TYLEMAGNE,
    "Tylemagne",
    355, 666, 75, 18);
  refreshControllerList();
  SetWindowTheme(_controllerSelect, TEXT(""), TEXT(""));
  SendMessage(_controllerSelect, CB_SETDROPPEDWIDTH, 420, 0);
  setFont(_controllerSelect, 15, false);
  setFont(_statusLabel, 14, true);
  updateStatus();
  ShowWindow(_window, SW_SHOW);
}

/**
 * Creates the mapping name and key capture controls.
 * Params: none.
 * Returns: none.
 */
void GopherUI::createMappingRows()
{
  HINSTANCE instance = (HINSTANCE)GetWindowLongPtr(_window, GWLP_HINSTANCE);
  for (int index = 0; index < MAPPING_COUNT; ++index)
  {
    int y = 135 + index * 24;
    HWND label = CreateWindowA("STATIC", MAPPING_NAMES[index], WS_CHILD | WS_VISIBLE,
      40, y, 260, 22, _window, NULL, instance, NULL);
    HWND button = CreateWindow(TEXT("BUTTON"), TEXT(""), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
      360, y - 2, 180, 23, _window, (HMENU)(ID_MAPPING_BASE + index), instance, NULL);
    setFont(label, 13, false);
    setFont(button, 12, false);
    SetWindowLongPtr(button, GWLP_USERDATA, (LONG_PTR)this);
    SetProp(button, TEXT("GopherMapping"), (HANDLE)(LONG_PTR)index);
    SetWindowTextA(button, keyText(*_config, MAPPING_KEYS[index]).c_str());
  }
}

/**
 * Refreshes the controller selector and keeps the first connected slot selected.
 * Params: none.
 * Returns: none.
 */
void GopherUI::refreshControllerList()
{
  std::vector<int> connectedSlots;
  for (int index = 0; index < (int)_controllers.size(); ++index)
  {
    if (_controllers[index]->IsConnected())
    {
      connectedSlots.push_back(index);
    }
  }
  bool changed = connectedSlots.size() != _connectedSlots.size();
  for (size_t index = 0; !changed && index < connectedSlots.size(); ++index)
  {
    changed = connectedSlots[index] != _connectedSlots[index];
  }
  if (!changed)
  {
    if (connectedSlots.empty())
    {
      TCHAR current[64];
      GetWindowText(_controllerSelect, current, ARRAYSIZE(current));
      if (lstrcmp(current, TEXT("No controller")) != 0)
        SetWindowText(_controllerSelect, TEXT("No controller"));
    }
    return;
  }
  int previousSlot = -1;
  int previousItem = (int)SendMessage(_controllerSelect, CB_GETCURSEL, 0, 0);
  if (previousItem >= 0)
  {
    previousSlot = (int)SendMessage(_controllerSelect, CB_GETITEMDATA, previousItem, 0);
  }
  _connectedSlots = connectedSlots;
  SendMessage(_controllerSelect, CB_RESETCONTENT, 0, 0);
  int selected = CB_ERR;
  for (size_t index = 0; index < connectedSlots.size(); ++index)
  {
    std::string name = _controllers[connectedSlots[index]]->GetDisplayName();
    int item = (int)SendMessageA(_controllerSelect, CB_ADDSTRING, 0, (LPARAM)name.c_str());
    SendMessage(_controllerSelect, CB_SETITEMDATA, item, connectedSlots[index]);
    if (connectedSlots[index] == previousSlot || selected == CB_ERR)
    {
      selected = item;
    }
  }
  if (selected != CB_ERR)
  {
    SendMessage(_controllerSelect, CB_SETCURSEL, selected, 0);
    _gopher->setController(_controllers[SendMessage(_controllerSelect, CB_GETITEMDATA, selected, 0)]);
  }
  else
  {
    TCHAR current[64];
    GetWindowText(_controllerSelect, current, ARRAYSIZE(current));
    if (lstrcmp(current, TEXT("No controller")) != 0)
      SetWindowText(_controllerSelect, TEXT("No controller"));
  }
}

/**
 * Updates the settings dialog controls from either disk values or built-in defaults.
 * Params: defaults selects the in-memory default configuration.
 * Returns: none.
 */
void GopherUI::refreshSettings(bool defaults)
{
  _settingsDefaults = defaults;
  ConfigFile values = *_config;
  if (defaults)
  {
    values.resetDefaults();
  }
  SendMessage(_reduceCheck, BM_SETCHECK, values.getValueOfKey<int>("CONFIG_REDUCE_WHEN_CLOSING", 1) ? BST_CHECKED : BST_UNCHECKED, 0);
  SendMessage(_bootCheck, BM_SETCHECK, values.getValueOfKey<int>("CONFIG_START_ON_BOOT", 0) ? BST_CHECKED : BST_UNCHECKED, 0);
}

/**
 * Captures the next keyboard key and writes it to the selected mapping.
 * Params: key is the virtual-key code received from Windows.
 * Returns: none.
 */
void GopherUI::captureKey(WPARAM key)
{
  if (_capturing < 0 || _capturing >= MAPPING_COUNT)
  {
    return;
  }
  char value[32];
  StringCchPrintfA(value, ARRAYSIZE(value), key == VK_ESCAPE ? "0" : "0x%02lX", (unsigned long)key);
  _config->setValue(MAPPING_KEYS[_capturing], value);
  _config->save();
  _gopher->reloadConfig();
  HWND button = GetDlgItem(_window, ID_MAPPING_BASE + _capturing);
  SetWindowTextA(button, key == VK_ESCAPE ? "Unassigned" : controllerKeyText((unsigned long)key, MAPPING_KEYS[_capturing]).c_str());
  _capturing = -1;
  SetFocus(_window);
}

/**
 * Captures the next newly pressed XInput controller button for the visual keyboard mapping.
 * Params: none.
 * Returns: none.
 */
void GopherUI::captureControllerButton()
{
  if (!_capturingController)
  {
    return;
  }
  int selected = (int)SendMessage(_controllerSelect, CB_GETCURSEL, 0, 0);
  if (selected < 0)
  {
    return;
  }
  int slot = (int)SendMessage(_controllerSelect, CB_GETITEMDATA, selected, 0);
  if (slot < 0 || slot >= (int)_controllers.size())
  {
    _capturingController = false;
    _controllerCaptureMapping = -1;
    return;
  }
  XINPUT_STATE state = _controllers[slot]->GetState();
  WORD pressed = state.Gamepad.wButtons;
  WORD newlyPressed = pressed & (WORD)~_previousControllerButtons;
  _previousControllerButtons = pressed;
  if (newlyPressed == 0)
  {
    return;
  }
  DWORD value = newlyPressed & (DWORD)-(int)newlyPressed;
  char serialized[32];
  StringCchPrintfA(serialized, ARRAYSIZE(serialized), "0x%04lX", value);
  const char *mappingKey = _controllerCaptureMapping == -2 ? "CONFIG_OSK" : MOUSE_MAPPING_KEYS[_controllerCaptureMapping];
  int buttonId = _controllerCaptureMapping == -2 ? ID_OSK_MAPPING : ID_MOUSE_MAPPING_BASE + _controllerCaptureMapping;
  _config->setValue(mappingKey, serialized);
  _config->save();
  _gopher->reloadConfig();
  SetWindowTextA(GetDlgItem(_window, buttonId), xinputButtonText(value).c_str());
  _capturingController = false;
  _controllerCaptureMapping = -1;
  _previousControllerButtons = 0;
  SetFocus(_window);
}

/**
 * Applies the temporary settings values, persists them, and updates startup registration.
 * Params: none.
 * Returns: none.
 */
void GopherUI::applySettings()
{
  _config->setValue("CONFIG_REDUCE_WHEN_CLOSING", IsDlgButtonChecked(_settingsWindow, ID_SETTING_REDUCE) == BST_CHECKED ? "1" : "0");
  _config->setValue("CONFIG_START_ON_BOOT", IsDlgButtonChecked(_settingsWindow, ID_SETTING_BOOT) == BST_CHECKED ? "1" : "0");
  _config->save();
  setStartup(IsDlgButtonChecked(_settingsWindow, ID_SETTING_BOOT) == BST_CHECKED);
  DestroyWindow(_settingsWindow);
  _settingsWindow = NULL;
}

/**
 * Adds or removes the current executable from the user's startup registry key.
 * Params: enabled controls whether the startup value exists.
 * Returns: none.
 */
void GopherUI::setStartup(bool enabled)
{
  HKEY key;
  if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
  {
    return;
  }
  TCHAR path[MAX_PATH];
  GetModuleFileName(NULL, path, MAX_PATH);
  if (enabled)
  {
    RegSetValueEx(key, TEXT("Gopher360"), 0, REG_SZ, (const BYTE *)path, ((DWORD)lstrlen(path) + 1) * sizeof(TCHAR));
  }
  else
  {
    RegDeleteValue(key, TEXT("Gopher360"));
  }
  RegCloseKey(key);
}

/**
 * Hides the main window while leaving the process available from the tray.
 * Params: none.
 * Returns: none.
 */
void GopherUI::hideToTray()
{
  ShowWindow(_window, SW_HIDE);
}

/**
 * Restores the main window from the tray.
 * Params: none.
 * Returns: none.
 */
void GopherUI::showFromTray()
{
  ShowWindow(_window, SW_SHOW);
  SetForegroundWindow(_window);
}

/**
 * Removes the tray icon before process exit.
 * Params: none.
 * Returns: none.
 */
void GopherUI::destroyTray()
{
  if (_tray.cbSize != 0)
  {
    Shell_NotifyIcon(NIM_DELETE, &_tray);
  }
}

/**
 * Refreshes the controller connection indicator and runtime status text.
 * Params: none.
 * Returns: none.
 */
void GopherUI::updateStatus()
{
  int index = (int)SendMessage(_controllerSelect, CB_GETCURSEL, 0, 0);
  bool connected = false;
  if (index >= 0)
  {
    int slot = (int)SendMessage(_controllerSelect, CB_GETITEMDATA, index, 0);
    if (slot >= 0 && slot < (int)_controllers.size())
    {
      connected = _controllers[slot]->IsConnected();
    }
  }
  bool enabled = _gopher->isEnabled();
  if (connected != _lastConnected)
  {
    SetWindowText(_statusLabel, connected ? TEXT("Connected") : TEXT("No controller"));
    _lastConnected = connected;
    InvalidateRect(_statusLabel, NULL, TRUE);
  }
  (void)enabled;
}

/**
 * Creates and displays the settings subwindow.
 * Params: none.
 * Returns: none.
 */
void GopherUI::createSettingsWindow()
{
  if (_settingsWindow != NULL)
  {
    SetForegroundWindow(_settingsWindow);
    return;
  }
  HINSTANCE instance = (HINSTANCE)GetWindowLongPtr(_window, GWLP_HINSTANCE);
  _settingsWindow = CreateWindowEx(WS_EX_DLGMODALFRAME, TEXT("Gopher360Settings"), TEXT("Gopher360 settings"),
    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 0, 0, 380, 250, _window, NULL, instance, NULL);
  SetWindowLongPtr(_settingsWindow, GWLP_USERDATA, (LONG_PTR)this);
  _reduceCheck = CreateWindow(TEXT("BUTTON"), TEXT("Reduce to tray when closing"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
    28, 28, 300, 26, _settingsWindow, (HMENU)ID_SETTING_REDUCE, instance, NULL);
  _bootCheck = CreateWindow(TEXT("BUTTON"), TEXT("Start on PC boot"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
    28, 62, 300, 26, _settingsWindow, (HMENU)ID_SETTING_BOOT, instance, NULL);
  CreateWindowA("STATIC", "Github Repo:", WS_CHILD | WS_VISIBLE, 28, 100, 78, 20, _settingsWindow, (HMENU)ID_SETTINGS_REPO_LABEL, instance, NULL);
  createHyperlink(_settingsWindow, instance, ID_LINK_REPOSITORY,
    "github.com/ezzud/gopher360-enhanced", 103, 100, 245, 20);
  CreateWindowA("STATIC", "Version: " GOPHER_VERSION_DISPLAY, WS_CHILD | WS_VISIBLE,
    28, 124, 170, 20, _settingsWindow, (HMENU)ID_SETTINGS_VERSION_LABEL, instance, NULL);
  CreateWindowA("STATIC", "Original project:", WS_CHILD | WS_VISIBLE, 28, 148, 100, 20, _settingsWindow, (HMENU)ID_SETTINGS_ORIGINAL_LABEL, instance, NULL);
  createHyperlink(_settingsWindow, instance, ID_LINK_TYLEMAGNE,
    "github.com/tylemagne/gopher360", 128, 148, 220, 20);
  HWND reset = CreateWindow(TEXT("BUTTON"), TEXT("Reset defaults"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 28, 172, 110, 30, _settingsWindow, (HMENU)ID_SETTING_RESET, instance, NULL);
  HWND cancel = CreateWindow(TEXT("BUTTON"), TEXT("Cancel"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 153, 172, 80, 30, _settingsWindow, (HMENU)ID_SETTING_CANCEL, instance, NULL);
  HWND apply = CreateWindow(TEXT("BUTTON"), TEXT("Apply"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 248, 172, 80, 30, _settingsWindow, (HMENU)ID_SETTING_APPLY, instance, NULL);
  setFont(_reduceCheck, 13, false);
  setFont(_bootCheck, 13, false);
  setFont(GetDlgItem(_settingsWindow, ID_SETTINGS_REPO_LABEL), 10, false);
  setFont(GetDlgItem(_settingsWindow, ID_SETTINGS_VERSION_LABEL), 10, false);
  setFont(GetDlgItem(_settingsWindow, ID_SETTINGS_ORIGINAL_LABEL), 10, false);
  setFont(reset, 12, false);
  setFont(cancel, 12, false);
  setFont(apply, 12, true);
  refreshSettings(false);
  RECT mainRect;
  RECT settingsRect;
  GetWindowRect(_window, &mainRect);
  GetWindowRect(_settingsWindow, &settingsRect);
  int width = settingsRect.right - settingsRect.left;
  int height = settingsRect.bottom - settingsRect.top;
  int x = mainRect.left + ((mainRect.right - mainRect.left) - width) / 2;
  int y = mainRect.top + ((mainRect.bottom - mainRect.top) - height) / 2;
  SetWindowPos(_settingsWindow, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
  ShowWindow(_settingsWindow, SW_SHOW);
}

/**
 * Loads the supplied settings PNG into a device-independent bitmap for the icon button.
 * Params: none.
 * Returns: none.
 */
void GopherUI::loadSettingsBitmap()
{
  HINSTANCE instance = GetModuleHandle(NULL);
  HRSRC resource = FindResource(instance, MAKEINTRESOURCE(IDR_SETTINGS_PNG), RT_RCDATA);
  if (resource == NULL)
  {
    return;
  }
  HGLOBAL loaded = LoadResource(instance, resource);
  DWORD size = SizeofResource(instance, resource);
  if (loaded == NULL || size == 0)
  {
    return;
  }
  HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size);
  if (memory == NULL)
  {
    return;
  }
  void *destination = GlobalLock(memory);
  CopyMemory(destination, LockResource(loaded), size);
  GlobalUnlock(memory);
  if (CreateStreamOnHGlobal(memory, TRUE, &_settingsStream) != S_OK)
  {
    GlobalFree(memory);
    return;
  }
  _settingsImage = Gdiplus::Bitmap::FromStream(_settingsStream, FALSE);
  if (_settingsImage == NULL || _settingsImage->GetLastStatus() != Gdiplus::Ok)
  {
    delete _settingsImage;
    _settingsImage = NULL;
  }
}

/**
 * Handles messages for the main application window and tray icon.
 * Params: window is the target; message, wParam, and lParam are the Windows message data.
 * Returns: message result.
 */
LRESULT CALLBACK GopherUI::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  GopherUI *ui = (GopherUI *)GetWindowLongPtr(window, GWLP_USERDATA);
  if (message == WM_NCCREATE)
  {
    CREATESTRUCT *create = (CREATESTRUCT *)lParam;
    ui = (GopherUI *)create->lpCreateParams;
    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)ui);
  }
  if (ui == NULL)
  {
    return DefWindowProc(window, message, wParam, lParam);
  }
  if (message == WM_KEYDOWN)
  {
    if (ui->_capturingController && wParam == VK_ESCAPE)
    {
      ui->_capturingController = false;
      ui->_controllerCaptureMapping = -1;
      ui->_previousControllerButtons = 0;
      SetWindowText(GetDlgItem(window, ID_OSK_MAPPING), TEXT("Cancelled"));
    }
    else
    {
      ui->captureKey(wParam);
    }
    return 0;
  }
  if (message == WM_DRAWITEM)
  {
    DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)lParam;
    if ((draw->CtlID >= ID_MAPPING_BASE && draw->CtlID < ID_MAPPING_BASE + MAPPING_COUNT) ||
        (draw->CtlID >= ID_MOUSE_MAPPING_BASE && draw->CtlID < ID_MOUSE_MAPPING_BASE + MOUSE_MAPPING_COUNT) ||
        draw->CtlID == ID_OSK_MAPPING)
    {
      drawMappingButton(draw);
      return TRUE;
    }
    if (draw->CtlID == ID_SETTINGS)
    {
      HBRUSH backgroundBrush = CreateSolidBrush(BACKGROUND);
      FillRect(draw->hDC, &draw->rcItem, backgroundBrush);
      DeleteObject(backgroundBrush);
      if (ui->_settingsImage != NULL)
      {
        Gdiplus::Graphics graphics(draw->hDC);
        int width = draw->rcItem.right - draw->rcItem.left - 8;
        int height = draw->rcItem.bottom - draw->rcItem.top - 8;
        int size = min(width, height);
        int left = draw->rcItem.left + ((draw->rcItem.right - draw->rcItem.left) - width) / 2;
        int top = draw->rcItem.top + ((draw->rcItem.bottom - draw->rcItem.top) - height) / 2;
        left = draw->rcItem.left + ((draw->rcItem.right - draw->rcItem.left) - size) / 2;
        top = draw->rcItem.top + ((draw->rcItem.bottom - draw->rcItem.top) - size) / 2;
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.DrawImage(ui->_settingsImage, Gdiplus::Rect(left, top, size, size));
      }
      return TRUE;
    }
    if (draw->CtlID == ID_CONTROLLER)
    {
      HBRUSH brush = CreateSolidBrush(PANEL);
      FillRect(draw->hDC, &draw->rcItem, brush);
      DeleteObject(brush);
      char name[128] = "No controller";
      if (draw->itemID != (UINT)-1)
      {
        SendMessageA(draw->hwndItem, CB_GETLBTEXT, draw->itemID, (LPARAM)name);
      }
      else
      {
        GetWindowTextA(draw->hwndItem, name, ARRAYSIZE(name));
      }
      SetBkMode(draw->hDC, TRANSPARENT);
      SetTextColor(draw->hDC, TEXT);
      RECT textRect = draw->rcItem;
      DrawTextA(draw->hDC, name, -1, &textRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
      return TRUE;
    }
  }
  if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLORBTN || message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX)
  {
    static HBRUSH backgroundBrush = CreateSolidBrush(BACKGROUND);
    static HBRUSH panelBrush = CreateSolidBrush(PANEL);
    HDC deviceContext = (HDC)wParam;
    HWND control = (HWND)lParam;
    COLORREF color = TEXT;
    int controlId = GetDlgCtrlID(control);
    if (controlId == ID_LINK_EZZUD || controlId == ID_LINK_TYLEMAGNE || controlId == ID_LINK_REPOSITORY)
    {
      SetTextColor(deviceContext, RGB(90, 160, 225));
      SetBkMode(deviceContext, TRANSPARENT);
      return (LRESULT)backgroundBrush;
    }
    if (GetDlgCtrlID(control) == ID_STATUS)
    {
      color = ui->_lastConnected ? ENABLED : DISABLED;
    }
    else if (GetDlgCtrlID(control) == ID_MAIN_FOOTER_LABEL || GetDlgCtrlID(control) == ID_MAIN_FOOTER_ORIGINAL)
    {
      color = RGB(145, 153, 165);
    }
    SetTextColor(deviceContext, color);
    SetBkMode(deviceContext, TRANSPARENT);
    SetBkColor(deviceContext, BACKGROUND);
    return (LRESULT)(message == WM_CTLCOLORBTN ? panelBrush : backgroundBrush);
  }
  if (message == WM_NOTIFY)
  {
    NMHDR *header = (NMHDR *)lParam;
    if (header != NULL && header->code == NM_CLICK)
    {
      openLink(header->idFrom);
      return 0;
    }
  }
  if (message == WM_COMMAND)
  {
    int id = LOWORD(wParam);
    if (id == ID_LINK_EZZUD || id == ID_LINK_TYLEMAGNE || id == ID_LINK_REPOSITORY)
    {
      openLink(id);
      return 0;
    }
    if (id == ID_SETTINGS)
    {
      ui->createSettingsWindow();
    }
    else if (id == ID_CONTROLLER && HIWORD(wParam) == CBN_SELCHANGE)
    {
      int selected = (int)SendMessage(ui->_controllerSelect, CB_GETCURSEL, 0, 0);
      if (selected >= 0)
      {
        int controller = (int)SendMessage(ui->_controllerSelect, CB_GETITEMDATA, selected, 0);
        if (controller >= 0 && controller < (int)ui->_controllers.size())
        {
          ui->_gopher->setController(ui->_controllers[controller]);
        }
      }
    }
    else if (id == ID_OSK_MAPPING)
    {
      ui->_capturing = -1;
      ui->_capturingController = true;
      ui->_controllerCaptureMapping = -2;
      ui->_previousControllerButtons = 0;
      int selected = (int)SendMessage(ui->_controllerSelect, CB_GETCURSEL, 0, 0);
      if (selected >= 0)
      {
        int slot = (int)SendMessage(ui->_controllerSelect, CB_GETITEMDATA, selected, 0);
        if (slot >= 0 && slot < (int)ui->_controllers.size())
        {
          ui->_previousControllerButtons = ui->_controllers[slot]->GetState().Gamepad.wButtons;
        }
      }
      SetWindowText(GetDlgItem(window, ID_OSK_MAPPING), TEXT("Press a button"));
      SetFocus(window);
    }
    else if (id >= ID_MOUSE_MAPPING_BASE && id < ID_MOUSE_MAPPING_BASE + MOUSE_MAPPING_COUNT)
    {
      ui->_capturing = -1;
      ui->_capturingController = true;
      ui->_controllerCaptureMapping = id - ID_MOUSE_MAPPING_BASE;
      ui->_previousControllerButtons = 0;
      int selected = (int)SendMessage(ui->_controllerSelect, CB_GETCURSEL, 0, 0);
      if (selected >= 0)
      {
        int slot = (int)SendMessage(ui->_controllerSelect, CB_GETITEMDATA, selected, 0);
        if (slot >= 0 && slot < (int)ui->_controllers.size())
          ui->_previousControllerButtons = ui->_controllers[slot]->GetState().Gamepad.wButtons;
      }
      SetWindowText(GetDlgItem(window, id), TEXT("Press a button"));
      SetFocus(window);
    }
    else if (id >= ID_MAPPING_BASE && id < ID_MAPPING_BASE + MAPPING_COUNT)
    {
      ui->_capturing = id - ID_MAPPING_BASE;
      SetWindowText(GetDlgItem(window, id), TEXT("Press a key"));
      SetFocus(window);
    }
  }
  else if (message == WM_TIMER)
  {
    if (ui->_capturing >= 0 && (GetAsyncKeyState(VK_ESCAPE) & 1) != 0)
    {
      ui->captureKey(VK_ESCAPE);
    }
    ui->captureControllerButton();
    ui->updateStatus();
    ++ui->_controllerRefreshTicks;
    if (ui->_controllerRefreshTicks >= 15)
    {
      ui->_controllerRefreshTicks = 0;
      ui->refreshControllerList();
    }
  }
  else if (message == WM_CLOSE)
  {
    if (ui->_config->getValueOfKey<int>("CONFIG_REDUCE_WHEN_CLOSING", 1))
    {
      ui->hideToTray();
      ui->_tray.uFlags = NIF_INFO;
      StringCchCopy(ui->_tray.szInfoTitle, ARRAYSIZE(ui->_tray.szInfoTitle), TEXT("Gopher360"));
      StringCchCopy(ui->_tray.szInfo, ARRAYSIZE(ui->_tray.szInfo), TEXT("Gopher360 is still running in the system tray."));
      ui->_tray.dwInfoFlags = NIIF_INFO;
      Shell_NotifyIcon(NIM_MODIFY, &ui->_tray);
      ui->_tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    }
    else
    {
      DestroyWindow(window);
    }
    return 0;
  }
  else if (message == WM_DESTROY)
  {
    ui->_inputRunning = false;
    if (ui->_inputThread.joinable())
    {
      ui->_inputThread.join();
    }
    if (ui->_settingsImage != NULL)
    {
      delete ui->_settingsImage;
      ui->_settingsImage = NULL;
    }
    if (ui->_settingsStream != NULL)
    {
      ui->_settingsStream->Release();
      ui->_settingsStream = NULL;
    }
    if (ui->_gdiplusToken != 0)
    {
      Gdiplus::GdiplusShutdown(ui->_gdiplusToken);
      ui->_gdiplusToken = 0;
    }
    ui->destroyTray();
    PostQuitMessage(0);
    return 0;
  }
  else if (message == WM_APP + 1)
  {
    if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK)
    {
      ui->showFromTray();
    }
    else if (lParam == WM_RBUTTONUP)
    {
      HMENU menu = CreatePopupMenu();
      AppendMenu(menu, MF_STRING, ID_TRAY_OPEN, TEXT("Open Gopher360"));
      AppendMenu(menu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));
      POINT point;
      GetCursorPos(&point);
      SetForegroundWindow(window);
      int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, point.x, point.y, 0, window, NULL);
      DestroyMenu(menu);
      if (command == ID_TRAY_OPEN) ui->showFromTray();
      if (command == ID_TRAY_EXIT) DestroyWindow(window);
    }
  }
  else if (message == WM_APP + 2)
  {
    ui->_tray.uFlags = NIF_INFO;
    StringCchCopy(ui->_tray.szInfoTitle, ARRAYSIZE(ui->_tray.szInfoTitle), TEXT("Gopher360"));
    StringCchCopy(ui->_tray.szInfo, ARRAYSIZE(ui->_tray.szInfo), (LPCTSTR)lParam);
    ui->_tray.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIcon(NIM_MODIFY, &ui->_tray);
    ui->_tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    return 0;
  }
  return DefWindowProc(window, message, wParam, lParam);
}

/**
 * Handles messages for the settings subwindow.
 * Params: window is the target; message, wParam, and lParam are the Windows message data.
 * Returns: message result.
 */
LRESULT CALLBACK GopherUI::settingsProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  GopherUI *ui = (GopherUI *)GetWindowLongPtr(window, GWLP_USERDATA);
  if (message == WM_ERASEBKGND)
  {
    RECT client;
    GetClientRect(window, &client);
    HBRUSH brush = CreateSolidBrush(PANEL);
    FillRect((HDC)wParam, &client, brush);
    DeleteObject(brush);
    return 1;
  }
  if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLORBTN)
  {
    HDC deviceContext = (HDC)wParam;
    HWND control = (HWND)lParam;
    int controlId = GetDlgCtrlID(control);
    if (controlId == ID_LINK_EZZUD || controlId == ID_LINK_TYLEMAGNE || controlId == ID_LINK_REPOSITORY)
    {
      SetTextColor(deviceContext, RGB(90, 160, 225));
      SetBkMode(deviceContext, TRANSPARENT);
      static HBRUSH settingsLinkBrush = CreateSolidBrush(PANEL);
      return (LRESULT)settingsLinkBrush;
    }
    COLORREF color = TEXT;
    if (controlId == ID_SETTINGS_REPO_LABEL || controlId == ID_SETTINGS_VERSION_LABEL || controlId == ID_SETTINGS_ORIGINAL_LABEL)
    {
      color = RGB(155, 164, 176);
    }
    SetTextColor(deviceContext, color);
    SetBkColor(deviceContext, PANEL);
    SetBkMode(deviceContext, OPAQUE);
    static HBRUSH panelBrush = CreateSolidBrush(PANEL);
    return (LRESULT)panelBrush;
  }
  if (message == WM_NOTIFY)
  {
    NMHDR *header = (NMHDR *)lParam;
    if (header != NULL && header->code == NM_CLICK)
    {
      openLink(header->idFrom);
      return 0;
    }
  }
  if (message == WM_DRAWITEM && ui != NULL)
  {
    DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)lParam;
    if (draw->CtlID == ID_SETTING_RESET)
    {
      drawSettingsButton(draw, TEXT("Reset defaults"), false);
      return TRUE;
    }
    if (draw->CtlID == ID_SETTING_CANCEL)
    {
      drawSettingsButton(draw, TEXT("Cancel"), false);
      return TRUE;
    }
    if (draw->CtlID == ID_SETTING_APPLY)
    {
      drawSettingsButton(draw, TEXT("Apply"), true);
      return TRUE;
    }
  }
  if (message == WM_COMMAND && ui != NULL)
  {
    if (LOWORD(wParam) == ID_LINK_TYLEMAGNE || LOWORD(wParam) == ID_LINK_REPOSITORY)
    {
      openLink(LOWORD(wParam));
      return 0;
    }
    switch (LOWORD(wParam))
    {
      case ID_SETTING_APPLY: ui->applySettings(); return 0;
      case ID_SETTING_CANCEL: DestroyWindow(window); ui->_settingsWindow = NULL; return 0;
      case ID_SETTING_RESET: ui->refreshSettings(true); return 0;
    }
  }
  if (message == WM_CLOSE && ui != NULL)
  {
    DestroyWindow(window);
    ui->_settingsWindow = NULL;
    return 0;
  }
  return DefWindowProc(window, message, wParam, lParam);
}

/**
 * Handles unused mapping button subclass messages.
 * Params: window is the target; message, wParam, and lParam are the Windows message data.
 * Returns: message result.
 */
LRESULT CALLBACK GopherUI::mappingProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  return DefWindowProc(window, message, wParam, lParam);
}

/**
 * Runs the UI message loop and controller polling timer.
 * Params: instance is the process module handle.
 * Returns: process exit code.
 */
int GopherUI::run(HINSTANCE instance)
{
  INITCOMMONCONTROLSEX controls;
  controls.dwSize = sizeof(controls);
  controls.dwICC = ICC_LINK_CLASS;
  InitCommonControlsEx(&controls);
  Gdiplus::GdiplusStartupInput gdiplusInput;
  Gdiplus::GdiplusStartup(&_gdiplusToken, &gdiplusInput, NULL);
  WNDCLASS settingsClass;
  ZeroMemory(&settingsClass, sizeof(settingsClass));
  settingsClass.hInstance = instance;
  settingsClass.lpfnWndProc = settingsProc;
  settingsClass.lpszClassName = TEXT("Gopher360Settings");
  settingsClass.hbrBackground = CreateSolidBrush(PANEL);
  RegisterClass(&settingsClass);
  loadSettingsBitmap();
  createMainWindow(instance);
  _gopher->setNotificationWindow(_window);
  _tray.cbSize = sizeof(_tray);
  _tray.hWnd = _window;
  _tray.uID = 1;
  _tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  _tray.uCallbackMessage = WM_APP + 1;
  _tray.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
  StringCchCopy(_tray.szTip, ARRAYSIZE(_tray.szTip), TEXT("Gopher360"));
  Shell_NotifyIcon(NIM_ADD, &_tray);
  timeBeginPeriod(1);
  _inputRunning = true;
  _inputThread = std::thread(&GopherUI::inputLoop, this);
  SetTimer(_window, 1, 16, NULL);
  MSG message;
  while (GetMessage(&message, NULL, 0, 0) > 0)
  {
    TranslateMessage(&message);
    DispatchMessage(&message);
  }
  timeEndPeriod(1);
  return (int)message.wParam;
}

/**
 * Runs the original high-frequency controller loop independently from UI painting.
 * Params: none.
 * Returns: none.
 */
void GopherUI::inputLoop()
{
  while (_inputRunning)
  {
    _gopher->loop();
  }
}
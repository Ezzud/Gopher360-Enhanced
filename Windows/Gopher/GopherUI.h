#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

class Gopher;
class CXBOXController;
class ConfigFile;

class GopherUI
{
private:
  Gopher *_gopher;
  ConfigFile *_config;
  std::vector<CXBOXController *> _controllers;
  HWND _window;
  HWND _controllerSelect;
  HWND _statusLabel;
  HWND _enabledLabel;
  HWND _enabledButton;
  HWND _settingsWindow;
  HWND _reduceCheck;
  HWND _bootCheck;
  int _capturing;
  bool _settingsDefaults;
  bool _lastConnected;
  bool _lastEnabled;
  bool _capturingController;
  int _controllerCaptureMapping;
  WORD _previousControllerButtons;
  std::vector<int> _connectedSlots;
  unsigned int _controllerRefreshTicks;
  Gdiplus::Bitmap *_settingsImage;
  IStream *_settingsStream;
  ULONG_PTR _gdiplusToken;
  std::thread _inputThread;
  std::atomic<bool> _inputRunning;
  NOTIFYICONDATA _tray;

  static GopherUI *_instance;

  static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK settingsProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK mappingProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
  void createMainWindow(HINSTANCE instance);
  void createSettingsWindow();
  void createMappingRows();
  void refreshControllerList();
  void refreshSettings(bool defaults);
  void captureKey(WPARAM key);
  void captureControllerButton();
  void applySettings();
  void setStartup(bool enabled);
  void hideToTray();
  void showFromTray();
  void destroyTray();
  void updateStatus();
  void loadSettingsBitmap();
  void inputLoop();

public:
  GopherUI(Gopher *gopher, ConfigFile *config, const std::vector<CXBOXController *> &controllers);

  /**
   * Runs the UI message loop and controller polling timer.
   * Params: instance is the process module handle.
   * Returns: process exit code.
   */
  int run(HINSTANCE instance);
};
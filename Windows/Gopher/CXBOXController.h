#pragma once

#include <windows.h>
#include <xinput.h>
#include <string>

class CXBOXController
{
private:
  XINPUT_STATE _controllerState;
  int _controllerNum;
public:
  CXBOXController(int playerNumber);
  XINPUT_STATE GetState();
  bool IsConnected();
  void Vibrate(int leftVal, int rightVal);

  /**
   * Returns the XInput slot represented by this controller.
   * Params: none.
   * Returns: zero-based XInput slot.
   */
  int GetControllerNumber() const;

  /**
   * Returns a stable display label for this XInput slot.
   * Params: none.
   * Returns: controller label for the user interface.
   */
  std::string GetDisplayName() const;
};

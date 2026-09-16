#include "CXBOXController.h"

CXBOXController::CXBOXController(int playerNumber)
{
  _controllerNum = playerNumber - 1; //set number
}

XINPUT_STATE CXBOXController::GetState()
{
  ZeroMemory(&this->_controllerState, sizeof(XINPUT_STATE));
  XInputGetState(_controllerNum, &this->_controllerState);
  return _controllerState;
}

bool CXBOXController::IsConnected()
{
  ZeroMemory(&this->_controllerState, sizeof(XINPUT_STATE));
  DWORD Result = XInputGetState(_controllerNum, &this->_controllerState);

  return (Result == ERROR_SUCCESS);
}

void CXBOXController::Vibrate(int leftVal, int rightVal)
{
  // Create a Vibraton State
  XINPUT_VIBRATION Vibration;

  // Zeroise the Vibration
  ZeroMemory(&Vibration, sizeof(XINPUT_VIBRATION));

  // Set the Vibration Values
  Vibration.wLeftMotorSpeed = leftVal;
  Vibration.wRightMotorSpeed = rightVal;

  // Vibrate the controller
  XInputSetState(_controllerNum, &Vibration);
}

/**
 * Returns the XInput slot represented by this controller.
 * Params: none.
 * Returns: zero-based XInput slot.
 */
int CXBOXController::GetControllerNumber() const
{
  return _controllerNum;
}

/**
 * Returns a stable display label for this XInput slot.
 * Params: none.
 * Returns: controller label for the user interface.
 */
std::string CXBOXController::GetDisplayName() const
{
  XINPUT_CAPABILITIES capabilities;
  ZeroMemory(&capabilities, sizeof(capabilities));
  if (XInputGetCapabilities(_controllerNum, XINPUT_FLAG_GAMEPAD, &capabilities) != ERROR_SUCCESS)
  {
    return "Controller " + std::to_string(_controllerNum + 1);
  }

  return "Gamepad " + std::to_string(_controllerNum + 1);
}
#include "ConfigFile.h"
#include <iostream>
#include <fstream> 
#include <windows.h>

namespace
{
  const char *DEFAULT_CONFIG =
    "# Gopher360 configuration\n"
    "# Controller shortcuts use XInput button masks. Set 0 for no function.\n"
    "# Mouse clicks and visual keyboard are also assigned to XInput buttons.\n"
    "# Keyboard values use Windows virtual-key codes.\n"
    "\n"
    "# Mouse and application controls\n"
    "CONFIG_MOUSE_LEFT = 0x1000\t# Left mouse button\n"
    "CONFIG_MOUSE_RIGHT = 0x4000\t# Right mouse button\n"
    "CONFIG_MOUSE_MIDDLE = 0x0040\t# Middle mouse button\n"
    "CONFIG_HIDE = 0x8000\t\t# Hide the application\n"
    "CONFIG_DISABLE = 0x0030\t\t# Enable or disable mappings\n"
    "CONFIG_DISABLE_VIBRATION = 0x0011\t# Disable vibration\n"
    "CONFIG_SPEED_CHANGE = 0x0300\t# Change cursor speed\n"
    "CONFIG_OSK = 0x0020\t# Show visual keyboard (Select)\n"
    "\n"
    "# Keyboard mappings\n"
    "GAMEPAD_DPAD_UP = 0x26\n"
    "GAMEPAD_DPAD_DOWN = 0x28\n"
    "GAMEPAD_DPAD_LEFT = 0x25\n"
    "GAMEPAD_DPAD_RIGHT = 0x27\n"
    "GAMEPAD_START = 0x5B\n"
    "GAMEPAD_BACK = 0xA8\n"
    "GAMEPAD_LEFT_THUMB = 0\n"
    "GAMEPAD_RIGHT_THUMB = 0x71\n"
    "GAMEPAD_LEFT_SHOULDER = 0xA6\n"
    "GAMEPAD_RIGHT_SHOULDER = 0xA7\n"
    "GAMEPAD_A = 0\n"
    "GAMEPAD_B = 0x0D\n"
    "GAMEPAD_X = 0\n"
    "GAMEPAD_Y = 0\n"
    "GAMEPAD_TRIGGER_LEFT = 0x20\n"
    "GAMEPAD_TRIGGER_RIGHT = 0x08\n"
    "\n"
    "# Application settings\n"
    "CONFIG_REDUCE_WHEN_CLOSING = 1\t# Minimize to the tray when closing\n"
    "CONFIG_START_ON_BOOT = 0\t# Start with Windows\n"
    "\n"
    "# Advanced settings\n"
    "CURSOR_SPEED = ULTRALOW=0.015,LOW=0.03,MED=0.04,HIGH=0.06\n"
    "ACCELERATION_FACTOR = 0\n"
    "DEAD_ZONE = 6000\n"
    "SCROLL_DEAD_ZONE = 5000\n"
    "SCROLL_SPEED = 0.1\n"
    "SWAP_THUMBSTICKS = 0\n";

  void writeValue(std::ofstream &file, const ConfigFile &config, const char *key, const char *fallback)
  {
    std::string value = config.getValueOfKey<std::string>(key, fallback);
    if ((value.find("0x") == 0 || value.find("0X") == 0) && strtoul(value.c_str(), NULL, 0) == 0)
    {
      value = "0";
    }
    file << key << " = " << value << "\n";
  }
}

void ConfigFile::removeComment(std::string &line) const
{
  if (line.find('#') != line.npos)
  {
    line.erase(line.find('#'));
  }
}

bool ConfigFile::onlyWhitespace(const std::string &line) const
{
  return (line.find_first_not_of(' ') == line.npos);
}

bool ConfigFile::validLine(const std::string &line) const
{
  std::string temp = line;
  temp.erase(0, temp.find_first_not_of("\t "));
  if (temp[0] == '=')
  {
    return false;
  }

  for (size_t i = temp.find('=') + 1; i < temp.length(); i++)
  {
    if (temp[i] != ' ')
    {
      return true;
    }
  }

  return false;
}

void ConfigFile::extractKey(std::string &key, size_t const &sepPos, const std::string &line) const
{
  key = line.substr(0, sepPos);
  if (key.find('\t') != line.npos || key.find(' ') != line.npos)
  {
    key.erase(key.find_first_of("\t "));
  }
}
void ConfigFile::extractValue(std::string &value, size_t const &sepPos, const std::string &line) const
{
  value = line.substr(sepPos + 1);
  value.erase(0, value.find_first_not_of("\t "));
  value.erase(value.find_last_not_of("\t ") + 1);
}

void ConfigFile::extractContents(const std::string &line)
{
  std::string temp = line;
  temp.erase(0, temp.find_first_not_of("\t "));
  size_t sepPos = temp.find('=');

  std::string key, value;
  extractKey(key, sepPos, temp);
  extractValue(value, sepPos, temp);

  if (!keyExists(key))
  {
    contents.insert(std::pair<std::string, std::string>(key, value));
  }
  else
  {
    exitWithError("CFG: Can only have unique key names!\n");
  }
}

void ConfigFile::parseLine(const std::string &line, size_t const lineNo)
{
  if (line.find('=') == line.npos)
  {
    exitWithError("CFG: Couldn't find separator on line: " + Convert::T_to_string(lineNo) + "\n");
  }

  if (!validLine(line))
  {
    exitWithError("CFG: Bad format for line: " + Convert::T_to_string(lineNo) + "\n");
  }

  extractContents(line);
}

void ConfigFile::ExtractKeys()
{
  std::ifstream file;
  file.open(fName.c_str());

  if (!file)
  {
    printf("%s not found! Building a fresh one... ", fName.c_str());

    std::ofstream outfile(fName.c_str());

    // Begin config dump to file
      outfile << DEFAULT_CONFIG;
    // End config dump

    outfile.close();
    
    file.open(fName.c_str());

    if (!file)
    {

      exitWithError("\nERROR! Configuration file " + fName + " still couldn't be found!\n");
    }
    else
    {

      printf("Success!\nNow using %s.\n", fName.c_str());
    }
  }

  //exitWithError("\nSafety exit!\n");

  std::string line;
  size_t lineNo = 0;
  while (std::getline(file, line))
  {
    lineNo++;
    std::string temp = line;

    if (temp.empty())
    {
      continue;
    }

    removeComment(temp);
    if (onlyWhitespace(temp))
    {
      continue;
    }

    parseLine(temp, lineNo);
  }

  file.close();
}

ConfigFile::ConfigFile(const std::string &fName)
{
  this->fName = fName;
  reload();
}

bool ConfigFile::reload()
{
  contents.clear();
  ExtractKeys();
  return true;
}

void ConfigFile::setValue(const std::string &key, const std::string &value)
{
  contents[key] = value;
}

bool ConfigFile::save() const
{
  std::ofstream file(fName.c_str(), std::ios::trunc);
  if (!file)
  {
    return false;
  }

  file << "# Gopher360 configuration\n";
  file << "# Controller shortcuts use XInput button masks. Set 0 for no function.\n";
  file << "# Mouse clicks and visual keyboard are also assigned to XInput buttons.\n";
  file << "# Keyboard values use Windows virtual-key codes.\n\n";
  file << "# Mouse and application controls\n";
  writeValue(file, *this, "CONFIG_MOUSE_LEFT", "0x1000");
  writeValue(file, *this, "CONFIG_MOUSE_RIGHT", "0x4000");
  writeValue(file, *this, "CONFIG_MOUSE_MIDDLE", "0x0040");
  writeValue(file, *this, "CONFIG_HIDE", "0x8000");
  writeValue(file, *this, "CONFIG_DISABLE", "0x0030");
  writeValue(file, *this, "CONFIG_DISABLE_VIBRATION", "0x0011");
  writeValue(file, *this, "CONFIG_SPEED_CHANGE", "0x0300");
  writeValue(file, *this, "CONFIG_OSK", "0x0020");
  file << "\n# Keyboard mappings\n";
  writeValue(file, *this, "GAMEPAD_DPAD_UP", "0x26");
  writeValue(file, *this, "GAMEPAD_DPAD_DOWN", "0x28");
  writeValue(file, *this, "GAMEPAD_DPAD_LEFT", "0x25");
  writeValue(file, *this, "GAMEPAD_DPAD_RIGHT", "0x27");
  writeValue(file, *this, "GAMEPAD_START", "0x5B");
  writeValue(file, *this, "GAMEPAD_BACK", "0xA8");
  writeValue(file, *this, "GAMEPAD_LEFT_THUMB", "0");
  writeValue(file, *this, "GAMEPAD_RIGHT_THUMB", "0x71");
  writeValue(file, *this, "GAMEPAD_LEFT_SHOULDER", "0xA6");
  writeValue(file, *this, "GAMEPAD_RIGHT_SHOULDER", "0xA7");
  writeValue(file, *this, "GAMEPAD_A", "0");
  writeValue(file, *this, "GAMEPAD_B", "0x0D");
  writeValue(file, *this, "GAMEPAD_X", "0");
  writeValue(file, *this, "GAMEPAD_Y", "0");
  writeValue(file, *this, "GAMEPAD_TRIGGER_LEFT", "0x20");
  writeValue(file, *this, "GAMEPAD_TRIGGER_RIGHT", "0x08");
  file << "\n# Application settings\n";
  writeValue(file, *this, "CONFIG_REDUCE_WHEN_CLOSING", "1");
  writeValue(file, *this, "CONFIG_START_ON_BOOT", "0");
  file << "\n# Advanced settings\n";
  writeValue(file, *this, "CURSOR_SPEED", "ULTRALOW=0.015,LOW=0.03,MED=0.04,HIGH=0.06");
  writeValue(file, *this, "ACCELERATION_FACTOR", "0");
  writeValue(file, *this, "DEAD_ZONE", "6000");
  writeValue(file, *this, "SCROLL_DEAD_ZONE", "5000");
  writeValue(file, *this, "SCROLL_SPEED", "0.1");
  writeValue(file, *this, "SWAP_THUMBSTICKS", "0");
  return file.good();
}

void ConfigFile::resetDefaults()
{
  contents.clear();
  std::istringstream defaults(DEFAULT_CONFIG);
  std::string line;
  size_t lineNo = 0;
  while (std::getline(defaults, line))
  {
    ++lineNo;
    std::string parsedLine = line;
    removeComment(parsedLine);
    if (!parsedLine.empty() && !onlyWhitespace(parsedLine))
    {
      parseLine(parsedLine, lineNo);
    }
  }
}

std::vector<std::string> ConfigFile::keys() const
{
  std::vector<std::string> result;
  for (std::map<std::string, std::string>::const_iterator it = contents.begin(); it != contents.end(); ++it)
  {
    result.push_back(it->first);
  }
  return result;
}

const std::string &ConfigFile::fileName() const
{
  return fName;
}

bool ConfigFile::keyExists(const std::string &key) const
{
  return contents.find(key) != contents.end();
}

void ConfigFile::exitWithError(const std::string &error)
{
  std::cout << error;
  std::cin.ignore();
  std::cin.get();

  exit(EXIT_FAILURE);
}

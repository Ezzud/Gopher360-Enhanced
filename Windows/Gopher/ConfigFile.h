#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <vector>
#include "Convert.h"

class ConfigFile
{
private:
  std::map<std::string, std::string> contents;
  std::string fName;

  void removeComment(std::string &line) const;

  bool onlyWhitespace(const std::string &line) const;
  bool validLine(const std::string &line) const;

  void extractKey(std::string &key, size_t const &sepPos, const std::string &line) const;
  void extractValue(std::string &value, size_t const &sepPos, const std::string &line) const;

  void extractContents(const std::string &line);

  void parseLine(const std::string &line, size_t const lineNo);

  void ExtractKeys();

public:
  ConfigFile(const std::string &fName);

  bool keyExists(const std::string &key) const;

  template <typename ValueType>
  ValueType getValueOfKey(const std::string &key, ValueType const &defaultValue = ValueType()) const
  {
    if (!keyExists(key))
      return defaultValue;

    return Convert::string_to_T<ValueType>(contents.find(key)->second);
  };

  /**
   * Reloads the configuration file, creating it from the built-in defaults when needed.
   * Params: none.
   * Returns: true when the configuration was loaded successfully.
   */
  bool reload();

  /**
   * Sets or replaces a configuration value in memory.
   * Params: key is the configuration key; value is its serialized value.
   * Returns: none.
   */
  void setValue(const std::string &key, const std::string &value);

  /**
   * Writes the current configuration values to disk.
   * Params: none.
   * Returns: true when the file was written successfully.
   */
  bool save() const;

  /**
   * Restores the built-in defaults without writing them to disk.
   * Params: none.
   * Returns: none.
   */
  void resetDefaults();

  /**
   * Returns all stored configuration keys.
   * Params: none.
   * Returns: a vector containing the stored key names.
   */
  std::vector<std::string> keys() const;

  /**
   * Returns the path used by this configuration object.
   * Params: none.
   * Returns: the configuration path.
   */
  const std::string &fileName() const;

  void exitWithError(const std::string &error);
};
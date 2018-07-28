#include <ArduinoSTL.h>

#include <Arduino.h>

// split
namespace str_util {
  // assumptions:
  // input string has at least one character, that character is not the delimiting character
  void split(String str, char delim, std::vector<String>& tokens) {
    tokens.clear(); // remove existing contents if there is any
    if(str.length() > 0) {
      str += delim; // a hack such that the last section is captured
      uint16_t prev_index = 0;
      for(uint16_t index = 1; index < str.length(); index++) {
        if(str.charAt(index) == delim) {
          if(index != prev_index) {
            tokens.push_back(str.substring(prev_index, index));
          }
          prev_index = index + 1;
        }
      }
    }
  }
}


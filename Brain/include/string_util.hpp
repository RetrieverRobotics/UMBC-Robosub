#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <string>
#include <regex>
#include <vector>

namespace string_util {
	void splitOnChar(const std::string&, char, std::vector<std::string>&);
	std::string removeWhitespace(const std::string&);
}

#endif
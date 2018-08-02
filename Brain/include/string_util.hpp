#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <string>
#include <regex>
#include <vector>

/*
This file defines extra string helper functions.
*/

namespace string_util {
	void splitOnChar(const std::string&, char, std::vector<std::string>&);
	std::string removeWhitespace(const std::string&);
	std::string trim(const std::string&);
}

#endif
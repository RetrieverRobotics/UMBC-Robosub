#include "string_util.hpp"

void string_util::splitOnChar(const std::string& s, char delim, std::vector<std::string>& v) {
    std::size_t pos = s.find(delim);
    if(pos == std::string::npos) {
    	v.push_back(s);
    } else {
    	int i = 0;
	    while (pos != std::string::npos) {
	      v.push_back(s.substr(i, pos-i));
	      i = ++pos;
	      pos = s.find(delim, pos);

	      if (pos == std::string::npos)
	         v.push_back(s.substr(i, s.length()));
	    }
	}
}

std::string string_util::removeWhitespace(const std::string& s) {
	return std::regex_replace( s, std::regex("\\s+"), "" );
}

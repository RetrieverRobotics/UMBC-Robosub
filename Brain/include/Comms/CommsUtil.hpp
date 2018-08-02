#ifndef COMMS_UTIL_H
#define COMMS_UTIL_H

#include <string>
#include <chrono>
#include <unordered_map>

/*
This is a helper file that provides a namespace in which to store various
internal types for Comms and typedefs. This file exists to avoid circular
include issues.
*/

class Comms;

namespace comms_util {
	enum class Hint {
		Bool,
		Int, IntVector,
		Double, DoubleVector,
		String,
		Other
	};
	// https://stackoverflow.com/questions/3559412/how-to-store-different-data-types-in-one-list-c
	class DataTS {
	public:
		DataTS() {}
		explicit DataTS(Hint _type_hint) : type_hint(_type_hint), timestamp(std::chrono::steady_clock::now()) {}
		virtual ~DataTS() {}

		const std::chrono::steady_clock::time_point& getTimePoint() { return timestamp; }
		const Hint& getTypeHint() { return type_hint; }

	private:
		Hint type_hint;
		std::chrono::steady_clock::time_point timestamp;
	};

	template<typename T>
	class TypedDataTS : public DataTS {
	public:
		explicit TypedDataTS(Hint _type_hint, T _val) : val(_val), DataTS(_type_hint) {}

		const T& getContents(void) { return val; }
	private:
		TypedDataTS() {} // to prevent calling of default constructor
		T val;
	};

	typedef std::unordered_map<std::string, std::shared_ptr<DataTS>> inner_map_t;
}

#endif
#ifndef COMMS_UTIL_ACCESSOR_H
#define COMMS_UTIL_ACCESSOR_H

#include <mutex>
#include <string>
#include <memory>

#include "CommsUtil.hpp"

/*
This file solves a very specific problem. Comms maintains an unordered_map of 'CommsLink's and when necessary calls the send
function with the appropriate arguments. In order to call that function, it must include 'CommsLink's header file.
Comms also maintains an unordered_map as a data buffer for each CommsLink. When Comms::send() is called, new data is stored
in that buffer before send() is called on the link. However, when a link receives data, it must somehow get that data back into
the buffer stored by the Comms instance. This is problematic as CommsLink cannot include the header file for Comms - that results
in a circular dependency. Additionally, b/c the set<T>() function of Comms is templated, implementation for any similar function
must be in the header file. To write an alternate set<T>() function requires some idea of where the data will end up and this
necessarily requires including the Comms header _in the header_ where the new set<T>() function resides. Of course this leads
back to circular dependencies when CommsLink must include the new header and it finds an #include for Comms. A mere forward declaration
does not work, because in this version of the structure, Comms needs both a reference to a CommsLink instance and member access, and the
same for CommsLink - reference to the Comms instance and a member function as a callback. Moving on.

I've tried several methods including pointers to member functions, generic lambdas, and other ways of storing
function objects, but that yielded even more issues. Lambdas cannot be templated, except in c++14 with auto, and even then, a function
cannot deduce the type of its argument, so even though one can write auto f = []() {} (lambda syntax), one cannot easily pass f
anywhere except into a carefully declared std::function wrapper. That version happily compiled, happily ran, and happily segfaulted.
'auto' cannot be used with a function argument (in C++11 at least). And several more which I will edit in if I remember them.

So in the end I realized that instead of trying to get the entire set<T>() function to be accessible, I really just needed the mutex
being used to guard the unordered_map writes (that being the part that is not thread-safe). To that end I again investigated lambdas
returning the current implementation of the getOpMutexFrom() function, except that are instantiated from within Comms and tried to use
the 'this' pointer. I don't remember the error for that one, but there was one, and so I moved on.

And here we stand. A separate file, a non-templated function, a mere forward declaration with no attempt at member access,
and Comms.hpp included in an implementation file. Yay! Took me about 12 hours to get this far and I probably still need to substitute
unique_lock for the raw pointer... but I'll do that later. -_- It might even be thread-safe (but I wouldn't bet on it yet :)
*/

class Comms;

namespace comms_util {
	std::mutex* getOpMutexFrom(Comms& comms, const std::string& link_id, std::shared_ptr<comms_util::inner_map_t> asserted_map);
}

#endif

/*
http://deathbytape.com/articles/2015/02/03/cpp-threading.html
https://eli.thegreenplace.net/2016/the-promises-and-challenges-of-stdasync-task-based-parallelism-in-c11/

future - asynchronous return object - wait for shared state to be ready and return it
	from async / packaged_task::get_future / promise::get_future
	"valid", "shared" state
	get() - blocks until shared state is ready and returns, wait() - merely blocks, wait_until(a_point_in_time), wait_for(timeout)
	share() - returns shared state (to shared_future) and invalidates shared_state in future
shared_future
promise - asynchronous provider - sets value of shared state
	get_future
packaged_task - wraps callable object (function pointer, functor, etc)
	similar purpose to promise in that it sets value of shared state through its return 
	get_future
async
	takes function, has future which gains return of function
	multiple launch protocols: [async = spawn thread] [deferred = run function when get is called (assumedly in current thread)]
	limited control available
	up to 14 times more efficient than std::thread
	may (does?) use threadpool
thread
	takes callable object, similar to async
	joinable(), join(), detach()
	RAII wrapper is useful to force join/detach on out-of-scope
	does not provide any direct means to get data back from spawned thread (no return), meaning you'll have to use mutexes to make access/writes thread-safe
mutex
unique_lock
shared_lock
lock_guard - RAII for mutex, scoped

packaged_task or async is probably cleanest. Thread Pool implementations appear to use packaged_task b/c they can be assigned to run in specific threads
Note that universal references can make the syntax for declaring them look pretty ugly.

std::async does not define any way to keep threads loaded, however for some reason it is more efficient???
async allows easy returns, does not prevent passing data pointers in arguments for access during thread run

Still have to make sure that global utility classes are thread safe for any public access or send method
*/

#include <iostream>
#include <vector>
#include <string>
#include <thread> // { thread }
#include <chrono>
#include <mutex> // { mutex, timed_mutex, recursive_mutex, recursive_timed_mutex, lock_guard, unique_lock, scoped_lock, once_flag }
#include <atomic> // { atomic, atomic_flag, memory_order }
#include <future> // { promise, packaged_task, future, shared_future, launch, future_status, future_error... } (... async ...)
#include <functional>

class ThreadRAII { // Resource Acquisition is Initialization
public:
	explicit ThreadRAII(std::thread& _m_thread) : m_thread(_m_thread) {}
	~ThreadRAII() {
		if(m_thread.joinable()) m_thread.join();
	}
private:
	std::thread& m_thread;
};

void dummy(int i) {
	std::cout << "Function 'dummy' says " << i << std::endl;
}

int doLotsOfWork(int val) {
	long big_val = val;
	for(int i = 0; i < 11; i++) {
		big_val *= 2;
	}
	for(int i = 0; i < 10; i++) {
		big_val /= 2;
	}
	return (int)big_val;
}

bool isPrime(int x) {
  for (int i=2; i<x; ++i) if (x%i==0) return false;
  return true;
}

void printInt(std::future<int>& fut) {
	int x = fut.get(); // when run as thread, blocks until promise sets value of future
	std::cout << "value: " << x << std::endl;
}

class Functor {
public:
	explicit Functor(int _i) : i(_i) {}

	bool operator()() {
		std::cout << "Functor says " << i << std::endl;
		return true;
	}
private:
	int i;
};


int main(int argc, char** argv) {
	using namespace std;
	/* // this section is designed to cause the main thread to terminate b/c thread1 is intentionally not joined or detached
	cout << "This section will crash the program as the thread is not joined or detached." << endl;
	thread thread1(dummy, 1);

	this_thread::sleep_for(chrono::milliseconds(100)); // waits long enough to allow thread1 to print before failure to join/detach terminates main thread :P
	*/

	//
	{
		cout << "This uses a RAII wrapper to call join when the wrapper destructs, which in this case, is immediately because of the block scope." << endl;
		thread thread2(dummy, 2);
		ThreadRAII thread2_wrapper(thread2); // calls destructor on thread reference when wrapper goes out of scope
		// however I cheated by making a block scoped around it so it goes out of scope immediately, basically calls _thread_.join() right here
	}

	//
	{
		cout << "Similarly, this uses a vector of RAII wrappers and the threads are instantiated in the loop." << endl;
		vector<ThreadRAII> wrappers;
		for(int i = 0; i < 3; ++i) {
			wrappers.push_back( ThreadRAII( *(new thread(dummy, 10 + i)) ) );
		}
	}

	{
		cout << "Now we'll try to use futures." << endl;
		cout << "Starting";
		std::future<bool> fut = std::async(std::launch::async, isPrime, 444444443);

		std::chrono::milliseconds span(50);
		while(fut.wait_for(span) == std::future_status::timeout) {
			cout << ".";
		}

		bool prime = fut.get();

		cout << " " << (prime ? "prime" : "not prime") << endl;
	}

	{
		std::promise<int> prom;
		std::future<int> fut = prom.get_future();
		std::thread th(printInt, std::ref(fut));
		prom.set_value(10);

		th.join();
	}

	{
		Functor f(3);

		std::future<bool> fut = std::async(std::launch::async, f);
		bool ret = fut.get();
		cout << "Functor returns " << (ret ? "true" : "false") << std::endl;
	}

	{
		struct ThreadHouse {
			explicit ThreadHouse(std::thread _t) : t(std::move(_t)) {}
			std::thread t;
		};
		struct ThreadHouseRef {
			explicit ThreadHouseRef(std::thread& _rt) : t(std::move(_rt)) {}
			std::thread t;
		};

		Functor f(5), g(6), h(7); // return type must be known to instantiate futures

		std::thread t1(f);

		ThreadHouse house1(std::move(t1));
		house1.t.detach();

		std::thread t2(g);

		ThreadHouseRef house2(t2);
		house2.t.detach();

		//t2.detach(); // causes crash, as even with reference, t2 was moved to house2
		
	}

	return 0;
}
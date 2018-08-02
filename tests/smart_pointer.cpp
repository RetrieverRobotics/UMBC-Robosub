#include <iostream>
#include <unordered_map>
#include <memory>

int main(int argc, char** argv) {
	{
		std::unique_ptr<int> up_i(new int(3));
		std::cout << *up_i << std::endl;
		// dereferencing the smart pointer returns the address of the actual pointer
		std::cout << &up_i << std::endl;

		int* p_i = up_i.release();
		std::cout << &p_i << std::endl;
	}

	{
		int* p_i = new int(5);
		std::unique_ptr<int> up_i(p_i);
		std::cout << *up_i << std::endl;
		// bad usage of unique_ptr, this will attempt to free p_i a second time
		//std::unique_ptr<int> up_i2(p_i);
		//std::cout << *up_i2 << std::endl;
	}

	{
		int* p_i = new int(7);
		std::shared_ptr<int> sp_i(p_i);
		std::cout << *sp_i << std::endl;
		// This is still a bad idea b/c the shared_ptr can't track references
		// unless it goes through a constructor of some sort
		//std::shared_ptr<int> sp_i2(p_i);
		//std::cout << *sp_i2 << std::endl;
	}

	{
		std::shared_ptr<int> sp_i = std::make_shared<int>(9); // instantiation
		std::cout << *sp_i << std::endl;
		std::shared_ptr<int> sp_i2 = sp_i; // assignment
		std::cout << *sp_i2 << std::endl;
		std::shared_ptr<int> sp_i3(sp_i2); // copy
		std::cout << *sp_i3 << std::endl;
	}
	
	{
		class Base {
		public:
			Base() {
				std::cout << "Base constructor called" << std::endl;
			}
			virtual ~Base() {
				std::cout << "Base destructor called" << std::endl;
			}
		};
		class Derived : public Base {
		public:
			Derived(int _val) : val(_val), Base() {
				std::cout << "Derived constructor called" << std::endl;
			}
			~Derived() {
				std::cout << "Derived destructor called" << std::endl;
			}
			int getVal() { return val; }
		private:
			int val;
		};

		std::unordered_map<std::string, std::shared_ptr<Base>> uo_map;
		uo_map.emplace("eleven", std::make_shared<Derived>(11));
		std::shared_ptr<Base> sp_b = uo_map.at("eleven");
		std::shared_ptr<Derived> sp_d = std::dynamic_pointer_cast<Derived>(uo_map.at("eleven"));

		// std::cout << sp_b->getVal() << std::endl; // won't work base call doesn't have getVal();
		std::cout << sp_d->getVal() << std::endl;

		std::cout << "Leaving scope... " << std::endl;;
	}
	std::cout << "Out of scope." << std::endl;

	return 0;
}
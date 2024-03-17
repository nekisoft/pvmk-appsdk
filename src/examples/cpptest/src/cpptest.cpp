//cpptest.cpp
//Test of C++ static runtime for Neki32
//Bryan E. Topp <betopp@betopp.com> 2024

#include <iostream>

//PVMK system calls library
#include <sc.h>

class example_exception : public std::exception
{
public:
	virtual const char *what() const noexcept override
	{
		return "example_exception";
	}
};

int main(void)
{
	printf("Hello from C!\n");
	fflush(stdout);
	
	std::cout << "Hello from C++!" << std::endl;
	std::flush(std::cout);
	try
	{
		std::cout << "Uh oh, I think I'm gonna be sick..." << std::endl;
		std::flush(std::cout);
		throw example_exception();
	}
	catch(std::exception &e)
	{
		std::cout << "This is the catch block for: " << e.what() << std::endl;
		std::flush(std::cout);
	}
	
	printf("This concludes the usage of C++.\nPlease see a doctor for any lingering nausea.\n");
	fflush(stdout);
	
	//No one is invalidated, but no one is right. The world is being engulfed in "truth".
	while(1) { _sc_pause(); }
	return 0;
}

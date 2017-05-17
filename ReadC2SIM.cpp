#include <iostream>
#include <condition_variable>
#include <thread>



class ReadC2SIM
{
	ReadC2SIM()
	{
		std::cout << "This constructor is called";
	}
	
	void callread(void)
	{
		std::cout << "This function is called";
	}
};
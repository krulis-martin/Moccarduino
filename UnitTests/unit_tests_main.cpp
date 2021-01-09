#include "test.hpp"

#include <map>
#include <string>
#include <iostream>

using tests_t = const std::map<std::string, const MoccarduinoTest*>;

/**
 * Check that given test name is on the argument list.
 * More precisely, whether one of the given keywords (in arguments) is a substring of the name.
 */
bool on_list(const std::string& name, char* argv[])
{
	if (*argv == nullptr) return true;	// list is empty, let's accept everything ...
	while (*argv != nullptr && name.find(*argv) == std::string::npos) {
		++argv;
	}
	return (*argv != nullptr);	// not null => match was found
}


int main(int argc, char* argv[])
{
	try {
		tests_t& tests = MoccarduinoTest::getTests();
		std::size_t errors = 0, executed = 0;
		for (tests_t::const_iterator it = tests.begin(); it != tests.end(); ++it) {
			if (!on_list(it->first, argv + 1)) continue;	// argv + 1 ... skip the application name

			++executed;
			std::cout << "TEST: " << it->first << " ... ";
			std::cout.flush();
			try {
				it->second->run();
				std::cout << "passed" << std::endl;
			}
			catch (TestException &e) {
				std::cout << "FAILED!" << std::endl;
				++errors;
				std::cout << e.what() << std::endl;
			}
		}

		std::cout << std::endl << "Total " << (executed-errors) << " / " << executed << " tests passed";
		if (errors > 0) {
			std::cout << ", but " << errors << " tests FAILED!";
		}
		std::cout << std::endl;

		return errors == 0 ? 0 : 1;
	}
	catch (std::exception& e) {
		std::cerr << "Uncaught exception: " << e.what() << std::endl;
		return 2;
	}
}

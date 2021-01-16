#ifndef MOCCARDUINO_UNIT_TESTS_TEST_HPP
#define MOCCARDUINO_UNIT_TESTS_TEST_HPP

#include <map>
#include <string>
#include <functional>
#include <sstream>
#include <exception>

/**
 * Specific exception that behaves like a stream, so it can cummulate error messages more easily.
 */
class TestException : public std::exception
{
protected:
	std::string mMessage;	///< Internal buffer where the message is kept.

public:
	TestException() : std::exception() {}
	TestException(const char* msg) : std::exception(), mMessage(msg) {}
	TestException(const std::string& msg) : std::exception(), mMessage(msg) {}
	virtual ~TestException() noexcept {}

	virtual const char* what() const throw()
	{
		return mMessage.c_str();
	}

	// Overloading << operator that uses stringstream to append data to mMessage.
	template<typename T>
	TestException& operator<<(const T& data)
	{
		std::stringstream stream;
		stream << mMessage << data;
		mMessage = stream.str();
		return *this;
	}
};


/*
 * Assertion helpers
 */

inline void assert_true_(bool condition, const char* conditionStr, const std::string& comment, int line, const char* srcFile)
{
	if (!condition) {
		throw (TestException() << "Assertion failed (" << conditionStr << "): " << comment << "\n"
			<< "at " << srcFile << "[" << line << "]");
	}
}

template<typename T1, typename T2>
inline void assert_eq_(bool condition, const char* exprStr, const char* correctStr, T1 expr, T2 correct, const std::string& comment, int line, const char* srcFile)
{
	if (!condition) {
		throw (TestException() << "Assertion failed (" << exprStr << " == " << correctStr << "): " << comment << "\n"
			<< "value " << expr << " given, but " << correct << " was expected at " << srcFile << "[" << line << "]");
	}
}

template<class E>
inline void assert_exception_(std::function<void()> const& op, const char* exceptionTypeStr, const std::string& comment, int line, const char* srcFile)
{
	try {
		try {
			op();
		}
		catch (E&) {
			// this is correct behavior
			return;
		}
	}
	catch (std::exception&) {
		throw (TestException() << "Assertion failed (" << exceptionTypeStr << " was expected, but another exception was thrown): " << comment << "\n"
			<< "at " << srcFile << "[" << line << "]");
	}

	throw (TestException() << "Assertion failed (" << exceptionTypeStr << " was expected, but no exception was thrown): " << comment << "\n"
		<< "at " << srcFile << "[" << line << "]");

}

#define ASSERT_TRUE(condition, comment) assert_true_((condition), #condition, comment, __LINE__, __FILE__)
#define ASSERT_FALSE(condition, comment) assert_true_(!(condition), #condition, comment, __LINE__, __FILE__)
#define ASSERT_EQ(expr, correct, comment) assert_eq_((expr) == (correct), #expr, #correct, (expr), (correct), comment, __LINE__, __FILE__)
#define ASSERT_EXCEPTION(exClass, op, comment) assert_exception_<exClass>(op, #exClass, comment, __LINE__, __FILE__)


/**
 * Base class for all tests. It defines virtual interface (method run()) and naming/registration mechanism for the tests.
 */
class MoccarduinoTest
{
private:
	typedef std::map<std::string, const MoccarduinoTest*> registry_t;

	/**
	 * Global test registry declared as static singleton.
	 */
	static registry_t& _getTests()
	{
		static registry_t tests;
		return tests;
	}


protected:
	/**
	 * Name of the test. The name should be directly assigned by the derived constructor.
	 * Name should reflect the file hierarchy and the local name of the test.
	 */
	std::string mName;

public:
	/**
	 * Constructor names and registers the object within the global naming registry.
	 */
	MoccarduinoTest(const std::string& name) : mName(name)
	{
		registry_t& tests = _getTests();
		if (tests.find(mName) != tests.end())
			throw (TestException() << "Moccarduino test named '" << name << "' is already registered!");

		tests[mName] = this;
	}


	/**
	 * Enforces virtual destructors and removes the object from the registry.
	 */
	virtual ~MoccarduinoTest()
	{
		_getTests().erase(mName);
	}


	/**
	 * Testing interface method that performs the test itself.
	 * If TestException is thrown (possibly using MYASSERT), the test is considered failed.
	 * If any other exception is thrown, internal error is reported (testing is halted completely).
	 */
	virtual void run() const = 0;


	/**
	 * A way to access (read-only) the list of registered tests.
	 */
	static const std::map<std::string, const MoccarduinoTest*>& getTests()
	{
		return _getTests();
	}
};


#endif

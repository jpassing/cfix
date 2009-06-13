/*--------------------------------------------------------------------------*
  
  BinaryNumberTest.cpp

  This test file provides some examples of using WinUnit, by testing a 
  "BinaryNumber" class.  The BinaryNumber class has two constructors: 
  one takes a string consisting of '1' and '0' characters; the other takes 
  an unsigned short. The class has NumericValue and StringValue properties, 
  which return counterparts to what the constructors take, except the string 
  value is normalized with leading zeros.  See ExampleLib\BinaryNumber.h for 
  the "spec" of the class.

  Things to note:
  1. There is a one-to-one correspondence between the test file and the 
     class.  If I had additional classes I would make an additional test 
     file for each.
  2. There is exactly one .h/.cpp pair for each production class.
  3. The test file name for the class is <class> + Test.cpp.  This is a 
     convention that makes it easier to match up test files with 
     production classes.
  4. The test names start with the name of the class, followed by the 
     name (or description, in the case of constructors) of the method(s) 
     being tested, followed by some statement of what's expected to be
     shown by the test.  This makes it easier to run related tests as a 
     group (using the winunit -p option).
  5. The WIN_ASSERT* macros take optional printf-style format strings and
     arguments.  Note that here I've written them using the _T("") style
     because I want to be able to build both with and without _UNICODE.  If
     you are only building with _UNICODE you can use wchar_t*.
  6. I usually put a prefix on messages passed to the WIN_TRACE macro, such 
     as "INFO:" or "WARNING:", to help distinguish them from errors.

 *--------------------------------------------------------------------------*/

#include "WinUnit.h"

#include "..\SampleLib\BinaryNumber.h"
#include "BinaryNumberTestDataProvider.h"
#include "BinaryNumberDataRow.h"

using namespace ExampleLib;

// This overload of WinUnit::ToString() allows the BinaryNumber object
// to be displayed correctly if it shows up in error messages.  Implement this 
// especially for objects that implement operator== and are compared using 
// WIN_ASSERT_EQUAL (see below).  If this were not implemented, everything 
// would still work--you'd just see "[OBJECT]" in error messages containing
// a BinaryNumber object
template<> 
const TCHAR* WinUnit::ToString(const BinaryNumber& object, 
                               TCHAR buffer[], size_t bufferSize)
{
    // This is because I want to be able to run both with and without _UNICODE,
    // and my BinaryNumber string field is only non-Unicode. (%S means to use
    // the opposite "wideness" from the version of the string function being 
    // called.) 
#ifdef _UNICODE
    wchar_t* formatString = L"%S";
#else
    char* formatString = "%s";
#endif
    ::_sntprintf_s(buffer, bufferSize, _TRUNCATE, formatString, object.StringValue);
    return buffer;
}

// This test demonstrates WIN_ASSERT_EQUAL and WIN_ASSERT_STRING_EQUAL.  It's 
// usually a good idea to add more details via message parameters to the assert.
// Note that messages are always expected to be of type TCHAR* (i.e. Unicode
// when _UNICODE is defined; non-Unicode otherwise).  However, the first
// two arguments of WIN_ASSERT_STRING_EQUAL can be either wchar_t* or char*
// (but both must be the same).
BEGIN_TEST(BinaryNumberPlusAddsTwoNumbersCorrectly)
{
    BinaryNumber sum = BinaryNumber("101") + BinaryNumber("110");
    WIN_ASSERT_EQUAL(11, sum.NumericValue, _T("5 + 6 should be 11."));
    WIN_ASSERT_STRING_EQUAL("0000000000001011", sum.StringValue, _T("Value is 11."));
}
END_TEST

// In order to use WIN_ASSERT_EQUAL on non-fundamental datatypes, operator=
// must be implemented for the object in question (which it is, here).  If
// There were an failure here (which you can see by changing the lines so 
// the two are not equivalent), the objects would be displayed using the 
// ToString implementation above.
BEGIN_TEST(BinaryNumberConstructorsShouldBeEquivalent)
{
    BinaryNumber bn1(7);
    BinaryNumber bn2("111");
    WIN_ASSERT_EQUAL(bn1, bn2);
}
END_TEST

// The following three tests demonstrate expected exceptions.

BEGIN_TEST(BinaryNumberStringConstructorOnlyAllowsOnesAndZeros)
{
    WIN_ASSERT_THROWS(BinaryNumber("-100"), BinaryNumber::InvalidInputException,
        _T("Only ones and zeros should be allowed."));
}
END_TEST

BEGIN_TEST(BinaryNumberStringConstructorDisallowsStringTooLong)
{
    char longString[BinaryNumber::MaxStringValueLength + 2] = "";
    memset(longString, '1', ARRAYSIZE(longString) - 1);
    WIN_ASSERT_THROWS(BinaryNumber(longString), 
        BinaryNumber::InvalidInputException, 
        _T("BinaryNumber constructor should restrict length of string."));
}
END_TEST

BEGIN_TEST(BinaryNumberPlusRecognizesIntegerOverflow)
{
    unsigned short s1 = USHRT_MAX / 2 + 1;
    unsigned short s2 = s1;

    BinaryNumber bn1(s1);
    BinaryNumber bn2(s2);

    WIN_ASSERT_THROWS(bn1 + bn2, BinaryNumber::IntegerOverflowException);
}
END_TEST

// Leading zeros are added to make all BinaryNumber string representations
// the same length.  This test verifies one instance of that.
BEGIN_TEST(BinaryNumberStringConstructorShouldNormalizeWithLeadingZeroes)
{
    BinaryNumber bn1("00000111");
    BinaryNumber bn2("111");
    WIN_ASSERT_EQUAL(bn1, bn2);
}
END_TEST

// Here we demonstrate that any way you construct a BinaryNumber of value
// zero, it ends up the same.
BEGIN_TEST(BinaryNumberConstructorsHandleZeroCorrectly)
{
    BinaryNumber bn1("0");
    WIN_ASSERT_EQUAL(0, bn1.NumericValue);
    WIN_ASSERT_STRING_EQUAL("0000000000000000", bn1.StringValue);

    BinaryNumber bn2((unsigned short)0);
    WIN_ASSERT_EQUAL(bn1, bn2);
}
END_TEST


// ------------------------------------------------------------
// Tests that use fixture
// ------------------------------------------------------------
namespace
{

// Here's our "data provider"--a file local instance of a class that was 
// designed to be used in this way.  (See notes in *DataProvider.h.)
BinaryNumberTestDataProvider s_dataProvider;

// We first declare a fixture, then we can create SETUP and TEARDOWN functions,
// and refer to it as the second parameter of the BEGIN_TESTF macro.
// SETUP and TEARDOWN will be called before and after (respectively) running
// any test that uses the fixture.
FIXTURE(BinaryNumberTestDataFixture);

// This function opens the "data provider" object.  Since that object is meant
// to read from a text file, this function prepares the full path to the text
// file and passes it to the data provider's "Open" method.
SETUP(BinaryNumberTestDataFixture)
{    
    // Get directory for test data.  If the environment variable TestDir is set,
    // use that.  Otherwise use the current directory.  Note that you can
    // set environment variables on the WinUnit command line via the "--" 
    // option.
    TCHAR buffer[MAX_PATH] = _T("");
    bool testDirectoryVarSet = WinUnit::Environment::GetVariable(_T("TestDir"), 
        buffer, MAX_PATH);
    if (!testDirectoryVarSet)
    {
        WIN_TRACE("INFO: Environment variable TestDir not set; "
                  "looking for BinaryNumberTestData.txt in current directory.\n");
        DWORD charCount = GetCurrentDirectory(MAX_PATH, buffer);
        // This macro should be used to verify the results of any WinAPI 
        // function that sets last error (i.e. in the documentation it tells
        // you to call GetLastError for more error information).  For the first
        // argument, you pass an expression that should be true if the function
        // succeeded.  If it fails, it will show the system error message
        // associated with the result of GetLastError(), as well as whatever
        // message you passed in.
        WIN_ASSERT_WINAPI_SUCCESS(charCount != 0 && charCount <= MAX_PATH, 
            _T("GetCurrentDirectory failed."));
    }

    // Append trailing backslash if necessary.
    size_t directoryLength = _tcslen(buffer);
    WIN_ASSERT_TRUE(directoryLength < MAX_PATH - 1, 
        _T("Directory name too long: %s."), buffer);
    if (directoryLength > 0 && buffer[directoryLength - 1] != _T('\\'))
    {
        _tcsncat_s(buffer, ARRAYSIZE(buffer), _T("\\"), _TRUNCATE);
    }

    // Append filename.
    WIN_ASSERT_ZERO(::_tcsncat_s(buffer, MAX_PATH, 
        _T("BinaryNumberTestData.txt"), _TRUNCATE));

    // Finally, open the data provider.
    s_dataProvider.Open(buffer);
}

TEARDOWN(BinaryNumberTestDataFixture)
{
    s_dataProvider.Close();
}

// Here we're using the "data provider" (which reads lines from a file) to
// test multiple combinations.
BEGIN_TESTF(BinaryNumberStringConstructorTest, BinaryNumberTestDataFixture)
{
    // The "data rows" (non-comment, non-empty lines in a file) contain a 
    // string value and a numeric value.

    // We can use trace statements to make it easier to see on which row an 
    // error occurs, if any.

    BinaryNumberDataRow row;
    while (s_dataProvider.GetNextDataRow(row))
    {
        // Ensure that there was a "StringValue" field in the row. (Note
        // that this value will be invalid/changed once you get the next
        // data row, so use it in this loop or copy it.)
        const char* stringValue = NULL;
        WIN_ASSERT_TRUE(row.GetItem("StringValue", stringValue), 
            _T("\"StringValue\" field not found in row %d."), row.LineNumber);

        // Ensure there was a "NumericValue" field found in the row.
        unsigned int numericValue = 0;
        WIN_ASSERT_TRUE(row.GetItem("NumericValue", numericValue), 
            _T("\"NumericValue\" field not found in row %d."), row.LineNumber);
        // Writing out a trace statement makes it easier to tell which row
        // a failure occurred on.
        WIN_TRACE("INFO: Line %d: \"%s\" [%d].\n", row.LineNumber, stringValue, numericValue);

        // Now we just want to make sure that the BinaryNumber constructed
        // from the string value has the expected numeric value.
        BinaryNumber binaryNumber(stringValue);
        WIN_ASSERT_EQUAL(numericValue, binaryNumber.NumericValue, 
            _T("Data file, line %d."), row.LineNumber);
    }
}
END_TESTF

// This test is the same as the previous one, except it's intended to test
// the numeric constructor.
BEGIN_TESTF(BinaryNumberNumericConstructorTest, BinaryNumberTestDataFixture)
{
    BinaryNumberDataRow row;
    while (s_dataProvider.GetNextDataRow(row))
    {
        const char* stringValue = NULL;
        WIN_ASSERT_TRUE(row.GetItem("StringValue", stringValue), 
            _T("\"StringValue\" field not found in row %d."), row.LineNumber);

        unsigned int numericValue = 0;
        WIN_ASSERT_TRUE(row.GetItem("NumericValue", numericValue), 
            _T("\"NumericValue\" field not found in row %d."), row.LineNumber);
        WIN_TRACE("INFO: Line %d: \"%s\" [%d].\n", row.LineNumber, stringValue, numericValue);
        BinaryNumber bn1(( unsigned short ) numericValue);
        // Since string values are normalized in the constructor, we can't
        // just compare the string value of bn1 with the string value found
        // in the file--we have to create another object using the string value
        // and make sure they were both normalized to the same thing.
        BinaryNumber bn2(stringValue);
        WIN_ASSERT_STRING_EQUAL(bn1.StringValue, bn2.StringValue, 
            _T("(Data file, line %d."), row.LineNumber);
        WIN_ASSERT_EQUAL(bn1, bn2, _T("Data file, line %d."), row.LineNumber);
    }
}
END_TESTF

}
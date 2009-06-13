/*--------------------------------------------------------------------------*
  
  BinaryNumberTestDataProvider.cpp

  This class implements TestData::FileDataProvider to read in a line from
  a test file containing "binary number test data" (a binary number in 
  string form followed by a decimal number).

  The binary number is read in as a string, and the decimal number as a 
  number, in order to test the BinaryNumber class's conversions.

  The BinaryNmberDataRow structure can be found in BinaryNumberDataRow.h.

 *--------------------------------------------------------------------------*/
#include "WinUnit.h"
#include "BinaryNumberTestDataProvider.h"
#include "BinaryNumberDataRow.h"

bool BinaryNumberTestDataProvider::ParseLine(
        const char* line, unsigned int lineNumber, BinaryNumberDataRow& row)
{
    unsigned short numericValue = 0;
    char stringValue[ExampleLib::BinaryNumber::MaxStringValueLength + 1] = "";
    int matches = sscanf_s(line, "%s %hu", stringValue, ARRAYSIZE(stringValue), &numericValue);

    WIN_ASSERT_EQUAL(2, 
        matches, 
        _T("Line %d was incorrectly formatted or string segment was too long."), 
        lineNumber);

    row = BinaryNumberDataRow(lineNumber, numericValue, stringValue);
    return true;
}

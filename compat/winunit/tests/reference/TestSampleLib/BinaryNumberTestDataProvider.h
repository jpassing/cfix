/*--------------------------------------------------------------------------*
  
  BinaryNumberTestDataProvider.h

  This class implements TestData::FileDataProvider to read in a line from
  a test file containing "binary number test data" (a binary number in 
  string form followed by a decimal number).

  The binary number is read in as a string, and the decimal number as a 
  number, in order to test the BinaryNumber class's conversions.

  The BinaryNmberDataRow structure can be found in BinaryNumberDataRow.h.

 *--------------------------------------------------------------------------*/
#pragma once
#include "FileDataProvider.h"

class BinaryNumberDataRow;

class BinaryNumberTestDataProvider : public TestData::FileDataProvider<BinaryNumberDataRow>
{
public:
    virtual bool ParseLine(
            const char* line, unsigned int lineNumber, BinaryNumberDataRow& row);
};

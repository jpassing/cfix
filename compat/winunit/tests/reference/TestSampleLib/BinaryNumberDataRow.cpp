/*--------------------------------------------------------------------------*
  
  BinaryNumberDataRow.cpp

  This class implements a "data row" type, to be used by
  BinaryNumberTestDataProvider.

  Notes:
   1. The class is made generic by implementing the following, and overloads:
       
    template<typename T> bool GetItem(const char* name, T& value);

   2. Instead of using a hash table, the data members are explicitly present
      as members and accessed via the appropriate GetItem overloads.

   3. This class is completely tied to BinaryNumberTestDataProvider::
      ParseLine.  If one changes, the other should change.

 *--------------------------------------------------------------------------*/
#include "BinaryNumberDataRow.h"

#include "WinUnit.h"
#include <errno.h>

// We need a default constructor because it has to be constructed and 
// passed into methods as a reference for its first population.
BinaryNumberDataRow::BinaryNumberDataRow()
:
_lineNumber(0),
_numericValue(0)
{
    _stringValue[0] = '\0';
}

// This constructor contains all the data found in the row (we also take
// the line number, to be used later for error messages, if necessary).
BinaryNumberDataRow::BinaryNumberDataRow(
    unsigned int lineNumber, 
    unsigned short numericValue, 
    const char* stringValue
    )
    :
_lineNumber(lineNumber),
_numericValue(numericValue)
{
    errno_t result = strncpy_s(_stringValue, ARRAYSIZE(_stringValue), stringValue, ARRAYSIZE(_stringValue));
    WIN_ASSERT_NOT_EQUAL(EINVAL, result, 
        _T("String value too long on line %d; must be less than %d characters."), 
        lineNumber, ARRAYSIZE(_stringValue));
}

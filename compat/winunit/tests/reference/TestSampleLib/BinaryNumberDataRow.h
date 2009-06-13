/*--------------------------------------------------------------------------*
  
  BinaryNumberDataRow.h

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
#pragma once

#include "..\SampleLib\BinaryNumber.h"

class BinaryNumberDataRow
{
private:
    char _stringValue[ExampleLib::BinaryNumber::MaxStringValueLength + 1];
    unsigned int _numericValue;
    unsigned int _lineNumber;
public:
    BinaryNumberDataRow();
    BinaryNumberDataRow(unsigned int lineNumber, 
        unsigned short numericValue, const char* stringValue);
public:
    // I decided to make LineNumber a separate property.  Having it
    // retrievable via GetItem seemed like inconsistent semantics, as GetItem
    // is meant for the data items found in the row.
    __declspec(property(get=GetLineNumber)) unsigned int LineNumber;
    unsigned int GetLineNumber();

    template<typename T>
    bool GetItem(const char* name, T& value);
};

template<typename T>
inline bool BinaryNumberDataRow::GetItem(const char* name, T& value) 
{ 
    return false; 
}

template<>
inline bool BinaryNumberDataRow::GetItem(const char* name, unsigned int& value)
{
    if (_stricmp(name, "NumericValue") == 0)
    {
        value = _numericValue;
        return true;
    }
    return false;
}

template<>
inline bool BinaryNumberDataRow::GetItem(const char* name, const char*& value)
{
    if (_stricmp(name, "StringValue") == 0)
    {
        value = _stringValue;
        return true;
    }
    return false;
}

inline unsigned int BinaryNumberDataRow::GetLineNumber()
{
    return _lineNumber;
}

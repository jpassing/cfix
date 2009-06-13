/*--------------------------------------------------------------------------*
  
  DataProvider.h

  This base class is simply an example of how you might wish to implement
  a test "data provider".  See FileDataProvider.h for an example.

  The "DataRow" type can be anything, but one idea is to have it implement
  a function like this:

    template<typename T>
    bool GetItem(const char* name, T& value) { return false; }
  
  ...with various overloads for the types present in the row.  See
  BinaryNumberDataRow.h for an example.  

  Though no part of the WinUnit framework cares about the format of test data
  providers, using this format might be convenient for readability (if
  you use the same format for all your tests) and in case you want to swap
  in a different implementation (e.g. getting data from an XML-formatted
  file or a database).

 *--------------------------------------------------------------------------*/
#pragma once

namespace TestData
{
    template<typename DataRow>
    class DataProvider
    {
    public:
        virtual void Open(const TCHAR* providerString) = 0;
        virtual void Close() = 0;
        virtual bool GetNextDataRow(DataRow& row) = 0;
    };
}
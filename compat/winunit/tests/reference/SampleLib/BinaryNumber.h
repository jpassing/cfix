/*--------------------------------------------------------------------------*
  
  BinaryNumber.h

  Features of the BinaryNumber class:
  1) It has a NumericValue property and a StringValue property.  The 
     StringValue is a string containing characters '1' and '0'.  The string
     has the number of characters as there are bits in the NumericValue
     (unsigned short).
  2) You can construct one with a string of '1' and '0' characters and it will
     normalize the string by adding leading 0's.  This constructor will 
     throw an exception if the string passed in is too long or contains
     invalid characters.
  3) You can construct one with an unsigned short.
  4) You can add two BinaryNumber objects together using '+'.  It will throw
     an exception if there is an arithmetic overflow.

 *--------------------------------------------------------------------------*/
#pragma once

#include <windows.h> // CHAR_BIT

#ifndef CHAR_BIT
	#define CHAR_BIT 8
#endif

namespace ExampleLib
{
    class BinaryNumber
    {
        // This typedef is to ensure that if we change the type of the 
        // numeric value, the buffer for the string value will remain the 
        // right size.
        typedef unsigned short NumericValueType;
    
    public:
        static const unsigned int MaxStringValueLength = 
            sizeof(NumericValueType) * CHAR_BIT;

    private:
        NumericValueType _numericValue;
        char _stringValue[MaxStringValueLength + 1];
    
    public:
        BinaryNumber(const char* stringValue);
        BinaryNumber(unsigned short numericValue);
    
    public:
        BinaryNumber operator+(const BinaryNumber& rhs);
        bool operator==(const BinaryNumber& rhs) const;

        // This Microsoft extension for "properties" arguably promotes 
        // readability (if you're familiar with it).
        // NumericValue and StringValue can be accessed as though they 
        // were data members, but they are read-only, and you can set 
        // breakpoints in their associated methods.

        __declspec(property(get=GetNumericValue)) unsigned int NumericValue;
        unsigned int GetNumericValue() const;

        __declspec(property(get=GetStringValue)) const char* StringValue;
        const char* GetStringValue() const;

    private:
        void PopulateStringValue(unsigned short numericValue);

    public:
        class Exception
        {
        };

        class InvalidInputException : public Exception
        {
        };

        class IntegerOverflowException : public Exception
        {
        };
    };

    // Note that although I've inlined these methods, I put them outside
    // the class body.  I find this to be a good practice for keeping
    // class definitions compact and readable.
    inline unsigned int BinaryNumber::GetNumericValue() const
    {
        return _numericValue;
    }

    inline const char* BinaryNumber::GetStringValue() const
    {
        return _stringValue;
    }
}
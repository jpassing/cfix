/*--------------------------------------------------------------------------*
  
  BinaryNumber.cpp

  See BinaryNumber.h for notes on this class.

 *--------------------------------------------------------------------------*/
#include "BinaryNumber.h"

#include <string.h> // strlen
#include <windows.h> // ARRAYSIZE

namespace ExampleLib
{
    BinaryNumber::BinaryNumber(const char* stringValue)
        :
    _numericValue(0)
    {
        size_t stringLength = strlen(stringValue);
        if (stringLength > MaxStringValueLength)
        {
            throw BinaryNumber::InvalidInputException();
        }

        const char* cursor = stringValue + stringLength - 1;
        unsigned int placeValue = 1;
        while(cursor >= stringValue)
        {
            switch(*cursor)
            {
            case '1':
                _numericValue += ( unsigned short ) placeValue;
                break;
            case '0':
                break;
            default:
                throw BinaryNumber::InvalidInputException();
            }
            placeValue *= 2;
            cursor--;
        }

        PopulateStringValue(_numericValue);
    }

    BinaryNumber::BinaryNumber(unsigned short numericValue)
        :
    _numericValue(numericValue)
    {
        PopulateStringValue(numericValue);
    }

    void BinaryNumber::PopulateStringValue(unsigned short numericValue)
    {
        unsigned short value = numericValue;
        char* cursor = _stringValue + MaxStringValueLength;
        *cursor-- = '\0';

        while(value > 0 && cursor >= _stringValue)
        {
            *cursor-- = (value & 1) ? '1' : '0';
            value = value >> 1;
        }
        while(cursor >= _stringValue)
        {
            *cursor-- = '0';
        }
    }

    BinaryNumber BinaryNumber::operator+(const BinaryNumber& rhs)
    {
        unsigned short sum = this->_numericValue + rhs._numericValue;
        if (sum < this->_numericValue) 
        { 
            throw BinaryNumber::IntegerOverflowException(); 
        }

        return BinaryNumber(sum);
    }

    bool BinaryNumber::operator==(const BinaryNumber& rhs) const
    {
        return ((this->_numericValue == rhs._numericValue) &&
               (strcmp(this->_stringValue, rhs._stringValue) == 0));
    }
}
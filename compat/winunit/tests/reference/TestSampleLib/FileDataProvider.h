/*--------------------------------------------------------------------------*
  
  FileDataProvider.h

  This class is an example of how a DataProvider could be (partially) 
  implemented to read data from a file.

  Because the DataRow structure is still not specified, FileDataProvider must
  be inherited, with the ParseLine method implemented.

  However, this class does the work of opening and closing the file, and
  skipping past blank lines and comment lines (lines starting with '#').

  Notes:
  1. This class is expressly intended to be used inside WinUnit tests, and 
     not elsewhere.  Therefore, WIN_ASSERT* macros are used within the class 
     implementation.
  2. The class does not have a non-default destructor or a non-trivial 
     constructor. This is because I wanted the ability to instantiate an 
     instance of the class as a file local ("global") without having to do a 
     lot of computation to get the argument to the constructor, and also 
     because I wanted to control the opening and closing of the provider 
     (the file) independently of the lifetime of the object (i.e. in the 
     SETUP and TEARDOWN of a fixture).  Ordinarily one might do the Close 
     operation in the destructor of such an object.
  3. The strings are not wide-character because that's the default for reading 
     text from files.  Implementing it for wide characters is left as
     an exercise for the reader. :)
  4. The line number is stored (even for blank and comment lines) because it
     can be useful for displaying status messages, such as errors.

 *--------------------------------------------------------------------------*/
#pragma once

#include "DataProvider.h"
#include "WinUnit.h"

namespace TestData
{
    template<typename DataRow, unsigned int MaxLineLength = 1024>
    class FileDataProvider : public DataProvider<DataRow>
    {
    private:
        // Holds the current line that was just read in. MaxLineLength
        // is the length after which the line will be truncated.
        char _line[MaxLineLength];

        // The file pointer of the currently opened file (NULL before
        // Open is called and after Close is called).
        FILE* _file;

        // Holds the current line number in the file.  This is one-based--
        // the first line of the file is 1, not 0.
        unsigned int _lineNumber;

        // True if we're in the middle of a truncated line. (This means
        // _lineNumber won't be incremented the next time around.)
        bool _truncatedLine;
    public:
        FileDataProvider();
    public:
        // Members of DataProvider
        virtual void Open(const TCHAR* fileName);
        virtual void Close();
        virtual bool GetNextDataRow(DataRow& row);
    public:
        // To be implemented by inheritor
        virtual bool ParseLine(
            const char* line, unsigned int lineNumber, DataRow& row) = 0;
    private:
        void Reset();
    };


    template<typename DataRow, unsigned int MaxLength>
    inline FileDataProvider<DataRow, MaxLength>::FileDataProvider()
    :
    _file(NULL),
    _lineNumber(0),
    _truncatedLine(false)
    {
        Reset();
    }

    template<typename DataRow, unsigned int MaxLength>
    inline void FileDataProvider<DataRow, MaxLength>::Reset()
    {
        _file = NULL;
        _lineNumber = 0;
        _truncatedLine = false;
        _line[0] = '\0';
    }

    template<typename DataRow, unsigned int MaxLength>
    inline void FileDataProvider<DataRow, MaxLength>::Open(const TCHAR* fileName)
    {
        WIN_ASSERT_NULL(_file, _T("Close must be called before calling Open again."));
        // Make sure file exists and is not a directory
        DWORD attributes = ::GetFileAttributes(fileName);
        WIN_ASSERT_WINAPI_SUCCESS(INVALID_FILE_ATTRIBUTES != attributes, _T("Invalid file: %s."), fileName);
        WIN_ASSERT_TRUE((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0, _T("%s must not be a directory."), fileName);

        // Open file
        WIN_ASSERT_ZERO(_tfopen_s(&_file, fileName, _T("rtc")), _T("Could not open file: %s."), fileName);
    }

    template<typename DataRow, unsigned int MaxLineLength>
    inline void FileDataProvider<DataRow, MaxLineLength>::Close()
    {
        WIN_ASSERT_NOT_NULL(_file, _T("Open() must have been successfully called first."));
        // Close file
        WIN_ASSERT_ZERO(fclose(_file), _T("Could not close file."));

        Reset();
    }

    template<typename DataRow, unsigned int MaxLineLength>
    bool FileDataProvider<DataRow, MaxLineLength>::GetNextDataRow(DataRow& row)
    {
        WIN_ASSERT_NOT_NULL(_file, _T("Must call Open() before GetNextDataRow."));
        if (feof(_file)) { return false; }

        // -------------------------------------------------------------------
        // Skip comments and empty lines
        // -------------------------------------------------------------------
        bool isComment = false;
        while(true)
        {
            char* line = fgets(_line, MaxLineLength, _file);
            if (line == NULL) { return false; }

            if (_truncatedLine) 
            {
                _truncatedLine = false;
            }
            else
            {
                _lineNumber++;
            }

            // If line is an empty line, skip it.
            if (line[0] == '\n') { continue; }

            // Check for truncated line
            size_t lineLength = strlen(_line);
            if (lineLength == MaxLineLength - 1 && 
                _line[lineLength - 1] != '\n' && 
                !feof(_file))
            {
                _truncatedLine = true;
            }

            // If line starts with comment character, eat the rest of the line
            // and skip it.
            if (line[0] == '#') 
            {
                if (_truncatedLine)
                {
                    // Scan up to the next newline
                    fscanf_s(_file, "%*[^\n]");
                    // Scan the next newline
                    fscanf_s(_file, "%*1[\n]");
                    // We're past the truncated line
                    _truncatedLine = false;
                }
                continue; 
            }

            break;
        }

        // -------------------------------------------------------------------
        // Remove trailing newline, if any
        // -------------------------------------------------------------------
        if (_truncatedLine)
        {
            WIN_TRACE(
                "WARNING: Line %d was truncated and will be read in multiple parts; "
                "consider increasing MaxLineLength: %s.\n", _lineNumber, _line);
        }
        else
        {
            size_t lineLength = strlen(_line);
            if (lineLength > 0 && _line[lineLength - 1] == '\n')
            {
                _line[lineLength - 1] = '\0';
            }
        }

        // ------------------------------------------------------------------
        // Parse the line
        // ------------------------------------------------------------------
        return ParseLine(_line, _lineNumber, row);
    }
}
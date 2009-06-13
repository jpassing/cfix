/*--------------------------------------------------------------------------*
  
  DeleteFileTest.cpp

  This file includes a basic example of fixture use.

 *--------------------------------------------------------------------------*/

#include "WinUnit.h"

#include <windows.h>

// Fixture must be declared.
FIXTURE(DeleteFileFixture);

namespace
{
    TCHAR s_tempFileName[MAX_PATH] = _T("");
    bool IsFileValid(TCHAR* fileName);
}

// Both SETUP and TEARDOWN must be present. An empty body is allowed
// for either function, but use with care.  Anything done in SETUP
// should be undone in TEARDOWN, so ordinarily you will want code in both. 
SETUP(DeleteFileFixture)
{
    // This is the maximum size of the directory passed to GetTempFileName.
    const unsigned int maxTempPath = MAX_PATH - 14; 
    TCHAR tempPath[maxTempPath + 1] = _T("");
    DWORD charsWritten = GetTempPath(maxTempPath + 1, tempPath);
    // (charsWritten does not include null character)
    WIN_ASSERT_TRUE(charsWritten <= maxTempPath && charsWritten > 0, 
        _T("GetTempPath failed."));

    // Create a temporary file
    UINT tempFileNumber = GetTempFileName(tempPath, _T("WUT"), 
        0, // This means the file will get created and closed.
        s_tempFileName);

    // Make sure that the file actually exists
    WIN_ASSERT_WINAPI_SUCCESS(IsFileValid(s_tempFileName), 
        _T("File %s is invalid or does not exist."), s_tempFileName);
}

// Teardown does the inverse of SETUP, as well as undoing whatever side effects
// the tests could have caused.
TEARDOWN(DeleteFileFixture)
{
    // Delete the temp file if it still exists.
    if (IsFileValid(s_tempFileName))
    {
        // Ensure file is not read-only
        DWORD fileAttributes = GetFileAttributes(s_tempFileName);
        if (fileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            WIN_ASSERT_WINAPI_SUCCESS(
                SetFileAttributes(s_tempFileName, 
                    fileAttributes ^ FILE_ATTRIBUTE_READONLY),
                _T("Unable to undo read-only attribute of file %s."),
                s_tempFileName);
        }

        // Since I'm testing DeleteFile, I use the alternative CRT file
        // deletion function in my cleanup.
        WIN_ASSERT_ZERO(_tremove(s_tempFileName), 
            _T("Unable to delete file %s."), s_tempFileName);
    }

    // Clear the temp file name.
    ZeroMemory(s_tempFileName, 
        ARRAYSIZE(s_tempFileName) * sizeof(s_tempFileName[0]));
}

BEGIN_TESTF(DeleteFileShouldDeleteFileIfNotReadOnly, DeleteFileFixture)
{
    WIN_ASSERT_WINAPI_SUCCESS(DeleteFile(s_tempFileName));
    WIN_ASSERT_FALSE(IsFileValid(s_tempFileName), 
        _T("DeleteFile did not delete %s correctly."),
        s_tempFileName);
}
END_TESTF

BEGIN_TESTF(DeleteFileShouldFailIfFileIsReadOnly, DeleteFileFixture)
{
    // Set file to read-only
    DWORD fileAttributes = GetFileAttributes(s_tempFileName);
    WIN_ASSERT_WINAPI_SUCCESS(
        SetFileAttributes(s_tempFileName, 
            fileAttributes | FILE_ATTRIBUTE_READONLY));

    // Verify that DeleteFile fails with ERROR_ACCESS_DENIED
    // (according to spec)
    WIN_ASSERT_FALSE(DeleteFile(s_tempFileName));
    WIN_ASSERT_EQUAL(ERROR_ACCESS_DENIED, GetLastError());
}
END_TESTF

namespace
{
    bool IsFileValid(TCHAR* fileName)
    {
        return (GetFileAttributes(fileName) != INVALID_FILE_ATTRIBUTES);
    }
}
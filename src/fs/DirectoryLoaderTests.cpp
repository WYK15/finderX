#include "fs/FileMetadata.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

static void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

int main() {
    require(formatFileSize(0, true) == L"--", "directory size should be --");
    require(formatFileSize(42, false) == L"42 bytes", "byte size should be exact");
    require(formatFileSize(1536, false) == L"2 KB", "KB size should round up");
    require(formatFileSize(1024 * 1024, false) == L"1.0 MB", "MB size should use one decimal");
    require(kindTextForAttributes(FILE_ATTRIBUTE_DIRECTORY) == L"Folder", "directory kind text");
    require(kindTextForAttributes(FILE_ATTRIBUTE_ARCHIVE) == L"File", "file kind text");
    require(isSkippableDirectoryEntry(L"."), "dot should be skipped");
    require(isSkippableDirectoryEntry(L".."), "dot-dot should be skipped");
    require(!isSkippableDirectoryEntry(L"Documents"), "normal name should not be skipped");
    std::cout << "DirectoryLoaderTests passed\n";
    return 0;
}

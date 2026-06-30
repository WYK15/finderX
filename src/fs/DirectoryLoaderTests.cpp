#include "fs/DirectoryLoader.h"
#include "fs/FileMetadata.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
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
    require(fileNameFromPath(L"C:\\Users\\Example") == L"Example", "fileNameFromPath should extract backslash component");
    require(fileNameFromPath(L"C:/Users/Example/") == L"Example", "fileNameFromPath should ignore trailing slash");

    const auto tempRoot = std::filesystem::temp_directory_path() / "finderx_loader_test";
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot / "FolderA");
    {
        std::ofstream file(tempRoot / "file-b.txt");
        file << "hello";
    }

    DirectoryLoader loader;
    const auto loaded = loader.loadChildren(tempRoot.wstring());
    require(loaded.size() == 2, "loader should enumerate temp directory children");
    require(loaded[0].kind == FileKind::Folder, "folders should sort before files");
    require(loaded[0].name == L"FolderA", "folder name should match");
    require(loaded[0].path == (tempRoot / "FolderA").wstring(), "folder path should match");
    require(!loaded[0].childrenLoaded, "folder children should not be marked loaded");
    require(loaded[1].name == L"file-b.txt", "file name should match");
    require(loaded[1].path == (tempRoot / "file-b.txt").wstring(), "file path should match");

    std::filesystem::remove_all(tempRoot);
    std::cout << "DirectoryLoaderTests passed\n";
    return 0;
}

#include "fs/DirectoryLoader.h"
#include "fs/FileCreation.h"
#include "fs/FileMetadata.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace finderx;

static void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static std::wstring uniqueTempChild(const std::wstring& prefix) {
    DWORD length = GetTempPathW(0, nullptr);
    require(length != 0, "GetTempPathW should report a temp path length");

    std::wstring tempPath(length, L'\0');
    DWORD written = GetTempPathW(length, tempPath.data());
    require(written != 0 && written < length, "GetTempPathW should return a temp path");
    tempPath.resize(written);

    if (!tempPath.empty() && tempPath.back() != L'\\' && tempPath.back() != L'/') {
        tempPath += L'\\';
    }

    return tempPath + prefix + L"_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
           std::to_wstring(GetTickCount64());
}

static void removeTreeIfExists(const std::wstring& path) {
    std::error_code ignored;
    std::filesystem::remove_all(std::filesystem::path(path), ignored);
}

struct TempDirectoryCleanup {
    explicit TempDirectoryCleanup(std::filesystem::path path) : path(std::move(path)) {}
    ~TempDirectoryCleanup() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }

    std::filesystem::path path;
};

struct EnvironmentVariableGuard {
    explicit EnvironmentVariableGuard(const wchar_t* name) : name(name), original(read(name)) {}

    ~EnvironmentVariableGuard() {
        SetEnvironmentVariableW(name.c_str(), hadOriginal ? original.c_str() : nullptr);
    }

    void set(const wchar_t* value) {
        SetEnvironmentVariableW(name.c_str(), value);
    }

    std::wstring name;
    bool hadOriginal = false;
    std::wstring original;

private:
    std::wstring read(const wchar_t* name) {
        const DWORD needed = GetEnvironmentVariableW(name, nullptr, 0);
        if (needed == 0) {
            hadOriginal = false;
            return L"";
        }

        hadOriginal = true;
        std::wstring value(needed, L'\0');
        const DWORD written = GetEnvironmentVariableW(name, value.data(), needed);
        value.resize(written);
        return value;
    }
};

static void testFileCreationHelpers() {
    const std::wstring root = uniqueTempChild(L"finderx_creation_test");
    removeTreeIfExists(root);
    require(CreateDirectoryW(root.c_str(), nullptr), "temp creation root should be created");

    const std::wstring firstFolder = root + L"\\New Folder";
    require(nextNewFolderPath(root) == firstFolder, "first new folder path should use default name");
    require(CreateDirectoryW(firstFolder.c_str(), nullptr), "first new folder should be created");

    const std::wstring secondFolder = root + L"\\New Folder 2";
    require(nextNewFolderPath(root) == secondFolder, "second new folder path should use numeric suffix");
    const FileCreationResult folderResult = createNewFolder(root);
    require(folderResult.success, "createNewFolder should succeed");
    require(folderResult.path == secondFolder, "createNewFolder should create the second available folder");
    require(std::filesystem::is_directory(std::filesystem::path(secondFolder)),
            "created folder should exist");

    const std::wstring firstFile = root + L"\\New Text Document.txt";
    require(nextNewTextFilePath(root) == firstFile, "first new text file path should use default name");
    {
        std::ofstream file{std::filesystem::path(firstFile)};
        require(file.good(), "first new text file should be created");
    }

    const std::wstring secondFile = root + L"\\New Text Document 2.txt";
    require(nextNewTextFilePath(root) == secondFile,
            "second new text file path should use numeric suffix");
    const FileCreationResult fileResult = createNewTextFile(root);
    require(fileResult.success, "createNewTextFile should succeed");
    require(fileResult.path == secondFile, "createNewTextFile should create the second available file");
    require(std::filesystem::is_regular_file(std::filesystem::path(secondFile)),
            "created text file should exist");

    removeTreeIfExists(root);
}

static void runTests() {
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

    {
        EnvironmentVariableGuard userProfile(L"USERPROFILE");
        EnvironmentVariableGuard homeDrive(L"HOMEDRIVE");
        EnvironmentVariableGuard homePath(L"HOMEPATH");
        userProfile.set(nullptr);
        homeDrive.set(L"D:");
        homePath.set(L"\\Users\\Fallback");
        require(defaultHomeDirectory() == L"D:\\Users\\Fallback", "HOMEDRIVE should combine with HOMEPATH");
    }

    {
        EnvironmentVariableGuard userProfile(L"USERPROFILE");
        EnvironmentVariableGuard homeDrive(L"HOMEDRIVE");
        EnvironmentVariableGuard homePath(L"HOMEPATH");
        EnvironmentVariableGuard systemDrive(L"SystemDrive");
        userProfile.set(nullptr);
        homeDrive.set(nullptr);
        homePath.set(L"\\Users\\Fallback");
        systemDrive.set(L"E:");
        require(defaultHomeDirectory() == L"E:\\Users\\Fallback",
                "HOMEPATH without HOMEDRIVE should use SystemDrive");
    }

    const auto tempRoot = std::filesystem::temp_directory_path() /
                          (L"finderx_loader_test_" + std::to_wstring(GetCurrentProcessId()) +
                           L"_" + std::to_wstring(GetTickCount64()));
    TempDirectoryCleanup cleanup(tempRoot);
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot / "FolderA");
    std::filesystem::create_directories(tempRoot / "folderb");
    {
        std::ofstream file(tempRoot / "file-b.txt");
        file << "hello";
    }
    {
        std::ofstream file(tempRoot / "File-A.txt");
        file << "hello";
    }

    DirectoryLoader loader;
    const auto loadedWithStatus = loader.loadChildrenWithStatus(tempRoot.wstring());
    require(loadedWithStatus.ok(), "loader status should be ok for temp directory");
    const auto& loaded = loadedWithStatus.children;
    require(loaded.size() == 4, "loader should enumerate temp directory children");
    require(loaded[0].kind == FileKind::Folder, "folders should sort before files");
    require(loaded[0].name == L"FolderA", "folder name should match");
    require(loaded[0].path == (tempRoot / "FolderA").wstring(), "folder path should match");
    require(loaded[0].modifiedTicks > 0, "folder should expose raw modified ticks");
    require(loaded[0].sizeBytes == 0, "folder raw size should be zero");
    require(!loaded[0].childrenLoaded, "folder children should not be marked loaded");
    require(loaded[1].name == L"folderb", "folder names should sort case-insensitively");
    require(loaded[2].name == L"File-A.txt", "file names should sort case-insensitively");
    require(loaded[3].name == L"file-b.txt", "file name should match");
    require(loaded[3].path == (tempRoot / "file-b.txt").wstring(), "file path should match");
    require(loaded[3].modifiedTicks > 0, "file should expose raw modified ticks");
    require(loaded[3].sizeBytes > 0, "file should expose raw size bytes");

    const auto wrapperLoaded = loader.loadChildren(tempRoot.wstring());
    require(wrapperLoaded.size() == loaded.size(), "loadChildren should return status result children");

    const auto missing = loader.loadChildrenWithStatus((tempRoot / "missing").wstring());
    require(missing.children.empty(), "missing directory should have no children");
    require(!missing.ok(), "missing directory should preserve failure status");
    require(missing.error != ERROR_SUCCESS, "missing directory should expose Win32 error");

    testFileCreationHelpers();
}

int main() {
    try {
        runTests();
        std::cout << "DirectoryLoaderTests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
}

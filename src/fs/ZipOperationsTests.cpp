#include "fs/ZipOperations.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include <windows.h>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

std::filesystem::path tempDirectory(const wchar_t* name) {
    wchar_t tempPath[MAX_PATH]{};
    const DWORD length = GetTempPathW(static_cast<DWORD>(std::size(tempPath)), tempPath);
    require(length > 0 && length < std::size(tempPath), "temp path should be available");
    const std::filesystem::path path = std::filesystem::path(tempPath) / (std::wstring(name) + std::to_wstring(GetTickCount64()));
    std::filesystem::create_directories(path);
    return path;
}

void testZipExtensionIsCaseInsensitive() {
    require(isZipArchivePath(L"C:\\Temp\\archive.zip"), "lowercase zip extension should match");
    require(isZipArchivePath(L"C:\\Temp\\archive.ZIP"), "uppercase zip extension should match");
    require(!isZipArchivePath(L"C:\\Temp\\archive.zip.tmp"), "non-zip extension should not match");
}

void testDefaultArchivePathUsesSelectedName() {
    const std::filesystem::path root = tempDirectory(L"finderx-zip-name-");
    const std::wstring current = root.wstring();
    const std::filesystem::path source = root / L"report.txt";
    std::ofstream(source) << "x";

    const std::vector<std::wstring> sources{source.wstring()};
    const std::wstring archive = defaultZipArchivePath(current, sources);
    require(archive == (root / L"report.zip").wstring(), "single file archive should use file stem");

    std::filesystem::remove_all(root);
}

void testDefaultArchivePathAvoidsExistingFiles() {
    const std::filesystem::path root = tempDirectory(L"finderx-zip-existing-");
    const std::wstring current = root.wstring();
    std::ofstream(root / L"Archive.zip") << "x";

    const std::vector<std::wstring> sources{
        (root / L"one.txt").wstring(),
        (root / L"two.txt").wstring(),
    };
    const std::wstring archive = defaultZipArchivePath(current, sources);
    require(archive == (root / L"Archive 2.zip").wstring(), "multi-select archive should avoid existing Archive.zip");

    std::filesystem::remove_all(root);
}

void testDefaultExtractDirectoryUsesZipStem() {
    const std::filesystem::path root = tempDirectory(L"finderx-zip-extract-");
    const std::wstring current = root.wstring();
    std::filesystem::create_directory(root / L"backup");

    const std::wstring target = defaultZipExtractDirectory(current, (root / L"backup.zip").wstring());
    require(target == (root / L"backup 2").wstring(), "extract directory should avoid existing stem directory");

    std::filesystem::remove_all(root);
}

void testDefaultExtractDirectoryUsesZipParentDirectory() {
    const std::filesystem::path root = tempDirectory(L"finderx-zip-extract-parent-");
    const std::filesystem::path nested = root / L"nested";
    std::filesystem::create_directories(nested);

    const std::wstring target = defaultZipExtractDirectory(root.wstring(), (nested / L"backup.zip").wstring());
    require(target == (nested / L"backup").wstring(), "extract directory should use zip parent directory");

    std::filesystem::remove_all(root);
}

void testCompressAndExtractRoundTrip() {
    const std::filesystem::path root = tempDirectory(L"finderx-zip-roundtrip-");
    const std::wstring current = root.wstring();
    const std::filesystem::path source = root / L"hello.txt";
    std::ofstream(source) << "hello";

    const std::vector<std::wstring> sources{source.wstring()};
    const ZipOperationResult compress = compressToZip(current, sources);
    require(compress.success, "compressToZip should create archive with tar.exe");
    require(std::filesystem::exists(compress.outputPath), "compressToZip should create output zip");

    std::filesystem::remove(source);
    const ZipOperationResult extract = extractZipHere(current, compress.outputPath);
    require(extract.success, "extractZipHere should extract archive with tar.exe");
    require(std::filesystem::exists(std::filesystem::path(extract.outputPath) / L"hello.txt"),
            "extracted directory should contain archived file");

    std::filesystem::remove_all(root);
}

} // namespace

int main() {
    testZipExtensionIsCaseInsensitive();
    testDefaultArchivePathUsesSelectedName();
    testDefaultArchivePathAvoidsExistingFiles();
    testDefaultExtractDirectoryUsesZipStem();
    testDefaultExtractDirectoryUsesZipParentDirectory();
    testCompressAndExtractRoundTrip();
    std::cout << "ZipOperationsTests passed\n";
}

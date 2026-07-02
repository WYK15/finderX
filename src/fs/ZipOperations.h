#pragma once

#include <span>
#include <string>
#include <vector>

namespace finderx {

struct ZipOperationResult {
    bool success = false;
    std::wstring outputPath;
    std::wstring message;
};

bool isZipArchivePath(const std::wstring& path);
std::wstring defaultZipArchivePath(const std::wstring& currentDirectory, std::span<const std::wstring> sourcePaths);
std::wstring defaultZipExtractDirectory(const std::wstring& currentDirectory, const std::wstring& zipPath);
ZipOperationResult compressToZip(const std::wstring& currentDirectory, std::span<const std::wstring> sourcePaths);
ZipOperationResult extractZipHere(const std::wstring& currentDirectory, const std::wstring& zipPath);

} // namespace finderx

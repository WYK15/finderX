#include "fs/FileMetadata.h"

#include <iomanip>
#include <sstream>

namespace finderx {

std::wstring formatFileSize(unsigned long long bytes, bool isDirectory) {
    if (isDirectory) {
        return L"--";
    }

    constexpr unsigned long long kib = 1024;
    constexpr unsigned long long mib = kib * 1024;
    constexpr unsigned long long gib = mib * 1024;

    std::wostringstream out;
    if (bytes >= gib) {
        out << std::fixed << std::setprecision(1)
            << static_cast<double>(bytes) / static_cast<double>(gib) << L" GB";
    } else if (bytes >= mib) {
        out << std::fixed << std::setprecision(1)
            << static_cast<double>(bytes) / static_cast<double>(mib) << L" MB";
    } else if (bytes >= kib) {
        out << ((bytes + kib - 1) / kib) << L" KB";
    } else {
        out << bytes << L" bytes";
    }
    return out.str();
}

std::wstring formatFileTime(const FILETIME& fileTime) {
    SYSTEMTIME utcTime{};
    SYSTEMTIME localTime{};
    if (!FileTimeToSystemTime(&fileTime, &utcTime) ||
        !SystemTimeToTzSpecificLocalTime(nullptr, &utcTime, &localTime)) {
        return L"";
    }

    std::wostringstream out;
    out << localTime.wYear << L"/"
        << std::setw(2) << std::setfill(L'0') << localTime.wMonth << L"/"
        << std::setw(2) << std::setfill(L'0') << localTime.wDay << L" "
        << std::setw(2) << std::setfill(L'0') << localTime.wHour << L":"
        << std::setw(2) << std::setfill(L'0') << localTime.wMinute;
    return out.str();
}

std::wstring kindTextForAttributes(DWORD attributes) {
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) ? L"Folder" : L"File";
}

bool isSkippableDirectoryEntry(std::wstring_view name) {
    return name == L"." || name == L"..";
}

}

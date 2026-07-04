#include "ui/FileIconResolver.h"

#include <algorithm>
#include <cwctype>
#include <string>
#include <string_view>

namespace finderx {

namespace {

std::wstring lowercase(std::wstring_view text) {
    std::wstring result;
    result.reserve(text.size());
    for (wchar_t ch : text) {
        result.push_back(static_cast<wchar_t>(std::towlower(ch)));
    }
    return result;
}

std::wstring extensionOf(std::wstring_view name) {
    const std::size_t slash = name.find_last_of(L"\\/");
    const std::size_t dot = name.find_last_of(L'.');
    if (dot == std::wstring_view::npos || dot == 0 || (slash != std::wstring_view::npos && dot < slash)) {
        return {};
    }
    return lowercase(name.substr(dot + 1));
}

bool contains(std::initializer_list<std::wstring_view> values, std::wstring_view target) {
    return std::find(values.begin(), values.end(), target) != values.end();
}

} // namespace

FileIconKind resolveFileIconKind(const FileNode& node) {
    return resolveFileIconKind(node.kind, node.name);
}

FileIconKind resolveFileIconKind(FileKind kind, std::wstring_view name) {
    if (kind == FileKind::Folder) {
        return FileIconKind::Folder;
    }

    const std::wstring extension = extensionOf(name);
    if (extension.empty()) {
        return FileIconKind::Document;
    }

    if (contains({L"png", L"jpg", L"jpeg", L"gif", L"bmp", L"webp", L"ico", L"tif", L"tiff", L"svg"}, extension)) {
        return FileIconKind::Image;
    }
    if (contains({L"mp4", L"mkv", L"mov", L"avi", L"wmv", L"webm", L"m4v"}, extension)) {
        return FileIconKind::Video;
    }
    if (contains({L"mp3", L"wav", L"flac", L"aac", L"ogg", L"m4a", L"wma"}, extension)) {
        return FileIconKind::Audio;
    }
    if (contains({L"zip", L"7z", L"rar", L"tar", L"gz", L"bz2", L"xz"}, extension)) {
        return FileIconKind::Archive;
    }
    if (contains({L"cpp", L"c", L"h", L"hpp", L"cs", L"js", L"jsx", L"ts", L"tsx", L"py", L"rs", L"go", L"java", L"zig", L"html", L"css", L"json", L"xml", L"yml", L"yaml", L"md", L"cmake"}, extension)) {
        return FileIconKind::Code;
    }
    if (extension == L"pdf") {
        return FileIconKind::Pdf;
    }
    if (contains({L"exe", L"msi", L"bat", L"cmd", L"ps1", L"com"}, extension)) {
        return FileIconKind::Executable;
    }

    return FileIconKind::Document;
}

} // namespace finderx

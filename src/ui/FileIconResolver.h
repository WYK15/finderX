#pragma once

#include "model/FileNode.h"

#include <string_view>

namespace finderx {

enum class FileIconKind {
    Folder,
    Document,
    Image,
    Video,
    Audio,
    Archive,
    Code,
    Pdf,
    Executable,
};

FileIconKind resolveFileIconKind(const FileNode& node);
FileIconKind resolveFileIconKind(FileKind kind, std::wstring_view name);

} // namespace finderx

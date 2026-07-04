#include "ui/FileIconResolver.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

void testFoldersResolveToFolderIcon() {
    require(resolveFileIconKind(FileKind::Folder, L"photo.zip") == FileIconKind::Folder,
            "folders should always resolve to folder icon");
}

void testCommonExtensionsResolveToSpecializedIcons() {
    require(resolveFileIconKind(FileKind::File, L"photo.PNG") == FileIconKind::Image,
            "image extension should resolve case-insensitively");
    require(resolveFileIconKind(FileKind::File, L"movie.mp4") == FileIconKind::Video,
            "video extension should resolve to video");
    require(resolveFileIconKind(FileKind::File, L"song.flac") == FileIconKind::Audio,
            "audio extension should resolve to audio");
    require(resolveFileIconKind(FileKind::File, L"backup.7z") == FileIconKind::Archive,
            "archive extension should resolve to archive");
    require(resolveFileIconKind(FileKind::File, L"main.cpp") == FileIconKind::Code,
            "code extension should resolve to code");
    require(resolveFileIconKind(FileKind::File, L"manual.pdf") == FileIconKind::Pdf,
            "pdf extension should resolve to pdf");
    require(resolveFileIconKind(FileKind::File, L"tool.exe") == FileIconKind::Executable,
            "executable extension should resolve to executable");
}

void testUnknownFilesResolveToDocument() {
    require(resolveFileIconKind(FileKind::File, L"notes.unknown") == FileIconKind::Document,
            "unknown extension should resolve to document");
    require(resolveFileIconKind(FileKind::File, L"README") == FileIconKind::Document,
            "extensionless file should resolve to document");
}

} // namespace

int main() {
    testFoldersResolveToFolderIcon();
    testCommonExtensionsResolveToSpecializedIcons();
    testUnknownFilesResolveToDocument();
    std::cout << "FileIconResolverTests passed\n";
}

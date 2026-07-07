#include "ui/Localization.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    using finderx::ui::StringId;
    using finderx::ui::tr;

    require(tr(StringId::NewFolder) == L"新建文件夹", "new folder label should be localized");
    require(tr(StringId::NewFolder, finderx::LanguageMode::English) == L"New Folder", "new folder label should support English");
    require(tr(StringId::Language) == L"语言", "language setting label should be localized");
    require(tr(StringId::Language, finderx::LanguageMode::English) == L"Language", "language setting label should support English");
    require(tr(StringId::About) == L"关于", "about page label should be localized");
    require(tr(StringId::About, finderx::LanguageMode::English) == L"About", "about page label should support English");
    require(!tr(StringId::AppDescription).empty(), "about description should be localized");
    require(tr(StringId::OpenPowerShellHere) == L"在此处打开 PowerShell", "PowerShell toolbar label should be localized");
    require(tr(StringId::FinderXSettings) == L"FinderX 设置", "settings title should be localized");
    require(tr(StringId::MoveToTrash) == L"移到回收站", "trash command should be localized");

    std::cout << "LocalizationTests passed\n";
    return 0;
}

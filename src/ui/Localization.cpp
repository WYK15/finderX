#include "ui/Localization.h"

namespace finderx::ui {

namespace {

std::wstring_view trZh(StringId id) {
    switch (id) {
    case StringId::NewFolder: return L"新建文件夹";
    case StringId::NewFile: return L"新建文件";
    case StringId::Sort: return L"排序";
    case StringId::Settings: return L"设置";
    case StringId::OpenPowerShellHere: return L"在此处打开 PowerShell";
    case StringId::Search: return L"搜索";
    case StringId::FinderXSettings: return L"FinderX 设置";
    case StringId::SettingsSubtitle: return L"配置外观、浏览行为、启动窗口、收藏夹、工具栏和快捷键。";
    case StringId::Appearance: return L"外观";
    case StringId::Startup: return L"启动";
    case StringId::Files: return L"文件";
    case StringId::Shortcuts: return L"快捷键";
    case StringId::Toolbar: return L"工具栏";
    case StringId::ContextMenu: return L"右键菜单";
    case StringId::FontFamily: return L"字体";
    case StringId::FontSize: return L"字体大小";
    case StringId::ContextMenuFont: return L"右键菜单字体";
    case StringId::TextSize: return L"文字大小";
    case StringId::MenuTextSize: return L"菜单文字大小";
    case StringId::IconSize: return L"图标大小";
    case StringId::ItemPadding: return L"项目间距";
    case StringId::Theme: return L"主题";
    case StringId::Language: return L"语言";
    case StringId::Chinese: return L"中文";
    case StringId::English: return L"English";
    case StringId::Light: return L"浅色";
    case StringId::Dark: return L"深色";
    case StringId::WindowStartup: return L"窗口与启动";
    case StringId::FileBrowsing: return L"文件浏览";
    case StringId::WindowWidth: return L"窗口宽度";
    case StringId::WindowHeight: return L"窗口高度";
    case StringId::Width: return L"宽度";
    case StringId::Height: return L"高度";
    case StringId::RememberWindowSize: return L"记住窗口大小";
    case StringId::StartupFolder: return L"启动文件夹";
    case StringId::UseCurrentFolder: return L"使用当前文件夹";
    case StringId::UseCurrent: return L"使用当前";
    case StringId::FileBehavior: return L"文件行为";
    case StringId::ShowHiddenAndSystem: return L"显示隐藏和系统项目";
    case StringId::Favorites: return L"收藏夹";
    case StringId::FavoritesDescription: return L"调整侧边栏收藏夹顺序和名称。";
    case StringId::FavoriteName: return L"收藏名称";
    case StringId::DisplayName: return L"显示名称";
    case StringId::TargetPath: return L"目标路径";
    case StringId::Keyboard: return L"键盘";
    case StringId::AvailableActions: return L"可用操作";
    case StringId::ToolbarOrder: return L"工具栏顺序";
    case StringId::Add: return L"添加";
    case StringId::Remove: return L"移除";
    case StringId::MoveUp: return L"上移";
    case StringId::MoveDown: return L"下移";
    case StringId::Up: return L"上移";
    case StringId::Down: return L"下移";
    case StringId::Ok: return L"确定";
    case StringId::Cancel: return L"取消";
    case StringId::MenuLabel: return L"菜单名称";
    case StringId::ProgramPath: return L"程序路径";
    case StringId::Browse: return L"浏览...";
    case StringId::Arguments: return L"参数";
    case StringId::ShowForFiles: return L"用于文件";
    case StringId::ShowForFolders: return L"用于文件夹";
    case StringId::VsCode: return L"VS Code";
    case StringId::Update: return L"更新";
    case StringId::ChooseProgram: return L"选择程序";
    case StringId::CreateShortcut: return L"创建快捷方式";
    case StringId::OpenInVsCode: return L"在 VS Code 中打开";
    case StringId::Open: return L"打开";
    case StringId::Rename: return L"重命名";
    case StringId::Copy: return L"复制";
    case StringId::Cut: return L"剪切";
    case StringId::Paste: return L"粘贴";
    case StringId::MoveToTrash: return L"移到回收站";
    case StringId::CompressZip: return L"压缩为 ZIP";
    case StringId::ExtractHere: return L"解压到此处";
    case StringId::OpenInNewTab: return L"在新标签页打开";
    case StringId::AddToFavorites: return L"添加到收藏夹";
    case StringId::RemoveFromFavorites: return L"从收藏夹移除";
    case StringId::RevealInExplorer: return L"在资源管理器中显示";
    case StringId::CopyPath: return L"复制路径";
    case StringId::Refresh: return L"刷新";
    case StringId::CannotOpenPowerShell: return L"无法打开 PowerShell";
    case StringId::CannotOpenFolder: return L"无法打开文件夹";
    case StringId::CannotOpenFile: return L"无法打开文件";
    case StringId::CannotRefreshFolder: return L"无法刷新文件夹";
    case StringId::CannotShowInExplorer: return L"无法在资源管理器中显示";
    case StringId::CannotCopyPath: return L"无法复制路径";
    case StringId::CannotOpenExternalTool: return L"无法打开外部工具";
    }
    return L"";
}

std::wstring_view trEn(StringId id) {
    switch (id) {
    case StringId::NewFolder: return L"New Folder";
    case StringId::NewFile: return L"New File";
    case StringId::Sort: return L"Sort";
    case StringId::Settings: return L"Settings";
    case StringId::OpenPowerShellHere: return L"Open PowerShell Here";
    case StringId::Search: return L"Search";
    case StringId::FinderXSettings: return L"FinderX Settings";
    case StringId::SettingsSubtitle: return L"Configure appearance, browsing behavior, startup, favorites, toolbar, and shortcuts.";
    case StringId::Appearance: return L"Appearance";
    case StringId::Startup: return L"Startup";
    case StringId::Files: return L"Files";
    case StringId::Shortcuts: return L"Shortcuts";
    case StringId::Toolbar: return L"Toolbar";
    case StringId::ContextMenu: return L"Context Menu";
    case StringId::FontFamily: return L"Font family";
    case StringId::FontSize: return L"Font size";
    case StringId::ContextMenuFont: return L"Context menu font";
    case StringId::TextSize: return L"Text size";
    case StringId::MenuTextSize: return L"Menu text size";
    case StringId::IconSize: return L"Icon size";
    case StringId::ItemPadding: return L"Item padding";
    case StringId::Theme: return L"Theme";
    case StringId::Language: return L"Language";
    case StringId::Chinese: return L"中文";
    case StringId::English: return L"English";
    case StringId::Light: return L"Light";
    case StringId::Dark: return L"Dark";
    case StringId::WindowStartup: return L"Window and startup";
    case StringId::FileBrowsing: return L"File Browsing";
    case StringId::WindowWidth: return L"Window width";
    case StringId::WindowHeight: return L"Window height";
    case StringId::Width: return L"Width";
    case StringId::Height: return L"Height";
    case StringId::RememberWindowSize: return L"Remember last window size";
    case StringId::StartupFolder: return L"Startup folder";
    case StringId::UseCurrentFolder: return L"Use current folder";
    case StringId::UseCurrent: return L"Use current";
    case StringId::FileBehavior: return L"File behavior";
    case StringId::ShowHiddenAndSystem: return L"Show hidden and system items";
    case StringId::Favorites: return L"Favorites";
    case StringId::FavoritesDescription: return L"Order and rename sidebar favorites.";
    case StringId::FavoriteName: return L"Favorite name";
    case StringId::DisplayName: return L"Display name";
    case StringId::TargetPath: return L"Target path";
    case StringId::Keyboard: return L"Keyboard";
    case StringId::AvailableActions: return L"Available actions";
    case StringId::ToolbarOrder: return L"Toolbar order";
    case StringId::Add: return L"Add";
    case StringId::Remove: return L"Remove";
    case StringId::MoveUp: return L"Move up";
    case StringId::MoveDown: return L"Move down";
    case StringId::Up: return L"Up";
    case StringId::Down: return L"Down";
    case StringId::Ok: return L"OK";
    case StringId::Cancel: return L"Cancel";
    case StringId::MenuLabel: return L"Menu label";
    case StringId::ProgramPath: return L"Program path";
    case StringId::Browse: return L"Browse...";
    case StringId::Arguments: return L"Arguments";
    case StringId::ShowForFiles: return L"Show for files";
    case StringId::ShowForFolders: return L"Show for folders";
    case StringId::VsCode: return L"VS Code";
    case StringId::Update: return L"Update";
    case StringId::ChooseProgram: return L"Choose Program";
    case StringId::CreateShortcut: return L"Create Shortcut";
    case StringId::OpenInVsCode: return L"Open in VS Code";
    case StringId::Open: return L"Open";
    case StringId::Rename: return L"Rename";
    case StringId::Copy: return L"Copy";
    case StringId::Cut: return L"Cut";
    case StringId::Paste: return L"Paste";
    case StringId::MoveToTrash: return L"Move to Trash";
    case StringId::CompressZip: return L"Compress to ZIP";
    case StringId::ExtractHere: return L"Extract Here";
    case StringId::OpenInNewTab: return L"Open in New Tab";
    case StringId::AddToFavorites: return L"Add to Favorites";
    case StringId::RemoveFromFavorites: return L"Remove from Favorites";
    case StringId::RevealInExplorer: return L"Show in Explorer";
    case StringId::CopyPath: return L"Copy Path";
    case StringId::Refresh: return L"Refresh";
    case StringId::CannotOpenPowerShell: return L"Cannot open PowerShell";
    case StringId::CannotOpenFolder: return L"Cannot open folder";
    case StringId::CannotOpenFile: return L"Cannot open file";
    case StringId::CannotRefreshFolder: return L"Cannot refresh folder";
    case StringId::CannotShowInExplorer: return L"Cannot show in Explorer";
    case StringId::CannotCopyPath: return L"Cannot copy path";
    case StringId::CannotOpenExternalTool: return L"Cannot open external tool";
    }
    return L"";
}

} // namespace

std::wstring_view tr(StringId id, LanguageMode languageMode) {
    return languageMode == LanguageMode::English ? trEn(id) : trZh(id);
}

std::wstring_view tr(StringId id) {
    return tr(id, LanguageMode::Chinese);
}

} // namespace finderx::ui

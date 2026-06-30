#pragma once

#include <string>
#include <vector>

namespace finderx::ui {

enum class PendingFileOperationKind { None, Copy, Cut };

class FileOperationState {
public:
    void setCopy(std::wstring path);
    void setCopy(std::vector<std::wstring> paths);
    void setCut(std::wstring path);
    void setCut(std::vector<std::wstring> paths);
    void clear();
    void markPasteSucceeded();
    bool hasPendingOperation() const;
    PendingFileOperationKind kind() const;
    const std::wstring& sourcePath() const;
    const std::vector<std::wstring>& sourcePaths() const;

private:
    PendingFileOperationKind kind_ = PendingFileOperationKind::None;
    std::vector<std::wstring> sourcePaths_;
};

} // namespace finderx::ui

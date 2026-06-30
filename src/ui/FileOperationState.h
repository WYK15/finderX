#pragma once

#include <string>

namespace finderx::ui {

enum class PendingFileOperationKind { None, Copy, Cut };

class FileOperationState {
public:
    void setCopy(std::wstring path);
    void setCut(std::wstring path);
    void clear();
    void markPasteSucceeded();
    bool hasPendingOperation() const;
    PendingFileOperationKind kind() const;
    const std::wstring& sourcePath() const;

private:
    PendingFileOperationKind kind_ = PendingFileOperationKind::None;
    std::wstring sourcePath_;
};

} // namespace finderx::ui

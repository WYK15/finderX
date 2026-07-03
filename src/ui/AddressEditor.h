#pragma once

#include <cstddef>
#include <string>

namespace finderx::ui {

class AddressEditor {
public:
    void begin(std::wstring text);
    void clear();
    bool active() const;

    const std::wstring& text() const;
    std::size_t caret() const;
    std::size_t selectionStart() const;
    std::size_t selectionEnd() const;
    bool hasSelection() const;
    bool caretVisible() const;
    void setCaretVisible(bool visible);
    void toggleCaretVisible();

    void selectAll();
    void moveCaret(std::size_t index, bool selecting);
    void insertText(std::wstring_view text);
    void backspace();

private:
    void clampSelection();

    bool active_ = false;
    std::wstring text_;
    std::size_t caret_ = 0;
    std::size_t anchor_ = 0;
    bool caretVisible_ = false;
};

} // namespace finderx::ui

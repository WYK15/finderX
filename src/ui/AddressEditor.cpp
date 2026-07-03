#include "ui/AddressEditor.h"

#include <algorithm>

namespace finderx::ui {

void AddressEditor::begin(std::wstring text) {
    active_ = true;
    text_ = std::move(text);
    caret_ = text_.size();
    anchor_ = caret_;
    caretVisible_ = true;
}

void AddressEditor::clear() {
    active_ = false;
    text_.clear();
    caret_ = 0;
    anchor_ = 0;
    caretVisible_ = false;
}

bool AddressEditor::active() const {
    return active_;
}

const std::wstring& AddressEditor::text() const {
    return text_;
}

std::size_t AddressEditor::caret() const {
    return caret_;
}

std::size_t AddressEditor::selectionStart() const {
    return (std::min)(caret_, anchor_);
}

std::size_t AddressEditor::selectionEnd() const {
    return (std::max)(caret_, anchor_);
}

bool AddressEditor::hasSelection() const {
    return selectionStart() < selectionEnd();
}

bool AddressEditor::caretVisible() const {
    return caretVisible_;
}

void AddressEditor::setCaretVisible(bool visible) {
    caretVisible_ = visible;
}

void AddressEditor::toggleCaretVisible() {
    caretVisible_ = !caretVisible_;
}

void AddressEditor::selectAll() {
    anchor_ = 0;
    caret_ = text_.size();
    caretVisible_ = true;
}

void AddressEditor::moveCaret(std::size_t index, bool selecting) {
    index = (std::min)(index, text_.size());
    if (!selecting) {
        anchor_ = index;
    }
    caret_ = index;
    caretVisible_ = true;
}

void AddressEditor::moveLeft(bool selecting) {
    if (!selecting && hasSelection()) {
        caret_ = selectionStart();
        anchor_ = caret_;
        caretVisible_ = true;
        return;
    }

    if (caret_ > 0) {
        --caret_;
    }
    if (!selecting) {
        anchor_ = caret_;
    }
    caretVisible_ = true;
}

void AddressEditor::moveRight(bool selecting) {
    if (!selecting && hasSelection()) {
        caret_ = selectionEnd();
        anchor_ = caret_;
        caretVisible_ = true;
        return;
    }

    if (caret_ < text_.size()) {
        ++caret_;
    }
    if (!selecting) {
        anchor_ = caret_;
    }
    caretVisible_ = true;
}

void AddressEditor::insertText(std::wstring_view text) {
    const std::size_t start = selectionStart();
    const std::size_t end = selectionEnd();
    text_.replace(start, end - start, text);
    caret_ = start + text.size();
    anchor_ = caret_;
    caretVisible_ = true;
}

void AddressEditor::backspace() {
    if (hasSelection()) {
        const std::size_t start = selectionStart();
        const std::size_t end = selectionEnd();
        text_.erase(start, end - start);
        caret_ = start;
        anchor_ = caret_;
        caretVisible_ = true;
        return;
    }

    if (caret_ > 0) {
        text_.erase(caret_ - 1, 1);
        --caret_;
        anchor_ = caret_;
    }
    caretVisible_ = true;
}

void AddressEditor::clampSelection() {
    caret_ = (std::min)(caret_, text_.size());
    anchor_ = (std::min)(anchor_, text_.size());
}

} // namespace finderx::ui

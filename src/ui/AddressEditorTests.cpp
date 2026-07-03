#include "ui/AddressEditor.h"

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
    using finderx::ui::AddressEditor;

    {
        AddressEditor editor;
        editor.begin(L"D:\\alpha\\beta");
        editor.selectAll();
        require(editor.hasSelection(), "ctrl+a should select the address text");
        require(editor.selectionStart() == 0, "selection should start at the beginning");
        require(editor.selectionEnd() == editor.text().size(), "selection should end at the text length");
    }

    {
        AddressEditor editor;
        editor.begin(L"D:\\alpha\\beta");
        editor.selectAll();
        editor.insertText(L"C:\\Users");
        require(editor.text() == L"C:\\Users", "typed or pasted text should replace selection");
        require(!editor.hasSelection(), "replace should clear selection");
        require(editor.caret() == editor.text().size(), "replace should put caret after inserted text");
    }

    {
        AddressEditor editor;
        editor.begin(L"D:\\alpha\\beta");
        editor.moveCaret(3, false);
        editor.moveCaret(8, true);
        require(editor.hasSelection(), "mouse drag should create a selection range");
        require(editor.selectionStart() == 3, "drag selection should keep the lower endpoint");
        require(editor.selectionEnd() == 8, "drag selection should keep the upper endpoint");
        editor.backspace();
        require(editor.text() == L"D:\\\\beta", "backspace should delete the selected range");
        require(!editor.hasSelection(), "backspace should clear selection");
    }

    {
        AddressEditor editor;
        editor.begin(L"D:\\alpha");
        require(editor.caretVisible(), "starting edit should show caret");
        editor.toggleCaretVisible();
        require(!editor.caretVisible(), "caret should toggle off");
        editor.toggleCaretVisible();
        require(editor.caretVisible(), "caret should toggle on");
    }

    std::cout << "AddressEditorTests passed\n";
    return 0;
}

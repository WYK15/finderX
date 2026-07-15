#include "app/App.h"

#include "ui/MainWindow.h"

#include <ole2.h>

namespace finderx {

int App::run(HINSTANCE instance, int showCommand) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const HRESULT oleResult = OleInitialize(nullptr);
    if (FAILED(oleResult)) {
        return 1;
    }

    MainWindow window;
    if (!window.create(instance, showCommand)) {
        OleUninitialize();
        return 1;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    OleUninitialize();
    return static_cast<int>(message.wParam);
}

}

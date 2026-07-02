set(required_files
    "${CMAKE_CURRENT_LIST_DIR}/../resources/FinderX.ico"
    "${CMAKE_CURRENT_LIST_DIR}/../resources/FinderX.rc"
    "${CMAKE_CURRENT_LIST_DIR}/../resources/resource.h"
)

foreach(required_file IN LISTS required_files)
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Missing app resource: ${required_file}")
    endif()
endforeach()

file(READ "${CMAKE_CURRENT_LIST_DIR}/../resources/FinderX.rc" finderx_rc)
if(NOT finderx_rc MATCHES "IDI_FINDERX[ \t]+ICON[ \t]+\"FinderX.ico\"")
    message(FATAL_ERROR "FinderX.rc does not bind IDI_FINDERX to FinderX.ico")
endif()

file(READ "${CMAKE_CURRENT_LIST_DIR}/../src/ui/MainWindow.cpp" main_window_cpp)
if(NOT main_window_cpp MATCHES "IDI_FINDERX")
    message(FATAL_ERROR "MainWindow.cpp does not load the FinderX app icon")
endif()

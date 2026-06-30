cmake_minimum_required(VERSION 3.24)

project(FinderX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

function(finderx_enable_utf8 target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8)
    endif()
endfunction()

function(finderx_enable_windows_unicode target)
    target_compile_definitions(${target} PRIVATE UNICODE _UNICODE)
endfunction()

add_library(finderx_model
    src/model/FileTree.cpp
)

target_include_directories(finderx_model PUBLIC src)
finderx_enable_utf8(finderx_model)

add_library(finderx_settings
    src/settings/AppSettings.cpp
)

target_include_directories(finderx_settings PUBLIC src)
finderx_enable_utf8(finderx_settings)
finderx_enable_windows_unicode(finderx_settings)

add_library(finderx_fs
    src/fs/FileMetadata.cpp
    src/fs/DirectoryLoader.cpp
    src/fs/FileSorter.cpp
    src/fs/FileCreation.cpp
    src/fs/DriveEnumerator.cpp
)

target_include_directories(finderx_fs PUBLIC src)
target_link_libraries(finderx_fs PUBLIC finderx_model finderx_settings)
finderx_enable_utf8(finderx_fs)
finderx_enable_windows_unicode(finderx_fs)

add_library(finderx_navigation
    src/navigation/NavigationHistory.cpp
    src/navigation/SidebarModel.cpp
)

target_include_directories(finderx_navigation PUBLIC src)
target_link_libraries(finderx_navigation PUBLIC finderx_fs)
finderx_enable_utf8(finderx_navigation)
finderx_enable_windows_unicode(finderx_navigation)

add_executable(finderx_model_tests
    src/model/FileTreeTests.cpp
)

target_link_libraries(finderx_model_tests PRIVATE finderx_model)
finderx_enable_utf8(finderx_model_tests)
add_test(NAME finderx_model_tests COMMAND finderx_model_tests)

add_executable(finderx_fs_tests
    src/fs/DirectoryLoaderTests.cpp
)

target_link_libraries(finderx_fs_tests PRIVATE finderx_fs)
finderx_enable_utf8(finderx_fs_tests)
finderx_enable_windows_unicode(finderx_fs_tests)
add_test(NAME finderx_fs_tests COMMAND finderx_fs_tests)

add_executable(finderx_drive_enumerator_tests
    src/fs/DriveEnumeratorTests.cpp
)

target_link_libraries(finderx_drive_enumerator_tests PRIVATE finderx_fs)
finderx_enable_utf8(finderx_drive_enumerator_tests)
finderx_enable_windows_unicode(finderx_drive_enumerator_tests)
add_test(NAME finderx_drive_enumerator_tests COMMAND finderx_drive_enumerator_tests)

add_executable(finderx_file_sorter_tests
    src/fs/FileSorterTests.cpp
)

target_link_libraries(finderx_file_sorter_tests PRIVATE finderx_fs finderx_settings)
finderx_enable_utf8(finderx_file_sorter_tests)
finderx_enable_windows_unicode(finderx_file_sorter_tests)
add_test(NAME finderx_file_sorter_tests COMMAND finderx_file_sorter_tests)

add_executable(finderx_navigation_tests
    src/navigation/NavigationHistoryTests.cpp
)

target_link_libraries(finderx_navigation_tests PRIVATE finderx_navigation)
finderx_enable_utf8(finderx_navigation_tests)
finderx_enable_windows_unicode(finderx_navigation_tests)
add_test(NAME finderx_navigation_tests COMMAND finderx_navigation_tests)

add_executable(finderx_settings_tests
    src/settings/AppSettingsTests.cpp
)

target_link_libraries(finderx_settings_tests PRIVATE finderx_settings)
finderx_enable_utf8(finderx_settings_tests)
finderx_enable_windows_unicode(finderx_settings_tests)
add_test(NAME finderx_settings_tests COMMAND finderx_settings_tests)

add_executable(finderx_rename_dialog_tests
    src/ui/RenameDialogTests.cpp
    src/ui/RenameDialog.cpp
)

target_include_directories(finderx_rename_dialog_tests PRIVATE src)
finderx_enable_utf8(finderx_rename_dialog_tests)
finderx_enable_windows_unicode(finderx_rename_dialog_tests)
add_test(NAME finderx_rename_dialog_tests COMMAND finderx_rename_dialog_tests)

add_executable(finderx_file_operation_state_tests
    src/ui/FileOperationStateTests.cpp
    src/ui/FileOperationState.cpp
)

target_include_directories(finderx_file_operation_state_tests PRIVATE src)
finderx_enable_utf8(finderx_file_operation_state_tests)
finderx_enable_windows_unicode(finderx_file_operation_state_tests)
add_test(NAME finderx_file_operation_state_tests COMMAND finderx_file_operation_state_tests)

add_executable(finderx_tab_manager_tests
    src/ui/TabManagerTests.cpp
)

target_include_directories(finderx_tab_manager_tests PRIVATE src)
finderx_enable_utf8(finderx_tab_manager_tests)
finderx_enable_windows_unicode(finderx_tab_manager_tests)
add_test(NAME finderx_tab_manager_tests COMMAND finderx_tab_manager_tests)

add_executable(finderx_directory_refresh_debouncer_tests
    src/ui/DirectoryRefreshDebouncerTests.cpp
)

target_include_directories(finderx_directory_refresh_debouncer_tests PRIVATE src)
finderx_enable_utf8(finderx_directory_refresh_debouncer_tests)
finderx_enable_windows_unicode(finderx_directory_refresh_debouncer_tests)
add_test(NAME finderx_directory_refresh_debouncer_tests COMMAND finderx_directory_refresh_debouncer_tests)

add_executable(finderx_dpi_scaling_tests
    src/ui/DpiScalingTests.cpp
)

target_include_directories(finderx_dpi_scaling_tests PRIVATE src)
target_link_libraries(finderx_dpi_scaling_tests PRIVATE d2d1)
finderx_enable_utf8(finderx_dpi_scaling_tests)
finderx_enable_windows_unicode(finderx_dpi_scaling_tests)
add_test(NAME finderx_dpi_scaling_tests COMMAND finderx_dpi_scaling_tests)

add_executable(finderx_list_view_tests
    src/ui/FinderListViewTests.cpp
    src/ui/FinderListView.cpp
    src/ui/RenderContext.cpp
)

target_include_directories(finderx_list_view_tests PRIVATE src)
target_link_libraries(finderx_list_view_tests PRIVATE finderx_model d2d1 dwrite windowscodecs)
finderx_enable_utf8(finderx_list_view_tests)
finderx_enable_windows_unicode(finderx_list_view_tests)
add_test(NAME finderx_list_view_tests COMMAND finderx_list_view_tests)

set(FINDERX_APP_SOURCES
    src/main.cpp
    src/app/App.cpp
    src/shell/ShellActions.cpp
    src/shell/ShellFileOperations.cpp
    src/ui/FileOperationState.cpp
    src/ui/FinderChrome.cpp
    src/ui/FinderListView.cpp
    src/ui/MainWindow.cpp
    src/ui/RenameDialog.cpp
    src/ui/RenderContext.cpp
)

set(FINDERX_APP_SOURCES_EXIST TRUE)
foreach(source IN LISTS FINDERX_APP_SOURCES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        set(FINDERX_APP_SOURCES_EXIST FALSE)
    endif()
endforeach()

if(FINDERX_APP_SOURCES_EXIST)
    add_executable(FinderX WIN32
        ${FINDERX_APP_SOURCES}
    )

    target_include_directories(FinderX PRIVATE src)
    target_link_libraries(FinderX PRIVATE
        finderx_model
        finderx_fs
        finderx_navigation
        d2d1
        dwrite
        windowscodecs
        ole32
        shell32
    )
    finderx_enable_utf8(FinderX)
    finderx_enable_windows_unicode(FinderX)
endif()

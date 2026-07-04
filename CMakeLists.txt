cmake_minimum_required(VERSION 3.24)

project(FinderX LANGUAGES CXX RC)

set(FINDERX_VERSION "0.1.0" CACHE STRING "FinderX package version")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_test(
    NAME finderx_app_resources
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyAppResources.cmake"
)

find_program(FINDERX_POWERSHELL NAMES pwsh powershell)
if(FINDERX_POWERSHELL)
    add_test(
        NAME finderx_icon_transparency
        COMMAND ${FINDERX_POWERSHELL}
            -NoProfile
            -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyIconTransparency.ps1"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/FinderX.png"
    )
endif()

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
    src/fs/ZipOperations.cpp
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

add_executable(finderx_zip_operations_tests
    src/fs/ZipOperationsTests.cpp
)

target_link_libraries(finderx_zip_operations_tests PRIVATE finderx_fs)
finderx_enable_utf8(finderx_zip_operations_tests)
finderx_enable_windows_unicode(finderx_zip_operations_tests)
add_test(NAME finderx_zip_operations_tests COMMAND finderx_zip_operations_tests)

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

add_executable(finderx_settings_dialog_tests
    src/ui/SettingsDialogTests.cpp
    src/ui/SettingsDialog.cpp
    src/ui/NativeTitleBar.cpp
    src/ui/ThemeTokens.cpp
)

target_include_directories(finderx_settings_dialog_tests PRIVATE src)
target_link_libraries(finderx_settings_dialog_tests PRIVATE finderx_settings dwmapi comdlg32)
finderx_enable_utf8(finderx_settings_dialog_tests)
finderx_enable_windows_unicode(finderx_settings_dialog_tests)
add_test(NAME finderx_settings_dialog_tests COMMAND finderx_settings_dialog_tests)

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

add_executable(finderx_drag_feedback_tests
    src/ui/DragFeedbackTests.cpp
    src/ui/DragFeedback.cpp
)

target_include_directories(finderx_drag_feedback_tests PRIVATE src)
finderx_enable_utf8(finderx_drag_feedback_tests)
finderx_enable_windows_unicode(finderx_drag_feedback_tests)
add_test(NAME finderx_drag_feedback_tests COMMAND finderx_drag_feedback_tests)

add_executable(finderx_theme_tokens_tests
    src/ui/ThemeTokensTests.cpp
    src/ui/ThemeTokens.cpp
)

target_include_directories(finderx_theme_tokens_tests PRIVATE src)
target_link_libraries(finderx_theme_tokens_tests PRIVATE d2d1)
finderx_enable_utf8(finderx_theme_tokens_tests)
finderx_enable_windows_unicode(finderx_theme_tokens_tests)
add_test(NAME finderx_theme_tokens_tests COMMAND finderx_theme_tokens_tests)

add_executable(finderx_file_icon_resolver_tests
    src/ui/FileIconResolverTests.cpp
    src/ui/FileIconResolver.cpp
)

target_include_directories(finderx_file_icon_resolver_tests PRIVATE src)
target_link_libraries(finderx_file_icon_resolver_tests PRIVATE finderx_model)
finderx_enable_utf8(finderx_file_icon_resolver_tests)
finderx_enable_windows_unicode(finderx_file_icon_resolver_tests)
add_test(NAME finderx_file_icon_resolver_tests COMMAND finderx_file_icon_resolver_tests)

add_executable(finderx_native_title_bar_tests
    src/ui/NativeTitleBarTests.cpp
    src/ui/NativeTitleBar.cpp
)

target_include_directories(finderx_native_title_bar_tests PRIVATE src)
target_link_libraries(finderx_native_title_bar_tests PRIVATE finderx_settings dwmapi)
finderx_enable_utf8(finderx_native_title_bar_tests)
finderx_enable_windows_unicode(finderx_native_title_bar_tests)
add_test(NAME finderx_native_title_bar_tests COMMAND finderx_native_title_bar_tests)

add_executable(finderx_toolbar_customization_tests
    src/ui/ToolbarCustomizationTests.cpp
    src/ui/ToolbarCustomization.cpp
)

target_include_directories(finderx_toolbar_customization_tests PRIVATE src)
target_link_libraries(finderx_toolbar_customization_tests PRIVATE finderx_settings)
finderx_enable_utf8(finderx_toolbar_customization_tests)
finderx_enable_windows_unicode(finderx_toolbar_customization_tests)
add_test(NAME finderx_toolbar_customization_tests COMMAND finderx_toolbar_customization_tests)

add_executable(finderx_address_editor_tests
    src/ui/AddressEditorTests.cpp
    src/ui/AddressEditor.cpp
)

target_include_directories(finderx_address_editor_tests PRIVATE src)
finderx_enable_utf8(finderx_address_editor_tests)
finderx_enable_windows_unicode(finderx_address_editor_tests)
add_test(NAME finderx_address_editor_tests COMMAND finderx_address_editor_tests)

add_executable(finderx_shell_actions_tests
    src/shell/ShellActionsTests.cpp
    src/shell/ShellActions.cpp
)

target_include_directories(finderx_shell_actions_tests PRIVATE src)
target_link_libraries(finderx_shell_actions_tests PRIVATE ole32 shell32)
finderx_enable_utf8(finderx_shell_actions_tests)
finderx_enable_windows_unicode(finderx_shell_actions_tests)
add_test(NAME finderx_shell_actions_tests COMMAND finderx_shell_actions_tests)

add_executable(finderx_dpi_scaling_tests
    src/ui/DpiScalingTests.cpp
)

target_include_directories(finderx_dpi_scaling_tests PRIVATE src)
target_link_libraries(finderx_dpi_scaling_tests PRIVATE d2d1)
finderx_enable_utf8(finderx_dpi_scaling_tests)
finderx_enable_windows_unicode(finderx_dpi_scaling_tests)
add_test(NAME finderx_dpi_scaling_tests COMMAND finderx_dpi_scaling_tests)

add_executable(finderx_render_context_tests
    src/ui/RenderContextTests.cpp
    src/ui/RenderContext.cpp
)

target_include_directories(finderx_render_context_tests PRIVATE src)
target_link_libraries(finderx_render_context_tests PRIVATE d2d1 dwrite)
finderx_enable_utf8(finderx_render_context_tests)
finderx_enable_windows_unicode(finderx_render_context_tests)
add_test(NAME finderx_render_context_tests COMMAND finderx_render_context_tests)

add_executable(finderx_list_view_tests
    src/ui/FinderListViewTests.cpp
    src/ui/FinderListView.cpp
    src/ui/FileIconResolver.cpp
    src/ui/ThemeTokens.cpp
    src/ui/RenderContext.cpp
)

target_include_directories(finderx_list_view_tests PRIVATE src)
target_link_libraries(finderx_list_view_tests PRIVATE finderx_model d2d1 dwrite windowscodecs)
finderx_enable_utf8(finderx_list_view_tests)
finderx_enable_windows_unicode(finderx_list_view_tests)
add_test(NAME finderx_list_view_tests COMMAND finderx_list_view_tests)

add_executable(finderx_chrome_tests
    src/ui/FinderChromeTests.cpp
    src/ui/FinderChrome.cpp
    src/ui/ThemeTokens.cpp
    src/ui/RenderContext.cpp
)

target_include_directories(finderx_chrome_tests PRIVATE src)
target_link_libraries(finderx_chrome_tests PRIVATE finderx_navigation d2d1 dwrite windowscodecs)
finderx_enable_utf8(finderx_chrome_tests)
finderx_enable_windows_unicode(finderx_chrome_tests)
add_test(NAME finderx_chrome_tests COMMAND finderx_chrome_tests)

set(FINDERX_APP_SOURCES
    src/main.cpp
    src/app/App.cpp
    src/shell/ShellActions.cpp
    src/shell/ShellFileOperations.cpp
    src/ui/AddressEditor.cpp
    src/ui/FileOperationState.cpp
    src/ui/DragFeedback.cpp
    src/ui/FileIconResolver.cpp
    src/ui/FinderChrome.cpp
    src/ui/FinderListView.cpp
    src/ui/MainWindow.cpp
    src/ui/NativeTitleBar.cpp
    src/ui/RenameDialog.cpp
    src/ui/SettingsDialog.cpp
    src/ui/ToolbarCustomization.cpp
    src/ui/ToolbarCustomizationDialog.cpp
    src/ui/ThemeTokens.cpp
    src/ui/RenderContext.cpp
    resources/FinderX.rc
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

    target_include_directories(FinderX PRIVATE src resources)
    target_link_libraries(FinderX PRIVATE
        finderx_model
        finderx_fs
        finderx_navigation
        finderx_settings
        d2d1
        dwrite
        windowscodecs
        ole32
        shell32
        comctl32
        dwmapi
        comdlg32
    )
    finderx_enable_utf8(FinderX)
    finderx_enable_windows_unicode(FinderX)

    install(TARGETS FinderX
        RUNTIME DESTINATION .
    )
endif()

set(CPACK_PACKAGE_NAME "FinderX")
set(CPACK_PACKAGE_VENDOR "FinderX")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "FinderX Windows file manager")
set(CPACK_PACKAGE_VERSION "${FINDERX_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "FinderX")
set(CPACK_PACKAGE_EXECUTABLES "FinderX" "FinderX")
set(CPACK_CREATE_DESKTOP_LINKS "FinderX")
set(CPACK_VERBATIM_VARIABLES YES)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt" COPYONLY)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")
endif()

set(CPACK_GENERATOR "WIX")
set(CPACK_WIX_VERSION 4)
set(CPACK_WIX_UPGRADE_GUID "48D5AA78-8EEF-4DC2-A2F3-EDA36CF1B7E4")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "FinderX")
set(CPACK_WIX_ARCHITECTURE "x64")
set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/FinderX.ico")
set(CPACK_WIX_EXTENSIONS WixToolset.UI.wixext)
set(CPACK_WIX_CUSTOM_XMLNS "ui=http://wixtoolset.org/schemas/v4/wxs/ui")

include(CPack)
